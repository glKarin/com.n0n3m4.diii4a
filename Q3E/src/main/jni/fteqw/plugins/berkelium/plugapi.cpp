#include "../plugin.h"
#include "../engine.h"

#include "berkelium/Berkelium.hpp"
#include "berkelium/Window.hpp"
#include "berkelium/WindowDelegate.hpp"
#include "berkelium/Context.hpp"

#include <string>

qboolean inited;

class decctx
{
public:
	Berkelium::Window *wnd;
	int width;
	int height;
	unsigned int *buffer;
	bool repainted;

	int paintedwidth;
	int paintedheight;
};

class MyDelegate : public Berkelium::WindowDelegate
{
private:
	decctx *ctx;

	virtual void onCrashedWorker(Berkelium::Window *win)
	{
		int i;
		Con_Printf("Berkelium worker crashed\n");

		/*black it out*/
		for (i = 0; i < ctx->width*ctx->height; i++)
		{
			ctx->buffer[i] = 0xff000000;
		}
		ctx->repainted = true;
	}

	virtual void onCrashed(Berkelium::Window *win)
	{
		int i;
		Con_Printf("Berkelium window crashed\n");

		/*black it out*/
		for (i = 0; i < ctx->width*ctx->height; i++)
		{
			ctx->buffer[i] = 0xff000000;
		}
		ctx->repainted = true;
	}
	virtual void onUnresponsive(Berkelium::Window *win)
	{
		Con_Printf("Berkelium window unresponsive\n");
	}
	virtual void onResponsive(Berkelium::Window *win)
	{
		Con_Printf("Berkelium window responsive again, yay\n");
	}

	virtual void onPaint(Berkelium::Window *wini, const unsigned char *bitmap_in, const Berkelium::Rect &bitmap_rect, size_t num_copy_rects, const Berkelium::Rect *copy_rects, int dx, int dy, const Berkelium::Rect& scroll_rect)
	{
		int i;
		// handle paint events...
		if (dx || dy)
		{
			int y, m;
			int dt = scroll_rect.top();
			int dl = scroll_rect.left();
			int w = scroll_rect.width();
			int h = scroll_rect.height();
			int st = dt - dy;
			int sl = dl - dx;

			/*bound the output rect*/
			if (dt < 0)
			{
				st -= dt;
				h += dt;
				dt = 0;
			}
			if (dl < 0)
			{
				sl -= dl;
				w += dl;
				dl = 0;
			}
			/*bound the source rect*/
			if (st < 0)
			{
				dt -= st;
				h += st;
				st = 0;
			}
			if (sl < 0)
			{
				dl -= sl;
				w += sl;
				sl = 0;
			}
			/*bound the width*/
			m = (dl>sl)?dl:sl;
			if (m + w > ctx->width)
				w = ctx->width - m;
			m = (dt>st)?dt:st;
			if (m + h > ctx->height)
				h = ctx->height - m;

			if (w > 0 && h > 0)
			{
				if (dy > 0)
				{
					//if we're moving downwards, we need to write the bottom before the top (so we don't overwrite the data before its copied)
					for (y = h-1; y >= 0; y--)
					{
						memmove(ctx->buffer + (dl + (dt+y)*ctx->width), ctx->buffer + (sl + (st+y)*ctx->width), w*4);
					}
				}
				else
				{
					//moving upwards requires we write the top row first
					for (y = 0; y < h; y++)
					{
						memmove(ctx->buffer + (dl + (dt+y)*ctx->width), ctx->buffer + (sl + (st+y)*ctx->width), w*4);
					}
				}
			}
		}
		for (i = 0; i < num_copy_rects; i++)
		{
			unsigned int *out = ctx->buffer;
			const unsigned int *in = (const unsigned int*)bitmap_in;
			int x, y;
			int t = copy_rects[i].top();
			int l = copy_rects[i].left();
			int r = copy_rects[i].width() + l;
			int b = copy_rects[i].height() + t;
			int w, h;

			//Clip the rect to the display. This should generally happen anyway, but resizes can be lagged a bit with the whole multi-process/thread thing.
			//don't need to clip to the bitmap rect, that should be correct.
			if (l < 0)
				l = 0;
			if (t < 0)
				t = 0;
			if (r > ctx->width)
				r = ctx->width;
			if (b > ctx->height)
				b = ctx->height;
			w = r - l;
			h = b - t;

			unsigned int instride = bitmap_rect.width() - (w);
			unsigned int outstride = ctx->width - (w);

			out += l;
			out += t * ctx->width;

			in += (l-bitmap_rect.left());
			in += (t-bitmap_rect.top()) * bitmap_rect.width();

			for (y = 0; y < h; y++)
			{
				for (x = 0; x < w; x++)
				{
					*out++ = *in++;
				}
				in += instride;
				out += outstride;
			}
		}

		ctx->repainted = true;
	}

public:
	MyDelegate(decctx *_ctx) : ctx(_ctx) {};
};

