// SPDX-License-Identifier: GPL-3.0-or-later
#include <vector>
#include "idlib/precompiled.h"
#include "renderer/tr_local.h"
#include "renderer/RenderWorld_local.h"
#include "metal_rt.h"
#include "metal_backend.h"

idCVar r_metalRT("r_metalRT", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL,
                 "Native Metal ray traced static-world sun shadows (Apple M3+)");
idCVar r_metalRTStrength("r_metalRTStrength", "0.35", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
                         "Additional visibility attenuation for ray traced sun shadows", 0.0f, 1.0f);
idCVar r_metalRTScale("r_metalRTScale", "0.25", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
                      "Ray tracing mask scale (vid_restart)", 0.125f, 1.0f);

namespace {
bool initialized = false, worldReady = false, failed = false;
GLuint program = 0, depthTexture = 0, depthFramebuffer = 0;
int renderWidth = 0, renderHeight = 0, maskWidth = 0, maskHeight = 0;
size_t triangles = 0;
unsigned long frames = 0;
double gpuMilliseconds = 0;

GLuint compile(GLenum type, const char *source) {
    GLuint shader = qglCreateShader(type);
    qglShaderSource(shader, 1, &source, NULL);
    qglCompileShader(shader);
    GLint ok;
    qglGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        qglGetShaderInfoLog(shader, sizeof(log), NULL, log);
        common->Warning("Metal RT composite: %s", log);
        qglDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool setup() {
    renderWidth = glConfig.vidWidth;
    renderHeight = glConfig.vidHeight;
    maskWidth = Max(1, int(renderWidth * r_metalRTScale.GetFloat()));
    maskHeight = Max(1, int(renderHeight * r_metalRTScale.GetFloat()));
    char error[4096];
    if (!ETQW_MetalRT_Init(maskWidth, maskHeight, error, sizeof(error))) {
        common->Warning("Metal RT unavailable: %s", error);
        return false;
    }
    const char *vs =
        "#version 300 es\nprecision highp float;\nvoid main(){vec2 "
        "p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.0-1.0,0,1);}";
    const char *fs = R"glsl(#version 300 es
precision highp float;
uniform sampler2D maskTex;
uniform highp sampler2D depthTex;
uniform vec4 projection;
uniform vec2 imageSize;
out vec4 color;
void main() {
    vec2 uv=gl_FragCoord.xy/imageSize;
    float d=texture(depthTex,uv).r;
    float distance=projection.y/(2.0*d-1.0+projection.x);
    vec2 slope=(2.0*uv-1.0)*projection.zw;
    distance*=sqrt(1.0+dot(slope,slope));
    vec2 size=vec2(textureSize(maskTex,0));
    vec2 pos=uv*size-.5, base=floor(pos), f=fract(pos);
    float visibility=0.0, weight=0.0;
    for(int y=0;y<2;y++) for(int x=0;x<2;x++) {
        vec4 m=texture(maskTex,(base+vec2(x,y)+.5)/size);
        float w=(x==0 ? 1.0-f.x : f.x)*(y==0 ? 1.0-f.y : f.y);
        // Reject silhouettes, alpha cards, moving objects and the depth-hacked weapon.
        w*=step(abs(m.y-distance),max(3.0,distance*.008))*step(.01,m.y);
        visibility+=m.x*w; weight+=w;
    }
    float value=(weight>.05 && d<.999999) ? visibility/weight : 1.0;
    color=vec4(vec3(value),1.0);
})glsl";
    GLuint vertex = compile(GL_VERTEX_SHADER, vs), fragment = compile(GL_FRAGMENT_SHADER, fs);
    if (!vertex || !fragment)
        return false;
    program = qglCreateProgram();
    qglAttachShader(program, vertex);
    qglAttachShader(program, fragment);
    qglLinkProgram(program);
    qglDeleteShader(vertex);
    qglDeleteShader(fragment);
    GLint ok;
    qglGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
        return false;
    GLint tex, read, draw;
    qglGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    qglGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    qglGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    qglGenTextures(1, &depthTexture);
    qglBindTexture(GL_TEXTURE_2D, depthTexture);
    qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, renderWidth, renderHeight, 0, GL_DEPTH_STENCIL,
                  GL_UNSIGNED_INT_24_8, NULL);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    qglGenFramebuffers(1, &depthFramebuffer);
    qglBindFramebuffer(GL_FRAMEBUFFER, depthFramebuffer);
    qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    bool complete = qglCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    qglBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
    qglBindTexture(GL_TEXTURE_2D, tex);
    return complete;
}

