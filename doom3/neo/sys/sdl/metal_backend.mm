// SPDX-License-Identifier: GPL-3.0-or-later
// Native Metal implementation of the renderer's existing command interface.
// The interface names are retained to avoid changing game/material code; no
// OpenGL context, EGL image, ANGLE library or GL driver is used by this backend.
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Foundation/Foundation.h>
#include <SDL.h>
#include <SDL_metal.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
typedef GLDEBUGPROCKHR GLDEBUGPROC;
#include <CommonCrypto/CommonDigest.h>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <atomic>
#include <array>
#include "metal_backend.h"
#include "metal_shader.h"
extern "C" void *objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void *);

namespace {
constexpr unsigned MaxAttributes = 16, MaxTextures = 32, FrameCount = 3;
constexpr size_t UploadCapacity = 64 * 1024 * 1024;
id<MTLDevice> device;
id<MTLCommandQueue> queue;
id<MTLCommandBuffer> command;
id<MTLCommandBuffer> lastSubmitted;
id<MTLRenderCommandEncoder> encoder;
CAMetalLayer *layer;
SDL_MetalView metalView;
SDL_Window *window;
NSString *shaderCacheDirectory;
id<CAMetalDrawable> drawable;
id<MTLTexture> screenColor, screenDepth, whiteTexture, whiteCube;
dispatch_semaphore_t frameSemaphore;
unsigned long frame = 0, drawCalls = 0, passCount = 0, textureBytes = 0, unsupportedCalls = 0;
std::atomic<unsigned long> completedFrame{0};
std::atomic<double> gpuTime{0};
bool frameAcquired = false;
void *pool = nullptr;
struct FrameUpload {
    id<MTLBuffer> buffer;
    size_t offset = 0;
};
FrameUpload uploads[FrameCount];
std::vector<id<MTLBuffer>> spillBuffers;
struct Texture {
    id<MTLTexture> image;
    GLenum target = GL_TEXTURE_2D, format = GL_RGBA;
    int min = GL_NEAREST_MIPMAP_LINEAR, mag = GL_LINEAR, s = GL_REPEAT, t = GL_REPEAT, r = GL_REPEAT,
        maxLevel = 1000;
    int compareMode = GL_NONE, compareFunc = GL_LEQUAL;
    float anisotropy = 1;
    bool depth = false;
    unsigned long usedFrame = 0;
};
struct Buffer {
    id<MTLBuffer> object;
    GLenum usage = GL_STATIC_DRAW;
    size_t size = 0;
    unsigned long usedFrame = 0;
};
struct Attachment {
    GLuint id = 0;
    bool renderbuffer = false;
    unsigned level = 0, slice = 0;
};
struct Framebuffer {
    Attachment color, depth, stencil;
};
struct Shader {
    GLenum type;
    std::string source, error;
    bool compiled = true;
};
struct SamplerBinding {
    unsigned slot, target;
    bool vertex, fragment;
};
struct Program {
    GLuint vert = 0, frag = 0;
    bool linked = false;
    std::string error;
    std::map<std::string, unsigned> bindings;
    MetalShaderProgram translated;
    id<MTLFunction> vertex, fragment;
    std::vector<unsigned char> uniforms;
    std::vector<int> samplerUnits;
    std::map<std::string, GLint> locations;
    std::vector<SamplerBinding> resources;
    unsigned bc5Offset = ~0u;
};
struct Attribute {
    bool enabled = false;
    int size = 4;
    GLenum type = GL_FLOAT;
    bool normalized = false;
    int stride = 0;
    uintptr_t pointer = 0;
    GLuint buffer = 0;
    float constant[4] = {0, 0, 0, 1};
};
struct Stencil {
    GLenum func = GL_ALWAYS, fail = GL_KEEP, zfail = GL_KEEP, pass = GL_KEEP;
    unsigned read = 255, write = 255;
    int ref = 0;
};
std::unordered_map<GLuint, Texture> textures, renderbuffers;
std::unordered_map<GLuint, Buffer> buffers;
std::unordered_map<GLuint, Framebuffer> framebuffers;
std::unordered_map<GLuint, Shader> shaders;
std::unordered_map<GLuint, Program> programs;
std::unordered_map<std::string, id<MTLRenderPipelineState>> pipelines;
std::unordered_map<std::string, id<MTLDepthStencilState>> depthStates;
std::unordered_map<std::string, id<MTLSamplerState>> samplers;
GLuint nextId = 1, currentProgram = 0, arrayBuffer = 0, indexBuffer = 0, renderbuffer = 0, drawFbo = 0,
       readFbo = 0;
GLuint boundTextures[MaxTextures][3] = {};
unsigned activeTexture = 0;
Attribute attributes[MaxAttributes];
std::set<GLenum> enabled;
GLenum depthFunc = GL_LESS, cullFace = GL_BACK, frontFace = GL_CCW;
GLenum srcRGB = GL_ONE, dstRGB = GL_ZERO, srcAlpha = GL_ONE, dstAlpha = GL_ZERO, eqRGB = GL_FUNC_ADD,
       eqAlpha = GL_FUNC_ADD;
bool depthWrite = true;
unsigned colorMask = 15;
float blendColor[4] = {};
int viewport[4] = {0, 0, 1280, 720}, scissor[4] = {0, 0, 1280, 720}, packAlignment = 4, unpackAlignment = 4,
    unpackRowLength = 0;
double nearDepth = 0, farDepth = 1;
float depthBias = 0, slopeBias = 0;
MTLClearColor clearColor = {0, 0, 0, 0};
double clearDepth = 1;
unsigned clearStencil = 0;
Stencil frontStencil, backStencil;
GLenum lastError = GL_NO_ERROR;
std::set<std::string> warned;
struct Query {
    id<MTLBuffer> buffer;
    unsigned long pending = 0;
};
std::unordered_map<GLuint, Query> queries;
GLuint activeQuery = 0;

void unsupported(const char *name) {
    ++unsupportedCalls;
    if (warned.insert(name).second)
        fprintf(stderr, "Metal backend unsupported command: %s\n", name);
    lastError = GL_INVALID_OPERATION;
}
void error(const char *s) {
    fprintf(stderr, "Metal backend: %s\n", s);
    lastError = GL_INVALID_OPERATION;
}
void endEncoding() {
    if (encoder) {
        [encoder endEncoding];
        encoder = nil;
    }
}
void beginCommand() {
    if (!pool)
        pool = objc_autoreleasePoolPush();
    if (!frameAcquired) {
        dispatch_semaphore_wait(frameSemaphore, DISPATCH_TIME_FOREVER);
        frameAcquired = true;
        uploads[frame % FrameCount].offset = 0;
        spillBuffers.clear();
    }
    if (!command) {
        command = [queue commandBuffer];
        command.label = @"ETQW frame";
    }
}

void submit(bool wait, bool present = false) {
    endEncoding();
    if (!command) {
        if (wait && lastSubmitted)
            [lastSubmitted waitUntilCompleted];
        return;
    }
    id<MTLCommandBuffer> cb = command;
    command = nil;
    if (present && drawable) {
        [cb presentDrawable:drawable];
        drawable = nil;
    }
    unsigned long submitted = frame + 1;
    if (present) {
        auto sem = frameSemaphore;
        [cb addCompletedHandler:^(id<MTLCommandBuffer> b) {
          gpuTime.store((b.GPUEndTime - b.GPUStartTime) * 1000);
          completedFrame.store(submitted);
          if (b.error)
              fprintf(stderr, "Metal GPU error: %s\n", b.error.description.UTF8String);
          dispatch_semaphore_signal(sem);
        }];
        frameAcquired = false;
        ++frame;
    }
    [cb commit];
    lastSubmitted = cb;
    if (wait)
        [cb waitUntilCompleted];
    if (wait && cb.error)
        error(cb.error.description.UTF8String);
}
std::pair<id<MTLBuffer>, size_t> upload(const void *data, size_t length, size_t alignment = 256) {
    beginCommand();
    auto &f = uploads[frame % FrameCount];
    size_t offset = (f.offset + alignment - 1) & ~(alignment - 1);
    id<MTLBuffer> b = f.buffer;
    if (offset + length > UploadCapacity) {
        b = [device newBufferWithLength:std::max(length, size_t(16)) options:MTLResourceStorageModeShared];
        offset = 0;
        spillBuffers.push_back(b);
    } else
        f.offset = offset + length;
    if (data && length)
        memcpy((char *)b.contents + offset, data, length);
    return {b, offset};
}
unsigned textureSlot(GLenum target) {
    return target == GL_TEXTURE_CUBE_MAP ||
                   (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
               ? 1
           : (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY) ? 2
                                                                        : 0;
}
Texture &boundTexture(GLenum target) {
    return textures[boundTextures[activeTexture][textureSlot(target)]];
}
id<MTLTexture> attachment(const Attachment &a) {
    return a.id ? (a.renderbuffer ? renderbuffers[a.id].image : textures[a.id].image) : nil;
}
id<MTLTexture> colorTarget(GLuint fbo) {
    return fbo ? attachment(framebuffers[fbo].color) : screenColor;
}
id<MTLTexture> depthTarget(GLuint fbo) {
    return fbo ? attachment(framebuffers[fbo].depth) : screenDepth;
}
bool hasStencil(id<MTLTexture> t) {
    return t &&
           (t.pixelFormat == MTLPixelFormatDepth32Float_Stencil8 || t.pixelFormat == MTLPixelFormatStencil8);
}
MTLCompareFunction compare(GLenum f) {
    switch (f) {
    case GL_NEVER:
        return MTLCompareFunctionNever;
    case GL_LESS:
        return MTLCompareFunctionLess;
    case GL_EQUAL:
        return MTLCompareFunctionEqual;
    case GL_LEQUAL:
        return MTLCompareFunctionLessEqual;
    case GL_GREATER:
        return MTLCompareFunctionGreater;
    case GL_NOTEQUAL:
        return MTLCompareFunctionNotEqual;
    case GL_GEQUAL:
        return MTLCompareFunctionGreaterEqual;
    default:
        return MTLCompareFunctionAlways;
    }
}
MTLStencilOperation stencilOp(GLenum f) {
    switch (f) {
    case GL_ZERO:
        return MTLStencilOperationZero;
    case GL_REPLACE:
        return MTLStencilOperationReplace;
    case GL_INCR:
        return MTLStencilOperationIncrementClamp;
    case GL_DECR:
        return MTLStencilOperationDecrementClamp;
    case GL_INVERT:
        return MTLStencilOperationInvert;
    case GL_INCR_WRAP:
        return MTLStencilOperationIncrementWrap;
    case GL_DECR_WRAP:
        return MTLStencilOperationDecrementWrap;
    default:
        return MTLStencilOperationKeep;
    }
}
MTLBlendFactor blendFactor(GLenum f) {
    switch (f) {
    case GL_ZERO:
        return MTLBlendFactorZero;
    case GL_ONE:
        return MTLBlendFactorOne;
    case GL_SRC_COLOR:
        return MTLBlendFactorSourceColor;
    case GL_ONE_MINUS_SRC_COLOR:
        return MTLBlendFactorOneMinusSourceColor;
    case GL_DST_COLOR:
        return MTLBlendFactorDestinationColor;
    case GL_ONE_MINUS_DST_COLOR:
        return MTLBlendFactorOneMinusDestinationColor;
    case GL_SRC_ALPHA:
        return MTLBlendFactorSourceAlpha;
    case GL_ONE_MINUS_SRC_ALPHA:
        return MTLBlendFactorOneMinusSourceAlpha;
    case GL_DST_ALPHA:
        return MTLBlendFactorDestinationAlpha;
    case GL_ONE_MINUS_DST_ALPHA:
        return MTLBlendFactorOneMinusDestinationAlpha;
    case GL_CONSTANT_COLOR:
        return MTLBlendFactorBlendColor;
    case GL_ONE_MINUS_CONSTANT_COLOR:
        return MTLBlendFactorOneMinusBlendColor;
    case GL_CONSTANT_ALPHA:
        return MTLBlendFactorBlendAlpha;
    case GL_ONE_MINUS_CONSTANT_ALPHA:
        return MTLBlendFactorOneMinusBlendAlpha;
    default:
        return MTLBlendFactorSourceAlphaSaturated;
    }
}
MTLBlendOperation blendOp(GLenum f) {
    return f == GL_FUNC_SUBTRACT           ? MTLBlendOperationSubtract
           : f == GL_FUNC_REVERSE_SUBTRACT ? MTLBlendOperationReverseSubtract
           : f == GL_MIN                   ? MTLBlendOperationMin
           : f == GL_MAX                   ? MTLBlendOperationMax
                                           : MTLBlendOperationAdd;
}
void beginPass(GLbitfield clear = 0) {
    beginCommand();
    if (encoder && !clear)
        return;
    endEncoding();
    auto color = colorTarget(drawFbo), depth = depthTarget(drawFbo);
    if (!color && !depth) {
        error("draw without render target");
        return;
    }
    auto desc = [MTLRenderPassDescriptor renderPassDescriptor];
    if (color) {
        desc.colorAttachments[0].texture = color;
        desc.colorAttachments[0].loadAction =
            (clear & GL_COLOR_BUFFER_BIT) ? MTLLoadActionClear : MTLLoadActionLoad;
        desc.colorAttachments[0].storeAction = MTLStoreActionStore;
        desc.colorAttachments[0].clearColor = clearColor;
    }
    if (depth) {
        desc.depthAttachment.texture = depth;
        desc.depthAttachment.loadAction =
            (clear & GL_DEPTH_BUFFER_BIT) ? MTLLoadActionClear : MTLLoadActionLoad;
        desc.depthAttachment.storeAction = MTLStoreActionStore;
        desc.depthAttachment.clearDepth = clearDepth;
    }
    if (hasStencil(depth)) {
        desc.stencilAttachment.texture = depth;
        desc.stencilAttachment.loadAction =
            (clear & GL_STENCIL_BUFFER_BIT) ? MTLLoadActionClear : MTLLoadActionLoad;
        desc.stencilAttachment.storeAction = MTLStoreActionStore;
        desc.stencilAttachment.clearStencil = clearStencil;
    }
    if (drawFbo) {
        const auto &f = framebuffers[drawFbo];
        desc.colorAttachments[0].level = f.color.level;
        desc.colorAttachments[0].slice = f.color.slice;
        desc.depthAttachment.level = f.depth.level;
        desc.depthAttachment.slice = f.depth.slice;
        desc.stencilAttachment.level = f.depth.level;
        desc.stencilAttachment.slice = f.depth.slice;
    }
    if (activeQuery)
        desc.visibilityResultBuffer = queries[activeQuery].buffer;
    encoder = [command renderCommandEncoderWithDescriptor:desc];
    encoder.label = @"ETQW raster";
    ++passCount;
    if (activeQuery)
        [encoder setVisibilityResultMode:MTLVisibilityResultModeBoolean offset:0];
}
MTLPixelFormat pixelFormat(GLenum f) {
    switch (f) {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
        return MTLPixelFormatDepth32Float;
    case GL_DEPTH_STENCIL:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
        return MTLPixelFormatDepth32Float_Stencil8;
    case GL_STENCIL_INDEX8:
        return MTLPixelFormatStencil8;
    case GL_RGBA16F:
        return MTLPixelFormatRGBA16Float;
    case GL_RGBA32F:
        return MTLPixelFormatRGBA32Float;
    case GL_R8:
        return MTLPixelFormatR8Unorm;
    case GL_RG8:
        return MTLPixelFormatRG8Unorm;
    case 0x83f0:
    case 0x83f1:
        return MTLPixelFormatBC1_RGBA;
    case 0x83f2:
        return MTLPixelFormatBC2_RGBA;
    case 0x83f3:
        return MTLPixelFormatBC3_RGBA;
    case 0x8dbd:
        return MTLPixelFormatBC5_RGUnorm;
    default:
        return MTLPixelFormatRGBA8Unorm;
    }
}
bool compressed(MTLPixelFormat f) {
    return f == MTLPixelFormatBC1_RGBA || f == MTLPixelFormatBC2_RGBA || f == MTLPixelFormatBC3_RGBA ||
           f == MTLPixelFormatBC5_RGUnorm;
}
int pixelBytes(MTLPixelFormat f) {
    return f == MTLPixelFormatRGBA32Float   ? 16
           : f == MTLPixelFormatRGBA16Float ? 8
           : f == MTLPixelFormatR8Unorm     ? 1
           : f == MTLPixelFormatRG8Unorm    ? 2
                                            : 4;
}
void allocateTexture(Texture &t, GLenum target, GLenum format, int w, int h, int depth = 1,
                     bool mipmaps = true) {
    endEncoding();
    auto f = pixelFormat(format);
    t.format = format;
    t.target = target;
    t.depth = (f == MTLPixelFormatDepth32Float || f == MTLPixelFormatDepth32Float_Stencil8);
    auto d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:f
                                                                width:w
                                                               height:h
                                                            mipmapped:mipmaps && !t.depth];
    if (textureSlot(target) == 1) {
        d.textureType = MTLTextureTypeCube;
        d.arrayLength = 1;
    }
    if (target == GL_TEXTURE_3D) {
        d.textureType = MTLTextureType3D;
        d.depth = depth;
    }
    if (target == GL_TEXTURE_2D_ARRAY) {
        d.textureType = MTLTextureType2DArray;
        d.arrayLength = depth;
    }
    d.storageMode = MTLStorageModePrivate;
    d.usage = MTLTextureUsageShaderRead;
    if (format == 0x83f0)
        d.swizzle = MTLTextureSwizzleChannelsMake(MTLTextureSwizzleRed, MTLTextureSwizzleGreen,
                                                  MTLTextureSwizzleBlue, MTLTextureSwizzleOne);
    if (!compressed(f))
        d.usage |= MTLTextureUsageRenderTarget;
    t.image = [device newTextureWithDescriptor:d];
    if (!t.image)
        error("texture allocation failed");
}
void uploadTexture(Texture &t, unsigned level, unsigned slice, int x, int y, int w, int h, GLenum format,
                   GLenum type, const void *pixels, bool isCompressed = false) {
    if (!pixels || !t.image || level >= t.image.mipmapLevelCount || w <= 0 || h <= 0)
        return;
    int bytes = pixelBytes(t.image.pixelFormat);
    size_t rowBytes = size_t(w) * bytes;
    std::vector<unsigned char> converted;
    if (isCompressed) {
        bytes = t.image.pixelFormat == MTLPixelFormatBC1_RGBA ? 8 : 16;
        rowBytes = ((w + 3) / 4) * bytes;
    } else if (type == GL_UNSIGNED_BYTE && t.image.pixelFormat == MTLPixelFormatRGBA8Unorm &&
               format != GL_RGBA) {
        int components = format == GL_RGB ? 3 : format == GL_LUMINANCE_ALPHA ? 2 : 1;
        int inputRow = ((unpackRowLength ? unpackRowLength : w) * components + unpackAlignment - 1) &
                       ~(unpackAlignment - 1);
        converted.resize(size_t(w) * h * 4);
        auto src = (const unsigned char *)pixels;
        for (int j = 0; j < h; ++j)
            for (int i = 0; i < w; ++i) {
                auto p = src + j * inputRow + i * components;
                auto q = &converted[(size_t(j) * w + i) * 4];
                q[0] = format == GL_ALPHA ? 255 : p[0];
                q[1] = format == GL_RGB ? p[1] : q[0];
                q[2] = format == GL_RGB ? p[2] : q[0];
                q[3] = format == GL_ALPHA ? p[0] : format == GL_LUMINANCE_ALPHA ? p[1] : 255;
            }
        pixels = converted.data();
    }
    size_t rows = isCompressed ? (h + 3) / 4 : h;
    size_t alignedRow = (rowBytes + 255) & ~size_t(255), length = alignedRow * rows;
    auto data = upload(nullptr, length);
    auto dst = (char *)data.first.contents + data.second;
    size_t sourceRow = rowBytes;
    if (!isCompressed && converted.empty())
        sourceRow =
            ((unpackRowLength ? unpackRowLength : w) * bytes + unpackAlignment - 1) & ~(unpackAlignment - 1);
    for (size_t i = 0; i < rows; ++i)
        memcpy(dst + i * alignedRow, (const char *)pixels + i * sourceRow, rowBytes);
    endEncoding();
    auto blit = [command blitCommandEncoder];
    [blit copyFromBuffer:data.first
               sourceOffset:data.second
          sourceBytesPerRow:alignedRow
        sourceBytesPerImage:length
                 sourceSize:MTLSizeMake(w, h, 1)
                  toTexture:t.image
           destinationSlice:slice
           destinationLevel:level
          destinationOrigin:MTLOriginMake(x, y, 0)];
    [blit endEncoding];
    textureBytes += length;
    // Bound startup staging memory. Normal frame uploads fit in the frame ring.
    if (textureBytes > 128 * 1024 * 1024) {
        submit(true);
        textureBytes = 0;
        uploads[frame % FrameCount].offset = 0;
        spillBuffers.clear();
    }
}
MTLVertexFormat vertexFormat(const Attribute &a) {
    if (a.type == GL_FLOAT)
        return (MTLVertexFormat)(MTLVertexFormatFloat + a.size - 1);
    if (a.type == GL_UNSIGNED_BYTE)
        return a.normalized ? (MTLVertexFormat)(MTLVertexFormatUChar2Normalized + a.size - 2)
                            : (MTLVertexFormat)(MTLVertexFormatUChar2 + a.size - 2);
    if (a.type == GL_SHORT)
        return a.normalized ? (MTLVertexFormat)(MTLVertexFormatShort2Normalized + a.size - 2)
                            : (MTLVertexFormat)(MTLVertexFormatShort2 + a.size - 2);
    if (a.type == GL_HALF_FLOAT)
        return (MTLVertexFormat)(MTLVertexFormatHalf2 + a.size - 2);
    return MTLVertexFormatFloat4;
}
int componentBytes(GLenum t) {
    return t == GL_FLOAT || t == GL_INT || t == GL_UNSIGNED_INT            ? 4
           : t == GL_SHORT || t == GL_UNSIGNED_SHORT || t == GL_HALF_FLOAT ? 2
                                                                           : 1;
}
id<MTLSamplerState> sampler(Texture &t) {
    struct SamplerKey {
        int min, mag, s, t, r, maxLevel, mode, func;
        float aniso;
    } k = {t.min, t.mag, t.s, t.t, t.r, t.maxLevel, t.compareMode, t.compareFunc, t.anisotropy};
    std::string key(reinterpret_cast<const char *>(&k), sizeof(k));
    auto old = samplers.find(key);
    if (old != samplers.end())
        return old->second;
    auto d = [MTLSamplerDescriptor new];
    d.minFilter =
        (t.min == GL_NEAREST || t.min == GL_NEAREST_MIPMAP_NEAREST || t.min == GL_NEAREST_MIPMAP_LINEAR)
            ? MTLSamplerMinMagFilterNearest
            : MTLSamplerMinMagFilterLinear;
    d.magFilter = t.mag == GL_NEAREST ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
    d.mipFilter = (t.min == GL_NEAREST || t.min == GL_LINEAR) ? MTLSamplerMipFilterNotMipmapped
                  : (t.min == GL_NEAREST_MIPMAP_NEAREST || t.min == GL_LINEAR_MIPMAP_NEAREST)
                      ? MTLSamplerMipFilterNearest
                      : MTLSamplerMipFilterLinear;
    auto address = [](int m) {
        return m == GL_REPEAT            ? MTLSamplerAddressModeRepeat
               : m == GL_MIRRORED_REPEAT ? MTLSamplerAddressModeMirrorRepeat
                                         : MTLSamplerAddressModeClampToEdge;
    };
    d.sAddressMode = address(t.s);
    d.tAddressMode = address(t.t);
    d.rAddressMode = address(t.r);
    d.maxAnisotropy = std::max(1, std::min(16, int(t.anisotropy)));
    d.lodMaxClamp = t.maxLevel;
    d.compareFunction =
        t.compareMode == GL_COMPARE_REF_TO_TEXTURE ? compare(t.compareFunc) : MTLCompareFunctionNever;
    auto result = [device newSamplerStateWithDescriptor:d];
    samplers.emplace(key, result);
    return result;
}
bool bindDraw(unsigned maxVertex) {
    auto found = programs.find(currentProgram);
    if (found == programs.end() || !found->second.linked)
        return false;
    auto &p = found->second;
    auto color = colorTarget(drawFbo), depth = depthTarget(drawFbo);
    if (!color && !depth)
        return false;
    struct Key {
        unsigned program, color, depth, blend, src, dst, srcA, dstA, eq, eqA, mask;
        unsigned attr[MaxAttributes][3];
    } key{};
    key.program = currentProgram;
    key.color = color ? color.pixelFormat : 0;
    key.depth = depth ? depth.pixelFormat : 0;
    key.blend = enabled.count(GL_BLEND);
    key.src = srcRGB;
    key.dst = dstRGB;
    key.srcA = srcAlpha;
    key.dstA = dstAlpha;
    key.eq = eqRGB;
    key.eqA = eqAlpha;
    key.mask = colorMask;
    std::array<std::pair<id<MTLBuffer>, size_t>, MaxAttributes> streams{};
    for (unsigned a : p.translated.vertex.attributes) {
        if (a >= MaxAttributes) {
            error("attribute index exceeds backend limit");
            return false;
        }
        auto &v = attributes[a];
        auto format = v.enabled ? vertexFormat(v) : MTLVertexFormatFloat4;
        unsigned stride = v.enabled ? (v.stride ? v.stride : v.size * componentBytes(v.type)) : 16;
        key.attr[a][0] = format;
        key.attr[a][1] = stride;
        key.attr[a][2] = v.enabled;
        if (!v.enabled)
            streams[a] = upload(v.constant, 16);
        else if (v.buffer) {
            auto &b = buffers[v.buffer];
            streams[a] = {b.object, v.pointer};
            b.usedFrame = frame + 1;
        } else if (!v.pointer) {
            error("null client vertex array");
            return false;
        }
    }
    // The engine's client arrays are usually fields of one idDrawVert. Stage
    // each interleaved stream once, rather than copying it for every attribute.
    for (unsigned a : p.translated.vertex.attributes) {
        auto &v = attributes[a];
        if (streams[a].first || !v.enabled || v.buffer)
            continue;
        unsigned stride = key.attr[a][1];
        uintptr_t base = v.pointer;
        for (unsigned b : p.translated.vertex.attributes) {
            auto &other = attributes[b];
            if (other.enabled && !other.buffer && key.attr[b][1] == stride && other.pointer < base &&
                v.pointer - other.pointer < stride)
                base = other.pointer;
        }
        size_t length = size_t(maxVertex) * stride;
        for (unsigned b : p.translated.vertex.attributes) {
            auto &other = attributes[b];
            if (other.enabled && !other.buffer && key.attr[b][1] == stride && other.pointer >= base &&
                other.pointer - base < stride)
                length = std::max(length, size_t(maxVertex) * stride + other.pointer - base +
                                              other.size * componentBytes(other.type));
        }
        auto staged = upload((void *)base, length);
        for (unsigned b : p.translated.vertex.attributes) {
            auto &other = attributes[b];
            if (other.enabled && !other.buffer && key.attr[b][1] == stride && other.pointer >= base &&
                other.pointer - base < stride)
                streams[b] = {staged.first, staged.second + other.pointer - base};
        }
    }
    std::string cacheKey((const char *)&key, sizeof(key));
    auto it = pipelines.find(cacheKey);
    id<MTLRenderPipelineState> pipeline;
    if (it != pipelines.end())
        pipeline = it->second;
    else {
        auto vd = [MTLVertexDescriptor vertexDescriptor];
        for (unsigned a : p.translated.vertex.attributes) {
            vd.attributes[a].format = (MTLVertexFormat)key.attr[a][0];
            vd.attributes[a].offset = 0;
            vd.attributes[a].bufferIndex = a + 1;
            vd.layouts[a + 1].stride = key.attr[a][1];
            vd.layouts[a + 1].stepFunction =
                key.attr[a][2] ? MTLVertexStepFunctionPerVertex : MTLVertexStepFunctionConstant;
            vd.layouts[a + 1].stepRate = key.attr[a][2] ? 1 : 0;
        }
        auto d = [MTLRenderPipelineDescriptor new];
        d.vertexFunction = p.vertex;
        d.fragmentFunction = p.fragment;
        d.vertexDescriptor = vd;
        d.depthAttachmentPixelFormat = depth ? depth.pixelFormat : MTLPixelFormatInvalid;
        d.stencilAttachmentPixelFormat = hasStencil(depth) ? depth.pixelFormat : MTLPixelFormatInvalid;
        if (color) {
            auto a = d.colorAttachments[0];
            a.pixelFormat = color.pixelFormat;
            a.writeMask = (MTLColorWriteMask)(((colorMask & 1) ? MTLColorWriteMaskRed : 0) |
                                              ((colorMask & 2) ? MTLColorWriteMaskGreen : 0) |
                                              ((colorMask & 4) ? MTLColorWriteMaskBlue : 0) |
                                              ((colorMask & 8) ? MTLColorWriteMaskAlpha : 0));
            a.blendingEnabled = key.blend;
            a.sourceRGBBlendFactor = blendFactor(srcRGB);
            a.destinationRGBBlendFactor = blendFactor(dstRGB);
            a.sourceAlphaBlendFactor = blendFactor(srcAlpha);
            a.destinationAlphaBlendFactor = blendFactor(dstAlpha);
            a.rgbBlendOperation = blendOp(eqRGB);
            a.alphaBlendOperation = blendOp(eqAlpha);
        }
        NSError *e = nil;
        pipeline = [device newRenderPipelineStateWithDescriptor:d error:&e];
        if (!pipeline) {
            error(e.description.UTF8String);
            return false;
        }
        pipelines.emplace(cacheKey, pipeline);
    }
    uint32_t bc5Mask = 0;
    for (const auto &u : p.resources) {
        int unit = p.samplerUnits[u.slot];
        if (unit >= 0 && unit < MaxTextures && textures[boundTextures[unit][0]].format == 0x8dbd)
            bc5Mask |= 1u << u.slot;
    }
    if (p.bc5Offset != ~0u)
        memcpy(p.uniforms.data() + p.bc5Offset, &bc5Mask, 4);
    auto constants = upload(p.uniforms.data(), p.uniforms.size());
    beginPass();
    if (!encoder)
        return false;
    [encoder setRenderPipelineState:pipeline];
    [encoder setVertexBuffer:constants.first offset:constants.second atIndex:0];
    [encoder setFragmentBuffer:constants.first offset:constants.second atIndex:0];
    for (unsigned a : p.translated.vertex.attributes)
        [encoder setVertexBuffer:streams[a].first offset:streams[a].second atIndex:a + 1];
    for (const auto &u : p.resources) {
        unsigned slot = u.slot;
        int unit = p.samplerUnits[slot];
        if (unit < 0 || unit >= MaxTextures)
            unit = 0;
        auto &tex = textures[boundTextures[unit][u.target]];
        id<MTLTexture> image = tex.image ? tex.image : (u.target == 1 ? whiteCube : whiteTexture);
        auto s = sampler(tex);
        tex.usedFrame = frame + 1;
        if (u.vertex) {
            [encoder setVertexTexture:image atIndex:slot];
            [encoder setVertexSamplerState:s atIndex:slot];
        }
        if (u.fragment) {
            [encoder setFragmentTexture:image atIndex:slot];
            [encoder setFragmentSamplerState:s atIndex:slot];
        }
    }
    struct DepthKey {
        unsigned enabled, write, func, stencil;
        Stencil front, back;
    } dk{};
    dk.enabled = enabled.count(GL_DEPTH_TEST);
    dk.write = depthWrite;
    dk.func = depthFunc;
    dk.stencil = enabled.count(GL_STENCIL_TEST);
    dk.front = frontStencil;
    dk.back = backStencil;
    dk.front.ref = dk.back.ref = 0;
    std::string depthKey((char *)&dk, sizeof(dk));
    auto ds = depthStates.find(depthKey);
    id<MTLDepthStencilState> state;
    if (ds != depthStates.end())
        state = ds->second;
    else {
        auto d = [MTLDepthStencilDescriptor new];
        d.depthCompareFunction = dk.enabled ? compare(depthFunc) : MTLCompareFunctionAlways;
        d.depthWriteEnabled = dk.enabled && depthWrite;
        if (dk.stencil) {
            auto make = [](const Stencil &s) {
                auto d = [MTLStencilDescriptor new];
                d.stencilCompareFunction = compare(s.func);
                d.stencilFailureOperation = stencilOp(s.fail);
                d.depthFailureOperation = stencilOp(s.zfail);
                d.depthStencilPassOperation = stencilOp(s.pass);
                d.readMask = s.read;
                d.writeMask = s.write;
                return d;
            };
            d.frontFaceStencil = make(frontStencil);
            d.backFaceStencil = make(backStencil);
        }
        state = [device newDepthStencilStateWithDescriptor:d];
        depthStates.emplace(depthKey, state);
    }
    [encoder setDepthStencilState:state];
    [encoder setStencilFrontReferenceValue:frontStencil.ref backReferenceValue:backStencil.ref];
    // The vertex shader reverses Y to retain the engine's bottom-origin FBOs.
    // Reverse winding as well, including the front/back stencil classification.
    [encoder setFrontFacingWinding:frontFace == GL_CCW ? MTLWindingClockwise : MTLWindingCounterClockwise];
    [encoder setCullMode:!enabled.count(GL_CULL_FACE) ? MTLCullModeNone
                         : cullFace == GL_FRONT       ? MTLCullModeFront
                                                      : MTLCullModeBack];
    [encoder setDepthBias:enabled.count(GL_POLYGON_OFFSET_FILL) ? depthBias : 0
               slopeScale:enabled.count(GL_POLYGON_OFFSET_FILL) ? slopeBias : 0
                    clamp:0];
    [encoder setBlendColorRed:blendColor[0] green:blendColor[1] blue:blendColor[2] alpha:blendColor[3]];
    [encoder setViewport:MTLViewport{double(viewport[0]), double(viewport[1]), double(viewport[2]),
                                     double(viewport[3]), nearDepth, farDepth}];
    unsigned w = color ? color.width : depth.width, h = color ? color.height : depth.height;
    int x = 0, y = 0, r = w, b = h;
    if (enabled.count(GL_SCISSOR_TEST)) {
        x = std::max(0, scissor[0]);
        y = std::max(0, scissor[1]);
        r = std::min(int(w), scissor[0] + scissor[2]);
        b = std::min(int(h), scissor[1] + scissor[3]);
    }
    if (r <= x || b <= y)
        return false;
    [encoder setScissorRect:MTLScissorRect{(NSUInteger)x, (NSUInteger)y, (NSUInteger)(r - x),
                                           (NSUInteger)(b - y)}];
    ++drawCalls;
    return true;
}
MTLPrimitiveType primitive(GLenum mode) {
    switch (mode) {
    case GL_POINTS:
        return MTLPrimitiveTypePoint;
    case GL_LINES:
        return MTLPrimitiveTypeLine;
    case GL_LINE_STRIP:
        return MTLPrimitiveTypeLineStrip;
    case GL_TRIANGLE_STRIP:
        return MTLPrimitiveTypeTriangleStrip;
    default:
        return MTLPrimitiveTypeTriangle;
    }
}
void copyTexture(id<MTLTexture> src, id<MTLTexture> dst, unsigned srcLevel, unsigned dstLevel, unsigned slice,
                 int sx, int sy, int dx, int dy, int w, int h) {
    if (!src || !dst || w <= 0 || h <= 0)
        return;
    beginCommand();
    endEncoding();
    w = std::min(w, std::min(int(src.width >> srcLevel) - sx, int(dst.width >> dstLevel) - dx));
    h = std::min(h, std::min(int(src.height >> srcLevel) - sy, int(dst.height >> dstLevel) - dy));
    if (w <= 0 || h <= 0)
        return;
    auto b = [command blitCommandEncoder];
    [b copyFromTexture:src
              sourceSlice:0
              sourceLevel:srcLevel
             sourceOrigin:MTLOriginMake(sx, sy, 0)
               sourceSize:MTLSizeMake(w, h, 1)
                toTexture:dst
         destinationSlice:slice
         destinationLevel:dstLevel
        destinationOrigin:MTLOriginMake(dx, dy, 0)];
    [b endEncoding];
}
} // namespace

