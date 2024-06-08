#include "../sys_local.h"
#include "win_local.h"
#include "rc/doom_resource.h"

//
// idSplashScreen
//
class idSplashScreen {
public:
	idSplashScreen();	
	~idSplashScreen();

	int SetBitmap(unsigned int bitmapIdentifier, unsigned int maskColor);
	void Create(void);

	HWND GetHwnd(void) { return hWnd; }
	void Destroy(void);
private:
	static LRESULT __stdcall	WndProc(HWND__* hWnd, unsigned int message, unsigned int wParam, int lParam);

	HRGN __cdecl CreateRgnFromBitmap(HWND__* hWnd, HBITMAP__* hBmp, tagBITMAP bm, unsigned int color);
	void DrawWindow(HWND__* hWnd, HDC__* hdc);
	ATOM RegisterClass(void);


	HGDIOBJ bitmap;
	HGDIOBJ region;
	HWND hWnd = 0;
	HINSTANCE hInstance;
	BITMAP bitmapInfo;		
};

idSplashScreen* currentSplashScreen = nullptr;

/*
====================
idSplashScreen::idSplashScreen
====================
*/
idSplashScreen::idSplashScreen()
{
	bitmap = 0;
	region = 0;
	hWnd = 0;
	hInstance = win32.hInstance;
	bitmapInfo.bmType = 0;
	bitmapInfo.bmWidth = 0;
	bitmapInfo.bmHeight = 0;
	bitmapInfo.bmWidthBytes = 0;
	bitmapInfo.bmPlanes = 0;
	bitmapInfo.bmBits = 0;
}

/*
====================
idSplashScreen::~idSplashScreen
====================
*/

idSplashScreen::~idSplashScreen()
{
	Destroy();
}

/*
====================
idSplashScreen::Create
====================
*/
void idSplashScreen::Create(void)
{
	if (!RegisterClass())
	{
		return;
	}

	hWnd = CreateWindowEx(0,
		"idSplashScreen",
		GAME_NAME,
		WS_POPUP,
		0, 0, 740, 416,
		NULL,
		NULL,
		win32.hInstance,
		NULL);


	ShowWindow(hWnd, SW_SHOW);
}

/*
====================
idSplashScreen::Destroy
====================
*/
void idSplashScreen::Destroy(void)
{
	if (this->bitmap)
	{
		DeleteObject(bitmap);
		bitmap = 0;
	}
	if (region)
	{
		DeleteObject(region);
		region = 0;
	}

	DestroyWindow(hWnd);
}

/*
====================
idSplashScreen::DrawWindow
====================
*/
void idSplashScreen::DrawWindow(HWND__* hWnd, HDC__* hdc)
{
	HDC__* v4; 
	HDC v5; 
	HGDIOBJ oldBitmap; 

	if (bitmap)
	{
		v4 = hdc;
		v5 = CreateCompatibleDC(hdc);
		oldBitmap = SelectObject(v5, bitmap);
		BitBlt(v4, 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight, v5, 0, 0, SRCCOPY);
		SelectObject(v5, oldBitmap);
		DeleteDC(v5);
	}
}

int idSplashScreen::SetBitmap( unsigned int bitmapIdentifier, unsigned int maskColor)
{
	idSplashScreen* v3; // esi
	HBITMAP v4; // eax
	int v5; // eax
	int v6; // ebp
	int v7; // ebx
	int v8; // eax
	HRGN v10; // eax

	v3 = this;
	if (this->bitmap)
	{
		DeleteObject(this->bitmap);
		v3->bitmap = 0;
	}
	if (v3->region)
	{
		DeleteObject(v3->region);
		v3->region = 0;
	}
	v4 = LoadBitmapA(v3->hInstance, MAKEINTRESOURCEA(bitmapIdentifier));
	v3->bitmap = v4;
	if (!v4)
		return 0;
	GetObjectA(v4, 24, &v3->bitmapInfo);
	v5 = GetSystemMetrics(16);
	v6 = v3->bitmapInfo.bmHeight;
	v7 = (v5 - v3->bitmapInfo.bmWidth) / 2;
	v8 = GetSystemMetrics(17);
	MoveWindow(v3->hWnd, v7, (v8 - v6) / 2, v3->bitmapInfo.bmWidth, v6, 1);
	v10 = CreateRgnFromBitmap(v3->hWnd, (HBITMAP__ *)&v3->bitmap, v3->bitmapInfo, maskColor);
	v3->region = v10;
	if (v10)
		SetWindowRgn(v3->hWnd, v10, 1);
	return 1;
}


