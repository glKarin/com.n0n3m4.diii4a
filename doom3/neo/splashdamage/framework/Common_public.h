// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __COMMON_PUBLIC_H__
#define __COMMON_PUBLIC_H__

// tool tips
typedef struct toolTip_s {
    int id;
    char *tip;
} toolTip_t;

#define PROC_FILE_EXT			"proc"
#define PROCB_FILE_EXT			"procb"
#define ENTITY_FILE_EXT			"entities"
#define BOT_ENTITY_FILE_EXT		"bot_entities"

#define STUFF_FILE_EXT			".clust"
#define STUFFB_FILE_EXT			".clustb"
#define STUFF_FILE_ID			"Version 2"

// shared between the renderer, game, and Maya export DLL
#define MD5_VERSION_STRING		"MD5Version"
#define MD5_MESH_EXT			"md5mesh"
#define MD5_ANIM_EXT			"md5anim"
#define MD5_CAMERA_EXT			"md5camera"

const int MD5_VERSION			= 11;

#define _arraycount( array ) ( sizeof( array ) / sizeof( array[ 0 ] ) )

class idInterpreter;
class idProgram;
class sdDeclLocStr;
class idSoundWorld;

struct vidmode_t {
    const char*		description;
    int				width, height;
    int				aspectRatio;
    bool			available;
};


typedef enum {
    TOOL_NOCACHE_MEDIA,
    TOOL_NOLOAD_IMAGES,
    TOOL_MAX
} eToolFlag_t;

#include "../../framework/CmdSystem.h"
#include "../../framework/CVarSystem.h"
#include "../../framework/Common.h"

#endif /* !__COMMON_PUBLIC_H__ */
