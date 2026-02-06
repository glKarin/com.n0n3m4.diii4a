//fte defs
#include "../plugin.h"
#include "../engine.h"

static plugfsfuncs_t *fsfuncs;
static plugsubconsolefuncs_t *confuncs;
static plugclientfuncs_t *clientfuncs;

#pragma GCC diagnostic ignored "-Wstrict-prototypes"	//not my bug.

//libcef defs
#include "include/cef_version.h"
#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_parser_capi.h"
#include "include/capi/cef_request_context_handler_capi.h"
//#include "include/capi/cef_url_capi.h"
#include "assert.h"
#if defined(_DEBUG) && defined(_MSC_VER)
	#include <crtdbg.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#define EXPECTED_COMMIT_NUMBER  2179	//last version of libcef we tried building against...
#if EXPECTED_COMMIT_NUMBER != EXPECTED_COMMIT_NUMBER
	#warning "libcef version different from expected. expect problems with libcef's unstable API."
#endif

#define cef_addref(ptr)		(ptr)->base.add_ref(&(ptr)->base)
#define cef_release(ptr)	(((ptr)->base.release)(&(ptr)->base))

#if !defined(LIBCEF_STATIC) && !defined(LIBCEF_DYNAMIC)
	#define LIBCEF_DYNAMIC
#endif
#ifdef LIBCEF_DYNAMIC
//avoid conflicts with cef headers
#define cef_version_info						pcef_version_info
#define cef_initialize							pcef_initialize
#define cef_do_message_loop_work				pcef_do_message_loop_work
#define cef_shutdown							pcef_shutdown
#define cef_execute_process						pcef_execute_process
#define cef_browser_host_create_browser_sync	pcef_browser_host_create_browser_sync
#define cef_string_utf8_to_utf16				pcef_string_utf8_to_utf16
#define cef_string_utf16_to_utf8				pcef_string_utf16_to_utf8
#define cef_string_utf16_clear					pcef_string_utf16_clear
#define cef_string_utf16_set					pcef_string_utf16_set
#define cef_string_utf8_clear					pcef_string_utf8_clear
#define cef_string_utf8_set						pcef_string_utf8_set
#define cef_string_userfree_utf16_free			pcef_string_userfree_utf16_free
#define cef_register_scheme_handler_factory		pcef_register_scheme_handler_factory
#define cef_get_mime_type						pcef_get_mime_type
#define cef_v8value_create_function				pcef_v8value_create_function
#define cef_v8value_create_string				pcef_v8value_create_string
#define cef_process_message_create				pcef_process_message_create
#define cef_v8context_get_current_context		pcef_v8context_get_current_context
#define	cef_post_task							pcef_post_task
#define cef_request_context_create_context		pcef_request_context_create_context
#define cef_string_multimap_alloc				pcef_string_multimap_alloc
#define cef_string_multimap_append				pcef_string_multimap_append
#define cef_string_multimap_size				pcef_string_multimap_size
#define cef_string_multimap_key					pcef_string_multimap_key
#define cef_string_multimap_value				pcef_string_multimap_value
#define cef_string_multimap_free				pcef_string_multimap_free
#define cef_string_list_size					pcef_string_list_size
#define cef_string_list_value					pcef_string_list_value

static int						(*cef_version_info)(int entry);
static int						(*cef_initialize)(const struct _cef_main_args_t* args, const cef_settings_t* settings, cef_app_t* application, void* windows_sandbox_info);
static void						(*cef_do_message_loop_work)(void);
static void						(*cef_shutdown)(void);
static int						(*cef_execute_process)(const cef_main_args_t* args, cef_app_t* application, void* windows_sandbox_info);
static cef_browser_t*			(*cef_browser_host_create_browser_sync)(const cef_window_info_t* windowInfo, cef_client_t* client, const cef_string_t* url, const cef_browser_settings_t* settings, cef_dictionary_value_t* extra_info, cef_request_context_t* request_context);
static int						(*cef_string_utf8_to_utf16)(const char* src, size_t src_len, cef_string_utf16_t* output);
static int						(*cef_string_utf16_to_utf8)(const char16* src, size_t src_len, cef_string_utf8_t* output);
static void						(*cef_string_utf16_clear)(cef_string_utf16_t* str);
static int						(*cef_string_utf16_set)(const char16* src, size_t src_len, cef_string_utf16_t* output, int copy);
static void						(*cef_string_utf8_clear)(cef_string_utf8_t* str);
static int						(*cef_string_utf8_set)(const char* src, size_t src_len, cef_string_utf8_t* output, int copy);
static void						(*cef_string_userfree_utf16_free)(cef_string_userfree_utf16_t str);
static int						(*cef_register_scheme_handler_factory)(const cef_string_t* scheme_name, const cef_string_t* domain_name, cef_scheme_handler_factory_t* factory);
static cef_string_userfree_t	(*cef_get_mime_type)(const cef_string_t* extension);
static cef_v8value_t*			(*cef_v8value_create_function)(const cef_string_t* name, cef_v8handler_t* handler);
static cef_v8value_t*			(*cef_v8value_create_string)(const cef_string_t* value);
static cef_process_message_t*	(*cef_process_message_create)(const cef_string_t* name);
static cef_v8context_t*			(*cef_v8context_get_current_context)(void);	//typical C++ programmers omitted the void.
static int						(*cef_post_task)(cef_thread_id_t threadId, cef_task_t* task);
static cef_request_context_t*	(*cef_request_context_create_context)(const cef_request_context_settings_t* settings, cef_request_context_handler_t* handler);
static cef_string_multimap_t	(*cef_string_multimap_alloc)(void);
static int						(*cef_string_multimap_append)(cef_string_multimap_t map, const cef_string_t* key, const cef_string_t* value);
static size_t					(*cef_string_multimap_size)(cef_string_multimap_t map);
static int						(*cef_string_multimap_key)(cef_string_multimap_t map, size_t index, cef_string_t* key);
static int						(*cef_string_multimap_value)(cef_string_multimap_t map, size_t index, cef_string_t* value);
static void						(*cef_string_multimap_free)(cef_string_multimap_t map);
static size_t					(*cef_string_list_size)(cef_string_list_t list);
static int						(*cef_string_list_value)(cef_string_list_t list, size_t index, cef_string_t* value);
#endif

static cvar_t	*cef_incognito;
static cvar_t	*cef_allowplugins;
static cvar_t	*cef_allowcvars;
static cvar_t	*cef_devtools;

static char plugname[MAX_OSPATH];
static char *newconsole;

/*static void setcefstring(char *str, cef_string_t *r)
{
	cef_string_from_utf8(str, strlen(str), r);
}*/
static cef_string_t makecefstring(char *str)
{
	cef_string_t r = {NULL};
	cef_string_from_utf8(str, strlen(str), &r);
	return r;
}
static cef_string_t *makecefstringptr(char *str, cef_string_t *ptr)
{
	cef_string_from_utf8(str, strlen(str), ptr);
	return ptr;
}

static char *Info_JSONify (char *s, char *o, size_t outlen)
{
	outlen--;	//so we don't have to consider nulls

	if (*s == '\\')
		s++;
	while (*s)
	{
		//min overhead
		if (outlen < 6)
			break;
		outlen -= 6;

		*o++ = ',';
		*o++ = '\"';
		for (; *s && *s != '\\'; s++)
		{
			if (*s != '\"' && outlen)
			{
				outlen--;
				*o++ = *s;
			}
		}
		*o++ = '\"';
		*o++ = ':';
		*o++ = '\"';

		if (!*s++)
		{
			//should never happen.
			*o++ = '\"';
			*o = 0;
			return o;
		}

		for (; *s && *s != '\\'; s++)
		{
			if (*s != '\"' && outlen)
			{
				outlen--;
				*o++ = *s;
			}
		}

		*o++ = '\"';

		if (*s)
			s++;
	}
	*o = 0;
	return o;
}

#ifdef _MSC_VER
	#define atomic_fetch_add(p,v) (InterlockedIncrement(p)-1)
	#define atomic_fetch_sub(p,v) (InterlockedDecrement(p)+1)
	#define atomic_uint32_t LONG
#else
	#define atomic_fetch_add __sync_fetch_and_add
	#define atomic_fetch_sub __sync_fetch_and_sub
	#define atomic_uint32_t unsigned int
#endif

typedef struct
{
	atomic_uint32_t refcount;	//this needs to be atomic to avoid multiple threads adding at the same time
	//cef interface objects
	cef_client_t client;
	cef_render_handler_t render_handler;
	cef_display_handler_t display_handler;
	cef_request_handler_t request_handler;
	cef_life_span_handler_t life_span_handler;
	cef_context_menu_handler_t context_menu_handler;
	cef_browser_t *thebrowser;

	void *videodata;
	int videowidth;
	int videoheight;
	qboolean updated;
	int desiredwidth;
	int desiredheight;
	char *consolename;	//for internal plugin use.
	qboolean fullscreen;
	cef_string_utf8_t currenturl;
	cef_string_utf8_t currenticon;
	cef_string_utf8_t currenttitle;
	cef_string_utf8_t currentstatus;

	cef_mouse_event_t mousepos;
	unsigned char keystate[K_MAX];
} browser_t;
unsigned int numbrowsers;

static void browser_addref(browser_t *br)
{
	atomic_fetch_add(&br->refcount, 1);
}
static int browser_release(browser_t *br)
{
	if (atomic_fetch_sub(&br->refcount, 1) == 1)
	{
		if (br->consolename)
			free(br->consolename);
		if (br->videodata)
			free(br->videodata);
		cef_string_utf8_clear(&br->currenturl);
		cef_string_utf8_clear(&br->currenticon);
		cef_string_utf8_clear(&br->currenttitle);
		cef_string_utf8_clear(&br->currentstatus);

		numbrowsers--;

		free(br);
		return true;
	}
	return false;
}

#define browser_subs(sub) \
	static void CEF_CALLBACK browser_##sub##_addref(cef_base_ref_counted_t* self) {browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, sub.base)); browser_addref(br);};	\
	static int CEF_CALLBACK browser_##sub##_release(cef_base_ref_counted_t* self) {browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, sub.base)); return browser_release(br);};	\
	static int CEF_CALLBACK browser_##sub##_hasoneref(cef_base_ref_counted_t* self) {browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, sub.base)); return br->refcount == 1;};
browser_subs(client);
browser_subs(render_handler);
browser_subs(display_handler);
browser_subs(request_handler);
browser_subs(life_span_handler);
browser_subs(context_menu_handler);
#undef browser_subs

