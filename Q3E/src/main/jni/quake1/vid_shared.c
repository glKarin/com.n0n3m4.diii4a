
#include "quakedef.h"
#include "cdaudio.h"
#include "image.h"

#ifdef WIN32
//#include <XInput.h>
#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30
#define XUSER_INDEX_ANY                 0x000000FF

typedef struct xinput_gamepad_s
{
	WORD wButtons;
	BYTE bLeftTrigger;
	BYTE bRightTrigger;
	SHORT sThumbLX;
	SHORT sThumbLY;
	SHORT sThumbRX;
	SHORT sThumbRY;
}
xinput_gamepad_t;

typedef struct xinput_state_s
{
	DWORD dwPacketNumber;
	xinput_gamepad_t Gamepad;
}
xinput_state_t;

typedef struct xinput_keystroke_s
{
    WORD    VirtualKey;
    WCHAR   Unicode;
    WORD    Flags;
    BYTE    UserIndex;
    BYTE    HidCode;
}
xinput_keystroke_t;

DWORD (WINAPI *qXInputGetState)(DWORD index, xinput_state_t *state);
DWORD (WINAPI *qXInputGetKeystroke)(DWORD index, DWORD reserved, xinput_keystroke_t *keystroke);

qbool vid_xinputinitialized = false;
int vid_xinputindex = -1;
#endif

// global video state
viddef_t vid;

// AK FIXME -> input_dest
qbool in_client_mouse = true;

// AK where should it be placed ?
float in_mouse_x, in_mouse_y;
float in_windowmouse_x, in_windowmouse_y;

// LadyHavoc: if window is hidden, don't update screen
qbool vid_hidden = true;
// LadyHavoc: if window is not the active window, don't hog as much CPU time,
// let go of the mouse, turn off sound, and restore system gamma ramps...
qbool vid_activewindow = true;

vid_joystate_t vid_joystate;

