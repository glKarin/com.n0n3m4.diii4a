#ifndef _MODEL_PSA_H
#define _MODEL_PSA_H

#include "Model_psk.h"

#define PSK_CHUNK_ID_TOTAL_LENGTH 20
#define PSK_CHUNK_ID_LENGTH 8

class idMd5MeshFile;
class idMd5AnimFile;

#pragma pack( push, 1 )
// Bones (VBone .. VJointPos ) Name|Flgs|NumChld|PrntIdx|Qw|Qx|Qy|Qz|LocX|LocY|LocZ|Lngth|XSize|YSize|ZSize
// 64s3i11f
// 64s56x
typedef pskHeader_t psaHeader_t;
typedef pskBone_t psaBone_t;

// AniminfoBinary
// 64s64s4i3f3i
// total_bones == psaBone_t's num
// key_quotum = total_bones * num_raw_frames
typedef struct psaAnimInfo_s
{
    char action_name[64];
    char group_name[64];
    int total_bones;
    int root_include;
    int key_compression_style;
    int key_quotum;
    float key_reduction;
    float track_time;
    float anim_rate;
    int start_bone;
    int first_raw_frame;
    int num_raw_frames;
} psaAnimInfo_t;

// Raw keys (VQuatAnimKey) 3f vec, 4f quat, 1f time
// 3f4f4x
typedef struct psaAnimKey_s
{
    float posx;
    float posy;
    float posz;
    float quatx;
    float quaty;
    float quatz;
    float quatw;
    float time;
} psaAnimKey_t;
#pragma pack( pop )

typedef enum psaHeaderType_e
{
    ANIMHEAD = 0,
    BONENAME, // 120
    ANIMINFO, // 168
    ANIMKEYS, // 32
    SCALEKEY, // 16
    PSAHT_TOTAL,
} psaHeaderType_t;

class idModelPsk;
class idModelPsa
{
public:
    idModelPsa(void);
    ~idModelPsa(void);
    bool Parse(const char *psaPath);
    bool ToMd5Anim(const idModelPsk &psk, idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int flags = 0, float scale = -1.0f, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
    void Print(void) const;

private:
    void Clear(void);
    void MarkType(int type);
    bool IsTypeMarked(int type) const;
    int ReadHeader(psaHeader_t &header);
    int ReadBones(void);
    int ReadAnimInfos(void);
    int ReadAnimKeys(void);
    int Skip(void);

private:
    idList<psaBone_t> bones;
    idList<psaAnimInfo_t> animInfos;
    idList<psaAnimKey_t> animKeys;
    idFile *file;
    int types;
    pskHeader_t header;

    friend class idModelPsk;
};

#endif
