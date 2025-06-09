/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef SE_INCL_KEYNAMES_H
#define SE_INCL_KEYNAMES_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// key defines for all keys

// reserved for 'no-key-pressed'
#define KID_NONE            0x00
                            
// numbers row              
#define KID_1               0x11
#define KID_2               0x12
#define KID_3               0x13
#define KID_4               0x14
#define KID_5               0x15
#define KID_6               0x16
#define KID_7               0x17
#define KID_8               0x18
#define KID_9               0x19
#define KID_0               0x1A
#define KID_MINUS           0x1B
#define KID_EQUALS          0x1C
                            
// 1st alpha row            
#define KID_Q               0x20
#define KID_W               0x21
#define KID_E               0x22
#define KID_R               0x23
#define KID_T               0x24
#define KID_Y               0x25
#define KID_U               0x26
#define KID_I               0x27
#define KID_O               0x28
#define KID_P               0x29
#define KID_LBRACKET        0x2A
#define KID_RBRACKET        0x2B
#define KID_BACKSLASH       0x2C
                            
// 2nd alpha row            
#define KID_A               0x30
#define KID_S               0x31
#define KID_D               0x32
#define KID_F               0x33
#define KID_G               0x34
#define KID_H               0x35
#define KID_J               0x36
#define KID_K               0x37
#define KID_L               0x38
#define KID_SEMICOLON       0x39
#define KID_APOSTROPHE      0x3A
                            
// 3rd alpha row            
#define KID_Z               0x40
#define KID_X               0x41
#define KID_C               0x42
#define KID_V               0x43
#define KID_B               0x44
#define KID_N               0x45
#define KID_M               0x46
#define KID_COMMA           0x47
#define KID_PERIOD          0x48
#define KID_SLASH           0x49
                            
// row with F-keys          
#define KID_F1              0x51
#define KID_F2              0x52
#define KID_F3              0x53
#define KID_F4              0x54
#define KID_F5              0x55
#define KID_F6              0x56
#define KID_F7              0x57
#define KID_F8              0x58
#define KID_F9              0x59
#define KID_F10             0x5A
#define KID_F11             0x5B
#define KID_F12             0x5C
                            
// extra keys               
#define KID_ESCAPE          0x60
#define KID_TILDE           0x61
#define KID_BACKSPACE       0x62
#define KID_TAB             0x63
#define KID_CAPSLOCK        0x64
#define KID_ENTER           0x65
#define KID_SPACE           0x66
                            
// modifier keys            
#define KID_LSHIFT          0x70
#define KID_RSHIFT          0x71
#define KID_LCONTROL        0x72
#define KID_RCONTROL        0x73
#define KID_LALT            0x74
#define KID_RALT            0x75
                            
// navigation keys          
#define KID_ARROWUP         0x80
#define KID_ARROWDOWN       0x81
#define KID_ARROWLEFT       0x82
#define KID_ARROWRIGHT      0x83
#define KID_INSERT          0x84
#define KID_DELETE          0x85
#define KID_HOME            0x86
#define KID_END             0x87
#define KID_PAGEUP          0x88
#define KID_PAGEDOWN        0x89
#define KID_PRINTSCR        0x8A
#define KID_SCROLLLOCK      0x8B
#define KID_PAUSE           0x8C
                            
// numpad numbers           
#define KID_NUM0            0x90
#define KID_NUM1            0x91
#define KID_NUM2            0x92
#define KID_NUM3            0x93
#define KID_NUM4            0x94
#define KID_NUM5            0x95
#define KID_NUM6            0x96
#define KID_NUM7            0x97
#define KID_NUM8            0x98
#define KID_NUM9            0x99
#define KID_NUMDECIMAL      0x9A
                            
// numpad gray keys         
#define KID_NUMLOCK         0xA0
#define KID_NUMSLASH        0xA1
#define KID_NUMMULTIPLY     0xA2
#define KID_NUMMINUS        0xA3
#define KID_NUMPLUS         0xA4
#define KID_NUMENTER        0xA5

// mouse buttons
#define KID_MOUSE1          0xC0
#define KID_MOUSE2          0xC1
#define KID_MOUSE3          0xC2
#define KID_MOUSE4          0xC3
#define KID_MOUSE5          0xC4
#define KID_MOUSEWHEELUP    0xC5
#define KID_MOUSEWHEELDOWN  0xC6

// 2nd mouse buttons
#define KID_2MOUSE1         0xD0
#define KID_2MOUSE2         0xD1
#define KID_2MOUSE3         0xD2


#endif  /* include-once check. */

