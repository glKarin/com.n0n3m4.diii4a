/*
 * Include this BEFORE darkplaces.h because it breaks wrapping
 * _Static_assert. Cloudwalk has no idea how or why so don't ask.
 */
#include <SDL.h>

#include "darkplaces.h"
#include "fs.h"
#include "vid.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <string.h>


EM_JS(float, js_GetViewportWidth, (void), {
	return document.documentElement.clientWidth
});
EM_JS(float, js_GetViewportHeight, (void), {
	return document.documentElement.clientHeight
});
static EM_BOOL em_on_resize(int etype, const EmscriptenUiEvent *event, void *UData)
{
	if(vid_resizable.integer)
	{
		Cvar_SetValueQuick(&vid_width, js_GetViewportWidth());
		Cvar_SetValueQuick(&vid_height, js_GetViewportHeight());
		Cvar_SetQuick(&vid_fullscreen, "0");
	}
	return EM_FALSE;
}


// =======================================================================
// General routines
// =======================================================================

#ifdef WASM_USER_ADJUSTABLE
EM_JS(char *, js_listfiles, (const char *directory), {
	if(UTF8ToString(directory) == "")
	{
		console.log("listing cwd");
		return stringToNewUTF8(FS.readdir(FS.cwd()).toString());
	}

	try
	{
		return stringToNewUTF8(FS.readdir(UTF8ToString(directory)).toString());
	}
	catch (error)
	{
		return stringToNewUTF8("directory not found");
	}
});
static void em_listfiles_f(cmd_state_t *cmd)
{
	char *output = js_listfiles(Cmd_Argc(cmd) == 2 ? Cmd_Argv(cmd, 1) : "");

	Con_Printf("%s\n", output);
	free(output);
}

EM_JS(char *, js_upload, (const char *todirectory), {
	if (UTF8ToString(todirectory).slice(-1) != "/")
	{
		currentname = UTF8ToString(todirectory) + "/";
	}
	else
	{
		currentname = UTF8ToString(todirectory);
	}

	file_selector.click();
	return stringToNewUTF8("Upload started");
});
static void em_upload_f(cmd_state_t *cmd)
{
	char *output = js_upload(Cmd_Argc(cmd) == 2 ? Cmd_Argv(cmd, 1) : fs_basedir);

	Con_Printf("%s\n", output);
	free(output);
}

EM_JS(char *, js_rm, (const char *path), {
	const mode = FS.lookupPath(UTF8ToString(path)).node.mode;

	if (FS.isFile(mode))
	{
		FS.unlink(UTF8ToString(path));
		return stringToNewUTF8("File removed");
	}

	return stringToNewUTF8(UTF8ToString(path)+" is not a File.");
});
static void em_rm_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 2)
		Con_Printf("No file to remove\n");
	else
	{
		char *output = js_rm(Cmd_Argv(cmd, 1));
		Con_Printf("%s\n", output);
		free(output);
	}
}

EM_JS(char *, js_rmdir, (const char *path), {
	const mode = FS.lookupPath(UTF8ToString(path)).node.mode;
	if (FS.isDir(mode))
	{
		try
		{
			FS.rmdir(UTF8ToString(path));
		}
		catch (error)
		{
			return stringToNewUTF8("Unable to remove directory. Is it not empty?");
		}
		return stringToNewUTF8("Directory removed");
	}

	return stringToNewUTF8(UTF8ToString(path)+" is not a directory.");
});
static void em_rmdir_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 2)
		Con_Printf("No directory to remove\n");
	else
	{
		char *output = js_rmdir(Cmd_Argv(cmd, 1));
		Con_Printf("%s\n", output);
		free(output);
	}
}

EM_JS(char *, js_mkd, (const char *path), {
	try
	{
		FS.mkdir(UTF8ToString(path));
	}
	catch (error)
	{
		return stringToNewUTF8("Unable to create directory. Does it already exist?");
	}
	return stringToNewUTF8(UTF8ToString(path)+" directory was created.");
});
static void em_mkdir_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 2)
		Con_Printf("No directory to create\n");
	else
	{
		char *output = js_mkd(Cmd_Argv(cmd, 1));
		Con_Printf("%s\n", output);
		free(output);
	}
}

EM_JS(char *, js_move, (const char *oldpath, const char *newpath), {
	try
	{
		FS.rename(UTF8ToString(oldpath),UTF8ToString(newpath))
	}
	catch (error)
	{
		return stringToNewUTF8("unable to move.");
	}
	return stringToNewUTF8("File Moved");
});
static void em_mv_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 3)
		Con_Printf("Nothing to move\n");
	else
	{
		char *output = js_move(Cmd_Argv(cmd,1), Cmd_Argv(cmd,2));
		Con_Printf("%s\n", output);
		free(output);
	}
}

