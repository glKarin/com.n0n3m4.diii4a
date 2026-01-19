package com.n0n3m4.q3e.keycode;

/*
* generic key code integer -> KeyCodesGeneric's field name -> KeyCodes's field key code value
if(KeyCodesGeneric.K_SOME == KeyCodesGeneric.getFields()[some].get())
{
    return KeyCodes.getField(KeyCodesGeneric.getFields()[some].getName()).get()
}
 */
public final class KeyCodesGeneric
{
    public static final int K_MOUSE1     = 187;
    public static final int K_MOUSE2     = 188;
    public static final int K_MOUSE3     = 189;
    public static final int K_MOUSE4     = 190;
    public static final int K_MOUSE5     = 191;
    public static final int K_MWHEELUP   = 195;
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

    public static final int K_F1  = 149;
    public static final int K_F2  = 150;
    public static final int K_F3  = 151;
    public static final int K_F4  = 152;
    public static final int K_F5  = 153;
    public static final int K_F6  = 154;
    public static final int K_F7  = 155;
    public static final int K_F8  = 156;
    public static final int K_F9  = 157;
    public static final int K_F10 = 158;
    public static final int K_F11 = 159;
    public static final int K_F12 = 160;

    public static final int K_BACKSPACE  = 127;
    public static final int K_TAB        = 9;
    public static final int K_ENTER      = 13;
    public static final int K_SHIFT      = 142;
    public static final int K_CTRL       = 141;
    public static final int K_ALT        = 140;
    public static final int K_CAPSLOCK   = 129;
    public static final int K_ESCAPE     = 27;
    public static final int K_SPACE      = 32;
    public static final int K_PGUP       = 146;
    public static final int K_PGDN       = 145;
    public static final int K_END        = 148;
    public static final int K_HOME       = 147;
    public static final int K_LEFTARROW  = 135;
    public static final int K_UPARROW    = 133;
    public static final int K_RIGHTARROW = 136;
    public static final int K_DOWNARROW  = 134;
    public static final int K_INS        = 143;
    public static final int K_DEL        = 144;

    public static final int K_SEMICOLON  = 59;
    public static final int K_EQUALS     = 61;
    public static final int K_COMMA      = 44;
    public static final int K_MINUS      = 45;
    public static final int K_PERIOD     = 46;
    public static final int K_SLASH      = 47;
    public static final int K_GRAVE      = 96; // 192
    public static final int K_LBRACKET   = 91;
    public static final int K_BACKSLASH  = 92;
    public static final int K_RBRACKET   = 93;
    public static final int K_APOSTROPHE = 39;

    public static final int J_LEFT  = -'a';
    public static final int J_RIGHT = -'d';
    public static final int J_UP    = -K_UPARROW;
    public static final int J_DOWN  = -K_DOWNARROW;

    public static final int K_KP_1 = 171;
    public static final int K_KP_2 = 172;
    public static final int K_KP_3 = 173;
    public static final int K_KP_4 = 168;
    public static final int K_KP_5 = 169;
    public static final int K_KP_6 = 170;
    public static final int K_KP_7 = 165;
    public static final int K_KP_8 = 166;
    public static final int K_KP_9 = 167;
    public static final int K_KP_0 = 175;

    public static final boolean RAW = false;

    // GamePad
    private static final int J_BASE = 256;
    public static final int J_DPAD_UP = J_BASE + 11;
    public static final int J_DPAD_DOWN = J_BASE + 12;
    public static final int J_DPAD_LEFT = J_BASE + 13;
    public static final int J_DPAD_RIGHT = J_BASE + 14;
    public static final int J_DPAD_CENTER = J_BASE + 36;
    public static final int J_BUTTON_A = J_BASE;
    public static final int J_BUTTON_B = J_BASE + 1;
    public static final int J_BUTTON_C = J_BASE + 17;
    public static final int J_BUTTON_X = J_BASE + 2;
    public static final int J_BUTTON_Y = J_BASE + 3;
    public static final int J_BUTTON_Z = J_BASE + 18;
    public static final int J_BUTTON_1 = J_BASE + 20;
    public static final int J_BUTTON_2 = J_BASE + 21;
    public static final int J_BUTTON_3 = J_BASE + 22;
    public static final int J_BUTTON_4 = J_BASE + 23;
    public static final int J_BUTTON_5 = J_BASE + 24;
    public static final int J_BUTTON_6 = J_BASE + 25;
    public static final int J_BUTTON_7 = J_BASE + 26;
    public static final int J_BUTTON_8 = J_BASE + 27;
    public static final int J_BUTTON_9 = J_BASE + 28;
    public static final int J_BUTTON_10 = J_BASE + 29;
    public static final int J_BUTTON_11 = J_BASE + 30;
    public static final int J_BUTTON_12 = J_BASE + 31;
    public static final int J_BUTTON_13 = J_BASE + 32;
    public static final int J_BUTTON_14 = J_BASE + 33;
    public static final int J_BUTTON_15 = J_BASE + 34;
    public static final int J_BUTTON_16 = J_BASE + 35;
    public static final int J_BUTTON_START = J_BASE + 6;
    public static final int J_BUTTON_SELECT = J_BASE + 4;
    public static final int J_BUTTON_L1 = J_BASE + 9;
    public static final int J_BUTTON_R1 = J_BASE + 10;
    public static final int J_BUTTON_L2 = J_BASE + 15;
    public static final int J_BUTTON_R2 = J_BASE + 16;
    public static final int J_BUTTON_L3 = J_BASE + 7;
    public static final int J_BUTTON_R3 = J_BASE + 8;
}
