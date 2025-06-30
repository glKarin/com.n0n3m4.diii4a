#if !defined(INTERFACE_METHOD)
#error "must define INTERFACE_METHOD first"
#endif

/* Java class SDLActivity */
INTERFACE_METHOD(const char *, nativeGetVersion, ())

INTERFACE_METHOD(void, nativeSetupJNI, ())

INTERFACE_METHOD(int, nativeRunMain, (const char * library, const char * function, const char **array))

INTERFACE_METHOD(void, onNativeDropFile, (const char * filename))

INTERFACE_METHOD(void, nativeSetScreenResolution, (int surfaceWidth, int surfaceHeight,
    int deviceWidth, int deviceHeight, float rate))

INTERFACE_METHOD(void, onNativeResize, ())

INTERFACE_METHOD(void, onNativeSurfaceCreated, ())

INTERFACE_METHOD(void, onNativeSurfaceChanged, ())

INTERFACE_METHOD(void, onNativeSurfaceDestroyed, ())

INTERFACE_METHOD(void, onNativeKeyDown, (int keycode))

INTERFACE_METHOD(void, onNativeKeyUp, (int keycode))

INTERFACE_METHOD(int, onNativeSoftReturnKey, ())

INTERFACE_METHOD(void, onNativeKeyboardFocusLost, ())

INTERFACE_METHOD(void, onNativeTouch, (
    int touch_device_id_in, int pointer_finger_id_in,
    int action, float x, float y, float p))

INTERFACE_METHOD(void, onNativeMouse, (
    int button, int action, float x, float y, int relative))

INTERFACE_METHOD(void, onNativeAccel, (
    float x, float y, float z))

INTERFACE_METHOD(void, onNativeClipboardChanged, ())

INTERFACE_METHOD(void, nativeLowMemory, ())

INTERFACE_METHOD(void, onNativeLocaleChanged, ())

INTERFACE_METHOD(void, nativeSendQuit, ())

INTERFACE_METHOD(void, nativeQuit, ())

INTERFACE_METHOD(void, nativePause, ())

INTERFACE_METHOD(void, nativeResume, ())

INTERFACE_METHOD(void, nativeFocusChanged, (int hasFocus))

INTERFACE_METHOD(const char *, nativeGetHint, (const char * name))

INTERFACE_METHOD(int, nativeGetHintBoolean, (const char * name, int default_value))

INTERFACE_METHOD(void, nativeSetenv, (const char * name, const char * value))

INTERFACE_METHOD(void, onNativeOrientationChanged, (int orientation))

INTERFACE_METHOD(void, nativeAddTouch, (int touchId, const char * name))

INTERFACE_METHOD(void, nativePermissionResult, (int requestCode, int result))

/* Java class SDLInputConnection */
INTERFACE_METHOD(void, connection_nativeCommitText, (const char *text, int newCursorPosition))

INTERFACE_METHOD(void, connection_nativeGenerateScancodeForUnichar, (jchar chUnicode))

/* Java class SDLAudioManager */
INTERFACE_METHOD(void, audio_nativeSetupJNI, ())

INTERFACE_METHOD(void,
    audio_addAudioDevice, ( int is_capture,
                                             int device_id))

INTERFACE_METHOD(void,
    audio_removeAudioDevice, ( int is_capture,
                                                int device_id))

/* Java class SDLControllerManager */
INTERFACE_METHOD(void, controller_nativeSetupJNI, ())

INTERFACE_METHOD(int, controller_onNativePadDown, (int device_id, int keycode))

INTERFACE_METHOD(int, controller_onNativePadUp, (
    int device_id, int keycode))

INTERFACE_METHOD(void, controller_onNativeJoy, (
    int device_id, int axis, float value))

INTERFACE_METHOD(void, controller_onNativeHat, (
    int device_id, int hat_id, int x, int y))

INTERFACE_METHOD(int, controller_nativeAddJoystick, (
    int device_id, const char * device_name, const char * device_desc, int vendor_id, int product_id,
    int is_accelerometer, int button_mask, int naxes, int axis_mask, int nhats, int nballs))

INTERFACE_METHOD(int, controller_nativeRemoveJoystick, (
    int device_id))

INTERFACE_METHOD(int, controller_nativeAddHaptic, (
    int device_id, const char * device_name))

INTERFACE_METHOD(int, controller_nativeRemoveHaptic, (
    int device_id))

#undef INTERFACE_METHOD

