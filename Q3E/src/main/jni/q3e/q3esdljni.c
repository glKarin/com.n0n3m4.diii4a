#ifdef _Q3E_SDL

#include "q3esdljni.h"

#define CALL_SDL_JAVA_INTERFACE(function) function
#define CALL_SDL_JAVA_AUDIO_INTERFACE(function) audio_##function
#define CALL_SDL_JAVA_CONTROLLER_INTERFACE(function) controller_##function
#define CALL_SDL_JAVA_INTERFACE_INPUT_CONNECTION(function) connection_##function

/* Paddown */
JNIEXPORT jint JNICALL SDL_JAVA_CONTROLLER_INTERFACE(onNativePadDown)(
    JNIEnv *env, jclass jcls,
    jint device_id, jint keycode)
{
    return CALLRET_SDL(CALL_SDL_JAVA_CONTROLLER_INTERFACE(onNativePadDown), 0, device_id, keycode);
}

/* Padup */
JNIEXPORT jint JNICALL SDL_JAVA_CONTROLLER_INTERFACE(onNativePadUp)(
    JNIEnv *env, jclass jcls,
    jint device_id, jint keycode)
{
    return CALLRET_SDL(CALL_SDL_JAVA_CONTROLLER_INTERFACE(onNativePadUp), 0, device_id, keycode);
}

/* Joy */
JNIEXPORT void JNICALL SDL_JAVA_CONTROLLER_INTERFACE(onNativeJoy)(
    JNIEnv *env, jclass jcls,
    jint device_id, jint axis, jfloat value)
{
    CALL_SDL(CALL_SDL_JAVA_CONTROLLER_INTERFACE(onNativeJoy), device_id, axis, value);
}

/* POV Hat */
JNIEXPORT void JNICALL SDL_JAVA_CONTROLLER_INTERFACE(onNativeHat)(
    JNIEnv *env, jclass jcls,
    jint device_id, jint hat_id, jint x, jint y)
{
    CALL_SDL(CALL_SDL_JAVA_CONTROLLER_INTERFACE(onNativeHat), device_id, hat_id, x, y);
}

JNIEXPORT jint JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeAddJoystick)(
    JNIEnv *env, jclass jcls,
    jint device_id, jstring device_name, jstring device_desc,
    jint vendor_id, jint product_id, jboolean is_accelerometer,
    jint button_mask, jint naxes, jint axis_mask, jint nhats, jint nballs)
{
    int retval;
    const char *name = (*env)->GetStringUTFChars(env, device_name, NULL);
    const char *desc = (*env)->GetStringUTFChars(env, device_desc, NULL);

    retval = CALLRET_SDL(CALL_SDL_JAVA_CONTROLLER_INTERFACE(nativeAddJoystick), 0, device_id, name, desc, vendor_id, product_id, is_accelerometer ? SDL_TRUE : SDL_FALSE, button_mask, naxes, axis_mask, nhats, nballs);

    (*env)->ReleaseStringUTFChars(env, device_name, name);
    (*env)->ReleaseStringUTFChars(env, device_desc, desc);

    return retval;
}

JNIEXPORT jint JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeRemoveJoystick)(
    JNIEnv *env, jclass jcls,
    jint device_id)
{
    return CALLRET_SDL(CALL_SDL_JAVA_CONTROLLER_INTERFACE(nativeRemoveJoystick), 0, device_id);
}

JNIEXPORT jint JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeAddHaptic)(
    JNIEnv *env, jclass jcls, jint device_id, jstring device_name)
{
    int retval;
    const char *name = (*env)->GetStringUTFChars(env, device_name, NULL);

    retval = CALLRET_SDL(CALL_SDL_JAVA_CONTROLLER_INTERFACE(nativeAddHaptic), 0, device_id, name);

    (*env)->ReleaseStringUTFChars(env, device_name, name);

    return retval;
}

JNIEXPORT jint JNICALL SDL_JAVA_CONTROLLER_INTERFACE(nativeRemoveHaptic)(
    JNIEnv *env, jclass jcls, jint device_id)
{
    return CALLRET_SDL(CALL_SDL_JAVA_CONTROLLER_INTERFACE(nativeRemoveHaptic), 0, device_id);
}



JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(nativeAddTouch)(
    JNIEnv *env, jclass cls,
    jint touchId, jstring name)
{
    const char *utfname = (*env)->GetStringUTFChars(env, name, NULL);

    CALL_SDL(nativeAddTouch, (SDL_TouchID)touchId, utfname);

    (*env)->ReleaseStringUTFChars(env, name, utfname);
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeKeyDown)(JNIEnv *env, jclass cls, jint keycode)
{
    CALL_SDL(onNativeKeyDown, keycode);
}

JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeKeyUp)(JNIEnv *env, jclass cls, jint keycode)
{
    CALL_SDL(onNativeKeyUp, keycode);
}

/* Mouse */
JNIEXPORT void JNICALL SDL_JAVA_INTERFACE(onNativeMouse)(
    JNIEnv *env, jclass jcls,
    jint button, jint action, jfloat x, jfloat y, jboolean relative)
{
    CALL_SDL(onNativeMouse, button, action, x, y, relative);
}



JNIEXPORT void JNICALL SDL_JAVA_INTERFACE_INPUT_CONNECTION(nativeCommitText)(
    JNIEnv *env, jclass cls,
    jstring text, jint newCursorPosition)
{
    const char *utftext = (*env)->GetStringUTFChars(env, text, NULL);

    CALL_SDL(CALL_SDL_JAVA_INTERFACE_INPUT_CONNECTION(nativeCommitText), utftext, newCursorPosition);

    (*env)->ReleaseStringUTFChars(env, text, utftext);
}



JNIEXPORT jboolean JNICALL Java_com_n0n3m4_q3e_sdl_Q3ESDL_UsingSDL(JNIEnv *env, jclass cls)
{
    return USING_SDL ? JNI_TRUE : JNI_FALSE;
}

#if 0
static void register_methods(JNIEnv *env, const char *classname, JNINativeMethod *methods, int nb)
{
    jclass clazz = (*env)->FindClass(env, classname);
    if (!clazz || (*env)->RegisterNatives(env, clazz, methods, nb) < 0) {
        LOGE("Failed to register methods of %s", classname);
        return;
    }
}

