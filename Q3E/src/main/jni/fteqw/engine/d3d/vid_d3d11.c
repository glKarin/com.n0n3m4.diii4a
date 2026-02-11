#include "quakedef.h"
#ifdef D3D11QUAKE
#include "winquake.h"
#include "gl_draw.h"
#include "glquake.h"
#include "shader.h"
#include "renderque.h"
#include "resource.h"
#include "vr.h"

#define FUCKDXGI

#define COBJMACROS
#include <d3d11.h>

ID3D11Device *pD3DDev11;
ID3D11DeviceContext *d3ddevctx;

//#include <d3d11_1.h>
//ID3D11DeviceContext1 *d3ddevctx1;

#ifdef WINRT	//winrt crap has its own non-hwnd window crap, after years of microsoft forcing everyone to use hwnds for everything. I wonder why they don't have that many winrt apps.
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3D11.lib")
#include "dxgi1_2.h"
#else

/*Fixup outdated windows headers*/
#ifndef WM_XBUTTONDOWN
   #define WM_XBUTTONDOWN      0x020B
   #define WM_XBUTTONUP      0x020C
#endif
#ifndef MK_XBUTTON1
   #define MK_XBUTTON1         0x0020
#endif
#ifndef MK_XBUTTON2
   #define MK_XBUTTON2         0x0040
#endif
// copied from DarkPlaces in an attempt to grab more buttons
#ifndef MK_XBUTTON3
   #define MK_XBUTTON3         0x0080
#endif
#ifndef MK_XBUTTON4
   #define MK_XBUTTON4         0x0100
#endif
#ifndef MK_XBUTTON5
   #define MK_XBUTTON5         0x0200
#endif
#ifndef MK_XBUTTON6
   #define MK_XBUTTON6         0x0400
#endif
#ifndef MK_XBUTTON7
   #define MK_XBUTTON7         0x0800
#endif

#ifndef WM_INPUT
	#define WM_INPUT 255
#endif
#endif

#define DEFINE_QGUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
		const GUID DECLSPEC_SELECTANY name \
				= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_QGUID(qIID_ID3D11Texture2D,0x6f15aaf2,0xd208,0x4e89,0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c);

#ifdef WINRT
IDXGISwapChain1 *d3dswapchain;
#else
IDXGISwapChain *d3dswapchain;
#endif
IDXGIOutput *d3dscreen;

ID3D11RenderTargetView *fb_backbuffer;
ID3D11DepthStencilView *fb_backdepthstencil;
static DXGI_FORMAT	depthformat;

void *d3d11mod;
static unsigned int d3d11multisample_count, d3d11multisample_quality;

static qboolean vid_initializing;

extern qboolean		scr_initialized;                // ready to draw
extern qboolean		scr_drawloading;
extern qboolean		scr_con_forcedraw;
static qboolean d3d_resized;

static void released3dbackbuffer(void);
static qboolean resetd3dbackbuffer(int width, int height);

#if 0//def _DEBUG
#include <dxgidebug.h>
const GUID IID_IDXGIDebug = { 0x119E7452,0xDE9E,0x40fe, { 0x88,0x06,0x88,0xF9,0x0C,0x12,0xB4,0x41 } };

const GUID DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8 }};

void DoDXGIDebug(void)
{
	IDXGIDebug *dbg = NULL;

	HRESULT (WINAPI *pDXGIGetDebugInterface)(REFIID riid, void **ppDebug);
	dllfunction_t dxdidebugfuncs[] =
	{
		{(void**)&pDXGIGetDebugInterface, "DXGIGetDebugInterface"},
		{NULL}
	};
	pDXGIGetDebugInterface = NULL;
	Sys_LoadLibrary("dxgidebug", dxdidebugfuncs);
	pDXGIGetDebugInterface(&IID_IDXGIDebug, &dbg);
	if (dbg)
	{
		IDXGIDebug_ReportLiveObjects(dbg, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		IDXGIDebug_Release(dbg);
	}
}
#else
#define DoDXGIDebug()
#endif

char *D3D_NameForResult(HRESULT hr)
{
	if (hr == DXGI_ERROR_DEVICE_REMOVED && pD3DDev11)
		hr = ID3D11Device_GetDeviceRemovedReason(pD3DDev11);

	switch(hr)
	{
	case E_OUTOFMEMORY:						return "E_OUTOFMEMORY";
	case E_NOINTERFACE:						return "E_NOINTERFACE";
	case DXGI_ERROR_DEVICE_HUNG:			return "DXGI_ERROR_DEVICE_HUNG";
	case DXGI_ERROR_DEVICE_REMOVED:			return "DXGI_ERROR_DEVICE_REMOVED";
	case DXGI_ERROR_DEVICE_RESET:			return "DXGI_ERROR_DEVICE_RESET";
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR:	return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
	case DXGI_ERROR_INVALID_CALL:			return "DXGI_ERROR_INVALID_CALL";
	default:								return va("%lx", hr);
	}
}

static void D3D11_PresentOrCrash(void)
{
	extern cvar_t vid_vsync;
	RSpeedMark();
	HRESULT hr = IDXGISwapChain_Present(d3dswapchain, max(0,vid_vsync.ival), 0);
	if (FAILED(hr))
		Sys_Error("IDXGISwapChain_Present: %s\n", D3D_NameForResult(hr));
	RSpeedEnd(RSPEED_PRESENT);
}

typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLWINDOW, MS_UNINIT} dx11modestate_t;
static dx11modestate_t modestate;

//FIXME: need to push/pop render targets like gl does, to not harm shadowmaps/refraction/etc.
void D3D11_ApplyRenderTargets(qboolean usedepth)
{
	unsigned int width = 0, height = 0;
	int i;
	texid_t textures[1];
	texid_t depth;
	ID3D11RenderTargetView *rtv[sizeof(textures)/sizeof(textures[0])];
	ID3D11DepthStencilView *dsv;

	for (i = 0; i < sizeof(textures)/sizeof(textures[0]); i++)
	{
		if (!*r_refdef.rt_destcolour[i].texname)
			break;
		textures[i] = R2D_RT_GetTexture(r_refdef.rt_destcolour[i].texname, &width, &height);
		if (textures[i]->ptr2)
		{
			ID3D11ShaderResourceView_Release((ID3D11ShaderResourceView*)textures[i]->ptr2);
			textures[i]->ptr2 = NULL;
		}
		ID3D11Device_CreateRenderTargetView(pD3DDev11, textures[i]->ptr, NULL, &rtv[i]);
	}

	if (usedepth)
	{
		if (*r_refdef.rt_depth.texname)
			depth = R2D_RT_GetTexture(r_refdef.rt_depth.texname, &width, &height);
		else
			depth = R2D_RT_Configure("depth", width, height, PTI_DEPTH24, RT_IMAGEFLAGS);
	}
	else
		depth = NULL;
	if (depth && depth->ptr)
	{
		if (depth->ptr2)
		{
			ID3D11DepthStencilView_Release((ID3D11DepthStencilView*)depth->ptr2);
			depth->ptr2 = NULL;
		}
		ID3D11Device_CreateDepthStencilView(pD3DDev11, depth->ptr, NULL, &dsv);
	}
	else
		dsv = NULL;

	ID3D11DeviceContext_OMSetRenderTargets(d3ddevctx, i, rtv, dsv);
	for (i = 0; i < sizeof(textures)/sizeof(textures[0]); i++)
		if (rtv[i])
			ID3D11RenderTargetView_Release(rtv[i]);
	if (dsv)
	{
		ID3D11DeviceContext_ClearDepthStencilView(d3ddevctx, dsv, D3D11_CLEAR_DEPTH, 1, 0);	//is it faster to clear the stencil too?
		ID3D11DepthStencilView_Release(dsv);
	}
}


