package com.n0n3m4.q3e.sdl;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.UiModeManager;
import android.content.ClipboardManager;
import android.content.ClipData;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.hardware.Sensor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.SparseArray;
import android.view.Display;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.PointerIcon;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EMain;
import com.n0n3m4.q3e.Q3EView;

import java.util.Hashtable;
import java.util.Locale;


/**
    SDL Activity
*/
public class SDLActivity {
    private static final String TAG = "SDL";
    private static final int SDL_MAJOR_VERSION = 2;
    private static final int SDL_MINOR_VERSION = 32;
    private static final int SDL_MICRO_VERSION = 5;

    public static boolean mIsResumedCalled, mHasFocus;
    public static final boolean mHasMultiWindow = (Build.VERSION.SDK_INT >= 24  /* Android 7.0 (N) */);

    protected static final int SDL_ORIENTATION_UNKNOWN = 0;
    protected static final int SDL_ORIENTATION_LANDSCAPE = 1;
    protected static final int SDL_ORIENTATION_LANDSCAPE_FLIPPED = 2;
    protected static final int SDL_ORIENTATION_PORTRAIT = 3;
    protected static final int SDL_ORIENTATION_PORTRAIT_FLIPPED = 4;

    protected static int mCurrentOrientation;
    protected static Locale mCurrentLocale;

    // Handle the state of the native layer
    public enum NativeState {
           INIT, RESUMED, PAUSED
    }

    public static NativeState mNextNativeState;
    public static NativeState mCurrentNativeState;

    /** If shared libraries (e.g. SDL or the native application) could not be loaded. */
    public static boolean mBrokenLibraries = true;

    // Main components
    protected static Q3EMain mSingleton;
    protected static Q3EView mSurface;
    protected static boolean mScreenKeyboardShown;
    protected static ViewGroup mLayout;
    protected static SDLGenericMotionListener_API12 mMotionListener;

    // This is what SDL runs in. It invokes SDL_main(), eventually
    protected static Thread mSDLThread;

    protected static SDLGenericMotionListener_API12 getMotionListener() {
        if (mMotionListener == null) {
            if (Build.VERSION.SDK_INT >= 26 /* Android 8.0 (O) */) {
                mMotionListener = new SDLGenericMotionListener_API26();
            } else if (Build.VERSION.SDK_INT >= 24 /* Android 7.0 (N) */) {
                mMotionListener = new SDLGenericMotionListener_API24();
            } else {
                mMotionListener = new SDLGenericMotionListener_API12();
            }
        }

        return mMotionListener;
    }

    public static void initialize() {
        // The static nature of the singleton and Android quirkyness force us to initialize everything here
        // Otherwise, when exiting the app and returning to it, these variables *keep* their pre exit values
        mSingleton = null;
        mSurface = null;
        mLayout = null;
        mSDLThread = null;
        mIsResumedCalled = false;
        mHasFocus = true;
        mNextNativeState = NativeState.INIT;
        mCurrentNativeState = NativeState.INIT;
    }