//client methods
static cef_render_handler_t *CEF_CALLBACK browser_get_render_handler(cef_client_t *self)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, client));
	cef_addref(&br->render_handler);
	return &br->render_handler;
}
static cef_life_span_handler_t *CEF_CALLBACK browser_get_life_span_handler(cef_client_t *self)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, client));
	cef_addref(&br->life_span_handler);
	return &br->life_span_handler;
}
static cef_context_menu_handler_t *CEF_CALLBACK browser_get_context_menu_handler(cef_client_t *self)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, client));
	cef_addref(&br->context_menu_handler);
	return &br->context_menu_handler;
}
static cef_display_handler_t *CEF_CALLBACK browser_get_display_handler(cef_client_t *self)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, client));
	cef_addref(&br->display_handler);
	return &br->display_handler;
}
static cef_request_handler_t *CEF_CALLBACK browser_get_request_handler(cef_client_t *self)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, client));
	cef_addref(&br->request_handler);
	return &br->request_handler;
}

static qboolean browser_handle_query(const char *req, char *buffer, size_t buffersize)
{
	if (!req)
		return false;
	else if (!strncmp(req, "getcvar_", 8))
	{
		if (cef_allowcvars->value && cvarfuncs->GetString(req+8, buffer, buffersize))
			return true;
	}
	else if (!strncmp(req, "setcvar_", 8))
	{
		const char *eq = strchr(req+8, '=');
		if (eq)
			*(char*)eq++ = 0;
		else
			eq = req+strlen(req);

		if (cef_allowcvars->value)
		{
			cvarfuncs->SetString(req+8, eq);
			*buffer = 0;
			return true;
		}
	}
	else if (!strcmp(req, "getstats"))
	{	//1 [, one sign, 10 chars, one ], one comma
		//FIXME: should be more than just a one-off.
		unsigned int stats[256], i, m;
		char *e = buffer;
		m = clientfuncs->GetStats(0, stats, countof(stats));
		if (!m)
		{
			m = 0;
			stats[m++] = 0;
		}

		*e++ = '[';
		for (i = 0; i < m; i++)
		{
			if (i)
				*e++ = ',';
			
			sprintf(e, "%i", (int)stats[i]);
			e += strlen(e);
		}
		*e++ = ']';
		*e = 0;
		assert(e <= buffer + buffersize);
		return true;
	}
	else if (!strcmp(req, "getseats"))
	{
		int i;
		char *e = buffer;
		int players[MAX_SPLITS];
		int tracks[MAX_SPLITS];
		int seats = clientfuncs->GetLocalPlayerNumbers(0, MAX_SPLITS, players, tracks);
		*e++ = '[';
		for (i = 0; i < seats; i++)
		{
			if (i)
				*e++ = ',';
			sprintf(e, "{\"player\":%i,\"track\":%i}", players[i], tracks[i]); e += strlen(e);
		}
		*e++ = ']';
		*e = 0;
		assert(e <= buffer + buffersize);
		return true;
	}
	else if (!strcmp(req, "getserverinfo"))
	{
		char serverinfo[4096];
		char *e = buffer;
		clientfuncs->GetServerInfoRaw(serverinfo, sizeof(serverinfo));
		e = Info_JSONify(serverinfo, e, buffer + buffersize - e-1);
		if (e == buffer) e++;
		*buffer = '{';
		*e++ = '}';
		*e = 0;
		assert(e <= buffer + buffersize);
		return true;
	}
	else if (!strcmp(req, "getplayers"))
	{
		unsigned int i;
		char *e = buffer;
		plugclientinfo_t info;

		*e++ = '[';
		for (i = 0; ; i++)
		{
			clientfuncs->GetPlayerInfo(i, &info);

			if (buffer + buffersize - e-1 < 100)
				break;

			if (i)
				*e++ = ',';
			*e++ = '{';
			//splurge the specific info
			sprintf(e, "\"frags\":%i,\"ping\":%i,\"pl\":%i,\"active\":%i,\"userid\":%i", info.frags, info.ping, info.pl, info.activetime, info.userid);
			e += strlen(e);
			//splurge the generic info (colours, name, team)
			e = Info_JSONify(info.userinfo, e, buffer + buffersize - e-1);
			*e++ = '}';
		}
		*e++ = ']';
		*e = 0;
		assert(e <= buffer + buffersize);
		return true;
	}

	return false;
}

static int CEF_CALLBACK browser_on_process_message_received(cef_client_t* self, cef_browser_t* browser, cef_frame_t* frame, cef_process_id_t source_process, cef_process_message_t* message)
{
	int handled = false;
//	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, request_handler));
	cef_string_userfree_t msgnameunusable = message->get_name(message);
	cef_string_utf8_t name = {NULL};
	cef_string_to_utf8(msgnameunusable->str, msgnameunusable->length, &name);
	if (!strcmp(name.str, "fte_query"))
	{
		char buffer[8192];
		int id1, id2;
		cef_process_message_t *reply;
		cef_string_utf8_t queryname = {NULL};
		cef_string_t str = {NULL};
		cef_list_value_t *args = message->get_argument_list(message);
		cef_string_userfree_t cmdunusable = args->get_string(args, 0);
		cef_string_to_utf8(cmdunusable?cmdunusable->str:NULL, cmdunusable?cmdunusable->length:0, &queryname);

		id1 = args->get_int(args, 2);
		id2 = args->get_int(args, 3);
		cef_release(args);

		reply = cef_process_message_create(msgnameunusable);
		args = reply->get_argument_list(reply);
		args->set_string(args, 0, cmdunusable);
		args->set_int(args, 2, id1);
		args->set_int(args, 3, id2);

		if (browser_handle_query(queryname.str, buffer, sizeof(buffer)))
		{
			str = makecefstring(buffer);
			args->set_string(args, 1, &str);
			cef_string_clear(&str);
		}
		else
			args->set_null(args, 1);
		cef_release(args);
		cef_string_utf8_clear(&queryname);
		if (cmdunusable)
			cef_string_userfree_free(cmdunusable);
		frame->send_process_message(frame, source_process, reply);
		handled = true;
	}
	cef_release(message);
	cef_release(browser);
	cef_string_utf8_clear(&name);
	cef_string_userfree_free(msgnameunusable);
//	if (messagerouter->OnProcessMessageReceived(browser, source_process, message))
//		return true;
	return handled;
}

//render_handler methods
static void CEF_CALLBACK browser_get_view_rect(cef_render_handler_t *self, cef_browser_t *browser, cef_rect_t *rect)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, render_handler));

	rect->x = 0;
	rect->y = 0;
	rect->width = br->desiredwidth;
	rect->height = br->desiredheight;
	cef_release(browser);
}
static void CEF_CALLBACK browser_on_paint(cef_render_handler_t *self, cef_browser_t *browser, cef_paint_element_type_t type, size_t dirtyRectsCount, cef_rect_t const* dirtyRects, const void* buffer, int width, int height)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, render_handler));

	cef_release(browser);

	//popups are gonna be so awkward...
	if (type != PET_VIEW)
		return;

	/*
	if (pbocontext)
	{
		if (!PBO_Lock(pbocontext, width, height, &lost, rgba))
			return;
		if (lost)
			PBO_Update(pbocontext, buffer, width, height, stride);
		else while (dirtyRectsCount --> 0)
			PBO_Update(pbocontext, (char*)buffer+(width*dirtyRects->y + dirtyRects->x)*4), dirtyRects->width, dirtyRects->height, width*4);
		PBO_Unlock(pbocontext);
	}
	else
	*/

	if (br->videowidth != width || br->videoheight != height)
	{	//copy the entire thing.
		if (br->videodata)
			free(br->videodata);
		br->videowidth = width;
		br->videoheight = height;
		br->videodata = malloc(width * height * 4);
		memcpy(br->videodata, buffer, width * height * 4);
	}
	else
	{	//try to save cpu time by copying only the dirty parts
		while (dirtyRectsCount --> 0)
		{
			if (width == dirtyRects->width && height == dirtyRects->height)
				memcpy(br->videodata, buffer, width * height * 4);
			else
			{
				int y;
				const unsigned int *src;
				unsigned int *dst;
				src = buffer;
				src += width * dirtyRects->y + dirtyRects->x;
				dst = br->videodata;
				dst += width * dirtyRects->y + dirtyRects->x;

				for (y = 0; y < dirtyRects->height; y++)
				{
					memcpy(dst, src, dirtyRects->width*4);
					src += width;
					dst += width;
				}
			}
			dirtyRects++;
		}
	}
	br->updated = true;
}

static void CEF_CALLBACK browser_on_before_close(cef_life_span_handler_t* self, cef_browser_t* browser)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, life_span_handler));
	if (br->thebrowser)
	{	//we may have already released our reference to this, if something else was blocking for some reason.
		cef_release(br->thebrowser);
		br->thebrowser = NULL;
	}
	cef_release(browser);
}

//context_menu_handler methods
static void CEF_CALLBACK browser_on_before_context_menu(struct _cef_context_menu_handler_t* self, struct _cef_browser_t* browser, struct _cef_frame_t* frame, struct _cef_context_menu_params_t* params, struct _cef_menu_model_t* model)
{
//	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, context_menu_handler));

	//wipe whatever elements libcef thinks it should add
	model->clear(model);

	//don't bother adding any new ones.


	cef_release(browser);
	cef_release(frame);
	cef_release(params);
	cef_release(model);
}

//display_handler methods
//redirect console.log messages to quake's console, but only display them if we've got developer set.
static int CEF_CALLBACK browser_on_console_message(cef_display_handler_t* self, cef_browser_t* browser, cef_log_severity_t level, const cef_string_t* message, const cef_string_t* source, int line)
{
	cef_string_utf8_t u8_source = {NULL};
	cef_string_utf8_t u8_message = {NULL};
	if (source)
		cef_string_to_utf8(source->str, source->length, &u8_source);
	if (message)
		cef_string_to_utf8(message->str, message->length, &u8_message);

	Con_DPrintf("%s:%i: %s\n", u8_source.str, line, u8_message.str);

	cef_string_utf8_clear(&u8_source);
	cef_string_utf8_clear(&u8_message);
	cef_release(browser);
	return true;
}
static void CEF_CALLBACK browser_on_title_change(cef_display_handler_t* self, cef_browser_t* browser, const cef_string_t* title)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, display_handler));
	if (title)
		cef_string_to_utf8(title->str, title->length, &br->currenttitle);
	else
		cef_string_utf8_copy(br->currenturl.str, br->currenturl.length, &br->currenttitle);

	if (br->consolename)
		confuncs->SetConsoleString(br->consolename, "title", br->currenttitle.str?br->currenttitle.str:"");

	cef_release(browser);
}
static void CEF_CALLBACK browser_on_favicon_urlchange(cef_display_handler_t* self, cef_browser_t* browser, cef_string_list_t favicon)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, display_handler));
	cef_string_t str = {NULL};

	if (favicon)
	{
		//size_t opts = cef_string_list_size(favicon);
		cef_string_list_value(favicon, 0, &str);
	}

	cef_string_to_utf8(str.str, str.length, &br->currenticon);
	cef_string_clear(&str);

	if (br->consolename)
		confuncs->SetConsoleString(br->consolename, "icon", br->currenticon.str?br->currenticon.str:"");

	cef_release(browser);
}
static void CEF_CALLBACK browser_on_fullscreenmode_change(cef_display_handler_t* self, cef_browser_t* browser, int fullscreen)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, display_handler));
	br->fullscreen = fullscreen;

	if (br->consolename)
		confuncs->SetConsoleFloat(br->consolename, "fullscreen", br->fullscreen);

	cef_release(browser);
}
static void CEF_CALLBACK browser_on_address_change(cef_display_handler_t* self, cef_browser_t* browser, cef_frame_t* frame, const cef_string_t* url)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, display_handler));

	cef_string_to_utf8(url->str, url->length, &br->currenturl);

	if (br->currenticon.length)
	{
		cef_string_utf8_clear(&br->currenticon);
		if (br->consolename)
			confuncs->SetConsoleString(br->consolename, "icon", br->currenticon.str?br->currenticon.str:"");
	}

	//FIXME: should probably make sure its the root frame
