
// z-fill perforated depth
GLSL_SHADER const char DEPTH_PERFORATED_VERT[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"attribute vec4 attr_TexCoord;\n"
"attribute highp vec4 attr_Vertex;\n"
"\n"
"uniform mat4 u_textureMatrix;\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"varying vec2 var_TexDiffuse;\n"
"\n"
"void main(void)\n"
"{\n"
"    var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;\n"
"\n"
"    gl_Position = attr_Vertex * u_modelViewProjectionMatrix;\n"
"}\n"
;
