#include "quakedef.h"
#ifdef SWQUAKE
#include "sw.h"

#include "winquake.h"
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



typedef struct dibinfo
{
	BITMAPINFOHEADER	header;
	RGBQUAD				acolors[256];
} dibinfo_t;

//struct
//{
	HBITMAP hDIBSection;
	qbyte *pDIBBase;

	HDC mainhDC;
	HDC hdcDIBSection;
	HGDIOBJ previously_selected_GDI_obj;

	int framenumber;
	void *screenbuffer;
	qintptr_t screenpitch;
	int window_x, window_y;
	unsigned int *depthbuffer;
struct
{
	qboolean isfullscreen;
} w32sw;
HWND mainwindow;

void DIB_Shutdown(void)
{
	if (depthbuffer)
	{
		BZ_Free(depthbuffer);
		depthbuffer = NULL;
	}

	if (hdcDIBSection)
	{
		SelectObject(hdcDIBSection, previously_selected_GDI_obj);
		DeleteDC(hdcDIBSection);
		hdcDIBSection = NULL;
	}

	if (hDIBSection)
	{
		DeleteObject(hDIBSection);
		hDIBSection = NULL;
		pDIBBase = NULL;
	}

	if (mainhDC)
	{
		ReleaseDC(mainwindow, mainhDC);
		mainhDC = 0;
	}
}
qboolean DIB_Init(void)
{
	dibinfo_t   dibheader;
	BITMAPINFO *pbmiDIB = ( BITMAPINFO * ) &dibheader;
	int i;

	memset( &dibheader, 0, sizeof( dibheader ) );

	/*
	** grab a DC
	*/
	if ( !mainhDC )
	{
		if ( ( mainhDC = GetDC( mainwindow ) ) == NULL )
			return false;
	}

	/*
	** fill in the BITMAPINFO struct
	*/
	pbmiDIB->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	pbmiDIB->bmiHeader.biWidth         = vid.pixelwidth;
	pbmiDIB->bmiHeader.biHeight        = vid.pixelheight;
	pbmiDIB->bmiHeader.biPlanes        = 1;
	pbmiDIB->bmiHeader.biBitCount      = 32;
	pbmiDIB->bmiHeader.biCompression   = BI_RGB;
	pbmiDIB->bmiHeader.biSizeImage     = 0;
	pbmiDIB->bmiHeader.biXPelsPerMeter = 0;
	pbmiDIB->bmiHeader.biYPelsPerMeter = 0;
	pbmiDIB->bmiHeader.biClrUsed       = 0;
	pbmiDIB->bmiHeader.biClrImportant  = 0;

	/*
	** fill in the palette
	*/
	for ( i = 0; i < 256; i++ )
	{
		dibheader.acolors[i].rgbRed   = ( d_8to24rgbtable[i] >> 0 )  & 0xff;
		dibheader.acolors[i].rgbGreen = ( d_8to24rgbtable[i] >> 8 )  & 0xff;
		dibheader.acolors[i].rgbBlue  = ( d_8to24rgbtable[i] >> 16 ) & 0xff;
	}

	/*
	** create the DIB section
	*/
	hDIBSection = CreateDIBSection( mainhDC,
											 pbmiDIB,
											 DIB_RGB_COLORS,
											 (void**)&pDIBBase,
											 NULL,
											 0 );

	if ( hDIBSection == NULL )
	{
		Con_Printf( "DIB_Init() - CreateDIBSection failed\n" );
		goto fail;
	}

	if (pbmiDIB->bmiHeader.biHeight < 0)
	{
		// bottom up
		screenbuffer	= pDIBBase + ( vid.pixelheight - 1 ) * vid.pixelwidth * 4;
		screenpitch		= -(int)vid.pixelwidth;
	}
	else
	{
		// top down
		screenbuffer	= pDIBBase;
		screenpitch		= vid.pixelwidth;
	}

	/*
	** clear the DIB memory buffer
	*/
	memset( pDIBBase, 0, vid.pixelwidth * vid.pixelheight * 4);

	if ( ( hdcDIBSection = CreateCompatibleDC( mainhDC ) ) == NULL )
	{
		Con_Printf( "DIB_Init() - CreateCompatibleDC failed\n" );
		goto fail;
	}
	if ( ( previously_selected_GDI_obj = SelectObject( hdcDIBSection, hDIBSection ) ) == NULL )
	{
		Con_Printf( "DIB_Init() - SelectObject failed\n" );
		goto fail;
	}

	depthbuffer = BZ_Malloc(vid.pixelwidth * vid.pixelheight * sizeof(*depthbuffer));
	if (depthbuffer)
		return true;

fail:
	DIB_Shutdown();
	return false;
}
void DIB_SwapBuffers(void)
{
//	extern float usingstretch;

//	if (usingstretch == 1)
		BitBlt( mainhDC,
				0, 0,
				vid.pixelwidth,
				vid.pixelheight,
				hdcDIBSection,
				0, 0,
				SRCCOPY );
/*	else
		StretchBlt( mainhDC,	//Why is StretchBlt not optimised for a scale of 2? Surly that would be a frequently used quantity?
			0, 0,
			vid.width*usingstretch,
			vid.height*usingstretch,
			hdcDIBSection,
			0, 0,
			vid.width, vid.height,
			SRCCOPY );
*/
}