#ifndef WINRT	//winrt crap has its own non-hwnd window crap, after years of microsoft forcing everyone to use hwnds for everything. I wonder why they don't have that many winrt apps.
static void D3DVID_UpdateWindowStatus (HWND hWnd)
{
	POINT p;
	RECT nr;
	int window_x, window_y;
	int window_width, window_height;
	GetClientRect(hWnd, &nr);

//	Sys_Printf("Update: %i %i %i %i\n", nr.left, nr.top, nr.right, nr.bottom);

	//if its bad then we're probably minimised
	if (nr.right <= nr.left)
		return;
	if (nr.bottom <= nr.top)
		return;

	p.x = 0;
	p.y = 0;
	ClientToScreen(hWnd, &p);
	window_x = p.x;
	window_y = p.y;
	window_width = nr.right - nr.left;
	window_height = nr.bottom - nr.top;
//	vid.pixelwidth = window_width;
//	vid.pixelheight = window_height;

	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

//	Sys_Printf("Window: %i %i %i %i\n", window_x, window_y, window_width, window_height);


	INS_UpdateClipCursor ();
}

static qboolean D3D11AppActivate(BOOL fActive, BOOL minimize)
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	static BOOL	sound_active;

	if (vid.activeapp == fActive && Minimized == minimize)
		return false;	//so windows doesn't crash us over and over again.

	vid.activeapp = fActive;
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!vid.activeapp && sound_active)
	{
		S_BlockSound ();
		sound_active = false;
	}
	else if (vid.activeapp && !sound_active)
	{
		S_UnblockSound ();
		sound_active = true;
	}

	INS_UpdateGrabs(modestate != MS_WINDOWED, vid.activeapp);

	return true;
}

