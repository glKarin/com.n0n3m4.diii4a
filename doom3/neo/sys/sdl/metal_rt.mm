// SPDX-License-Identifier: GPL-3.0-or-later
#import <Metal/Metal.h>
#include <SDL.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "metal_rt.h"
#include "metal_backend.h"

namespace {
id<MTLDevice> device;
id<MTLCommandQueue> queue;
id<MTLComputePipelineState> pipeline;
id<MTLAccelerationStructure> scene;
id<MTLBuffer> vertices;
id<MTLTexture> mask;
EGLDisplay display;
EGLImageKHR image;
GLuint texture;
void *eglLibrary;
decltype(&eglGetProcAddress) getProc;
int maskWidth, maskHeight;
bool nativeBackend = false;

const char *shader = R"metal(
#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;
struct Camera { float4 origin, forward, left, up, sun, options; };
kernel void shadowMask(texture2d<float, access::write> out [[texture(0)]],
    primitive_acceleration_structure scene [[buffer(0)]],
    const device packed_float3 *vertices [[buffer(1)]], constant Camera &c [[buffer(2)]],
    uint2 p [[thread_position_in_grid]]) {
    if (p.x >= out.get_width() || p.y >= out.get_height()) return;
    float2 uv = (float2(p) + .5) / float2(out.get_width(), out.get_height());
    ray r;
    r.origin = c.origin.xyz;
    r.direction = normalize(c.forward.xyz + c.left.xyz * ((1-2*uv.x)*c.options.x)
                           + c.up.xyz * ((2*uv.y-1)*c.options.y));
    r.min_distance = 2; r.max_distance = 131072;
    intersector<triangle_data> query;
    auto hit = query.intersect(r, scene);
    if (hit.type != intersection_type::triangle) { out.write(float4(1,0,0,1),p); return; }
    float3 a = vertices[hit.primitive_id*3], b = vertices[hit.primitive_id*3+1], d = vertices[hit.primitive_id*3+2];
    float3 n = normalize(cross(b-a,d-a));
    if (dot(n,r.direction) > 0) n = -n;
    float3 sun = normalize(c.sun.xyz);
    float visibility = 1;
    if (dot(n,sun) > .02) {
        float3 center = r.origin + hit.distance*r.direction + n*1.5;
        float3 tangent = normalize(cross(sun, abs(sun.z) < .9 ? float3(0,0,1) : float3(0,1,0)));
        float3 bitangent = cross(sun,tangent);
        float lit = 0;
        query.accept_any_intersection(true);
        for (int i=0;i<4;i++) {
            float angle = float(i)*2.39996323;
            float radius = sqrt((float(i)+.5)/4.0)*.00465;
            ray shadow; shadow.origin=center;
            shadow.direction=normalize(sun + radius*(cos(angle)*tangent+sin(angle)*bitangent));
            shadow.min_distance=.5; shadow.max_distance=131072;
            auto h = query.intersect(shadow,scene);
            lit += h.type == intersection_type::none ? 1.0 : 0.0;
        }
        // Only attenuate the sun-facing irradiance, not the baked ambient
        // term. A constant darkening at grazing angles exposes every coarse
        // terrain triangle as a hard shadow terminator.
        visibility = 1-c.options.z*(1-lit*.25)*clamp(dot(n,sun),0.0,1.0);
    }
    out.write(float4(visibility,hit.distance,0,1),p);
}
)metal";

bool fail(char *out, size_t size, const char *message) {
    snprintf(out, size, "%s", message ? message : "unknown Metal error");
    return false;
}
} // namespace