    // Setup
    public void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "Device: " + Build.DEVICE);
        Log.v(TAG, "Model: " + Build.MODEL);
        Log.v(TAG, "onCreate()");

        // Set up JNI
        SDL.setupJNI();

        // Initialize state
        SDL.initialize();

        // So we can call stuff from static callbacks
        mSingleton = Q3E.activity;
        SDL.setContext(mSingleton);

        // Set up the surface
        mSurface = Q3E.gameView;

        mLayout = mSingleton.GetMainLayout();

        // Get our current screen orientation and pass it down.
        mCurrentOrientation = SDLActivity.getCurrentOrientation();
        // Only record current orientation
        //onNativeOrientationChanged(mCurrentOrientation);

        try {
            if (Build.VERSION.SDK_INT < 24 /* Android 7.0 (N) */) {
                mCurrentLocale = getContext().getResources().getConfiguration().locale;
            } else {
                mCurrentLocale = getContext().getResources().getConfiguration().getLocales().get(0);
            }
        } catch(Exception ignored) {
        }
    }

    protected void pauseNativeThread() {
        mNextNativeState = NativeState.PAUSED;
        mIsResumedCalled = false;

        SDLActivity.handleNativeState();
    }

    protected void resumeNativeThread() {
        mNextNativeState = NativeState.RESUMED;
        mIsResumedCalled = true;

        SDLActivity.handleNativeState();
    }

    // Events
    public void onPause() {
        Log.v(TAG, "onPause()");

        if (!mHasMultiWindow) {
            pauseNativeThread();
        }
    }

    public void onResume() {
        Log.v(TAG, "onResume()");

        if (!mHasMultiWindow) {
            resumeNativeThread();
        }
    }

    public void onStop() {
        Log.v(TAG, "onStop()");
        if (mHasMultiWindow) {
            pauseNativeThread();
        }
    }

    public void onStart() {
        Log.v(TAG, "onStart()");
        if (mHasMultiWindow) {
            resumeNativeThread();
        }
    }

    public static int getCurrentOrientation() {
        int result = SDL_ORIENTATION_UNKNOWN;

        Activity activity = (Activity)getContext();
        if (activity == null) {
            return result;
        }
        Display display = activity.getWindowManager().getDefaultDisplay();

        switch (display.getRotation()) {
            case Surface.ROTATION_0:
                result = SDL_ORIENTATION_PORTRAIT;
                break;

            case Surface.ROTATION_90:
                result = SDL_ORIENTATION_LANDSCAPE;
                break;

            case Surface.ROTATION_180:
                result = SDL_ORIENTATION_PORTRAIT_FLIPPED;
                break;

            case Surface.ROTATION_270:
                result = SDL_ORIENTATION_LANDSCAPE_FLIPPED;
                break;
        }

        return result;
    }

    public void onWindowFocusChanged(boolean hasFocus) {
        Log.v(TAG, "onWindowFocusChanged(): " + hasFocus);

        mHasFocus = hasFocus;
        if (hasFocus) {
           mNextNativeState = NativeState.RESUMED;
           SDLActivity.getMotionListener().reclaimRelativeMouseModeIfNeeded();

           SDLActivity.handleNativeState();
           //nativeFocusChanged(true);

        } else {
           //nativeFocusChanged(false);
           if (!mHasMultiWindow) {
               mNextNativeState = NativeState.PAUSED;
               SDLActivity.handleNativeState();
           }
        }
    }

    public void onDestroy() {
        Log.v(TAG, "onDestroy()");

        if (SDLActivity.mSDLThread != null) {

            // Send Quit event to "SDLThread" thread
            //SDLActivity.nativeSendQuit();
        }

        //SDLActivity.nativeQuit();
    }

    /* Transition to next state */
    public static void handleNativeState() {

        if (mNextNativeState == mCurrentNativeState) {
            // Already in same state, discard.
            return;
        }

        // Try a transition to init state
        if (mNextNativeState == NativeState.INIT) {

            mCurrentNativeState = mNextNativeState;
            return;
        }

        // Try a transition to paused state
        if (mNextNativeState == NativeState.PAUSED) {
            if (mSDLThread != null) {
                //nativePause();
            }
            if (mSurface != null) {
                //mSurface.handlePause();
            }
            mCurrentNativeState = mNextNativeState;
            return;
        }

        // Try a transition to resumed state
        if (mNextNativeState == NativeState.RESUMED) {
            if (/*mSurface.mInit &&*/ mHasFocus && mIsResumedCalled) {
                //nativeResume();
                //mSurface.handleResume();

                mCurrentNativeState = mNextNativeState;
            }
        }
    }

    // C functions we call
    public static int nativeSetupJNI() { return Q3ESDL.nativeSetupJNI(); }
    public static void onNativeKeyDown(int keycode) { Q3ESDL.onNativeKeyDown(keycode); }
    public static void onNativeKeyUp(int keycode) { Q3ESDL.onNativeKeyUp(keycode); }
    //public static void onNativeOrientationChanged(int orientation) { Q3ESDL.onNativeOrientationChanged(orientation); }
//    public static native void nativeSendQuit();
//    public static native void nativeQuit();
//    public static native void nativePause();
//    public static native void nativeResume();
//    public static native void nativeFocusChanged(boolean hasFocus);
//    public static native void onNativeDropFile(String filename);
//    public static native void nativeSetScreenResolution(int surfaceWidth, int surfaceHeight, int deviceWidth, int deviceHeight, float rate);
//    public static native void onNativeResize();
//    public static native boolean onNativeSoftReturnKey();
//    public static native void onNativeKeyboardFocusLost();
    public static void onNativeMouse(int button, int action, float x, float y, boolean relative) { Q3ESDL.onNativeMouse(button, action, x, y, relative); }