/*
====================
idSplashScreen::DrawWindow
====================
*/
HRGN idSplashScreen::CreateRgnFromBitmap(HWND__* hWnd, HBITMAP__* hBmp, tagBITMAP bm, unsigned int color)
{
	HDC v5; // eax
	HDC v6; // edi
	signed int v7; // ebx
	int* v8; // esi
	int v9; // edi
	unsigned int v10; // ebp
	unsigned int v11; // ebx
	bool v12; // al
	int v13; // ecx
	int* v14; // eax
	void* v15; // ebp
	HRGN v16; // ebp
	int* v17; // edi
	HRGN v18; // ebx
	int cBlocks; // [esp+8h] [ebp-18h]
	int first; // [esp+Ch] [ebp-14h]
	int v21; // [esp+10h] [ebp-10h]
	HDC__* dcBmp; // [esp+14h] [ebp-Ch]
	HDC hdc; // [esp+18h] [ebp-8h]
	char wasfirst; // [esp+28h] [ebp+8h]
	int wasfirstb; // [esp+28h] [ebp+8h]
	unsigned int wasfirsta; // [esp+28h] [ebp+8h]

	if (!hBmp)
		return 0;
	v5 = GetDC(hWnd);
	v6 = CreateCompatibleDC(v5);
	hdc = v6;
	SelectObject(v6, hBmp);
	v7 = 1;
	v21 = 0;
	wasfirst = 0;
	first = 1;
	v8 = (int*)operator new(0x2A0u);
	memset(v8, 0, 0x2A0u);
	*v8 = 32;
	v8[1] = 1;
	v8[2] = 0;
	if (bm.bmHeight > 0)
	{
		cBlocks = bm.bmHeight;
		dcBmp = (HDC__*)bm.bmHeight;
		do
		{
			v9 = 0;
			if (bm.bmWidth > 0)
			{
				v10 = 40 * v7;
				v11 = 640 * v7 + 32;
				do
				{
					v12 = GetPixel(hdc, v9, cBlocks - 1) != color;
					if (wasfirst)
					{
						if (v12 && (v13 = bm.bmWidth, v9 == bm.bmWidth - 1) || (v13 = bm.bmWidth, v12 != v9 < bm.bmWidth))
						{
							v14 = &v8[4 * (v8[2] + 2)];
							*v14 = v21;
							v14[1] = cBlocks - 1;
							v14[2] = v9 + (v9 == v13 - 1);
							v14[3] = cBlocks;
							if (++v8[2] >= v10)
							{
								++first;
								v11 += 640;
								wasfirstb = v10 + 40;
								v15 = operator new(v11);
								memcpy(v15, v8, v11 - 640);
								operator delete(v8);
								v8 = (int*)v15;
								v10 = wasfirstb;
							}
							wasfirst = 0;
						}
					}
					else if (v12)
					{
						v21 = v9;
						wasfirst = 1;
					}
					++v9;
				} while (v9 < bm.bmWidth);
				v7 = first;
			}
			--cBlocks;
			dcBmp = (HDC__*)((char*)dcBmp - 1);
		} while (dcBmp);
		v6 = hdc;
	}
	DeleteDC(v6);
	v16 = CreateRectRgn(0, 0, 0, 0);
	wasfirsta = 0;
	if (v8[2])
	{
		v17 = v8 + 10;
		do
		{
			v18 = CreateRectRgn(*(v17 - 2), *(v17 - 1), *v17, v17[1]);
			CombineRgn(v16, v16, v18, 2);
			if (v18)
				DeleteObject(v18);
			v17 += 4;
			++wasfirsta;
		} while (wasfirsta < v8[2]);
	}
	operator delete(v8);
	return v16;
}

/*
====================
idSplashScreen::WndProc
====================
*/
LRESULT __stdcall idSplashScreen::WndProc(HWND__* hWnd, unsigned int message, unsigned int wParam, int lParam)
{
	tagPAINTSTRUCT ps;

	if (message > 0x14)
	{
		if (message != 792)
			return DefWindowProcA(hWnd, message, wParam, lParam);

		currentSplashScreen->DrawWindow(hWnd, (HDC__*)wParam);
		return 1;
	}
	if (message == 20)
		return 1;
	if (message != 2)
	{
		if (message == 15)
		{
			HDC v7 = BeginPaint(hWnd, (tagPAINTSTRUCT*)&ps);
			currentSplashScreen->DrawWindow(hWnd, v7);
			EndPaint(hWnd, (tagPAINTSTRUCT*)&ps);
			return 0;
		}
		return DefWindowProcA(hWnd, message, wParam, lParam);
	}

	currentSplashScreen->Destroy();

	return 0;
}

/*
====================
idSplashScreen::RegisterClass
====================
*/
ATOM idSplashScreen::RegisterClass(void)
{
	WNDCLASS wc;

	memset(&wc, 0, sizeof(wc));

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)idSplashScreen::WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = win32.hInstance;
	wc.hIcon = LoadIcon(win32.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (struct HBRUSH__*)COLOR_WINDOW;
	wc.lpszMenuName = 0;
	wc.lpszClassName = "idSplashScreen";

	return ::RegisterClass(&wc);
}

void idSysLocal::ShowSplashScreen(bool show) {
	if (show)
	{
		if (currentSplashScreen)
		{
			delete currentSplashScreen;
			currentSplashScreen = nullptr;
		}

		currentSplashScreen = new idSplashScreen();
		currentSplashScreen->Create();
		currentSplashScreen->SetBitmap(IDB_BITMAP_SPLASH, 0);
		AnimateWindow(currentSplashScreen->GetHwnd(), 500, 0x80000);
		SetForegroundWindow(currentSplashScreen->GetHwnd());
	}
	else
	{
		if (currentSplashScreen)
		{
			delete currentSplashScreen;
			currentSplashScreen = nullptr;
		}
	}
}