#ifndef FUCKDXGI
static void D3D11_DoResize(void)
{
	d3d_resized = true;

	D3DVID_UpdateWindowStatus(mainwindow);

	if (d3dscreen)
	{	//seriously? this is disgusting.
		DXGI_OUTPUT_DESC desc;
		IDXGIOutput_GetDesc(d3dscreen, &desc);
		vid.pixelwidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
		vid.pixelheight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
	}
	else
	{
		vid.pixelwidth = window_rect.right - window_rect.left;
		vid.pixelheight = window_rect.bottom - window_rect.top;
	}
//	Con_Printf("Resizing buffer to %i*%i\n", vid.pixelwidth, vid.pixelheight);
	released3dbackbuffer();
	IDXGISwapChain_ResizeBuffers(d3dswapchain, 0, vid.pixelwidth, vid.pixelheight, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	D3D11BE_Reset(true);
	resetd3dbackbuffer(vid.pixelwidth, vid.pixelheight);
	D3D11BE_Reset(false);
}
#endif

static void ClearAllStates (void)
{
	int		i;

// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<256 ; i++)
	{
		Key_Event (0, i, 0, false);
	}

	Key_ClearStates ();
	INS_ClearStates ();
}

static LRESULT WINAPI D3D11_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG    lRet = 0;
	int		temp;
	extern unsigned int uiWheelMessage;
#ifndef FUCKDXGI
	extern qboolean	keydown[K_MAX];
#endif

	if ( uMsg == uiWheelMessage )
		uMsg = WM_MOUSEWHEEL;

	switch (uMsg)
	{
#if 1
		case WM_KILLFOCUS:
			if (modestate == MS_FULLWINDOW)
				ShowWindow(mainwindow, SW_SHOWMINNOACTIVE);
			D3D11AppActivate(false, false);
			break;

		case WM_SETFOCUS:
			if (modestate == MS_FULLWINDOW)
				ShowWindow(mainwindow, SW_SHOWMAXIMIZED);
			D3D11AppActivate(true, false);

			if (modestate == MS_FULLSCREEN && d3dswapchain)
				IDXGISwapChain_SetFullscreenState(d3dswapchain, vid.activeapp, (vid.activeapp)?d3dscreen:NULL);
			Cvar_ForceCallback(&v_gamma);

			ClearAllStates ();
			break;

//		case WM_CREATE:
//			break;

		case WM_MOVE:
			D3DVID_UpdateWindowStatus (hWnd);
			lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
#if 0
			if (keydown[K_LALT] && wParam == '\r')
			{
				if (d3dscreen)
				{
					IDXGIOutput_Release(d3dscreen);
					d3dscreen = NULL;
				}

				if (modestate == MS_FULLSCREEN)
					modestate = MS_WINDOWED;
				else
				{
					RECT rect;
					extern cvar_t vid_width, vid_height;
					int width = vid_width.ival;
					int height = vid_height.ival;
					if (!width || !height)
					{
						DXGI_OUTPUT_DESC desc;
						IDXGISwapChain_GetContainingOutput(d3dswapchain, &d3dscreen);
						IDXGIOutput_GetDesc(d3dscreen, &desc);
						rect = desc.DesktopCoordinates;
					}
					else
					{
						rect.left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
						rect.top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
						rect.right = rect.left+width;
						rect.bottom = rect.top+height;
					}
					AdjustWindowRectEx(&rect, WS_OVERLAPPED, FALSE, 0);
					SetWindowPos(hWnd, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_SHOWWINDOW|SWP_FRAMECHANGED);
					modestate = MS_FULLSCREEN;
				}

				if (!d3dscreen && modestate == MS_FULLSCREEN)
					IDXGISwapChain_GetContainingOutput(d3dswapchain, &d3dscreen);
				IDXGISwapChain_SetFullscreenState(d3dswapchain, modestate == MS_FULLSCREEN, (modestate == MS_FULLSCREEN)?d3dscreen:NULL);

				if (modestate == MS_WINDOWED)
				{
					RECT rect;
					int width = 640;
					int height = 480;
					rect.left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
					rect.top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
					rect.right = rect.left+width;
					rect.bottom = rect.top+height;
					AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
					SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);	//make sure dxgi didn't break us.
					SetWindowPos(hWnd, HWND_TOP, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_SHOWWINDOW|SWP_FRAMECHANGED);
					SetForegroundWindow(hWnd);
					SetFocus(hWnd);

					//work around a windows bug by forcing all windows to be repainted.
					InvalidateRect(NULL, NULL, false);
				}
				D3D11_DoResize();
				Cvar_ForceCallback(&v_gamma);
			}
			else
#endif
			if (!vid_initializing)
				INS_TranslateKeyEvent (wParam, lParam, true, 0, false);
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (!vid_initializing)
				INS_TranslateKeyEvent (wParam, lParam, false, 0, false);
			break;

		case WM_APPCOMMAND:
			lRet = INS_AppCommand(lParam);
			break;

		case WM_SYSCHAR:
		// keep Alt-Space from happening
			break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			if (wParam & MK_XBUTTON1)
				temp |= 8;

			if (wParam & MK_XBUTTON2)
				temp |= 16;

			if (wParam & MK_XBUTTON3)
				temp |= 32;

			if (wParam & MK_XBUTTON4)
				temp |= 64;

			if (wParam & MK_XBUTTON5)
				temp |= 128;

			if (wParam & MK_XBUTTON6)
				temp |= 256;

			if (wParam & MK_XBUTTON7)
				temp |= 512;

			if (!vid_initializing)
				INS_MouseEvent (temp);

			break;

		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper
		// Event.
		case WM_MOUSEWHEEL:
			if (!vid_initializing)
			{
				if ((short) HIWORD(wParam) > 0)
				{
					Key_Event(0, K_MWHEELUP, 0, true);
					Key_Event(0, K_MWHEELUP, 0, false);
				}
				else
				{
					Key_Event(0, K_MWHEELDOWN, 0, true);
					Key_Event(0, K_MWHEELDOWN, 0, false);
				}
			}
			break;

		case WM_INPUT:
			// raw input handling
			if (!vid_initializing)
				INS_RawInput_Read((HANDLE)lParam);
			break;
		case WM_DEVICECHANGE:
			COM_AddWork(WG_MAIN, INS_DeviceChanged, NULL, NULL, uMsg, 0);
			lRet = TRUE;
			break;

		case WM_SETCURSOR:
			//only use a custom cursor if the cursor is inside the client area
			switch(lParam&0xffff)
			{
			case 0:
				break;
			case HTCLIENT:
				if (hCustomCursor)	//custom cursor enabled
					SetCursor(hCustomCursor);
				else				//fallback on an arrow cursor, just so we have something visible at startup or so
					SetCursor(hArrowCursor);
				lRet = TRUE;
				break;
			default:
				lRet = DefWindowProcW (hWnd, uMsg, wParam, lParam);
				break;
			}
			break;

		case WM_GETMINMAXINFO:
			{
				RECT windowrect;
				RECT clientrect;
				MINMAXINFO *mmi = (MINMAXINFO *) lParam;

				GetWindowRect (hWnd, &windowrect);
				GetClientRect (hWnd, &clientrect);

				mmi->ptMinTrackSize.x = 320 + ((windowrect.right - windowrect.left) - (clientrect.right - clientrect.left));
				mmi->ptMinTrackSize.y = 200 + ((windowrect.bottom - windowrect.top) - (clientrect.bottom - clientrect.top));
			}
			return 0;
		case WM_SIZE:
			if (d3dswapchain)
			{
				d3d_resized = true;

				D3DVID_UpdateWindowStatus(mainwindow);

				released3dbackbuffer();
				IDXGISwapChain_ResizeBuffers(d3dswapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

				D3D11BE_Reset(true);
				vid.pixelwidth = window_rect.right - window_rect.left;
				vid.pixelheight = window_rect.bottom - window_rect.top;
				resetd3dbackbuffer(vid.pixelwidth, vid.pixelheight);
				D3D11BE_Reset(false);
			}
			lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
			break;

		case WM_CLOSE:
			if (!vid_initializing)
				if (MessageBox (mainwindow, "Are you sure you want to quit?", "Confirm Exit",
							MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
				{
					Cbuf_AddText("\nquit\n", RESTRICT_LOCAL);
				}

			break;

		case WM_DESTROY:
		{
//			if (dibwindow)
//				DestroyWindow (dibwindow);
		}
		break;
#ifdef HAVE_CDPLAYER
		case MM_MCINOTIFY:
			lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
			break;
#endif
#endif
		case WM_ERASEBKGND:
			return 1;
		default:
			/* pass all unhandled messages to DefWindowProc */
			lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
		break;
	}

	/* return 1 if handled message, 0 if not */
	return lRet;
}
#endif

#if (WINVER < 0x500) && !defined(__GNUC__)
typedef struct tagMONITORINFO
{
	DWORD   cbSize;
	RECT    rcMonitor;
	RECT    rcWork;
	DWORD   dwFlags;
} MONITORINFO, *LPMONITORINFO;
#endif

static void released3dbackbuffer(void)
{
	if (d3ddevctx)
		ID3D11DeviceContext_OMSetRenderTargets(d3ddevctx, 0, NULL, NULL);
	if (fb_backbuffer)
		ID3D11RenderTargetView_Release(fb_backbuffer);
	fb_backbuffer = NULL;
	if (fb_backdepthstencil)
		ID3D11DepthStencilView_Release(fb_backdepthstencil);
	fb_backdepthstencil = NULL;
}

static qboolean resetd3dbackbuffer(int width, int height)
{
	D3D11_TEXTURE2D_DESC t2ddesc;
//	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ID3D11Texture2D *backbuftex, *depthtex;

	released3dbackbuffer();

	//get a proper handle to the backbuffer (silly hurdles)
	if (FAILED(IDXGISwapChain_GetBuffer(d3dswapchain, 0, &qIID_ID3D11Texture2D, (LPVOID*)&backbuftex)))
		return false;
	if (FAILED(ID3D11Device_CreateRenderTargetView(pD3DDev11, (ID3D11Resource*)backbuftex, NULL, &fb_backbuffer)))
		return false;
	ID3D11Texture2D_Release(backbuftex);

	//set up a depth buffer.
	memset(&t2ddesc, 0, sizeof(t2ddesc));
	t2ddesc.Width = width;
	t2ddesc.Height = height;
	t2ddesc.MipLevels = 1;
	t2ddesc.ArraySize = 1;
	t2ddesc.Format = depthformat;
	t2ddesc.SampleDesc.Count = d3d11multisample_count;
	t2ddesc.SampleDesc.Quality = d3d11multisample_quality;
	t2ddesc.Usage = D3D11_USAGE_DEFAULT;
	t2ddesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	t2ddesc.CPUAccessFlags = 0;
	t2ddesc.MiscFlags = 0;
	if(FAILED(ID3D11Device_CreateTexture2D(pD3DDev11, &t2ddesc, NULL, &depthtex)))
		return false;
//	dsvd.Format = t2ddesc.Format;
//	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
//	dsvd.Texture2D.MipSlice = 0;
	if(FAILED(ID3D11Device_CreateDepthStencilView(pD3DDev11, (ID3D11Resource*)depthtex, NULL/*&dsvd*/, &fb_backdepthstencil)))
		return false;
	ID3D11Texture2D_Release(depthtex);

	//now tell d3d which render targets to use.
	ID3D11DeviceContext_OMSetRenderTargets(d3ddevctx, 1, &fb_backbuffer, fb_backdepthstencil);

	return true;
}

#ifdef WINRT	//winrt crap has its own non-hwnd window crap, after years of microsoft forcing everyone to use hwnds for everything. I wonder why they don't have that many winrt apps.
void D3D11_DoResize(int newwidth, int newheight)
{
	d3d_resized = true;

//	Con_Printf("Resizing buffer to %i*%i\n", vid.pixelwidth, vid.pixelheight);
	released3dbackbuffer();
	if (d3dswapchain)
	{
		IDXGISwapChain_ResizeBuffers(d3dswapchain, 0, vid.pixelwidth, vid.pixelheight, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

		D3D11BE_Reset(true);
		resetd3dbackbuffer(vid.pixelwidth, vid.pixelheight);
		D3D11BE_Reset(false);
	}
}
void *RT_GetCoreWindow(int *width, int *height);
static qboolean D3D11_VID_Init(rendererstate_t *info, unsigned char *palette)
{
	static IID factiid1 = {0x770aae78, 0xf26f, 0x4dba, 0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87};
	static IID factiid2 = {0x50c83a1c, 0xe072, 0x4c48, 0x87, 0xb0, 0x36, 0x30, 0xfa, 0x36, 0xa6, 0xd0};
	IDXGIFactory2 *fact = NULL;
	HRESULT hr;
	D3D_FEATURE_LEVEL flevel, flevels[] =
	{
		//D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,

		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	DXGI_SWAP_CHAIN_DESC1 scd = {0};

	IUnknown *window = RT_GetCoreWindow(&info->width, &info->height);

	modestate = MS_FULLSCREEN;

	if (info->depthbits == 16)
		depthformat = DXGI_FORMAT_D16_UNORM;
	else if (info->depthbits == 32)
		depthformat = DXGI_FORMAT_D32_FLOAT;
	else
		depthformat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//fill scd
	scd.Width = info->width;
	scd.Height = info->height;
	scd.Format = info->srgb?DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:DXGI_FORMAT_B8G8R8A8_UNORM;
	scd.Stereo = info->stereo;
	scd.SampleDesc.Count = d3d11multisample_count = max(1,info->multisample);
	scd.SampleDesc.Quality = d3d11multisample_quality = (d3d11multisample_count>1)?~0:0;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = 2+info->triplebuffer;	//rt only supports fullscreen, so the frontbuffer needs to be created by us.
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	scd.Flags = 0;

	//create d3d stuff
	hr = CreateDXGIFactory1(&factiid2, &fact);
	if (FAILED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, flevels, sizeof(flevels)/sizeof(flevels[0]), D3D11_SDK_VERSION, &pD3DDev11, &flevel, &d3ddevctx)))
		Sys_Error("D3D11CreateDevice failed\n");
	else
	{
		if (FAILED(IDXGIFactory2_CreateSwapChainForCoreWindow(fact, (IUnknown*)pD3DDev11, window, &scd, NULL, &d3dswapchain)))
			Sys_Error("IDXGIFactory2_CreateSwapChainForCoreWindow failed\n");
		else
		{
			vid.numpages = scd.BufferCount;
			if (!resetd3dbackbuffer(info->width, info->height))
				Sys_Error("unable to reset back buffer\n");
			else
			{
				if (!D3D11Shader_Init(flevel))
					Con_Printf("Unable to intialise a suitable HLSL compiler, please install the DirectX runtime.\n");
				else
					return true;
			}
		}
	}
	return false;
}
#else
static qboolean initD3D11Device(HWND hWnd, rendererstate_t *info, PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN func, IDXGIAdapter *adapt)
{
	UINT support;
	int flags = 0;//= D3D11_CREATE_DEVICE_SINGLETHREADED;
	D3D_DRIVER_TYPE drivertype;
	DXGI_SWAP_CHAIN_DESC scd;
	D3D_FEATURE_LEVEL flevel, flevels[] =
	{
		//D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,

		//FIXME: need npot.
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	memset(&scd, 0, sizeof(scd));

	if (!stricmp(info->subrenderer, "debug"))
		flags |= D3D11_CREATE_DEVICE_DEBUG;

	if (!stricmp(info->subrenderer, "warp"))
		drivertype = D3D_DRIVER_TYPE_WARP;
	else if (!stricmp(info->subrenderer, "ref"))
		drivertype = D3D_DRIVER_TYPE_REFERENCE;
	else if (!stricmp(info->subrenderer, "hw"))
		drivertype = D3D_DRIVER_TYPE_HARDWARE;
	else if (!stricmp(info->subrenderer, "null"))
		drivertype = D3D_DRIVER_TYPE_NULL;
	else if (!stricmp(info->subrenderer, "software"))
		drivertype = D3D_DRIVER_TYPE_SOFTWARE;
	else if (!stricmp(info->subrenderer, "unknown"))
		drivertype = D3D_DRIVER_TYPE_UNKNOWN;
	else
		drivertype = adapt?D3D_DRIVER_TYPE_UNKNOWN:D3D_DRIVER_TYPE_HARDWARE;

	if (info->depthbits == 16)
		depthformat = DXGI_FORMAT_D16_UNORM;
	else if (info->depthbits == 32)
		depthformat = DXGI_FORMAT_D32_FLOAT;
	else
		depthformat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//for stereo support, we would have to rewrite all of this in a way that would make us dependant upon windows 8 or 7+platform update, which would exclude vista.
	scd.BufferDesc.Width = info->width;
	scd.BufferDesc.Height = info->height;
	scd.BufferDesc.RefreshRate.Numerator = 0;
	scd.BufferDesc.RefreshRate.Denominator = 0;
	scd.BufferCount = 1+info->triplebuffer;	//back buffer count
	if (info->srgb)
	{
		if (info->srgb >= 2)	//fixme: detect properly.
			scd.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;	//on nvidia, outputs linear rgb to srgb devices, which means info->srgb is effectively set
		else
			scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else if (info->bpp == 30)	//fixme: detect properly.
		scd.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	else
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = d3d11multisample_count = max(1, info->multisample);	//as we're starting up windowed (and switching to fullscreen after), the frontbuffer is handled by windows.
	scd.SampleDesc.Quality = d3d11multisample_quality = 0;
	scd.Windowed = TRUE;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;// | DXGI_SWAP_CHAIN_FLAG_NONPREROTATED;

#ifdef _DEBUG
//	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	if (drivertype == D3D_DRIVER_TYPE_UNKNOWN && adapt)
	{
		DXGI_ADAPTER_DESC adesc;
		IDXGIAdapter_GetDesc(adapt, &adesc);
		Con_Printf("D3D11 Adaptor: %S\n", adesc.Description);
	}
	else
	{
		adapt = NULL;
		Con_Printf("D3D11 Adaptor: %s\n", "Unspecified");
	}

	if (FAILED(func(adapt, drivertype, NULL, flags,
				flevels, sizeof(flevels)/sizeof(flevels[0]),
				D3D11_SDK_VERSION,
				&scd,
				&d3dswapchain,
				&pD3DDev11,
				&flevel,
				&d3ddevctx)))
		return false;

	if (!pD3DDev11)
		return false;

	Con_Printf("D3D11 Feature level: %i_%i\n", flevel>>12, (flevel>>8) & 0xf);

	if (!resetd3dbackbuffer(info->width, info->height))
		return false;

	if (info->fullscreen)
	{
	}

	memset(&sh_config, 0, sizeof(sh_config));
	sh_config.texture_non_power_of_two = flevel>=D3D_FEATURE_LEVEL_10_0;	//npot MUST be supported on all d3d10+ cards.
	sh_config.texture_non_power_of_two_pic = true;	//always supported in d3d11, supposedly, even with d3d9 devices.
	sh_config.texture_allow_block_padding = false;	//microsoft blocks this.
	sh_config.npot_rounddown = false;
	if (flevel>=D3D_FEATURE_LEVEL_11_0)
		sh_config.texture2d_maxsize = 16384;
	else if (flevel>=D3D_FEATURE_LEVEL_10_0)
		sh_config.texture2d_maxsize = 8192;
	else if (flevel>=D3D_FEATURE_LEVEL_9_3)
		sh_config.texture2d_maxsize = 4096;
	else
		sh_config.texture2d_maxsize = 2048;

	if (flevel>=D3D_FEATURE_LEVEL_9_3)
		sh_config.texture2d_maxsize = 4096;
	else
		sh_config.texture2d_maxsize = 512;

//11.1 formats
#define DXGI_FORMAT_B4G4R4A4_UNORM 115

	//why does d3d11 have no rgbx format? anyone else think that weird?

	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_B5G6R5_UNORM, &support)))			//crippled to win8+ only.
		sh_config.texfmt[PTI_RGB565] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_B5G5R5A1_UNORM, &support)))		//crippled to win8+ only.
		sh_config.texfmt[PTI_ARGB1555] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_B4G4R4A4_UNORM, &support)))		//crippled to win8+ only.
		sh_config.texfmt[PTI_ARGB4444] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);

	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_R8G8B8A8_UNORM, &support)))
		sh_config.texfmt[PTI_RGBA8] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, &support)))
		sh_config.texfmt[PTI_RGBA8_SRGB] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_R10G10B10A2_UNORM, &support)))
		sh_config.texfmt[PTI_A2BGR10] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D) && !!(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_R9G9B9E5_SHAREDEXP, &support)))
		sh_config.texfmt[PTI_E5BGR9] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_R11G11B10_FLOAT, &support)))
		sh_config.texfmt[PTI_B10G11R11F] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_R16G16B16A16_FLOAT, &support)))
		sh_config.texfmt[PTI_RGBA16F] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D) && !!(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_R32G32B32A32_FLOAT, &support)))
		sh_config.texfmt[PTI_RGBA32F] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D) && !!(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);

	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_B8G8R8A8_UNORM, &support)))
		sh_config.texfmt[PTI_BGRA8] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, &support)))
		sh_config.texfmt[PTI_BGRA8_SRGB] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_B8G8R8X8_UNORM, &support)))
		sh_config.texfmt[PTI_BGRX8] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, &support)))
		sh_config.texfmt[PTI_BGRX8_SRGB] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);

	//compressed formats
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC1_UNORM, &support)))
		sh_config.texfmt[PTI_BC1_RGBA] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC1_UNORM_SRGB, &support)))
		sh_config.texfmt[PTI_BC1_RGBA_SRGB] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC2_UNORM, &support)))
		sh_config.texfmt[PTI_BC2_RGBA] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC2_UNORM_SRGB, &support)))
		sh_config.texfmt[PTI_BC2_RGBA_SRGB] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC3_UNORM, &support)))
		sh_config.texfmt[PTI_BC3_RGBA] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC3_UNORM_SRGB, &support)))
		sh_config.texfmt[PTI_BC3_RGBA_SRGB] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC4_UNORM, &support)))
		sh_config.texfmt[PTI_BC4_R] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC4_SNORM, &support)))
		sh_config.texfmt[PTI_BC4_R_SNORM] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC5_UNORM, &support)))
		sh_config.texfmt[PTI_BC5_RG] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC5_SNORM, &support)))
		sh_config.texfmt[PTI_BC5_RG_SNORM] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC6H_UF16, &support)))
		sh_config.texfmt[PTI_BC6_RGB_UFLOAT] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC6H_SF16, &support)))
		sh_config.texfmt[PTI_BC6_RGB_SFLOAT] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC7_UNORM, &support)))
		sh_config.texfmt[PTI_BC7_RGBA] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
	if (SUCCEEDED(ID3D11Device_CheckFormatSupport(pD3DDev11, DXGI_FORMAT_BC7_UNORM_SRGB, &support)))
		sh_config.texfmt[PTI_BC7_RGBA_SRGB] = !!(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);

	//these formats are not officially supported as specified, but noone cares
	sh_config.texfmt[PTI_RGBX8] = sh_config.texfmt[PTI_RGBA8];
	sh_config.texfmt[PTI_RGBX8_SRGB] = sh_config.texfmt[PTI_RGBA8_SRGB];
	sh_config.texfmt[PTI_BC1_RGB] = sh_config.texfmt[PTI_BC1_RGBA];
	sh_config.texfmt[PTI_BC1_RGB_SRGB] = sh_config.texfmt[PTI_BC1_RGBA_SRGB];

	switch(scd.BufferDesc.Format)
	{
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		vid.flags |= VID_SRGB_FB_LINEAR|VID_FP16;	//these are apparently linear already.
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		vid.flags |= VID_SRGB_FB_LINEAR;	//effectively linear.
		break;
	default:
		//non-linear formats.
		break;
	}
	if ((vid.flags & VID_SRGB_FB) && info->srgb >= 0)
		vid.flags |= VID_SRGBAWARE;

	vid.numpages = scd.BufferCount;
	if (!D3D11Shader_Init(flevel))
	{
		Con_Printf("Unable to intialise a suitable HLSL compiler, please install the DirectX runtime.\n");
		return false;
	}
	return true;
}

