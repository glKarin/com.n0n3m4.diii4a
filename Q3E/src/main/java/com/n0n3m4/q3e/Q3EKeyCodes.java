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

import android.util.Log;
import android.view.KeyEvent;

import com.n0n3m4.q3e.device.Q3EOuya;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;

public class Q3EKeyCodes
{

    public static final int K_VKBD = 9000;

    public static class KeyCodesRTCW
    {
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;

        public static final int K_BACKSPACE = 127;

        public static final int K_COMMAND = 128;
        public static final int K_CAPSLOCK = 129;
        public static final int K_POWER = 130;
        public static final int K_PAUSE = 131;

        public static final int K_UPARROW = 132;
        public static final int K_DOWNARROW = 133;
        public static final int K_LEFTARROW = 134;
        public static final int K_RIGHTARROW = 135;

        public static final int K_ALT = 136;
        public static final int K_CTRL = 137;
        public static final int K_SHIFT = 138;
        public static final int K_INS = 139;
        public static final int K_DEL = 140;
        public static final int K_PGDN = 141;
        public static final int K_PGUP = 142;
        public static final int K_HOME = 143;
        public static final int K_END = 144;

        public static final int K_F1 = 145;
        public static final int K_F2 = 146;
        public static final int K_F3 = 147;
        public static final int K_F4 = 148;
        public static final int K_F5 = 149;
        public static final int K_F6 = 150;
        public static final int K_F7 = 151;
        public static final int K_F8 = 152;
        public static final int K_F9 = 153;
        public static final int K_F10 = 154;
        public static final int K_F11 = 155;
        public static final int K_F12 = 156;
        public static final int K_F13 = 157;
        public static final int K_F14 = 158;
        public static final int K_F15 = 159;

        public static final int K_KP_HOME = 160;
        public static final int K_KP_UPARROW = 161;
        public static final int K_KP_PGUP = 162;
        public static final int K_KP_LEFTARROW = 163;
        public static final int K_KP_5 = 164;
        public static final int K_KP_RIGHTARROW = 165;
        public static final int K_KP_END = 166;
        public static final int K_KP_DOWNARROW = 167;
        public static final int K_KP_PGDN = 168;
        public static final int K_KP_ENTER = 169;
        public static final int K_KP_INS = 170;
        public static final int K_KP_DEL = 171;
        public static final int K_KP_SLASH = 172;
        public static final int K_KP_MINUS = 173;
        public static final int K_KP_PLUS = 174;
        public static final int K_KP_NUMLOCK = 175;
        public static final int K_KP_STAR = 176;
        public static final int K_KP_EQUALS = 177;

        public static final int K_MOUSE1 = 178;
        public static final int K_MOUSE2 = 179;
        public static final int K_MOUSE3 = 180;
        public static final int K_MOUSE4 = 181;
        public static final int K_MOUSE5 = 182;