static int window_width;
static int window_height;
void SWV_UpdateWindowStatus(void)
{
	POINT p;
	RECT nr;
	RECT WindowRect;
	GetClientRect(mainwindow, &nr);

	//if its bad then we're probably minimised
	if (nr.right <= nr.left)
		return;
	if (nr.bottom <= nr.top)
		return;

	WindowRect = nr;
	p.x = 0;
	p.y = 0;
	ClientToScreen(mainwindow, &p);
	window_x = p.x;
	window_y = p.y;
	window_width = WindowRect.right - WindowRect.left;
	window_height = WindowRect.bottom - WindowRect.top;

	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	INS_UpdateClipCursor ();
}


qboolean SWAppActivate(BOOL fActive, BOOL minimize)
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

	INS_UpdateGrabs(false, vid.activeapp);

/*
	if (fActive)
	{
		if (modestate != MS_WINDOWED)
		{
			if (vid_canalttab && vid_wassuspended) {
				vid_wassuspended = false;
				ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN);
				ShowWindow(mainwindow, SW_SHOWNORMAL);
			}
		}
	}
	if (!fActive)
	{
		if (modestate != MS_WINDOWED)
		{
			if (vid_canalttab) {
				ChangeDisplaySettings (NULL, 0);
				vid_wassuspended = true;
			}
		}
	}
*/
	return true;
}

LONG WINAPI MainWndProc (
	HWND    hWnd,
	UINT    uMsg,
	WPARAM  wParam,
	LPARAM  lParam)
{
	LONG			lRet = 0;
	int				fActive, fMinimized, temp;
	HDC				hdc;
	PAINTSTRUCT		ps;
	extern unsigned int uiWheelMessage;
//	static int		recursiveflag;

	if ( uMsg == uiWheelMessage ) {
		uMsg = WM_MOUSEWHEEL;
		wParam <<= 16;
	}

	switch (uMsg)
	{
		case WM_CREATE:
			break;

		case WM_SYSCOMMAND:

		// Check for maximize being hit
			switch (wParam & ~0x0F)
			{
				case SC_MAXIMIZE:
					Cbuf_AddText("vid_fullscreen 1;vid_restart\n", RESTRICT_LOCAL);
				// if minimized, bring up as a window before going fullscreen,
				// so MGL will have the right state to restore
/*					if (Minimized)
					{
						force_mode_set = true;
						VID_SetMode (vid_modenum, vid_curpal);
						force_mode_set = false;
					}

					VID_SetMode ((int)vid_fullscreen_mode.value, vid_curpal);
*/					break;

				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
					if (w32sw.isfullscreen)
					{
					// don't call DefWindowProc() because we don't want to start
					// the screen saver fullscreen
						break;
					}

				// fall through windowed and allow the screen saver to start

				default:
//				if (!vid_initializing)
//				{
//					S_BlockSound ();					
//				}

				lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);

//				if (!vid_initializing)
//				{
//					S_UnblockSound ();
//				}
			}
			break;

		case WM_MOVE:
			SWV_UpdateWindowStatus ();

//			if ((modestate == MS_WINDOWED) && !in_mode_set && !Minimized)
//				VID_RememberWindowPos ();

			break;

		case WM_SIZE:
			SWV_UpdateWindowStatus ();
/*			Minimized = false;
			
			if (!(wParam & SIZE_RESTORED))
			{
				if (wParam & SIZE_MINIMIZED)
					Minimized = true;
			}

			if (!Minimized && !w32sw.isfullscreen)
			{
				int nt, nl;
				int nw, nh;
				qboolean move = false;
				RECT r;
				GetClientRect (hWnd, &r);
				nw = (int)(r.right - r.left)&~3;
				nh = (int)(r.bottom - r.top)&~3;

				window_width = nw;
				window_height = nh;
				VID2_UpdateWindowStatus();

				if (nw < 320)
				{
					move = true;
					nw = 320;
				}
				if (nh < 200)
				{
					move = true;
					nh = 200;
				}
				if (nh > MAXHEIGHT)
				{
					move = true;
					nh = MAXHEIGHT;
				}
				if ((r.right - r.left) & 3)
					move = true;
				if ((r.bottom - r.top) & 3)
					move = true;

				GetWindowRect (hWnd, &r);
				nl = r.left;
				nt = r.top;
				r.left =0;
				r.top = 0;
				r.right = nw*usingstretch;
				r.bottom = nh*usingstretch;
				AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, FALSE, 0);
				if (move)
					MoveWindow(hWnd, nl, nt, r.right - r.left, r.bottom - r.top, true);
				else
				{
					if (vid.width != nw || vid.height != nh)
					{
						M_RemoveAllMenus();	//can cause probs
						DIB_Shutdown();
						vid.conwidth = vid.width = nw;//vid_stretch.value;
						vid.conheight = vid.height = nh;///vid_stretch.value;
						
						DIB_Init( &vid.buffer, &vid.rowbytes );
						vid.conbuffer = vid.buffer;
						vid.conrowbytes = vid.rowbytes;	

						if (VID_AllocBuffers(vid.width, vid.height, r_pixbytes))
							D_InitCaches (vid_surfcache, vid_surfcachesize);

						SCR_UpdateWholeScreen();
					}
					else
						SCR_UpdateWholeScreen();
				}

			}
*/
			break;

		case WM_SYSCHAR:
		// keep Alt-Space from happening
			break;

		case WM_ACTIVATE:
			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);
			SWAppActivate(!(fActive == WA_INACTIVE), fMinimized);

		// fix the leftover Alt from any Alt-Tab or the like that switched us away