bool buildWorld() {
    idRenderWorldLocal *world = backEnd.viewDef->renderWorld;
    if (!world)
        return false;
    std::vector<float> xyz;
    for (int i = 0; i < world->entityDefs.Num(); i++) {
        const idRenderEntityLocal *def = world->entityDefs[i];
        if (!def || !def->parms.hModel || def->parms.flags.weaponDepthHack ||
            def->parms.hModel->IsDynamicModel() != DM_STATIC)
            continue;
        const idRenderModel *model = def->parms.hModel;
        for (int j = 0; j < model->NumSurfaces(); j++) {
            const modelSurface_t *surface = model->Surface(j);
            if (!surface || !surface->geometry || !surface->material)
                continue;
            const idMaterial *mat = surface->material;
            if (!mat->IsDrawn() || mat->Coverage() != MC_OPAQUE || mat->Deform() != DFRM_NONE ||
                mat->GetSort() >= SS_POST_PROCESS)
                continue;
            const srfTriangles_t *geo = surface->geometry;
            if (!geo->verts || !geo->indexes)
                continue;
            for (int k = 0; k < geo->numIndexes; k++) {
                idVec3 p;
                R_LocalPointToGlobal(def->modelMatrix, geo->verts[geo->indexes[k]].xyz, p);
                xyz.push_back(p.x);
                xyz.push_back(p.y);
                xyz.push_back(p.z);
            }
        }
    }
    triangles = xyz.size() / 9;
    if (!triangles)
        return false; // Loading screens also submit small render worlds.
    char error[4096];
    bool ok = ETQW_MetalRT_Build(xyz.data(), xyz.size() / 3, error, sizeof(error));
    if (!ok)
        common->Warning("Metal RT geometry: %s", error);
    return ok;
}
} // namespace

