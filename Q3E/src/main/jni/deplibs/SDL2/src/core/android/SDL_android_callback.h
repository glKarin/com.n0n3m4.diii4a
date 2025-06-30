#if !defined(CALLBACK_METHOD)
#error "must define CALLBACK_METHOD first"
#endif

CALLBACK_METHOD(const char *, clipboardGetText, ());
CALLBACK_METHOD(int, clipboardHasText, ());
CALLBACK_METHOD(void, clipboardSetText, (const char *));
CALLBACK_METHOD(int, createCustomCursor, (int *, int, int, int, int));
CALLBACK_METHOD(void, destroyCustomCursor, (int));
CALLBACK_METHOD(void *, getContext, ());
CALLBACK_METHOD(void *, getDisplayDPI, ());
CALLBACK_METHOD(void *, getNativeSurface, ());
CALLBACK_METHOD(void, initTouch, ());
CALLBACK_METHOD(int, isAndroidTV, ());
CALLBACK_METHOD(int, isChromebook, ());
CALLBACK_METHOD(int, isDeXMode, ());
CALLBACK_METHOD(int, isScreenKeyboardShown, ());
CALLBACK_METHOD(int, isTablet, ());
CALLBACK_METHOD(void, manualBackButton, ());
CALLBACK_METHOD(void, minimizeWindow, ());
CALLBACK_METHOD(int, openURL, (const char *));
CALLBACK_METHOD(void, requestPermission, (const char *, int));
CALLBACK_METHOD(int, showToast, (const char *, int, int, int, int));
CALLBACK_METHOD(int, sendMessage, (int, int));
CALLBACK_METHOD(int, setActivityTitle, (const char *));
CALLBACK_METHOD(int, setCustomCursor, (int));
CALLBACK_METHOD(void, setWindowStyle, (int));
CALLBACK_METHOD(int, shouldMinimizeOnFocusLoss, ());
CALLBACK_METHOD(int, showTextInput, (int, int, int, int));
CALLBACK_METHOD(int, supportsRelativeMouse, ());
CALLBACK_METHOD(int, setSystemCursor, (int));
CALLBACK_METHOD(void, setOrientation, (int, int, int, const char *));
CALLBACK_METHOD(int, setRelativeMouseEnabled, (int));
CALLBACK_METHOD(int, getManifestEnvironmentVariables, ());
CALLBACK_METHOD(int, messageboxShowMessageBox, (int, const char *, const char *, int *, int *,  const char *[], int *));

CALLBACK_METHOD(int *, getAudioOutputDevices, ());
CALLBACK_METHOD(int *, getAudioInputDevices, ());
CALLBACK_METHOD(int *, audioOpen, (int, int, int, int, int));
CALLBACK_METHOD(void, audioWriteByteBuffer, (char *));
CALLBACK_METHOD(void, audioWriteShortBuffer, (short *));
CALLBACK_METHOD(void, audioWriteFloatBuffer, (float *));
CALLBACK_METHOD(void, audioClose, ());
CALLBACK_METHOD(int *, captureOpen, (int, int, int, int, int));
CALLBACK_METHOD(int, captureReadByteBuffer, (char *, int));
CALLBACK_METHOD(int, captureReadShortBuffer, (short *, int));
CALLBACK_METHOD(int, captureReadFloatBuffer, (float *, int));
CALLBACK_METHOD(void, captureClose, ());
CALLBACK_METHOD(void, audioSetThreadPriority, (int, int));

CALLBACK_METHOD(void, pollInputDevices, ());
CALLBACK_METHOD(void, pollHapticDevices, ());
CALLBACK_METHOD(void, hapticRun, (int, float, int));
CALLBACK_METHOD(void, hapticStop, (int));

CALLBACK_METHOD(void, attachCurrentThread, ());
CALLBACK_METHOD(void, detachCurrentThread, ());

#undef CALLBACK_METHOD
