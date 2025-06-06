/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "SeriousSam/StdH.h"
#include "resource.h"

#ifdef PLATFORM_WIN32
#define NAME "Splash"
static HBITMAP _hbmSplash = NULL;
static BITMAP _bmSplash;
static HBITMAP _hbmSplashMask = NULL;
static BITMAP _bmSplashMask;
static HWND hwnd = NULL;

static LONG_PTR FAR PASCAL SplashWindowProc( HWND hWnd, UINT message, 
			    WPARAM wParam, LPARAM lParam )
{
  switch( message ) {
  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(hWnd, &ps); 

    HDC hdcMem = CreateCompatibleDC(ps.hdc); 
    SelectObject(hdcMem, _hbmSplashMask); 
    BitBlt(ps.hdc, 0, 0, _bmSplash.bmWidth, _bmSplash.bmHeight, hdcMem, 0, 0, 
      SRCAND); 
    SelectObject(hdcMem, _hbmSplash); 
    BitBlt(ps.hdc, 0, 0, _bmSplash.bmWidth, _bmSplash.bmHeight, hdcMem, 0, 0, 
      SRCPAINT); 

    DeleteDC(hdcMem); 
    EndPaint(hWnd, &ps); 
   
    return 0;
                 } break;
  case WM_ERASEBKGND: return 1; break;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

void ShowSplashScreen(HINSTANCE hInstance)
{
  _hbmSplash = LoadBitmapA(hInstance, (char*)IDB_SPLASH);
  if (_hbmSplash==NULL) {
    return;
  }
  _hbmSplashMask = LoadBitmapA(hInstance, (char*)IDB_SPLASHMASK);
  if (_hbmSplashMask==NULL) {
    return;
  }

  GetObject(_hbmSplash, sizeof(BITMAP), (LPSTR) &_bmSplash); 
  GetObject(_hbmSplashMask, sizeof(BITMAP), (LPSTR) &_bmSplashMask);
  if (_bmSplashMask.bmWidth  != _bmSplash.bmWidth
    ||_bmSplashMask.bmHeight != _bmSplash.bmHeight) {
    return;
  }

	int iScreenX = ::GetSystemMetrics(SM_CXSCREEN);	// screen size
	int iScreenY = ::GetSystemMetrics(SM_CYSCREEN);

  WNDCLASSA wc;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC)SplashWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon( hInstance, (LPCTSTR)IDR_MAINFRAME );
  wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NAME;
  wc.lpszClassName = NAME;
  RegisterClassA(&wc);

  /*
   * create a window
   */
  hwnd = CreateWindowExA(
	  WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW,
	  NAME,
	  "SeriousSam loading...",   // title
    WS_POPUP,
	  iScreenX/2-_bmSplash.bmWidth/2,
	  iScreenY/2-_bmSplash.bmHeight/2,
	  _bmSplash.bmWidth,_bmSplash.bmHeight,  // window size
	  NULL,
	  NULL,
	  hInstance,
	  NULL);

  if(!hwnd) {
	  return;
  }
 
  ShowWindow( hwnd, SW_SHOW);
  RECT rect;
  GetClientRect(hwnd, &rect); 
  InvalidateRect(hwnd, &rect, TRUE); 
  UpdateWindow(hwnd); 
}

void HideSplashScreen(void)
{
  if (hwnd==NULL) {
    return;
  }
  DestroyWindow(hwnd);
  DeleteObject(_hbmSplash);
  DeleteObject(_hbmSplashMask);
}

#elif defined(_DIII4A) //karin: no splash
void HideSplashScreen(void){}
void ShowSplashScreen(HINSTANCE hInstance){}
#else  // this is the non-win32 code, below.

#include "SDL_shape.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

void HideSplashScreen(void)
{
  if (texture) { SDL_DestroyTexture(texture); texture = NULL; }
  if (renderer) { SDL_DestroyRenderer(renderer); renderer = NULL; }
  if (window) { SDL_DestroyWindow(window); window = NULL; }
}

void ShowSplashScreen(HINSTANCE hInstance)
{
  SDL_Surface *bmp = SDL_LoadBMP("splash.bmp");
  if (!bmp) {
    return;
  }

  SDL_Surface *bmpmask = SDL_LoadBMP("splashmask.bmp");
  if (bmpmask) {
    if ((bmpmask->w != bmp->w) || (bmpmask->h != bmp->h)) {
      SDL_FreeSurface(bmpmask);
      SDL_FreeSurface(bmp);
      return;
    }
    window = SDL_CreateShapedWindow("SeriousSam loading...", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, bmp->w, bmp->h, SDL_WINDOW_BORDERLESS); // RAKE!: commented out as its post SDL2.0.4 |SDL_WINDOW_SKIP_TASKBAR);
    if (window) {
      SDL_WindowShapeMode mode;
      SDL_zero(mode);
      mode.mode = ShapeModeColorKey;
      SDL_GetRGBA(SDL_MapRGB(bmpmask->format, 0xFF, 0xFF, 0xFF), bmpmask->format, &mode.parameters.colorKey.r, &mode.parameters.colorKey.g, &mode.parameters.colorKey.b, &mode.parameters.colorKey.a);
      if (SDL_SetWindowShape(window, bmpmask, &mode) != 0) {
        SDL_DestroyWindow(window);
        window = NULL;
      }
    }
    SDL_FreeSurface(bmpmask);
  }

  if (!window) {
    window = SDL_CreateWindow("SeriousSam loading...", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, bmp->w, bmp->h, SDL_WINDOW_BORDERLESS); // RAKE!: commented out as its post SDL2.0.4 |SDL_WINDOW_SKIP_TASKBAR);
  }

  bool okay = false;
  if (window) {
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      SDL_RenderClear(renderer);
      SDL_RenderPresent(renderer);

      texture = SDL_CreateTextureFromSurface(renderer, bmp);
      if (texture) {
        okay = true;
      }
    }
  }

  SDL_FreeSurface(bmp);

  if (!okay) {
    HideSplashScreen();
  } else {
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }
}

#endif