//    public static native void onNativeTouch(int touchDevId, int pointerFingerId,
//                                            int action, float x,
//                                            float y, float p);
//    public static native void onNativeAccel(float x, float y, float z);
//    public static native void onNativeClipboardChanged();
//    public static native void onNativeSurfaceCreated();
//    public static native void onNativeSurfaceChanged();
//    public static native void onNativeSurfaceDestroyed();
//    public static native String nativeGetHint(String name);
//    public static native boolean nativeGetHintBoolean(String name, boolean default_value);
//    public static native void nativeSetenv(String name, String value);
    public static void nativeAddTouch(int touchId, String name) { Q3ESDL.nativeAddTouch(touchId, name); }
//    public static native void nativePermissionResult(int requestCode, boolean result);
//    public static native void onNativeLocaleChanged();

    /**
     * This method is called by SDL using JNI.
     */
    public static void minimizeWindow() {

        if (mSingleton == null) {
            return;
        }

        Intent startMain = new Intent(Intent.ACTION_MAIN);
        startMain.addCategory(Intent.CATEGORY_HOME);
        startMain.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mSingleton.startActivity(startMain);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean shouldMinimizeOnFocusLoss() {
        return false;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isScreenKeyboardShown()
    {
        if (!mScreenKeyboardShown) {
            return false;
        }

        InputMethodManager imm = (InputMethodManager) mSingleton.getSystemService(Context.INPUT_METHOD_SERVICE);
        return imm.isAcceptingText();

    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean supportsRelativeMouse()
    {
        // DeX mode in Samsung Experience 9.0 and earlier doesn't support relative mice properly under
        // Android 7 APIs, and simply returns no data under Android 8 APIs.
        //
        // This is fixed in Samsung Experience 9.5, which corresponds to Android 8.1.0, and
        // thus SDK version 27.  If we are in DeX mode and not API 27 or higher, as a result,
        // we should stick to relative mode.
        //
        if (Build.VERSION.SDK_INT < 27 /* Android 8.1 (O_MR1) */ && isDeXMode()) {
            return false;
        }

        return SDLActivity.getMotionListener().supportsRelativeMouse();
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean setRelativeMouseEnabled(boolean enabled)
    {
        if (enabled && !supportsRelativeMouse()) {
            return false;
        }

        return SDLActivity.getMotionListener().setRelativeMouseEnabled(enabled);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static Context getContext() {
        return SDL.getContext();
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isAndroidTV() {
        UiModeManager uiModeManager = (UiModeManager) getContext().getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
            return true;
        }
        if (Build.MANUFACTURER.equals("MINIX") && Build.MODEL.equals("NEO-U1")) {
            return true;
        }
        if (Build.MANUFACTURER.equals("Amlogic") && Build.MODEL.equals("X96-W")) {
            return true;
        }
        return Build.MANUFACTURER.equals("Amlogic") && Build.MODEL.startsWith("TV");
    }

    public static double getDiagonal()
    {
        DisplayMetrics metrics = new DisplayMetrics();
        Activity activity = (Activity)getContext();
        if (activity == null) {
            return 0.0;
        }
        activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        double dWidthInches = metrics.widthPixels / (double)metrics.xdpi;
        double dHeightInches = metrics.heightPixels / (double)metrics.ydpi;

        return Math.sqrt((dWidthInches * dWidthInches) + (dHeightInches * dHeightInches));
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isTablet() {
        // If our diagonal size is seven inches or greater, we consider ourselves a tablet.
        return (getDiagonal() >= 7.0);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isChromebook() {
        if (getContext() == null) {
            return false;
        }
        return getContext().getPackageManager().hasSystemFeature("org.chromium.arc.device_management");
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isDeXMode() {
        if (Build.VERSION.SDK_INT < 24 /* Android 7.0 (N) */) {
            return false;
        }
        try {
            final Configuration config = getContext().getResources().getConfiguration();
            final Class<?> configClass = config.getClass();
            return configClass.getField("SEM_DESKTOP_MODE_ENABLED").getInt(configClass)
                    == configClass.getField("semDesktopModeEnabled").getInt(config);
        } catch(Exception ignored) {
            return false;
        }
    }

    // This method is called by SDLControllerManager's API 26 Generic Motion Handler.
    public static View getContentView() {
        return mLayout;
    }

    public static boolean isTextInputEvent(KeyEvent event) {

        // Key pressed with Ctrl should be sent as SDL_KEYDOWN/SDL_KEYUP and not SDL_TEXTINPUT
        if (event.isCtrlPressed()) {
            return false;
        }

        return event.isPrintingKey() || event.getKeyCode() == KeyEvent.KEYCODE_SPACE;
    }

    public static boolean handleKeyEvent(View v, int keyCode, KeyEvent event, InputConnection ic, int unicodeChar) {
        int deviceId = event.getDeviceId();
        int source = event.getSource();

        if (source == InputDevice.SOURCE_UNKNOWN) {
            InputDevice device = InputDevice.getDevice(deviceId);
            if (device != null) {
                source = device.getSources();
            }
        }

        // Dispatch the different events depending on where they come from
        // Some SOURCE_JOYSTICK, SOURCE_DPAD or SOURCE_GAMEPAD are also SOURCE_KEYBOARD
        // So, we try to process them as JOYSTICK/DPAD/GAMEPAD events first, if that fails we try them as KEYBOARD
        //
        // Furthermore, it's possible a game controller has SOURCE_KEYBOARD and
        // SOURCE_JOYSTICK, while its key events arrive from the keyboard source
        // So, retrieve the device itself and check all of its sources
        if (SDLControllerManager.isDeviceSDLJoystick(deviceId)) {
            // Note that we process events with specific key codes here
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                if (SDLControllerManager.onNativePadDown(deviceId, keyCode) == 0) {
                    return true;
                }
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                if (SDLControllerManager.onNativePadUp(deviceId, keyCode) == 0) {
                    return true;
                }
            }
        }

        if ((source & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE) {
            // on some devices key events are sent for mouse BUTTON_BACK/FORWARD presses
            // they are ignored here because sending them as mouse input to SDL is messy
            if ((keyCode == KeyEvent.KEYCODE_BACK) || (keyCode == KeyEvent.KEYCODE_FORWARD)) {
                switch (event.getAction()) {
                case KeyEvent.ACTION_DOWN:
                case KeyEvent.ACTION_UP:
                    // mark the event as handled or it will be handled by system
                    // handling KEYCODE_BACK by system will call onBackPressed()
                    return true;
                }
            }
        }

        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            if (isTextInputEvent(event)) {
                if (ic != null) {
                    ic.commitText(String.valueOf((char) event.getUnicodeChar()), 1);
                } else {
                    SDLInputConnection.nativeCommitText(String.valueOf((char) unicodeChar), 1);
                }
            }
            onNativeKeyDown(keyCode);
            return true;
        } else if (event.getAction() == KeyEvent.ACTION_UP) {
            onNativeKeyUp(keyCode);
            return true;
        }

        return false;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static Surface getNativeSurface() {
        return Q3E.surface;
    }

    // Input

    /**
     * This method is called by SDL using JNI.
     */
    public static void initTouch() {
        int[] ids = InputDevice.getDeviceIds();

        for (int id : ids) {
            InputDevice device = InputDevice.getDevice(id);
            /* Allow SOURCE_TOUCHSCREEN and also Virtual InputDevices because they can send TOUCHSCREEN events */
            if (device != null && ((device.getSources() & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN
                    || device.isVirtual())) {

                int touchDevId = device.getId();
                /*
                 * Prevent id to be -1, since it's used in SDL internal for synthetic events
                 * Appears when using Android emulator, eg:
                 *  adb shell input mouse tap 100 100
                 *  adb shell input touchscreen tap 100 100
                 */
                if (touchDevId < 0) {
                    touchDevId -= 1;
                }
                nativeAddTouch(touchDevId, device.getName());
            }
        }
    }
}

class SDLInputConnection {
    public static void nativeCommitText(String text, int newCursorPosition) { Q3ESDL.nativeCommitText(text, newCursorPosition); }
}

