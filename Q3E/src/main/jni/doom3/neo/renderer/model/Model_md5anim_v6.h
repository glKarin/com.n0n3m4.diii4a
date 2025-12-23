#ifndef _MODEL_MD5ANIM_V6_H
#define _MODEL_MD5ANIM_V6_H

typedef struct md5animV6Channel_s
{
    int index;
    idStr joint;
    idStr attribute;
    float starttime;
    float endtime;
    float framerate;
    int strings;
    int range[2];
    idList<float> keys;
} md5animV6Channel_t;

class idModelMd5animV6
{
public:
    idModelMd5animV6(void);
    bool Parse(const char *path);
    void Print(void) const;
    bool ToMd5Anim(const idModelMd5meshV6 &meshv6, idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int flags = 0, float scale = -1.0f, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;

private:
    void Clear(void);

private:
    int version;
    idStr commandline;
    idList<md5animV6Channel_t> channels;

    friend class idModelMd5meshV6;
};

#endif
