package com.n0n3m4.q3e;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public final class Q3EGlobals
{
    public static final String CONST_PACKAGE_NAME = "com.karin.idTech4Amm";
    public static final String CONST_APP_NAME     = "idTech4A++"; // "DIII4A++";

    // log tag
    public static final String CONST_Q3E_LOG_TAG = "Q3E";

    // on-screen buttons index
    public static final int UI_JOYSTICK     = 0;
    public static final int UI_SHOOT        = 1;
    public static final int UI_JUMP         = 2;
    public static final int UI_CROUCH       = 3;
    public static final int UI_RELOADBAR    = 4;
    public static final int UI_PDA          = 5;
    public static final int UI_FLASHLIGHT   = 6;
    public static final int UI_SAVE         = 7;
    public static final int UI_1            = 8;
    public static final int UI_2            = 9;
    public static final int UI_3            = 10;
    public static final int UI_KBD          = 11;
    public static final int UI_CONSOLE      = 12;
    public static final int UI_RUN          = 13;
    public static final int UI_ZOOM         = 14;
    public static final int UI_INTERACT     = 15;
    public static final int UI_WEAPON_PANEL = 16;
    public static final int UI_SCORE        = 17;
    public static final int UI_0            = 18;
    public static final int UI_4            = 19;
    public static final int UI_5            = 20;
    public static final int UI_6            = 21;
    public static final int UI_7            = 22;
    public static final int UI_8            = 23;
    public static final int UI_9            = 24;
    public static final int UI_NUM_PANEL    = 25;
    public static final int UI_Y            = 26;
    public static final int UI_N            = 27;
    public static final int UI_PLUS         = 28;
    public static final int UI_MINUS        = 29;

    public static final int UI_Q            = 30;
    public static final int UI_W            = 31;
    public static final int UI_E            = 32;
    public static final int UI_U            = 33;
    public static final int UI_T            = 34;
    public static final int UI_I            = 35;
    public static final int UI_O            = 36;
    public static final int UI_P            = 37;
    public static final int UI_A            = 38;
    public static final int UI_S            = 39;
    public static final int UI_D            = 40;
    public static final int UI_G            = 41;
    public static final int UI_H            = 42;
    public static final int UI_J            = 43;
    public static final int UI_K            = 44;
    public static final int UI_L            = 45;
    public static final int UI_X            = 46;
    public static final int UI_V            = 47;
    public static final int UI_B            = 48;

    public static final int UI_SIZE         = UI_B + 1;

    // on-screen item type
    public static final int TYPE_BUTTON       = 0;
    public static final int TYPE_SLIDER       = 1;
    public static final int TYPE_JOYSTICK     = 2;
    public static final int TYPE_DISC         = 3;
    public static final int TYPE_MOUSE        = -1;
    public static final int TYPE_MOUSE_BUTTON = -2;

    // mouse
    public static final int MOUSE_EVENT  = 1;
    public static final int MOUSE_DEVICE = 2;

    // default size
    public static final int SCREEN_WIDTH  = 640;
    public static final int SCREEN_HEIGHT = 480;

    // default ratio
    public static final int RATIO_WIDTH  = 4;
    public static final int RATIO_HEIGHT = 3;

    // on-screen button type
    public static final int ONSCREEN_BUTTON_TYPE_FULL         = 0;
    public static final int ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM = 1;
    public static final int ONSCREEN_BUTTON_TYPE_CENTER       = 2;
    public static final int ONSCREEN_BUTTON_TYPE_LEFT_TOP     = 3;

    // on-screen button can hold
    public static final int ONSCRREN_BUTTON_NOT_HOLD = 0;
    public static final int ONSCRREN_BUTTON_CAN_HOLD = 1;

    // on-screen slider type
    public static final int ONSCRREN_SLIDER_STYLE_LEFT_RIGHT             = 0;
    public static final int ONSCRREN_SLIDER_STYLE_DOWN_RIGHT             = 1;
    public static final int ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK = 2;
    public static final int ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK = 3;

    // on-screen joystick visible
    public static final int ONSCRREN_JOYSTICK_VISIBLE_ALWAYS       = 0;
    public static final int ONSCRREN_JOYSTICK_VISIBLE_HIDDEN       = 1;
    public static final int ONSCRREN_JOYSTICK_VISIBLE_ONLY_PRESSED = 2;

    // button swipe release delay
    public static final int BUTTON_SWIPE_RELEASE_DELAY_AUTO        = -1;
    public static final int BUTTON_SWIPE_RELEASE_DELAY_NONE        = 0;
    public static final int SERIOUS_SAM_BUTTON_SWIPE_RELEASE_DELAY = 50;

    // disc button trigger
    public static final int ONSCRREN_DISC_SWIPE = 0;
    public static final int ONSCRREN_DISC_CLICK = 1;

    // game state
    public static final int STATE_NONE    = 0;
    public static final int STATE_ACT     = 1; // RTCW4A-specific, keep
    public static final int STATE_GAME    = 1 << 1; // map spawned
    public static final int STATE_KICK    = 1 << 2; // RTCW4A-specific, keep
    public static final int STATE_LOADING = 1 << 3; // current GUI is guiLoading
    public static final int STATE_CONSOLE = 1 << 4; // fullscreen or not
    public static final int STATE_MENU    = 1 << 5; // any menu excludes guiLoading
    public static final int STATE_DEMO    = 1 << 6; // demo

    // game view control
    public static final int VIEW_MOTION_CONTROL_TOUCH     = 1;
    public static final int VIEW_MOTION_CONTROL_GYROSCOPE = 1 << 1;
    public static final int VIEW_MOTION_CONTROL_ALL       = VIEW_MOTION_CONTROL_TOUCH | VIEW_MOTION_CONTROL_GYROSCOPE;

    // signals handler
    public static final int SIGNALS_HANDLER_GAME      = 0;
    public static final int SIGNALS_HANDLER_NO_HANDLE = 1;
    public static final int SIGNALS_HANDLER_BACKTRACE = 2;

    // screen resolution
    public static final int SCREEN_FULL = 0;
    public static final int SCREEN_SCALE_BY_LENGTH = 1;
    public static final int SCREEN_SCALE_BY_AREA = 2;
    public static final int SCREEN_CUSTOM = 3;
    public static final int SCREEN_FIXED_RATIO = 4;

    public static final String[] CONTROLS_NAMES = {
            "Joystick",
            "Shoot",
            "Jump",
            "Crouch",
            "Reload",
            "PDA",
            "Flashlight",
            "Pause",
            "Extra 1",
            "Extra 2",
            "Extra 3",
            "Keyboard",
            "Console",
            "Run",
            "Zoom",
            "Interact",
            "Weapon",
            "Score",
            "Extra 0",
            "Extra 4",
            "Extra 5",
            "Extra 6",
            "Extra 7",
            "Extra 8",
            "Extra 9",
            "Number",
            "Key Y",
            "Key N",
            "Key +",
            "Key -",
            "Key Q",
            "Key W",
            "Key E",
            "Key U",
            "Key T",
            "Key I",
            "Key O",
            "Key P",
            "Key A",
            "Key S",
            "Key D",
            "Key G",
            "Key H",
            "Key J",
            "Key K",
            "Key L",
            "Key X",
            "Key V",
            "Key B",
    };

    // OpenGL Surface color format
    public static final int GLFORMAT_RGB565      = 0x0565;
    public static final int GLFORMAT_RGBA4444    = 0x4444;
    public static final int GLFORMAT_RGBA5551    = 0x5551;
    public static final int GLFORMAT_RGBA8888    = 0x8888;
    public static final int GLFORMAT_RGBA1010102 = 0xaaa2;

    // back key function mask
    public static final int ENUM_BACK_NONE   = 0;
    public static final int ENUM_BACK_ESCAPE = 1;
    public static final int ENUM_BACK_EXIT   = 2;
    public static final int ENUM_BACK_ALL    = 0xFF;

    public static final int CONST_DOUBLE_PRESS_BACK_TO_EXIT_INTERVAL = 500; // 1000
    public static final int CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT    = 3;

    public static final int DEFAULT_DEPTH_BITS = 24; // 16 32

    public static final String IDTECH4AMM_PAK_SUFFIX = ".zipak";



    public static boolean IsFDroidVersion()
    {
        return "fdroid".equalsIgnoreCase(BuildConfig.PRODUCT_FLAVOR);
    }

    public static boolean IsGithubVersion()
    {
        return !IsFDroidVersion();
    }



    public static  boolean IS_NEON      = false; // only armv7-a 32. arm64 always support, but using hard
    public static  boolean IS_64        = false;
    public static  boolean SYSTEM_64    = false;
    public static  String  ARCH         = "";
    public static  String  ARCH_DIR     = "";
    private static boolean _is_detected = false;


    private static boolean GetCpuInfo()
    {
        if(_is_detected)
            return true;
        IS_64 = Q3EJNI.Is64();
        ARCH = IS_64 ? "aarch64" : "arm";
        ARCH_DIR = IS_64 ? "arm64-v8a" : "armeabi-v7a";
        BufferedReader br = null;
        try
        {
            br = new BufferedReader(new FileReader("/proc/cpuinfo"));
            String l;
            while((l = br.readLine()) != null)
            {
                if((l.contains("Features")) && (l.contains("neon")))
                {
                    IS_NEON = true;
                }
                if(l.contains("Processor") && (l.contains("AArch64")))
                {
                    SYSTEM_64 = true;
                    IS_NEON = true;
                }

            }
            _is_detected = true;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            _is_detected = false;
        }
        finally
        {
            try
            {
                if(br != null)
                    br.close();
            }
            catch(IOException ioe)
            {
                ioe.printStackTrace();
            }
        }
        return _is_detected;
    }

    static
    {
        GetCpuInfo();
    }

    private Q3EGlobals() {}
}
