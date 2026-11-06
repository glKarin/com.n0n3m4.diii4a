
// fog
GLSL_SHADER const char ES3_FOG_FRAG[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"in vec2 var_TexFog;            // input Fog TexCoord\n"
"in vec2 var_TexFogEnter;       // input FogEnter TexCoord\n"
"\n"
"uniform sampler2D u_fragmentMap0;   // Fog Image\n"
"uniform sampler2D u_fragmentMap1;   // Fog Enter Image\n"
"uniform lowp vec4 u_fogColor;       // Fog Color\n"
"out vec4 _gl_FragColor;\n"
"\n"
"void main(void)\n"
"{\n"
"    _gl_FragColor = texture( u_fragmentMap0, var_TexFog ) * texture( u_fragmentMap1, var_TexFogEnter ) * vec4(u_fogColor.rgb, 1.0);\n"
"}\n"
;