namespace {
NSString *shaderCachePath(const Program &p) {
    std::string key = "ETQW-Metal-MSL-6\n" + shaders[p.vert].source + shaders[p.frag].source;
    for (auto &b : p.bindings)
        key += b.first + std::to_string(b.second);
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(key.data(), (CC_LONG)key.size(), digest);
    char hex[65];
    for (int i = 0; i < 32; ++i)
        snprintf(hex + i * 2, 3, "%02x", digest[i]);
    NSString *root = shaderCacheDirectory;
    if (!root)
        return nil;
    [[NSFileManager defaultManager] createDirectoryAtPath:root
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:nil];
    return [root stringByAppendingPathComponent:[NSString stringWithFormat:@"%s.plist", hex]];
}
bool loadShaderCache(NSString *path, MetalShaderProgram &p) {
    if (!path)
        return false;
    NSDictionary *d = [NSDictionary dictionaryWithContentsOfFile:path];
    if (!d || ![d[@"vertex"] isKindOfClass:[NSString class]] ||
        ![d[@"fragment"] isKindOfClass:[NSString class]])
        return false;
    p.uniformSize = [d[@"size"] unsignedIntValue];
    p.vertex.source = [d[@"vertex"] UTF8String];
    p.fragment.source = [d[@"fragment"] UTF8String];
    for (NSDictionary *v in d[@"uniforms"]) {
        MetalUniform u;
        u.name = [v[@"name"] UTF8String];
        u.type = [v[@"type"] UTF8String];
        u.offset = [v[@"offset"] unsignedIntValue];
        u.count = [v[@"count"] unsignedIntValue];
        u.stride = [v[@"stride"] unsignedIntValue];
        u.matrixStride = [v[@"matrix"] unsignedIntValue];
        u.sampler = [v[@"sampler"] intValue];
        p.uniforms.push_back(u);
    }
    for (NSNumber *n in d[@"attributes"])
        p.vertex.attributes.push_back(n.unsignedIntValue);
    for (NSNumber *n in d[@"vertexSamplers"])
        p.vertex.samplers.push_back(n.unsignedIntValue);
    for (NSNumber *n in d[@"fragmentSamplers"])
        p.fragment.samplers.push_back(n.unsignedIntValue);
    return true;
}
void saveShaderCache(NSString *path, const MetalShaderProgram &p) {
    if (!path)
        return;
    NSMutableArray *uniforms = [NSMutableArray new], *attributes = [NSMutableArray new],
                   *vs = [NSMutableArray new], *fs = [NSMutableArray new];
    for (auto &u : p.uniforms)
        [uniforms addObject:@{
            @"name" : @(u.name.c_str()),
            @"type" : @(u.type.c_str()),
            @"offset" : @(u.offset),
            @"count" : @(u.count),
            @"stride" : @(u.stride),
            @"matrix" : @(u.matrixStride),
            @"sampler" : @(u.sampler)
        }];
    for (auto a : p.vertex.attributes)
        [attributes addObject:@(a)];
    for (auto a : p.vertex.samplers)
        [vs addObject:@(a)];
    for (auto a : p.fragment.samplers)
        [fs addObject:@(a)];
    [@{
        @"size" : @(p.uniformSize),
        @"vertex" : @(p.vertex.source.c_str()),
        @"fragment" : @(p.fragment.source.c_str()),
        @"uniforms" : uniforms,
        @"attributes" : attributes,
        @"vertexSamplers" : vs,
        @"fragmentSamplers" : fs
    } writeToFile:path
        atomically:YES];
}
void mglLinkProgram(GLuint id) {
    auto &p = programs[id];
    p.translated = {};
    p.locations.clear();
    p.linked = false;
    auto path = shaderCachePath(p);
    if (!loadShaderCache(path, p.translated)) {
        if (!Metal_TranslateShaders(shaders[p.vert].source, shaders[p.frag].source, p.bindings, p.translated,
                                    p.error)) {
            error(p.error.c_str());
            return;
        }
        saveShaderCache(path, p.translated);
    }
    NSError *e = nil;
    auto options = [MTLCompileOptions new];
    options.languageVersion = MTLLanguageVersion2_3;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
    if (@available(macOS 15.0, *))
        options.mathMode = MTLMathModeSafe;
    else
#endif
        options.fastMathEnabled = NO;
    auto v = [device newLibraryWithSource:@(p.translated.vertex.source.c_str()) options:options error:&e];
    if (!v) {
        p.error = e.description.UTF8String;
        error(p.error.c_str());
        return;
    }
    auto f = [device newLibraryWithSource:@(p.translated.fragment.source.c_str()) options:options error:&e];
    if (!f) {
        p.error = e.description.UTF8String;
        error(p.error.c_str());
        return;
    }
    p.vertex = [v newFunctionWithName:@"etqw_vertex"];
    p.fragment = [f newFunctionWithName:@"etqw_fragment"];
    // SPIR-V reports the end of the last member; Metal requires the trailing
    // padding of the struct too, including when the upload ring spills.
    p.uniforms.resize(std::max(16u, (p.translated.uniformSize + 15) & ~15u));
    p.samplerUnits.resize(MaxTextures);
    for (unsigned i = 0; i < p.translated.uniforms.size(); ++i) {
        auto &u = p.translated.uniforms[i];
        p.locations[u.name] = i;
        if (u.count > 1)
            for (unsigned element = 0; element < u.count; ++element)
                p.locations[u.name + "[" + std::to_string(element) + "]"] = i + (element << 16);
    }
    p.resources.clear();
    p.bc5Offset = ~0u;
    for (const auto &u : p.translated.uniforms) {
        if (u.name == "etqw_bc5Mask")
            p.bc5Offset = u.offset;
        if (u.sampler < 0)
            continue;
        bool v = std::find(p.translated.vertex.samplers.begin(), p.translated.vertex.samplers.end(),
                           u.sampler) != p.translated.vertex.samplers.end();
        bool f = std::find(p.translated.fragment.samplers.begin(), p.translated.fragment.samplers.end(),
                           u.sampler) != p.translated.fragment.samplers.end();
        if (v || f)
            p.resources.push_back(
                {unsigned(u.sampler),
                 u.type.find("Cube") != std::string::npos                                                ? 1u
                 : (u.type.find("3D") != std::string::npos || u.type.find("Array") != std::string::npos) ? 2u
                                                                                                         : 0u,
                 v, f});
    }
    p.linked = p.vertex && p.fragment;
}
GLuint mglCreateShader(GLenum t) {
    GLuint n = nextId++;
    shaders[n].type = t;
    return n;
}
GLuint mglCreateProgram() {
    GLuint n = nextId++;
    programs[n] = {};
    return n;
}
void mglShaderSource(GLuint s, GLsizei count, const GLchar *const *strings, const GLint *lengths) {
    auto &v = shaders[s].source;
    v.clear();
    for (int i = 0; i < count; ++i)
        v.append(strings[i], lengths && lengths[i] >= 0 ? size_t(lengths[i]) : strlen(strings[i]));
}
void mglCompileShader(GLuint s) {
    shaders[s].compiled = !shaders[s].source.empty();
}
void mglAttachShader(GLuint p, GLuint s) {
    if (shaders[s].type == GL_VERTEX_SHADER)
        programs[p].vert = s;
    else
        programs[p].frag = s;
}
void mglDetachShader(GLuint, GLuint) {}
void mglDeleteShader(GLuint s) {
    shaders.erase(s);
}
void mglDeleteProgram(GLuint p) {
    programs.erase(p);
    if (currentProgram == p)
        currentProgram = 0;
}
void mglUseProgram(GLuint p) {
    currentProgram = p;
}
void mglBindAttribLocation(GLuint p, GLuint index, const GLchar *name) {
    programs[p].bindings[name] = index;
}
GLint mglGetAttribLocation(GLuint p, const GLchar *name) {
    auto program = programs.find(p);
    if (!p || program == programs.end())
        return -1;
    auto &b = program->second.bindings;
    auto i = b.find(name);
    return i == b.end() ? -1 : i->second;
}
GLint mglGetUniformLocation(GLuint p, const GLchar *name) {
    auto program = programs.find(p);
    if (!p || program == programs.end())
        return -1;
    auto &m = program->second.locations;
    auto i = m.find(name);
    return i == m.end() ? -1 : i->second;
}
void mglGetShaderiv(GLuint s, GLenum name, GLint *out) {
    auto &v = shaders[s];
    *out = name == GL_COMPILE_STATUS         ? v.compiled
           : name == GL_INFO_LOG_LENGTH      ? v.error.size() + 1
           : name == GL_SHADER_SOURCE_LENGTH ? v.source.size() + 1
           : name == GL_SHADER_TYPE          ? v.type
                                             : 0;
}
void mglGetProgramiv(GLuint p, GLenum name, GLint *out) {
    auto &v = programs[p];
    *out = (name == GL_LINK_STATUS || name == GL_VALIDATE_STATUS) ? v.linked
           : name == GL_INFO_LOG_LENGTH                           ? v.error.size() + 1
           : name == GL_ACTIVE_UNIFORMS                           ? v.translated.uniforms.size()
           : name == GL_ACTIVE_ATTRIBUTES                         ? v.bindings.size()
           : name == GL_ACTIVE_UNIFORM_MAX_LENGTH || name == GL_ACTIVE_ATTRIBUTE_MAX_LENGTH ? 256
                                                                                            : 0;
}
void copyString(const std::string &s, GLsizei max, GLsizei *len, GLchar *out) {
    int n = std::max(0, std::min(int(s.size()), max - 1));
    if (out && max > 0) {
        memcpy(out, s.data(), n);
        out[n] = 0;
    }
    if (len)
        *len = n;
}
void mglGetShaderInfoLog(GLuint s, GLsizei max, GLsizei *len, GLchar *out) {
    copyString(shaders[s].error, max, len, out);
}
void mglGetProgramInfoLog(GLuint p, GLsizei max, GLsizei *len, GLchar *out) {
    copyString(programs[p].error, max, len, out);
}
void mglGetShaderSource(GLuint s, GLsizei max, GLsizei *len, GLchar *out) {
    copyString(shaders[s].source, max, len, out);
}
GLenum uniformType(const std::string &s) {
    static const std::map<std::string, GLenum> types = {{"float", GL_FLOAT},
                                                        {"vec2", GL_FLOAT_VEC2},
                                                        {"vec3", GL_FLOAT_VEC3},
                                                        {"vec4", GL_FLOAT_VEC4},
                                                        {"int", GL_INT},
                                                        {"ivec2", GL_INT_VEC2},
                                                        {"ivec3", GL_INT_VEC3},
                                                        {"ivec4", GL_INT_VEC4},
                                                        {"bool", GL_BOOL},
                                                        {"mat2", GL_FLOAT_MAT2},
                                                        {"mat3", GL_FLOAT_MAT3},
                                                        {"mat4", GL_FLOAT_MAT4},
                                                        {"sampler2D", GL_SAMPLER_2D},
                                                        {"samplerCube", GL_SAMPLER_CUBE},
                                                        {"sampler3D", GL_SAMPLER_3D},
                                                        {"sampler2DShadow", GL_SAMPLER_2D_SHADOW}};
    auto i = types.find(s);
    return i == types.end() ? GL_FLOAT_VEC4 : i->second;
}
void mglGetActiveUniform(GLuint p, GLuint index, GLsizei max, GLsizei *length, GLint *size, GLenum *type,
                         GLchar *name) {
    auto &u = programs[p].translated.uniforms;
    if (index >= u.size()) {
        if (length)
            *length = 0;
        return;
    }
    copyString(u[index].name + (u[index].count > 1 ? "[0]" : ""), max, length, name);
    *size = u[index].count;
    *type = uniformType(u[index].type);
}
void mglGetActiveAttrib(GLuint p, GLuint index, GLsizei max, GLsizei *length, GLint *size, GLenum *type,
                        GLchar *name) {
    auto &a = programs[p].bindings;
    auto i = a.begin();
    if (index >= a.size())
        return;
    std::advance(i, index);
    copyString(i->first, max, length, name);
    *size = 1;
    *type = GL_FLOAT_VEC4;
}
void mglValidateProgram(GLuint) {}
void mglGetShaderPrecisionFormat(GLenum, GLenum, GLint *range, GLint *precision) {
    range[0] = range[1] = 127;
    *precision = 23;
}
void uniform(GLint location, GLsizei count, const void *values, int components, bool integer = false,
             int matrix = 0) {
    if (location < 0 || !values || !currentProgram)
        return;
    auto &p = programs[currentProgram];
    unsigned element = unsigned(location) >> 16, index = unsigned(location) & 65535u;
    if (index >= p.translated.uniforms.size())
        return;
    auto &u = p.translated.uniforms[index];
    if (element >= u.count)
        return;
    if (u.sampler >= 0) {
        p.samplerUnits[u.sampler] = *(const int *)values;
        return;
    }
    unsigned stride = u.stride ? u.stride : components * 4;
    unsigned n = std::min(unsigned(count), u.count - element);
    for (unsigned i = 0; i < n; ++i) {
        unsigned off = u.offset + (i + element) * stride;
        if (matrix) {
            for (int col = 0; col < matrix; ++col) {
                unsigned dest = off + col * u.matrixStride;
                if (dest + matrix * 4 <= p.uniforms.size())
                    memcpy(p.uniforms.data() + dest,
                           (const char *)values + (i * matrix * matrix + col * matrix) * 4, matrix * 4);
            }
        } else if (off + components * 4 <= p.uniforms.size())
            memcpy(p.uniforms.data() + off, (const char *)values + i * components * 4, components * 4);
    }
}
#define UNIFORM_VEC(N)                                                                                       \
    void mglUniform##N##fv(GLint l, GLsizei c, const GLfloat *v) {                                           \
        uniform(l, c, v, N);                                                                                 \
    }                                                                                                        \
    void mglUniform##N##iv(GLint l, GLsizei c, const GLint *v) {                                             \
        uniform(l, c, v, N, true);                                                                           \
    }