static void initD3D11(HWND hWnd, rendererstate_t *info)
{
	static dllhandle_t *dxgi;
	static PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN fnc;
	static HRESULT (WINAPI *pCreateDXGIFactory1)(IID *riid, void **ppFactory);
	static IID factiid = {0x770aae78, 0xf26f, 0x4dba, {0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87}};
	IDXGIFactory1 *fact = NULL;
	IDXGIAdapter *adapt = NULL;
	vrsetup_t vrsetup = {sizeof(vrsetup)};
	dllfunction_t d3d11funcs[] =
	{
		{(void**)&fnc, "D3D11CreateDeviceAndSwapChain"},
		{NULL}
	};
	dllfunction_t dxgifuncs[] =
	{
		{(void**)&pCreateDXGIFactory1, "CreateDXGIFactory1"},
		{NULL}
	};

	if (!dxgi)
		dxgi = Sys_LoadLibrary("dxgi", dxgifuncs);
	if (!d3d11mod)
		d3d11mod = Sys_LoadLibrary("d3d11", d3d11funcs);

	if (!d3d11mod)
		return;

	vrsetup.vrplatform = VR_D3D11;
	if (pCreateDXGIFactory1)
	{
		HRESULT hr;
		hr = pCreateDXGIFactory1(&factiid, (void**)&fact);
		if (FAILED(hr))
			Con_Printf("CreateDXGIFactory1 failed: %s\n", D3D_NameForResult(hr));
		if (fact)
		{
			ULONGLONG devid = strtoull(info->subrenderer, NULL, 16);
			vrsetup.deviceid[0] = (devid    )&0xffffffff;
			vrsetup.deviceid[1] = (devid>>32)&0xffffffff;
			if (info->vr)
			{
				if (!info->vr->Prepare(&vrsetup))
				{
					info->vr->Shutdown();
					info->vr = NULL;
				}
			}

			if (vrsetup.deviceid[0] || vrsetup.deviceid[1])
			{
				int id = 0;
				while (S_OK==IDXGIFactory1_EnumAdapters(fact, id++, &adapt))
				{
					DXGI_ADAPTER_DESC desc;
					IDXGIAdapter_GetDesc(adapt, &desc);
					if (desc.AdapterLuid.LowPart == vrsetup.deviceid[0] && desc.AdapterLuid.HighPart == vrsetup.deviceid[1])
						break;
					IDXGIAdapter_Release(adapt);
					adapt = NULL;
				}
			}
			else
				IDXGIFactory1_EnumAdapters(fact, 0, &adapt);
		}
	}
	else
		info->vr = NULL;	//o.O

	
	initD3D11Device(hWnd, info, fnc, adapt);

	if (adapt)
		IDXGIAdapter_Release(adapt);
	if (fact)
	{
		//DXGI SUCKS and fucks up alt+tab every single time. its pointless to go from fullscreen to fullscreen-with-taskbar-obscuring-half-the-window.
		//I'm just going to handle that stuff myself.
		//IDXGIFactory1_MakeWindowAssociation(fact, hWnd, DXGI_MWA_NO_WINDOW_CHANGES|DXGI_MWA_NO_ALT_ENTER|DXGI_MWA_NO_PRINT_SCREEN);
		IDXGIFactory1_Release(fact);
	}
}

