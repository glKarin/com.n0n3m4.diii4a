#ifndef _MODEL_GLTF_H
#define _MODEL_GLTF_H

#include "Model_glb.h"

class idMd5MeshFile;

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-animation

#define GLTF_SCALAR "SCALAR"
#define GLTF_VEC2 "VEC2"
#define GLTF_VEC3 "VEC3"
#define GLTF_VEC4 "VEC4"
#define GLTF_MAT2 "MAT2"
#define GLTF_MAT3 "MAT3"
#define GLTF_MAT4 "MAT4"

#define GLTF_TRANSLATION "translation"
#define GLTF_ROTATION "rotation"
#define GLTF_SCALE "scale"

#pragma pack( push, 1 )

// Metadata about the glTF asset.
typedef struct gltfAsset_s {
    //idStr copyright; // A copyright message suitable for display to credit the content creator.
    //idStr generator; // Tool that generated this glTF model. Useful for debugging.
    idStr version; // The glTF version in the form of <major>.<minor> that this asset targets.
    //idStr minVersion; // The minimum glTF version in the form of <major>.<minor> that this asset targets. This property MUST NOT be greater than the asset version.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.

    float full_version;
    int major_version;
    int minor_version;
} gltfAsset_t;

// The root nodes of a scene.
typedef struct gltfScene_s {
    idList<int> nodes; // The indices of each root node.
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfScene_t;

// The root nodes of a scene.
typedef struct gltfNode_s {
    //int camera; // The index of the camera referenced by this node.
    idList<int> children; // The indices of this node’s children.
    int skin;  // The index of the skin referenced by this node.
    float matrix[16]; // = [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1] // A floating-point 4x4 transformation matrix stored in column-major order.
    int mesh; // The index of the mesh in this node.
    float rotation[4]; // = [0,0,0,1] // The node’s unit quaternion rotation in the order (x, y, z, w), where w is the scalar.
    float scale[3]; // = [1,1,1] // The node’s non-uniform scale, given as the scaling factors along the x, y, and z axes.
    float translation[3]; // = [0,0,0] // number The node’s translation along the x, y, and z axes.
    //idList<int> weights; // The weights of the instantiated morph target. The number of array elements MUST match the number of morph targets of the referenced mesh. When defined, mesh MUST also be defined.
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfNode_t;

typedef struct gltfPrimitiveAttribute_s {
    int POSITION;
    int NORMAL;
    int TANGENT;
    idList<int> TEXCOORD_;
    idList<int> COLOR_;
    idList<int> JOINTS_;
    idList<int> WEIGHTS_;
} gltfPrimitiveAttribute_t;

typedef enum gltfPrimitiveMode_e {
    GLTF_POINTS = 0,
    GLTF_LINES = 1,
    GLTF_LINE_LOOP = 2,
    GLTF_LINE_STRIP = 3,
    GLTF_TRIANGLES = 4,
    GLTF_TRIANGLE_STRIP = 5,
    GLTF_TRIANGLE_FAN = 6,
} gltfPrimitiveMode_t;

// Geometry to be rendered with the given material.
typedef struct gltfPrimitive_s {
    gltfPrimitiveAttribute_t attributes; // A plain JSON object, where each key corresponds to a mesh attribute semantic and each value is the index of the accessor containing attribute’s data.
    int indices; // The index of the accessor that contains the vertex indices.
    int material; // The index of the material to apply to this primitive when rendering.
    int mode; // = 4 // The topology type of primitives to render.
    // targets // An array of morph targets.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfPrimitive_t;

// A set of primitives to be rendered. Its global transform is defined by a node that references it.
typedef struct gltfMesh_s {
    idList<gltfPrimitive_t> primitives; // An array of primitives, each defining geometry to be rendered.
    idList<int> weights; // Array of weights to be applied to the morph targets. The number of array elements MUST match the number of morph targets.
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfMesh_t;

// A buffer points to binary geometry, animation, or skins.
typedef struct gltfBuffer_s {
    idList<byte> uri; // The URI (or IRI) of the buffer.
    int byteLength; // The length of the buffer in bytes.
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfBuffer_t;

// A view into a buffer generally representing a subset of the buffer.
typedef struct gltfBufferView_s {
    int buffer; // The index of the buffer.
    int byteOffset; // The offset into the buffer in bytes.
    int byteLength; // The length of the bufferView in bytes.
    int byteStride; // The stride, in bytes.
    int target; // The hint representing the intended GPU buffer type to use with this buffer view.
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfBufferView_t;

// The material appearance of a primitive.
typedef struct gltfMaterial_s {
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
    // pbrMetallicRoughness // A set of parameter values that are used to define the metallic-roughness material model from Physically Based Rendering (PBR) methodology. When undefined, all the default values of pbrMetallicRoughness MUST apply.
    // normalTexture // The tangent space normal texture.
    // occlusionTexture // The occlusion texture.
    // emissiveTexture; // The emissive texture.
    // emissiveFactor; [3] = {0, 0, 0} // The factors for the emissive color of the material.
    // alphaMode; // = "OPAQUE" // The alpha rendering mode of the material.
    // alphaCutoff; // = 0.5 // The alpha cutoff value of the material.
    // doubleSided; // = false // Specifies whether the material is double sided.
} gltfMaterial_t;

typedef enum gltfComponentType_e {
    GLTF_BYTE = 5120,
    GLTF_UNSIGNED_BYTE = 5121,
    GLTF_SHORT = 5122,
    GLTF_UNSIGNED_SHORT = 5123,
    GLTF_UNSIGNED_INT = 5125,
    GLTF_FLOAT = 5126,
} gltfComponentType_t;

// A typed view into a buffer view that contains raw binary data.
typedef struct gltfAccessor_s {
    int bufferView; // The index of the bufferView.
    int byteOffset; // = 0 // The offset relative to the start of the buffer view in bytes.
    int componentType; // The datatype of the accessor’s components.
    bool normalized; // = false // Specifies whether integer data values are normalized before usage.
    int count; // The number of elements referenced by this accessor.
    idStr type; // Specifies if the accessor’s elements are scalars, vectors, or matrices.
    //float max[16]; // Maximum value of each component in this accessor.
    //float min[16]; // Minimum value of each component in this accessor.
    // sparse // Sparse storage of elements that deviate from their initialization value.
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfAccessor_t;

// The descriptor of the animated property.
typedef struct gltfTarget_s {
    int node; // The index of the node to animate. When undefined, the animated object MAY be defined by an extension.
    idStr path; // The name of the node’s TRS property to animate, or the "weights" of the Morph Targets it instantiates. For the "translation" property, the values that are provided by the sampler are the translation along the X, Y, and Z axes. For the "rotation" property, the values are a quaternion in the order (x, y, z, w), where w is the scalar. For the "scale" property, the values are the scaling factors along the X, Y, and Z axes.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfTarget_t;

// An animation channel combines an animation sampler with a target property being animated.
typedef struct gltfChannel_s {
    int sampler; // The index of a sampler in this animation used to compute the value for the target.
    gltfTarget_t target; // The descriptor of the animated property.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfChannel_t;

// An animation sampler combines timestamps with a sequence of output values and defines an interpolation algorithm.
typedef struct gltfSampler_s {
    int input; // The index of an accessor containing keyframe timestamps.
    idStr interpolation; // "LINEAR" // Interpolation algorithm.
    int output; // The index of an accessor, containing keyframe output values.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfSampler_t;

// A keyframe animation.
typedef struct gltfAnimation_s {
    idList<gltfChannel_t> channels; // An array of animation channels. An animation channel combines an animation sampler with a target property being animated. Different channels of the same animation MUST NOT have the same targets.
    idList<gltfSampler_t> samplers; // An array of animation samplers. An animation sampler combines timestamps with a sequence of output values and defines an interpolation algorithm.
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfAnimation_t;

// Joints and matrices defining a skin.
typedef struct gltfSkin_s {
    int inverseBindMatrices; // The index of the accessor containing the floating-point 4x4 inverse-bind matrices.
    int skeleton; // The index of the node used as a skeleton root.
    idList<int> joints; // Indices of skeleton nodes, used as joints in this skin.
    idStr name; // The user-defined name of this object.
    // extensions // JSON object with extension-specific objects.
    // extras // Application-specific data.
} gltfSkin_t;

#pragma pack( pop )

typedef enum gltfSectionType_e
{
    GLTF_ASSET,
    GLTF_SCENES,
    GLTF_SCENE,
    GLTF_NODE,
    GLTF_BUFFERVIEW,
    GLTF_BUFFER,
    GLTF_ACCESSOR,
    GLTF_MATERIAL,
    GLTF_MESH,
    GLTF_SKIN,
    GLTF_ANIMATION,
} gltfSectionType_t;

union json_u;

class idModelGLTF
{
public:
    idModelGLTF(void);
    ~idModelGLTF(void);
    bool Parse(const char *filePath);
    bool ParseGLTF(const char *filePath);
    bool ParseGLB(const char *filePath);
	bool ParseMemory(idLexer &lexer, const char *filePath = NULL);
    void Print(void) const;
    bool ToMd5Mesh(idMd5MeshFile &md5mesh, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
    bool ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int animIndex, float scale, bool addOrigin, const idVec3 *animOffset, const idMat3 *animRotation) const;
    bool ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, const char *animName, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
    int ToMd5AnimList(idList<idMd5AnimFile> &md5anim, idMd5MeshFile &md5mesh, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
    const char * GetAnim(unsigned int index) const;
    int GetAnimCount(void) const;
#ifdef _MODEL_OBJ
    bool ToObj(objModel_t &objModel, bool keepDup = false) const;
#endif

private:
    void Clear(void);
    void MarkType(int type);
    bool IsTypeMarked(int type) const;
    int ReadAsset(void);
    int ReadScene(void);
    int ReadScenes(void);
    int ReadNodes(void);
    int ReadBufferViews(void);
    int ReadBuffers(const char *filePath = NULL);
    int ReadAccessors(void);
    int ReadMaterials(void);
    int ReadMeshes(void);
    int ReadSkins(void);
    int ReadChannels(const union json_u &object, idList<gltfChannel_t> &channels);
    int ReadSamplers(const union json_u &object, idList<gltfSampler_t> &samplers);
    int ReadAnimations(void);
	void ReadPrimitives(const union json_u &object, gltfPrimitive_t &primitive);
    int DecodeBase64Str(const char *base64, const char *mime, idList<byte> &bytes);
    int FileToBytes(const char *path, idList<byte> &bytes);
	int ReadHeader(glbHeader_t &header);
	int ReadChunk(glbChunk_t &chunk, int mask = 0);
    int GroupTriangle(idList<idList<idDrawVert> > &verts, idList<idList<int> > &faces, idStrList &mats, bool keepDup = false) const;
    int GetMeshNode(void) const;
    const char * FindParentNode(int index) const;

private:
    gltfAsset_t asset;
    idList<gltfScene_t> scenes;
    int scene;
    idList<gltfNode_t> nodes;
    idList<gltfBufferView_t> bufferViews;
    idList<gltfBuffer_t> buffers;
    idList<gltfAccessor_t> accessors;
    idList<gltfMaterial_t> materials;
    idList<gltfMesh_t> meshes;
    idList<gltfSkin_t> skins;
    idList<gltfAnimation_t> animations;
    union json_u *json;
	idFile *file;
    int types;

	friend class idRenderModelStatic;

private:
    struct AccessorHelper
    {
        AccessorHelper(void);
        AccessorHelper(const idModelGLTF *p, int index = -1);
        AccessorHelper(const idModelGLTF *p, const idList<int> indexes);

        void SetIndex(int index);

        template <class T>
        const T * Offset(int index) const {
            if(!ptr)
                return NULL;
            int offset = bufferView->byteStride > 0 ? bufferView->byteStride : size;
            const T *res = (const T *)(ptr + index * offset);
            return res;
        }

        template <class U>
        int ToList(int index, idList<U> &list) const;

        template <class T, class U>
        int TypeToList(int index, idList<U> &list) const;

        template <class U>
        int ToArray(int index, U array[], int count) const;

        template <class T, class U>
        int TypeToArray(int index, U array[], int count) const;

        int GetComponentSize(void) const;
        int GetComponentNum(void) const;

        int Count(void) const {
            return accessor->count;
        }

        int ComponentType(void) const {
            return accessor->componentType;
        }

        int accessorIndex;
        const idModelGLTF * gltf;
        const gltfAccessor_t *accessor;
        const gltfBufferView_t *bufferView;
        const gltfBuffer_t *buffer;
        const byte *ptr;
        int size; // accessor->componentType * accessor->type
    };
};

template <class U>
int idModelGLTF::AccessorHelper::ToList(int index, idList<U> &list) const {
    if(!ptr)
        return 0;

    int num;

    switch (ComponentType()) {
        case GLTF_BYTE:
            num = TypeToList<signed char, U>(index, list);
            break;
        case GLTF_UNSIGNED_BYTE:
            num = TypeToList<unsigned char, U>(index, list);
            break;
        case GLTF_SHORT:
            num = TypeToList<short, U>(index, list);
            break;
        case GLTF_UNSIGNED_SHORT:
            num = TypeToList<unsigned short, U>(index, list);
            break;
        case GLTF_FLOAT:
            num = TypeToList<float, U>(index, list);
            break;
        case GLTF_UNSIGNED_INT:
        default:
            num = TypeToList<unsigned int, U>(index, list);
            break;
    }

    return num;
}

template <class T, class U>
int idModelGLTF::AccessorHelper::TypeToList(int index, idList<U> &list) const {
    if(!ptr)
        return 0;

    int num = GetComponentNum();
    list.SetNum(num);
    const T *rawArr = Offset<T>(index);
    for(int o = 0; o < num; o++)
    {
        list[o] = (U)(T)rawArr[o];
    }

    return list.Num();
}

template <class U>
int idModelGLTF::AccessorHelper::ToArray(int index, U array[], int count) const {
    if(!ptr)
        return 0;

    int num;

    switch (ComponentType()) {
        case GLTF_BYTE:
            num = TypeToArray<signed char, U>(index, array, count);
            break;
        case GLTF_UNSIGNED_BYTE:
            num = TypeToArray<unsigned char, U>(index, array, count);
            break;
        case GLTF_SHORT:
            num = TypeToArray<short, U>(index, array, count);
            break;
        case GLTF_UNSIGNED_SHORT:
            num = TypeToArray<unsigned short, U>(index, array, count);
            break;
        case GLTF_FLOAT:
            num = TypeToArray<float, U>(index, array, count);
            break;
        case GLTF_UNSIGNED_INT:
        default:
            num = TypeToArray<unsigned int, U>(index, array, count);
            break;
    }

    return num;
}

template <class T, class U>
int idModelGLTF::AccessorHelper::TypeToArray(int index, U array[], int count) const {
    if(!ptr)
        return 0;

    const T *rawArr = Offset<T>(index);
    for(int o = 0; o < count; o++)
    {
        array[o] = (U)(T)rawArr[o];
    }

    return count;
}

#endif