UNIFORM_VEC(1)
UNIFORM_VEC(2) UNIFORM_VEC(3) UNIFORM_VEC(4)
#undef UNIFORM_VEC
    void mglUniform1f(GLint l, GLfloat a) {
    uniform(l, 1, &a, 1);
}
void mglUniform1i(GLint l, GLint a) {
    uniform(l, 1, &a, 1, true);
}
void mglUniform2f(GLint l, GLfloat a, GLfloat b) {
    float v[] = {a, b};
    uniform(l, 1, v, 2);
}
void mglUniform2i(GLint l, GLint a, GLint b) {
    int v[] = {a, b};
    uniform(l, 1, v, 2, true);
}
void mglUniform3f(GLint l, GLfloat a, GLfloat b, GLfloat c) {
    float v[] = {a, b, c};
    uniform(l, 1, v, 3);
}
void mglUniform3i(GLint l, GLint a, GLint b, GLint c) {
    int v[] = {a, b, c};
    uniform(l, 1, v, 3, true);
}
void mglUniform4f(GLint l, GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
    float v[] = {a, b, c, d};
    uniform(l, 1, v, 4);
}
void mglUniform4i(GLint l, GLint a, GLint b, GLint c, GLint d) {
    int v[] = {a, b, c, d};
    uniform(l, 1, v, 4, true);
}
void mglUniformMatrix2fv(GLint l, GLsizei n, GLboolean, const GLfloat *v) {
    uniform(l, n, v, 4, false, 2);
}
void mglUniformMatrix3fv(GLint l, GLsizei n, GLboolean, const GLfloat *v) {
    uniform(l, n, v, 9, false, 3);
}
void mglUniformMatrix4fv(GLint l, GLsizei n, GLboolean, const GLfloat *v) {
    uniform(l, n, v, 16, false, 4);
}
void mglGenTextures(GLsizei n, GLuint *ids) {
    for (int i = 0; i < n; ++i) {
        ids[i] = nextId++;
        textures[ids[i]] = {};
    }
}
void mglDeleteTextures(GLsizei n, const GLuint *ids) {
    endEncoding();
    for (int i = 0; i < n; ++i)
        textures.erase(ids[i]);
}
void mglBindTexture(GLenum target, GLuint id) {
    boundTextures[activeTexture][textureSlot(target)] = id;
    if (id)
        textures[id].target = target;
}
void mglActiveTexture(GLenum t) {
    activeTexture = std::min(unsigned(t - GL_TEXTURE0), MaxTextures - 1);
}
void mglTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint, GLenum format,
                   GLenum type, const void *data) {
    auto &t = boundTexture(target);
    if (!t.image || (level == 0 && (t.image.width != w || t.image.height != h ||
                                    t.image.pixelFormat != pixelFormat(internal))))
        allocateTexture(t, target, internal, w << level, h << level);
    uploadTexture(t, level, textureSlot(target) == 1 ? target - GL_TEXTURE_CUBE_MAP_POSITIVE_X : 0, 0, 0, w,
                  h, format, type, data);
}
void mglTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei w, GLsizei h, GLenum format,
                      GLenum type, const void *data) {
    uploadTexture(boundTexture(target), level,
                  textureSlot(target) == 1 ? target - GL_TEXTURE_CUBE_MAP_POSITIVE_X : 0, x, y, w, h, format,
                  type, data);
}
void mglTexImage3D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLsizei depth, GLint,
                   GLenum format, GLenum type, const void *data) {
    auto &t = boundTexture(target);
    if (level == 0 || !t.image)
        allocateTexture(t, target, internal, w << level, h << level, depth, target == GL_TEXTURE_3D);
    if (!data)
        return;
    if (type != GL_UNSIGNED_BYTE || format != GL_RGBA) {
        unsupported("TexImage3D format");
        return;
    }
    size_t row = (size_t(w) * 4 + 255) & ~size_t(255), plane = row * h;
    auto staging = upload(nullptr, plane * depth);
    auto p = (char *)staging.first.contents + staging.second;
    for (int z = 0; z < depth; ++z)
        for (int y = 0; y < h; ++y)
            memcpy(p + z * plane + y * row, (const char *)data + (size_t(z) * h + y) * w * 4, w * 4);
    endEncoding();
    auto e = [command blitCommandEncoder];
    if (target == GL_TEXTURE_3D)
        [e copyFromBuffer:staging.first
                   sourceOffset:staging.second
              sourceBytesPerRow:row
            sourceBytesPerImage:plane
                     sourceSize:MTLSizeMake(w, h, depth)
                      toTexture:t.image
               destinationSlice:0
               destinationLevel:level
              destinationOrigin:MTLOriginMake(0, 0, 0)];
    else
        for (int z = 0; z < depth; ++z)
            [e copyFromBuffer:staging.first
                       sourceOffset:staging.second + z * plane
                  sourceBytesPerRow:row
                sourceBytesPerImage:plane
                         sourceSize:MTLSizeMake(w, h, 1)
                          toTexture:t.image
                   destinationSlice:z
                   destinationLevel:level
                  destinationOrigin:MTLOriginMake(0, 0, 0)];
    [e endEncoding];
}
void mglCompressedTexImage2D(GLenum target, GLint level, GLenum format, GLsizei w, GLsizei h, GLint, GLsizei,
                             const void *data) {
    auto &t = boundTexture(target);
    if (!t.image || (level == 0 && (t.image.width != w || t.image.height != h ||
                                    t.image.pixelFormat != pixelFormat(format))))
        allocateTexture(t, target, format, w << level, h << level);
    uploadTexture(t, level, textureSlot(target) == 1 ? target - GL_TEXTURE_CUBE_MAP_POSITIVE_X : 0, 0, 0, w,
                  h, format, 0, data, true);
}
void mglCompressedTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei w, GLsizei h,
                                GLenum format, GLsizei, const void *data) {
    uploadTexture(boundTexture(target), level,
                  textureSlot(target) == 1 ? target - GL_TEXTURE_CUBE_MAP_POSITIVE_X : 0, x, y, w, h, format,
                  0, data, true);
}
void mglTexParameteri(GLenum target, GLenum pname, GLint value) {
    auto &t = boundTexture(target);
    switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
        t.min = value;
        break;
    case GL_TEXTURE_MAG_FILTER:
        t.mag = value;
        break;
    case GL_TEXTURE_WRAP_S:
        t.s = value;
        break;
    case GL_TEXTURE_WRAP_T:
        t.t = value;
        break;
    case GL_TEXTURE_WRAP_R:
        t.r = value;
        break;
    case GL_TEXTURE_MAX_LEVEL:
        t.maxLevel = value;
        break;
    case GL_TEXTURE_COMPARE_MODE:
        t.compareMode = value;
        break;
    case GL_TEXTURE_COMPARE_FUNC:
        t.compareFunc = value;
        break;
    case 0x84fe:
        t.anisotropy = value;
        break;
    default:
        break;
    }
}
void mglTexParameterf(GLenum t, GLenum p, GLfloat v) {
    mglTexParameteri(t, p, v);
}
void mglTexParameteriv(GLenum t, GLenum p, const GLint *v) {
    mglTexParameteri(t, p, *v);
}
void mglTexParameterfv(GLenum t, GLenum p, const GLfloat *v) {
    mglTexParameterf(t, p, *v);
}
void mglGetTexParameteriv(GLenum target, GLenum pname, GLint *value) {
    auto &t = boundTexture(target);
    *value = pname == GL_TEXTURE_MIN_FILTER   ? t.min
             : pname == GL_TEXTURE_MAG_FILTER ? t.mag
             : pname == GL_TEXTURE_WRAP_S     ? t.s
             : pname == GL_TEXTURE_WRAP_T     ? t.t
             : pname == GL_TEXTURE_MAX_LEVEL  ? t.maxLevel
                                              : 0;
}
void mglGetTexParameterfv(GLenum t, GLenum p, GLfloat *v) {
    GLint i;
    mglGetTexParameteriv(t, p, &i);
    *v = i;
}
void mglGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *value) {
    auto &t = boundTexture(target);
    *value = pname == 0x1000   ? std::max(1, int(t.image.width) >> level)
             : pname == 0x1001 ? std::max(1, int(t.image.height) >> level)
             : pname == 0x1003 ? t.format
                               : 0;
}
void mglPixelStorei(GLenum p, GLint v) {
    if (p == GL_UNPACK_ALIGNMENT)
        unpackAlignment = v;
    else if (p == GL_PACK_ALIGNMENT)
        packAlignment = v;
    else if (p == GL_UNPACK_ROW_LENGTH)
        unpackRowLength = v;
}
void mglGenBuffers(GLsizei n, GLuint *ids) {
    for (int i = 0; i < n; ++i) {
        ids[i] = nextId++;
        buffers[ids[i]] = {};
    }
}
void mglDeleteBuffers(GLsizei n, const GLuint *ids) {
    for (int i = 0; i < n; ++i)
        buffers.erase(ids[i]);
}
void mglBindBuffer(GLenum target, GLuint id) {
    if (target == GL_ARRAY_BUFFER)
        arrayBuffer = id;
    else if (target == GL_ELEMENT_ARRAY_BUFFER)
        indexBuffer = id;
}
void mglBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    auto &b = buffers[target == GL_ARRAY_BUFFER ? arrayBuffer : indexBuffer];
    b.size = size;
    b.usage = usage;
    b.usedFrame = 0;
    b.object = size ? [device newBufferWithLength:size options:MTLResourceStorageModeShared] : nil;
    if (data && size)
        memcpy(b.object.contents, data, size);
}
void mglBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    auto &b = buffers[target == GL_ARRAY_BUFFER ? arrayBuffer : indexBuffer];
    if (offset + size > b.size) {
        error("buffer subdata out of range");
        return;
    }
    if (b.usedFrame > completedFrame.load()) {
        auto copy = [device newBufferWithLength:b.size options:MTLResourceStorageModeShared];
        memcpy(copy.contents, b.object.contents, b.size);
        b.object = copy;
        b.usedFrame = 0;
    }
    memcpy((char *)b.object.contents + offset, data, size);
}
void mglGetBufferParameteriv(GLenum t, GLenum p, GLint *v) {
    auto &b = buffers[t == GL_ARRAY_BUFFER ? arrayBuffer : indexBuffer];
    *v = p == GL_BUFFER_SIZE ? b.size : b.usage;
}
void mglEnableVertexAttribArray(GLuint i) {
    if (i < MaxAttributes)
        attributes[i].enabled = true;
}
void mglDisableVertexAttribArray(GLuint i) {
    if (i < MaxAttributes)
        attributes[i].enabled = false;
}
void mglVertexAttribPointer(GLuint i, GLint size, GLenum type, GLboolean norm, GLsizei stride,
                            const void *ptr) {
    if (i >= MaxAttributes)
        return;
    auto &a = attributes[i];
    a.size = size;
    a.type = type;
    a.normalized = norm;
    a.stride = stride;
    a.pointer = (uintptr_t)ptr;
    a.buffer = arrayBuffer;
}
void mglVertexAttrib4f(GLuint i, GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
    if (i < MaxAttributes) {
        float v[] = {a, b, c, d};
        memcpy(attributes[i].constant, v, 16);
    }
}
void mglVertexAttrib4fv(GLuint i, const GLfloat *v) {
    mglVertexAttrib4f(i, v[0], v[1], v[2], v[3]);
}
void mglVertexAttrib3f(GLuint i, GLfloat a, GLfloat b, GLfloat c) {
    mglVertexAttrib4f(i, a, b, c, 1);
}
void mglVertexAttrib3fv(GLuint i, const GLfloat *v) {
    mglVertexAttrib3f(i, v[0], v[1], v[2]);
}
void mglVertexAttrib2f(GLuint i, GLfloat a, GLfloat b) {
    mglVertexAttrib4f(i, a, b, 0, 1);
}
void mglVertexAttrib2fv(GLuint i, const GLfloat *v) {
    mglVertexAttrib2f(i, v[0], v[1]);
}
void mglVertexAttrib1f(GLuint i, GLfloat a) {
    mglVertexAttrib4f(i, a, 0, 0, 1);
}
void mglVertexAttrib1fv(GLuint i, const GLfloat *v) {
    mglVertexAttrib1f(i, *v);
}
void mglDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    if (count <= 0)
        return;
    const void *src = indices;
    id<MTLBuffer> index;
    size_t offset = 0;
    if (indexBuffer) {
        auto &b = buffers[indexBuffer];
        src = (char *)b.object.contents + (uintptr_t)indices;
        index = b.object;
        offset = (uintptr_t)indices;
        b.usedFrame = frame + 1;
    }
    bool clientArrays = false;
    for (unsigned a : programs[currentProgram].translated.vertex.attributes)
        clientArrays |= attributes[a].enabled && !attributes[a].buffer;
    unsigned max = 0;
    if (clientArrays)
        for (int i = 0; i < count; ++i)
            max = std::max(max, type == GL_UNSIGNED_INT     ? ((const uint32_t *)src)[i]
                                : type == GL_UNSIGNED_SHORT ? ((const uint16_t *)src)[i]
                                                            : ((const uint8_t *)src)[i]);
    if (type == GL_UNSIGNED_BYTE) {
        std::vector<uint16_t> expanded(count);
        for (int i = 0; i < count; ++i)
            expanded[i] = ((const uint8_t *)src)[i];
        auto u = upload(expanded.data(), count * 2);
        index = u.first;
        offset = u.second;
    }
    if (!index) {
        auto u = upload(src, count * (type == GL_UNSIGNED_INT ? 4 : 2));
        index = u.first;
        offset = u.second;
    }
    if (bindDraw(max))
        [encoder drawIndexedPrimitives:primitive(mode)
                            indexCount:count
                             indexType:type == GL_UNSIGNED_INT ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16
                           indexBuffer:index
                     indexBufferOffset:offset];
}
void mglDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (count <= 0)
        return;
    if (mode == GL_TRIANGLE_FAN) {
        std::vector<uint32_t> ids;
        for (int i = 1; i < count - 1; ++i) {
            ids.push_back(first);
            ids.push_back(first + i);
            ids.push_back(first + i + 1);
        }
        GLuint old = indexBuffer;
        indexBuffer = 0;
        mglDrawElements(GL_TRIANGLES, ids.size(), GL_UNSIGNED_INT, ids.data());
        indexBuffer = old;
        return;
    }
    if (bindDraw(first + count - 1))
        [encoder drawPrimitives:primitive(mode) vertexStart:first vertexCount:count];
}
void mglEnable(GLenum c) {
    enabled.insert(c);
}
void mglDisable(GLenum c) {
    enabled.erase(c);
}
GLboolean mglIsEnabled(GLenum c) {
    return enabled.count(c);
}
void mglDepthFunc(GLenum f) {
    depthFunc = f;
}
void mglDepthMask(GLboolean b) {
    depthWrite = b;
}
void mglDepthRangef(GLfloat n, GLfloat f) {
    nearDepth = n;
    farDepth = f;
}
void mglColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    colorMask = r | g << 1 | b << 2 | a << 3;
}
void mglBlendFuncSeparate(GLenum a, GLenum b, GLenum c, GLenum d) {
    srcRGB = a;
    dstRGB = b;
    srcAlpha = c;
    dstAlpha = d;
}
void mglBlendFunc(GLenum a, GLenum b) {
    mglBlendFuncSeparate(a, b, a, b);
}
void mglBlendEquationSeparate(GLenum a, GLenum b) {
    eqRGB = a;
    eqAlpha = b;
}
void mglBlendEquation(GLenum a) {
    mglBlendEquationSeparate(a, a);
}
void mglBlendColor(GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
    blendColor[0] = a;
    blendColor[1] = b;
    blendColor[2] = c;
    blendColor[3] = d;
}
void mglFrontFace(GLenum f) {
    frontFace = f;
}
void mglCullFace(GLenum f) {
    cullFace = f;
}
void mglPolygonOffset(GLfloat factor, GLfloat units) {
    depthBias = units;
    slopeBias = factor;
}
void mglScissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    scissor[0] = x;
    scissor[1] = y;
    scissor[2] = w;
    scissor[3] = h;
}
void mglViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    viewport[0] = x;
    viewport[1] = y;
    viewport[2] = w;
    viewport[3] = h;
}
void mglStencilFuncSeparate(GLenum face, GLenum f, GLint ref, GLuint mask) {
    if (face != GL_BACK) {
        frontStencil.func = f;
        frontStencil.ref = ref;
        frontStencil.read = mask;
    }
    if (face != GL_FRONT) {
        backStencil.func = f;
        backStencil.ref = ref;
        backStencil.read = mask;
    }
}
void mglStencilFunc(GLenum f, GLint ref, GLuint mask) {
    mglStencilFuncSeparate(GL_FRONT_AND_BACK, f, ref, mask);
}
void mglStencilOpSeparate(GLenum face, GLenum a, GLenum b, GLenum c) {
    if (face != GL_BACK) {
        frontStencil.fail = a;
        frontStencil.zfail = b;
        frontStencil.pass = c;
    }
    if (face != GL_FRONT) {
        backStencil.fail = a;
        backStencil.zfail = b;
        backStencil.pass = c;
    }
}
void mglStencilOp(GLenum a, GLenum b, GLenum c) {
    mglStencilOpSeparate(GL_FRONT_AND_BACK, a, b, c);
}
void mglStencilMaskSeparate(GLenum face, GLuint m) {
    if (face != GL_BACK)
        frontStencil.write = m;
    if (face != GL_FRONT)
        backStencil.write = m;
}
void mglStencilMask(GLuint m) {
    mglStencilMaskSeparate(GL_FRONT_AND_BACK, m);
}
void partialClear(GLbitfield mask);
void mglClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    clearColor = MTLClearColorMake(r, g, b, a);
}
void mglClearDepthf(GLfloat d) {
    clearDepth = d;
}
void mglClearStencil(GLint s) {
    clearStencil = s;
}
void mglClear(GLbitfield mask) {
    auto target = colorTarget(drawFbo);
    if (!target)
        target = depthTarget(drawFbo);
    if (!target)
        return;
    bool full =
        !enabled.count(GL_SCISSOR_TEST) ||
        (scissor[0] == 0 && scissor[1] == 0 && scissor[2] >= target.width && scissor[3] >= target.height);
    bool masks = (!(mask & GL_COLOR_BUFFER_BIT) || colorMask == 15) &&
                 (!(mask & GL_DEPTH_BUFFER_BIT) || depthWrite) &&
                 (!(mask & GL_STENCIL_BUFFER_BIT) || frontStencil.write == 255);
    if (full && masks)
        beginPass(mask);
    else
        partialClear(mask);
}
void mglFinish() {
    submit(true);
}
void mglFlush() {
    submit(false);
}
GLenum mglGetError() {
    auto e = lastError;
    lastError = GL_NO_ERROR;
    return e;
}
void mglHint(GLenum, GLenum) {}
void mglLineWidth(GLfloat) {}
void mglSampleCoverage(GLfloat, GLboolean) {}
} // namespace

