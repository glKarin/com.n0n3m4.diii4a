
#ifndef MODEL_PSK_H
#define MODEL_PSK_H

typedef struct pskchunk_s
{
	// id is one of the following:
	// .psk:
	// ACTRHEAD (recordsize = 0, numrecords = 0)
	// PNTS0000 (recordsize = 12, pskpnts_t)
	// VTXW0000 (recordsize = 16, pskvtxw_t)
	// FACE0000 (recordsize = 12, pskface_t)
	// MATT0000 (recordsize = 88, pskmatt_t)
	// REFSKELT (recordsize = 120, pskboneinfo_t)
	// RAWWEIGHTS (recordsize = 12, pskrawweights_t)
	// .psa:
	// ANIMHEAD (recordsize = 0, numrecords = 0)
	// BONENAMES (recordsize = 120, pskboneinfo_t)
	// ANIMINFO (recordsize = 168, pskaniminfo_t)
	// ANIMKEYS (recordsize = 32, pskanimkeys_t)
	char id[20];
	// in .psk always 0x1e83b9
	// in .psa always 0x2e
	int version;
	int recordsize;
	int numrecords;
}
pskchunk_t;

typedef struct pskpnts_s
{
	float origin[3];
}
pskpnts_t;

typedef struct pskvtxw_s
{
	unsigned short pntsindex; // index into PNTS0000 chunk
	unsigned char unknown1[2]; // seems to be garbage
	float texcoord[2];
	unsigned char mattindex; // index into MATT0000 chunk
	unsigned char unknown2; // always 0?
	unsigned char unknown3[2]; // seems to be garbage
}
pskvtxw_t;

typedef struct pskface_s
{
	unsigned short vtxwindex[3]; // triangle
	unsigned char mattindex; // index into MATT0000 chunk
	unsigned char unknown; // seems to be garbage
	unsigned int group; // faces seem to be grouped, possibly for smoothing?
}
pskface_t;

typedef struct pskmatt_s
{
	char name[64];
	int unknown[6]; // observed 0 0 0 0 5 0
}
pskmatt_t;

typedef struct pskpose_s
{
	float quat[4];
	float origin[3];
	float unknown; // probably a float, always seems to be 0
	float size[3];
}
pskpose_t;

typedef struct pskboneinfo_s
{
	char name[64];
	int unknown1;
	int numchildren;
	int parent; // root bones have 0 here
	pskpose_t basepose;
}
pskboneinfo_t;

typedef struct pskrawweights_s
{
	float weight;
	int pntsindex;
	int boneindex;
}
pskrawweights_t;

typedef struct pskaniminfo_s
{
	char name[64];
	char group[64];
	int numbones;
	int unknown1;
	int unknown2;
	int unknown3;
	float unknown4;
	float playtime; // not really needed
	float fps; // frames per second
	int unknown5;
	int firstframe;
	int numframes;
	// firstanimkeys = (firstframe + frameindex) * numbones
}
pskaniminfo_t;

typedef struct pskanimkeys_s
{
	float origin[3];
	float quat[4];
	float frametime;
}
pskanimkeys_t;

#endif

