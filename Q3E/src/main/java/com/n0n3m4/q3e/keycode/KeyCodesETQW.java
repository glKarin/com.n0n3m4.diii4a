package com.n0n3m4.q3e.keycode;

// ETQW
// sys/keynum.h
public final class KeyCodesETQW
{
    // A-Z is a(0x61)-z
    public static final int K_TAB    = 0x09;
    public static final int K_ENTER  = 0x0D;
    public static final int K_ESCAPE = 0x1B;
    public static final int K_SPACE  = 0x20;

    public static final int K_EXCLAMATION = 0x21;
    public static final int K_HASH = 0x23;
    public static final int K_DOLLAR = 0x24;
    public static final int K_AMPERSAND = 0x26;
    public static final int K_APOSTROPHE = 0x27;
    public static final int K_LEFTPARENTHESIS = 0x28;
    public static final int K_RIGHTPARENTHESIS = 0x29;
    public static final int K_ASTERISK = 0x2A;
    public static final int K_PLUS = 0x2B;
    public static final int K_COMMA = 0x2C;
    public static final int K_MINUS = 0x2D;
    public static final int K_PERIOD = 0x2E;
    public static final int K_SLASH = 0x2F;
    
    public static final int K_SEMICOLON = 0x3B;
    public static final int K_EQUALS = 0x3D;

    public static final int K_BACKSLASH = 0x5C;

    public static final int K_BACKQUOTE = 0x60;

    public static final int K_BACKSPACE = 0x08;

    public static final int K_COMMAND  = 0x80;
    public static final int K_CAPSLOCK = 0x81;
    public static final int K_SCROLL   = 0x82;
    public static final int K_PAUSE    = 0x83;

    public static final int K_UPARROW    = 0x85;
    public static final int K_DOWNARROW  = 0x86;
    public static final int K_LEFTARROW  = 0x87;
    public static final int K_RIGHTARROW = 0x88;

    // The 3 windows keys
    public static final int K_LWIN = 0x89;
    public static final int K_RWIN = 0x8A;
    public static final int K_MENU = 0x8B;

    public static final int K_ALT   = 0x8C;
    public static final int K_CTRL  = 0x8D;
    public static final int K_SHIFT = 0x8E;
    public static final int K_INS   = 0x8F;
    public static final int K_DEL   = 0x90;
    public static final int K_PGDN  = 0x91;
    public static final int K_PGUP  = 0x92;
    public static final int K_HOME  = 0x93;
    public static final int K_END   = 0x94;

    public static final int K_F1                   = 0x95;
    public static final int K_F2                   = 0x96;
    public static final int K_F3                   = 0x97;
    public static final int K_F4                   = 0x98;
    public static final int K_F5                   = 0x99;
    public static final int K_F6                   = 0x9A;
    public static final int K_F7                   = 0x9B;
    public static final int K_F8                   = 0x9C;
    public static final int K_F9                   = 0x9D;
    public static final int K_F10                  = 0x9E;
    public static final int K_F11                  = 0x9F;
    public static final int K_F12                  = 0xA0;
    public static final int K_F13                  = 0xA1;
    public static final int K_F14                  = 0xA2;
    public static final int K_F15                  = 0xA3;
    public static final int K_F16                  = 0xA4;

    public static final int K_KP_HOME         = 0xA5;
    public static final int K_KP_UPARROW      = 0xA6;
    public static final int K_KP_PGUP         = 0xA7;
    public static final int K_KP_LEFTARROW    = 0xA8;
    public static final int K_KP_5            = 0xA9;
    public static final int K_KP_RIGHTARROW   = 0xAA;
    public static final int K_KP_END          = 0xAB;
    public static final int K_KP_DOWNARROW    = 0xAC;
    public static final int K_KP_PGDN         = 0xAD;
    public static final int K_KP_ENTER        = 0xAE;
    public static final int K_KP_INS          = 0xAF;
    public static final int K_KP_DEL          = 0xB0;
    public static final int K_KP_SLASH        = 0xB1;
    public static final int K_KP_MINUS        = 0xB2;
    public static final int K_KP_PLUS         = 0xB3;
    public static final int K_KP_NUMLOCK      = 0xB4;
    public static final int K_KP_STAR         = 0xB5;
    public static final int K_KP_EQUALS       = 0xB6;

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
    public static final int K_LBRACKET    = 0x5B;
    public static final int K_RBRACKET    = 0x5D;

    public static final int K_POWER    = K_PAUSE + 1;

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
