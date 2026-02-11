#include "q3emisc.h"

#include <stdlib.h>
#include <string.h>

#include <android/log.h>

#include "q3estd.h"

#define LOG_TAG "Q3E::Misc"

// command line arguments
int q3e_argc = 0;
char **q3e_argv = NULL;

// Redirect stdout/stderr file
FILE *f_stdout = NULL;
FILE *f_stderr = NULL;

void Q3E_RedirectOutput(void)
{
	LOGI("Q3E open redirect output");
	f_stdout = freopen("stdout.txt","w",stdout);
	setvbuf(stdout, NULL, _IONBF, 0);
	f_stderr = freopen("stderr.txt","w",stderr);
	setvbuf(stderr, NULL, _IONBF, 0);
}

void Q3E_CloseRedirectOutput(void)
{
	LOGI("Q3E close redirect output");
	if(f_stdout != NULL)
	{
		fclose(f_stdout);
		f_stdout = NULL;
	}
	if(f_stderr != NULL)
	{
		fclose(f_stderr);
		f_stderr = NULL;
	}
}

void Q3E_DumpArgs(int argc, char **argv)
{
	LOGI("Q3E command line arguments: %d", argc);
	q3e_argc = argc;
	q3e_argv = (char **) malloc(sizeof(char *) * argc);
	for (int i = 0; i < argc; i++)
	{
		q3e_argv[i] = strdup(argv[i]);
		LOGI("  %d: %s", i, q3e_argv[i]);
	}
}

void Q3E_FreeArgs(void)
{
	LOGI("Q3E free args");
	for(int i = 0; i < q3e_argc; i++)
	{
		free(q3e_argv[i]);
	}
	free(q3e_argv);
}

void Q3E_PrintInterface(const Q3E_Interface_t *d3interface)
{
	LOGI("idTech4A++ interface ---------> ");

	LOGI("Main function: %p", d3interface->main);
	LOGI("Setup callbacks: %p", d3interface->setCallbacks);
	LOGI("Setup initial context: %p", d3interface->setInitialContext);

	LOGI("On pause: %p", d3interface->pause);
	LOGI("On resume: %p", d3interface->resume);
	LOGI("Exit function: %p", d3interface->exit);

	LOGI("Setup OpenGL context: %p", d3interface->setGLContext);
	LOGI("Request thread quit: %p", d3interface->requestThreadQuit);

	LOGI("Key event: %p", d3interface->keyEvent);
	LOGI("Analog event: %p", d3interface->analogEvent);
	LOGI("Motion event: %p", d3interface->motionEvent);
    LOGI("Mouse event: %p", d3interface->mouseEvent);

	LOGI("<---------");
}

void Q3E_PrintCallbacks(const Q3E_Callback_t *callback)
{
	LOGI("idTech4A++ callbacks ---------> ");

	LOGI("Q3E callback");
	LOGI("  AudioTrack: ");
	LOGI("    initAudio: %p", callback->AudioTrack_init);
	LOGI("    writeAudio: %p", callback->AudioTrack_write);
	LOGI("    shutdownAudio: %p", callback->AudioTrack_shutdown);
	LOGI("  Input: ");
	LOGI("    grab_mouse: %p", callback->Input_grabMouse);
	LOGI("    pull_input_event: %p", callback->Input_pullEvent);
	LOGI("    setup_smooth_joystick: %p", callback->Input_setupSmoothJoystick);
	LOGI("  System: ");
	LOGI("    attach_thread: %p", callback->Sys_attachThread);
	LOGI("    tmpfile: %p", callback->Sys_tmpfile);
	LOGI("    copy_to_clipboard: %p", callback->Sys_copyToClipboard);
	LOGI("    get_clipboard_text: %p", callback->Sys_getClipboardText);
	LOGI("    open_keyboard: %p", callback->Sys_openKeyboard);
	LOGI("    close_keyboard: %p", callback->Sys_closeKeyboard);
	LOGI("    open_url: %p", callback->Sys_openURL);
	LOGI("    exit_finish: %p", callback->Sys_exitFinish);
	LOGI("    show_cursor: %p", callback->Sys_showCursor);
	LOGI("  GUI: ");
	LOGI("    show_toast: %p", callback->Gui_ShowToast);
	LOGI("    open_dialog: %p", callback->Gui_openDialog);
	LOGI("  Other: ");
	LOGI("    setState: %p", callback->set_state);
    LOGI("    log_print: %p", callback->Log_Print);

	LOGI("<---------");
}

void Q3E_PrintInitialContext(const Q3E_InitialContext_t *context)
{
	LOGI("idTech4A++ initial context ---------> ");

	LOGI("Q3E initial context");
	LOGI("  OpenGL: ");
	LOGI("    Format: 0x%X", context->openGL_format);
	LOGI("    Depth bits: %d", context->openGL_depth);
	LOGI("    MSAA: %d", context->openGL_msaa);
	LOGI("    Version: %08x", context->openGL_version);
	LOGI("    Screen size: %d x %d", context->width, context->height);
	LOGI("    Window: %p", context->window);
	LOGI("  Variables: ");
	LOGI("    Native library directory: %s", context->nativeLibraryDir);
	LOGI("    Redirect output to file: %d", context->redirectOutputToFile);
	LOGI("    No handle signals: %d", context->noHandleSignals);
	LOGI("    Using mouse: %d", context->mouseAvailable);
	LOGI("    Game data directory: %s", context->gameDataDir);
	LOGI("    Application home directory: %s", context->appHomeDir);
	LOGI("    Refresh rate: %d", context->refreshRate);
	LOGI("    Smooth joystick: %d", context->smoothJoystick);
	LOGI("    Max console height frac: %d", context->consoleMaxHeightFrac);
	LOGI("    Using external libraries: %d", context->usingExternalLibs);
	LOGI("    Continue when missing OpenGL context: %d", context->continueWhenNoGLContext);

	LOGI("<---------");
}