
// colorProcess
GLSL_SHADER const char COLORPROCESS_FRAG[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"uniform sampler2D u_fragmentMap0;\n"
"uniform highp vec4 u_nonPowerOfTwo;\n"
"\n"
"varying highp vec4 var_TexCoord;\n"
"varying lowp vec4 var_Color;\n"
"\n"
"void main(void)\n"
"{\n"
"    vec4 src = texture2D( u_fragmentMap0, var_TexCoord.xy * u_nonPowerOfTwo.xy /* scale by the screen non-power-of-two-adjust */ );\n"
"    vec4 target = var_Color * dot( vec3( 0.333, 0.333, 0.333 ), src.xyz );\n"
"    gl_FragColor = mix( src, target, var_TexCoord.z );\n"
"}\n"
;