namespace {
void mglGenFramebuffers(GLsizei n, GLuint *ids) {
    for (int i = 0; i < n; ++i) {
        ids[i] = nextId++;
        framebuffers[ids[i]] = {};
    }
}
void mglDeleteFramebuffers(GLsizei n, const GLuint *ids) {
    endEncoding();
    for (int i = 0; i < n; ++i)
        framebuffers.erase(ids[i]);
}
void mglBindFramebuffer(GLenum target, GLuint id) {
    if (target != GL_READ_FRAMEBUFFER) {
        if (drawFbo != id)
            endEncoding();
        drawFbo = id;
    }
    if (target != GL_DRAW_FRAMEBUFFER)
        readFbo = id;
}
void mglGenRenderbuffers(GLsizei n, GLuint *ids) {
    for (int i = 0; i < n; ++i) {
        ids[i] = nextId++;
        renderbuffers[ids[i]] = {};
    }
}
void mglDeleteRenderbuffers(GLsizei n, const GLuint *ids) {
    endEncoding();
    for (int i = 0; i < n; ++i)
        renderbuffers.erase(ids[i]);
}
void mglBindRenderbuffer(GLenum, GLuint id) {
    renderbuffer = id;
}
void mglRenderbufferStorage(GLenum, GLenum format, GLsizei w, GLsizei h) {
    allocateTexture(renderbuffers[renderbuffer], GL_TEXTURE_2D, format, w, h, 1, false);
}
void attach(GLenum target, GLenum point, Attachment value) {
    endEncoding();
    auto &f = framebuffers[target == GL_READ_FRAMEBUFFER ? readFbo : drawFbo];
    if (point == GL_COLOR_ATTACHMENT0)
        f.color = value;
    else if (point == GL_DEPTH_ATTACHMENT)
        f.depth = value;
    else if (point == GL_STENCIL_ATTACHMENT)
        f.stencil = value;
    else if (point == GL_DEPTH_STENCIL_ATTACHMENT)
        f.depth = f.stencil = value;
}
void mglFramebufferTexture2D(GLenum target, GLenum point, GLenum textureTarget, GLuint id, GLint level) {
    attach(target, point,
           {id, false, unsigned(level),
            textureSlot(textureTarget) == 1 ? textureTarget - GL_TEXTURE_CUBE_MAP_POSITIVE_X : 0});
}
void mglFramebufferTextureLayer(GLenum target, GLenum point, GLuint id, GLint level, GLint slice) {
    attach(target, point, {id, false, unsigned(level), unsigned(slice)});
}
void mglFramebufferRenderbuffer(GLenum target, GLenum point, GLenum, GLuint id) {
    attach(target, point, {id, true, 0, 0});
}
GLenum mglCheckFramebufferStatus(GLenum target) {
    GLuint f = target == GL_READ_FRAMEBUFFER ? readFbo : drawFbo;
    return colorTarget(f) || depthTarget(f) ? GL_FRAMEBUFFER_COMPLETE
                                            : GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
}
void mglGetRenderbufferParameteriv(GLenum, GLenum p, GLint *out) {
    auto &t = renderbuffers[renderbuffer];
    *out = p == GL_RENDERBUFFER_WIDTH             ? t.image.width
           : p == GL_RENDERBUFFER_HEIGHT          ? t.image.height
           : p == GL_RENDERBUFFER_INTERNAL_FORMAT ? t.format
           : p == GL_RENDERBUFFER_DEPTH_SIZE      ? t.depth ? 32 : 0
                                                  : 8;
}
void mglGetFramebufferAttachmentParameteriv(GLenum target, GLenum point, GLenum p, GLint *out) {
    auto &f = framebuffers[target == GL_READ_FRAMEBUFFER ? readFbo : drawFbo];
    auto &a = point == GL_COLOR_ATTACHMENT0 ? f.color : point == GL_STENCIL_ATTACHMENT ? f.stencil : f.depth;
    *out = p == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE     ? (a.renderbuffer ? GL_RENDERBUFFER : GL_TEXTURE)
           : p == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME   ? a.id
           : p == GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL ? a.level
                                                          : 0;
}
void mglCopyTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLint sx, GLint sy, GLsizei w,
                          GLsizei h) {
    auto &t = boundTexture(target);
    copyTexture(t.depth ? depthTarget(readFbo) : colorTarget(readFbo), t.image, 0, level,
                textureSlot(target) == 1 ? target - GL_TEXTURE_CUBE_MAP_POSITIVE_X : 0, sx, sy, x, y, w, h);
}
void mglCopyTexImage2D(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei w, GLsizei h,
                       GLint) {
    mglTexImage2D(target, level, format, w, h, 0, format, GL_UNSIGNED_BYTE, nullptr);
    mglCopyTexSubImage2D(target, level, 0, 0, x, y, w, h);
}
id<MTLFunction> blitVertex, blitFragment;
std::map<unsigned, id<MTLRenderPipelineState>> blitPipelines;
const char *blitSource = R"msl(
#include <metal_stdlib>
using namespace metal;
struct V {float4 position [[position]];float2 uv;};
vertex V copyVertex(uint i [[vertex_id]],constant float4 &rect [[buffer(0)]]) {
 float2 p=float2((i<<1)&2,i&2);V v;v.position=float4(p*2-1,0,1);v.uv=rect.xy+float2(p.x,1-p.y)*rect.zw;return v;
}
fragment float4 copyFragment(V v [[stage_in]],texture2d<float> image [[texture(0)]]) {
 constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::linear);return image.sample(s,v.uv);
}
)msl";
void blitColor(id<MTLTexture> src, id<MTLTexture> dst, int sx0, int sy0, int sx1, int sy1, int dx0, int dy0,
               int dx1, int dy1, bool flip = false) {
    if (!src || !dst)
        return;
    beginCommand();
    endEncoding();
    if (!blitVertex) {
        NSError *e = nil;
        auto lib = [device newLibraryWithSource:@(blitSource) options:nil error:&e];
        if (!lib) {
            error(e.description.UTF8String);
            return;
        }
        blitVertex = [lib newFunctionWithName:@"copyVertex"];
        blitFragment = [lib newFunctionWithName:@"copyFragment"];
    }
    id<MTLRenderPipelineState> pipeline = blitPipelines[dst.pixelFormat];
    if (!pipeline) {
        auto d = [MTLRenderPipelineDescriptor new];
        d.vertexFunction = blitVertex;
        d.fragmentFunction = blitFragment;
        d.colorAttachments[0].pixelFormat = dst.pixelFormat;
        NSError *e = nil;
        pipeline = [device newRenderPipelineStateWithDescriptor:d error:&e];
        if (!pipeline) {
            error(e.description.UTF8String);
            return;
        }
        blitPipelines[dst.pixelFormat] = pipeline;
    }
    auto d = [MTLRenderPassDescriptor renderPassDescriptor];
    d.colorAttachments[0].texture = dst;
    d.colorAttachments[0].loadAction = MTLLoadActionLoad;
    d.colorAttachments[0].storeAction = MTLStoreActionStore;
    auto e = [command renderCommandEncoderWithDescriptor:d];
    e.label = @"ETQW scale/present";
    [e setRenderPipelineState:pipeline];
    [e setViewport:MTLViewport{double(dx0), double(dy0), double(dx1 - dx0), double(dy1 - dy0), 0, 1}];
    float rect[] = {float(sx0) / src.width, float(flip ? sy1 : sy0) / src.height,
                    float(sx1 - sx0) / src.width, float(flip ? sy0 - sy1 : sy1 - sy0) / src.height};
    [e setVertexBytes:rect length:sizeof(rect) atIndex:0];
    [e setFragmentTexture:src atIndex:0];
    [e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [e endEncoding];
}
void mglBlitFramebuffer(GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0, GLint dx1,
                        GLint dy1, GLbitfield mask, GLenum) {
    if (mask & GL_COLOR_BUFFER_BIT) {
        auto src = colorTarget(readFbo), dst = colorTarget(drawFbo);
        if (sx1 - sx0 == dx1 - dx0 && sy1 - sy0 == dy1 - dy0 && src.pixelFormat == dst.pixelFormat)
            copyTexture(src, dst, 0, 0, 0, sx0, sy0, dx0, dy0, dx1 - dx0, dy1 - dy0);
        else
            blitColor(src, dst, sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1);
    }
    if (mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
        copyTexture(depthTarget(readFbo), depthTarget(drawFbo), 0, 0, 0, sx0, sy0, dx0, dy0, dx1 - dx0,
                    dy1 - dy0);
}
void mglReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, void *out) {
    auto src = colorTarget(readFbo);
    if (!src || !out || w <= 0 || h <= 0)
        return;
    if (type != GL_UNSIGNED_BYTE || (format != GL_RGBA && format != GL_RGB)) {
        unsupported("ReadPixels format");
        return;
    }
    beginCommand();
    endEncoding();
    size_t row = (size_t(w) * 4 + 255) & ~size_t(255);
    auto b = [device newBufferWithLength:row * h options:MTLResourceStorageModeShared];
    auto e = [command blitCommandEncoder];
    [e copyFromTexture:src
                     sourceSlice:0
                     sourceLevel:0
                    sourceOrigin:MTLOriginMake(x, y, 0)
                      sourceSize:MTLSizeMake(w, h, 1)
                        toBuffer:b
               destinationOffset:0
          destinationBytesPerRow:row
        destinationBytesPerImage:row * h];
    [e endEncoding];
    submit(true);
    int channels = format == GL_RGB ? 3 : 4;
    size_t outRow = (size_t(w) * channels + packAlignment - 1) & ~size_t(packAlignment - 1);
    for (int j = 0; j < h; ++j) {
        auto p = (const unsigned char *)b.contents + j * row;
        auto q = (unsigned char *)out + j * outRow;
        for (int i = 0; i < w; ++i) {
            q[i * channels] = p[i * 4];
            q[i * channels + 1] = p[i * 4 + 1];
            q[i * channels + 2] = p[i * 4 + 2];
            if (channels == 4)
                q[i * 4 + 3] = p[i * 4 + 3];
        }
    }
}
void mglReadBuffer(GLenum) {}
void mglDrawBuffers(GLsizei, const GLenum *) {}
void partialClear(GLbitfield mask) {
    static id<MTLFunction> vertex, fragment, colorFragment;
    static std::map<std::string, id<MTLRenderPipelineState>> cache;
    auto color = colorTarget(drawFbo), depth = depthTarget(drawFbo);
    auto target = color ? color : depth;
    if (!target)
        return;
    if (!vertex) {
        const char *source = R"msl(
#include <metal_stdlib>
using namespace metal;
vertex float4 clearVertex(uint i [[vertex_id]]) {float2 p=float2((i<<1)&2,i&2);return float4(p*2-1,0,1);}
struct O {float4 color [[color(0)]];float depth [[depth(any)]];};
fragment O clearFragment(constant float4 *v [[buffer(0)]]) {return {v[0],v[1].x};}
fragment float4 clearColorFragment(constant float4 *v [[buffer(0)]]) {return v[0];}
)msl";
        NSError *e = nil;
        auto lib = [device newLibraryWithSource:@(source) options:nil error:&e];
        if (!lib) {
            error(e.description.UTF8String);
            return;
        }
        vertex = [lib newFunctionWithName:@"clearVertex"];
        fragment = [lib newFunctionWithName:@"clearFragment"];
        colorFragment = [lib newFunctionWithName:@"clearColorFragment"];
    }
    unsigned write = (mask & GL_COLOR_BUFFER_BIT) ? colorMask : 0;
    auto key = std::to_string(color.pixelFormat) + ":" + std::to_string(depth.pixelFormat) + ":" +
               std::to_string(write);
    auto p = cache[key];
    if (!p) {
        auto d = [MTLRenderPipelineDescriptor new];
        d.vertexFunction = vertex;
        d.fragmentFunction = depth ? fragment : colorFragment;
        d.colorAttachments[0].pixelFormat = color.pixelFormat;
        d.colorAttachments[0].writeMask = (MTLColorWriteMask)(((write & 1) ? MTLColorWriteMaskRed : 0) |
                                                              ((write & 2) ? MTLColorWriteMaskGreen : 0) |
                                                              ((write & 4) ? MTLColorWriteMaskBlue : 0) |
                                                              ((write & 8) ? MTLColorWriteMaskAlpha : 0));
        d.depthAttachmentPixelFormat = depth.pixelFormat;
        d.stencilAttachmentPixelFormat = hasStencil(depth) ? depth.pixelFormat : MTLPixelFormatInvalid;
        NSError *e = nil;
        p = [device newRenderPipelineStateWithDescriptor:d error:&e];
        if (!p) {
            error(e.description.UTF8String);
            return;
        }
        cache[key] = p;
    }
    beginPass();
    [encoder setRenderPipelineState:p];
    auto ds = [MTLDepthStencilDescriptor new];
    ds.depthCompareFunction = MTLCompareFunctionAlways;
    ds.depthWriteEnabled = (mask & GL_DEPTH_BUFFER_BIT) && depthWrite;
    if ((mask & GL_STENCIL_BUFFER_BIT) && hasStencil(depth)) {
        auto s = [MTLStencilDescriptor new];
        s.stencilCompareFunction = MTLCompareFunctionAlways;
        s.depthStencilPassOperation = MTLStencilOperationReplace;
        s.writeMask = frontStencil.write;
        ds.frontFaceStencil = ds.backFaceStencil = s;
    }
    [encoder setDepthStencilState:[device newDepthStencilStateWithDescriptor:ds]];
    [encoder setStencilReferenceValue:clearStencil];
    [encoder setCullMode:MTLCullModeNone];
    [encoder setDepthBias:0 slopeScale:0 clamp:0];
    [encoder setViewport:MTLViewport{0, 0, double(target.width), double(target.height), 0, 1}];
    int x = 0, y = 0, r = target.width, b = target.height;
    if (enabled.count(GL_SCISSOR_TEST)) {
        x = std::max(0, scissor[0]);
        y = std::max(0, scissor[1]);
        r = std::min(r, scissor[0] + scissor[2]);
        b = std::min(b, scissor[1] + scissor[3]);
    }
    if (r <= x || b <= y)
        return;
    [encoder setScissorRect:MTLScissorRect{(NSUInteger)x, (NSUInteger)y, (NSUInteger)(r - x),
                                           (NSUInteger)(b - y)}];
    float values[] = {float(clearColor.red),
                      float(clearColor.green),
                      float(clearColor.blue),
                      float(clearColor.alpha),
                      float(clearDepth),
                      0,
                      0,
                      0};
    [encoder setFragmentBytes:values length:sizeof(values) atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}
const GLubyte *mglGetString(GLenum name) {
    const char *s = "";
    switch (name) {
    case GL_VENDOR:
        s = "Apple";
        break;
    case GL_RENDERER:
        s = "ETQW native Metal";
        break;
    case GL_VERSION:
        s = "OpenGL ES 3.0 command interface / native Metal";
        break;
    case GL_SHADING_LANGUAGE_VERSION:
        s = "OpenGL ES GLSL ES 3.00 to MSL 2.3";
        break;
    case GL_EXTENSIONS:
        s = "GL_EXT_texture_compression_s3tc GL_EXT_texture_filter_anisotropic GL_OES_depth24 "
            "GL_OES_element_index_uint GL_OES_depth_texture GL_OES_packed_depth_stencil GL_OES_texture_npot "
            "GL_EXT_shader_texture_lod";
        break;
    }
    return (const GLubyte *)s;
}
void mglGetIntegerv(GLenum p, GLint *v) {
    switch (p) {
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
    case GL_MAX_RENDERBUFFER_SIZE:
        *v = 16384;
        break;
    case GL_MAX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        *v = 16;
        break;
    case GL_MAX_VERTEX_ATTRIBS:
        *v = 16;
        break;
    case GL_MAX_COLOR_ATTACHMENTS:
    case GL_MAX_DRAW_BUFFERS:
        *v = 1;
        break;
    case GL_MAX_VERTEX_UNIFORM_VECTORS:
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
        *v = 1024;
        break;
    case GL_MAX_VARYING_VECTORS:
        *v = 31;
        break;
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
    case GL_STENCIL_BITS:
        *v = 8;
        break;
    case GL_DEPTH_BITS:
        *v = 32;
        break;
    case GL_CURRENT_PROGRAM:
        *v = currentProgram;
        break;
    case GL_ACTIVE_TEXTURE:
        *v = GL_TEXTURE0 + activeTexture;
        break;
    case GL_TEXTURE_BINDING_2D:
        *v = boundTextures[activeTexture][0];
        break;
    case GL_TEXTURE_BINDING_CUBE_MAP:
        *v = boundTextures[activeTexture][1];
        break;
    case GL_ARRAY_BUFFER_BINDING:
        *v = arrayBuffer;
        break;
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
        *v = indexBuffer;
        break;
    case GL_FRAMEBUFFER_BINDING:
        *v = drawFbo;
        break;
    case GL_READ_FRAMEBUFFER_BINDING:
        *v = readFbo;
        break;
    case GL_RENDERBUFFER_BINDING:
        *v = renderbuffer;
        break;
    case GL_BLEND_SRC_RGB:
        *v = srcRGB;
        break;
    case GL_BLEND_DST_RGB:
        *v = dstRGB;
        break;
    case GL_BLEND_SRC_ALPHA:
        *v = srcAlpha;
        break;
    case GL_BLEND_DST_ALPHA:
        *v = dstAlpha;
        break;
    case GL_BLEND_EQUATION_RGB:
        *v = eqRGB;
        break;
    case GL_BLEND_EQUATION_ALPHA:
        *v = eqAlpha;
        break;
    case GL_SCISSOR_BOX:
        memcpy(v, scissor, sizeof(scissor));
        break;
    case GL_VIEWPORT:
        memcpy(v, viewport, sizeof(viewport));
        break;
    case GL_PACK_ALIGNMENT:
        *v = packAlignment;
        break;
    case GL_UNPACK_ALIGNMENT:
        *v = unpackAlignment;
        break;
    case GL_DEPTH_FUNC:
        *v = depthFunc;
        break;
    case GL_DEPTH_WRITEMASK:
        *v = depthWrite;
        break;
    case GL_CULL_FACE_MODE:
        *v = cullFace;
        break;
    case GL_FRONT_FACE:
        *v = frontFace;
        break;
    case GL_NUM_PROGRAM_BINARY_FORMATS:
        *v = 0;
        break;
    default:
        *v = 0;
        break;
    }
}
void mglGetFloatv(GLenum p, GLfloat *v) {
    if (p == 0x84ff) {
        *v = 16;
        return;
    }
    if (p == GL_COLOR_CLEAR_VALUE) {
        v[0] = clearColor.red;
        v[1] = clearColor.green;
        v[2] = clearColor.blue;
        v[3] = clearColor.alpha;
        return;
    }
    if (p == GL_DEPTH_RANGE) {
        v[0] = nearDepth;
        v[1] = farDepth;
        return;
    }
    GLint i;
    mglGetIntegerv(p, &i);
    *v = i;
}
void mglGetBooleanv(GLenum p, GLboolean *v) {
    if (p == GL_COLOR_WRITEMASK) {
        for (int i = 0; i < 4; ++i)
            v[i] = (colorMask >> i) & 1;
    } else if (p == GL_DEPTH_WRITEMASK)
        *v = depthWrite;
    else
        *v = enabled.count(p);
}
GLboolean mglIsTexture(GLuint id) {
    return textures.count(id);
}
GLboolean mglIsBuffer(GLuint id) {
    return buffers.count(id);
}
GLboolean mglIsFramebuffer(GLuint id) {
    return framebuffers.count(id);
}
GLboolean mglIsRenderbuffer(GLuint id) {
    return renderbuffers.count(id);
}
GLboolean mglIsShader(GLuint id) {
    return shaders.count(id);
}
GLboolean mglIsProgram(GLuint id) {
    // Zero is the unbound program, even if a compatibility query touched its map entry.
    return id != 0 && programs.count(id);
}
void mglGenQueries(GLsizei n, GLuint *ids) {
    for (int i = 0; i < n; ++i) {
        ids[i] = nextId++;
        queries[ids[i]].buffer = [device newBufferWithLength:8 options:MTLResourceStorageModeShared];
    }
}
void mglDeleteQueries(GLsizei n, const GLuint *ids) {
    for (int i = 0; i < n; ++i)
        queries.erase(ids[i]);
}
GLboolean mglIsQuery(GLuint id) {
    return queries.count(id);
}
void mglBeginQuery(GLenum, GLuint id) {
    endEncoding();
    auto &q = queries[id];
    q.buffer = [device newBufferWithLength:8 options:MTLResourceStorageModeShared];
    q.pending = frame + 1;
    activeQuery = id;
}
void mglEndQuery(GLenum) {
    endEncoding();
    activeQuery = 0;
}
void mglGetQueryObjectuiv(GLuint id, GLenum p, GLuint *out) {
    auto &q = queries[id];
    if (p == GL_QUERY_RESULT_AVAILABLE) {
        *out = q.pending <= completedFrame.load();
        return;
    }
    if (q.pending > completedFrame.load())
        submit(true);
    *out = *(uint64_t *)q.buffer.contents ? 1 : 0;
}
void mglGetQueryiv(GLenum, GLenum p, GLint *out) {
    *out = p == GL_CURRENT_QUERY ? activeQuery : 1;
}
void mglDebugMessageControl(GLenum, GLenum, GLenum, GLsizei, const GLuint *, GLboolean) {}
void mglDebugMessageCallback(GLDEBUGPROC, const void *) {}
GLuint mglGetDebugMessageLog(GLuint, GLsizei, GLenum *, GLenum *, GLuint *, GLenum *, GLsizei *, GLchar *) {
    return 0;
}
void mglGetInternalformativ(GLenum, GLenum, GLenum, GLsizei size, GLint *out) {
    if (size)
        *out = 0;
}
} // namespace