//	Con_Printf("new url: %s\n", url.ToString().c_str());
	cef_release(browser);
	cef_release(frame);
}
static int CEF_CALLBACK browser_on_tooltip(cef_display_handler_t* self, cef_browser_t* browser, cef_string_t* text)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, display_handler));
	if (br->consolename)
	{
		cef_string_utf8_t u8_text = {NULL};
		cef_string_to_utf8(text->str, text->length, &u8_text);
		confuncs->SetConsoleString(br->consolename, "tooltip", u8_text.str?u8_text.str:"");
		cef_string_utf8_clear(&u8_text);
	}
	cef_release(browser);
	return true;	//cef won't draw tooltips when running like this
}
static void CEF_CALLBACK browser_on_status_message(cef_display_handler_t* self, cef_browser_t* browser, const cef_string_t* value)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, display_handler));
	if (br->consolename)
	{
		cef_string_utf8_t u8_value = {NULL};
		if (value)
			cef_string_to_utf8(value->str, value->length, &u8_value);
		confuncs->SetConsoleString(br->consolename, "footer", u8_value.str?u8_value.str:"");
		cef_string_utf8_clear(&u8_value);
	}

	cef_release(browser);
}

//request_handler methods


static int CEF_CALLBACK browser_on_before_browse(cef_request_handler_t* self, cef_browser_t* browser, cef_frame_t* frame, cef_request_t* request, int user_gesture, int is_redirect)
{
//	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, request_handler));

	cef_release(browser);
	cef_release(frame);
	cef_release(request);

	return false;	//false = allow navigation, true = block
}
static void CEF_CALLBACK browser_on_render_process_terminated(cef_request_handler_t* self, cef_browser_t* browser, cef_termination_status_t status)
{
	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, request_handler));
	if (br->videodata)
		free(br->videodata);
	br->videowidth = 1;
	br->videoheight = 1;
	br->videodata = malloc(br->videowidth * br->videoheight * 4);
	memset(br->videodata, 0, br->videowidth * br->videoheight * 4);
	br->updated = true;

	cef_release(browser);
}


static browser_t *browser_create(void)
{
	browser_t *nb = malloc(sizeof(*nb));
	memset(nb, 0, sizeof(*nb));
	nb->refcount = 1;

#define browser_subs(sub) \
	nb->sub.base.add_ref = browser_##sub##_addref;		\
	nb->sub.base.release = browser_##sub##_release;	\
	nb->sub.base.has_one_ref = browser_##sub##_hasoneref;	\
	nb->sub.base.size = sizeof(nb->sub);
browser_subs(client);
browser_subs(render_handler);
browser_subs(display_handler);
browser_subs(request_handler);
browser_subs(life_span_handler);
browser_subs(context_menu_handler);
#undef browser_subs

	nb->client.get_context_menu_handler = browser_get_context_menu_handler;
	nb->client.get_dialog_handler = NULL;
	nb->client.get_display_handler = browser_get_display_handler;
	nb->client.get_download_handler = NULL;
	nb->client.get_drag_handler = NULL;
	nb->client.get_find_handler = NULL;
	nb->client.get_focus_handler = NULL;
	nb->client.get_jsdialog_handler = NULL;
	nb->client.get_keyboard_handler = NULL;
	nb->client.get_life_span_handler = browser_get_life_span_handler;
	nb->client.get_load_handler = NULL;
	nb->client.get_render_handler = browser_get_render_handler;
	nb->client.get_request_handler = browser_get_request_handler;
	nb->client.on_process_message_received = browser_on_process_message_received;

//	nb->render_handler.get_accessibility_handler = NULL;
	nb->render_handler.get_root_screen_rect = NULL;
	nb->render_handler.get_view_rect = browser_get_view_rect;
	nb->render_handler.get_screen_point = NULL;
	nb->render_handler.get_screen_info = NULL;
	nb->render_handler.on_popup_show = NULL;
	nb->render_handler.on_popup_size = NULL;
	nb->render_handler.on_paint = browser_on_paint;
//	nb->render_handler.on_cursor_change = NULL;
	nb->render_handler.start_dragging = NULL;
	nb->render_handler.update_drag_cursor = NULL;
	nb->render_handler.on_scroll_offset_changed = NULL;
//	nb->render_handler.on_ime_composition_range_changed = NULL;

	nb->display_handler.on_address_change = browser_on_address_change;
	nb->display_handler.on_title_change = browser_on_title_change;
	nb->display_handler.on_favicon_urlchange = browser_on_favicon_urlchange;
	nb->display_handler.on_fullscreen_mode_change = browser_on_fullscreenmode_change;
	nb->display_handler.on_tooltip = browser_on_tooltip;
	nb->display_handler.on_status_message = browser_on_status_message;
	nb->display_handler.on_console_message = browser_on_console_message;

	nb->request_handler.on_before_browse = browser_on_before_browse;
	nb->request_handler.on_open_urlfrom_tab = NULL;
//	nb->request_handler.on_before_resource_load = NULL;
//	nb->request_handler.get_resource_handler = NULL;
//	nb->request_handler.on_resource_redirect = NULL;
//	nb->request_handler.on_resource_response = NULL;
//	nb->request_handler.get_resource_response_filter = NULL;
//	nb->request_handler.on_resource_load_complete = NULL;
	nb->request_handler.get_auth_credentials = NULL;
	nb->request_handler.on_quota_request = NULL;
//	nb->request_handler.on_protocol_execution = NULL; //FIXME: should implement.
	nb->request_handler.on_certificate_error = NULL;
//	nb->request_handler.on_select_client_certificate = NULL; //we have no such certs
	nb->request_handler.on_plugin_crashed = NULL;
	nb->request_handler.on_render_view_ready = NULL;
	nb->request_handler.on_render_process_terminated = browser_on_render_process_terminated;

	nb->life_span_handler.on_before_popup = NULL;
	nb->life_span_handler.on_after_created = NULL;
	nb->life_span_handler.do_close = NULL;
	nb->life_span_handler.on_before_close = browser_on_before_close;

	nb->context_menu_handler.on_before_context_menu = browser_on_before_context_menu;
	nb->context_menu_handler.run_context_menu = NULL;			//fixme: implement a working context menu somehow
	nb->context_menu_handler.on_context_menu_command = NULL;	//for custom context things, like opening in a new window...
	nb->context_menu_handler.on_context_menu_dismissed = NULL;	//

	nb->desiredwidth = 640;
	nb->desiredheight = 480;

	if (newconsole)
		nb->consolename = strdup(newconsole);
	else
		nb->consolename = NULL;

	//make it white until there's actually something to draw
	nb->videowidth = 1;
	nb->videoheight = 1;
	nb->videodata = malloc(nb->videowidth * nb->videoheight * 4);
	*(int*)nb->videodata = 0x00ffffff;
	memset(nb->videodata, 0xff, nb->videowidth * nb->videoheight * 4);
	nb->updated = true;

	numbrowsers++;
	return nb;
}

//request contexts are per-session things. eg, incognito tabs would have their own private instance
static cef_request_context_t *request_context;
static cef_request_context_handler_t request_context_handler;
//request_context_handler methods
static int CEF_CALLBACK request_context_handler_on_before_plugin_load(cef_request_context_handler_t* self, const cef_string_t* mime_type, const cef_string_t* plugin_url, int is_main_frame, const cef_string_t* top_origin_url, cef_web_plugin_info_t* plugin_info, cef_plugin_policy_t* plugin_policy)
{
//	Con_DPrintf("%s (%s), \"%s\" \"%s\" \"%s\" \"%s\"\n", policy_url.ToString().c_str(), url.ToString().c_str(), 
//		info->GetName().ToString().c_str(), info->GetPath().ToString().c_str(), info->GetVersion().ToString().c_str(), info->GetDescription().ToString().c_str());

	*plugin_policy = PLUGIN_POLICY_BLOCK;	//block by default (user can manually override supposedly). most plugins are unlikely to cope well with our offscreen rendering stuff, and flash sucks.

	cef_release(plugin_info);

	if (!cef_allowplugins->value)
	{
		*plugin_policy = PLUGIN_POLICY_DISABLE;
//		Con_Printf("Blocking plugin: %s (%s)\n", info->GetName().ToString().c_str(), url.ToString().c_str());
	}
	else
		return false;	//false to use the 'recommended' policy
	return true;
}

//there's only one of these, so I'm not going to bother making separate objects for all of the interfaces, nor ref counting
static cef_app_t app;
static cef_browser_process_handler_t browser_process_handler;
static cef_render_process_handler_t render_process_handler;
static cef_v8handler_t v8handler_query;	//window.fte_query
static cef_scheme_handler_factory_t scheme_handler_factory;

static cef_browser_process_handler_t* CEF_CALLBACK app_get_browser_process_handler(cef_app_t* self)
{
	//cef_addref(&browser_process_handler);
	return &browser_process_handler;
}
static cef_render_process_handler_t* CEF_CALLBACK app_get_render_process_handler(cef_app_t* self)
{
	//cef_addref(&render_process_handler);
	return &render_process_handler;
}
static void CEF_CALLBACK app_on_register_custom_schemes(struct _cef_app_t* self, cef_scheme_registrar_t* registrar)
{
	cef_string_t fte = makecefstring("fte");
	registrar->add_custom_scheme(registrar, &fte, CEF_SCHEME_OPTION_NONE
		|CEF_SCHEME_OPTION_STANDARD
		/*|CEF_SCHEME_OPTION_LOCAL*/
		|CEF_SCHEME_OPTION_DISPLAY_ISOLATED
		|CEF_SCHEME_OPTION_SECURE
		|CEF_SCHEME_OPTION_CORS_ENABLED
		/*|CEF_SCHEME_OPTION_CSP_BYPASSING*/
		/*|CEF_SCHEME_OPTION_FETCH_ENABLED*/
		);
	cef_string_clear(&fte);
}

