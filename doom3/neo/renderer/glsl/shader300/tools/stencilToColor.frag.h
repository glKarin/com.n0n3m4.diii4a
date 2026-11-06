
// stencil to color shader
GLSL_SHADER const char ES3_STENCIL_TO_COLOR_FRAG[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"uniform mediump usampler2D u_fragmentMap0; // stencil index\n"
"\n"
"uniform int u_uniformParm0; // 0=RGB, 1=R, 2=G, 4=B\n"
"\n"
"in vec2 var_TexDiffuse;\n"
"out vec4 _gl_FragColor;\n"
"\n"
"void main(void)\n"
"{\n"
"    float index = float(texture(u_fragmentMap0, var_TexDiffuse).r) / 255.0;\n"
"    float r = bool(u_uniformParm0 & 1) || u_uniformParm0 == 0 ? index : 0.0;\n"
"    float g = bool(u_uniformParm0 & 2) || u_uniformParm0 == 0 ? index : 0.0;\n"
"    float b = bool(u_uniformParm0 & 4) || u_uniformParm0 == 0 ? index : 0.0;\n"
"    _gl_FragColor = vec4(r, g, b, 1.0);\n"
"}\n"
;