// Keep unsupported entry points detectable, rather than silently dropping draws.
namespace fallback {
template <typename T> T missing(const char *name) {
    unsupported(name);
    return T();
}
template <> void missing<void>(const char *name) {
    unsupported(name);
}
#define _OPENGLES3 1
#define QGLPROC(name, ret, args)                                                                             \
    ret name args {                                                                                          \
        return missing<ret>(#name);                                                                          \
    }
#include "../../renderer/qgl_proc.h"
} // namespace fallback

void *MetalBackend_Proc(const char *name) {
#define BIND(n)                                                                                              \
    if (strcmp(name, #n) == 0)                                                                               \
        return (void *)&m##n;
    BIND(glTexImage3D)
    BIND(glGetActiveUniform)
    BIND(glGetActiveAttrib)
    BIND(glActiveTexture)
    BIND(glAttachShader)
    BIND(glBeginQuery)
    BIND(glBindAttribLocation)
    BIND(glBindBuffer)
    BIND(glBindFramebuffer)
    BIND(glBindRenderbuffer)
    BIND(glBindTexture)
    BIND(glBlendColor)
    BIND(glBlendEquation)
    BIND(glBlendEquationSeparate)
    BIND(glBlendFunc)
    BIND(glBlendFuncSeparate)
    BIND(glBlitFramebuffer)
    BIND(glBufferData)
    BIND(glBufferSubData)
    BIND(glCheckFramebufferStatus)
    BIND(glClear)
    BIND(glClearColor)
    BIND(glClearDepthf)
    BIND(glClearStencil)
    BIND(glColorMask)
    BIND(glCompileShader)
    BIND(glCompressedTexImage2D)
    BIND(glCompressedTexSubImage2D)
    BIND(glCopyTexImage2D)
    BIND(glCopyTexSubImage2D)
    BIND(glCreateProgram)
    BIND(glCreateShader)
    BIND(glCullFace)
    BIND(glDebugMessageCallback)
    BIND(glDebugMessageControl)
    BIND(glDeleteBuffers)
    BIND(glDeleteFramebuffers)
    BIND(glDeleteProgram)
    BIND(glDeleteQueries)
    BIND(glDeleteRenderbuffers)
    BIND(glDeleteShader)
    BIND(glDeleteTextures)
    BIND(glDepthFunc)
    BIND(glDepthMask)
    BIND(glDepthRangef)
    BIND(glDetachShader)
    BIND(glDisable)
    BIND(glDisableVertexAttribArray)
    BIND(glDrawArrays)
    BIND(glDrawBuffers)
    BIND(glDrawElements)
    BIND(glEnable)
    BIND(glEnableVertexAttribArray)
    BIND(glEndQuery)
    BIND(glFinish)
    BIND(glFlush)
    BIND(glFramebufferRenderbuffer)
    BIND(glFramebufferTexture2D)
    BIND(glFramebufferTextureLayer)
    BIND(glFrontFace)
    BIND(glGenBuffers)
    BIND(glGenFramebuffers)
    BIND(glGenQueries)
    BIND(glGenRenderbuffers)
    BIND(glGenTextures)
    BIND(glGetAttribLocation)
    BIND(glGetBooleanv)
    BIND(glGetBufferParameteriv)
    BIND(glGetDebugMessageLog)
    BIND(glGetError)
    BIND(glGetFloatv)
    BIND(glGetFramebufferAttachmentParameteriv)
    BIND(glGetIntegerv)
    BIND(glGetInternalformativ)
    BIND(glGetProgramInfoLog)
    BIND(glGetProgramiv)
    BIND(glGetQueryObjectuiv)
    BIND(glGetQueryiv)
    BIND(glGetRenderbufferParameteriv)
    BIND(glGetShaderInfoLog)
    BIND(glGetShaderPrecisionFormat)
    BIND(glGetShaderSource)
    BIND(glGetShaderiv)
    BIND(glGetString)
    BIND(glGetTexLevelParameteriv)
    BIND(glGetTexParameterfv)
    BIND(glGetTexParameteriv)
    BIND(glGetUniformLocation)
    BIND(glHint)
    BIND(glIsBuffer)
    BIND(glIsEnabled)
    BIND(glIsFramebuffer)
    BIND(glIsProgram)
    BIND(glIsQuery)
    BIND(glIsRenderbuffer)
    BIND(glIsShader)
    BIND(glIsTexture)
    BIND(glLineWidth)
    BIND(glLinkProgram)
    BIND(glPixelStorei)
    BIND(glPolygonOffset)
    BIND(glReadBuffer)
    BIND(glReadPixels)
    BIND(glRenderbufferStorage)
    BIND(glSampleCoverage)
    BIND(glScissor)
    BIND(glShaderSource)
    BIND(glStencilFunc)
    BIND(glStencilFuncSeparate)
    BIND(glStencilMask)
    BIND(glStencilMaskSeparate)
    BIND(glStencilOp)
    BIND(glStencilOpSeparate)
    BIND(glTexImage2D)
    BIND(glTexParameterf)
    BIND(glTexParameterfv)
    BIND(glTexParameteri)
    BIND(glTexParameteriv)
    BIND(glTexSubImage2D)
    BIND(glUniform1f)
    BIND(glUniform1fv)
    BIND(glUniform1i)
    BIND(glUniform1iv)
    BIND(glUniform2f)
    BIND(glUniform2fv)
    BIND(glUniform2i)
    BIND(glUniform2iv)
    BIND(glUniform3f)
    BIND(glUniform3fv)
    BIND(glUniform3i)
    BIND(glUniform3iv)
    BIND(glUniform4f)
    BIND(glUniform4fv)
    BIND(glUniform4i)
    BIND(glUniform4iv)
    BIND(glUniformMatrix2fv)
    BIND(glUniformMatrix3fv)
    BIND(glUniformMatrix4fv)
    BIND(glUseProgram)
    BIND(glValidateProgram)
    BIND(glVertexAttrib1f)
    BIND(glVertexAttrib1fv)
    BIND(glVertexAttrib2f)
    BIND(glVertexAttrib2fv)
    BIND(glVertexAttrib3f)
    BIND(glVertexAttrib3fv)
    BIND(glVertexAttrib4f)
    BIND(glVertexAttrib4fv)
    BIND(glVertexAttribPointer)
    BIND(glViewport)
#undef BIND
#define QGLPROC(n, ret, args)                                                                                \
    if (strcmp(name, #n) == 0)                                                                               \
        return (void *)&fallback::n;
#include "../../renderer/qgl_proc.h"
    return nullptr;
}
void MetalBackend_SetCacheDirectory(const char *path) {
    shaderCacheDirectory = path && *path ? [NSString stringWithUTF8String:path] : nil;
}
bool MetalBackend_Active() {
    return device != nil;
}
bool MetalBackend_Init(SDL_Window *w, int width, int height) {
    device = MTLCreateSystemDefaultDevice();
    if (!device) {
        error("Metal device unavailable");
        return false;
    }
    window = w;
    metalView = SDL_Metal_CreateView(w);
    if (!metalView) {
        error(SDL_GetError());
        device = nil;
        return false;
    }
    layer = (__bridge CAMetalLayer *)SDL_Metal_GetLayer(metalView);
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;
    layer.maximumDrawableCount = FrameCount;
    layer.opaque = YES;
    queue = [device newCommandQueue];
    queue.label = @"ETQW raster + ray tracing";
    frameSemaphore = dispatch_semaphore_create(FrameCount);
    for (auto &u : uploads)
        u.buffer = [device newBufferWithLength:UploadCapacity options:MTLResourceStorageModeShared];
    int dw, dh;
    SDL_Metal_GetDrawableSize(w, &dw, &dh);
    layer.drawableSize = CGSizeMake(dw, dh);
    Texture color, depth;
    allocateTexture(color, GL_TEXTURE_2D, GL_RGBA8, dw, dh, 1, false);
    allocateTexture(depth, GL_TEXTURE_2D, GL_DEPTH24_STENCIL8, dw, dh, 1, false);
    screenColor = color.image;
    screenDepth = depth.image;
    unsigned char white[] = {255, 255, 255, 255};
    Texture t;
    allocateTexture(t, GL_TEXTURE_2D, GL_RGBA8, 1, 1, 1, false);
    uploadTexture(t, 0, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, white);
    whiteTexture = t.image;
    Texture cube;
    allocateTexture(cube, GL_TEXTURE_CUBE_MAP, GL_RGBA8, 1, 1, 1, false);
    for (int i = 0; i < 6; ++i)
        uploadTexture(cube, 0, i, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, white);
    whiteCube = cube.image;
    viewport[2] = scissor[2] = width;
    viewport[3] = scissor[3] = height;
    printf("Native Metal renderer: %s; %dx%d Retina drawable; 3 frames in flight; no ANGLE/EGL\n",
           device.name.UTF8String, dw, dh);
    return true;
}
void MetalBackend_DrawableSize(int *width, int *height) {
    SDL_Metal_GetDrawableSize(window, width, height);
}
void MetalBackend_Present(int interval) {
    int w, h;
    SDL_Metal_GetDrawableSize(window, &w, &h);
    if (w <= 0 || h <= 0)
        return;
    layer.displaySyncEnabled = interval != 0;
    if (screenColor.width != w || screenColor.height != h) {
        submit(true);
        Texture c, d;
        allocateTexture(c, GL_TEXTURE_2D, GL_RGBA8, w, h, 1, false);
        allocateTexture(d, GL_TEXTURE_2D, GL_DEPTH24_STENCIL8, w, h, 1, false);
        screenColor = c.image;
        screenDepth = d.image;
        layer.drawableSize = CGSizeMake(w, h);
    }
    beginCommand();
    drawable = [layer nextDrawable];
    if (drawable)
        blitColor(screenColor, drawable.texture, 0, 0, w, h, 0, 0, w, h, true);
    submit(false, true);
    textureBytes = 0;
    if (pool) {
        objc_autoreleasePoolPop(pool);
        pool = nullptr;
    }
}
void MetalBackend_Finish() {
    submit(true);
}
void MetalBackend_EndEncoding() {
    beginCommand();
    endEncoding();
}
void *MetalBackend_Device() {
    return (__bridge void *)device;
}
void *MetalBackend_Queue() {
    return (__bridge void *)queue;
}
void *MetalBackend_CommandBuffer() {
    beginCommand();
    endEncoding();
    return (__bridge void *)command;
}
unsigned MetalBackend_ImportTexture(void *handle) {
    GLuint name = nextId++;
    textures[name].image = (__bridge id<MTLTexture>)handle;
    return name;
}
void MetalBackend_Info() {
    size_t bc = 0, bc5 = 0;
    for (const auto &item : textures) {
        bc += compressed(item.second.image.pixelFormat);
        bc5 += item.second.image.pixelFormat == MTLPixelFormatBC5_RGUnorm;
    }
    printf("Metal renderer: active=%d frames=%lu draws=%lu passes=%lu pipelines=%zu textures=%zu "
           "unsupported=%lu GPU=%.3f ms allocated=%.1f MiB\n",
           device != nil, frame, drawCalls, passCount, pipelines.size(), textures.size(), unsupportedCalls,
           gpuTime.load(), double(device.currentAllocatedSize) / (1024 * 1024));
    printf("Metal compressed textures: %zu BC textures, including %zu BC5 normal maps\n", bc, bc5);
    fflush(stdout);
}
void MetalBackend_Shutdown() {
    submit(true);
    if (frameAcquired) {
        dispatch_semaphore_signal(frameSemaphore);
        frameAcquired = false;
    }
    textures.clear();
    renderbuffers.clear();
    buffers.clear();
    framebuffers.clear();
    shaders.clear();
    programs.clear();
    pipelines.clear();
    depthStates.clear();
    samplers.clear();
    queries.clear();
    blitPipelines.clear();
    spillBuffers.clear();
    screenColor = screenDepth = whiteTexture = whiteCube = nil;
    blitVertex = blitFragment = nil;
    for (auto &u : uploads)
        u.buffer = nil;
    drawable = nil;
    queue = nil;
    device = nil;
    if (metalView)
        SDL_Metal_DestroyView(metalView);
    metalView = nullptr;
    layer = nil;
    window = nullptr;
    if (pool) {
        objc_autoreleasePoolPop(pool);
        pool = nullptr;
    }
}