/* Library init */
void SDL_Register(JNIEnv *env)
{
    LOGI("Register SDL native methods");

    JNINativeMethod SDLActivity_tab[] = {
        //{ "nativeGetVersion", "()Ljava/lang/String;", SDL_JAVA_INTERFACE(nativeGetVersion) },
        { "nativeSetupJNI", "()I", SDL_JAVA_INTERFACE(nativeSetupJNI) },
        //{ "nativeRunMain", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;)I", SDL_JAVA_INTERFACE(nativeRunMain) },
        //{ "onNativeDropFile", "(Ljava/lang/String;)V", SDL_JAVA_INTERFACE(onNativeDropFile) },
        //{ "nativeSetScreenResolution", "(IIIIF)V", SDL_JAVA_INTERFACE(nativeSetScreenResolution) },
        //{ "onNativeResize", "()V", SDL_JAVA_INTERFACE(onNativeResize) },
        //{ "onNativeSurfaceCreated", "()V", SDL_JAVA_INTERFACE(onNativeSurfaceCreated) },
        //{ "onNativeSurfaceChanged", "()V", SDL_JAVA_INTERFACE(onNativeSurfaceChanged) },
        //{ "onNativeSurfaceDestroyed", "()V", SDL_JAVA_INTERFACE(onNativeSurfaceDestroyed) },
        { "onNativeKeyDown", "(I)V", SDL_JAVA_INTERFACE(onNativeKeyDown) },
        { "onNativeKeyUp", "(I)V", SDL_JAVA_INTERFACE(onNativeKeyUp) },
        //{ "onNativeSoftReturnKey", "()Z", SDL_JAVA_INTERFACE(onNativeSoftReturnKey) },
        //{ "onNativeKeyboardFocusLost", "()V", SDL_JAVA_INTERFACE(onNativeKeyboardFocusLost) },
        //{ "onNativeTouch", "(IIIFFF)V", SDL_JAVA_INTERFACE(onNativeTouch) },
        { "onNativeMouse", "(IIFFZ)V", SDL_JAVA_INTERFACE(onNativeMouse) },
        //{ "onNativeAccel", "(FFF)V", SDL_JAVA_INTERFACE(onNativeAccel) },
        //{ "onNativeClipboardChanged", "()V", SDL_JAVA_INTERFACE(onNativeClipboardChanged) },
        //{ "nativeLowMemory", "()V", SDL_JAVA_INTERFACE(nativeLowMemory) },
        //{ "onNativeLocaleChanged", "()V", SDL_JAVA_INTERFACE(onNativeLocaleChanged) },
        //{ "nativeSendQuit", "()V", SDL_JAVA_INTERFACE(nativeSendQuit) },
        //{ "nativeQuit", "()V", SDL_JAVA_INTERFACE(nativeQuit) },
        //{ "nativePause", "()V", SDL_JAVA_INTERFACE(nativePause) },
        //{ "nativeResume", "()V", SDL_JAVA_INTERFACE(nativeResume) },
        //{ "nativeFocusChanged", "(Z)V", SDL_JAVA_INTERFACE(nativeFocusChanged) },
        //{ "nativeGetHint", "(Ljava/lang/String;)Ljava/lang/String;", SDL_JAVA_INTERFACE(nativeGetHint) },
        //{ "nativeGetHintBoolean", "(Ljava/lang/String;Z)Z", SDL_JAVA_INTERFACE(nativeGetHintBoolean) },
        //{ "nativeSetenv", "(Ljava/lang/String;Ljava/lang/String;)V", SDL_JAVA_INTERFACE(nativeSetenv) },
        //{ "onNativeOrientationChanged", "(I)V", SDL_JAVA_INTERFACE(onNativeOrientationChanged) },
        //{ "nativeAddTouch", "(ILjava/lang/String;)V", SDL_JAVA_INTERFACE(nativeAddTouch) },
        //{ "nativePermissionResult", "(IZ)V", SDL_JAVA_INTERFACE(nativePermissionResult) }
    };

    JNINativeMethod SDLControllerManager_tab[] = {
        { "controller_nativeSetupJNI", "()I", SDL_JAVA_CONTROLLER_INTERFACE(controller_nativeSetupJNI) },
        { "onNativePadDown", "(II)I", SDL_JAVA_CONTROLLER_INTERFACE(onNativePadDown) },
        { "onNativePadUp", "(II)I", SDL_JAVA_CONTROLLER_INTERFACE(onNativePadUp) },
        { "onNativeJoy", "(IIF)V", SDL_JAVA_CONTROLLER_INTERFACE(onNativeJoy) },
        { "onNativeHat", "(IIII)V", SDL_JAVA_CONTROLLER_INTERFACE(onNativeHat) },
        { "nativeAddJoystick", "(ILjava/lang/String;Ljava/lang/String;IIZIIIII)I", SDL_JAVA_CONTROLLER_INTERFACE(nativeAddJoystick) },
        { "nativeRemoveJoystick", "(I)I", SDL_JAVA_CONTROLLER_INTERFACE(nativeRemoveJoystick) },
        { "nativeAddHaptic", "(ILjava/lang/String;)I", SDL_JAVA_CONTROLLER_INTERFACE(nativeAddHaptic) },
        { "nativeRemoveHaptic", "(I)I", SDL_JAVA_CONTROLLER_INTERFACE(nativeRemoveHaptic) }
    };

    JNINativeMethod SDLInputConnection_tab[] = {
        { "nativeCommitText", "(Ljava/lang/String;I)V", SDL_JAVA_INTERFACE_INPUT_CONNECTION(nativeCommitText) },
        // { "nativeGenerateScancodeForUnichar", "(C)V", SDL_JAVA_INTERFACE_INPUT_CONNECTION(nativeGenerateScancodeForUnichar) }
    };

#define Q3ESDL_CLASS "com/n0n3m4/q3e/sdl/Q3ESDL"
    register_methods(env, Q3ESDL_CLASS, SDLActivity_tab, sizeof(SDLActivity_tab) / sizeof(SDLActivity_tab[0]));
    register_methods(env, Q3ESDL_CLASS, SDLInputConnection_tab, sizeof(SDLInputConnection_tab) / sizeof(SDLInputConnection_tab[0]));
    //register_methods(env, Q3ESDL_CLASS, SDLAudioManager_tab, sizeof(SDLAudioManager_tab) / sizeof(SDLAudioManager_tab[0]));
    register_methods(env, Q3ESDL_CLASS, SDLControllerManager_tab, sizeof(SDLControllerManager_tab) / sizeof(SDLControllerManager_tab[0]));
}
#endif

#endif