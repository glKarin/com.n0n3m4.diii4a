
// fog
GLSL_SHADER const char FOG_FRAG[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"varying vec2 var_TexFog;            // input Fog TexCoord\n"
"varying vec2 var_TexFogEnter;       // input FogEnter TexCoord\n"
"\n"
"uniform sampler2D u_fragmentMap0;   // Fog Image\n"
"uniform sampler2D u_fragmentMap1;   // Fog Enter Image\n"
"uniform lowp vec4 u_fogColor;       // Fog Color\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = texture2D( u_fragmentMap0, var_TexFog ) * texture2D( u_fragmentMap1, var_TexFogEnter ) * vec4(u_fogColor.rgb, 1.0);\n"
"}\n"
;