static void CEF_CALLBACK browser_process_handler_on_context_initialized(cef_browser_process_handler_t* self)
{
	cef_string_t fte = makecefstring("fte");
	cef_register_scheme_handler_factory(&fte, NULL, &scheme_handler_factory);
	cef_string_clear(&fte);
}
static void CEF_CALLBACK browser_process_handler_on_before_child_process_launch(cef_browser_process_handler_t* self, cef_command_line_t* command_line)
{
	char *arg = "--plugwrapper";
	char *funcname = "CefSubprocessInit";
	cef_string_t cefisannoying = {NULL};

	cef_string_from_utf8(arg, strlen(arg), &cefisannoying);
	command_line->append_argument(command_line, &cefisannoying);
	cef_string_from_utf8(plugname, strlen(plugname), &cefisannoying);
	command_line->append_argument(command_line, &cefisannoying);
	cef_string_from_utf8(funcname, strlen(funcname), &cefisannoying);
	command_line->append_argument(command_line, &cefisannoying);

//	MessageBoxW(NULL, command_line->GetCommandLineString().c_str(), L"CEF", 0); 
	cef_string_clear(&cefisannoying);
	cef_release(command_line);
}

static void CEF_CALLBACK render_process_handler_on_context_created(cef_render_process_handler_t* self, cef_browser_t* browser, cef_frame_t* frame, cef_v8context_t* context)
{
	cef_v8value_t *jswindow = context->get_global(context);

//eg:	window.fte_query("getstats", function(req,res){console.log("health: "+JSON.parse(res)[0/*STAT_HEALTH*/]);});
	cef_string_t key = makecefstring("fte_query");
	jswindow->set_value_bykey(jswindow, &key, cef_v8value_create_function(&key, &v8handler_query), V8_PROPERTY_ATTRIBUTE_READONLY | V8_PROPERTY_ATTRIBUTE_DONTENUM | V8_PROPERTY_ATTRIBUTE_DONTDELETE);
	cef_string_clear(&key);

	cef_release(browser);
	cef_release(frame);
	cef_release(context);
}

//only use these in the 'renderer' thread / javascript thread.
typedef struct activequery_s
{
	cef_v8value_t *callbackfunc;	//this is the js function to call when the result is available.
	cef_v8context_t *context;
	cef_frame_t *frame;
	int64 queryid;
	struct activequery_s *next;
} activequery_t;
static activequery_t *queries;
static int64 next_queryid;

typedef struct
{
	cef_task_t task;
	cef_string_userfree_t request;
	cef_string_userfree_t result;
	int64 queryid;
	atomic_uint32_t refcount;
} queryresponse_t;
static void CEF_CALLBACK queryresponse_addref(cef_base_ref_counted_t* self)
{
	queryresponse_t *qr = (queryresponse_t*)((char*)self - offsetof(queryresponse_t, task.base)); 
	atomic_fetch_add(&qr->refcount, 1);
}
static int CEF_CALLBACK queryresponse_release(cef_base_ref_counted_t* self)
{
	queryresponse_t *qr = (queryresponse_t*)((char*)self - offsetof(queryresponse_t, task.base)); 
	if (atomic_fetch_sub(&qr->refcount, 1) == 1)
	{
		if (qr->request)
			cef_string_userfree_free(qr->request);
		if (qr->result)
			cef_string_userfree_free(qr->result);
		free(qr);
		return true;
	}
	return false;
}
static void CEF_CALLBACK queryresponse_execute(struct _cef_task_t* self)
{	//lethal injection.
	queryresponse_t *qr = (queryresponse_t*)((char*)self - offsetof(queryresponse_t, task.base)); 
	activequery_t **link, *q;
	for (link = &queries; (q=*link); link = &(*link)->next)
	{
		if (q->queryid == qr->queryid)
		{
			if (q->callbackfunc)
			{
				cef_v8value_t *args[2] = {cef_v8value_create_string(qr->request), cef_v8value_create_string(qr->result)};
				cef_v8value_t *r;
				cef_addref(q->context);
				r = q->callbackfunc->execute_function_with_context(q->callbackfunc, q->context, NULL, 2, args);
				cef_release(r);
			}

			//and clear up the request context too.
			*link = q->next;
			if (q->callbackfunc)
				cef_release(q->callbackfunc);
			cef_release(q->frame);
			cef_release(q->context);
			free(q);
			return;
		}
	}
}

static void CEF_CALLBACK render_process_handler_on_context_released(cef_render_process_handler_t* self, cef_browser_t* browser, cef_frame_t* frame, cef_v8context_t* context)
{
	activequery_t **link, *q;
	for (link = &queries; (q=*link); )
	{
		if (q->context == context)// && q->frame == frame)
		{
			*link = q->next;
			if (q->callbackfunc)
				cef_release(q->callbackfunc);
			cef_release(q->frame);
			cef_release(q->context);
			free(q);
			continue;
		}
		link = &(*link)->next;
	}
	cef_release(browser);
	cef_release(frame);
	cef_release(context);
}


/*javascript methods for the guest code to call*/
static cef_v8value_t *makev8string(char *str)
{
	cef_v8value_t *r;
	cef_string_t cs = makecefstring(str);
	r = cef_v8value_create_string(&cs);
	cef_string_clear(&cs);
	return r;
}
static int CEF_CALLBACK fsfunc_execute(cef_v8handler_t* self, const cef_string_t* name, cef_v8value_t* object, size_t argumentsCount, cef_v8value_t* const* arguments, cef_v8value_t** retval, cef_string_t* exception)
{
	cef_process_message_t *msg;
	cef_list_value_t *args;

	cef_v8context_t *v8ctx = cef_v8context_get_current_context();
	cef_browser_t *browser = v8ctx->get_browser(v8ctx);
	cef_frame_t *frame = v8ctx->get_frame(v8ctx);
//	int64 frame_id = frame->get_identifier(frame);
	
//	cef_string_t key = {L"omgwtfitkindaworks"};
//	key.length = wcslen(key.str);

//	*exception = makecefstring("SOME KIND OF EXCEPTION!");
	*retval = makev8string("");

	if (argumentsCount)
	{
		cef_string_userfree_t setting = arguments[0]->get_string_value(arguments[0]);

		activequery_t *q = malloc(sizeof(*q));
		memset(q, 0, sizeof(*q));
		q->context = v8ctx;	//hold on to these
		q->frame = frame;
		q->queryid = ++next_queryid;
		q->next = queries;
		q->callbackfunc = arguments[1];
		queries = q;

		cef_addref(q->callbackfunc);

		
		msg = cef_process_message_create(name);
		args = msg->get_argument_list(msg);

		args->set_string(args, 0, setting);
		args->set_null(args, 1);
		args->set_int(args, 2, q->queryid & 0xffffffff);
		args->set_int(args, 3, (q->queryid>>32) & 0xffffffff);
		cef_release(args);

		frame->send_process_message(frame, PID_BROWSER, msg);

		if (setting)
			cef_string_userfree_free(setting);
	}
	else
	{
		cef_release(frame);
		cef_release(v8ctx);
	}
	cef_release(browser);


	return 1;
} 

static int CEF_CALLBACK render_process_handler_on_process_message_received(cef_render_process_handler_t* self,cef_browser_t* browser, cef_frame_t* frame, cef_process_id_t source_process, cef_process_message_t* message)
{
	int handled = false;
//	browser_t *br = (browser_t*)((char*)self - offsetof(browser_t, request_handler));
	cef_string_userfree_t msgnameunusable = message->get_name(message);
	cef_string_utf8_t name = {NULL};
	cef_string_to_utf8(msgnameunusable->str, msgnameunusable->length, &name);
	if (!strcmp(name.str, "fte_query"))
	{
		cef_list_value_t *args = message->get_argument_list(message);
		queryresponse_t *task = malloc(sizeof(*task));
		memset(task, 0, sizeof(*task));
		task->refcount = 1;
		task->task.base.size = sizeof(task->task);
		task->task.base.add_ref = queryresponse_addref;
		task->task.base.release = queryresponse_release;
		task->task.execute = queryresponse_execute;
		task->request = args->get_string(args, 0);
		task->result = args->get_string(args, 1);
		task->queryid = args->get_int(args, 2) | ((int64)args->get_int(args, 3)<<32u);
		cef_release(args);
		cef_post_task(TID_RENDERER, &task->task);

		handled = true;
	}
	cef_string_utf8_clear(&name);
	cef_string_userfree_free(msgnameunusable);

	cef_release(browser);
	cef_release(message);
	return handled;
}

/* fte://file/path scheme handler */
typedef struct
{
	cef_resource_handler_t rh;
	atomic_uint32_t refcount;
	vfsfile_t *fh;
	char *data;
	size_t offset;
	size_t datasize;
	unsigned int resultcode;
	char *responseheaders;
} fteresource_t;
static void CEF_CALLBACK resource_handler_addref(cef_base_ref_counted_t* self)
{
	fteresource_t *rh = (fteresource_t*)((char*)self - offsetof(fteresource_t, rh.base)); 
	atomic_fetch_add(&rh->refcount, 1);
}
static int CEF_CALLBACK resource_handler_release(cef_base_ref_counted_t* self)
{
	fteresource_t *rh = (fteresource_t*)((char*)self - offsetof(fteresource_t, rh.base)); 
	if (atomic_fetch_sub(&rh->refcount, 1) == 1)
	{
		if (rh->fh)
			VFS_CLOSE(rh->fh);
		if (rh->data)
			free(rh->data);
		free(rh->responseheaders);
		free(rh);
		return true;
	}
	return false;
}
static void res_catfield_l(char **const orig, const char *key, int kl, const char *val, int vl)
{
	size_t ol = *orig?strlen(*orig):0;
	char *n;
	if (ol)
	{
		n = malloc(ol+1+kl+1+vl+1);
		memcpy(n, *orig, ol);
		n[ol++] = '\n';
	}
	else
		n = malloc(kl+1+vl+1);
	memcpy(n+ol, key, kl);
	ol+=kl;
	n[ol++] = '\n';
	memcpy(n+ol, val, vl);
	ol+=vl;
	n[ol++] = 0;
	free(*orig);
	*orig = n;
}
static void res_catfield(char **const orig, const char *key, const char *val)
{
	res_catfield_l(orig, key, strlen(key), val, strlen(val));
}
static void res_catfield_csuf(char **const orig, const char *key, cef_string_userfree_t cs)
{
	cef_string_utf8_t u8 = {NULL};
	cef_string_to_utf8(cs->str, cs->length, &u8);
	res_catfield_l(orig, key, strlen(key), u8.str, u8.length);
	cef_string_utf8_clear(&u8);
}
static void res_catfield_cs(char **const orig, cef_string_t *key, cef_string_t *val)
{
	cef_string_utf8_t keyu8 = {NULL};
	cef_string_utf8_t valu8 = {NULL};
	cef_string_to_utf8(key->str, key->length, &keyu8);
	cef_string_to_utf8(val->str, val->length, &valu8);
	res_catfield_l(orig, keyu8.str, keyu8.length, valu8.str, valu8.length);
	cef_string_utf8_clear(&keyu8);
	cef_string_utf8_clear(&valu8);
}