        public static final int K_MWHEELDOWN = 183;
        public static final int K_MWHEELUP = 184;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    public static class KeyCodesQ3
    {
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;

        public static final int K_BACKSPACE = 127;

        public static final int K_COMMAND = 128;
        public static final int K_CAPSLOCK = 129;
        public static final int K_POWER = 130;
        public static final int K_PAUSE = 131;

        public static final int K_UPARROW = 132;
        public static final int K_DOWNARROW = 133;
        public static final int K_LEFTARROW = 134;
        public static final int K_RIGHTARROW = 135;

        public static final int K_ALT = 136;
        public static final int K_CTRL = 137;
        public static final int K_SHIFT = 138;
        public static final int K_INS = 139;
        public static final int K_DEL = 140;
        public static final int K_PGDN = 141;
        public static final int K_PGUP = 142;
        public static final int K_HOME = 143;
        public static final int K_END = 144;

        public static final int K_F1 = 145;
        public static final int K_F2 = 146;
        public static final int K_F3 = 147;
        public static final int K_F4 = 148;
        public static final int K_F5 = 149;
        public static final int K_F6 = 150;
        public static final int K_F7 = 151;
        public static final int K_F8 = 152;
        public static final int K_F9 = 153;
        public static final int K_F10 = 154;
        public static final int K_F11 = 155;
        public static final int K_F12 = 156;
        public static final int K_F13 = 157;
        public static final int K_F14 = 158;
        public static final int K_F15 = 159;

        public static final int K_KP_HOME = 160;
        public static final int K_KP_UPARROW = 161;
        public static final int K_KP_PGUP = 162;
        public static final int K_KP_LEFTARROW = 163;
        public static final int K_KP_5 = 164;
        public static final int K_KP_RIGHTARROW = 165;
        public static final int K_KP_END = 166;
        public static final int K_KP_DOWNARROW = 167;
        public static final int K_KP_PGDN = 168;
        public static final int K_KP_ENTER = 169;
        public static final int K_KP_INS = 170;
        public static final int K_KP_DEL = 171;
        public static final int K_KP_SLASH = 172;
        public static final int K_KP_MINUS = 173;
        public static final int K_KP_PLUS = 174;
        public static final int K_KP_NUMLOCK = 175;
        public static final int K_KP_STAR = 176;
        public static final int K_KP_EQUALS = 177;

        public static final int K_MOUSE1 = 178;
        public static final int K_MOUSE2 = 179;
        public static final int K_MOUSE3 = 180;
        public static final int K_MOUSE4 = 181;
        public static final int K_MOUSE5 = 182;

        public static final int K_MWHEELDOWN = 183;
        public static final int K_MWHEELUP = 184;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    public static class KeyCodesD3
    {
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;
        public static final int K_BACKSPACE = 127;
        public static final int K_COMMAND = 128;
        public static final int K_CAPSLOCK = 129;
        public static final int K_SCROLL = 130;
        public static final int K_POWER = 131;
        public static final int K_PAUSE = 132;
        public static final int K_UPARROW = 133;
        public static final int K_DOWNARROW = 134;
        public static final int K_LEFTARROW = 135;
        public static final int K_RIGHTARROW = 136;
        public static final int K_LWIN = 137;
        public static final int K_RWIN = 138;
        public static final int K_MENU = 139;
        public static final int K_ALT = 140;
        public static final int K_CTRL = 141;
        public static final int K_SHIFT = 142;
        public static final int K_INS = 143;
        public static final int K_DEL = 144;
        public static final int K_PGDN = 145;
        public static final int K_PGUP = 146;
        public static final int K_HOME = 147;
        public static final int K_END = 148;
        public static final int K_F1 = 149;
        public static final int K_F2 = 150;
        public static final int K_F3 = 151;
        public static final int K_F4 = 152;
        public static final int K_F5 = 153;
        public static final int K_F6 = 154;
        public static final int K_F7 = 155;
        public static final int K_F8 = 156;
        public static final int K_F9 = 157;
        public static final int K_F10 = 158;
        public static final int K_F11 = 159;
        public static final int K_F12 = 160;
        public static final int K_INVERTED_EXCLAMATION = 161;
        public static final int K_F13 = 162;
        public static final int K_F14 = 163;
        public static final int K_F15 = 164;
        public static final int K_MOUSE1 = 187;
        public static final int K_MOUSE2 = 188;
        public static final int K_MOUSE3 = 189;
        public static final int K_MOUSE4 = 190;
        public static final int K_MOUSE5 = 191;
        public static final int K_MWHEELDOWN = 195;
        public static final int K_MWHEELUP = 196;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    public static class KeyCodesD3BFG
    {
        public static final int K_NONE = 0;
        public static final int K_ESCAPE = 1;
        public static final int K_1 = 2;
        public static final int K_2 = 3;
        public static final int K_3 = 4;
        public static final int K_4 = 5;
        public static final int K_5 = 6;
        public static final int K_6 = 7;
        public static final int K_7 = 8;
        public static final int K_8 = 9;
        public static final int K_9 = 10;
        public static final int K_0 = 11;
        public static final int K_MINUS = 12;
        public static final int K_EQUALS = 13;
        public static final int K_BACKSPACE = 14;
        public static final int K_TAB = 15;
        public static final int K_Q = 16;
        public static final int K_W = 17;
        public static final int K_E = 18;
        public static final int K_R = 19;
        public static final int K_T = 20;
        public static final int K_Y = 21;
        public static final int K_U = 22;
        public static final int K_I = 23;
        public static final int K_O = 24;
        public static final int K_P = 25;
        public static final int K_LBRACKET = 26;
        public static final int K_RBRACKET = 27;
        public static final int K_ENTER = 28;
        public static final int K_CTRL = 29;
        public static final int K_A = 30;
        public static final int K_S = 31;
        public static final int K_D = 32;
        public static final int K_F = 33;
        public static final int K_G = 34;
        public static final int K_H = 35;
        public static final int K_J = 36;
        public static final int K_K = 37;
        public static final int K_L = 38;
        public static final int K_SEMICOLON = 39;
        public static final int K_APOSTROPHE = 40;
        public static final int K_GRAVE = 41;
        public static final int K_SHIFT = 42;
        public static final int K_BACKSLASH = 43;
        public static final int K_Z = 44;
        public static final int K_X = 45;
        public static final int K_C = 46;
        public static final int K_V = 47;
        public static final int K_B = 48;
        public static final int K_N = 49;
        public static final int K_M = 50;
        public static final int K_COMMA = 51;
        public static final int K_PERIOD = 52;
        public static final int K_SLASH = 53;
        public static final int K_RSHIFT = 54;
        public static final int K_KP_STAR = 55;
        public static final int K_ALT = 56;
        public static final int K_SPACE = 57;
        public static final int K_CAPSLOCK = 58;
        public static final int K_F1 = 59;
        public static final int K_F2 = 60;
        public static final int K_F3 = 61;
        public static final int K_F4 = 62;
        public static final int K_F5 = 63;
        public static final int K_F6 = 64;
        public static final int K_F7 = 65;
        public static final int K_F8 = 66;
        public static final int K_F9 = 67;
        public static final int K_F10 = 68;
        public static final int K_NUMLOCK = 69;
        public static final int K_SCROLL = 70;
        public static final int K_KP_7 = 71;
        public static final int K_KP_8 = 72;
        public static final int K_KP_9 = 73;
        public static final int K_KP_MINUS = 74;
        public static final int K_KP_4 = 75;
        public static final int K_KP_5 = 76;
        public static final int K_KP_6 = 77;
        public static final int K_KP_PLUS = 78;
        public static final int K_KP_1 = 79;
        public static final int K_KP_2 = 80;
        public static final int K_KP_3 = 81;
        public static final int K_KP_0 = 82;
        public static final int K_KP_DOT = 83;
        public static final int K_F11 = 0x57;
        public static final int K_F12 = 0x58;
        public static final int K_F13 = 0x64;
        public static final int K_F14 = 0x65;
        public static final int K_F15 = 0x66;
        public static final int K_KANA = 0x70;
        public static final int K_CONVERT = 0x79;
        public static final int K_NOCONVERT = 0x7B;
        public static final int K_YEN = 0x7D;
        public static final int K_KP_EQUALS = 0x8D;
        public static final int K_CIRCUMFLEX = 0x90;
        public static final int K_AT = 0x91;
        public static final int K_COLON = 0x92;
        public static final int K_UNDERLINE = 0x93;
        public static final int K_KANJI = 0x94;
        public static final int K_STOP = 0x95;
        public static final int K_AX = 0x96;
        public static final int K_UNLABELED = 0x97;
        public static final int K_KP_ENTER = 0x9C;
        public static final int K_RCTRL = 0x9D;
        public static final int K_KP_COMMA = 0xB3;
        public static final int K_KP_SLASH = 0xB5;
        public static final int K_PRINTSCREEN = 0xB7;
        public static final int K_RALT = 0xB8;
        public static final int K_PAUSE = 0xC5;
        public static final int K_HOME = 0xC7;
        public static final int K_UPARROW = 0xC8;
        public static final int K_PGUP = 0xC9;
        public static final int K_LEFTARROW = 0xCB;
        public static final int K_RIGHTARROW = 0xCD;
        public static final int K_END = 0xCF;
        public static final int K_DOWNARROW = 0xD0;
        public static final int K_PGDN = 0xD1;
        public static final int K_INS = 0xD2;
        public static final int K_DEL = 0xD3;
        public static final int K_LWIN = 0xDB;
        public static final int K_RWIN = 0xDC;
        public static final int K_APPS = 0xDD;
        public static final int K_POWER = 0xDE;
        public static final int K_SLEEP = 0xDF;
        public static final int K_MOUSE1 = 286;
        public static final int K_MOUSE2 = 287;
        public static final int K_MOUSE3 = 288;
        public static final int K_MOUSE4 = 289;
        public static final int K_MOUSE5 = 290;
        public static final int K_MOUSE6 = 291;
        public static final int K_MOUSE7 = 292;
        public static final int K_MOUSE8 = 293;
        public static final int K_MWHEELDOWN = 294;
        public static final int K_MWHEELUP = 295;

        //karin: change to a/d
//        public static final int J_LEFT = K_LEFTARROW;
//        public static final int J_RIGHT = K_RIGHTARROW;
        public static final int J_LEFT = K_A;
        public static final int J_RIGHT = K_D;
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    public static class KeyCodesQ1
    {
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;
        public static final int K_BACKSPACE = 127;
        public static final int K_UPARROW = 128;
        public static final int K_DOWNARROW = 129;
        public static final int K_LEFTARROW = 130;
        public static final int K_RIGHTARROW = 131;
        public static final int K_ALT = 132;
        public static final int K_CTRL = 133;
        public static final int K_SHIFT = 134;
        public static final int K_F1 = 135;
        public static final int K_F2 = 136;
        public static final int K_F3 = 137;
        public static final int K_F4 = 138;
        public static final int K_F5 = 139;
        public static final int K_F6 = 140;
        public static final int K_F7 = 141;
        public static final int K_F8 = 142;
        public static final int K_F9 = 143;
        public static final int K_F10 = 144;
        public static final int K_F11 = 145;
        public static final int K_F12 = 146;
        public static final int K_INS = 147;
        public static final int K_DEL = 148;
        public static final int K_PGDN = 149;
        public static final int K_PGUP = 150;
        public static final int K_HOME = 151;
        public static final int K_END = 152;
        public static final int K_PAUSE = 153;
        public static final int K_NUMLOCK = 154;
        public static final int K_CAPSLOCK = 155;
        public static final int K_SCROLLOCK = 156;
        public static final int K_MOUSE1 = 512;
        public static final int K_MOUSE2 = 513;
        public static final int K_MOUSE3 = 514;
        public static final int K_MWHEELUP = 515;
        public static final int K_MWHEELDOWN = 516;
        public static final int K_MOUSE4 = 517;
        public static final int K_MOUSE5 = 518;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;


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
    }

    ;

    public static void InitRTCWKeycodes()
    {
        InitKeycodes(KeyCodesRTCW.class);
    }

    public static void InitQ3Keycodes()
    {
        InitKeycodes(KeyCodesQ3.class);
    }

    public static void InitD3Keycodes()
    {
        InitKeycodes(KeyCodesD3.class);
    }

    public static void InitD3BFGKeycodes()
    {
        InitKeycodes(KeyCodesD3BFG.class);
    }

    public static void InitQ1Keycodes()
    {
        InitKeycodes(KeyCodesQ1.class);
    }

    public static void InitKeycodes(Class<?> clazz)
    {
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "InitKeycodes: " + clazz.getName());
        for (Field f : KeyCodes.class.getFields())
        {
            try
            {
                f.set(null, clazz.getField(f.getName()).get(null));
            } catch (Exception ignored) {
                try // else setup generic key codes
                {
                    f.set(null, KeyCodesGeneric.class.getField(f.getName()).get(null));
                } catch (Exception ignored2) { }
            }
        }
    }

    public static int GetRealKeyCode(int keycodeGeneric)
    {
        Field[] fields = KeyCodesGeneric.class.getFields();
        for (Field field : fields)
        {
            try
            {
                int key = (Integer) field.get(null);
                if(key == keycodeGeneric)
                {
                    String name = field.getName();
                    Field f = KeyCodes.class.getField(name);
                    //Log.e("TAG", "GetRealKeyCode: " + name + " : " + keycodeGeneric + " -> " + f.get(null));
                    return (Integer) f.get(null);
                }
            } catch (Exception ignored) {}
        }
        return keycodeGeneric;
    }

    public static int[] GetRealKeyCodes(int[] keycodeGeneric)
    {
        int[] codes = new int[keycodeGeneric.length];
        for (int i = 0; i < keycodeGeneric.length; i++)
            codes[i] = Q3EKeyCodes.GetRealKeyCode(keycodeGeneric[i]);
        return codes;
    }

    public static void ConvertRealKeyCodes(int[] codes)
    {
        for (int i = 0; i < codes.length; i++)
            codes[i] = Q3EKeyCodes.GetRealKeyCode(codes[i]);
    }


    public static int convertKeyCode(int keyCode, KeyEvent event)
    {
        switch (keyCode)
        {
            case KeyEvent.KEYCODE_FOCUS:
                return KeyCodes.K_F1;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                return KeyCodes.K_F2;
            case KeyEvent.KEYCODE_VOLUME_UP:
                return KeyCodes.K_F3;
            case KeyEvent.KEYCODE_DPAD_UP:
                return KeyCodes.K_UPARROW;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                return KeyCodes.K_DOWNARROW;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                return KeyCodes.K_LEFTARROW;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                return KeyCodes.K_RIGHTARROW;
            case KeyEvent.KEYCODE_DPAD_CENTER:
                return KeyCodes.K_CTRL;
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
            case KeyEvent.KEYCODE_BUTTON_A:
                return 'c';
            case KeyEvent.KEYCODE_BUTTON_B:
                return 'r';
            case KeyEvent.KEYCODE_BUTTON_X:
                if (Q3EUtils.isOuya) return KeyCodes.K_ENTER;//No enter button on ouya
                return KeyCodes.K_SPACE;//Why not?
            case KeyEvent.KEYCODE_BUTTON_Y:
                return 'f';//RTCW use
            //These buttons are not so popular
            case KeyEvent.KEYCODE_BUTTON_C:
                return 'a';//That's why here is a, nobody cares.
            case KeyEvent.KEYCODE_BUTTON_Z:
                return 'z';
            //--------------------------------
            case KeyEvent.KEYCODE_BUTTON_START:
                return KeyCodes.K_ESCAPE;
            case KeyEvent.KEYCODE_BUTTON_SELECT:
                return KeyCodes.K_ENTER;
            case KeyEvent.KEYCODE_MENU:
                if (Q3EUtils.isOuya) return KeyCodes.K_ESCAPE;
                break;
            case KeyEvent.KEYCODE_BUTTON_L2:
                return KeyCodes.K_MWHEELDOWN;
            case KeyEvent.KEYCODE_BUTTON_R2:
                return KeyCodes.K_MWHEELUP;
            case KeyEvent.KEYCODE_BUTTON_R1:
                return KeyCodes.K_MOUSE1;//Sometimes it is necessary
            case KeyEvent.KEYCODE_BUTTON_L1:
                if (Q3EUtils.isOuya) return KeyCodes.K_SPACE;
                return 'l';//dunno why
            case Q3EOuya.BUTTON_L3:
                return '[';
            case Q3EOuya.BUTTON_R3:
                return ']';

        }
        int uchar = event.getUnicodeChar(0);
        if ((uchar < 127) && (uchar != 0))
            return uchar;
        return keyCode % 95 + 32;//Magic
    }

    public static class KeyCodesGeneric
    {
        public static final int K_MOUSE1 = 187;
        public static final int K_MOUSE2 = 188;
        public static final int K_MOUSE3 = 189;
        public static final int K_MOUSE4 = 190;
        public static final int K_MOUSE5 = 191;
        public static final int K_MWHEELUP = 195;
        public static final int K_MWHEELDOWN = 196;

        public static final int K_A = 97;
        public static final int K_B = 98;
        public static final int K_C = 99;
        public static final int K_D = 100;
        public static final int K_E = 101;
        public static final int K_F = 102;
        public static final int K_G = 103;
        public static final int K_H = 104;
        public static final int K_I = 105;
        public static final int K_J = 106;
        public static final int K_K = 107;
        public static final int K_L = 108;
        public static final int K_M = 109;
        public static final int K_N = 110;
        public static final int K_O = 111;
        public static final int K_P = 112;
        public static final int K_Q = 113;
        public static final int K_R = 114;
        public static final int K_S = 115;
        public static final int K_T = 116;
        public static final int K_U = 117;
        public static final int K_V = 118;
        public static final int K_W = 119;
        public static final int K_X = 120;
        public static final int K_Y = 121;
        public static final int K_Z = 122;

        public static final int K_0 = 48;
        public static final int K_1 = 49;
        public static final int K_2 = 50;
        public static final int K_3 = 51;
        public static final int K_4 = 52;
        public static final int K_5 = 53;
        public static final int K_6 = 54;
        public static final int K_7 = 55;
        public static final int K_8 = 56;
        public static final int K_9 = 57;

        public static final int K_F1 = 149;
        public static final int K_F2 = 150;
        public static final int K_F3 = 151;
        public static final int K_F4 = 152;
        public static final int K_F5 = 153;
        public static final int K_F6 = 154;
        public static final int K_F7 = 155;
        public static final int K_F8 = 156;
        public static final int K_F9 = 157;
        public static final int K_F10 = 158;
        public static final int K_F11 = 159;
        public static final int K_F12 = 160;

        public static final int K_BACKSPACE = 127;
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_SHIFT = 142;
        public static final int K_CTRL = 141;
        public static final int K_ALT = 140;
        public static final int K_CAPSLOCK = 129;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;
        public static final int K_PGUP = 146;
        public static final int K_PGDN = 145;
        public static final int K_END = 148;
        public static final int K_HOME = 147;
        public static final int K_LEFTARROW = 135;
        public static final int K_UPARROW = 133;
        public static final int K_RIGHTARROW = 136;
        public static final int K_DOWNARROW = 134;
        public static final int K_INS = 143;
        public static final int K_DEL = 144;

        public static final int K_SEMICOLON = 59;
        public static final int K_EQUALS = 61;
        public static final int K_COMMA = 44;
        public static final int K_MINUS = 45;
        public static final int K_PERIOD = 46;
        public static final int K_SLASH = 47;
        public static final int K_GRAVE = 96;
        public static final int K_LBRACKET = 91;
        public static final int K_BACKSLASH = 92;
        public static final int K_RBRACKET = 93;
        public static final int K_APOSTROPHE = 39;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    public static final String K_WEAPONS_STR = "1,2,3,4,5,6,7,8,9,q,0";
}