static void *Dec_Create(const char *medianame)
{
	/*only respond to berkelium: media prefixes*/
	if (!strncmp(medianame, "berkelium:", 10))
		medianame = medianame + 10;
	else if (!strcmp(medianame, "berkelium"))
		medianame = (char*)"about:blank";
	else if (!strncmp(medianame, "http:", 5) || !strncmp(medianame, "https:", 6))
		medianame = medianame;	//and direct http requests.
	else
		return NULL;

	if (!inited)
	{
		//linux lags behind and apparently returns void, so don't bother checking return values on windows, cos I'm lazy.
		Berkelium::init(Berkelium::FileString::empty());
		inited = qtrue;
	}

	decctx *ctx = new decctx();

	Berkelium::Context* context = Berkelium::Context::create();
	ctx->paintedwidth = ctx->width = 1024;
	ctx->paintedheight = ctx->height = 1024;
	ctx->repainted = false;
	ctx->buffer = (unsigned int*)malloc(ctx->width * ctx->height * 4);
	ctx->wnd = Berkelium::Window::create(context);
	delete context;

	ctx->wnd->setDelegate(new MyDelegate(ctx));


	ctx->wnd->resize(ctx->width, ctx->height);
	std::string url = medianame;
	ctx->wnd->navigateTo(Berkelium::URLString::point_to(url.data(), url.length()));

	return ctx;
}

static qboolean VARGS Dec_DisplayFrame(void *vctx, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ectx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ectx)
{
	decctx *ctx = (decctx*)vctx;
	if (forcevideo || ctx->repainted)
	{
		uploadtexture(ectx, TF_BGRA32, ctx->width, ctx->height, ctx->buffer, NULL);
		ctx->paintedwidth = ctx->width;
		ctx->paintedheight = ctx->height;
		ctx->repainted = false;
	}
	return qtrue;
}
static void Dec_Destroy(void *vctx)
{
	decctx *ctx = (decctx*)vctx;
	if (inited)	//make sure things don't happen in the wrong order. we can still leak though
		ctx->wnd->destroy();
	delete ctx;
}
static void Dec_GetSize (void *vctx, int *width, int *height)
{
	decctx *ctx = (decctx*)vctx;
	if (ctx->repainted)
	{
		*width = ctx->width;
		*height = ctx->height;
	}
	else
	{
		*width = ctx->paintedwidth;
		*height = ctx->paintedheight;
	}
}
static qboolean Dec_SetSize (void *vctx, int width, int height)
{
	decctx *ctx = (decctx*)vctx;
	if (width < 4)
		width = 4;
	if (height < 4)
		height = 4;
	if (ctx->width == width && ctx->height == height)
		return qtrue;	//no point

	//there's no resize notification. apparently javascript cannot resize windows. yay.
	unsigned int *newbuf = (unsigned int*)realloc(ctx->buffer, width * height * sizeof(*newbuf));
	if (!newbuf)
		return qfalse;	//failed?!?
	ctx->width = width;
	ctx->height = height;
	ctx->buffer = newbuf;
	ctx->repainted = false;

	ctx->wnd->resize(ctx->width, ctx->height);

	return qtrue;
}
static void Dec_CursorMove (void *vctx, float posx, float posy)
{
	decctx *ctx = (decctx*)vctx;
	ctx->wnd->mouseMoved((int)(posx * ctx->width), (int)(posy * ctx->height));
}
static void Dec_Key (void *vctx, int code, int unicode, int isup)
{
	decctx *ctx = (decctx*)vctx;
	wchar_t wchr = unicode;

	if (code >= 178 && code < 178+6)
	{
		code = code - 178;
		//swap mouse2+3
		if (code == 1)
			code = 2;
		else if (code == 2)
			code = 1;
		ctx->wnd->mouseButton(code, !isup);
	}
	else if (code == 188 || code == 189)
	{
		if (!isup)
			ctx->wnd->mouseWheel(0, (code==189)?-30:30);
	}
	else
	{
		int mods = 0;
		if (code == 127)
			code = 0x08;
		else if (code == 140)	//del
			code = 0x2e;
		else if (code == 143)	//home
			code = 0x24;
		else if (code == 144)	//end
			code = 0x23;
		else if (code == 141)	//pgdn
			code = 0x22;
		else if (code == 142)	//pgup
			code = 0x21;
		else if (code == 139)	//ins
			code = 0x2d;
		else if (code == 132)	//up
			code = 0x26;
		else if (code == 133)	//down
			code = 0x28;
		else if (code == 134)	//left
			code = 0x25;
		else if (code == 135)	//right
			code = 0x27;
		if (code)
			ctx->wnd->keyEvent(!isup, mods, code, 0);
		if (unicode && !isup)
		{
			wchar_t chars[2] = {unicode};
			if (unicode == 127 || unicode == 8 || unicode == 9 || unicode == 27)
				return;
			ctx->wnd->textEvent(chars, 1);
		}
	}
}

