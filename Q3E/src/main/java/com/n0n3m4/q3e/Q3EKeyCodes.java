/*
 	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import android.content.Context;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.n0n3m4.q3e.control.Q3EControllerControl;
import com.n0n3m4.q3e.device.Q3EOuya;
import com.n0n3m4.q3e.keycode.KeyCodesD3;
import com.n0n3m4.q3e.keycode.KeyCodesGeneric;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

@SuppressWarnings("unused")
public class Q3EKeyCodes
{
    // KARIN_NEW_GAME_BOOKMARK: add key code converter

    public static class KeyCodes
    {
        public static int K_TAB;
        public static int K_ENTER;
        public static int K_ESCAPE;
        public static int K_SPACE;
        public static int K_BACKSPACE;
        public static int K_CAPSLOCK;
        public static int K_PAUSE;
        public static int K_UPARROW;
        public static int K_DOWNARROW;
        public static int K_LEFTARROW;
        public static int K_RIGHTARROW;
        public static int K_ALT;
        public static int K_CTRL;
        public static int K_SHIFT;
        public static int K_INS;
        public static int K_DEL;
        public static int K_PGDN;
        public static int K_PGUP;
        public static int K_HOME;
        public static int K_END;
        public static int K_F1;
        public static int K_F2;
        public static int K_F3;
        public static int K_F4;
        public static int K_F5;
        public static int K_F6;
        public static int K_F7;
        public static int K_F8;
        public static int K_F9;
        public static int K_F10;
        public static int K_F11;
        public static int K_F12;
        public static int K_MOUSE1;
        public static int K_MOUSE2;
        public static int K_MOUSE3;
        public static int K_MOUSE4;
        public static int K_MOUSE5;
        public static int K_MWHEELDOWN;
        public static int K_MWHEELUP;

        public static int J_LEFT;
        public static int J_RIGHT;
        public static int J_UP;
        public static int J_DOWN;

        public static int K_A;
        public static int K_B;
        public static int K_C;
        public static int K_D;
        public static int K_E;
        public static int K_F;
        public static int K_G;
        public static int K_H;
        public static int K_I;
        public static int K_J;
        public static int K_K;
        public static int K_L;
        public static int K_M;
        public static int K_N;
        public static int K_O;
        public static int K_P;
        public static int K_Q;
        public static int K_R;
        public static int K_S;
        public static int K_T;
        public static int K_U;
        public static int K_V;
        public static int K_W;
        public static int K_X;
        public static int K_Y;
        public static int K_Z;

        public static int K_0;
        public static int K_1;
        public static int K_2;
        public static int K_3;
        public static int K_4;
        public static int K_5;
        public static int K_6;
        public static int K_7;
        public static int K_8;
        public static int K_9;

        public static int K_KP_1;
        public static int K_KP_2;
        public static int K_KP_3;
        public static int K_KP_4;
        public static int K_KP_5;
        public static int K_KP_6;
        public static int K_KP_7;
        public static int K_KP_8;
        public static int K_KP_9;
        public static int K_KP_0;

        public static int K_GRAVE;
        public static int K_LBRACKET;
        public static int K_RBRACKET;

        public static boolean RAW = false;

        public static int J_DPAD_UP;
        public static int J_DPAD_DOWN;
        public static int J_DPAD_LEFT;
        public static int J_DPAD_RIGHT;
        public static int J_DPAD_CENTER;
        public static int J_BUTTON_A;
        public static int J_BUTTON_B;
        public static int J_BUTTON_C;
        public static int J_BUTTON_X;
        public static int J_BUTTON_Y;
        public static int J_BUTTON_Z;
        public static int J_BUTTON_1;
        public static int J_BUTTON_2;
        public static int J_BUTTON_3;
        public static int J_BUTTON_4;
        public static int J_BUTTON_5;
        public static int J_BUTTON_6;
        public static int J_BUTTON_7;
        public static int J_BUTTON_8;
        public static int J_BUTTON_9;
        public static int J_BUTTON_10;
        public static int J_BUTTON_11;
        public static int J_BUTTON_12;
        public static int J_BUTTON_13;
        public static int J_BUTTON_14;
        public static int J_BUTTON_15;
        public static int J_BUTTON_16;
        public static int J_BUTTON_START;
        public static int J_BUTTON_SELECT;
        public static int J_BUTTON_L1;
        public static int J_BUTTON_R1;
        public static int J_BUTTON_L2;
        public static int J_BUTTON_R2;
        public static int J_BUTTON_L3;
        public static int J_BUTTON_R3;
    }

    public static final int K_VKBD = 9000;

    public static void InitD3Keycodes()
    {
        InitKeycodes(KeyCodesD3.class);
    }

    public static void InitKeycodes(Class<?> clazz)
    {
        KeyCodes.RAW = false;
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Using key map: " + clazz.getName());
        for(Field f : KeyCodes.class.getDeclaredFields())
        {
            try
            {
                f.set(null, clazz.getDeclaredField(f.getName()).get(null));
            }
            catch(Exception ignored)
            {
                try // else setup generic key codes
                {
                    f.set(null, KeyCodesGeneric.class.getDeclaredField(f.getName()).get(null));
                }
                catch(Exception ignored2)
                {
                }
            }
        }
    }

    public static int GetRealKeyCode(int keycodeGeneric)
    {
        Field[] fields = KeyCodesGeneric.class.getDeclaredFields();
        for(Field field : fields)
        {
            try
            {
                int key = (Integer) field.get(null);
                if(key == keycodeGeneric)
                {
                    String name = field.getName();
                    Field f = KeyCodes.class.getDeclaredField(name);
                    Object o = f.get(null);
                    Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Map virtual key: " + name + " = " + keycodeGeneric + " -> " + o);
                    return (Integer) o;
                }
            }
            catch(Exception ignored)
            {
            }
        }
        return keycodeGeneric;
    }

    // UNUSED
    public static int[] GetRealKeyCodes(int[] keycodeGeneric)
    {
        int[] codes = new int[keycodeGeneric.length];
        for(int i = 0; i < keycodeGeneric.length; i++)
            codes[i] = Q3EKeyCodes.GetRealKeyCode(keycodeGeneric[i]);
        return codes;
    }

    // Use in on-screen JoyStick
    public static void ConvertRealKeyCodes(int[] codes)
    {
        for(int i = 0; i < codes.length; i++)
            codes[i] = Q3EKeyCodes.GetRealKeyCode(codes[i]);
    }

    public static int convertKeyCode(int keyCode, int uchar, KeyEvent event)
    {
        switch(keyCode)
        {
            case KeyEvent.KEYCODE_FOCUS:
                return KeyCodes.K_F1;
            // volume keys
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                return KeyCodes.K_F2;
            case KeyEvent.KEYCODE_VOLUME_UP:
                return KeyCodes.K_F3;
            case KeyEvent.KEYCODE_ENTER:
                return KeyCodes.K_ENTER;
            case KeyEvent.KEYCODE_BACK:
                return KeyCodes.K_ESCAPE;
            case KeyEvent.KEYCODE_DEL:
                return KeyCodes.K_BACKSPACE;
            case KeyEvent.KEYCODE_ALT_LEFT:
            case KeyEvent.KEYCODE_ALT_RIGHT:
                return KeyCodes.K_ALT;
            case KeyEvent.KEYCODE_SHIFT_LEFT:
            case KeyEvent.KEYCODE_SHIFT_RIGHT:
                return KeyCodes.K_SHIFT;
            case KeyEvent.KEYCODE_CTRL_LEFT:
            case KeyEvent.KEYCODE_CTRL_RIGHT:
                return KeyCodes.K_CTRL;
            case KeyEvent.KEYCODE_INSERT:
                return KeyCodes.K_INS;
            case 122:
                return KeyCodes.K_HOME;
            case KeyEvent.KEYCODE_FORWARD_DEL:
                return KeyCodes.K_DEL;
            case 123:
                return KeyCodes.K_END;
            case KeyEvent.KEYCODE_ESCAPE:
                return KeyCodes.K_ESCAPE;
            case KeyEvent.KEYCODE_TAB:
                return KeyCodes.K_TAB;
            case KeyEvent.KEYCODE_F1:
                return KeyCodes.K_F1;
            case KeyEvent.KEYCODE_F2:
                return KeyCodes.K_F2;
            case KeyEvent.KEYCODE_F3:
                return KeyCodes.K_F3;
            case KeyEvent.KEYCODE_F4:
                return KeyCodes.K_F4;
            case KeyEvent.KEYCODE_F5:
                return KeyCodes.K_F5;
            case KeyEvent.KEYCODE_F6:
                return KeyCodes.K_F6;
            case KeyEvent.KEYCODE_F7:
                return KeyCodes.K_F7;
            case KeyEvent.KEYCODE_F8:
                return KeyCodes.K_F8;
            case KeyEvent.KEYCODE_F9:
                return KeyCodes.K_F9;
            case KeyEvent.KEYCODE_F10:
                return KeyCodes.K_F10;
            case KeyEvent.KEYCODE_F11:
                return KeyCodes.K_F11;
            case KeyEvent.KEYCODE_F12:
                return KeyCodes.K_F12;
            case KeyEvent.KEYCODE_CAPS_LOCK:
                return KeyCodes.K_CAPSLOCK;
            case KeyEvent.KEYCODE_PAGE_DOWN:
                return KeyCodes.K_PGDN;
            case KeyEvent.KEYCODE_PAGE_UP:
                return KeyCodes.K_PGUP;
            // keyboard arrow / dpad
            case KeyEvent.KEYCODE_DPAD_UP:
                return Q3EControllerControl.IsGamePadDevice(event) ? KeyCodes.J_DPAD_UP : KeyCodes.K_UPARROW;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                return Q3EControllerControl.IsGamePadDevice(event) ? KeyCodes.J_DPAD_DOWN : KeyCodes.K_DOWNARROW;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                return Q3EControllerControl.IsGamePadDevice(event) ? KeyCodes.J_DPAD_LEFT : KeyCodes.K_LEFTARROW;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                return Q3EControllerControl.IsGamePadDevice(event) ? KeyCodes.J_DPAD_RIGHT : KeyCodes.K_RIGHTARROW;
            case KeyEvent.KEYCODE_DPAD_CENTER:
                return Q3EControllerControl.IsGamePadDevice(event) ? KeyCodes.J_DPAD_CENTER : KeyCodes.K_ENTER;
            // Controller
            // a b c x y z
            case KeyEvent.KEYCODE_BUTTON_A:
                return KeyCodes.J_BUTTON_A;
            case KeyEvent.KEYCODE_BUTTON_B:
                return KeyCodes.J_BUTTON_B;
            case KeyEvent.KEYCODE_BUTTON_X:
                //if(Q3EUtils.isOuya) return KeyCodes.K_ENTER;//No enter button on ouya
                return KeyCodes.J_BUTTON_X;//Why not?
            case KeyEvent.KEYCODE_BUTTON_Y:
                return KeyCodes.J_BUTTON_Y;//RTCW use
            //These buttons are not so popular
            case KeyEvent.KEYCODE_BUTTON_C:
                return KeyCodes.J_BUTTON_C;//That's why here is a, nobody cares.
            case KeyEvent.KEYCODE_BUTTON_Z:
                return KeyCodes.J_BUTTON_Z;
            // 1- 16
            case KeyEvent.KEYCODE_BUTTON_1:
                return KeyCodes.J_BUTTON_1;
            case KeyEvent.KEYCODE_BUTTON_2:
                return KeyCodes.J_BUTTON_2;
            case KeyEvent.KEYCODE_BUTTON_3:
                return KeyCodes.J_BUTTON_3;
            case KeyEvent.KEYCODE_BUTTON_4:
                return KeyCodes.J_BUTTON_4;
            case KeyEvent.KEYCODE_BUTTON_5:
                return KeyCodes.J_BUTTON_5;
            case KeyEvent.KEYCODE_BUTTON_6:
                return KeyCodes.J_BUTTON_6;
            case KeyEvent.KEYCODE_BUTTON_7:
                return KeyCodes.J_BUTTON_7;
            case KeyEvent.KEYCODE_BUTTON_8:
                return KeyCodes.J_BUTTON_8;
            case KeyEvent.KEYCODE_BUTTON_9:
                return KeyCodes.J_BUTTON_9;
            case KeyEvent.KEYCODE_BUTTON_10:
                return KeyCodes.J_BUTTON_10;

            case KeyEvent.KEYCODE_BUTTON_11:
                return KeyCodes.J_BUTTON_11;
            case KeyEvent.KEYCODE_BUTTON_12:
                return KeyCodes.J_BUTTON_12;
            case KeyEvent.KEYCODE_BUTTON_13:
                return KeyCodes.J_BUTTON_13;
            case KeyEvent.KEYCODE_BUTTON_14:
                return KeyCodes.J_BUTTON_14;
            case KeyEvent.KEYCODE_BUTTON_15:
                return KeyCodes.J_BUTTON_15;
            case KeyEvent.KEYCODE_BUTTON_16:
                return KeyCodes.J_BUTTON_16;

            //--------------------------------
            case KeyEvent.KEYCODE_BUTTON_START:
                return KeyCodes.J_BUTTON_START;
            case KeyEvent.KEYCODE_BUTTON_SELECT:
                return KeyCodes.J_BUTTON_SELECT;
            case KeyEvent.KEYCODE_BUTTON_L2: // left trigger
                return KeyCodes.J_BUTTON_L2;
            case KeyEvent.KEYCODE_BUTTON_R2: // right trigger
                return KeyCodes.J_BUTTON_R2;
            case KeyEvent.KEYCODE_BUTTON_R1: // right shoulder
                return KeyCodes.J_BUTTON_R1;//Sometimes it is necessary
            case KeyEvent.KEYCODE_BUTTON_L1: // left shoulder
                //if(Q3EUtils.isOuya) return KeyCodes.K_SPACE;
                return KeyCodes.J_BUTTON_L1;//dunno why
/*            case Q3EOuya.BUTTON_L3:
                return '[';
            case Q3EOuya.BUTTON_R3:
                return ']';*/
            case KeyEvent.KEYCODE_BUTTON_THUMBL: // left joystick
                return KeyCodes.J_BUTTON_L3;
            case KeyEvent.KEYCODE_BUTTON_THUMBR: // right joystick
                return KeyCodes.J_BUTTON_R3;
            // end Controller

            case KeyEvent.KEYCODE_MENU:
                /*if(Q3EUtils.isOuya)*/ return KeyCodes.K_ESCAPE;
                //break;
        }
        if(KeyCodes.RAW)
            return keyCode;

        //int uchar = event.getUnicodeChar(0);
        if((uchar < 127) && (uchar != 0))
            return uchar;
        return keyCode % 95 + 32;//Magic
    }

    public static class ControllerCodesGeneric
    {
        public static final int J_DPAD_UP = KeyCodesGeneric.J_DPAD_UP;
        public static final int J_DPAD_DOWN = KeyCodesGeneric.J_DPAD_DOWN;
        public static final int J_DPAD_LEFT = KeyCodesGeneric.J_DPAD_LEFT;
        public static final int J_DPAD_RIGHT = KeyCodesGeneric.J_DPAD_RIGHT;
        public static final int J_DPAD_CENTER = KeyCodesGeneric.J_DPAD_CENTER;
        public static final int J_BUTTON_A = KeyCodesGeneric.J_BUTTON_A;
        public static final int J_BUTTON_B = KeyCodesGeneric.J_BUTTON_B;
        public static final int J_BUTTON_C = KeyCodesGeneric.J_BUTTON_C;
        public static final int J_BUTTON_X = KeyCodesGeneric.J_BUTTON_X;
        public static final int J_BUTTON_Y = KeyCodesGeneric.J_BUTTON_Y;
        public static final int J_BUTTON_Z = KeyCodesGeneric.J_BUTTON_Z;
        public static final int J_BUTTON_1 = KeyCodesGeneric.J_BUTTON_1;
        public static final int J_BUTTON_2 = KeyCodesGeneric.J_BUTTON_2;
        public static final int J_BUTTON_3 = KeyCodesGeneric.J_BUTTON_3;
        public static final int J_BUTTON_4 = KeyCodesGeneric.J_BUTTON_4;
        public static final int J_BUTTON_5 = KeyCodesGeneric.J_BUTTON_5;
        public static final int J_BUTTON_6 = KeyCodesGeneric.J_BUTTON_6;
        public static final int J_BUTTON_7 = KeyCodesGeneric.J_BUTTON_7;
        public static final int J_BUTTON_8 = KeyCodesGeneric.J_BUTTON_8;
        public static final int J_BUTTON_9 = KeyCodesGeneric.J_BUTTON_9;
        public static final int J_BUTTON_10 = KeyCodesGeneric.J_BUTTON_10;
        public static final int J_BUTTON_11 = KeyCodesGeneric.J_BUTTON_11;
        public static final int J_BUTTON_12 = KeyCodesGeneric.J_BUTTON_12;
        public static final int J_BUTTON_13 = KeyCodesGeneric.J_BUTTON_13;
        public static final int J_BUTTON_14 = KeyCodesGeneric.J_BUTTON_14;
        public static final int J_BUTTON_15 = KeyCodesGeneric.J_BUTTON_15;
        public static final int J_BUTTON_16 = KeyCodesGeneric.J_BUTTON_16;
        public static final int J_BUTTON_START = KeyCodesGeneric.J_BUTTON_START;
        public static final int J_BUTTON_SELECT = KeyCodesGeneric.J_BUTTON_SELECT;
        public static final int J_BUTTON_L1 = KeyCodesGeneric.J_BUTTON_L1;
        public static final int J_BUTTON_R1 = KeyCodesGeneric.J_BUTTON_R1;
        public static final int J_BUTTON_L2 = KeyCodesGeneric.J_BUTTON_L2;
        public static final int J_BUTTON_R2 = KeyCodesGeneric.J_BUTTON_R2;
        public static final int J_BUTTON_L3 = KeyCodesGeneric.J_BUTTON_L3;
        public static final int J_BUTTON_R3 = KeyCodesGeneric.J_BUTTON_R3;
    }

    public static int GetKeycodeByName(String name)
    {
        Field field;
        try
        {
            field = KeyCodes.class.getDeclaredField(name);
            return (int) field.get(null);
        }
        catch(NoSuchFieldException e)
        {
            try
            {
                field = KeyCodesGeneric.class.getDeclaredField(name);
                return (int) field.get(null);
            }
            catch(Exception ex)
            {
                ex.printStackTrace();
            }
        }
        catch(IllegalAccessException e)
        {
            e.printStackTrace();
        }
        return 0;
    }

    public static void SetKeycodeByName(String name, Integer code)
    {
        Field field;
        try
        {
            if(null == code)
            {
                field = KeyCodesGeneric.class.getDeclaredField(name);
                code = (int)field.get(null);
            }
            field = KeyCodes.class.getDeclaredField(name);
            field.set(null, code);
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Map key: " + name + " -> " + code);
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }
    }

    public static final int ONSCRREN_DISC_KEYS_WEAPON = 1;
    public static final int ONSCRREN_DISC_KEYS_NUM    = 2;

    // disc button keys
    public static final String[] ONSCRREN_DISC_KEYS_STRS     = new String[] {
            "1,2,3,4,5,6,7,8,9,q,0",
            "1,2,3,4,5,6,7,8,9,0",
    };
    public static final int[][]  ONSCRREN_DISC_KEYS_KEYCODES = new int[][] {
            null,
            {KeyCodesGeneric.K_KP_1, KeyCodesGeneric.K_KP_2, KeyCodesGeneric.K_KP_3, KeyCodesGeneric.K_KP_4, KeyCodesGeneric.K_KP_5, KeyCodesGeneric.K_KP_6, KeyCodesGeneric.K_KP_7, KeyCodesGeneric.K_KP_8, KeyCodesGeneric.K_KP_9, KeyCodesGeneric.K_KP_0},
    };

    public static final String[] CONTROLLER_BUTTONS = {
            "button_a",
            "button_b",
            "button_c",
            "button_x",
            "button_y",
            "button_z",
            "button_l1",
            "button_r1",
            "button_l2",
            "button_r2",
            "button_l3",
            "button_r3",
            "button_start",
            "button_select",
            "button_1",
            "button_2",
            "button_3",
            "button_4",
            "button_5",
            "button_6",
            "button_7",
            "button_8",
            "button_9",
            "button_10",
            "button_11",
            "button_12",
            "button_13",
            "button_14",
            "button_15",
            "button_16",
    };

    public static int GetDefaultGamePadButtonCode(String button)
    {
        try {
            Field field = ControllerCodesGeneric.class.getDeclaredField("J_" + button.toUpperCase());
            Object o = field.get(null);
            return (Integer) o;
        } catch (Exception e) {
            e.printStackTrace();
            return 0;
        }
    }

    public static String GetDefaultGamePadButtonFieldName(String button)
    {
        try {
            Field field = ControllerCodesGeneric.class.getDeclaredField("J_" + button.toUpperCase());
            return field.getName();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public static Map<String, Integer> LoadGamePadButtonCodeMap(Context context)
    {
        Set<String> codeSet = PreferenceManager.getDefaultSharedPreferences(context).getStringSet(Q3EPreference.pref_harm_gamepad_keymap, new HashSet<>());
        Map<String, Integer> codeMap = new HashMap<>();
        for (String s : codeSet) {
            String[] split = s.split(":");
            codeMap.put(split[0], Q3EUtils.parseInt_s(split[1]));
        }
        return codeMap;
    }
}