static int CEF_CALLBACK resource_handler_process_request(cef_resource_handler_t* self, cef_request_t* request, cef_callback_t* callback)
{
	fteresource_t *rh = (fteresource_t*)((char*)self - offsetof(fteresource_t, rh));
	cef_string_userfree_t url = request->get_url(request), method;
	cef_post_data_t *postdata;
	size_t numelements;
	cef_post_data_element_t *elements[1];
	size_t postsize;
	char *postbytes;
	char *q;
	char *e;
	cef_string_utf8_t u8_url = {NULL}, u8={NULL};
	cef_string_t ext = {NULL};
	cef_string_to_utf8(url->str, url->length, &u8_url);
	rh->resultcode = 404;

	//hack at the url to hide the
	q = strchr(u8_url.str, '?');
	if (q)
		*q = 0;
	for(e = q?q:u8_url.str+strlen(u8_url.str); e > u8_url.str; )
	{
		e--;
		if (*e == '/')
			break;		//no extension
		if (*e == '.')
		{
			e++;
			cef_string_from_utf8(e, strlen(e), &ext);
			break;
		}
	}

	res_catfield(&rh->responseheaders, "Access-Control-Allow-Origin", "fte://data");
	res_catfield(&rh->responseheaders, "Access-Control-Allow-Origin", "fte://csqc");
	res_catfield(&rh->responseheaders, "Access-Control-Allow-Origin", "fte://ssqc");
	res_catfield(&rh->responseheaders, "Access-Control-Allow-Origin", "fte://menu");

	//sandboxed to the same dir that qc can fopen/fwrite.
	//(also blocks any fancy http:// parsing that an engine might do)
	if (!strncmp(u8_url.str, "fte://data/", 11))
	{
		rh->fh = fsfuncs->OpenVFS(u8_url.str+6, "rb", FS_GAME);
		if (rh->fh)
		{
			cef_string_userfree_t mt = cef_get_mime_type(&ext);
			if (mt)
			{
				res_catfield_csuf(&rh->responseheaders, "Content-Type", mt);
				cef_string_userfree_free(mt);
			}
			rh->resultcode = 200;
		}
		else
		{
			rh->resultcode = 404;
			res_catfield(&rh->responseheaders, "Content-Type", "text/html");
			rh->data = strdup("<html><style type=\"text/css\">body {background-color: lightblue;}</style><title>not found</title>File not found within game filesystem.</html>");
			rh->datasize = strlen(rh->data);
		}
	}
	else if (!strncmp(u8_url.str, "fte://ssqc/", 11) || !strncmp(u8_url.str, "fte://csqc/", 11) || !strncmp(u8_url.str, "fte://menu/", 11))
	{
		struct pubprogfuncs_s *progs;
		const char *page;
		const char *respheaders = NULL;
		const char *reqheaders = NULL;
		if (ext.str)
		{
			cef_string_userfree_t mt = cef_get_mime_type(&ext);
			if (mt)
			{
				res_catfield_csuf((char**)&respheaders, "Content-Type", mt);
				cef_string_userfree_free(mt);
			}
		}

		if(q)
			*q = '?';	//put it back so the qc can get the full url.

		rh->resultcode = 404;

		if (!strncmp(u8_url.str, "fte://ssqc/", 11))
			progs = plugfuncs->GetEngineInterface("SSQCVM", sizeof(*progs));	//WARNING: goes direct rather than via the server, so basically single-player only.
		else if (!strncmp(u8_url.str, "fte://csqc/", 11))
			progs = plugfuncs->GetEngineInterface("CSQCVM", sizeof(*progs));
		else if (!strncmp(u8_url.str, "fte://menu/", 11))
			progs = plugfuncs->GetEngineInterface("MenuQCVM", sizeof(*progs));
		else
			progs = NULL;
		
		if (progs)
		{
			func_t func = progs->FindFunction(progs, "Cef_GeneratePage", PR_ANY);
			if (func)
			{
				void *pr_globals = PR_globals(progs, PR_CURRENT);
				((string_t *)pr_globals)[OFS_PARM0] = progs->TempString(progs, u8_url.str+11);

				method = request->get_method(request);
				cef_string_to_utf8(method->str, method->length, &u8);
				((string_t *)pr_globals)[OFS_PARM1] = progs->TempString(progs, u8.str);
				cef_string_userfree_free(method);
				cef_string_utf8_clear(&u8);

				postdata = request->get_post_data(request);
				if (postdata)
				{
					numelements = countof(elements);
					memset(elements, 0, sizeof(elements));
					postdata->get_elements(postdata, &numelements, elements);
					postsize = elements[0]->get_bytes_count(elements[0]);
					postbytes = malloc(postsize+1);
					elements[0]->get_bytes(elements[0], postsize, postbytes);
					postbytes[postsize] = 0;
					((string_t *)pr_globals)[OFS_PARM2] = progs->TempString(progs, postbytes);
					free(postbytes);
					cef_release(elements[0]);
				}
				else
					((string_t *)pr_globals)[OFS_PARM2] = 0;

				{
					size_t i, elems;
					cef_string_t key = {NULL}, val = {NULL};
					cef_string_multimap_t hmap = cef_string_multimap_alloc();
					request->get_header_map(request, hmap);
					elems = cef_string_multimap_size(hmap);
					for (i = 0; i < elems; i++)
					{
						cef_string_multimap_key(hmap, i, &key);
						cef_string_multimap_key(hmap, i, &val);
						res_catfield_cs(&rh->responseheaders, &key, &val);
						cef_string_clear(&key);
						cef_string_clear(&val);
					}
					cef_string_multimap_free(hmap);
				}

				((string_t *)pr_globals)[OFS_PARM3] = reqheaders?progs->TempString(progs, reqheaders):0;	//request heders
				((string_t *)pr_globals)[OFS_PARM4] = rh->responseheaders?progs->TempString(progs, rh->responseheaders):0;	//response headers
				((string_t *)pr_globals)[OFS_PARM5] = 0;
				((string_t *)pr_globals)[OFS_PARM6] = 0;
				((string_t *)pr_globals)[OFS_PARM7] = 0;
				progs->ExecuteProgram(progs, func);

				if (((string_t *)pr_globals)[OFS_RETURN])
				{
					page = progs->StringToNative(progs, ((string_t *)pr_globals)[OFS_RETURN]);
					respheaders = progs->StringToNative(progs, ((string_t *)pr_globals)[OFS_PARM4]);
					rh->resultcode = 200;
				}
				else
					page = "<html><style type=\"text/css\">body {background-color: lightblue;}</style><title>not found</title>Cef_GeneratePage returned null</html>";
			}
			else
				page = "<html><style type=\"text/css\">body {background-color: lightblue;}</style><title>not found</title>Cef_GeneratePage not implemented by mod</html>";
		}
		else
			page = "<html><style type=\"text/css\">body {background-color: lightblue;}</style><title>not found</title>That QCVM is not running</html>";

		if (*respheaders == '\n')
			respheaders++;
		rh->responseheaders = strdup(respheaders);
		//FIXME: only return any data if we were successful OR the mime is text/html
		rh->data = strdup(page);
		rh->datasize = strlen(rh->data);
	}
	else
	{
		rh->resultcode = 403;
		res_catfield(&rh->responseheaders, "Content-Type", "text/html");
		rh->data = strdup("<html><style type=\"text/css\">body {background-color: lightblue;}</style><title>forbidden</title><a href=\"fte://data/index.html\">Try here</a> <a href=\"fte://csqc/index.html\">Or try here</a></html>");
		rh->datasize = strlen(rh->data);
	}

	cef_string_userfree_free(url);
	cef_string_utf8_clear(&u8_url);

	callback->cont(callback);	//headers are now known... should be delayed.
	cef_release(callback);
	cef_release(request);
	return 1;	//failure is reported as an http error code rather than an exception
}
static char *strseps(char *str, char *chars)
{	//find the next separator
	char *best = str+strlen(str);
	char *c;
	if (*str)
		while(*chars)
		{
			c = strchr(str, *chars++);
			if (c && c < best)
				best = c;
		}
	return best;
}
static void CEF_CALLBACK resource_handler_get_response_headers(cef_resource_handler_t* self, cef_response_t* response, int64* response_length, cef_string_t* redirectUrl)
{
	fteresource_t *rh = (fteresource_t*)((char*)self - offsetof(fteresource_t, rh));
	cef_string_multimap_t *hmap;
	cef_string_t key = {NULL}, val={NULL};

	if (rh->fh)
		*response_length = VFS_GETLEN(rh->fh);
	else if (rh->data)
		*response_length = rh->datasize;
	else
		*response_length = -1;

	hmap = cef_string_multimap_alloc();
	if (rh->responseheaders)
	{
		char *start;
		char *sep;
		char *nl;
		for (start = rh->responseheaders; *start; )
		{
			sep = strseps(start, ":\n");
			nl = strseps(sep+1, "\n");

			cef_string_from_utf8(start, sep-start, &key);
			if (*sep)
				sep++;
			cef_string_from_utf8(sep, nl-sep, &val);

			cef_string_multimap_append(hmap, &key, &val);
			if (*nl)
				start = nl+1;
			else
				break;
		}
	}

	cef_string_multimap_append(hmap, makecefstringptr("Access-Control-Allow-Origin", &key), makecefstringptr("fte://data", &val));
	response->set_header_map(response, hmap);

//	response->set_mime_type(response, &rh->mimetype);
	response->set_status(response, rh->resultcode);

	cef_string_clear(&key);
	cef_string_clear(&val);
	cef_release(response);
}
static int CEF_CALLBACK resource_handler_read_response(cef_resource_handler_t* self, void* data_out, int bytes_to_read, int* bytes_read, cef_callback_t* callback)
{
	fteresource_t *rh = (fteresource_t*)((char*)self - offsetof(fteresource_t, rh));

	if (rh->fh)
		*bytes_read = VFS_READ(rh->fh, data_out, bytes_to_read);
	else if (rh->data)
	{
		if (bytes_to_read > rh->datasize - rh->offset)
			bytes_to_read = rh->datasize - rh->offset;
		*bytes_read = bytes_to_read;
		memcpy(data_out, rh->data + rh->offset, bytes_to_read);
		rh->offset += bytes_to_read;
	}
	else
		*bytes_read = 0;

	//callback->cont(callback);	//headers are now known... should be delayed.
	cef_release(callback);

	if (*bytes_read <= 0)
	{
		*bytes_read = 0;
		return 0;
	}
	return true;	//more to come
}
static void CEF_CALLBACK resource_handler_cancel(cef_resource_handler_t* self)
{
	fteresource_t *rh = (fteresource_t*)((char*)self - offsetof(fteresource_t, rh));

	if (rh->fh)
		VFS_CLOSE(rh->fh);
	rh->fh = NULL;
	if (rh->data)
		free(rh->data);
	rh->data = NULL;
	rh->offset = 0;
	rh->datasize = 0;
}

