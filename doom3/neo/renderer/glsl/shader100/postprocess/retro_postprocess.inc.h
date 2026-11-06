
#define _RETRO_POSTPROCESS_GENERAL_ARRAY \
"    ArrayDitherArray8x8[0] = 0;\n" \
"    ArrayDitherArray8x8[1] = 32;\n" \
"    ArrayDitherArray8x8[2] = 8;\n" \
"    ArrayDitherArray8x8[3] = 40;\n" \
"    ArrayDitherArray8x8[4] = 2;\n" \
"    ArrayDitherArray8x8[5] = 34;\n" \
"    ArrayDitherArray8x8[6] = 10;\n" \
"    ArrayDitherArray8x8[7] = 42;\n" \
"    ArrayDitherArray8x8[8] = 48;\n" \
"    ArrayDitherArray8x8[9] = 16;\n" \
"    ArrayDitherArray8x8[10] = 56;\n" \
"    ArrayDitherArray8x8[11] = 24;\n" \
"    ArrayDitherArray8x8[12] = 50;\n" \
"    ArrayDitherArray8x8[13] = 18;\n" \
"    ArrayDitherArray8x8[14] = 58;\n" \
"    ArrayDitherArray8x8[15] = 26;\n" \
"    ArrayDitherArray8x8[16] = 12;\n" \
"    ArrayDitherArray8x8[17] = 44;\n" \
"    ArrayDitherArray8x8[18] = 4;\n" \
"    ArrayDitherArray8x8[19] = 36;\n" \
"    ArrayDitherArray8x8[20] = 14;\n" \
"    ArrayDitherArray8x8[21] = 46;\n" \
"    ArrayDitherArray8x8[22] = 6;\n" \
"    ArrayDitherArray8x8[23] = 38;\n" \
"    ArrayDitherArray8x8[24] = 60;\n" \
"    ArrayDitherArray8x8[25] = 28;\n" \
"    ArrayDitherArray8x8[26] = 52;\n" \
"    ArrayDitherArray8x8[27] = 20;\n" \
"    ArrayDitherArray8x8[28] = 62;\n" \
"    ArrayDitherArray8x8[29] = 30;\n" \
"    ArrayDitherArray8x8[30] = 54;\n" \
"    ArrayDitherArray8x8[31] = 22;\n" \
"    ArrayDitherArray8x8[32] = 3;\n" \
"    ArrayDitherArray8x8[33] = 35;\n" \
"    ArrayDitherArray8x8[34] = 11;\n" \
"    ArrayDitherArray8x8[35] = 43;\n" \
"    ArrayDitherArray8x8[36] = 1;\n" \
"    ArrayDitherArray8x8[37] = 33;\n" \
"    ArrayDitherArray8x8[38] = 9;\n" \
"    ArrayDitherArray8x8[39] = 41;\n" \
"    ArrayDitherArray8x8[40] = 51;\n" \
"    ArrayDitherArray8x8[41] = 19;\n" \
"    ArrayDitherArray8x8[42] = 59;\n" \
"    ArrayDitherArray8x8[43] = 27;\n" \
"    ArrayDitherArray8x8[44] = 49;\n" \
"    ArrayDitherArray8x8[45] = 17;\n" \
"    ArrayDitherArray8x8[46] = 57;\n" \
"    ArrayDitherArray8x8[47] = 25;\n" \
"    ArrayDitherArray8x8[48] = 15;\n" \
"    ArrayDitherArray8x8[49] = 47;\n" \
"    ArrayDitherArray8x8[50] = 7;\n" \
"    ArrayDitherArray8x8[51] = 39;\n" \
"    ArrayDitherArray8x8[52] = 13;\n" \
"    ArrayDitherArray8x8[53] = 45;\n" \
"    ArrayDitherArray8x8[54] = 5;\n" \
"    ArrayDitherArray8x8[55] = 37;\n" \
"    ArrayDitherArray8x8[56] = 63;\n" \
"    ArrayDitherArray8x8[57] = 31;\n" \
"    ArrayDitherArray8x8[58] = 55;\n" \
"    ArrayDitherArray8x8[59] = 23;\n" \
"    ArrayDitherArray8x8[60] = 61;\n" \
"    ArrayDitherArray8x8[61] = 29;\n" \
"    ArrayDitherArray8x8[62] = 53;\n" \
"    ArrayDitherArray8x8[63] = 21;\n"

#define _RETRO_POSTPROCESS_GENERAL_FUNCTION \
"#define RGB(r, g, b) vec3(float(r)/255.0, float(g)/255.0, float(b)/255.0)\n" \
"\n" \
"// array/table version from http://www.anisopteragames.com/how-to-fix-color-banding-with-dithering/\n" \
"int ArrayDitherArray8x8[64];\n" \
"\n" \
"int glsl100_mod( int a, int b )\n" \
"{\n" \
"    return a - (a / b) * b;\n" \
"}\n" \
"\n" \
"float DitherArray8x8( vec2 pos )\n" \
"{\n" \
"/* 8x8 Bayer ordered dithering  */\n" \
"/* pattern.  Each input pixel   */\n" \
"/* is scaled to the 0..63 range */\n" \
"/* before looking in this table */\n" \
"/* to determine the action.     */\n" \
"\n" \
"    int stippleOffset = glsl100_mod( int( pos.y ), 8 ) * 8 + glsl100_mod( int( pos.x ), 8 );\n" \
"    int byte = ArrayDitherArray8x8[stippleOffset];\n" \
"    float stippleThreshold = float(byte) / 64.0f;\n" \
"    return stippleThreshold;\n" \
"}\n"