bool ETQW_MetalRT_Init(int width, int height, char *error, size_t errorSize) {
    @autoreleasepool {
        nativeBackend = MetalBackend_Active();
        if (nativeBackend) {
            device = (__bridge id<MTLDevice>)MetalBackend_Device();
            queue = (__bridge id<MTLCommandQueue>)MetalBackend_Queue();
        } else {
            eglLibrary = dlopen(getenv("SDL_EGL_LIBRARY"), RTLD_NOW | RTLD_LOCAL);
            if (!eglLibrary)
                return fail(error, errorSize, "EGL library unavailable");
            auto current = (decltype(&eglGetCurrentDisplay))dlsym(eglLibrary, "eglGetCurrentDisplay");
            getProc = (decltype(&eglGetProcAddress))dlsym(eglLibrary, "eglGetProcAddress");
            if (!current || !getProc)
                return fail(error, errorSize, "EGL device query unavailable");
            display = current();
            auto qdisplay = (PFNEGLQUERYDISPLAYATTRIBEXTPROC)getProc("eglQueryDisplayAttribEXT");
            auto qdevice = (PFNEGLQUERYDEVICEATTRIBEXTPROC)getProc("eglQueryDeviceAttribEXT");
            EGLAttrib handle = 0, metal = 0;
            if (!qdisplay || !qdevice || !qdisplay(display, EGL_DEVICE_EXT, &handle) ||
                !qdevice((EGLDeviceEXT)handle, 0x34A6, &metal))
                return fail(error, errorSize, "Metal device sharing unavailable");
            device = (__bridge id<MTLDevice>)(void *)metal;
        }
        if (!device.supportsRaytracing || ![device supportsFamily:MTLGPUFamilyApple9])
            return fail(error, errorSize, "Hardware ray tracing requires Apple M3 or newer");
        MTLCompileOptions *options = [MTLCompileOptions new];
        options.languageVersion = MTLLanguageVersion3_1;
        NSError *e = nil;
        auto library = [device newLibraryWithSource:[NSString stringWithUTF8String:shader]
                                            options:options
                                              error:&e];
        if (!library)
            return fail(error, errorSize, e.description.UTF8String);
        pipeline = [device newComputePipelineStateWithFunction:[library newFunctionWithName:@"shadowMask"]
                                                         error:&e];
        if (!pipeline)
            return fail(error, errorSize, e.description.UTF8String);
        if (!nativeBackend)
            queue = [device newCommandQueue];
        MTLTextureDescriptor *td =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                                               width:width
                                                              height:height
                                                           mipmapped:NO];
        td.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        mask = [device newTextureWithDescriptor:td];
        if (!mask)
            return fail(error, errorSize, "Unable to allocate ray tracing mask");
        if (nativeBackend) {
            texture = MetalBackend_ImportTexture((__bridge void *)mask);
            maskWidth = width;
            maskHeight = height;
            printf("Native Metal RT: %s, %dx%d, unified raster/RT command queue\n", device.name.UTF8String,
                   width, height);
            return true;
        }
        auto create = (PFNEGLCREATEIMAGEKHRPROC)getProc("eglCreateImageKHR");
        EGLint attrs[] = {EGL_NONE};
        image = create(display, EGL_NO_CONTEXT, 0x34A7, (__bridge EGLClientBuffer)mask, attrs);
        if (!image)
            return fail(error, errorSize, "Metal texture import failed");
        auto gen = (PFNGLGENTEXTURESPROC)SDL_GL_GetProcAddress("glGenTextures");
        auto bind = (PFNGLBINDTEXTUREPROC)SDL_GL_GetProcAddress("glBindTexture");
        auto target =
            (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)SDL_GL_GetProcAddress("glEGLImageTargetTexture2DOES");
        auto param = (PFNGLTEXPARAMETERIPROC)SDL_GL_GetProcAddress("glTexParameteri");
        auto get = (PFNGLGETINTEGERVPROC)SDL_GL_GetProcAddress("glGetIntegerv");
        if (!target)
            return fail(error, errorSize, "GLES EGL image import unavailable");
        GLint previous;
        get(GL_TEXTURE_BINDING_2D, &previous);
        gen(1, &texture);
        bind(GL_TEXTURE_2D, texture);
        target(GL_TEXTURE_2D, image);
        param(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        param(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        param(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        param(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        bind(GL_TEXTURE_2D, previous);
        maskWidth = width;
        maskHeight = height;
        printf("Native Metal RT: %s, %dx%d, shared GPU texture, four sun rays/pixel\n",
               device.name.UTF8String, width, height);
        return true;
    }
}

bool ETQW_MetalRT_Build(const float *triangles, size_t count, char *error, size_t errorSize) {
    @autoreleasepool {
        if (!device || count < 3)
            return fail(error, errorSize, "No opaque world geometry");
        vertices = [device newBufferWithBytes:triangles
                                       length:count * 3 * sizeof(float)
                                      options:MTLResourceStorageModeShared];
        auto geometry = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
        geometry.vertexBuffer = vertices;
        geometry.vertexStride = 12;
        geometry.vertexFormat = MTLAttributeFormatFloat3;
        geometry.triangleCount = count / 3;
        geometry.opaque = YES;
        auto desc = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
        desc.geometryDescriptors = @[ geometry ];
        auto sizes = [device accelerationStructureSizesWithDescriptor:desc];
        scene = [device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
        auto scratch = [device newBufferWithLength:sizes.buildScratchBufferSize
                                           options:MTLResourceStorageModePrivate];
        if (!vertices || !scene || !scratch)
            return fail(error, errorSize, "Metal acceleration structure allocation failed");
        auto cb = [queue commandBuffer];
        auto encoder = [cb accelerationStructureCommandEncoder];
        [encoder buildAccelerationStructure:scene
                                 descriptor:desc
                              scratchBuffer:scratch
                        scratchBufferOffset:0];
        [encoder endEncoding];
        [cb commit];
        [cb waitUntilCompleted];
        if (cb.error)
            return fail(error, errorSize, cb.error.description.UTF8String);
        printf("Native Metal RT: %zu static triangles, %.1f MiB acceleration structure\n", count / 3,
               double(sizes.accelerationStructureSize) / (1024 * 1024));
        return true;
    }
}

unsigned int ETQW_MetalRT_Trace(const float *camera, double *milliseconds) {
    @autoreleasepool {
        if (!scene)
            return 0;
        auto cb = nativeBackend ? (__bridge id<MTLCommandBuffer>)MetalBackend_CommandBuffer()
                                : [queue commandBuffer];
        auto encoder = [cb computeCommandEncoder];
        [encoder setComputePipelineState:pipeline];
        [encoder setAccelerationStructure:scene atBufferIndex:0];
        [encoder setBuffer:vertices offset:0 atIndex:1];
        [encoder setBytes:camera length:24 * sizeof(float) atIndex:2];
        [encoder setTexture:mask atIndex:0];
        [encoder dispatchThreads:MTLSizeMake(maskWidth, maskHeight, 1)
            threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
        [encoder endEncoding];
        if (nativeBackend) {
            *milliseconds = 0;
            return texture;
        }
        [cb commit];
        [cb waitUntilCompleted];
        if (cb.error) {
            fprintf(stderr, "Native Metal RT: %s\n", cb.error.description.UTF8String);
            return 0;
        }
        *milliseconds = (cb.GPUEndTime - cb.GPUStartTime) * 1000;
        return texture;
    }
}

void ETQW_MetalRT_Shutdown() {
    @autoreleasepool {
        if (nativeBackend)
            MetalBackend_Finish();
        if (texture && !nativeBackend) {
            auto del = (PFNGLDELETETEXTURESPROC)SDL_GL_GetProcAddress("glDeleteTextures");
            if (del)
                del(1, &texture);
            texture = 0;
        }
        if (image && getProc)
            ((PFNEGLDESTROYIMAGEKHRPROC)getProc("eglDestroyImageKHR"))(display, image);
        image = nullptr;
        mask = nil;
        scene = nil;
        vertices = nil;
        pipeline = nil;
        queue = nil;
        device = nil;
        if (eglLibrary)
            dlclose(eglLibrary);
        eglLibrary = nullptr;
        getProc = nullptr;
    }
}