static cef_resource_handler_t* CEF_CALLBACK scheme_handler_factory_create(cef_scheme_handler_factory_t* self, cef_browser_t* browser, cef_frame_t* frame, const cef_string_t* scheme_name, cef_request_t* request)
{
	fteresource_t *rh = malloc(sizeof(*rh));
	memset(rh, 0, sizeof(*rh));

	rh->rh.base.size = sizeof(*rh);
	rh->rh.base.add_ref			= resource_handler_addref;
	rh->rh.base.release			= resource_handler_release;
	rh->rh.process_request		= resource_handler_process_request;
	rh->rh.get_response_headers	= resource_handler_get_response_headers;
	rh->rh.read_response		= resource_handler_read_response;
	rh->rh.cancel				= resource_handler_cancel;

	cef_addref(&rh->rh);

	cef_release(browser);
	cef_release(frame);
	cef_release(request);
	return &rh->rh;
}

static void app_initialize(void)
{
	app.base.size = sizeof(app);
	app.get_browser_process_handler = app_get_browser_process_handler;
	app.get_render_process_handler = app_get_render_process_handler;
	app.on_register_custom_schemes = app_on_register_custom_schemes;

	browser_process_handler.base.size = sizeof(browser_process_handler);
	browser_process_handler.on_before_child_process_launch	= browser_process_handler_on_before_child_process_launch;
	browser_process_handler.on_context_initialized			= browser_process_handler_on_context_initialized;

	render_process_handler.base.size = sizeof(render_process_handler);
	render_process_handler.on_context_created				= render_process_handler_on_context_created;
	render_process_handler.on_context_released				= render_process_handler_on_context_released;
	render_process_handler.on_process_message_received		= render_process_handler_on_process_message_received;

	v8handler_query.base.size = sizeof(v8handler_query);
	v8handler_query.execute									= fsfunc_execute;

	scheme_handler_factory.base.size = sizeof(scheme_handler_factory);
	scheme_handler_factory.create							= scheme_handler_factory_create;

	request_context_handler.base.size = sizeof(request_context_handler);
	request_context_handler.on_before_plugin_load			= request_context_handler_on_before_plugin_load;
}
static int cefwasinitialised;

cef_request_context_t *Cef_GetRequestContext(void)
{
	char utf8[MAX_OSPATH];
	cef_request_context_t *ret = NULL;
	qboolean incog;
	
	incog = cef_incognito->value;

	if (!incog)
		ret = request_context;

	if (!ret)
	{
		cef_request_context_settings_t csettings = {sizeof(csettings)};
		csettings.persist_user_preferences = !incog;
		if (!incog && fsfuncs->NativePath("cefcache", FS_ROOT, utf8, sizeof(utf8)))
			cef_string_from_utf8(utf8, strlen(utf8), &csettings.cache_path);	//should be empty for incognito.
		ret = cef_request_context_create_context(&csettings, &request_context_handler);
		cef_string_clear(&csettings.cache_path);
	}
	else
		cef_addref(ret);

	if (!incog && !request_context)
	{
		request_context = ret;
		cef_addref(request_context);
	}
	return ret;
}

static qboolean Cef_Init(qboolean engineprocess);

struct mediacallbacks_s;	//todo...
static void *Cef_Create(const char *name, struct mediacallbacks_s *callbacks)
{
	cef_window_info_t window_info = {0};
	cef_browser_settings_t browserSettings = {sizeof(browserSettings)};
	browser_t *newbrowser;
	cef_string_t url = {NULL};

	if (!strcmp(name, "cef"))
		name += 3;
	else if (!strncmp(name, "cef:", 4))
		name += 4;
	else if (!strcmp(name, "http"))
		name += 4;
	else if (!strncmp(name, "http:", 5))
		;
	else if (!strncmp(name, "https:", 6))
		;
	else if (!strncmp(name, "ftp:", 4))
		;
	else
		return NULL;


	if (!cefwasinitialised)
	{
		char utf8[MAX_OSPATH];
		cef_main_args_t mainargs = {0};
		cef_settings_t settings = {sizeof(settings)};

		if (!Cef_Init(true))
			return NULL;

		//const char *ua = "FTEBrowser";
		//cef_string_from_utf8(ua, strlen(ua), &settings.user_agent);

		if (fsfuncs->NativePath("cefcache", FS_ROOT, utf8, sizeof(utf8)))
			cef_string_from_utf8(utf8, strlen(utf8), &settings.cache_path);
		if (fsfuncs->NativePath("cef_debug.log", FS_ROOT, utf8, sizeof(utf8)))
			cef_string_from_utf8(utf8, strlen(utf8), &settings.log_file);

		if (fsfuncs->NativePath("", FS_BINARYPATH, utf8, sizeof(utf8)))
			cef_string_from_utf8(utf8, strlen(utf8), &settings.resources_dir_path);

		if (fsfuncs->NativePath("locales", FS_BINARYPATH, utf8, sizeof(utf8)))
		{
			struct stat statbuf;
			if (stat(utf8, &statbuf) < 0)
			{
				Con_Printf("%s not found\n", utf8);
				return NULL;
			}
			cef_string_from_utf8(utf8, strlen(utf8), &settings.locales_dir_path);
		}

#ifdef _WIN32
		{
			wchar_t omgwtfamonkey[MAX_OSPATH];
			if (GetModuleFileNameW(NULL, omgwtfamonkey, countof(omgwtfamonkey)))
				cef_string_from_utf16(omgwtfamonkey, wcslen(omgwtfamonkey), &settings.browser_subprocess_path);
			mainargs.instance = GetModuleHandle(NULL);
		}
#endif

#ifdef _DEBUG
		settings.log_severity = LOGSEVERITY_VERBOSE;
#else
		settings.log_severity = LOGSEVERITY_DISABLE;
#endif
		settings.background_color = 0x00ffffff;
//		settings.single_process = true;
#ifdef _WIN32
//		settings.multi_threaded_message_loop = true;	//fixme: use this.
#endif
		settings.windowless_rendering_enabled = true;
//		settings.command_line_args_disabled = true;
//		settings.persist_session_cookies = false;

/*		{
			char *s;
			strcpy(utf8, FULLENGINENAME "/" STRINGIFY(FTE_VER_MAJOR) "." STRINGIFY(FTE_VER_MINOR));
			while((s = strchr(utf8, ' ')))
				*s = '_';
			cef_string_from_utf8(utf8, strlen(utf8), &settings.product_version);
		}
*/
		cefwasinitialised = !!cef_initialize(&mainargs, &settings, &app, NULL);
		cef_string_clear(&settings.browser_subprocess_path);
//		cef_string_clear(&settings.product_version);
		cef_string_clear(&settings.cache_path);
		cef_string_clear(&settings.log_file);
	}

	if (!cefwasinitialised)
		return NULL;

	//tbh, web browser's are so horribly insecure that it seems pointless to even try disabling stuff that might be useful
	browserSettings.windowless_frame_rate = 60;
	browserSettings.javascript_close_windows = STATE_DISABLED;
	browserSettings.javascript_access_clipboard = STATE_DISABLED;
//	browserSettings.universal_access_from_file_urls = STATE_DISABLED;
//	browserSettings.file_access_from_file_urls = STATE_DISABLED;
	browserSettings.remote_fonts = STATE_DISABLED;
	browserSettings.plugins = STATE_DISABLED;
	browserSettings.background_color = 0x00ffffff;

	window_info.windowless_rendering_enabled = true;
	memset(&window_info.parent_window, 0, sizeof(window_info.parent_window));

	newbrowser = browser_create();
	if (!newbrowser)
		return NULL;

	if (!*name || !strcmp(name, "http:") || !strcmp(name, "https:"))
		name = "about:blank";
	cef_string_from_utf8(name, strlen(name), &url);

	cef_addref(&newbrowser->client);
	newbrowser->thebrowser = cef_browser_host_create_browser_sync(&window_info, &newbrowser->client, &url, &browserSettings, NULL, Cef_GetRequestContext());
	cef_string_to_utf8(url.str, url.length, &newbrowser->currenturl);
	cef_string_clear(&url);
	if (!newbrowser->thebrowser)
	{
		browser_release(newbrowser);
		return NULL;	//cef fucked up.
	}

	if (cef_devtools->value)
	{
		cef_browser_host_t *host = newbrowser->thebrowser->get_host(newbrowser->thebrowser);
		browser_t *devtools = browser_create();
		
#ifdef _WIN32
		window_info.style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
		window_info.parent_window = NULL;
		window_info.bounds.x = CW_USEDEFAULT;
		window_info.bounds.y = CW_USEDEFAULT;
		window_info.bounds.width = CW_USEDEFAULT;
		window_info.bounds.height = CW_USEDEFAULT;
		window_info.window_name = makecefstring("CEF Dev Tools");
#else
		memset(&window_info.parent_window, 0, sizeof(window_info.parent_window));
		window_info.x = 0;
		window_info.y = 0;
		window_info.width = 320;
		window_info.height = 240;
#endif
		window_info.windowless_rendering_enabled = false;

		cef_addref(&devtools->client);
		host->show_dev_tools(host, &window_info, &devtools->client, &browserSettings, NULL);
		cef_release(host);
		browser_release(devtools);	//cef should continue to hold a reference to it while its visible, but its otherwise out of engine now.

#ifdef _WIN32
		cef_string_clear(&window_info.window_name);
#endif
	}

	return (void*)newbrowser;
}

static void *Cef_CreateOld(const char *name)
{
	return Cef_Create(name, NULL);
}

static qboolean VARGS Cef_DisplayFrame(void *ctx, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ectx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ectx)
{
	browser_t *browser = (browser_t*)ctx;
	if (browser->updated || forcevideo)
	{
		uploadtexture(ectx, TF_BGRA32, browser->videowidth, browser->videoheight, browser->videodata, NULL);
		browser->updated = false;
	}
	return true;
}
static void Cef_Destroy(void *ctx)
{	//engine isn't allowed to talk about the browser any more. kill it.
	browser_t *br = (browser_t*)ctx;
	cef_browser_host_t *host = br->thebrowser->get_host(br->thebrowser);
	host->close_browser(host, true);
	cef_release(host);
	if (br->thebrowser)
	{
		cef_release(br->thebrowser);
		br->thebrowser = NULL;
	}

	//now release our reference to it.
	browser_release(br);	//hopefully this should be the last reference, but we might be waiting for something on the cef side. hopefully nothing blocking on an unload event...
}

static void VARGS Cef_CursorMove (void *ctx, float posx, float posy)
{
	browser_t *br = (browser_t*)ctx;
	cef_browser_host_t *host = br->thebrowser->get_host(br->thebrowser);
	br->mousepos.x = (int)(posx * br->desiredwidth);
	br->mousepos.y = (int)(posy * br->desiredheight);
	br->mousepos.modifiers = 0;
	host->send_mouse_move_event(host, &br->mousepos, false);
	cef_release(host);
}

