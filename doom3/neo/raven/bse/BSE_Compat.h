#ifndef _BSE_COMPAT_H
#define _BSE_COMPAT_H

#define COLOR_COMPONENT_FLOAT_TO_BYTE(f) ((byte)(f * 255.0f))

namespace BSE
{

union bytesAndInt32_u
{
	byte b[4];
	uint32_t i;
};

ID_INLINE uint32_t PackColor(byte r, byte g, byte b, byte a)
{
	bytesAndInt32_u u;
	u.b[0] = r;
	u.b[1] = g;
	u.b[2] = b;
	u.b[3] = a;
	return u.i;
}

ID_INLINE uint32_t PackColor(float r, float g, float b, float a)
{
	return PackColor(COLOR_COMPONENT_FLOAT_TO_BYTE(r), COLOR_COMPONENT_FLOAT_TO_BYTE(g), COLOR_COMPONENT_FLOAT_TO_BYTE(b), COLOR_COMPONENT_FLOAT_TO_BYTE(a));
}

ID_INLINE idMat3 Transposed(const idMat3 &m)
{
    idMat3 ret = m;
    ret.Transpose();
    return ret;
}

ID_INLINE idVec3 Normalized(const idVec3 &v)
{
    idVec3 ret = v;
    ret.Normalize();
    return ret;
}

ID_INLINE float NormalizeSafely(idVec3 &v)
{
    if(v.IsZero())
        return 0.0f;
    return v.Normalize();
}

ID_INLINE idVec3 NormalizedSafely(const idVec3 &v)
{
    idVec3 ret = v;
    NormalizeSafely(ret);
    return ret;
}

// vector3 multiply
ID_INLINE idVec3 operator^(const idVec3 &a, const idVec3 &b)
{
    return idVec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

ID_INLINE idVec3 Mult(const idVec3 &a, const idVec3 &b)
{
    return idVec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

ID_INLINE float Dot(const idVec3 &a, const idVec3 &b)
{
    return a * b;
}

ID_INLINE int RandIndex(int max)
{
    return rvRandom::irand(0, max - 1);
}

ID_INLINE bool ContainsBounds(const idBounds &a, const idBounds &b) {
    return a.Contains(b);
}

ID_INLINE idVec3 GetSize(const idBounds &b) {
    return b.Size();
}

ID_INLINE void GetBounds(const idRenderModel *model, idBounds &b) {
    b = model->Bounds();
}

ID_INLINE rvAngles RvAngles(const idAngles &a)
{
    return rvAngles(DEG2RAD(a[0]), DEG2RAD(a[1]), DEG2RAD(a[2]));
}

ID_INLINE idAngles IdAngles(const rvAngles &a)
{
    return idAngles(RAD2DEG(a[0]), RAD2DEG(a[1]), RAD2DEG(a[2]));
}

ID_INLINE void Resize(idWinding &a, int n)
{
    a.SetNumPoints(n);
}

ID_INLINE void AddSurface(rvRenderModelBSE* model, int id, const idMaterial* mat, srfTriangles_s* tri, int)
{
    modelSurface_t ret;
    ret.id = id;
    ret.shader = mat;
    ret.geometry = tri;
    model->AddSurface( ret );
}

ID_INLINE idMat4 FromMat3(const idMat3 &rot, const idVec3 &pos)
{
    return idMat4(rot, pos);
}

ID_INLINE idMat3 Scale(const idVec3 &fac)
{
    idMat3 mat = mat3_identity;

    mat[0][0] *= fac[0];

    mat[1][1] *= fac[1];

    mat[2][2] *= fac[2];

    return mat;
}

#define BSE_VERT_NORMAL 0
#if BSE_VERT_NORMAL == 1
#define BSE_SETUP_VERT_NORMAL(v, p) (v).normal = p;
#elif BSE_VERT_NORMAL == 2
#define BSE_SETUP_VERT_NORMAL(v, p) (v).normal = v.xyz;
#elif BSE_VERT_NORMAL == 3
#define BSE_SETUP_VERT_NORMAL(v, p) (v).normal = BSE::Normalized(v.xyz);
#else // 0
#define BSE_SETUP_VERT_NORMAL(v, p)
#endif

void AppendToSurface(const idRenderModel *model, srfTriangles_t* tri, const idMat4 &mat, uint32_t packed);

int GetTexelCount(const idMaterial *material);

};

#endif