static qboolean D3D11_VID_EnumerateDevices(void *usercontext, void(*callback)(void *context, const char *devicename, const char *outputname, const char *desc))
{
	static dllhandle_t *dxgi;
	static HRESULT (WINAPI *pCreateDXGIFactory1)(IID *riid, void **ppFactory);
	static IID factiid = {0x770aae78, 0xf26f, 0x4dba, {0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87}};
	IDXGIFactory1 *fact = NULL;
	dllfunction_t dxgifuncs[] =
	{
		{(void**)&pCreateDXGIFactory1, "CreateDXGIFactory1"},
		{NULL}
	};

	if (!dxgi)
		dxgi = Sys_LoadLibrary("dxgi", dxgifuncs);
	if (!dxgi)
		return true;

	if (pCreateDXGIFactory1)
	{
		HRESULT hr;
		hr = pCreateDXGIFactory1(&factiid, (void**)&fact);
		if (FAILED(hr))
			Con_Printf("CreateDXGIFactory1 failed: %s\n", D3D_NameForResult(hr));
		if (fact)
		{
			IDXGIAdapter *adapt = NULL;
			int id = 0;
			char devname[128*4];
			char dev[128*4];
			DXGI_ADAPTER_DESC desc;
			while (S_OK==IDXGIFactory1_EnumAdapters(fact, id++, &adapt))
			{
				IDXGIAdapter_GetDesc(adapt, &desc);

				Q_snprintfz(devname,sizeof(devname), "Direct3D11 - %s", narrowen(dev,sizeof(dev), desc.Description));
				Q_snprintfz(dev,sizeof(dev), "%"PRIx64, ((ULONGLONG)desc.AdapterLuid.HighPart<<32)|desc.AdapterLuid.LowPart);
				callback(usercontext, dev, ""/*FIXME*/, devname);

				IDXGIAdapter_Release(adapt);
				adapt = NULL;
			}

			IDXGIFactory1_Release(fact);
		}
	}
	return true;
}