static void VARGS Cef_Key (void *ctx, int code, int unicode, int event)
{
	browser_t *browser = (browser_t*)ctx;
	cef_browser_host_t *host = browser->thebrowser->get_host(browser->thebrowser);

	//handle mouse buttons
	if (code >= K_MOUSE1 && code <= K_MOUSE3)
	{
		int buttons[] = {MBT_LEFT, MBT_RIGHT, MBT_MIDDLE};
		if (!event || browser->keystate[code])
			host->send_mouse_click_event(host, &browser->mousepos, buttons[code-K_MOUSE1], event?true:false, 1);
		if (event)
			browser->keystate[code] = 0;
		else
			browser->keystate[code] = 1;
		cef_release(host);
		return;
	}

	if (code == K_TOUCH)
	{	//FIXME
		cef_release(host);
		return;
	}
	if (code == K_TOUCHSLIDE || code == K_TOUCHTAP || code == K_TOUCHLONG)
	{
		cef_release(host);
		return;	//has to do its own
	}


	//handle mouse wheels
	if (code == K_MWHEELUP || code == K_MWHEELDOWN)
	{
		if (!event)
			host->send_mouse_wheel_event(host, &browser->mousepos, 0, (code == K_MWHEELDOWN)?-32:32);
		cef_release(host);
		return;
	}

	//handle keypress/release events
	if (code)
	{
		cef_key_event_t kev = {0};
		if (event && !browser->keystate[code])
		{
			cef_release(host);
			return;	//releasing a key that is already released is weird.
		}

		kev.type = event?KEYEVENT_KEYUP:KEYEVENT_RAWKEYDOWN;
		kev.modifiers = 0;
		switch(code)
		{
		case 0:				kev.windows_key_code = 0;					break;
		case K_UPARROW:		kev.windows_key_code = 0x26/*VK_UP*/;		break;
		case K_DOWNARROW:	kev.windows_key_code = 0x28/*VK_DOWN*/;		break;
		case K_LEFTARROW:	kev.windows_key_code = 0x25/*VK_LEFT*/;		break;
		case K_RIGHTARROW:	kev.windows_key_code = 0x27/*VK_RIGHT*/;	break;
		case K_ESCAPE:		kev.windows_key_code = 0x1b/*VK_ESCAPE*/;	break;
		case K_SPACE:		kev.windows_key_code = 0x20/*VK_SPACE*/;	break;
		case K_RSHIFT:		kev.windows_key_code = 0x10/*VK_SHIFT*/;	break;
		case K_LSHIFT:		kev.windows_key_code = 0x10/*VK_SHIFT*/;	break;
		case K_RCTRL:		kev.windows_key_code = 0x11/*VK_CONTROL*/;	break;
		case K_LCTRL:		kev.windows_key_code = 0x11/*VK_CONTROL*/;	break;
		case K_RALT:		kev.windows_key_code = 0x12/*VK_MENU*/;		break;
		case K_LALT:		kev.windows_key_code = 0x12/*VK_MENU*/;		break;
		case K_TAB:			kev.windows_key_code = 0x09/*VK_TAB*/;		break;
		case K_RWIN:		kev.windows_key_code = 0x5c/*VK_RWIN*/;		break;
		case K_LWIN:		kev.windows_key_code = 0x5b/*VK_LWIN*/;		break;
		case K_APP:			kev.windows_key_code = 0x5d/*VK_APPS*/;		break;
		case K_F1:			kev.windows_key_code = 0x70/*VK_F1*/;		break;
		case K_F2:			kev.windows_key_code = 0x71/*VK_F2*/;		break;
		case K_F3:			kev.windows_key_code = 0x72/*VK_F3*/;		break;
		case K_F4:			kev.windows_key_code = 0x73/*VK_F4*/;		break;
		case K_F5:			kev.windows_key_code = 0x74/*VK_F5*/;		break;
		case K_F6:			kev.windows_key_code = 0x75/*VK_F6*/;		break;
		case K_F7:			kev.windows_key_code = 0x76/*VK_F7*/;		break;
		case K_F8:			kev.windows_key_code = 0x77/*VK_F8*/;		break;
		case K_F9:			kev.windows_key_code = 0x78/*VK_F9*/;		break;
		case K_F10:			kev.windows_key_code = 0x79/*VK_F10*/;		break;
		case K_F11:			kev.windows_key_code = 0x81/*VK_F11*/;		break;
		case K_F12:			kev.windows_key_code = 0x82/*VK_F12*/;		break;
		case K_BACKSPACE:	kev.windows_key_code = 0x08/*VK_BACK*/;		break;
		case K_DEL:			kev.windows_key_code = 0x2e/*VK_DELETE*/;	break;
		case K_HOME:		kev.windows_key_code = 0x24/*VK_HOME*/;		break;
		case K_END:			kev.windows_key_code = 0x23/*VK_END*/;		break;
		case K_INS:			kev.windows_key_code = 0x2d/*VK_INSERT*/;	break;
		case K_PGUP:		kev.windows_key_code = 0x21/*VK_PRIOR*/;	break;
		case K_PGDN:		kev.windows_key_code = 0x22/*VK_NEXT*/;		break;

		default:
			if ((code >= 0x30 && code <= 0x39) || (code >= 0x41 && code <= 0x5a))
				kev.windows_key_code = code;
			else if (code >= 'a' && code <= 'z')
				kev.windows_key_code = (code-'a') + 'A';
			else
				kev.windows_key_code = 0;
			break;
		}
		kev.native_key_code = unicode<<16;

		if (browser->keystate[code])
			kev.native_key_code |= 1<<30;
		if (event)
			kev.native_key_code |= 1u<<31;

		if (event)
			browser->keystate[code] = 0;
		else
			browser->keystate[code] = 1;

		kev.is_system_key = 0;
		kev.character = unicode;
		kev.unmodified_character = unicode;
		kev.focus_on_editable_field = true;
		host->send_key_event(host, &kev);
	}

	//handle text input events (down events only)
	if (unicode && !event)
	{
		cef_key_event_t kev;

		kev.type = KEYEVENT_CHAR;
		kev.modifiers = 0;
	
		kev.windows_key_code = unicode;
		kev.native_key_code = unicode<<16;

		if (browser->keystate[code])
			kev.native_key_code |= 1<<30;
		if (event)
			kev.native_key_code |= 1u<<31;

		kev.is_system_key = 0;
		kev.character = unicode;
		kev.unmodified_character = unicode;
		kev.focus_on_editable_field = true;
		host->send_key_event(host, &kev);
	}
	cef_release(host);
}
static qboolean VARGS Cef_SetSize (void *ctx, int width, int height)
{
	browser_t *browser = (browser_t*)ctx;
	cef_browser_host_t *host = browser->thebrowser->get_host(browser->thebrowser);
	if (browser->desiredwidth != width || browser->desiredheight != height)
	{
		browser->desiredwidth = width;
		browser->desiredheight = height;
		host->was_resized(host);
	}
	cef_release(host);
	return qtrue;
}
static void VARGS Cef_GetSize (void *ctx, int *width, int *height)
{
	//this specifies the logical size/aspect of the browser object
	browser_t *browser = (browser_t*)ctx;
	*width = browser->desiredwidth;
	*height = browser->desiredheight;
}
static void VARGS Cef_ChangeStream (void *ctx, const char *streamname)
{
	browser_t *browser = (browser_t*)ctx;
	cef_browser_host_t *host = browser->thebrowser->get_host(browser->thebrowser);
	cef_frame_t *frame = NULL;
	if (!strncmp(streamname, "cmd:", 4))
	{
		const char *cmd = streamname+4;
		if (!strcmp(cmd, "focus"))
			host->set_focus(host, true);
		else if (!strcmp(cmd, "unfocus"))
			host->set_focus(host, false);
		else if (!strcmp(cmd, "refresh"))
			browser->thebrowser->reload(browser->thebrowser);
		else if (!strcmp(cmd, "transparent"))
;
		else if (!strcmp(cmd, "opaque"))
;
		else if (!strcmp(cmd, "stop"))
			browser->thebrowser->stop_load(browser->thebrowser);
		else if (!strcmp(cmd, "back"))
			browser->thebrowser->go_back(browser->thebrowser);
		else if (!strcmp(cmd, "forward"))
			browser->thebrowser->go_forward(browser->thebrowser);
		else if (!strcmp(cmd, "home"))
			Cef_ChangeStream(ctx, "http://fte.triptohell.info");
		else
		{
			frame = browser->thebrowser->get_focused_frame(browser->thebrowser);
			if (!strcmp(cmd, "undo"))
				frame->undo(frame);
			else if (!strcmp(cmd, "redo"))
				frame->redo(frame);
			else if (!strcmp(cmd, "cut"))
				frame->cut(frame);
			else if (!strcmp(cmd, "copy"))
				frame->copy(frame);
			else if (!strcmp(cmd, "paste"))
				;//frame->paste(frame);	//possible security hole, as this uses the system clipboard
			else if (!strcmp(cmd, "del"))
				frame->del(frame);
			else if (!strcmp(cmd, "selectall"))
				frame->select_all(frame);
			else
				Con_Printf("unrecognised cmd: %s\n", cmd);
		}
	}
	else if (!strncmp(streamname, "javascript:", 11))
	{
		cef_string_t thescript = {NULL};
		cef_string_t url = {NULL};
		cef_string_from_utf8(streamname+11, strlen(streamname+11), &thescript);
		cef_string_from_utf8("http://localhost/", strlen("http://localhost/"), &url);
		frame = browser->thebrowser->get_main_frame(browser->thebrowser);
		frame->execute_java_script(frame, &thescript, &url, 1);
		cef_string_clear(&thescript);
		cef_string_clear(&url);
	}
	/*else if (!strncmp(streamname, "raw:", 4))
	{
		cef_string_t thehtml = {NULL};
		cef_string_t url = {NULL};
		cef_string_from_utf8(streamname+4, strlen(streamname+4), &thehtml);
		cef_string_from_utf8("http://localhost/", strlen("http://localhost/"), &url);
		frame = browser->thebrowser->get_main_frame(browser->thebrowser);
		frame->load_string(frame, &thehtml, &url);
		cef_string_clear(&thehtml);
		cef_string_clear(&url);
	}*/
	else if (*streamname && strcmp(streamname, "http:") && strcmp(streamname, "https:"))
	{
		cef_string_t url = {NULL};
		cef_string_from_utf8(streamname, strlen(streamname), &url);
		frame = browser->thebrowser->get_main_frame(browser->thebrowser);
		frame->load_url(frame, &url);
		cef_string_clear(&url);
	}
	if (frame)
		cef_release(frame);
	cef_release(host);
}

qboolean VARGS Cef_GetProperty (void *ctx, const char *field, char *out, size_t *outsize)
{
	browser_t *browser = (browser_t*)ctx;
	const char *ret = NULL;
	if (!strcmp(field, "url"))
		ret = browser->currenturl.str;
	else if (!strcmp(field, "title"))
		ret = browser->currenttitle.str;
	else if (!strcmp(field, "status"))
		ret = browser->currentstatus.str;
	else if (!strcmp(field, "icon"))
		ret = browser->currenticon.str;

	if (ret)
	{
		size_t retsize = strlen(ret);
		if (out)
		{
			if (*outsize < retsize)
				return false;	//caller fucked up
			memcpy(out, ret, retsize);
		}
		*outsize = retsize;
		return true;
	}
	return false;
}