//			ClearAllStates ();

			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);

//			if (!in_mode_set && host_initialized)
//				SCR_UpdateWholeScreen ();

			EndPaint(hWnd, &ps);
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
//			if (!vid_initializing)
				INS_TranslateKeyEvent(wParam, lParam, true, 0, false);
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
//			if (!vid_initializing)
				INS_TranslateKeyEvent(wParam, lParam, false, 0, false);
			break;

		case WM_APPCOMMAND:
			lRet = INS_AppCommand(lParam);
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
//			if (!vid_initializing)
			{
				temp = 0;

				if (wParam & MK_LBUTTON)
					temp |= 1;

				if (wParam & MK_RBUTTON)
					temp |= 2;

				if (wParam & MK_MBUTTON)
					temp |= 4;

				// extra buttons
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

				INS_MouseEvent (temp);
			}
			break;
		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper
		// Event.
		case WM_MOUSEWHEEL: 
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
			INS_RawInput_Read((HANDLE)lParam);
			break;
		case WM_DEVICECHANGE:
			COM_AddWork(WG_MAIN, INS_DeviceChanged, NULL, NULL, uMsg, 0);
			lRet = TRUE;
			break;
/*		case WM_DISPLAYCHANGE:
			if (!in_mode_set && (modestate == MS_WINDOWED) && !vid_fulldib_on_focus_mode)
			{
				force_mode_set = true;
				VID_SetMode (vid_modenum, vid_curpal);
				force_mode_set = false;
			}
			break;
*/
		case WM_CLOSE:
		// this causes Close in the right-click task bar menu not to work, but right
		// now bad things happen if Close is handled in that case (garbage and a
		// crash on Win95)
//			if (!vid_initializing)
			{
				if (MessageBox (mainwindow, "Are you sure you want to quit?", "Confirm Exit",
							MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
				{
					Cbuf_AddText("\nquit\n", RESTRICT_LOCAL);
				}
			}
			break;

#ifdef HAVE_CDPLAYER
		case MM_MCINOTIFY:
			lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
			break;
#endif

		default:
			/* pass all unhandled messages to DefWindowProc */
			lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
			break;
	}

	/* return 0 if handled message, 1 if not */
	return lRet;
}



/*
** VID_CreateWindow
*/
#define	WINDOW_CLASS_NAME FULLENGINENAME
#define	WINDOW_TITLE_NAME FULLENGINENAME


