
// shadow
GLSL_SHADER const char ES3_SHADOW_VERT[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"in highp vec4 attr_Vertex;\n"
"\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"uniform lowp vec4 u_glColor;\n"
"uniform vec4 u_lightOrigin;\n"
"\n"
"out lowp vec4 var_Color;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_Position = u_modelViewProjectionMatrix * (attr_Vertex.w * u_lightOrigin + attr_Vertex - u_lightOrigin);\n"
"\n"
"    var_Color = u_glColor;\n"
"}\n"
;