static media_decoder_funcs_t decoderfuncs;

static qintptr_t Cef_Tick(qintptr_t *args)
{
	if (cefwasinitialised)
	{
		cef_do_message_loop_work();

		/* libcef can't cope with this.
		if (!numbrowsers)
		{
			if (request_context)
			{
				cef_release(request_context);
				request_context = NULL;
			}
			cef_shutdown();
			cefwasinitialised = false;
		}
		*/
	}
	return 0;
}
static qintptr_t Cef_Shutdown(qintptr_t *args)
{
	if (cefwasinitialised)
	{
		int tries = 1000*10;//60*5;	//keep trying for a duration (in ms)... give up after then as it just isn't working.
		while(numbrowsers && tries > 0)
		{
			cef_do_message_loop_work();

			tries -= 10;
#ifdef _WIN32
			Sleep(10);
#else
			usleep(10*1000);
#endif
		}
#ifdef _WIN32
		if (numbrowsers)
		{	//this really should NOT be happening.
			MessageBox(NULL, "Browsers are still open", "CEF Fuckup", 0);
		}
#endif
		if (request_context)
		{
			cef_release(request_context);
			request_context = NULL;
		}
		cef_shutdown();
		cefwasinitialised = false;
		numbrowsers = 0;
	}

#if defined(_DEBUG) && defined(_MSC_VER)
//	_CrtDumpMemoryLeaks();
#endif

	return 0;
}

#ifndef _WIN32
static int argc=0;
static char *argv[64];
static char commandline[8192];
static void SetupArgv(cef_main_args_t *a)
{
	FILE *f;
	if (!argc)
	{
		f = fopen("/proc/self/cmdline", "r");
		if (f)
		{
			char *s = commandline;
			char *e = commandline+fread(commandline, 1, sizeof(commandline), f);
			fclose(f);
			while(s < e)
			{
				argv[argc++] = s;
				while(*s)
					s++;
				s++;
			}
		}
	}
	a->argc = argc;
	a->argv = argv;
}
#endif

//if we're a subprocess and somehow failed to add the --plugwrapper arg to the engine, then make sure we're not starting endless processes.
static qboolean Cef_Init(qboolean engineprocess)
{
	static qboolean cefwasloaded = qfalse;

#ifdef _WIN32
	cef_main_args_t args = {GetModuleHandle(NULL)};
#else
	cef_main_args_t args;
	SetupArgv(&args);
#endif

	if (cefwasloaded)
		return qtrue;

	{
		int result;

#ifdef LIBCEF_DYNAMIC
		dllfunction_t ceffuncs[] =
		{
			{(void **)&cef_version_info,					"cef_version_info"},
			{(void **)&cef_initialize,						"cef_initialize"},
			{(void **)&cef_do_message_loop_work,			"cef_do_message_loop_work"},
			{(void **)&cef_shutdown,						"cef_shutdown"},
			{(void **)&cef_execute_process,					"cef_execute_process"},
			{(void **)&cef_browser_host_create_browser_sync,"cef_browser_host_create_browser_sync"},
			{(void **)&cef_string_utf8_to_utf16,			"cef_string_utf8_to_utf16"},
			{(void **)&cef_string_utf16_to_utf8,			"cef_string_utf16_to_utf8"},
			{(void **)&cef_string_utf16_clear,				"cef_string_utf16_clear"},
			{(void **)&cef_string_utf16_set,				"cef_string_utf16_set"},
			{(void **)&cef_string_utf8_clear,				"cef_string_utf8_clear"},
			{(void **)&cef_string_utf8_set,					"cef_string_utf8_set"},
			{(void **)&cef_string_userfree_utf16_free,		"cef_string_userfree_utf16_free"},
			{(void **)&cef_register_scheme_handler_factory,	"cef_register_scheme_handler_factory"},
			{(void **)&cef_v8value_create_function,			"cef_v8value_create_function"},
			{(void **)&cef_v8value_create_string,			"cef_v8value_create_string"},
			{(void **)&cef_process_message_create,			"cef_process_message_create"},
			{(void **)&cef_v8context_get_current_context,	"cef_v8context_get_current_context"},
			{(void **)&cef_post_task,						"cef_post_task"},
			{(void **)&cef_request_context_create_context,	"cef_request_context_create_context"},
			{(void **)&cef_string_multimap_alloc,			"cef_string_multimap_alloc"},
			{(void **)&cef_string_multimap_append,			"cef_string_multimap_append"},
			{(void **)&cef_string_multimap_size,			"cef_string_multimap_size"},
			{(void **)&cef_string_multimap_key,				"cef_string_multimap_key"},
			{(void **)&cef_string_multimap_value,			"cef_string_multimap_value"},
			{(void **)&cef_string_multimap_free,			"cef_string_multimap_free"},
			{(void **)&cef_string_list_size,				"cef_string_list_size"},
			{(void **)&cef_string_list_value,				"cef_string_list_value"},
			{NULL}
		};

#ifdef _WIN32
		if (plugfuncs && !plugfuncs->LoadDLL("libcef", ceffuncs))
#else
		if (plugfuncs && !plugfuncs->LoadDLL("./libcef", ceffuncs))
#endif
		{
			if (engineprocess)
				Con_Printf("Unable to load libcef (version "CEF_VERSION")\n");
			return false;
		}
#endif

		if (engineprocess)
		{
			Con_DPrintf("libcef %i.%i.%i.%i (chrome %i.%i.%i.%i)\n", cef_version_info(0), cef_version_info(1), cef_version_info(2), cef_version_info(3), cef_version_info(4), cef_version_info(5), cef_version_info(6), cef_version_info(7));

			if (cef_version_info(0) != CEF_VERSION_MAJOR||
				cef_version_info(1) != CEF_VERSION_MINOR||
				cef_version_info(2) != CEF_VERSION_PATCH||
				cef_version_info(3) != CEF_COMMIT_NUMBER||
				cef_version_info(4) != CHROME_VERSION_MAJOR||
				cef_version_info(5) != CHROME_VERSION_MINOR||
				cef_version_info(6) != CHROME_VERSION_BUILD||
				cef_version_info(7) != CHROME_VERSION_PATCH)
			{	//the libcef api hash can be used to see if there's an api change that might break stuff.
				//refuse to load it if the api changed.
				Con_Printf("libcef outdated. Please install libcef version "CEF_VERSION"\n");
				return false;
			}
		}

		app_initialize();
		if (!engineprocess)
		{
			result = cef_execute_process(&args, &app, 0);
			if (result >= 0 || !engineprocess)
			{	//result is meant to be the exit code that the child process is meant to exit with
				//either way, we really don't want to return to the engine because that would run a second instance of it.
				exit(result);
				return qfalse;
			}
		}
	}
	return cefwasloaded=qtrue;
}
//works with the --plugwrapper engine argument
int NATIVEEXPORT CefSubprocessInit(plugcorefuncs_t *corefuncs)
{
	plugfuncs = corefuncs;
	return Cef_Init(false);
}

void Cef_ExecuteCommand(void)
{
	if (confuncs && Cef_Init(true))
	{
		static int sequence;
		char f[128];
		char videomap[8192];
		Q_snprintf(f, sizeof(f), "libcef:%i", ++sequence);
		newconsole = f;
		strcpy(videomap, "cef:");
		cmdfuncs->Argv(1, videomap+4, sizeof(videomap)-4);
		if (!videomap[4])
			strcpy(videomap, "cef:https://fte.triptohell.info");

		confuncs->SetConsoleString(f, "title", videomap+4);
		confuncs->SetConsoleFloat(f, "iswindow", true);
		confuncs->SetConsoleFloat(f, "forceutf8", true);
		confuncs->SetConsoleFloat(f, "wnd_w", 640+16);
		confuncs->SetConsoleFloat(f, "wnd_h", 480+16+8);
		confuncs->SetConsoleString(f, "backvideomap", videomap);
		confuncs->SetConsoleFloat(f, "linebuffered", 2);
		confuncs->SetActive(f);

		newconsole = NULL;
	}
}

static qboolean QDECL Cef_PluginMayUnload(void)
{
	if (cefwasinitialised)
		return false;	//cef is a piece of shite. we have to leave it running or the threads it spawns will crash+burn...
	return true;
}

qboolean Plug_Init(void)
{
	fsfuncs = (plugfsfuncs_t*)plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*fsfuncs));					//for fte://data/ scheme
	confuncs = (plugsubconsolefuncs_t*)plugfuncs->GetEngineInterface(plugsubconsolefuncs_name, sizeof(*confuncs));	//for cef command etc.
	clientfuncs = (plugclientfuncs_t*)plugfuncs->GetEngineInterface(plugclientfuncs_name, sizeof(*clientfuncs));	//for weird people trying to use xml requests to query game status (for hud stuff)
	if (!fsfuncs || !confuncs || !clientfuncs
		|| !plugfuncs->GetPluginName(-1, plugname, sizeof(plugname))
		|| !plugfuncs->ExportFunction("MayUnload", Cef_PluginMayUnload)
		|| !plugfuncs->ExportFunction("Tick", Cef_Tick)
		|| !plugfuncs->ExportFunction("Shutdown", Cef_Shutdown))
	{
		Con_Printf("CEF plugin failed: Required engine feature missing.\n");
		return false;
	}

	decoderfuncs.structsize = sizeof(media_decoder_funcs_t);
	decoderfuncs.drivername = "cef";
	decoderfuncs.createdecoder = Cef_CreateOld;
//	decoderfuncs.createdecoderCB = Cef_Create;
	decoderfuncs.decodeframe = Cef_DisplayFrame;
	decoderfuncs.shutdown = Cef_Destroy;
	decoderfuncs.cursormove = Cef_CursorMove;
	decoderfuncs.key = Cef_Key;
	decoderfuncs.setsize = Cef_SetSize;
	decoderfuncs.getsize = Cef_GetSize;
	decoderfuncs.changestream = Cef_ChangeStream;
	decoderfuncs.getproperty = Cef_GetProperty;

	if (!plugfuncs->ExportInterface("Media_VideoDecoder", &decoderfuncs, sizeof(decoderfuncs)))
	{
		Con_Printf("CEF plugin failed: Engine doesn't support media decoder plugins\n");
		return false;
	}

	cmdfuncs->AddCommand("cef", Cef_ExecuteCommand, "Open a web page!");

	cef_incognito = cvarfuncs->GetNVFDG("cef_incognito", "0", 0, NULL, "browser settings");
	cef_allowplugins = cvarfuncs->GetNVFDG("cef_allowplugins", "0", 0, NULL, "browser settings");
	cef_allowcvars = cvarfuncs->GetNVFDG("cef_allowcvars", "0", 0, NULL, "browser settings");
	cef_devtools = cvarfuncs->GetNVFDG("cef_devtools", "0", 0, NULL, "browser settings");

	return true;
}

