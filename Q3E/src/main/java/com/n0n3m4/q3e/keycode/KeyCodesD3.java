package com.n0n3m4.q3e.keycode;

import android.view.KeyEvent;

import com.n0n3m4.q3e.Q3EKeyCodes;

// DOOM3/Quake 4/Prey
// neo/framework/KeyInput.h
public final class KeyCodesD3
{
    public static final int K_TAB    = 9;
    public static final int K_ENTER  = 13;
    public static final int K_ESCAPE = 27;
    public static final int K_SPACE  = 32;

    public static final int K_BACKSPACE = 127;

    public static final int K_COMMAND  = 128;
    public static final int K_CAPSLOCK = 129;
    public static final int K_SCROLL   = 130;
    public static final int K_POWER    = 131;
    public static final int K_PAUSE    = 132;

    public static final int K_UPARROW    = 133;
    public static final int K_DOWNARROW  = 134;
    public static final int K_LEFTARROW  = 135;
    public static final int K_RIGHTARROW = 136;

    // The 3 windows keys
    public static final int K_LWIN = 137;
    public static final int K_RWIN = 138;
    public static final int K_MENU = 139;

    public static final int K_ALT   = 140;
    public static final int K_CTRL  = 141;
    public static final int K_SHIFT = 142;
    public static final int K_INS   = 143;
    public static final int K_DEL   = 144;
    public static final int K_PGDN  = 145;
    public static final int K_PGUP  = 146;
    public static final int K_HOME  = 147;
    public static final int K_END   = 148;

    public static final int K_F1                   = 149;
    public static final int K_F2                   = 150;
    public static final int K_F3                   = 151;
    public static final int K_F4                   = 152;
    public static final int K_F5                   = 153;
    public static final int K_F6                   = 154;
    public static final int K_F7                   = 155;
    public static final int K_F8                   = 156;
    public static final int K_F9                   = 157;
    public static final int K_F10                  = 158;
    public static final int K_F11                  = 159;
    public static final int K_F12                  = 160;
    public static final int K_INVERTED_EXCLAMATION = 161;    // upside down !
    public static final int K_F13                  = 162;
    public static final int K_F14                  = 163;
    public static final int K_F15                  = 164;

    public static final int K_KP_HOME         = 165;
    public static final int K_KP_UPARROW      = 166;
    public static final int K_KP_PGUP         = 167;
    public static final int K_KP_LEFTARROW    = 168;
    public static final int K_KP_5            = 169;
    public static final int K_KP_RIGHTARROW   = 170;
    public static final int K_KP_END          = 171;
    public static final int K_KP_DOWNARROW    = 172;
    public static final int K_KP_PGDN         = 173;
    public static final int K_KP_ENTER        = 174;
    public static final int K_KP_INS          = 175;
    public static final int K_KP_DEL          = 176;
    public static final int K_KP_SLASH        = 177;
    public static final int K_SUPERSCRIPT_TWO = 178;        // superscript 2
    public static final int K_KP_MINUS        = 179;
    public static final int K_ACUTE_ACCENT    = 180;            // accute accent
    public static final int K_KP_PLUS         = 181;
    public static final int K_KP_NUMLOCK      = 182;
    public static final int K_KP_STAR         = 183;
    public static final int K_KP_EQUALS       = 184;

    public static final int K_MASCULINE_ORDINATOR = 186;
    // K_MOUSE enums must be contiguous (no char codes in the middle)
    public static final int K_MOUSE1              = 187;
    public static final int K_MOUSE2              = 188;
    public static final int K_MOUSE3              = 189;
    public static final int K_MOUSE4              = 190;
    public static final int K_MOUSE5              = 191;
    public static final int K_MOUSE6              = 192;
    public static final int K_MOUSE7              = 193;
    public static final int K_MOUSE8              = 194;

    public static final int K_MWHEELDOWN = 195;
    public static final int K_MWHEELUP   = 196;

    public static final int K_KP_1 = K_KP_END;
    public static final int K_KP_2 = K_KP_DOWNARROW;
    public static final int K_KP_3 = K_KP_PGDN;
    public static final int K_KP_4 = K_KP_LEFTARROW;
    public static final int K_KP_6 = K_KP_RIGHTARROW;
    public static final int K_KP_7 = K_KP_HOME;
    public static final int K_KP_8 = K_KP_UPARROW;
    public static final int K_KP_9 = K_KP_PGUP;
    public static final int K_KP_0 = K_KP_INS;

    public static final int K_GRAVE      = KeyCodesGeneric.K_GRAVE;
    public static final int K_LBRACKET    = KeyCodesGeneric.K_LBRACKET;
    public static final int K_RBRACKET    = KeyCodesGeneric.K_RBRACKET;

    // GamePad
    public static final int J_BUTTON_A = 197;
    public static final int J_BUTTON_B = 198;
    public static final int J_BUTTON_X = 199;
    public static final int J_BUTTON_Y = 200;
    public static final int J_BUTTON_SELECT = 201;
    public static final int J_BUTTON_START = 203;
    public static final int J_BUTTON_L3 = 204;
    public static final int J_BUTTON_R3 = 205;
    public static final int J_BUTTON_L1 = 206;
    public static final int J_BUTTON_R1 = 207;
    public static final int J_DPAD_UP = 208;
    public static final int J_DPAD_DOWN = 209;
    public static final int J_DPAD_LEFT = 210;
    public static final int J_DPAD_RIGHT = 211;
    public static final int J_DPAD_CENTER = J_BUTTON_A;
    public static final int J_BUTTON_L2 = 212;
    public static final int J_BUTTON_R2 = 213;
    public static final int J_BUTTON_C = 214;
    public static final int J_BUTTON_Z = 215;
    public static final int J_BUTTON_1 = 217;
    public static final int J_BUTTON_2 = 218;
    public static final int J_BUTTON_3 = 219;
    public static final int J_BUTTON_4 = 220;
    public static final int J_BUTTON_5 = 221;
    public static final int J_BUTTON_6 = 222;
    public static final int J_BUTTON_7 = 223;
    public static final int J_BUTTON_8 = 224;
    public static final int J_BUTTON_9 = 225;
    public static final int J_BUTTON_10 = 226;
    public static final int J_BUTTON_11 = 227;
    public static final int J_BUTTON_12 = 228;
    public static final int J_BUTTON_13 = 229;
    public static final int J_BUTTON_14 = 202;
    public static final int J_BUTTON_15 = 216;
    public static final int J_BUTTON_16 = 233;
}