static void em_wss_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 3)
		Con_Printf("Not Enough Arguments (Expected URL and subprotocol)\n");
	else
	{
		if(strcmp(Cmd_Argv(cmd,2),"binary") == 0 || strcmp(Cmd_Argv(cmd,2),"text") == 0)
			Con_Printf("Set Websocket URL to %s and subprotocol to %s.\n", Cmd_Argv(cmd,1), Cmd_Argv(cmd,2));
		else
			Con_Printf("subprotocol must be either binary or text\n");
	}
}
#endif // WASM_USER_ADJUSTABLE

EM_JS(bool, js_syncFS, (bool populate), {
	FS.syncfs(populate, function(err) {
		if(err)
		{
			alert("FileSystem Save Error: " + err);
			return false;
		}

		alert("Filesystem Saved!");
		return true;
	});
});
static void em_savefs_f(cmd_state_t *cmd)
{
	Con_Printf("Saving Files\n");
	js_syncFS(false);
}

void Sys_SDL_Shutdown(void)
{
	js_syncFS(false);
	SDL_Quit();
}

// Sys_Abort early in startup might screw with automated
// workflows or something if we show the dialog by default.
static qbool nocrashdialog = true;
void Sys_SDL_Dialog(const char *title, const char *string)
{
	if(!nocrashdialog)
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, string, NULL);
}

// In a browser the clipboard can only be read asynchronously
// doing this efficiently and cleanly requires JSPI
// enable in makefile.inc with emcc option: -s JSPI
/* TODO: enable this code when JSPI is enabled by default in browsers
EM_ASYNC_JS(char *, js_getclipboard, (void),
{
	try
	{
		const text = await navigator.clipboard.readText();
		return stringToNewUTF8(text);
	}
	catch (err)
	{
		return stringToNewUTF8("clipboard error: ", err);
	}
}); */
EM_JS(char *, js_getclipboard, (void), {
	return stringToNewUTF8("clipboard access requires JSPI which is not currently enabled.");
});
char *Sys_SDL_GetClipboardData (void)
{
	char *data = NULL;
	char *cliptext;

	// SDL_GetClipboardText() does nothing in a browser, see above
	cliptext = js_getclipboard();
	if (cliptext != NULL) {
		size_t allocsize;
		allocsize = min(MAX_INPUTLINE, strlen(cliptext) + 1);
		data = (char *)Z_Malloc (allocsize);
		dp_strlcpy (data, cliptext, allocsize);
		free(cliptext);
	}

	return data;
}

void Sys_SDL_Init(void)
{
	if (SDL_Init(0) < 0)
		Sys_Error("SDL_Init failed: %s\n", SDL_GetError());

	// we don't know which systems we'll want to init, yet...
	// COMMANDLINEOPTION: sdl: -nocrashdialog disables "Engine Error" crash dialog boxes
	if(!Sys_CheckParm("-nocrashdialog"))
		nocrashdialog = false;
	
	emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_FALSE, em_on_resize);
}

void Sys_EM_Register_Commands(void)
{
#ifdef WASM_USER_ADJUSTABLE
	Cmd_AddCommand(CF_SHARED, "em_ls", em_listfiles_f, "Lists Files in specified directory defaulting to the current working directory (Emscripten Only)");
	Cmd_AddCommand(CF_SHARED, "em_upload", em_upload_f, "Upload file to specified directory defaulting to basedir (Emscripten Only)");
	Cmd_AddCommand(CF_SHARED, "em_rm", em_rm_f, "Remove a file from game Filesystem (Emscripten Only)");
	Cmd_AddCommand(CF_SHARED, "em_rmdir", em_rmdir_f, "Remove a directory from game Filesystem (Emscripten Only)");
	Cmd_AddCommand(CF_SHARED, "em_mkdir", em_mkdir_f, "Make a directory in game Filesystem (Emscripten Only)");
	Cmd_AddCommand(CF_SHARED, "em_mv", em_mv_f, "Rename or Move an item in game Filesystem (Emscripten only)");
	Cmd_AddCommand(CF_SHARED, "em_wss", em_wss_f, "Set Websocket URL and Protocol (Emscripten Only)");
#endif
	Cmd_AddCommand(CF_SHARED, "em_save", em_savefs_f, "Save file changes to browser (Emscripten Only)");
}

qbool sys_supportsdlgetticks = true;
unsigned int Sys_SDL_GetTicks(void)
{
	return SDL_GetTicks();
}

void Sys_SDL_Delay(unsigned int milliseconds)
{
	SDL_Delay(milliseconds);
}

int main(int argc, char *argv[])
{
	return Sys_Main(argc, argv);
}
