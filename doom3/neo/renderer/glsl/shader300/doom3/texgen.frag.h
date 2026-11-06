
// texgen(Only used in D3XP)
GLSL_SHADER const char ES3_TEXGEN_FRAG[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"uniform sampler2D u_fragmentMap0;\n"
"uniform lowp vec4 u_glColor;\n"
"\n"
"in vec4 var_TexCoord;\n"
"in lowp vec4 var_Color;\n"
"out vec4 _gl_FragColor;\n"
"\n"
"void main(void)\n"
"{\n"
"    // we always do a projective texture lookup so that we can support texgen\n"
"    // materials without a separate shader. Basic materials will have texture\n"
"    // coordinates with w = 1 which will result in a NOP projection when tex2Dproj\n"
"    // gets called.\n"
"    _gl_FragColor = textureProj( u_fragmentMap0, var_TexCoord.xyw ) * u_glColor * var_Color;\n"
"}\n"
;