#ifdef WIN32
cvar_t joy_xinputavailable = {CF_CLIENT | CF_READONLY, "joy_xinputavailable", "0", "indicates which devices are being reported by the Windows XInput API (first controller = 1, second = 2, third = 4, fourth = 8, added together)"};
#endif
cvar_t joy_active = {CF_CLIENT | CF_READONLY, "joy_active", "0", "indicates that a joystick is active (detected and enabled)"};
cvar_t joy_detected = {CF_CLIENT | CF_READONLY, "joy_detected", "0", "number of joysticks detected by engine"};
cvar_t joy_enable = {CF_CLIENT | CF_ARCHIVE, "joy_enable", "0", "enables joystick support"};
cvar_t joy_index = {CF_CLIENT, "joy_index", "0", "selects which joystick to use if you have multiple (0 uses the first controller, 1 uses the second, ...)"};
cvar_t joy_axisforward = {CF_CLIENT, "joy_axisforward", "1", "which joystick axis to query for forward/backward movement"};
cvar_t joy_axisside = {CF_CLIENT, "joy_axisside", "0", "which joystick axis to query for right/left movement"};
cvar_t joy_axisup = {CF_CLIENT, "joy_axisup", "-1", "which joystick axis to query for up/down movement"};
cvar_t joy_axispitch = {CF_CLIENT, "joy_axispitch", "3", "which joystick axis to query for looking up/down"};
cvar_t joy_axisyaw = {CF_CLIENT, "joy_axisyaw", "2", "which joystick axis to query for looking right/left"};
cvar_t joy_axisroll = {CF_CLIENT, "joy_axisroll", "-1", "which joystick axis to query for tilting head right/left"};
cvar_t joy_deadzoneforward = {CF_CLIENT, "joy_deadzoneforward", "0", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_deadzoneside = {CF_CLIENT, "joy_deadzoneside", "0", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_deadzoneup = {CF_CLIENT, "joy_deadzoneup", "0", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_deadzonepitch = {CF_CLIENT, "joy_deadzonepitch", "0", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_deadzoneyaw = {CF_CLIENT, "joy_deadzoneyaw", "0", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_deadzoneroll = {CF_CLIENT, "joy_deadzoneroll", "0", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_sensitivityforward = {CF_CLIENT, "joy_sensitivityforward", "-1", "movement multiplier"};
cvar_t joy_sensitivityside = {CF_CLIENT, "joy_sensitivityside", "1", "movement multiplier"};
cvar_t joy_sensitivityup = {CF_CLIENT, "joy_sensitivityup", "1", "movement multiplier"};
cvar_t joy_sensitivitypitch = {CF_CLIENT, "joy_sensitivitypitch", "1", "movement multiplier"};
cvar_t joy_sensitivityyaw = {CF_CLIENT, "joy_sensitivityyaw", "-1", "movement multiplier"};
cvar_t joy_sensitivityroll = {CF_CLIENT, "joy_sensitivityroll", "1", "movement multiplier"};
cvar_t joy_axiskeyevents = {CF_CLIENT | CF_ARCHIVE, "joy_axiskeyevents", "0", "generate uparrow/leftarrow etc. keyevents for joystick axes, use if your joystick driver is not generating them"};
cvar_t joy_axiskeyevents_deadzone = {CF_CLIENT | CF_ARCHIVE, "joy_axiskeyevents_deadzone", "0.5", "deadzone value for axes"};
cvar_t joy_x360_axisforward = {CF_CLIENT, "joy_x360_axisforward", "1", "which joystick axis to query for forward/backward movement"};
cvar_t joy_x360_axisside = {CF_CLIENT, "joy_x360_axisside", "0", "which joystick axis to query for right/left movement"};
cvar_t joy_x360_axisup = {CF_CLIENT, "joy_x360_axisup", "-1", "which joystick axis to query for up/down movement"};
cvar_t joy_x360_axispitch = {CF_CLIENT, "joy_x360_axispitch", "3", "which joystick axis to query for looking up/down"};
cvar_t joy_x360_axisyaw = {CF_CLIENT, "joy_x360_axisyaw", "2", "which joystick axis to query for looking right/left"};
cvar_t joy_x360_axisroll = {CF_CLIENT, "joy_x360_axisroll", "-1", "which joystick axis to query for tilting head right/left"};
cvar_t joy_x360_deadzoneforward = {CF_CLIENT, "joy_x360_deadzoneforward", "0.266", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_x360_deadzoneside = {CF_CLIENT, "joy_x360_deadzoneside", "0.266", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_x360_deadzoneup = {CF_CLIENT, "joy_x360_deadzoneup", "0.266", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_x360_deadzonepitch = {CF_CLIENT, "joy_x360_deadzonepitch", "0.266", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_x360_deadzoneyaw = {CF_CLIENT, "joy_x360_deadzoneyaw", "0.266", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_x360_deadzoneroll = {CF_CLIENT, "joy_x360_deadzoneroll", "0.266", "deadzone tolerance, suggested values are in the range 0 to 0.01"};
cvar_t joy_x360_sensitivityforward = {CF_CLIENT, "joy_x360_sensitivityforward", "1", "movement multiplier"};
cvar_t joy_x360_sensitivityside = {CF_CLIENT, "joy_x360_sensitivityside", "1", "movement multiplier"};
cvar_t joy_x360_sensitivityup = {CF_CLIENT, "joy_x360_sensitivityup", "1", "movement multiplier"};
cvar_t joy_x360_sensitivitypitch = {CF_CLIENT, "joy_x360_sensitivitypitch", "-1", "movement multiplier"};
cvar_t joy_x360_sensitivityyaw = {CF_CLIENT, "joy_x360_sensitivityyaw", "-1", "movement multiplier"};
cvar_t joy_x360_sensitivityroll = {CF_CLIENT, "joy_x360_sensitivityroll", "1", "movement multiplier"};

// VorteX: more info cvars, mostly set in VID_CheckExtensions
cvar_t gl_info_vendor = {CF_CLIENT | CF_READONLY, "gl_info_vendor", "", "indicates brand of graphics chip"};
cvar_t gl_info_renderer = {CF_CLIENT | CF_READONLY, "gl_info_renderer", "", "indicates graphics chip model and other information"};
cvar_t gl_info_version = {CF_CLIENT | CF_READONLY, "gl_info_version", "", "indicates version of current renderer. begins with 1.0.0, 1.1.0, 1.2.0, 1.3.1 etc."};
cvar_t gl_info_extensions = {CF_CLIENT | CF_READONLY, "gl_info_extensions", "", "indicates extension list found by engine, space separated."};
cvar_t gl_info_driver = {CF_CLIENT | CF_READONLY, "gl_info_driver", "", "name of driver library (opengl32.dll, libGL.so.1, or whatever)."};

cvar_t vid_fullscreen = {CF_CLIENT | CF_ARCHIVE, "vid_fullscreen", "1", "use fullscreen (1) or windowed (0)"};
cvar_t vid_borderless = {CF_CLIENT | CF_ARCHIVE, "vid_borderless", "0", "make the window borderless by removing all window decorations. has no effect in fullscreen mode"};
cvar_t vid_width = {CF_CLIENT | CF_ARCHIVE, "vid_width", "640", "resolution"};
cvar_t vid_height = {CF_CLIENT | CF_ARCHIVE, "vid_height", "480", "resolution"};
cvar_t vid_bitsperpixel = {CF_CLIENT | CF_READONLY, "vid_bitsperpixel", "32", "how many bits per pixel to render at (this is not currently configurable)"};
cvar_t vid_samples = {CF_CLIENT | CF_ARCHIVE, "vid_samples", "1", "how many anti-aliasing samples per pixel to request from the graphics driver (4 is recommended, 1 is faster)"};
cvar_t vid_refreshrate = {CF_CLIENT | CF_ARCHIVE, "vid_refreshrate", "0", "refresh rate to use, in hz (higher values feel smoother, if supported by your monitor), 0 uses the default"};
cvar_t vid_stereobuffer = {CF_CLIENT | CF_ARCHIVE, "vid_stereobuffer", "0", "enables 'quad-buffered' stereo rendering for stereo shutterglasses, HMD (head mounted display) devices, or polarized stereo LCDs, if supported by your drivers"};
// the density cvars are completely optional, set and use when something needs to have a density-independent size.
// TODO: set them when changing resolution, setting them from the commandline will be independent from the resolution - use only if you have a native fixed resolution.
// values for the Samsung Galaxy SIII, Snapdragon version: 2.000000 density, 304.799988 xdpi, 303.850464 ydpi
cvar_t vid_touchscreen_density = {CF_CLIENT, "vid_touchscreen_density", "2.0", "Standard quantized screen density multiplier (see Android documentation for DisplayMetrics), similar values are given on iPhoneOS"};
cvar_t vid_touchscreen_xdpi = {CF_CLIENT, "vid_touchscreen_xdpi", "300", "Horizontal DPI of the screen (only valid on Android currently)"};
cvar_t vid_touchscreen_ydpi = {CF_CLIENT, "vid_touchscreen_ydpi", "300", "Vertical DPI of the screen (only valid on Android currently)"};

cvar_t vid_vsync = {CF_CLIENT | CF_ARCHIVE, "vid_vsync", "0", "sync to vertical blank, prevents 'tearing' (seeing part of one frame and part of another on the screen at the same time) at the cost of latency, 1 always syncs and -1 is adaptive (stops syncing if the framerate drops, unsupported by some platforms), automatically disabled when doing timedemo benchmarks"};
cvar_t vid_mouse = {CF_CLIENT | CF_ARCHIVE, "vid_mouse", "1", "whether to use the mouse in windowed mode (fullscreen always does)"};
cvar_t vid_mouse_clickthrough = {CF_CLIENT | CF_ARCHIVE, "vid_mouse_clickthrough", "0", "mouse behavior in windowed mode: 0 = click to focus, 1 = allow interaction even if the window is not focused (click-through behaviour, can be useful when using third-party game overlays)"};
cvar_t vid_minimize_on_focus_loss = {CF_CLIENT | CF_ARCHIVE, "vid_minimize_on_focus_loss", "0", "whether to minimize the fullscreen window if it loses focus (such as by alt+tab)"};
cvar_t vid_grabkeyboard = {CF_CLIENT | CF_ARCHIVE, "vid_grabkeyboard", "0", "whether to grab the keyboard when mouse is active (prevents use of volume control keys, music player keys, etc on some keyboards)"};
cvar_t vid_minwidth = {CF_CLIENT, "vid_minwidth", "0", "minimum vid_width that is acceptable (to be set in default.cfg in mods)"};
cvar_t vid_minheight = {CF_CLIENT, "vid_minheight", "0", "minimum vid_height that is acceptable (to be set in default.cfg in mods)"};
cvar_t gl_finish = {CF_CLIENT | CF_CLIENT, "gl_finish", "0", "make the cpu wait for the graphics processor at the end of each rendered frame (can help with strange input or video lag problems on some machines)"};
cvar_t vid_sRGB = {CF_CLIENT | CF_ARCHIVE, "vid_sRGB", "0", "if hardware is capable, modify rendering to be gamma corrected for the sRGB color standard (computer monitors, TVs), recommended"};
cvar_t vid_sRGB_fallback = {CF_CLIENT | CF_ARCHIVE, "vid_sRGB_fallback", "0", "do an approximate sRGB fallback if not properly supported by hardware (2: also use the fallback if framebuffer is 8bit, 3: always use the fallback even if sRGB is supported)"};

cvar_t vid_touchscreen = {CF_CLIENT, "vid_touchscreen", "0", "Use touchscreen-style input (no mouse grab, track mouse motion only while button is down, screen areas for mimicing joystick axes and buttons"};
cvar_t vid_touchscreen_showkeyboard = {CF_CLIENT, "vid_touchscreen_showkeyboard", "0", "shows the platform's screen keyboard for text entry, can be set by csqc or menu qc if it wants to receive text input, does nothing if the platform has no screen keyboard"};
cvar_t vid_touchscreen_supportshowkeyboard = {CF_CLIENT | CF_READONLY, "vid_touchscreen_supportshowkeyboard", "0", "indicates if the platform supports a virtual keyboard"};
cvar_t vid_stick_mouse = {CF_CLIENT | CF_ARCHIVE, "vid_stick_mouse", "0", "have the mouse stuck in the center of the screen" };
cvar_t vid_resizable = {CF_CLIENT | CF_ARCHIVE, "vid_resizable", "1", "0: window not resizable, 1: resizable, 2: window can be resized but the framebuffer isn't adjusted" };
cvar_t vid_desktopfullscreen = {CF_CLIENT | CF_ARCHIVE, "vid_desktopfullscreen", "1", "force desktop resolution and refresh rate (disable modesetting), also use some OS-dependent tricks for better fullscreen integration; disabling may reveal OS/driver/SDL bugs with multi-monitor configurations"};
cvar_t vid_display = {CF_CLIENT | CF_ARCHIVE, "vid_display", "0", "which monitor to render on, numbered from 0 (system default)" };
cvar_t vid_info_displaycount = {CF_CLIENT | CF_READONLY, "vid_info_displaycount", "1", "how many monitors are currently available, updated by hotplug events" };
#ifdef WIN32
cvar_t vid_ignore_taskbar = {CF_CLIENT | CF_ARCHIVE, "vid_ignore_taskbar", "1", "in windowed mode, prevent the Windows taskbar and window borders from affecting the size and placement of the window. it will be aligned centered and uses the unaltered vid_width/vid_height values"};
#endif

cvar_t v_gamma = {CF_CLIENT | CF_ARCHIVE, "v_gamma", "1", "inverse gamma correction value, a brightness effect that does not affect white or black, and tends to make the image grey and dull"};
cvar_t v_contrast = {CF_CLIENT | CF_ARCHIVE, "v_contrast", "1", "brightness of white (values above 1 give a brighter image with increased color saturation, unlike v_gamma)"};
cvar_t v_brightness = {CF_CLIENT | CF_ARCHIVE, "v_brightness", "0", "brightness of black, useful for monitors that are too dark"};
cvar_t v_contrastboost = {CF_CLIENT | CF_ARCHIVE, "v_contrastboost", "1", "by how much to multiply the contrast in dark areas (1 is no change)"};
cvar_t v_color_enable = {CF_CLIENT | CF_ARCHIVE, "v_color_enable", "0", "enables black-grey-white color correction curve controls"};
cvar_t v_color_black_r = {CF_CLIENT | CF_ARCHIVE, "v_color_black_r", "0", "desired color of black"};
cvar_t v_color_black_g = {CF_CLIENT | CF_ARCHIVE, "v_color_black_g", "0", "desired color of black"};
cvar_t v_color_black_b = {CF_CLIENT | CF_ARCHIVE, "v_color_black_b", "0", "desired color of black"};
cvar_t v_color_grey_r = {CF_CLIENT | CF_ARCHIVE, "v_color_grey_r", "0.5", "desired color of grey"};
cvar_t v_color_grey_g = {CF_CLIENT | CF_ARCHIVE, "v_color_grey_g", "0.5", "desired color of grey"};
cvar_t v_color_grey_b = {CF_CLIENT | CF_ARCHIVE, "v_color_grey_b", "0.5", "desired color of grey"};
cvar_t v_color_white_r = {CF_CLIENT | CF_ARCHIVE, "v_color_white_r", "1", "desired color of white"};
cvar_t v_color_white_g = {CF_CLIENT | CF_ARCHIVE, "v_color_white_g", "1", "desired color of white"};
cvar_t v_color_white_b = {CF_CLIENT | CF_ARCHIVE, "v_color_white_b", "1", "desired color of white"};
cvar_t v_glslgamma_2d = {CF_CLIENT | CF_ARCHIVE, "v_glslgamma_2d", "1", "applies GLSL gamma to 2d pictures (HUD, fonts)"};
cvar_t v_psycho = {CF_CLIENT, "v_psycho", "0", "easter egg - R.I.P. zinx http://obits.al.com/obituaries/birmingham/obituary.aspx?n=christopher-robert-lais&pid=186080667"};

// brand of graphics chip
const char *gl_vendor;
// graphics chip model and other information
const char *gl_renderer;
// begins with 1.0.0, 1.1.0, 1.2.0, 1.2.1, 1.3.0, 1.3.1, or 1.4.0
const char *gl_version;

#ifndef USE_GLES2
GLboolean (GLAPIENTRY *qglIsBuffer) (GLuint buffer);
GLboolean (GLAPIENTRY *qglIsEnabled)(GLenum cap);
GLboolean (GLAPIENTRY *qglIsFramebuffer)(GLuint framebuffer);
GLboolean (GLAPIENTRY *qglIsQuery)(GLuint qid);
GLboolean (GLAPIENTRY *qglIsRenderbuffer)(GLuint renderbuffer);
GLboolean (GLAPIENTRY *qglUnmapBuffer) (GLenum target);
GLenum (GLAPIENTRY *qglCheckFramebufferStatus)(GLenum target);
GLenum (GLAPIENTRY *qglGetError)(void);
GLint (GLAPIENTRY *qglGetAttribLocation)(GLuint programObj, const GLchar *name);
GLint (GLAPIENTRY *qglGetUniformLocation)(GLuint programObj, const GLchar *name);
GLuint (GLAPIENTRY *qglCreateProgram)(void);
GLuint (GLAPIENTRY *qglCreateShader)(GLenum shaderType);
GLuint (GLAPIENTRY *qglGetDebugMessageLogARB)(GLuint count, GLsizei bufSize, GLenum* sources, GLenum* types, GLuint* ids, GLenum* severities, GLsizei* lengths, GLchar* messageLog);
GLuint (GLAPIENTRY *qglGetUniformBlockIndex)(GLuint program, const char* uniformBlockName);
GLvoid (GLAPIENTRY *qglBindFramebuffer)(GLenum target, GLuint framebuffer);
GLvoid (GLAPIENTRY *qglBindRenderbuffer)(GLenum target, GLuint renderbuffer);
GLvoid (GLAPIENTRY *qglBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
GLvoid (GLAPIENTRY *qglDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
GLvoid (GLAPIENTRY *qglDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
GLvoid (GLAPIENTRY *qglFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GLvoid (GLAPIENTRY *qglFramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLvoid (GLAPIENTRY *qglFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLvoid (GLAPIENTRY *qglFramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
GLvoid (GLAPIENTRY *qglFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
GLvoid (GLAPIENTRY *qglGenFramebuffers)(GLsizei n, GLuint *framebuffers);
GLvoid (GLAPIENTRY *qglGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
GLvoid (GLAPIENTRY *qglGenerateMipmap)(GLenum target);
GLvoid (GLAPIENTRY *qglGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
GLvoid (GLAPIENTRY *qglGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
GLvoid (GLAPIENTRY *qglRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GLvoid (GLAPIENTRY *qglRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
GLvoid* (GLAPIENTRY *qglMapBuffer) (GLenum target, GLenum access);
const GLubyte* (GLAPIENTRY *qglGetString)(GLenum name);
const GLubyte* (GLAPIENTRY *qglGetStringi)(GLenum name, GLuint index);
void (GLAPIENTRY *qglActiveTexture)(GLenum texture);
void (GLAPIENTRY *qglAttachShader)(GLuint containerObj, GLuint obj);
void (GLAPIENTRY *qglBeginQuery)(GLenum target, GLuint qid);
void (GLAPIENTRY *qglBindAttribLocation)(GLuint programObj, GLuint index, const GLchar *name);
void (GLAPIENTRY *qglBindBuffer) (GLenum target, GLuint buffer);
void (GLAPIENTRY *qglBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
void (GLAPIENTRY *qglBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
void (GLAPIENTRY *qglBindFragDataLocation)(GLuint programObj, GLuint index, const GLchar *name);
void (GLAPIENTRY *qglBindTexture)(GLenum target, GLuint texture);
void (GLAPIENTRY *qglBindVertexArray)(GLuint array);
void (GLAPIENTRY *qglBlendEquation)(GLenum); // also supplied by GL_blend_subtract
void (GLAPIENTRY *qglBlendFunc)(GLenum sfactor, GLenum dfactor);
void (GLAPIENTRY *qglBlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
void (GLAPIENTRY *qglBufferData) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
void (GLAPIENTRY *qglBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
void (GLAPIENTRY *qglClear)(GLbitfield mask);
void (GLAPIENTRY *qglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (GLAPIENTRY *qglClearDepth)(GLclampd depth);
void (GLAPIENTRY *qglClearStencil)(GLint s);
void (GLAPIENTRY *qglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void (GLAPIENTRY *qglCompileShader)(GLuint shaderObj);
void (GLAPIENTRY *qglCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
void (GLAPIENTRY *qglCompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
void (GLAPIENTRY *qglCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
void (GLAPIENTRY *qglCompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
void (GLAPIENTRY *qglCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void (GLAPIENTRY *qglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void (GLAPIENTRY *qglCopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void (GLAPIENTRY *qglCullFace)(GLenum mode);
void (GLAPIENTRY *qglDebugMessageCallbackARB)(GLDEBUGPROCARB callback, const GLvoid* userParam);
void (GLAPIENTRY *qglDebugMessageControlARB)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
void (GLAPIENTRY *qglDebugMessageInsertARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf);
void (GLAPIENTRY *qglDeleteBuffers) (GLsizei n, const GLuint *buffers);
void (GLAPIENTRY *qglDeleteProgram)(GLuint obj);
void (GLAPIENTRY *qglDeleteQueries)(GLsizei n, const GLuint *ids);
void (GLAPIENTRY *qglDeleteShader)(GLuint obj);
void (GLAPIENTRY *qglDeleteTextures)(GLsizei n, const GLuint *textures);
void (GLAPIENTRY *qglDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
void (GLAPIENTRY *qglDepthFunc)(GLenum func);
void (GLAPIENTRY *qglDepthMask)(GLboolean flag);
void (GLAPIENTRY *qglDepthRange)(GLclampd near_val, GLclampd far_val);
void (GLAPIENTRY *qglDepthRangef)(GLclampf near_val, GLclampf far_val);
void (GLAPIENTRY *qglDetachShader)(GLuint containerObj, GLuint attachedObj);
void (GLAPIENTRY *qglDisable)(GLenum cap);
void (GLAPIENTRY *qglDisableVertexAttribArray)(GLuint index);
void (GLAPIENTRY *qglDrawArrays)(GLenum mode, GLint first, GLsizei count);
void (GLAPIENTRY *qglDrawBuffer)(GLenum mode);
void (GLAPIENTRY *qglDrawBuffers)(GLsizei n, const GLenum *bufs);
void (GLAPIENTRY *qglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void (GLAPIENTRY *qglEnable)(GLenum cap);
void (GLAPIENTRY *qglEnableVertexAttribArray)(GLuint index);
void (GLAPIENTRY *qglEndQuery)(GLenum target);
void (GLAPIENTRY *qglFinish)(void);
void (GLAPIENTRY *qglFlush)(void);
void (GLAPIENTRY *qglGenBuffers) (GLsizei n, GLuint *buffers);
void (GLAPIENTRY *qglGenQueries)(GLsizei n, GLuint *ids);
void (GLAPIENTRY *qglGenTextures)(GLsizei n, GLuint *textures);
void (GLAPIENTRY *qglGenVertexArrays)(GLsizei n, GLuint *arrays);
void (GLAPIENTRY *qglGetActiveAttrib)(GLuint programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void (GLAPIENTRY *qglGetActiveUniform)(GLuint programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void (GLAPIENTRY *qglGetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, char* uniformBlockName);
void (GLAPIENTRY *qglGetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params);
void (GLAPIENTRY *qglGetActiveUniformName)(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, char* uniformName);
void (GLAPIENTRY *qglGetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params);
void (GLAPIENTRY *qglGetAttachedShaders)(GLuint containerObj, GLsizei maxCount, GLsizei *count, GLuint *obj);
void (GLAPIENTRY *qglGetBooleanv)(GLenum pname, GLboolean *params);
void (GLAPIENTRY *qglGetCompressedTexImage)(GLenum target, GLint lod, void *img);
void (GLAPIENTRY *qglGetDoublev)(GLenum pname, GLdouble *params);
void (GLAPIENTRY *qglGetFloatv)(GLenum pname, GLfloat *params);
void (GLAPIENTRY *qglGetIntegeri_v)(GLenum target, GLuint index, GLint* data);
void (GLAPIENTRY *qglGetIntegerv)(GLenum pname, GLint *params);
void (GLAPIENTRY *qglGetPointerv)(GLenum pname, GLvoid** params);
void (GLAPIENTRY *qglGetProgramInfoLog)(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void (GLAPIENTRY *qglGetProgramiv)(GLuint obj, GLenum pname, GLint *params);
void (GLAPIENTRY *qglGetQueryObjectiv)(GLuint qid, GLenum pname, GLint *params);
void (GLAPIENTRY *qglGetQueryObjectuiv)(GLuint qid, GLenum pname, GLuint *params);
void (GLAPIENTRY *qglGetQueryiv)(GLenum target, GLenum pname, GLint *params);
void (GLAPIENTRY *qglGetShaderInfoLog)(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void (GLAPIENTRY *qglGetShaderSource)(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *source);
void (GLAPIENTRY *qglGetShaderiv)(GLuint obj, GLenum pname, GLint *params);
void (GLAPIENTRY *qglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void (GLAPIENTRY *qglGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
void (GLAPIENTRY *qglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
void (GLAPIENTRY *qglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
void (GLAPIENTRY *qglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
void (GLAPIENTRY *qglGetUniformIndices)(GLuint program, GLsizei uniformCount, const char** uniformNames, GLuint* uniformIndices);
void (GLAPIENTRY *qglGetUniformfv)(GLuint programObj, GLint location, GLfloat *params);
void (GLAPIENTRY *qglGetUniformiv)(GLuint programObj, GLint location, GLint *params);
void (GLAPIENTRY *qglGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid **pointer);
void (GLAPIENTRY *qglGetVertexAttribdv)(GLuint index, GLenum pname, GLdouble *params);
void (GLAPIENTRY *qglGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat *params);
void (GLAPIENTRY *qglGetVertexAttribiv)(GLuint index, GLenum pname, GLint *params);
void (GLAPIENTRY *qglHint)(GLenum target, GLenum mode);
void (GLAPIENTRY *qglLinkProgram)(GLuint programObj);
void (GLAPIENTRY *qglPixelStorei)(GLenum pname, GLint param);
void (GLAPIENTRY *qglPointSize)(GLfloat size);
void (GLAPIENTRY *qglPolygonMode)(GLenum face, GLenum mode);
void (GLAPIENTRY *qglPolygonOffset)(GLfloat factor, GLfloat units);
void (GLAPIENTRY *qglReadBuffer)(GLenum mode);
void (GLAPIENTRY *qglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void (GLAPIENTRY *qglSampleCoverage)(GLclampf value, GLboolean invert);
void (GLAPIENTRY *qglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
void (GLAPIENTRY *qglShaderSource)(GLuint shaderObj, GLsizei count, const GLchar **string, const GLint *length);
void (GLAPIENTRY *qglStencilFunc)(GLenum func, GLint ref, GLuint mask);
void (GLAPIENTRY *qglStencilMask)(GLuint mask);
void (GLAPIENTRY *qglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
void (GLAPIENTRY *qglTexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void (GLAPIENTRY *qglTexImage3D)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void (GLAPIENTRY *qglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
void (GLAPIENTRY *qglTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
void (GLAPIENTRY *qglTexParameteri)(GLenum target, GLenum pname, GLint param);
void (GLAPIENTRY *qglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void (GLAPIENTRY *qglTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
void (GLAPIENTRY *qglUniform1f)(GLint location, GLfloat v0);
void (GLAPIENTRY *qglUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
void (GLAPIENTRY *qglUniform1i)(GLint location, GLint v0);
void (GLAPIENTRY *qglUniform1iv)(GLint location, GLsizei count, const GLint *value);
void (GLAPIENTRY *qglUniform2f)(GLint location, GLfloat v0, GLfloat v1);
void (GLAPIENTRY *qglUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
void (GLAPIENTRY *qglUniform2i)(GLint location, GLint v0, GLint v1);
void (GLAPIENTRY *qglUniform2iv)(GLint location, GLsizei count, const GLint *value);
void (GLAPIENTRY *qglUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void (GLAPIENTRY *qglUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
void (GLAPIENTRY *qglUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
void (GLAPIENTRY *qglUniform3iv)(GLint location, GLsizei count, const GLint *value);
void (GLAPIENTRY *qglUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void (GLAPIENTRY *qglUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
void (GLAPIENTRY *qglUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void (GLAPIENTRY *qglUniform4iv)(GLint location, GLsizei count, const GLint *value);
void (GLAPIENTRY *qglUniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
void (GLAPIENTRY *qglUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void (GLAPIENTRY *qglUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void (GLAPIENTRY *qglUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void (GLAPIENTRY *qglUseProgram)(GLuint programObj);
void (GLAPIENTRY *qglValidateProgram)(GLuint programObj);
void (GLAPIENTRY *qglVertexAttrib1d)(GLuint index, GLdouble v0);
void (GLAPIENTRY *qglVertexAttrib1dv)(GLuint index, const GLdouble *v);
void (GLAPIENTRY *qglVertexAttrib1f)(GLuint index, GLfloat v0);
void (GLAPIENTRY *qglVertexAttrib1fv)(GLuint index, const GLfloat *v);
void (GLAPIENTRY *qglVertexAttrib1s)(GLuint index, GLshort v0);
void (GLAPIENTRY *qglVertexAttrib1sv)(GLuint index, const GLshort *v);
void (GLAPIENTRY *qglVertexAttrib2d)(GLuint index, GLdouble v0, GLdouble v1);
void (GLAPIENTRY *qglVertexAttrib2dv)(GLuint index, const GLdouble *v);
void (GLAPIENTRY *qglVertexAttrib2f)(GLuint index, GLfloat v0, GLfloat v1);
void (GLAPIENTRY *qglVertexAttrib2fv)(GLuint index, const GLfloat *v);
void (GLAPIENTRY *qglVertexAttrib2s)(GLuint index, GLshort v0, GLshort v1);
void (GLAPIENTRY *qglVertexAttrib2sv)(GLuint index, const GLshort *v);
void (GLAPIENTRY *qglVertexAttrib3d)(GLuint index, GLdouble v0, GLdouble v1, GLdouble v2);
void (GLAPIENTRY *qglVertexAttrib3dv)(GLuint index, const GLdouble *v);
void (GLAPIENTRY *qglVertexAttrib3f)(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
void (GLAPIENTRY *qglVertexAttrib3fv)(GLuint index, const GLfloat *v);
void (GLAPIENTRY *qglVertexAttrib3s)(GLuint index, GLshort v0, GLshort v1, GLshort v2);
void (GLAPIENTRY *qglVertexAttrib3sv)(GLuint index, const GLshort *v);
void (GLAPIENTRY *qglVertexAttrib4Nbv)(GLuint index, const GLbyte *v);
void (GLAPIENTRY *qglVertexAttrib4Niv)(GLuint index, const GLint *v);
void (GLAPIENTRY *qglVertexAttrib4Nsv)(GLuint index, const GLshort *v);
void (GLAPIENTRY *qglVertexAttrib4Nub)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
void (GLAPIENTRY *qglVertexAttrib4Nubv)(GLuint index, const GLubyte *v);
void (GLAPIENTRY *qglVertexAttrib4Nuiv)(GLuint index, const GLuint *v);
void (GLAPIENTRY *qglVertexAttrib4Nusv)(GLuint index, const GLushort *v);
void (GLAPIENTRY *qglVertexAttrib4bv)(GLuint index, const GLbyte *v);
void (GLAPIENTRY *qglVertexAttrib4d)(GLuint index, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
void (GLAPIENTRY *qglVertexAttrib4dv)(GLuint index, const GLdouble *v);
void (GLAPIENTRY *qglVertexAttrib4f)(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void (GLAPIENTRY *qglVertexAttrib4fv)(GLuint index, const GLfloat *v);
void (GLAPIENTRY *qglVertexAttrib4iv)(GLuint index, const GLint *v);
void (GLAPIENTRY *qglVertexAttrib4s)(GLuint index, GLshort v0, GLshort v1, GLshort v2, GLshort v3);
void (GLAPIENTRY *qglVertexAttrib4sv)(GLuint index, const GLshort *v);
void (GLAPIENTRY *qglVertexAttrib4ubv)(GLuint index, const GLubyte *v);
void (GLAPIENTRY *qglVertexAttrib4uiv)(GLuint index, const GLuint *v);
void (GLAPIENTRY *qglVertexAttrib4usv)(GLuint index, const GLushort *v);
void (GLAPIENTRY *qglVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
void (GLAPIENTRY *qglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
#endif
#ifdef _GLDL
#if !defined(_GLDBG)
#define QGLPROC(name, rettype, args) rettype (GL_APIENTRYP q##name) args;
#include "android/qgl_proc.h"
#else
#define QGLPROC(name, rettype, args) rettype (GL_APIENTRYP __##name) args;
#include "android/qgl_proc.h"
#include "android/qgl_debug.inc"
#endif
#endif

#if _MSC_VER >= 1400
#define sscanf sscanf_s
#endif

typedef struct glfunction_s
{
	const char *extension;
	const char *name;
	void **funcvariable;
}
glfunction_t;

#ifndef USE_GLES2
// functions we look for - both core and extensions - it's okay if some of these are NULL for unsupported extensions.
static glfunction_t openglfuncs[] =
{
	{"core", "glActiveTexture", (void **) &qglActiveTexture},
	{"core", "glAttachShader", (void **) &qglAttachShader},
	{"core", "glBeginQuery", (void **) &qglBeginQuery},
	{"core", "glBindAttribLocation", (void **) &qglBindAttribLocation},
	{"core", "glBindBuffer", (void **) &qglBindBuffer},
	{"core", "glBindBufferBase", (void **) &qglBindBufferBase},
	{"core", "glBindBufferRange", (void **) &qglBindBufferRange},
	{"core", "glBindFramebuffer", (void **) &qglBindFramebuffer},
	{"core", "glBindRenderbuffer", (void **) &qglBindRenderbuffer},
	{"core", "glBindTexture", (void **) &qglBindTexture},
	{"core", "glBindVertexArray", (void **) &qglBindVertexArray},
	{"core", "glBlendEquation", (void **) &qglBlendEquation},
	{"core", "glBlendFunc", (void **) &qglBlendFunc},
	{"core", "glBlendFuncSeparate", (void **) &qglBlendFuncSeparate},
	{"core", "glBlitFramebuffer", (void **) &qglBlitFramebuffer},
	{"core", "glBufferData", (void **) &qglBufferData},
	{"core", "glBufferSubData", (void **) &qglBufferSubData},
	{"core", "glCheckFramebufferStatus", (void **) &qglCheckFramebufferStatus},
	{"core", "glClear", (void **) &qglClear},
	{"core", "glClearColor", (void **) &qglClearColor},
	{"core", "glClearDepth", (void **) &qglClearDepth},
	{"core", "glClearStencil", (void **) &qglClearStencil},
	{"core", "glColorMask", (void **) &qglColorMask},
	{"core", "glCompileShader", (void **) &qglCompileShader},
	{"core", "glCompressedTexImage2D", (void **) &qglCompressedTexImage2D},
	{"core", "glCompressedTexImage3D", (void **) &qglCompressedTexImage3D},
	{"core", "glCompressedTexSubImage2D", (void **) &qglCompressedTexSubImage2D},
	{"core", "glCompressedTexSubImage3D", (void **) &qglCompressedTexSubImage3D},
	{"core", "glCopyTexImage2D", (void **) &qglCopyTexImage2D},
	{"core", "glCopyTexSubImage2D", (void **) &qglCopyTexSubImage2D},
	{"core", "glCopyTexSubImage3D", (void **) &qglCopyTexSubImage3D},
	{"core", "glCreateProgram", (void **) &qglCreateProgram},
	{"core", "glCreateShader", (void **) &qglCreateShader},
	{"core", "glCullFace", (void **) &qglCullFace},
	{"core", "glDeleteBuffers", (void **) &qglDeleteBuffers},
	{"core", "glDeleteFramebuffers", (void **) &qglDeleteFramebuffers},
	{"core", "glDeleteProgram", (void **) &qglDeleteProgram},
	{"core", "glDeleteQueries", (void **) &qglDeleteQueries},
	{"core", "glDeleteRenderbuffers", (void **) &qglDeleteRenderbuffers},
	{"core", "glDeleteShader", (void **) &qglDeleteShader},
	{"core", "glDeleteTextures", (void **) &qglDeleteTextures},
	{"core", "glDeleteVertexArrays", (void **)&qglDeleteVertexArrays},
	{"core", "glDepthFunc", (void **) &qglDepthFunc},
	{"core", "glDepthMask", (void **) &qglDepthMask},
	{"core", "glDepthRange", (void **) &qglDepthRange},
	{"core", "glDepthRangef", (void **) &qglDepthRangef},
	{"core", "glDetachShader", (void **) &qglDetachShader},
	{"core", "glDisable", (void **) &qglDisable},
	{"core", "glDisableVertexAttribArray", (void **) &qglDisableVertexAttribArray},
	{"core", "glDrawArrays", (void **) &qglDrawArrays},
	{"core", "glDrawBuffer", (void **) &qglDrawBuffer},
	{"core", "glDrawBuffers", (void **) &qglDrawBuffers},
	{"core", "glDrawElements", (void **) &qglDrawElements},
	{"core", "glEnable", (void **) &qglEnable},
	{"core", "glEnableVertexAttribArray", (void **) &qglEnableVertexAttribArray},
	{"core", "glEndQuery", (void **) &qglEndQuery},
	{"core", "glFinish", (void **) &qglFinish},
	{"core", "glFlush", (void **) &qglFlush},
	{"core", "glFramebufferRenderbuffer", (void **) &qglFramebufferRenderbuffer},
	{"core", "glFramebufferTexture1D", (void **) &qglFramebufferTexture1D},
	{"core", "glFramebufferTexture2D", (void **) &qglFramebufferTexture2D},
	{"core", "glFramebufferTexture3D", (void **) &qglFramebufferTexture3D},
	{"core", "glFramebufferTextureLayer", (void **) &qglFramebufferTextureLayer},
	{"core", "glGenBuffers", (void **) &qglGenBuffers},
	{"core", "glGenFramebuffers", (void **) &qglGenFramebuffers},
	{"core", "glGenQueries", (void **) &qglGenQueries},
	{"core", "glGenRenderbuffers", (void **) &qglGenRenderbuffers},
	{"core", "glGenTextures", (void **) &qglGenTextures},
	{"core", "glGenVertexArrays", (void **)&qglGenVertexArrays},
	{"core", "glGenerateMipmap", (void **) &qglGenerateMipmap},
	{"core", "glGetActiveAttrib", (void **) &qglGetActiveAttrib},
	{"core", "glGetActiveUniform", (void **) &qglGetActiveUniform},
	{"core", "glGetActiveUniformBlockName", (void **) &qglGetActiveUniformBlockName},
	{"core", "glGetActiveUniformBlockiv", (void **) &qglGetActiveUniformBlockiv},
	{"core", "glGetActiveUniformName", (void **) &qglGetActiveUniformName},
	{"core", "glGetActiveUniformsiv", (void **) &qglGetActiveUniformsiv},
	{"core", "glGetAttachedShaders", (void **) &qglGetAttachedShaders},
	{"core", "glGetAttribLocation", (void **) &qglGetAttribLocation},
	{"core", "glGetBooleanv", (void **) &qglGetBooleanv},
	{"core", "glGetCompressedTexImage", (void **) &qglGetCompressedTexImage},
	{"core", "glGetDoublev", (void **) &qglGetDoublev},
	{"core", "glGetError", (void **) &qglGetError},
	{"core", "glGetFloatv", (void **) &qglGetFloatv},
	{"core", "glGetFramebufferAttachmentParameteriv", (void **) &qglGetFramebufferAttachmentParameteriv},
	{"core", "glGetIntegeri_v", (void **) &qglGetIntegeri_v},
	{"core", "glGetIntegerv", (void **) &qglGetIntegerv},
	{"core", "glGetProgramInfoLog", (void **) &qglGetProgramInfoLog},
	{"core", "glGetProgramiv", (void **) &qglGetProgramiv},
	{"core", "glGetQueryObjectiv", (void **) &qglGetQueryObjectiv},
	{"core", "glGetQueryObjectuiv", (void **) &qglGetQueryObjectuiv},
	{"core", "glGetQueryiv", (void **) &qglGetQueryiv},
	{"core", "glGetRenderbufferParameteriv", (void **) &qglGetRenderbufferParameteriv},
	{"core", "glGetShaderInfoLog", (void **) &qglGetShaderInfoLog},
	{"core", "glGetShaderSource", (void **) &qglGetShaderSource},
	{"core", "glGetShaderiv", (void **) &qglGetShaderiv},
	{"core", "glGetString", (void **) &qglGetString},
	{"core", "glGetStringi", (void **) &qglGetStringi},
	{"core", "glGetTexImage", (void **) &qglGetTexImage},
	{"core", "glGetTexLevelParameterfv", (void **) &qglGetTexLevelParameterfv},
	{"core", "glGetTexLevelParameteriv", (void **) &qglGetTexLevelParameteriv},
	{"core", "glGetTexParameterfv", (void **) &qglGetTexParameterfv},
	{"core", "glGetTexParameteriv", (void **) &qglGetTexParameteriv},
	{"core", "glGetUniformBlockIndex", (void **) &qglGetUniformBlockIndex},
	{"core", "glGetUniformIndices", (void **) &qglGetUniformIndices},
	{"core", "glGetUniformLocation", (void **) &qglGetUniformLocation},
	{"core", "glGetUniformfv", (void **) &qglGetUniformfv},
	{"core", "glGetUniformiv", (void **) &qglGetUniformiv},
	{"core", "glGetVertexAttribPointerv", (void **) &qglGetVertexAttribPointerv},
	{"core", "glGetVertexAttribdv", (void **) &qglGetVertexAttribdv},
	{"core", "glGetVertexAttribfv", (void **) &qglGetVertexAttribfv},
	{"core", "glGetVertexAttribiv", (void **) &qglGetVertexAttribiv},
	{"core", "glHint", (void **) &qglHint},
	{"core", "glIsBuffer", (void **) &qglIsBuffer},
	{"core", "glIsEnabled", (void **) &qglIsEnabled},
	{"core", "glIsFramebuffer", (void **) &qglIsFramebuffer},
	{"core", "glIsQuery", (void **) &qglIsQuery},
	{"core", "glIsRenderbuffer", (void **) &qglIsRenderbuffer},
	{"core", "glLinkProgram", (void **) &qglLinkProgram},
	{"core", "glMapBuffer", (void **) &qglMapBuffer},
	{"core", "glPixelStorei", (void **) &qglPixelStorei},
	{"core", "glPointSize", (void **) &qglPointSize},
	{"core", "glPolygonMode", (void **) &qglPolygonMode},
	{"core", "glPolygonOffset", (void **) &qglPolygonOffset},
	{"core", "glReadBuffer", (void **) &qglReadBuffer},
	{"core", "glReadPixels", (void **) &qglReadPixels},
	{"core", "glRenderbufferStorage", (void **) &qglRenderbufferStorage},
	{"core", "glRenderbufferStorageMultisample", (void **) &qglRenderbufferStorageMultisample},
	{"core", "glSampleCoverage", (void **) &qglSampleCoverage},
	{"core", "glScissor", (void **) &qglScissor},
	{"core", "glShaderSource", (void **) &qglShaderSource},
	{"core", "glStencilFunc", (void **) &qglStencilFunc},
	{"core", "glStencilMask", (void **) &qglStencilMask},
	{"core", "glStencilOp", (void **) &qglStencilOp},
	{"core", "glTexImage2D", (void **) &qglTexImage2D},
	{"core", "glTexImage3D", (void **) &qglTexImage3D},
	{"core", "glTexParameterf", (void **) &qglTexParameterf},
	{"core", "glTexParameterfv", (void **) &qglTexParameterfv},
	{"core", "glTexParameteri", (void **) &qglTexParameteri},
	{"core", "glTexSubImage2D", (void **) &qglTexSubImage2D},
	{"core", "glTexSubImage3D", (void **) &qglTexSubImage3D},
	{"core", "glUniform1f", (void **) &qglUniform1f},
	{"core", "glUniform1fv", (void **) &qglUniform1fv},
	{"core", "glUniform1i", (void **) &qglUniform1i},
	{"core", "glUniform1iv", (void **) &qglUniform1iv},
	{"core", "glUniform2f", (void **) &qglUniform2f},
	{"core", "glUniform2fv", (void **) &qglUniform2fv},
	{"core", "glUniform2i", (void **) &qglUniform2i},
	{"core", "glUniform2iv", (void **) &qglUniform2iv},
	{"core", "glUniform3f", (void **) &qglUniform3f},
	{"core", "glUniform3fv", (void **) &qglUniform3fv},
	{"core", "glUniform3i", (void **) &qglUniform3i},
	{"core", "glUniform3iv", (void **) &qglUniform3iv},
	{"core", "glUniform4f", (void **) &qglUniform4f},
	{"core", "glUniform4fv", (void **) &qglUniform4fv},
	{"core", "glUniform4i", (void **) &qglUniform4i},
	{"core", "glUniform4iv", (void **) &qglUniform4iv},
	{"core", "glUniformBlockBinding", (void **) &qglUniformBlockBinding},
	{"core", "glUniformMatrix2fv", (void **) &qglUniformMatrix2fv},
	{"core", "glUniformMatrix3fv", (void **) &qglUniformMatrix3fv},
	{"core", "glUniformMatrix4fv", (void **) &qglUniformMatrix4fv},
	{"core", "glUnmapBuffer", (void **) &qglUnmapBuffer},
	{"core", "glUseProgram", (void **) &qglUseProgram},
	{"core", "glValidateProgram", (void **) &qglValidateProgram},
	{"core", "glVertexAttrib1d", (void **) &qglVertexAttrib1d},
	{"core", "glVertexAttrib1dv", (void **) &qglVertexAttrib1dv},
	{"core", "glVertexAttrib1f", (void **) &qglVertexAttrib1f},
	{"core", "glVertexAttrib1fv", (void **) &qglVertexAttrib1fv},
	{"core", "glVertexAttrib1s", (void **) &qglVertexAttrib1s},
	{"core", "glVertexAttrib1sv", (void **) &qglVertexAttrib1sv},
	{"core", "glVertexAttrib2d", (void **) &qglVertexAttrib2d},
	{"core", "glVertexAttrib2dv", (void **) &qglVertexAttrib2dv},
	{"core", "glVertexAttrib2f", (void **) &qglVertexAttrib2f},
	{"core", "glVertexAttrib2fv", (void **) &qglVertexAttrib2fv},
	{"core", "glVertexAttrib2s", (void **) &qglVertexAttrib2s},
	{"core", "glVertexAttrib2sv", (void **) &qglVertexAttrib2sv},
	{"core", "glVertexAttrib3d", (void **) &qglVertexAttrib3d},
	{"core", "glVertexAttrib3dv", (void **) &qglVertexAttrib3dv},
	{"core", "glVertexAttrib3f", (void **) &qglVertexAttrib3f},
	{"core", "glVertexAttrib3fv", (void **) &qglVertexAttrib3fv},
	{"core", "glVertexAttrib3s", (void **) &qglVertexAttrib3s},
	{"core", "glVertexAttrib3sv", (void **) &qglVertexAttrib3sv},
	{"core", "glVertexAttrib4Nbv", (void **) &qglVertexAttrib4Nbv},
	{"core", "glVertexAttrib4Niv", (void **) &qglVertexAttrib4Niv},
	{"core", "glVertexAttrib4Nsv", (void **) &qglVertexAttrib4Nsv},
	{"core", "glVertexAttrib4Nub", (void **) &qglVertexAttrib4Nub},
	{"core", "glVertexAttrib4Nubv", (void **) &qglVertexAttrib4Nubv},
	{"core", "glVertexAttrib4Nuiv", (void **) &qglVertexAttrib4Nuiv},
	{"core", "glVertexAttrib4Nusv", (void **) &qglVertexAttrib4Nusv},
	{"core", "glVertexAttrib4bv", (void **) &qglVertexAttrib4bv},
	{"core", "glVertexAttrib4d", (void **) &qglVertexAttrib4d},
	{"core", "glVertexAttrib4dv", (void **) &qglVertexAttrib4dv},
	{"core", "glVertexAttrib4f", (void **) &qglVertexAttrib4f},
	{"core", "glVertexAttrib4fv", (void **) &qglVertexAttrib4fv},
	{"core", "glVertexAttrib4iv", (void **) &qglVertexAttrib4iv},
	{"core", "glVertexAttrib4s", (void **) &qglVertexAttrib4s},
	{"core", "glVertexAttrib4sv", (void **) &qglVertexAttrib4sv},
	{"core", "glVertexAttrib4ubv", (void **) &qglVertexAttrib4ubv},
	{"core", "glVertexAttrib4uiv", (void **) &qglVertexAttrib4uiv},
	{"core", "glVertexAttrib4usv", (void **) &qglVertexAttrib4usv},
	{"core", "glVertexAttribPointer", (void **) &qglVertexAttribPointer},
	{"core", "glViewport", (void **) &qglViewport},
	{"glBindFragDataLocation", "glBindFragDataLocation", (void **) &qglBindFragDataLocation}, // optional (no preference)
	{"GL_ARB_debug_output", "glDebugMessageControlARB", (void **)&qglDebugMessageControlARB},
	{"GL_ARB_debug_output", "glDebugMessageInsertARB", (void **)&qglDebugMessageInsertARB},
	{"GL_ARB_debug_output", "glDebugMessageCallbackARB", (void **)&qglDebugMessageCallbackARB},
	{"GL_ARB_debug_output", "glGetDebugMessageLogARB", (void **)&qglGetDebugMessageLogARB},
	{"GL_ARB_debug_output", "glGetPointerv", (void **)&qglGetPointerv},
	{NULL, NULL, NULL}
};
#endif
#ifdef _GLDL
#if !defined(_GLDBG)
#define QGLPROC(name, rettype, args) { "core", #name, (void **)&q##name },
#else
#define QGLPROC(name, rettype, args) { "core", #name, (void **)&__##name },
#endif
// functions we look for - both core and extensions - it's okay if some of these are NULL for unsupported extensions.
static glfunction_t openglfuncs[] =
{
	#include "android/qgl_proc.h"
	{NULL, NULL, NULL}
};
#endif

qbool GL_CheckExtension(const char *name, const char *disableparm, int silent)
{
	int failed = false;
	const glfunction_t *func;
	char extstr[MAX_INPUTLINE];

	Con_DPrintf("checking for %s...  ", name);

	if (disableparm && (Sys_CheckParm(disableparm) || Sys_CheckParm("-safe")))
	{
		Con_DPrint("disabled by commandline\n");
		return false;
	}

#ifdef _NOSDL
	if(strcmp("GL_ARB_texture_cube_map", name))
#endif
	if (!GL_ExtensionSupported(name))
	{
		Con_DPrint("not detected\n");
		return false;
	}

#ifndef USE_GLES2
	for (func = openglfuncs; func && func->name != NULL; func++)
	{
		if (!*func->funcvariable && !strcmp(name, func->extension))
		{
			if (!silent)
				Con_DPrintf("%s is missing function \"%s\" - broken driver!\n", name, func->name);
			failed = true;
		}
	}
#endif //USE_GLES2
	// delay the return so it prints all missing functions
	if (failed)
		return false;
	// VorteX: add to found extension list
	dpsnprintf(extstr, sizeof(extstr), "%s %s ", gl_info_extensions.string, name);
	Cvar_SetQuick(&gl_info_extensions, extstr);

	Con_DPrint("enabled\n");
	return true;
}

void VID_ClearExtensions(void)
{
	// VorteX: reset extensions info cvar, it got filled by GL_CheckExtension
	Cvar_SetQuick(&gl_info_extensions, "");

	// clear the extension flags
	memset(&vid.support, 0, sizeof(vid.support));
}

void GL_InitFunctions(void)
{
#ifdef _GLDL
	const glfunction_t *func;

	// first fetch the function pointers for everything - after this we can begin making GL calls.
	for (func = openglfuncs; func->name != NULL; func++)
		*func->funcvariable = (void *)GL_GetProcAddress(func->name);
#endif
#ifndef USE_GLES2
	const glfunction_t *func;
	qbool missingrequiredfuncs = false;
	static char missingfuncs[16384];

	// first fetch the function pointers for everything - after this we can begin making GL calls.
	for (func = openglfuncs; func->name != NULL; func++)
		*func->funcvariable = (void *)GL_GetProcAddress(func->name);

	missingfuncs[0] = 0;
	for (func = openglfuncs; func && func->name != NULL; func++)
	{
		if (!*func->funcvariable && !strcmp(func->extension, "core"))
		{
			Con_DPrintf("GL context is missing required function \"%s\"!\n", func->name);
			missingrequiredfuncs = true;
			dp_strlcat(missingfuncs, " ", sizeof(missingfuncs));
			dp_strlcat(missingfuncs, func->name, sizeof(missingfuncs));
		}
	}

	if (missingrequiredfuncs)
		Sys_Error("OpenGL driver/hardware lacks required features:\n%s", missingfuncs);
#endif
}

void GL_Setup(void)
{
	char *s;
	int j;
	GLint numextensions = 0;
	int majorv, minorv;

	gl_renderer = (const char *)qglGetString(GL_RENDERER);
	gl_vendor = (const char *)qglGetString(GL_VENDOR);
	gl_version = (const char *)qglGetString(GL_VERSION);

	Con_Printf("GL_VENDOR: %s\n", gl_vendor);
	Con_Printf("GL_RENDERER: %s\n", gl_renderer);
	Con_Printf("GL_VERSION: %s\n", gl_version);

#ifndef USE_GLES2
	qglGetIntegerv(GL_MAJOR_VERSION, &majorv);
	qglGetIntegerv(GL_MINOR_VERSION, &minorv);
	vid.support.glversion = 10 * majorv + minorv;
	if (vid.support.glversion < 32)
		// fallback, should never get here: GL context creation should have failed
		Sys_Error("OpenGL driver/hardware supports version %i.%i but 3.2 is the minimum\n", majorv, minorv);

	qglGetIntegerv(GL_NUM_EXTENSIONS, &numextensions);
	Con_DPrint("GL_EXTENSIONS:\n");
	for (j = 0; j < numextensions; j++)
	{
		const char *ext = (const char *)qglGetStringi(GL_EXTENSIONS, j);
		Con_DPrintf(" %s", ext);
		if(j && !(j % 3))
			Con_DPrintf("\n");
	}
	Con_DPrint("\n");
#endif //USE_GLES2

	Con_DPrint("Checking OpenGL extensions...\n");

	// detect what GLSL version is available, to enable features like higher quality reliefmapping
	vid.support.glshaderversion = 100;
	s = (char *) qglGetString(GL_SHADING_LANGUAGE_VERSION);
	if (s)
		vid.support.glshaderversion = (int)(atof(s) * 100.0f + 0.5f);
	if (vid.support.glshaderversion < 100)
		vid.support.glshaderversion = 100;
#ifdef _NOSDL
	vid.support.glshaderversion = 100;
#endif
	Con_Printf("Detected GLSL version %i\n", vid.support.glshaderversion);

#ifdef USE_GLES2
	// GLES devices in general do not like GL_BGRA, so use GL_RGBA
	vid.forcetextype = TEXTYPE_RGBA;
#else
	// GL drivers generally prefer GL_BGRA
	vid.forcetextype = GL_BGRA;
#endif

	vid.support.amd_texture_texture4 = GL_CheckExtension("GL_AMD_texture_texture4", "-notexture4", false);
	vid.support.arb_texture_gather = GL_CheckExtension("GL_ARB_texture_gather", "-notexturegather", false);
	vid.support.ext_texture_compression_s3tc = GL_CheckExtension("GL_EXT_texture_compression_s3tc", "-nos3tc", false);
	vid.support.ext_texture_filter_anisotropic = GL_CheckExtension("GL_EXT_texture_filter_anisotropic", "-noanisotropy", false);
#ifndef USE_GLES2
	vid.support.ext_texture_srgb = true; // GL3 core, but not GLES2
#endif
	vid.support.arb_debug_output = GL_CheckExtension("GL_ARB_debug_output", "-nogldebugoutput", false);
	vid.allowalphatocoverage = false;

// COMMANDLINEOPTION: GL: -noanisotropy disables GL_EXT_texture_filter_anisotropic (allows higher quality texturing)
// COMMANDLINEOPTION: GL: -nos3tc disables GL_EXT_texture_compression_s3tc (which allows use of .dds texture caching)
// COMMANDLINEOPTION: GL: -notexture4 disables GL_AMD_texture_texture4 (which provides fetch4 sampling)
// COMMANDLINEOPTION: GL: -notexturegather disables GL_ARB_texture_gather (which provides fetch4 sampling)
// COMMANDLINEOPTION: GL: -nogldebugoutput disables GL_ARB_debug_output (which provides the gl_debug feature, if enabled)

#ifdef WIN32
	// gl_texturecompression_color is somehow broken on AMD's Windows driver,
	// see: https://gitlab.com/xonotic/darkplaces/-/issues/228
	// HACK: force it off (less bad than adding hacky checks to the renderer)
	if (strncmp(gl_renderer, "AMD Radeon(TM)", 14) == 0)
	{
		Cvar_SetQuick(&gl_texturecompression_color, "0");
		gl_texturecompression_color.flags |= CF_READONLY;
	}
#endif

#ifdef GL_MAX_DRAW_BUFFERS
	qglGetIntegerv(GL_MAX_DRAW_BUFFERS, (GLint*)&vid.maxdrawbuffers);
	CHECKGLERROR
#endif
	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&vid.maxtexturesize_2d);
	CHECKGLERROR
#ifdef GL_MAX_CUBE_MAP_TEXTURE_SIZE
#ifdef USE_GLES2
	if (GL_CheckExtension("GL_ARB_texture_cube_map", "-nocubemap", false))
#endif
	{
		qglGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, (GLint*)&vid.maxtexturesize_cubemap);
		Con_DPrintf("GL_MAX_CUBE_MAP_TEXTURE_SIZE = %i\n", vid.maxtexturesize_cubemap);
	}
	CHECKGLERROR
#endif
#ifdef GL_MAX_3D_TEXTURE_SIZE
#ifdef USE_GLES2
	if (GL_CheckExtension("GL_EXT_texture3D", "-notexture3d", false)
	 || GL_CheckExtension("GL_OES_texture3D", "-notexture3d", false))
#endif
	{
		qglGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, (GLint*)&vid.maxtexturesize_3d);
		Con_DPrintf("GL_MAX_3D_TEXTURE_SIZE = %i\n", vid.maxtexturesize_3d);
	}
#endif
	CHECKGLERROR

#ifdef USE_GLES2
	Con_Print("Using GLES2 rendering path\n");
	vid.renderpath = RENDERPATH_GLES2;
	vid.sRGBcapable2D = false;
	vid.sRGBcapable3D = false;
#else
	Con_Print("Using GL32 rendering path\n");
	vid.renderpath = RENDERPATH_GL32;
	vid.sRGBcapable2D = false;
	vid.sRGBcapable3D = true;
	// enable multisample antialiasing if possible
	vid.allowalphatocoverage = true; // but see below, it may get turned to false again if GL_SAMPLES is <= 1
	{
		int samples = 0;
		qglGetIntegerv(GL_SAMPLES, &samples);
		vid.mode.samples = samples;
		if (samples > 1)
			qglEnable(GL_MULTISAMPLE);
		else
			vid.allowalphatocoverage = false;
	}
	// currently MSAA antialiasing is not implemented for fbo viewports, so we actually have to force this off anyway.
	vid.allowalphatocoverage = false;
#endif
	CHECKGLERROR

#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
	if (vid.support.ext_texture_filter_anisotropic)
		qglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint*)&vid.max_anisotropy);
#endif
	CHECKGLERROR
}

float VID_JoyState_GetAxis(const vid_joystate_t *joystate, int axis, float fsensitivity, float deadzone)
{
	float value;
	value = (axis >= 0 && axis < MAXJOYAXIS) ? joystate->axis[axis] : 0.0f;
	value = value > deadzone ? (value - deadzone) : (value < -deadzone ? (value + deadzone) : 0.0f);
	value *= deadzone > 0 ? (1.0f / (1.0f - deadzone)) : 1.0f;
	value = bound(-1, value, 1);
	return value * fsensitivity;
}

qbool VID_JoyBlockEmulatedKeys(int keycode)
{
	int j;
	vid_joystate_t joystate;

	if (!joy_axiskeyevents.integer)
		return false;
	if (vid_joystate.is360)
		return false;
	if (keycode != K_UPARROW && keycode != K_DOWNARROW && keycode != K_RIGHTARROW && keycode != K_LEFTARROW)
		return false;

	// block system-generated key events for arrow keys if we're emulating the arrow keys ourselves
	VID_BuildJoyState(&joystate);
	for (j = 32;j < 36;j++)
		if (vid_joystate.button[j] || joystate.button[j])
			return true;

	return false;
}

void VID_Shared_BuildJoyState_Begin(vid_joystate_t *joystate)
{
#ifdef WIN32
	xinput_state_t xinputstate;
#endif
	memset(joystate, 0, sizeof(*joystate));
#ifdef WIN32
	if (vid_xinputindex >= 0 && qXInputGetState && qXInputGetState(vid_xinputindex, &xinputstate) == S_OK)
	{
		joystate->is360 = true;
		joystate->button[ 0] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
		joystate->button[ 1] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
		joystate->button[ 2] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
		joystate->button[ 3] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
		joystate->button[ 4] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
		joystate->button[ 5] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
		joystate->button[ 6] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
		joystate->button[ 7] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
		joystate->button[ 8] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
		joystate->button[ 9] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
		joystate->button[10] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
		joystate->button[11] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
		joystate->button[12] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
		joystate->button[13] = (xinputstate.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
		joystate->button[14] = xinputstate.Gamepad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		joystate->button[15] = xinputstate.Gamepad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		joystate->button[16] = xinputstate.Gamepad.sThumbLY < -16384;
		joystate->button[17] = xinputstate.Gamepad.sThumbLY >  16384;
		joystate->button[18] = xinputstate.Gamepad.sThumbLX < -16384;
		joystate->button[19] = xinputstate.Gamepad.sThumbLX >  16384;
		joystate->button[20] = xinputstate.Gamepad.sThumbRY < -16384;
		joystate->button[21] = xinputstate.Gamepad.sThumbRY >  16384;
		joystate->button[22] = xinputstate.Gamepad.sThumbRX < -16384;
		joystate->button[23] = xinputstate.Gamepad.sThumbRX >  16384;
		joystate->axis[ 4] = xinputstate.Gamepad.bLeftTrigger * (1.0f / 255.0f);
		joystate->axis[ 5] = xinputstate.Gamepad.bRightTrigger * (1.0f / 255.0f);
		joystate->axis[ 0] = xinputstate.Gamepad.sThumbLX * (1.0f / 32767.0f);
		joystate->axis[ 1] = xinputstate.Gamepad.sThumbLY * (1.0f / 32767.0f);
		joystate->axis[ 2] = xinputstate.Gamepad.sThumbRX * (1.0f / 32767.0f);
		joystate->axis[ 3] = xinputstate.Gamepad.sThumbRY * (1.0f / 32767.0f);
	}
#endif
}

void VID_Shared_BuildJoyState_Finish(vid_joystate_t *joystate)
{
	float f, r;
	if (joystate->is360)
		return;
	// emulate key events for thumbstick
	f = VID_JoyState_GetAxis(joystate, joy_axisforward.integer, 1, joy_axiskeyevents_deadzone.value) * joy_sensitivityforward.value;
	r = VID_JoyState_GetAxis(joystate, joy_axisside.integer   , 1, joy_axiskeyevents_deadzone.value) * joy_sensitivityside.value;
#if MAXJOYBUTTON != 36
#error this code must be updated if MAXJOYBUTTON changes!
#endif
	joystate->button[32] = f > 0.0f;
	joystate->button[33] = f < 0.0f;
	joystate->button[34] = r > 0.0f;
	joystate->button[35] = r < 0.0f;
}

static void VID_KeyEventForButton(qbool oldbutton, qbool newbutton, int key, double *timer)
{
	if (oldbutton)
	{
		if (newbutton)
		{
			if (host.realtime >= *timer)
			{
				Key_Event(key, 0, true);
				*timer = host.realtime + 0.1;
			}
		}
		else
		{
			Key_Event(key, 0, false);
			*timer = 0;
		}
	}
	else
	{
		if (newbutton)
		{
			Key_Event(key, 0, true);
			*timer = host.realtime + 0.5;
		}
	}
}

#if MAXJOYBUTTON != 36
#error this code must be updated if MAXJOYBUTTON changes!
#endif
static int joybuttonkey[MAXJOYBUTTON][2] =
{
	{K_JOY1, K_ENTER}, {K_JOY2, K_ESCAPE}, {K_JOY3, 0}, {K_JOY4, 0}, {K_JOY5, 0}, {K_JOY6, 0}, {K_JOY7, 0}, {K_JOY8, 0}, {K_JOY9, 0}, {K_JOY10, 0}, {K_JOY11, 0}, {K_JOY12, 0}, {K_JOY13, 0}, {K_JOY14, 0}, {K_JOY15, 0}, {K_JOY16, 0},
	{K_AUX1, 0}, {K_AUX2, 0}, {K_AUX3, 0}, {K_AUX4, 0}, {K_AUX5, 0}, {K_AUX6, 0}, {K_AUX7, 0}, {K_AUX8, 0}, {K_AUX9, 0}, {K_AUX10, 0}, {K_AUX11, 0}, {K_AUX12, 0}, {K_AUX13, 0}, {K_AUX14, 0}, {K_AUX15, 0}, {K_AUX16, 0},
	{K_JOY_UP, K_UPARROW}, {K_JOY_DOWN, K_DOWNARROW}, {K_JOY_RIGHT, K_RIGHTARROW}, {K_JOY_LEFT, K_LEFTARROW},
};

static int joybuttonkey360[][2] =
{
	{K_X360_DPAD_UP, K_UPARROW},
	{K_X360_DPAD_DOWN, K_DOWNARROW},
	{K_X360_DPAD_LEFT, K_LEFTARROW},
	{K_X360_DPAD_RIGHT, K_RIGHTARROW},
	{K_X360_START, K_ESCAPE},
	{K_X360_BACK, K_ESCAPE},
	{K_X360_LEFT_THUMB, 0},
	{K_X360_RIGHT_THUMB, 0},
	{K_X360_LEFT_SHOULDER, 0},
	{K_X360_RIGHT_SHOULDER, 0},
	{K_X360_A, K_ENTER},
	{K_X360_B, K_ESCAPE},
	{K_X360_X, 0},
	{K_X360_Y, 0},
	{K_X360_LEFT_TRIGGER, 0},
	{K_X360_RIGHT_TRIGGER, 0},
	{K_X360_LEFT_THUMB_DOWN, K_DOWNARROW},
	{K_X360_LEFT_THUMB_UP, K_UPARROW},
	{K_X360_LEFT_THUMB_LEFT, K_LEFTARROW},
	{K_X360_LEFT_THUMB_RIGHT, K_RIGHTARROW},
	{K_X360_RIGHT_THUMB_DOWN, 0},
	{K_X360_RIGHT_THUMB_UP, 0},
	{K_X360_RIGHT_THUMB_LEFT, 0},
	{K_X360_RIGHT_THUMB_RIGHT, 0},
};

double vid_joybuttontimer[MAXJOYBUTTON];
void VID_ApplyJoyState(vid_joystate_t *joystate)
{
	int j;
	int c = joy_axiskeyevents.integer != 0;
	if (joystate->is360)
	{
#if 0
		// keystrokes (chatpad)
		// DOES NOT WORK - no driver support in xinput1_3.dll :(
		xinput_keystroke_t keystroke;
		while (qXInputGetKeystroke && qXInputGetKeystroke(XUSER_INDEX_ANY, 0, &keystroke) == S_OK)
			Con_Printf("XInput KeyStroke: VirtualKey %i, Unicode %i, Flags %x, UserIndex %i, HidCode %i\n", keystroke.VirtualKey, keystroke.Unicode, keystroke.Flags, keystroke.UserIndex, keystroke.HidCode);
#endif

		// emit key events for buttons
		for (j = 0;j < (int)(sizeof(joybuttonkey360)/sizeof(joybuttonkey360[0]));j++)
			VID_KeyEventForButton(vid_joystate.button[j] != 0, joystate->button[j] != 0, joybuttonkey360[j][c], &vid_joybuttontimer[j]);

		// axes
		cl.cmd.forwardmove += VID_JoyState_GetAxis(joystate, joy_x360_axisforward.integer, joy_x360_sensitivityforward.value, joy_x360_deadzoneforward.value) * cl_forwardspeed.value;
		cl.cmd.sidemove    += VID_JoyState_GetAxis(joystate, joy_x360_axisside.integer, joy_x360_sensitivityside.value, joy_x360_deadzoneside.value) * cl_sidespeed.value;
		cl.cmd.upmove      += VID_JoyState_GetAxis(joystate, joy_x360_axisup.integer, joy_x360_sensitivityup.value, joy_x360_deadzoneup.value) * cl_upspeed.value;
		cl.viewangles[0]   += VID_JoyState_GetAxis(joystate, joy_x360_axispitch.integer, joy_x360_sensitivitypitch.value, joy_x360_deadzonepitch.value) * cl.realframetime * cl_pitchspeed.value;
		cl.viewangles[1]   += VID_JoyState_GetAxis(joystate, joy_x360_axisyaw.integer, joy_x360_sensitivityyaw.value, joy_x360_deadzoneyaw.value) * cl.realframetime * cl_yawspeed.value;
		//cl.viewangles[2]   += VID_JoyState_GetAxis(joystate, joy_x360_axisroll.integer, joy_x360_sensitivityroll.value, joy_x360_deadzoneroll.value) * cl.realframetime * cl_rollspeed.value;
	}
	else
	{
		// emit key events for buttons
		for (j = 0;j < MAXJOYBUTTON;j++)
			VID_KeyEventForButton(vid_joystate.button[j] != 0, joystate->button[j] != 0, joybuttonkey[j][c], &vid_joybuttontimer[j]);

		// axes
		cl.cmd.forwardmove += VID_JoyState_GetAxis(joystate, joy_axisforward.integer, joy_sensitivityforward.value, joy_deadzoneforward.value) * cl_forwardspeed.value;
		cl.cmd.sidemove    += VID_JoyState_GetAxis(joystate, joy_axisside.integer, joy_sensitivityside.value, joy_deadzoneside.value) * cl_sidespeed.value;
		cl.cmd.upmove      += VID_JoyState_GetAxis(joystate, joy_axisup.integer, joy_sensitivityup.value, joy_deadzoneup.value) * cl_upspeed.value;
		cl.viewangles[0]   += VID_JoyState_GetAxis(joystate, joy_axispitch.integer, joy_sensitivitypitch.value, joy_deadzonepitch.value) * cl.realframetime * cl_pitchspeed.value;
		cl.viewangles[1]   += VID_JoyState_GetAxis(joystate, joy_axisyaw.integer, joy_sensitivityyaw.value, joy_deadzoneyaw.value) * cl.realframetime * cl_yawspeed.value;
		//cl.viewangles[2]   += VID_JoyState_GetAxis(joystate, joy_axisroll.integer, joy_sensitivityroll.value, joy_deadzoneroll.value) * cl.realframetime * cl_rollspeed.value;
	}

	vid_joystate = *joystate;
}

int VID_Shared_SetJoystick(int index)
{
#ifdef WIN32
	int i;
	int xinputcount = 0;
	int xinputindex = -1;
	int xinputavailable = 0;
	xinput_state_t state;
	// detect available XInput controllers
	for (i = 0;i < 4;i++)
	{
		if (qXInputGetState && qXInputGetState(i, &state) == S_OK)
		{
			xinputavailable |= 1<<i;
			if (index == xinputcount)
				xinputindex = i;
			xinputcount++;
		}
	}
	if (joy_xinputavailable.integer != xinputavailable)
		Cvar_SetValueQuick(&joy_xinputavailable, xinputavailable);
	if (vid_xinputindex != xinputindex)
	{
		vid_xinputindex = xinputindex;
		if (xinputindex >= 0)
			Con_Printf("Joystick %i opened (XInput Device %i)\n", index, xinputindex);
	}
	return xinputcount;
#else
	return 0;
#endif
}


static void Force_CenterView_f(cmd_state_t *cmd)
{
	cl.viewangles[PITCH] = 0;
}

static int gamma_forcenextframe = false;
static float cachegamma, cachebrightness, cachecontrast, cacheblack[3], cachegrey[3], cachewhite[3], cachecontrastboost;
static int cachecolorenable;

void VID_ApplyGammaToColor(const float *rgb, float *out)
{
	int i;
	if (cachecolorenable)
	{
		for (i = 0; i < 3; i++)
			out[i] = pow(cachecontrastboost * rgb[i] / ((cachecontrastboost - 1) * rgb[i] + 1), 1.0 / invpow(0.5, 1 - cachegrey[i])) * cachewhite[i] + cacheblack[i];
	}
	else
	{
		for (i = 0; i < 3; i++)
			out[i] = pow(cachecontrastboost * rgb[i] / ((cachecontrastboost - 1) * rgb[i] + 1), 1.0 / cachegamma) * cachecontrast + cachebrightness;
	}
}

unsigned int vid_gammatables_serial = 0; // so other subsystems can poll if gamma parameters have changed
qbool vid_gammatables_trivial = true;
void VID_BuildGammaTables(unsigned short *ramps, int rampsize)
{
	if (cachecolorenable)
	{
		BuildGammaTable16(1.0f, invpow(0.5, 1 - cachegrey[0]), cachewhite[0], cacheblack[0], cachecontrastboost, ramps, rampsize);
		BuildGammaTable16(1.0f, invpow(0.5, 1 - cachegrey[1]), cachewhite[1], cacheblack[1], cachecontrastboost, ramps + rampsize, rampsize);
		BuildGammaTable16(1.0f, invpow(0.5, 1 - cachegrey[2]), cachewhite[2], cacheblack[2], cachecontrastboost, ramps + rampsize*2, rampsize);
	}
	else
	{
		BuildGammaTable16(1.0f, cachegamma, cachecontrast, cachebrightness, cachecontrastboost, ramps, rampsize);
		BuildGammaTable16(1.0f, cachegamma, cachecontrast, cachebrightness, cachecontrastboost, ramps + rampsize, rampsize);
		BuildGammaTable16(1.0f, cachegamma, cachecontrast, cachebrightness, cachecontrastboost, ramps + rampsize*2, rampsize);
	}

	if(vid.sRGB2D || vid.sRGB3D)
	{
		int i;
		for(i = 0; i < 3*rampsize; ++i)
			ramps[i] = (int)floor(bound(0.0f, Image_sRGBFloatFromLinearFloat(ramps[i] / 65535.0f), 1.0f) * 65535.0f + 0.5f);
	}

	// LadyHavoc: this code came from Ben Winslow and Zinx Verituse, I have
	// immensely butchered it to work with variable framerates and fit in with
	// the rest of darkplaces.
	//
	// R.I.P. zinx http://obits.al.com/obituaries/birmingham/obituary.aspx?n=christopher-robert-lais&pid=186080667
	if (v_psycho.integer)
	{
		int x, y;
		float t;
		static float n[3], nd[3], nt[3];
		static int init = true;
		unsigned short *ramp;
		gamma_forcenextframe = true;
		if (init)
		{
			init = false;
			for (x = 0;x < 3;x++)
			{
				n[x] = lhrandom(0, 1);
				nd[x] = (rand()&1)?-0.25:0.25;
				nt[x] = lhrandom(1, 8.2);
			}
		}

		for (x = 0;x < 3;x++)
		{
			nt[x] -= cl.realframetime;
			if (nt[x] < 0)
			{
				nd[x] = -nd[x];
				nt[x] += lhrandom(1, 8.2);
			}
			n[x] += nd[x] * cl.realframetime;
			n[x] -= floor(n[x]);
		}

		for (x = 0, ramp = ramps;x < 3;x++)
			for (y = 0, t = n[x] - 0.75f;y < rampsize;y++, t += 0.75f * (2.0f / rampsize))
				*ramp++ = (unsigned short)(cos(t*(M_PI*2.0)) * 32767.0f + 32767.0f);
	}
}

void VID_UpdateGamma(void)
{
	cvar_t *c;
	float f;
	qbool gamma_changed = false;

#define BOUNDCVAR(cvar, m1, m2) c = &(cvar);f = bound(m1, c->value, m2);if (c->value != f) Cvar_SetValueQuick(c, f);
	BOUNDCVAR(v_gamma, 0.1, 5);
	BOUNDCVAR(v_contrast, 0.2, 5);
	BOUNDCVAR(v_brightness, -v_contrast.value * 0.8, 0.8);
	//BOUNDCVAR(v_contrastboost, 0.0625, 16);
	BOUNDCVAR(v_color_black_r, 0, 0.8);
	BOUNDCVAR(v_color_black_g, 0, 0.8);
	BOUNDCVAR(v_color_black_b, 0, 0.8);
	BOUNDCVAR(v_color_grey_r, 0, 0.95);
	BOUNDCVAR(v_color_grey_g, 0, 0.95);
	BOUNDCVAR(v_color_grey_b, 0, 0.95);
	BOUNDCVAR(v_color_white_r, 1, 5);
	BOUNDCVAR(v_color_white_g, 1, 5);
	BOUNDCVAR(v_color_white_b, 1, 5);
#undef BOUNDCVAR

	// set vid_gammatables_trivial to true if the current settings would generate the identity gamma table
	vid_gammatables_trivial = false;
	if(v_psycho.integer == 0)
	if(v_contrastboost.value == 1)
	if(!vid.sRGB2D)
	if(!vid.sRGB3D)
	{
		if(v_color_enable.integer)
		{
			if(v_color_black_r.value == 0)
			if(v_color_black_g.value == 0)
			if(v_color_black_b.value == 0)
			if(fabs(v_color_grey_r.value - 0.5) < 1e-6)
			if(fabs(v_color_grey_g.value - 0.5) < 1e-6)
			if(fabs(v_color_grey_b.value - 0.5) < 1e-6)
			if(v_color_white_r.value == 1)
			if(v_color_white_g.value == 1)
			if(v_color_white_b.value == 1)
				vid_gammatables_trivial = true;
		}
		else
		{
			if(v_gamma.value == 1)
			if(v_contrast.value == 1)
			if(v_brightness.value == 0)
				vid_gammatables_trivial = true;
		}
	}

	// if any gamma settings were changed, bump vid_gammatables_serial so we regenerate the gamma ramp texture
#define GAMMACHECK(cache, value) if (cache != (value)) gamma_changed = true;cache = (value)
	if(v_psycho.integer)
		gamma_changed = true;
	GAMMACHECK(cachegamma      , v_gamma.value);
	GAMMACHECK(cachecontrast   , v_contrast.value);
	GAMMACHECK(cachebrightness , v_brightness.value);
	GAMMACHECK(cachecontrastboost, v_contrastboost.value);
	GAMMACHECK(cachecolorenable, v_color_enable.integer);
	GAMMACHECK(cacheblack[0]   , v_color_black_r.value);
	GAMMACHECK(cacheblack[1]   , v_color_black_g.value);
	GAMMACHECK(cacheblack[2]   , v_color_black_b.value);
	GAMMACHECK(cachegrey[0]    , v_color_grey_r.value);
	GAMMACHECK(cachegrey[1]    , v_color_grey_g.value);
	GAMMACHECK(cachegrey[2]    , v_color_grey_b.value);
	GAMMACHECK(cachewhite[0]   , v_color_white_r.value);
	GAMMACHECK(cachewhite[1]   , v_color_white_g.value);
	GAMMACHECK(cachewhite[2]   , v_color_white_b.value);

	if(gamma_changed)
		++vid_gammatables_serial;
#undef GAMMACHECK
}

#ifdef WIN32
static dllfunction_t xinputdllfuncs[] =
{
	{"XInputGetState", (void **) &qXInputGetState},
	{"XInputGetKeystroke", (void **) &qXInputGetKeystroke},
	{NULL, NULL}
};
static const char* xinputdllnames [] =
{
	"xinput1_3.dll",
	"xinput1_2.dll",
	"xinput1_1.dll",
	NULL
};
static dllhandle_t xinputdll_dll = NULL;
#endif

void VID_Shared_Init(void)
{
	Cvar_RegisterVariable(&gl_info_vendor);
	Cvar_RegisterVariable(&gl_info_renderer);
	Cvar_RegisterVariable(&gl_info_version);
	Cvar_RegisterVariable(&gl_info_extensions);
	Cvar_RegisterVariable(&gl_info_driver);
	Cvar_RegisterVariable(&v_gamma);
	Cvar_RegisterVariable(&v_brightness);
	Cvar_RegisterVariable(&v_contrastboost);
	Cvar_RegisterVariable(&v_contrast);

	Cvar_RegisterVariable(&v_color_enable);
	Cvar_RegisterVariable(&v_color_black_r);
	Cvar_RegisterVariable(&v_color_black_g);
	Cvar_RegisterVariable(&v_color_black_b);
	Cvar_RegisterVariable(&v_color_grey_r);
	Cvar_RegisterVariable(&v_color_grey_g);
	Cvar_RegisterVariable(&v_color_grey_b);
	Cvar_RegisterVariable(&v_color_white_r);
	Cvar_RegisterVariable(&v_color_white_g);
	Cvar_RegisterVariable(&v_color_white_b);

	Cvar_RegisterVariable(&v_glslgamma_2d);

	Cvar_RegisterVariable(&v_psycho);

	Cvar_RegisterVariable(&vid_fullscreen);
	Cvar_RegisterVariable(&vid_borderless);
	Cvar_RegisterVariable(&vid_width);
	Cvar_RegisterVariable(&vid_height);
	Cvar_RegisterVariable(&vid_bitsperpixel);
	Cvar_RegisterVariable(&vid_samples);
	Cvar_RegisterVariable(&vid_refreshrate);
	Cvar_RegisterVariable(&vid_stereobuffer);
	Cvar_RegisterVariable(&vid_touchscreen_density);
	Cvar_RegisterVariable(&vid_touchscreen_xdpi);
	Cvar_RegisterVariable(&vid_touchscreen_ydpi);
	Cvar_RegisterVariable(&vid_vsync);
	Cvar_RegisterVariable(&vid_mouse);
	Cvar_RegisterVariable(&vid_mouse_clickthrough);
	Cvar_RegisterVariable(&vid_minimize_on_focus_loss);
	Cvar_RegisterVariable(&vid_grabkeyboard);
	Cvar_RegisterVariable(&vid_touchscreen);
	Cvar_RegisterVariable(&vid_touchscreen_showkeyboard);
	Cvar_RegisterVariable(&vid_touchscreen_supportshowkeyboard);
	Cvar_RegisterVariable(&vid_stick_mouse);
	Cvar_RegisterVariable(&vid_resizable);
	Cvar_RegisterVariable(&vid_desktopfullscreen);
	Cvar_RegisterVariable(&vid_display);
	Cvar_RegisterVariable(&vid_info_displaycount);
#ifdef WIN32
	Cvar_RegisterVariable(&vid_ignore_taskbar);
#endif
	Cvar_RegisterVariable(&vid_minwidth);
	Cvar_RegisterVariable(&vid_minheight);
	Cvar_RegisterVariable(&gl_finish);
	Cvar_RegisterVariable(&vid_sRGB);
	Cvar_RegisterVariable(&vid_sRGB_fallback);

	Cvar_RegisterVariable(&joy_active);
#ifdef WIN32
	Cvar_RegisterVariable(&joy_xinputavailable);
#endif
	Cvar_RegisterVariable(&joy_detected);
	Cvar_RegisterVariable(&joy_enable);
	Cvar_RegisterVariable(&joy_index);
	Cvar_RegisterVariable(&joy_axisforward);
	Cvar_RegisterVariable(&joy_axisside);
	Cvar_RegisterVariable(&joy_axisup);
	Cvar_RegisterVariable(&joy_axispitch);
	Cvar_RegisterVariable(&joy_axisyaw);
	//Cvar_RegisterVariable(&joy_axisroll);
	Cvar_RegisterVariable(&joy_deadzoneforward);
	Cvar_RegisterVariable(&joy_deadzoneside);
	Cvar_RegisterVariable(&joy_deadzoneup);
	Cvar_RegisterVariable(&joy_deadzonepitch);
	Cvar_RegisterVariable(&joy_deadzoneyaw);
	//Cvar_RegisterVariable(&joy_deadzoneroll);
	Cvar_RegisterVariable(&joy_sensitivityforward);
	Cvar_RegisterVariable(&joy_sensitivityside);
	Cvar_RegisterVariable(&joy_sensitivityup);
	Cvar_RegisterVariable(&joy_sensitivitypitch);
	Cvar_RegisterVariable(&joy_sensitivityyaw);
	//Cvar_RegisterVariable(&joy_sensitivityroll);
	Cvar_RegisterVariable(&joy_axiskeyevents);
	Cvar_RegisterVariable(&joy_axiskeyevents_deadzone);
	Cvar_RegisterVariable(&joy_x360_axisforward);
	Cvar_RegisterVariable(&joy_x360_axisside);
	Cvar_RegisterVariable(&joy_x360_axisup);
	Cvar_RegisterVariable(&joy_x360_axispitch);
	Cvar_RegisterVariable(&joy_x360_axisyaw);
	//Cvar_RegisterVariable(&joy_x360_axisroll);
	Cvar_RegisterVariable(&joy_x360_deadzoneforward);
	Cvar_RegisterVariable(&joy_x360_deadzoneside);
	Cvar_RegisterVariable(&joy_x360_deadzoneup);
	Cvar_RegisterVariable(&joy_x360_deadzonepitch);
	Cvar_RegisterVariable(&joy_x360_deadzoneyaw);
	//Cvar_RegisterVariable(&joy_x360_deadzoneroll);
	Cvar_RegisterVariable(&joy_x360_sensitivityforward);
	Cvar_RegisterVariable(&joy_x360_sensitivityside);
	Cvar_RegisterVariable(&joy_x360_sensitivityup);
	Cvar_RegisterVariable(&joy_x360_sensitivitypitch);
	Cvar_RegisterVariable(&joy_x360_sensitivityyaw);
	//Cvar_RegisterVariable(&joy_x360_sensitivityroll);

#ifdef WIN32
	Sys_LoadDependency(xinputdllnames, &xinputdll_dll, xinputdllfuncs);
#endif

	Cmd_AddCommand(CF_CLIENT, "force_centerview", Force_CenterView_f, "recenters view (stops looking up/down)");
	Cmd_AddCommand(CF_CLIENT, "vid_restart", VID_Restart_f, "restarts video system (closes and reopens the window, restarts renderer)");
}

/// NULL mode means read it from the cvars
static int VID_Mode(viddef_mode_t *mode)
{
	char vabuf[1024];
	viddef_mode_t _mode;

	if (!mode)
	{
		mode = &_mode;
		memset(mode, 0, sizeof(*mode));
		mode->display           = vid_display.integer;
		mode->fullscreen        = vid_fullscreen.integer != 0;
		mode->desktopfullscreen = vid_desktopfullscreen.integer != 0;
		mode->width             = vid_width.integer;
		mode->height            = vid_height.integer;
		mode->bitsperpixel      = vid_bitsperpixel.integer;
		mode->refreshrate       = max(0, vid_refreshrate.integer);
		mode->stereobuffer      = vid_stereobuffer.integer != 0;
#ifdef _DIII4A
		extern int screen_width;
		extern int screen_height;
		mode->display           = 0;
		mode->fullscreen        = true;
		mode->desktopfullscreen = true;
		mode->width             = screen_width;
		mode->height            = screen_height;
		mode->bitsperpixel      = 32;
		mode->refreshrate       = 60;
		mode->stereobuffer      = 0;
#endif
	}
	cl_ignoremousemoves = 2;
	VID_ClearExtensions();

	if (VID_InitMode(mode))
	{
		// bones_was_here: we no longer copy the (possibly modified) display mode to `vid` here
		// because complete modesetting failure isn't really what happens with SDL2.
		// Instead we update the active mode when we successfully apply settings,
		// if some can't be applied we still have a viable window.
		// Failure is still possible for other (non- display mode) reasons.
		vid.sRGB2D         = vid_sRGB.integer >= 1 && vid.sRGBcapable2D;
		vid.sRGB3D         = vid_sRGB.integer >= 1 && vid.sRGBcapable3D;

		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
#ifdef GL_STEREO
			{
				GLboolean stereo;
				qglGetBooleanv(GL_STEREO, &stereo);
				vid.mode.stereobuffer = stereo != 0;
			}
#endif
			break;
		case RENDERPATH_GLES2:
		default:
			vid.mode.stereobuffer = false;
			break;
		}

		if(
			(vid_sRGB_fallback.integer >= 3) // force fallback
			||
			(vid_sRGB_fallback.integer >= 2 && // fallback if framebuffer is 8bit
				r_viewfbo.integer < 2)
		)
			vid.sRGB2D = vid.sRGB3D = false;

		Con_Printf("Video Mode: %s%s %dx%d %dbpp%s%s on display %i\n",
			vid.mode.desktopfullscreen ? "desktop " : "",
			vid.mode.fullscreen ? "fullscreen" : "window",
			vid.mode.width, vid.mode.height, vid.mode.bitsperpixel,
			vid.mode.refreshrate ? va(vabuf, sizeof(vabuf), " %.2fhz", vid.mode.refreshrate) : "",
			vid.mode.stereobuffer ? " stereo" : "",
			vid.mode.display);

		if (vid_touchscreen.integer)
		{
			in_windowmouse_x = vid_width.value / 2.f;
			in_windowmouse_y = vid_height.value / 2.f;
		}

		return true;
	}
	else
		return false;
}

qbool vid_commandlinecheck = true;
extern qbool vid_opened;

void VID_Restart_f(cmd_state_t *cmd)
{
	char vabuf[1024];
	viddef_mode_t oldmode;

	// don't crash if video hasn't started yet
	if (vid_commandlinecheck)
		return;

	oldmode = vid.mode;

	Con_Printf("VID_Restart: changing from %s%s %dx%d %dbpp%s%s on display %i, to %s%s %dx%d %dbpp%s%s on display %i.\n",
		oldmode.desktopfullscreen ? "desktop " : "",
		oldmode.fullscreen ? "fullscreen" : "window",
		oldmode.width, oldmode.height, oldmode.bitsperpixel,
		oldmode.refreshrate ? va(vabuf, sizeof(vabuf), " %.2fhz", oldmode.refreshrate) : "",
		oldmode.stereobuffer ? " stereo" : "",
		oldmode.display,
		vid_desktopfullscreen.integer ? "desktop " : "",
		vid_fullscreen.integer ? "fullscreen" : "window",
		vid_width.integer, vid_height.integer, vid_bitsperpixel.integer,
		vid_fullscreen.integer && !vid_desktopfullscreen.integer && vid_refreshrate.integer ? va(vabuf, sizeof(vabuf), " %.2fhz", vid_refreshrate.value) : "",
		vid_stereobuffer.integer ? " stereo" : "",
		vid_display.integer);

	SCR_DeferLoadingPlaque(false);
	R_Modules_Shutdown();
	VID_Shutdown();
	if (!VID_Mode(NULL))
	{
		Con_Print(CON_ERROR "Video mode change failed\n");
		if (!VID_Mode(&oldmode))
			Sys_Error("Unable to restore to last working video mode");
		else
		{
			Cvar_SetValueQuick(&vid_display,           oldmode.display);
			Cvar_SetValueQuick(&vid_fullscreen,        oldmode.fullscreen);
			Cvar_SetValueQuick(&vid_desktopfullscreen, oldmode.desktopfullscreen);
			Cvar_SetValueQuick(&vid_width,             oldmode.width);
			Cvar_SetValueQuick(&vid_height,            oldmode.height);
			Cvar_SetValueQuick(&vid_bitsperpixel,      oldmode.bitsperpixel);
			Cvar_SetValueQuick(&vid_refreshrate,       oldmode.refreshrate);
			Cvar_SetValueQuick(&vid_stereobuffer,      oldmode.stereobuffer);
		}
	}
	R_Modules_Start();
	Key_ReleaseAll();
}

struct vidfallback_s
{
	cvar_t *cvar;
	const char *safevalue;
};
static struct vidfallback_s vidfallbacks[] =
{
	{&vid_stereobuffer, "0"},
	{&vid_samples, "1"},
	{&vid_refreshrate, "0"},
	{&vid_width, "640"},
	{&vid_height, "480"},
	{&vid_bitsperpixel, "32"},
	{NULL, NULL}
};

// this is only called once by CL_StartVideo and again on each FS_GameDir_f
void VID_Start(void)
{
	int i = 0;
	int width, height, success;
	if (vid_commandlinecheck)
	{
		// interpret command-line parameters
		vid_commandlinecheck = false;
// COMMANDLINEOPTION: Video: -window performs +vid_fullscreen 0
		if (Sys_CheckParm("-window") || Sys_CheckParm("-safe") || ((i = Sys_CheckParm("+vid_fullscreen")) != 0 && atoi(sys.argv[i+1]) == 0))
			Cvar_SetValueQuick(&vid_fullscreen, false);
// COMMANDLINEOPTION: Video: -borderless performs +vid_borderless 1
		if (Sys_CheckParm("-borderless") || ((i = Sys_CheckParm("+vid_borderless")) != 0 && atoi(sys.argv[i+1]) == 1))
		{
			Cvar_SetValueQuick(&vid_borderless, true);
			Cvar_SetValueQuick(&vid_fullscreen, false);
		}
// COMMANDLINEOPTION: Video: -fullscreen performs +vid_fullscreen 1
		if (Sys_CheckParm("-fullscreen") || ((i = Sys_CheckParm("+vid_fullscreen")) != 0 && atoi(sys.argv[i+1]) == 1))
			Cvar_SetValueQuick(&vid_fullscreen, true);
		width = 0;
		height = 0;
// COMMANDLINEOPTION: Video: -width <pixels> performs +vid_width <pixels> and also +vid_height <pixels*3/4> if only -width is specified (example: -width 1024 sets 1024x768 mode)
		if ((i = Sys_CheckParm("-width")) != 0 || ((i = Sys_CheckParm("+vid_width")) != 0))
			width = atoi(sys.argv[i+1]);
// COMMANDLINEOPTION: Video: -height <pixels> performs +vid_height <pixels> and also +vid_width <pixels*4/3> if only -height is specified (example: -height 768 sets 1024x768 mode)
		if ((i = Sys_CheckParm("-height")) != 0 || ((i = Sys_CheckParm("+vid_height")) != 0))
			height = atoi(sys.argv[i+1]);
		if (width == 0)
			width = height * 4 / 3;
		if (height == 0)
			height = width * 3 / 4;
		if (width)
			Cvar_SetValueQuick(&vid_width, width);
		if (height)
			Cvar_SetValueQuick(&vid_height, height);
// COMMANDLINEOPTION: Video: -density <multiplier> performs +vid_touchscreen_density <multiplier> (example -density 1 or -density 1.5)
		if ((i = Sys_CheckParm("-density")) != 0)
			Cvar_SetQuick(&vid_touchscreen_density, sys.argv[i+1]);
// COMMANDLINEOPTION: Video: -xdpi <dpi> performs +vid_touchscreen_xdpi <dpi> (example -xdpi 160 or -xdpi 320)
		if ((i = Sys_CheckParm("-touchscreen_xdpi")) != 0)
			Cvar_SetQuick(&vid_touchscreen_xdpi, sys.argv[i+1]);
// COMMANDLINEOPTION: Video: -ydpi <dpi> performs +vid_touchscreen_ydpi <dpi> (example -ydpi 160 or -ydpi 320)
		if ((i = Sys_CheckParm("-touchscreen_ydpi")) != 0)
			Cvar_SetQuick(&vid_touchscreen_ydpi, sys.argv[i+1]);
	}

	success = VID_Mode(NULL);
	if (!success)
	{
		Con_Print(CON_WARN "Desired video mode fail, trying fallbacks...\n");
		for (i = 0; !success && vidfallbacks[i].cvar != NULL; i++)
		{
			Cvar_SetQuick(vidfallbacks[i].cvar, vidfallbacks[i].safevalue);
			success = VID_Mode(NULL);
		}
		if (!success)
			Sys_Error("Video modes failed");
	}

	R_Modules_Start();
	Key_ReleaseAll();
}

static int VID_SortModes_Compare(const void *a_, const void *b_)
{
	vid_mode_t *a = (vid_mode_t *) a_;
	vid_mode_t *b = (vid_mode_t *) b_;
	if(a->width > b->width)
		return +1;
	if(a->width < b->width)
		return -1;
	if(a->height > b->height)
		return +1;
	if(a->height < b->height)
		return -1;
	if(a->refreshrate > b->refreshrate)
		return +1;
	if(a->refreshrate < b->refreshrate)
		return -1;
	if(a->bpp > b->bpp)
		return +1;
	if(a->bpp < b->bpp)
		return -1;
	if(a->pixelheight_num * b->pixelheight_denom > a->pixelheight_denom * b->pixelheight_num)
		return +1;
	if(a->pixelheight_num * b->pixelheight_denom < a->pixelheight_denom * b->pixelheight_num)
		return -1;
	return 0;
}
size_t VID_SortModes(vid_mode_t *modes, size_t count, qbool usebpp, qbool userefreshrate, qbool useaspect)
{
	size_t i;
	if(count == 0)
		return 0;
	// 1. sort them
	qsort(modes, count, sizeof(*modes), VID_SortModes_Compare);
	// 2. remove duplicates
	for(i = 0; i < count; ++i)
	{
		if(modes[i].width && modes[i].height)
		{
			if(i == 0)
				continue;
			if(modes[i].width != modes[i-1].width)
				continue;
			if(modes[i].height != modes[i-1].height)
				continue;
			if(userefreshrate)
				if(modes[i].refreshrate != modes[i-1].refreshrate)
					continue;
			if(usebpp)
				if(modes[i].bpp != modes[i-1].bpp)
					continue;
			if(useaspect)
				if(modes[i].pixelheight_num * modes[i-1].pixelheight_denom != modes[i].pixelheight_denom * modes[i-1].pixelheight_num)
					continue;
		}
		// a dupe, or a bogus mode!
		if(i < count-1)
			memmove(&modes[i], &modes[i+1], sizeof(*modes) * (count-1 - i));
		--i; // check this index again, as mode i+1 is now here
		--count;
	}
	return count;
}