void R_MetalRT_Draw() {
    if (!r_metalRT.GetBool() || failed || !backEnd.viewDef->viewEntitys || backEnd.viewDef->isSubview)
        return;
    if (!initialized) {
        initialized = setup();
        if (!initialized) {
            failed = true;
            return;
        }
    }
    if (!worldReady) {
        worldReady = buildWorld();
        if (!worldReady)
            return;
    }
    // The two APIs share the allocation. Finish outstanding reads before Metal writes;
    // the Metal command buffer completes before GLES samples it. No CPU pixel transfer.
    if (!MetalBackend_Active())
        qglFinish();
    float camera[24] = {0};
    const renderView_t &view = backEnd.viewDef->renderView;
    memcpy(camera, view.vieworg.ToFloatPtr(), 3 * sizeof(float));
    for (int i = 0; i < 3; i++)
        memcpy(camera + 4 + i * 4, view.viewaxis[i].ToFloatPtr(), 3 * sizeof(float));
#ifdef _SPLASHDAMAGE
    memcpy(camera + 16, backEnd.parms.sunDir.ToFloatPtr(), 3 * sizeof(float));
#else
    camera[18] = 1;
#endif
    camera[20] = tan(DEG2RAD(view.fov_x) * .5f);
    camera[21] = tan(DEG2RAD(view.fov_y) * .5f);
    camera[22] = r_metalRTStrength.GetFloat();
    GLuint mask = ETQW_MetalRT_Trace(camera, &gpuMilliseconds);
    if (!mask) {
        failed = true;
        return;
    }
    GLint previousProgram, active, tex0, tex1, read, draw, srcRGB, dstRGB, srcAlpha, dstAlpha, eqRGB, eqAlpha;
    qglGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    qglGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    qglGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    qglGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    qglGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
    qglGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
    qglGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    qglGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    qglGetIntegerv(GL_BLEND_EQUATION_RGB, &eqRGB);
    qglGetIntegerv(GL_BLEND_EQUATION_ALPHA, &eqAlpha);
    GLboolean depth = qglIsEnabled(GL_DEPTH_TEST), blend = qglIsEnabled(GL_BLEND),
              cull = qglIsEnabled(GL_CULL_FACE), scissor = qglIsEnabled(GL_SCISSOR_TEST),
              stencil = qglIsEnabled(GL_STENCIL_TEST);
    qglDisable(GL_SCISSOR_TEST);
    qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, depthFramebuffer);
    qglBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_DEPTH_BUFFER_BIT,
                       GL_NEAREST);
    qglBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
    qglActiveTexture(GL_TEXTURE0);
    qglGetIntegerv(GL_TEXTURE_BINDING_2D, &tex0);
    qglBindTexture(GL_TEXTURE_2D, mask);
    qglActiveTexture(GL_TEXTURE1);
    qglGetIntegerv(GL_TEXTURE_BINDING_2D, &tex1);
    qglBindTexture(GL_TEXTURE_2D, depthTexture);
    qglUseProgram(program);
    qglUniform1i(qglGetUniformLocation(program, "maskTex"), 0);
    qglUniform1i(qglGetUniformLocation(program, "depthTex"), 1);
    qglUniform2f(qglGetUniformLocation(program, "imageSize"), renderWidth, renderHeight);
    qglUniform4f(qglGetUniformLocation(program, "projection"), backEnd.viewDef->projectionMatrix[10],
                 backEnd.viewDef->projectionMatrix[14], camera[20], camera[21]);
    qglDisable(GL_DEPTH_TEST);
    qglDisable(GL_CULL_FACE);
    qglDisable(GL_STENCIL_TEST);
    qglEnable(GL_BLEND);
    qglBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    qglBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
    qglDrawArrays(GL_TRIANGLES, 0, 3);
    qglBlendEquationSeparate(eqRGB, eqAlpha);
    qglBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
    if (depth)
        qglEnable(GL_DEPTH_TEST);
    if (cull)
        qglEnable(GL_CULL_FACE);
    if (stencil)
        qglEnable(GL_STENCIL_TEST);
    if (scissor)
        qglEnable(GL_SCISSOR_TEST);
    if (!blend)
        qglDisable(GL_BLEND);
    qglActiveTexture(GL_TEXTURE1);
    qglBindTexture(GL_TEXTURE_2D, tex1);
    qglActiveTexture(GL_TEXTURE0);
    qglBindTexture(GL_TEXTURE_2D, tex0);
    qglActiveTexture(active);
    qglUseProgram(previousProgram);
    frames++;
    if (frames == 1)
        common->Printf("Native Metal RT active: %zu triangles, %dx%d mask, four rays/pixel\n", triangles,
                       maskWidth, maskHeight);
    GL_CheckErrors();
}

void R_MetalRT_ResetWorld() {
    worldReady = false;
    triangles = 0;
    frames = 0;
}
void R_MetalRT_Info_f(const idCmdArgs &) {
    common->Printf(
        "Metal RT: enabled %d, initialized %d, world %d, failed %d; %zu triangles; %dx%d; %lu frames\n",
        r_metalRT.GetBool(), initialized, worldReady, failed, triangles, maskWidth, maskHeight, frames);
    if (MetalBackend_Active())
        common->Printf("Raster + RT GPU duration: metalInfo (unified command buffer)\n");
    else
        common->Printf("RT-only GPU duration: %.2f ms\n", gpuMilliseconds);
}
void R_MetalRT_Shutdown() {
    if (program)
        qglDeleteProgram(program);
    if (depthFramebuffer)
        qglDeleteFramebuffers(1, &depthFramebuffer);
    if (depthTexture)
        qglDeleteTextures(1, &depthTexture);
    program = depthTexture = depthFramebuffer = 0;
    ETQW_MetalRT_Shutdown();
    R_MetalRT_ResetWorld();
    initialized = failed = false;
}
