
// simple vertex/texcoord vertex shader
GLSL_SHADER const char SIMPLE_VERTEX_TEXCOORD_VERT[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"attribute vec4 attr_TexCoord;\n"
"attribute highp vec4 attr_Vertex;\n"
"\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"varying vec2 var_TexDiffuse;\n"
"\n"
"void main(void)\n"
"{\n"
"    var_TexDiffuse = attr_TexCoord.xy;\n"
"\n"
"    gl_Position = attr_Vertex * u_modelViewProjectionMatrix;\n"
"}\n"
;

// depth to color shader
GLSL_SHADER const char DEPTH_TO_COLOR_FRAG[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"uniform highp sampler2D u_fragmentMap0; // depth\n"
"\n"
"uniform bool u_uniformParm0; // not pack to RGBA\n"
"\n"
"varying vec2 var_TexDiffuse;\n"
"\n"
"vec4 pack (highp float depth)\n"
"{\n"
"    const highp vec4 bitSh = vec4(256.0 * 256.0 * 256.0, 256.0 * 256.0, 256.0, 1.0);\n"
"    const highp vec4 bitMsk = vec4(0.0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);\n"
"    highp vec4 comp = fract(depth * bitSh);\n"
"    comp -= comp.xxyz * bitMsk;\n"
"    return depth < 1.0 ? comp : vec4(1.0, 1.0, 1.0, 1.0);\n"
"}\n"
"\n"
"void main(void)\n"
"{\n"
"    highp vec4 depth = texture2D(u_fragmentMap0, var_TexDiffuse);\n"
"    vec4 packColor = pack(depth.r);\n"
"    vec4 color = vec4(depth.r);\n"
"    gl_FragColor = u_uniformParm0 ? color : packColor;\n"
"}\n"
;

// depth to color shader
GLSL_SHADER const char STENCIL_TO_COLOR_FRAG[] =
""
;
