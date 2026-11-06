
// z-fill
GLSL_SHADER const char ZFILL_VERT[] = 
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"attribute highp vec4 attr_Vertex;\n"
"attribute vec4 attr_TexCoord;\n"
"uniform mat4 u_textureMatrix;\n"
"\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"varying vec2 var_TexDiffuse;\n"
"\n"
"void main(void)\n"
"{\n"
"    var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;\n"
"\n"
"    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;\n"
"}\n"
;
