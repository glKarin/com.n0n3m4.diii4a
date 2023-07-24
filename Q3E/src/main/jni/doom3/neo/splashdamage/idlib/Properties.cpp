// Copyright (C) 2007 Id Software, Inc.
//


#include "./precompiled.h"
#pragma hdrstop

idBlockAlloc< sdProperties::sdProperty, 64> sdProperties::sdPropertyHandler::propertyAllocator;
extern const char* const sdProperties::propertyTypeStrings[] = { "int", "float", "bool", "string", "wstring", "vec2", "vec3", "vec4", "color3", "color4", "angles" };
