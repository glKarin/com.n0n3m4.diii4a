#ifndef _MODEL_GLB_H
#define _MODEL_GLB_H

#define GLB_MAGIC 0x46546C67

#define GLB_CHUNK_TYPE_JSON 0x4E4F534A
#define GLB_CHUNK_TYPE_BIN 0x004E4942

#pragma pack( push, 1 )

// header
typedef struct glbHeader_s {
	uint32_t magic;
	uint32_t version;
	uint32_t length;
} glbHeader_t;

// chunk
typedef struct glbChunk_s {
	uint32_t chunkLength;
	uint32_t chunkType;
	idList<byte> chunkData;
} glbChunk_t;

#pragma pack( pop )

#endif