static qboolean D3D11_VID_Init(rendererstate_t *info, unsigned char *palette)
{
	DWORD width = info->width;
	DWORD height = info->height;
	//DWORD bpp = info->bpp;
	//DWORD zbpp = 16;
	//DWORD flags = 0;
	DWORD wstyle;
	RECT rect;
	MSG msg;
	HICON hIcon = LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON1));

	//DDGAMMARAMP gammaramp;
	//int i;

	char *CLASSNAME = "FTED3D11QUAKE";
	WNDCLASS wc = {
		0,
		&D3D11_WindowProc,
		0,
		0,
		NULL,
		hIcon,
		NULL,
		NULL,
		NULL,
		CLASSNAME
	};

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hCursor   = hArrowCursor = LoadCursor (NULL,IDC_ARROW);
	wc.hInstance = global_hInstance; 

	vid_initializing = true;

	RegisterClass(&wc);

	if (info->fullscreen == 2)
		modestate = MS_FULLWINDOW;
	else if (info->fullscreen)
		modestate = MS_FULLSCREEN;	//FIXME: I'm done with fighting dxgi. I'm just going to pick the easy method that doesn't end up with totally fucked up behaviour.
	else
		modestate = MS_WINDOWED;

	if (modestate == MS_FULLWINDOW)
	{
		wstyle = WS_POPUP;
		rect.right = GetSystemMetrics(SM_CXSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYSCREEN);
		rect.left = 0;
		rect.top = 0;
	}
	else
	{
		wstyle = WS_OVERLAPPEDWINDOW;
		rect.left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
		rect.top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
		rect.right = rect.left+width;
		rect.bottom = rect.top+height;
		AdjustWindowRectEx(&rect, wstyle, FALSE, 0);
	}
	mainwindow = CreateWindow(CLASSNAME, "Direct3D11", wstyle, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, NULL, NULL, NULL, NULL);

	// Try as specified.

	DoDXGIDebug();

	initD3D11(mainwindow, info);
	if (!pD3DDev11)
	{
		DoDXGIDebug();
		Con_Printf("No suitable D3D11 device found\n");
		return false;
	}

	vid.pixelwidth = width;
	vid.pixelheight = height;

	while (PeekMessage(&msg, NULL,  0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CL_UpdateWindowTitle();

	if (modestate == MS_FULLWINDOW)
		ShowWindow(mainwindow, SW_SHOWMAXIMIZED);
	else
		ShowWindow(mainwindow, SW_SHOWNORMAL);

	vid.width = vid.pixelwidth;
	vid.height = vid.pixelheight;

	if (modestate == MS_FULLSCREEN)
	{
		if (!d3dscreen)
			IDXGISwapChain_GetContainingOutput(d3dswapchain, &d3dscreen);
		IDXGISwapChain_SetFullscreenState(d3dswapchain, true, d3dscreen);
	}

	vid_initializing = false;

	GetWindowRect(mainwindow, &window_rect);

	{
		extern qboolean	mouseactive;
		mouseactive = false;
	}

	rf->VID_CreateCursor = WIN_CreateCursor;
	rf->VID_DestroyCursor = WIN_DestroyCursor;
	rf->VID_SetCursor = WIN_SetCursor;

	return true;
}
#endif

static void	 (D3D11_VID_DeInit)				(void)
{
	Image_Shutdown();

	/*we cannot shut down cleanly while in fullscreen, supposedly*/
	if(d3dswapchain)
		IDXGISwapChain_SetFullscreenState(d3dswapchain, false, NULL);

	released3dbackbuffer();
	if(d3dswapchain)
		IDXGISwapChain_Release(d3dswapchain);
	d3dswapchain = NULL;
	if (pD3DDev11)
		ID3D11Device_Release(pD3DDev11);
	pD3DDev11 = NULL;

	if (d3ddevctx)
	{
		ID3D11DeviceContext_ClearState(d3ddevctx);
		ID3D11DeviceContext_Flush(d3ddevctx);
		ID3D11DeviceContext_Release(d3ddevctx);
	}
	d3ddevctx = NULL;

#ifndef WINRT
	if (mainwindow)
	{
		DestroyWindow(mainwindow);
		mainwindow = NULL;
	}
#endif

	if (d3dscreen)
		IUnknown_Release(d3dscreen);
	d3dscreen = NULL;

	DoDXGIDebug();
}

extern float		hw_blend[4];		// rgba 0.0 - 1.0

static void D3D11_BuildRamps(int points, DXGI_RGB *out)
{
//FIXME: repack input rather than recalculating.
	int i;
	vec3_t cshift;
	vec3_t c;
	float sc;
	VectorScale(hw_blend, hw_blend[3], cshift);
	for (i = 0; i < points; i++)
	{
		sc = i / (float)(points-1);
		VectorSet(c, sc, sc, sc);
		VectorAdd(c, cshift, c);
		VectorScale(c, v_contrast.value, c);

		out[i].Red		= pow (c[0], v_gamma.value) + v_brightness.value;
		out[i].Green	= pow (c[1], v_gamma.value) + v_brightness.value;
		out[i].Blue		= pow (c[2], v_gamma.value) + v_brightness.value;
	}
}

static qboolean	D3D11_VID_ApplyGammaRamps(unsigned int gammarampsize, unsigned short *ramps)
{
	HRESULT hr;
	DXGI_GAMMA_CONTROL_CAPABILITIES caps;
	DXGI_GAMMA_CONTROL gam;
	//cache the screen, so we don't get too confused.
	if (!d3dscreen)
		return false;	//don't do it when we're running windowed.

	if (d3dscreen)
	{
		gam.Scale.Red = 1;
		gam.Scale.Green = 1;
		gam.Scale.Blue = 1;
		gam.Offset.Red = 0;
		gam.Offset.Green = 0;
		gam.Offset.Blue = 0;

		if (FAILED(IDXGIOutput_GetGammaControlCapabilities(d3dscreen, &caps)))
			return false;
		if (caps.NumGammaControlPoints > 1025)
			caps.NumGammaControlPoints = 1025;
		D3D11_BuildRamps(caps.NumGammaControlPoints, gam.GammaCurve);

		hr = IDXGIOutput_SetGammaControl(d3dscreen, &gam);
		if (SUCCEEDED(hr))
			return true;
	}
	return false;
}
static char	*D3D11_VID_GetRGBInfo(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt)
{
	//don't directly map the frontbuffer, as that can hold other things.
	//create a texture, copy the (gpu)backbuffer to that (cpu)texture
	//then map the (cpu)texture and copy out the parts we need, reordering as needed.
	D3D11_MAPPED_SUBRESOURCE lock;
	qbyte *rgb, *in, *r = NULL;
	unsigned int x,y;
	D3D11_TEXTURE2D_DESC texDesc;
	ID3D11Texture2D *texture;
	ID3D11Resource *backbuffer;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Width = vid.pixelwidth;
	texDesc.Height = vid.pixelheight;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	if (FAILED(ID3D11Device_CreateTexture2D(pD3DDev11, &texDesc, 0, &texture)))
		return NULL;
	ID3D11RenderTargetView_GetResource(fb_backbuffer, &backbuffer);
	ID3D11DeviceContext_CopyResource(d3ddevctx, (ID3D11Resource*)texture, backbuffer);
	ID3D11Resource_Release(backbuffer);
	if (!FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)texture, 0, D3D11_MAP_READ, 0, &lock)))
	{
		r = rgb = BZ_Malloc(3 * vid.pixelwidth * vid.pixelheight);
		for (y = vid.pixelheight; y-- > 0; )
		{
			in = lock.pData;
			in += y * lock.RowPitch;
			for (x = 0; x < vid.pixelwidth; x++, rgb+=3, in+=4)
			{
				rgb[0] = in[0];
				rgb[1] = in[1];
				rgb[2] = in[2];
			}
		}
		ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)texture, 0);
	}
	ID3D11Texture2D_Release(texture);
	*bytestride = vid.pixelwidth*3;
	*truevidwidth = vid.pixelwidth;
	*truevidheight = vid.pixelheight;
	*fmt = TF_RGB24;
	return r;
}
static void	(D3D11_VID_SetWindowCaption)		(const char *msg)
{
#ifndef WINRT
	SetWindowText(mainwindow, msg);
#endif
}