static void Dec_ChangeStream(void *vctx, const char *newstream)
{
	decctx *ctx = (decctx*)vctx;

	if (!strncmp(newstream, "cmd:", 4))
	{
		if (!strcmp(newstream+4, "refresh"))
			ctx->wnd->refresh();
		else if (!strcmp(newstream+4, "transparent"))
			ctx->wnd->setTransparent(true);
		else if (!strcmp(newstream+4, "focus"))
			ctx->wnd->focus();
		else if (!strcmp(newstream+4, "unfocus"))
			ctx->wnd->unfocus();
		else if (!strcmp(newstream+4, "opaque"))
			ctx->wnd->setTransparent(false);
		else if (!strcmp(newstream+4, "stop"))
			ctx->wnd->stop();
		else if (!strcmp(newstream+4, "back"))
			ctx->wnd->goBack();
		else if (!strcmp(newstream+4, "forward"))
			ctx->wnd->goForward();
		else if (!strcmp(newstream+4, "cut"))
			ctx->wnd->cut();
		else if (!strcmp(newstream+4, "copy"))
			ctx->wnd->copy();
		else if (!strcmp(newstream+4, "paste"))
			ctx->wnd->paste();
		else if (!strcmp(newstream+4, "del"))
			ctx->wnd->del();
		else if (!strcmp(newstream+4, "selectall"))
			ctx->wnd->selectAll();
	}
	else if (!strncmp(newstream, "javascript:", 11))
	{
		newstream+=11;
		int len = mblen(newstream, MB_CUR_MAX);
		wchar_t *wchrs = (wchar_t *)malloc((len+1)*2);
		len = mbstowcs(wchrs, newstream, len);
		ctx->wnd->executeJavascript(Berkelium::WideString::point_to(wchrs, len));
		free(wchrs);
	}
	else
	{
		std::string url = newstream;
		ctx->wnd->navigateTo(Berkelium::URLString::point_to(url.data(), url.length()));
	}
}

static bool Dec_Init(void)
{
	return true;
}

static qintptr_t Dec_Tick(qintptr_t *args)
{
	//need to keep it ticking over, if any work is to be done.
	if (inited)
		Berkelium::update();
	return 0;
}

static qintptr_t Dec_Shutdown(qintptr_t *args)
{
	//force-kill all.
	if (inited)
		Berkelium::destroy();
	inited = qfalse;
	return 0;
}

static media_decoder_funcs_t decoderfuncs =
{
	sizeof(media_decoder_funcs_t),

	"berkelium",
	Dec_Create,
	Dec_DisplayFrame,
	Dec_Destroy,
	NULL,//rewind

	Dec_CursorMove,
	Dec_Key,
	Dec_SetSize,
	Dec_GetSize,
	Dec_ChangeStream
};

extern "C" qintptr_t Plug_Init(qintptr_t *args)
{
	if (!Plug_Export("Tick", Dec_Tick))
	{
		Con_Printf("Berkelium plugin failed: Engine doesn't support Tick feature\n");
		return false;
	}
	if (!Plug_Export("Shutdown", Dec_Shutdown))
	{
		Con_Printf("Berkelium plugin failed: Engine doesn't support Shutdown feature\n");
		return false;
	}
	if (!pPlug_ExportNative("Media_VideoDecoder", &decoderfuncs))
	{
		Con_Printf("Berkelium plugin failed: Engine doesn't support media decoder plugins\n");
		return false;
	}
	return Dec_Init();
}