void VID_CreateWindow(int width, int height, qboolean fullscreen)
{
	WNDCLASS		wc;
	RECT			r;
	int				x, y, w, h;
	int				exstyle;
	int stylebits;


	if (fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP;
	}
	else
	{
		exstyle = 0;
		stylebits = WS_OVERLAPPEDWINDOW;
	}

	/* Register the frame class */
	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC)MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = global_hInstance;
	wc.hIcon         = 0;
	wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (void *)COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WINDOW_CLASS_NAME;

	RegisterClass(&wc);

	r.left = 0;
	r.top = 0;
	r.right  = width;
	r.bottom = height;

	AdjustWindowRectEx (&r, stylebits, FALSE, exstyle);

	window_rect = r;

	w = r.right - r.left;
	h = r.bottom - r.top;
	x = 0;//vid_xpos.value;
	y = 0;//vid_ypos.value;

	mainwindow = CreateWindowEx(
		exstyle,
		 WINDOW_CLASS_NAME,
		 WINDOW_TITLE_NAME,
		 stylebits,
		 x, y, w, h,
		 NULL,
		 NULL,
		 global_hInstance,
		 NULL);

	if (!mainwindow)
		Sys_Error("Couldn't create window");
	
	ShowWindow(mainwindow, SW_SHOWNORMAL);
	UpdateWindow(mainwindow);
	SetForegroundWindow(mainwindow);
	SetFocus(mainwindow);

	SWV_UpdateWindowStatus();
}








void SW_VID_UpdateViewport(wqcom_t *com)
{
	com->viewport.cbuf = screenbuffer;
	com->viewport.dbuf = depthbuffer;
	com->viewport.width = vid.pixelwidth;
	com->viewport.height = vid.pixelheight;
	com->viewport.stride = screenpitch;
	com->viewport.framenum = framenumber;
}

void SW_VID_SwapBuffers(void)
{
	extern cvar_t vid_conautoscale;
	DIB_SwapBuffers();
	framenumber++;

	INS_UpdateGrabs(false, vid.activeapp);

//	memset( pDIBBase, 0, vid.pixelwidth * vid.pixelheight * 4);


	if (window_width != vid.pixelwidth || window_height != vid.pixelheight)
	{
		DIB_Shutdown();
		vid.pixelwidth = window_width;
		vid.pixelheight = window_height;
		if (!DIB_Init())
			Sys_Error("resize reinitialization error");

		Cvar_ForceCallback(&vid_conautoscale);
	}
}

qboolean SW_VID_Init(rendererstate_t *info, unsigned char *palette)
{
	vid.pixelwidth = info->width;
	vid.pixelheight = info->height;

	if (info->fullscreen)	//don't do this with d3d - d3d should set it's own video mode.
	{	//make windows change res.
		DEVMODE gdevmode;
		gdevmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
		if (info->bpp)
			gdevmode.dmFields |= DM_BITSPERPEL;
		if (info->rate)
			gdevmode.dmFields |= DM_DISPLAYFREQUENCY;
		gdevmode.dmBitsPerPel = info->bpp;
		if (info->bpp && (gdevmode.dmBitsPerPel < 15))
		{
			Con_Printf("Forcing at least 16bpp\n");
			gdevmode.dmBitsPerPel = 16;
		}
		gdevmode.dmDisplayFrequency = info->rate;
		gdevmode.dmPelsWidth = info->width;
		gdevmode.dmPelsHeight = info->height;
		gdevmode.dmSize = sizeof (gdevmode);

		if (ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			Con_SafePrintf((gdevmode.dmFields&DM_DISPLAYFREQUENCY)?"Windows rejected mode %i*%i*%i*%i\n":"Windows rejected mode %i*%i*%i\n", (int)gdevmode.dmPelsWidth, (int)gdevmode.dmPelsHeight, (int)gdevmode.dmBitsPerPel, (int)gdevmode.dmDisplayFrequency);
			return false;
		}
	}

	VID_CreateWindow(vid.pixelwidth, vid.pixelheight, info->fullscreen);

	if (!DIB_Init())
		return false;

	return true;
}
void SW_VID_DeInit(void)
{
	Image_Shutdown();

	DIB_Shutdown();
	DestroyWindow(mainwindow);

	ChangeDisplaySettings (NULL, 0);
}
qboolean SW_VID_ApplyGammaRamps		(unsigned int gammarampsize, unsigned short *ramps)
{
	return false;
}
char *SW_VID_GetRGBInfo(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt)
{
	char *buf = NULL;
	char *src, *dst;
	int w, h;
	buf = BZ_Malloc((vid.pixelwidth * vid.pixelheight * 3));
	dst = buf;
	for (h = 0; h < vid.pixelheight; h++)
	{
		for (w = 0, src = (char*)screenbuffer + (h * vid.pixelwidth*4); w < vid.pixelwidth; w++, dst += 3, src += 4)
		{
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
		}
	}
	*bytestride = vid.pixelwidth*3;
	*truevidwidth = vid.pixelwidth;
	*truevidheight = vid.pixelheight;
	*fmt = TF_BGR24;
	return buf;
}
void SW_VID_SetWindowCaption(const char *msg)
{
}
#endif
