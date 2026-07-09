
#define _ES3_RETRO_POSTPROCESS_GENERAL_FUNCTION \
"#define RGB(r, g, b) vec3(float(r)/255.0, float(g)/255.0, float(b)/255.0)\n" \
"\n" \
"// array/table version from http://www.anisopteragames.com/how-to-fix-color-banding-with-dithering/\n" \
"const int ArrayDitherArray8x8[64] = int[64] (\n" \
"    0, 32,  8, 40,  2, 34, 10, 42,   /* 8x8 Bayer ordered dithering  */\n" \
"    48, 16, 56, 24, 50, 18, 58, 26,  /* pattern.  Each input pixel   */\n" \
"    12, 44,  4, 36, 14, 46,  6, 38,  /* is scaled to the 0..63 range */\n" \
"    60, 28, 52, 20, 62, 30, 54, 22,  /* before looking in this table */\n" \
"    3, 35, 11, 43,  1, 33,  9, 41,   /* to determine the action.     */\n" \
"    51, 19, 59, 27, 49, 17, 57, 25,\n" \
"    15, 47,  7, 39, 13, 45,  5, 37,\n" \
"    63, 31, 55, 23, 61, 29, 53, 21\n" \
");\n" \
"\n" \
"float DitherArray8x8( vec2 pos )\n" \
"{\n" \
"    int stippleOffset = ( int( pos.y ) % 8 ) * 8 + ( int( pos.x ) % 8 );\n" \
"    int byte = ArrayDitherArray8x8[stippleOffset];\n" \
"    float stippleThreshold = float(byte) / 64.0f;\n" \
"    return stippleThreshold;\n" \
"}\n"