void D3D11_Set2D (void)
{
//	Matrix4x4_CM_Orthographic(r_refdef.m_projection, 0 + (0.5*vid.width/vid.pixelwidth), vid.width + (0.5*vid.width/vid.pixelwidth), 0 + (0.5*vid.height/vid.pixelheight), vid.height + (0.5*vid.height/vid.pixelheight), 0, 100);
	Matrix4x4_CM_Orthographic(r_refdef.m_projection_std, 0, vid.width, vid.height, 0, 0, 99999);
	Matrix4x4_Identity(r_refdef.m_view);


	vid.fbvwidth = vid.width;
	vid.fbvheight = vid.height;
	vid.fbpwidth = vid.pixelwidth;
	vid.fbpheight = vid.pixelheight;

	r_refdef.pxrect.x = 0;
	r_refdef.pxrect.y = 0;
	r_refdef.pxrect.width = vid.fbpwidth;
	r_refdef.pxrect.height = vid.fbpheight;

	D3D11BE_Set2D();
}

static qboolean	(D3D11_SCR_UpdateScreen)			(void)
{
	//extern int keydown[];
	//extern cvar_t vid_conheight;
#ifdef TEXTEDITOR
	//extern qboolean editormodal;
#endif
	qboolean nohud, noworld;

	if (r_clear.ival)
	{
		float colours[4] = {(r_clear.ival&1)?1:0, (r_clear.ival&2)?1:0, (r_clear.ival&4)?1:0, 1};
		ID3D11DeviceContext_ClearRenderTargetView(d3ddevctx, fb_backbuffer, colours);
	}

	if (d3d_resized)
	{
		extern cvar_t vid_conautoscale, vid_conwidth;
		d3d_resized = false;

		// force width/height to be updated
		//vid.pixelwidth = window_rect.right - window_rect.left;
		//vid.pixelheight = window_rect.bottom - window_rect.top;
		Cvar_ForceCallback(&vid_conautoscale);
		Cvar_ForceCallback(&vid_conwidth);
	}
	R2D_Font_Changed();

	if (scr_disabled_for_loading)
	{
		extern float scr_disabled_time;
		if (Sys_DoubleTime() - scr_disabled_time > 60 || Key_Dest_Has(~kdm_game))
		{
			scr_disabled_for_loading = false;
		}
		else
		{
//			IDirect3DDevice9_BeginScene(pD3DDev9);
			scr_drawloading = true;
			SCR_DrawLoading (true);
			scr_drawloading = false;
			if (R2D_Flush)
				R2D_Flush();
//			IDirect3DDevice9_EndScene(pD3DDev9);
			D3D11_PresentOrCrash();
			return true;
		}
	}

	if (!scr_initialized || !con_initialized)
	{
		return false;                         // not initialized yet
	}

	Shader_DoReload();

//	d3d11error(IDirect3DDevice9_BeginScene(pD3DDev9));
/*
#ifdef TEXTEDITOR
	if (editormodal)
	{
		Editor_Draw();
		V_UpdatePalette (false);
		Media_RecordFrame();
		R2D_BrightenScreen();

		if (key_dest == key_console)
			Con_DrawConsole(vid_conheight.value/2, false);
		GL_EndRendering ();
		GL_DoSwap();
		RSpeedEnd(RSPEED_TOTALREFRESH);
		return;
	}
#endif
*/

//
// do 3D refresh drawing, and then update the screen
//
	SCR_SetUpToDrawConsole ();

	noworld = false;
	nohud = false;

	if (topmenu && topmenu->isopaque)
		nohud = true;
#ifdef VM_CG
	else if (q3 && q3->cg.Redraw(cl.time))
		nohud = true;
#endif
#ifdef CSQC_DAT
	else if (CSQC_DrawView())
		nohud = true;
#endif
	else if (r_worldentity.model && cls.state == ca_active)
		V_RenderView (nohud);
	else
		noworld = true;

	D3D11_Set2D();

	if (!noworld)
	{
		R2D_BrightenScreen();
	}

	scr_con_forcedraw = false;
	if (noworld)
	{
		if ((!Key_Dest_Has(~(kdm_game|kdm_console))) && SCR_GetLoadingStage() == LS_NONE)
			scr_con_current = vid.height;

		if (scr_con_current != vid.height)
			R2D_ConsoleBackground(0, vid.height, true);
		else
			scr_con_forcedraw = true;

		nohud = true;
	}

	SCR_DrawTwoDimensional(nohud);

	V_UpdatePalette (false);

	Media_RecordFrame();

	RSpeedShow();

	D3D11_PresentOrCrash();

	window_center_x = (window_rect.left + window_rect.right)/2;
	window_center_y = (window_rect.top + window_rect.bottom)/2;


	INS_UpdateGrabs(modestate != MS_WINDOWED, vid.activeapp);
	return true;
}







static void	D3D11_Draw_Init				(void)
{
	R2D_Init();
}
static void	D3D11_Draw_Shutdown				(void)
{
	R2D_Shutdown();
}

static void	D3D11_R_Init					(void)
{
}
static void	D3D11_R_DeInit					(void)
{
	R_GAliasFlushSkinCache(true);
	Surf_DeInit();
	D3D11BE_Shutdown();
	Shader_Shutdown();
}



static void D3D11_SetupViewPort(void)
{
	int		x, x2, y2, y;

	float fov_x, fov_y;
	float fovv_x, fovv_y;

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);

	//
	// set up viewpoint
	//
	x = r_refdef.vrect.x * vid.pixelwidth/(int)vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * vid.pixelwidth/(int)vid.width;
	y = (r_refdef.vrect.y) * vid.pixelheight/(int)vid.height;
	y2 = ((int)(r_refdef.vrect.y + r_refdef.vrect.height)) * vid.pixelheight/(int)vid.height;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < vid.pixelwidth)
		x2++;
	if (y < 0)
		y--;
	if (y2 < vid.pixelheight)
		y2++;

	fov_x = r_refdef.fov_x;//+sin(cl.time)*5;
	fov_y = r_refdef.fov_y;//-sin(cl.time+1)*5;
	fovv_x = r_refdef.fovv_x;
	fovv_y = r_refdef.fovv_y;

	if ((r_refdef.flags & RDF_UNDERWATER) && !(r_refdef.flags & RDF_WATERWARP))
	{
		fov_x *= 1 + (((sin(cl.time * 4.7) + 1) * 0.015) * r_waterwarp.value);
		fov_y *= 1 + (((sin(cl.time * 3.0) + 1) * 0.015) * r_waterwarp.value);
		fovv_x *= 1 + (((sin(cl.time * 4.7) + 1) * 0.015) * r_waterwarp.value);
		fovv_y *= 1 + (((sin(cl.time * 3.0) + 1) * 0.015) * r_waterwarp.value);
	}

	if (r_xflip.ival)
	{
		fov_x *= -1;
		r_refdef.flipcull ^= SHADER_CULL_FLIP;
		fovv_x *= -1;
	}

	/*view matrix*/
	Matrix4x4_CM_ModelViewMatrixFromAxis(r_refdef.m_view, vpn, vright, vup, r_refdef.vieworg);

	/*projection matricies (main game, and viewmodel)*/
	Matrix4x4_CM_Projection_Offset(r_refdef.m_projection_std,  -fov_x/2, fov_x/2, -fov_y/2, fov_y/2, r_refdef.mindist, r_refdef.maxdist, true);
	Matrix4x4_CM_Projection_Offset(r_refdef.m_projection_view, -fovv_x/2, fovv_x/2, -fovv_y/2, fovv_y/2, r_refdef.mindist, r_refdef.maxdist, true);
}

