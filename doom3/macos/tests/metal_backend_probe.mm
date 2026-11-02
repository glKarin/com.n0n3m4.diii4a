#include <SDL.h>
#include <GLES3/gl3.h>
#include "metal_backend.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>

template <typename T> T proc(const char *s) {
    auto p = MetalBackend_Proc(s);
    if (!p) {
        fprintf(stderr, "missing %s\n", s);
        exit(1);
    }
    return (T)p;
}
#define GL(name) proc<decltype(&gl##name)>("gl" #name)
int main() {
    SDL_Init(SDL_INIT_VIDEO);
    auto w = SDL_CreateWindow("ETQW Metal regression", 0, 0, 64, 64, SDL_WINDOW_METAL | SDL_WINDOW_HIDDEN);
    if (!MetalBackend_Init(w, 64, 64))
        return 1;
    // Invalid lookups must not turn the unbound program into an existing object.
    GLint unboundStatus = 0;
    GL(GetProgramiv)(0, GL_LINK_STATUS, &unboundStatus);
    if (GL(IsProgram)(0) || GL(GetUniformLocation)(0, "color") != -1 ||
        GL(GetAttribLocation)(0, "attr_Vertex") != -1)
        return 20;
    const char *v = "#version 300 es\nprecision highp float;in vec3 attr_Vertex;"
                    "attribute vec3 attr_Vertex;uniform mat4 u_modelViewProjectionMatrix;void "
                    "main(){gl_Position=gl_ModelViewProjectionMatrix*vec4(attr_Vertex,1);}";
    const char *f = "#version 300 es\nprecision highp float;uniform vec4 megaTextureParams_0;out vec4 result;void "
                    "main(){if(megatextureparams_0.a<.5)discard;result=megatextureparams_0;}";
    GLuint vs = GL(CreateShader)(GL_VERTEX_SHADER), fs = GL(CreateShader)(GL_FRAGMENT_SHADER),
           p = GL(CreateProgram)();
    GL(ShaderSource)(vs, 1, &v, nullptr);
    GL(ShaderSource)(fs, 1, &f, nullptr);
    GL(CompileShader)(vs);
    GL(CompileShader)(fs);
    GL(AttachShader)(p, vs);
    GL(AttachShader)(p, fs);
    GL(BindAttribLocation)(p, 0, "attr_Vertex");
    GL(LinkProgram)(p);
    GLint linked = 0;
    GL(GetProgramiv)(p, GL_LINK_STATUS, &linked);
    if (!linked)
        return 2;
    GL(UseProgram)(p);
    const float retailMatrix[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    GL(UniformMatrix4fv)(GL(GetUniformLocation)(p, "u_modelViewProjectionMatrix"), 1, 0, retailMatrix);
    auto color = GL(GetUniformLocation)(p, "megaTextureParams_0");
    GLuint buffer;
    GL(GenBuffers)(1, &buffer);
    GL(BindBuffer)(GL_ARRAY_BUFFER, buffer);
    float tri[] = {-1, -1, 0, 1, -1, 0, 0, 1, 0};
    GL(BufferData)(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
    GL(VertexAttribPointer)(0, 3, GL_FLOAT, 0, 12, nullptr);
    GL(EnableVertexAttribArray)(0);
    GL(Viewport)(0, 0, 64, 64);
    GL(Disable)(GL_CULL_FACE);
    GL(Enable)(GL_DEPTH_TEST);
    GL(DepthFunc)(GL_LESS);
    GL(ClearColor)(0, 0, 1, 1);
    GL(ClearDepthf)(1);
    GL(Clear)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL(Uniform4f)(color, 1, 0, 0, 1);
    GL(DrawArrays)(GL_TRIANGLES, 0, 3);
    // A deeper triangle must not overwrite red. It also exercises buffer orphaning.
    tri[2] = tri[5] = tri[8] = .8;
    GL(BufferData)(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
    GL(Uniform4f)(color, 0, 1, 0, 1);
    GL(DrawArrays)(GL_TRIANGLES, 0, 3);
    // Discard must preserve both color and depth.
    tri[2] = tri[5] = tri[8] = -.8;
    GL(BufferData)(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
    GL(Uniform4f)(color, 0, 1, 0, .1);
    GL(DrawArrays)(GL_TRIANGLES, 0, 3);
    unsigned char pixel[4];
    GL(ReadPixels)(32, 24, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    printf("center RGBA=%u,%u,%u,%u\n", pixel[0], pixel[1], pixel[2], pixel[3]);
    bool ok = pixel[0] > 250 && pixel[1] < 5 && pixel[2] < 5 && GL(GetError)() == GL_NO_ERROR;
    GL(ReadPixels)(0, 63, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok &= pixel[2] > 250 && pixel[0] < 5;
    // Match the game's interleaved GUI vertex, attribute locations and matrix uniforms.
    const char *tv =
        "#version 300 es\nprecision highp float;uniform mat4 mvp;uniform mat4 texmat;in vec3 attr_Vertex;in "
        "vec2 attr_TexCoord;in vec4 attr_Color;out vec2 uv;out vec4 col;void "
        "main(){gl_Position=mvp*vec4(attr_Vertex,1);uv=(texmat*vec4(attr_TexCoord,0,1)).xy;col=attr_Color;}";
    const char *tf = "#version 300 es\nprecision highp float;uniform sampler2D image;in vec2 uv;in vec4 "
                     "col;out vec4 result;void main(){result=texture(image,uv)*col;}";
    auto t = GL(CreateProgram)(), v2 = GL(CreateShader)(GL_VERTEX_SHADER),
         f2 = GL(CreateShader)(GL_FRAGMENT_SHADER);
    GL(ShaderSource)(v2, 1, &tv, nullptr);
    GL(ShaderSource)(f2, 1, &tf, nullptr);
    GL(CompileShader)(v2);
    GL(CompileShader)(f2);
    GL(AttachShader)(t, v2);
    GL(AttachShader)(t, f2);
    GL(BindAttribLocation)(t, 12, "attr_Vertex");
    GL(BindAttribLocation)(t, 8, "attr_TexCoord");
    GL(BindAttribLocation)(t, 13, "attr_Color");
    GL(LinkProgram)(t);
    GL(GetProgramiv)(t, GL_LINK_STATUS, &linked);
    if (!linked)
        return 4;
    GL(UseProgram)(t);
    float identity[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    float projection[] = {2.0f / 64, 0, 0, 0, 0, 2.0f / 64, 0, 0, 0, 0, 1, 0, -1, -1, 0, 1};
    GL(UniformMatrix4fv)(GL(GetUniformLocation)(t, "mvp"), 1, 0, projection);
    GL(UniformMatrix4fv)(GL(GetUniformLocation)(t, "texmat"), 1, 0, identity);
    struct Vertex {
        float xyz[3], uv[2];
        unsigned char color[4];
    };
    Vertex quad[] = {{{0, 0, 0}, {0, 0}, {128, 255, 255, 255}},
                     {{64, 0, 0}, {1, 0}, {128, 255, 255, 255}},
                     {{64, 64, 0}, {1, 1}, {128, 255, 255, 255}},
                     {{0, 64, 0}, {0, 1}, {128, 255, 255, 255}}};
    GL(BufferData)(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    GL(VertexAttribPointer)(12, 3, GL_FLOAT, 0, sizeof(Vertex), (void *)offsetof(Vertex, xyz));
    GL(EnableVertexAttribArray)(12);
    GL(VertexAttribPointer)(8, 2, GL_FLOAT, 0, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    GL(EnableVertexAttribArray)(8);
    GL(VertexAttribPointer)(13, 4, GL_UNSIGNED_BYTE, 1, sizeof(Vertex), (void *)offsetof(Vertex, color));
    GL(EnableVertexAttribArray)(13);
    GLuint tex;
    GL(GenTextures)(1, &tex);
    GL(ActiveTexture)(GL_TEXTURE3);
    GL(BindTexture)(GL_TEXTURE_2D, tex);
    unsigned char checker[] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    GL(TexImage2D)(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, checker);
    GL(TexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL(TexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GL(Uniform1i)(GL(GetUniformLocation)(t, "image"), 3);
    GL(Disable)(GL_DEPTH_TEST);
    GL(Enable)(GL_CULL_FACE);
    GL(CullFace)(GL_BACK);
    GL(FrontFace)(GL_CCW);
    unsigned short ids[] = {0, 1, 2, 0, 2, 3};
    GL(DrawElements)(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, ids);
    const unsigned char expected[4][3] = {{128, 0, 0}, {0, 255, 0}, {0, 0, 255}, {128, 255, 255}};
    for (int i = 0; i < 4; ++i) {
        GL(ReadPixels)(16 + (i % 2) * 32, 16 + (i / 2) * 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        printf("textured quadrant %d RGBA=%u,%u,%u,%u\n", i, pixel[0], pixel[1], pixel[2], pixel[3]);
        for (int c = 0; c < 3; ++c)
            ok &= abs(int(pixel[c]) - expected[i][c]) < 3;
    }
    // Constant BC5 XY normal: Z must be reconstructed, not sampled as zero.
    unsigned char bc5[16] = {128, 128, 0, 0, 0, 0, 0, 0, 128, 128, 0, 0, 0, 0, 0, 0};
    GL(CompressedTexImage2D)(GL_TEXTURE_2D, 0, 0x8dbd, 4, 4, 0, sizeof(bc5), bc5);
    GL(DrawElements)(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, ids);
    GL(ReadPixels)(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    printf("BC5 normal RGBA=%u,%u,%u,%u\n", pixel[0], pixel[1], pixel[2], pixel[3]);
    ok &= abs(int(pixel[0]) - 64) < 3 && abs(int(pixel[1]) - 128) < 3 && pixel[2] > 250 &&
          abs(int(pixel[3]) - 128) < 3;
    // A scissored, color-masked clear must not erase other channels or pixels.
    GL(Enable)(GL_SCISSOR_TEST);
    GL(Scissor)(0, 0, 16, 16);
    GL(ColorMask)(1, 0, 0, 0);
    GL(ClearColor)(1, 0, 0, 0);
    GL(Clear)(GL_COLOR_BUFFER_BIT);
    GL(ReadPixels)(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok &= pixel[0] > 250 && abs(int(pixel[1]) - 128) < 3 && pixel[2] > 250;
    GL(ReadPixels)(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok &= abs(int(pixel[0]) - 64) < 3;
    ok &= GL(GetError)() == GL_NO_ERROR;
    MetalBackend_Info();
    MetalBackend_Shutdown();
    SDL_DestroyWindow(w);
    SDL_Quit();
    printf("%s: native Metal triangle, depth occlusion, alpha discard, buffer lifetime and readback\n",
           ok ? "PASS" : "FAIL");
    return ok ? 0 : 3;
}
