#ifndef _MODEL_MD5ANIM_H
#define _MODEL_MD5ANIM_H

#define	MD5ANIM_TX			BIT( 0 )
#define	MD5ANIM_TY			BIT( 1 )
#define	MD5ANIM_TZ			BIT( 2 )
#define	MD5ANIM_QX			BIT( 3 )
#define	MD5ANIM_QY			BIT( 4 )
#define	MD5ANIM_QZ			BIT( 5 )
#define	MD5ANIM_ALL			63

typedef struct md5animHierarchy_s
{
    idStr boneName; // The name of this bone.
    int parentIndex; // The index of this bone’s parent.
    int numComp; // a flag that defines what components are keyframed.
    int frameIndex; // index into the frame data, pointing to the first animated component of this bone
} md5animHierarchy_t;

typedef idBounds md5animBounds_t;

typedef struct md5animFrame_s
{
    float xPos; // The X component of this bone’s XYZ position in relation to the baseframe.
    float yPos; // The Y component of this bone’s XYZ position in relation to the baseframe.
    float zPos; // The Z component of this bone’s XYZ position in relation to the baseframe.
    float xOrient; // The X component of this bone’s XYZ orentation quaternion in relation to the baseframe.
    float yOrient; // The Y component of this bone’s XYZ orentation quaternion in relation to the baseframe.
    float zOrient; // The Z component of this bone’s XYZ orentation quaternion in relation to the baseframe.
} md5animFrame_t;

/*
xPos - The X component of this bone’s XYZ position relative to parent bone’s position.
yPos - The Y component of this bone’s XYZ position relative to parent bone’s position.
zPos - The Z component of this bone’s XYZ position relative to parent bone’s position.
xOrient - The X component of this bone’s XYZ orentation quaternion.
yOrient - The Y component of this bone’s XYZ orentation quaternion.
zOrient - The Z component of this bone’s XYZ orentation quaternion.
 */
typedef md5animFrame_t md5animBaseframe_t;
typedef struct md5animFrames_s
{
    int index; // frame index.
    idList<md5animFrame_t> joints;
} md5animFrames_t;

class idMd5MeshFile;
class idMd5AnimFile
{
public:
    idMd5AnimFile(void);
    void Write(const char *path) const;
    bool Parse(const char *path);
    idList<md5animHierarchy_t> & Hierarchies(void);
    idList<md5animBounds_t> & Bounds(void);
    idList<md5animBaseframe_t> & Baseframe(void);
    idList<md5animFrames_t> & Frames(void);
    md5animFrames_t & Frame(int index);
    idStr & Commandline(void);
    int & FrameRate(void);
    int & NumAnimatedComponents(void);
    void Clear(void);
    void CalcBounds(const idMd5MeshFile &mesh);

    static void CalcFrameBounds(const idMd5MeshFile &mesh, const idMd5AnimFile &anim, int frame, idBounds &bounds);

private:
    int version;
    idStr commandline;
    int frameRate;
    int numAnimatedComponents;
    idList<md5animHierarchy_t> hierarchies;
    idList<md5animBounds_t> bounds;
    idList<md5animBaseframe_t> baseframe;
    idList<md5animFrames_t> frames;

    friend class idMd5MeshFile;
};

ID_INLINE idList<md5animHierarchy_t> & idMd5AnimFile::Hierarchies()
{
    return hierarchies;
}

ID_INLINE idList<md5animBounds_t> & idMd5AnimFile::Bounds()
{
    return bounds;
}

ID_INLINE idList<md5animBaseframe_t> & idMd5AnimFile::Baseframe()
{
    return baseframe;
}

ID_INLINE idList<md5animFrames_t> & idMd5AnimFile::Frames()
{
    return frames;
}

ID_INLINE md5animFrames_t & idMd5AnimFile::Frame(int index)
{
    return frames[index];
}

ID_INLINE idStr & idMd5AnimFile::Commandline()
{
    return commandline;
}

ID_INLINE int & idMd5AnimFile::FrameRate(void)
{
    return frameRate;
}

ID_INLINE int & idMd5AnimFile::NumAnimatedComponents(void)
{
    return numAnimatedComponents;
}

#endif