static void	(D3D11_R_RenderView)				(void)
{
	float x, x2, y, y2;
	double time1 = 0, time2 = 0;
	qboolean dofbo = *r_refdef.rt_destcolour[0].texname || *r_refdef.rt_depth.texname;
	int cull = r_refdef.flipcull;
//	texid_t colourrt[1];

	if (!r_norefresh.value)
	{
		if (r_speeds.ival)
			time1 = Sys_DoubleTime();

		if (dofbo)
			D3D11_ApplyRenderTargets(true);
		else
			ID3D11DeviceContext_ClearDepthStencilView(d3ddevctx, fb_backdepthstencil, D3D11_CLEAR_DEPTH, 1, 0);	//is it faster to clear the stencil too?

		//check if we can do underwater warp
		if (cls.protocol != CP_QUAKE2)	//quake2 tells us directly
		{
			if (r_viewcontents & FTECONTENTS_FLUID)
				r_refdef.flags |= RDF_UNDERWATER;
			else
				r_refdef.flags &= ~RDF_UNDERWATER;
		}
		if (r_refdef.flags & RDF_UNDERWATER)
		{
			extern cvar_t r_projection;
			if (!r_waterwarp.value || r_projection.ival)
				r_refdef.flags &= ~RDF_UNDERWATER;	//no warp at all
	//		else if (r_waterwarp.value > 0 && scenepp_waterwarp)
	//			r_refdef.flags |= RDF_WATERWARP;	//try fullscreen warp instead if we can
		}

		D3D11_SetupViewPort();
		//unlike gl, we clear colour beforehand, because that seems more sane.
		//always clear depth

		x = (r_refdef.vrect.x * (int)vid.pixelwidth)/(int)vid.width;
		x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * (int)vid.pixelwidth/(int)vid.width;
		y = (r_refdef.vrect.y * (int)vid.pixelheight)/(int)vid.height;
		y2 = (r_refdef.vrect.y + r_refdef.vrect.height) * (int)vid.pixelheight/(int)vid.height;
		r_refdef.pxrect.x = floor(x);
		r_refdef.pxrect.y = floor(y);
		r_refdef.pxrect.width = (int)ceil(x2) - r_refdef.pxrect.x;
		r_refdef.pxrect.height = (int)ceil(y2) - r_refdef.pxrect.y;

		Surf_SetupFrame();

		//fixme: waterwarp fov

		R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);
		RQ_BeginFrame();
		
		if (!(r_refdef.flags & RDF_NOWORLDMODEL))
			if (!r_worldentity.model || r_worldentity.model->loadstate != MLS_LOADED || !cl.worldmodel)
			{
				D3D11_Set2D ();
				R2D_ImageColours(0, 0, 0, 1);
				R2D_FillBlock(r_refdef.vrect.x, r_refdef.vrect.y, r_refdef.vrect.width, r_refdef.vrect.height);
				R2D_ImageColours(1, 1, 1, 1);

				r_refdef.flipcull = cull;
				if (dofbo)
					D3D11_ApplyRenderTargets(false);
				return;
			}
//		if (!(r_refdef.flags & RDF_NOWORLDMODEL))
//		{
//			if (!r_refdef.recurse && !(r_refdef.flags & RDF_DISABLEPARTICLES))
//				P_DrawParticles ();
//		}
		Surf_DrawWorld();
		RQ_RenderBatchClear();

		r_refdef.flipcull = cull;

		if (r_speeds.ival)
		{
			time2 = Sys_DoubleTime();
			RQuantAdd(RQUANT_MSECS, (int)((time2-time1)*1000000));
		}

		if (dofbo)
			D3D11_ApplyRenderTargets(false);
	}
	D3D11_Set2D ();
}

void D3D11BE_RenderToTextureUpdate2d(qboolean destchanged)
{
	if (destchanged)
	{
		if (*r_refdef.rt_destcolour[0].texname)
			D3D11_ApplyRenderTargets(false);
		else
			ID3D11DeviceContext_OMSetRenderTargets(d3ddevctx, 1, &fb_backbuffer, fb_backdepthstencil);

		D3D11_Set2D();
	}
	else
	{
//		shaderstate.tex_sourcecol = R2D_RT_GetTexture(r_refdef.rt_sourcecolour.texname, &width, &height);
//		shaderstate.tex_sourcedepth = R2D_RT_GetTexture(r_refdef.rt_depth.texname, &width, &height);
	}
}

rendererinfo_t d3d11rendererinfo =
{
	"Direct3D11",
	{
		"D3D11",
		"Direct3d11",
		"DirectX11",
		"DX11"
	},
	QR_DIRECT3D11,

	D3D11_Draw_Init,
	D3D11_Draw_Shutdown,

	D3D11_UpdateFiltering,
	D3D11_LoadTextureMips,
	D3D11_DestroyTexture,
/*
	D3D11_LoadTexture,
	D3D11_LoadTexture8Pal24,
	D3D11_LoadTexture8Pal32,
	D3D11_LoadCompressed,
	D3D11_FindTexture,
	D3D11_AllocNewTexture,
	D3D11_Upload,
	D3D11_DestroyTexture,
*/
	D3D11_R_Init,
	D3D11_R_DeInit,
	D3D11_R_RenderView,

	D3D11_VID_Init,
	D3D11_VID_DeInit,
	D3D11_PresentOrCrash,
	D3D11_VID_ApplyGammaRamps,
	NULL,
	NULL,
	NULL,
	D3D11_VID_SetWindowCaption,
	D3D11_VID_GetRGBInfo,

	D3D11_SCR_UpdateScreen,

	D3D11BE_SelectMode,
	D3D11BE_DrawMesh_List,
	D3D11BE_DrawMesh_Single,
	D3D11BE_SubmitBatch,
	D3D11BE_GetTempBatch,
	D3D11BE_DrawWorld,
	D3D11BE_Init,
	D3D11BE_GenBrushModelVBO,
	D3D11BE_ClearVBO,
	D3D11BE_UploadAllLightmaps,
	D3D11BE_SelectEntity,
	D3D11BE_SelectDLight,
	D3D11BE_Scissor,
	D3D11BE_LightCullModel,

	D3D11BE_VBO_Begin,
	D3D11BE_VBO_Data,
	D3D11BE_VBO_Finish,
	D3D11BE_VBO_Destroy,

	D3D11BE_RenderToTextureUpdate2d,

	"no more",
	NULL,	//VID_GetPriority
	NULL,	//VID_EnumerateVideoModes
	D3D11_VID_EnumerateDevices
};
#endif
