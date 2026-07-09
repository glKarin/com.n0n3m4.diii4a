/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../../idlib/precompiled.h"
#pragma hdrstop

#include "AASFile.h"
#include "AASFile_local.h"

//karin: only load/write max reach count per area, else raise error in game library
#define HARM_AAS_SAFETY_RW 0
#if HARM_AAS_SAFETY_RW
static idCVar harm_aas_areaReachSafety("harm_aas_areaReachSafety", "0", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_TOOL, "Keep max reach per area under limit. 1=only read AAS; 2=only write AAS; 3=read and write AAS.", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3>);
#endif

/*
===============================================================================

	idReachability

===============================================================================
*/

/*
================
Reachability_Write
================
*/
bool Reachability_Write(idFile *fp, idReachability *reach)
{
#ifdef _SPLASHDAMAGE
	fp->WriteFloatString("\t\t%d %d (%d %d %d) (%d %d %d) %d %d",
						 (int) reach->travelType, (int) reach->toAreaNum, reach->start[0], reach->start[1], reach->start[2],
						 reach->end[0], reach->end[1], reach->end[2], reach->edgeNum, (int) reach->travelTime);
#else
	fp->WriteFloatString("\t\t%d %d (%f %f %f) (%f %f %f) %d %d",
	                     (int) reach->travelType, (int) reach->toAreaNum, reach->start.x, reach->start.y, reach->start.z,
	                     reach->end.x, reach->end.y, reach->end.z, reach->edgeNum, (int) reach->travelTime);
#endif
	return true;
}

/*
================
Reachability_Read
================
*/
bool Reachability_Read(idLexer &src, idReachability *reach)
{
	reach->travelType = src.ParseInt();
	reach->toAreaNum = src.ParseInt();
#ifdef _SPLASHDAMAGE
	reach->start[0] = src.ParseInt();
	reach->start[1] = src.ParseInt();
	reach->start[2] = src.ParseInt();
	reach->end[0] = src.ParseInt();
	reach->end[1] = src.ParseInt();
	reach->end[2] = src.ParseInt();
#else
	src.Parse1DMatrix(3, reach->start.ToFloatPtr());
	src.Parse1DMatrix(3, reach->end.ToFloatPtr());
#endif
	reach->edgeNum = src.ParseInt();
	reach->travelTime = src.ParseInt();
	return true;
}

/*
================
idReachability::CopyBase
================
*/
void idReachability::CopyBase(idReachability &reach)
{
	travelType = reach.travelType;
	toAreaNum = reach.toAreaNum;
#ifdef _SPLASHDAMAGE
	travelFlags = reach.travelFlags;
	fromAreaNum = reach.fromAreaNum;
	areaTTOfsAndNumber = reach.areaTTOfsAndNumber;
	start[0] = reach.start[0];
	start[1] = reach.start[1];
	start[2] = reach.start[2];
	end[0] = reach.end[0];
	end[1] = reach.end[1];
	end[2] = reach.end[2];
#else
	start = reach.start;
	end = reach.end;
#endif
	edgeNum = reach.edgeNum;
	travelTime = reach.travelTime;
}


/*
===============================================================================

	idReachability_Special

===============================================================================
*/

/*
================
Reachability_Special_Write
================
*/
bool Reachability_Special_Write(idFile *fp, idReachability_Special *reach)
{
	int i;
	const idKeyValue *keyValue;

	fp->WriteFloatString("\n\t\t{\n");

	for (i = 0; i < reach->dict.GetNumKeyVals(); i++) {
		keyValue = reach->dict.GetKeyVal(i);
		fp->WriteFloatString("\t\t\t\"%s\" \"%s\"\n", keyValue->GetKey().c_str(), keyValue->GetValue().c_str());
	}

	fp->WriteFloatString("\t\t}\n");

	return true;
}

/*
================
Reachability_Special_Read
================
*/
bool Reachability_Special_Read(idLexer &src, idReachability_Special *reach)
{
	idToken key, value;

	src.ExpectTokenString("{");

	while (src.ReadToken(&key)) {
		if (key == "}") {
			return true;
		}

		src.ExpectTokenType(TT_STRING, 0, &value);
		reach->dict.Set(key, value);
	}

	return false;
}

/*
===============================================================================

	idAASSettings

===============================================================================
*/

/*
============
idAASSettings::idAASSettings
============
*/
idAASSettings::idAASSettings(void)
{
	numBoundingBoxes = 1;
	boundingBoxes[0] = idBounds(idVec3(-16, -16, 0), idVec3(16, 16, 72));
	usePatches = false;
#ifdef _RAVEN
// jmarshall - aas 1.08
	generateAllFaces = false;
	generateTacticalFeatures = false;
// jmarshall end
#endif
	writeBrushMap = false;
	playerFlood = false;
	noOptimize = false;
	allowSwimReachabilities = false;
	allowFlyReachabilities = false;
#ifdef _SPLASHDAMAGE
	fileExtension = "aas_player";
#else
	fileExtension = "aas48";
#endif
	// physics settings
	gravity = idVec3(0, 0, -1066);
	gravityDir = gravity;
	gravityValue = gravityDir.Normalize();
	invGravityDir = -gravityDir;
	maxStepHeight = 14.0f;
	maxBarrierHeight = 32.0f;
	maxWaterJumpHeight = 20.0f;
	maxFallHeight = 64.0f;
	minFloorCos = 0.7f;
	// fixed travel times
	tt_barrierJump = 100;
	tt_startCrouching = 100;
	tt_waterJump = 100;
	tt_startWalkOffLedge = 100;
#ifdef _SPLASHDAMAGE
	type = AAS_PLAYER;
	boundingBox = idBounds(idVec3(-16, -16, 0), idVec3(16, 16, 72));
	primitiveModeBrush = AAS_PRIMITIVE_MODE_DEFAULT;
	primitiveModePatch = AAS_PRIMITIVE_MODE_NEVER;
	primitiveModeModel = AAS_PRIMITIVE_MODE_NEVER;
	primitiveModeTerrain = AAS_PRIMITIVE_MODE_ALWAYS;
	minHighCeiling = 80.0f;
	groundSpeed = 256.0f;
	waterSpeed = 150.0f;
	ladderSpeed = 50.0f;
	wallCornerEdgeRadius = 0.0f;
	ledgeCornerEdgeRadius = 16.0f;
	obstaclePVSRadius = 1024.0f;
	tt_startLadderClimb = 100;
#endif
}

/*
============
idAASSettings::ParseBool
============
*/
bool idAASSettings::ParseBool(idLexer &src, bool &b)
{
	if (!src.ExpectTokenString("=")) {
		return false;
	}

	b = src.ParseBool();
	return true;
}

/*
============
idAASSettings::ParseInt
============
*/
bool idAASSettings::ParseInt(idLexer &src, int &i)
{
	if (!src.ExpectTokenString("=")) {
		return false;
	}

	i = src.ParseInt();
	return true;
}

/*
============
idAASSettings::ParseFloat
============
*/
bool idAASSettings::ParseFloat(idLexer &src, float &f)
{
	if (!src.ExpectTokenString("=")) {
		return false;
	}

	f = src.ParseFloat();
	return true;
}

/*
============
idAASSettings::ParseVector
============
*/
bool idAASSettings::ParseVector(idLexer &src, idVec3 &vec)
{
	if (!src.ExpectTokenString("=")) {
		return false;
	}

	return (src.Parse1DMatrix(3, vec.ToFloatPtr()) != 0);
}

/*
============
idAASSettings::ParseBBoxes
============
*/
bool idAASSettings::ParseBBoxes(idLexer &src)
{
	idToken token;
	idBounds bounds;

	numBoundingBoxes = 0;

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	while (src.ReadToken(&token)) {
		if (token == "}") {
			return true;
		}

		src.UnreadToken(&token);
		src.Parse1DMatrix(3, bounds[0].ToFloatPtr());

		if (!src.ExpectTokenString("-")) {
			return false;
		}

		src.Parse1DMatrix(3, bounds[1].ToFloatPtr());

		boundingBoxes[numBoundingBoxes++] = bounds;
	}

	return false;
}

/*
============
idAASSettings::FromParser
============
*/
bool idAASSettings::FromParser(idLexer &src)
{
	idToken token;

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	// parse the file
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (token == "}") {
			break;
		}

		if (token == "bboxes") {
			if (!ParseBBoxes(src)) {
				return false;
			}
		} else if (token == "usePatches") {
			if (!ParseBool(src, usePatches)) {
				return false;
			}
#ifdef _RAVEN // quake4 aas file
// jmarshall: AAS 1.08
		} else if (token == "generateTacticalFeatures") {
			if (!ParseBool(src, generateTacticalFeatures)) { return false; }
		} else if (token == "generateAllFaces") {
			if (!ParseBool(src, generateAllFaces)) { return false; }
// jmarshall end
#endif
		} else if (token == "writeBrushMap") {
			if (!ParseBool(src, writeBrushMap)) {
				return false;
			}
		} else if (token == "playerFlood") {
			if (!ParseBool(src, playerFlood)) {
				return false;
			}
		} else if (token == "allowSwimReachabilities") {
			if (!ParseBool(src, allowSwimReachabilities)) {
				return false;
			}
		} else if (token == "allowFlyReachabilities") {
			if (!ParseBool(src, allowFlyReachabilities)) {
				return false;
			}
		} else if (token == "fileExtension") {
			src.ExpectTokenString("=");
			src.ExpectTokenType(TT_STRING, 0, &token);
			fileExtension = token;
		} else if (token == "gravity") {
			ParseVector(src, gravity);
			gravityDir = gravity;
			gravityValue = gravityDir.Normalize();
			invGravityDir = -gravityDir;
		} else if (token == "maxStepHeight") {
			if (!ParseFloat(src, maxStepHeight)) {
				return false;
			}
		} else if (token == "maxBarrierHeight") {
			if (!ParseFloat(src, maxBarrierHeight)) {
				return false;
			}
		} else if (token == "maxWaterJumpHeight") {
			if (!ParseFloat(src, maxWaterJumpHeight)) {
				return false;
			}
		} else if (token == "maxFallHeight") {
			if (!ParseFloat(src, maxFallHeight)) {
				return false;
			}
		} else if (token == "minFloorCos") {
			if (!ParseFloat(src, minFloorCos)) {
				return false;
			}
		} else if (token == "tt_barrierJump") {
			if (!ParseInt(src, tt_barrierJump)) {
				return false;
			}
		} else if (token == "tt_startCrouching") {
			if (!ParseInt(src, tt_startCrouching)) {
				return false;
			}
		} else if (token == "tt_waterJump") {
			if (!ParseInt(src, tt_waterJump)) {
				return false;
			}
		} else if (token == "tt_startWalkOffLedge") {
			if (!ParseInt(src, tt_startWalkOffLedge)) {
				return false;
			}
		} else {
			src.Error("invalid token '%s'", token.c_str());
		}
	}

	if (numBoundingBoxes <= 0) {
		src.Error("no valid bounding box");
	}

	return true;
}

/*
============
idAASSettings::FromFile
============
*/
bool idAASSettings::FromFile(const idStr &fileName)
{
	idLexer src(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT);
	idStr name;

	name = fileName;

	common->Printf("loading %s\n", name.c_str());

	if (!src.LoadFile(name)) {
		common->Error("WARNING: couldn't load %s\n", name.c_str());
		return false;
	}

	if (!src.ExpectTokenString("settings")) {
		common->Error("%s is not a settings file", name.c_str());
		return false;
	}

	if (!FromParser(src)) {
		common->Error("failed to parse %s", name.c_str());
		return false;
	}

	return true;
}

/*
============
idAASSettings::FromDict
============
*/
bool idAASSettings::FromDict(const char *name, const idDict *dict)
{
	idBounds bounds;

	if (!dict->GetVector("mins", "0 0 0", bounds[ 0 ])) {
		common->Error("Missing 'mins' in entityDef '%s'", name);
	}

	if (!dict->GetVector("maxs", "0 0 0", bounds[ 1 ])) {
		common->Error("Missing 'maxs' in entityDef '%s'", name);
	}

	numBoundingBoxes = 1;
	boundingBoxes[0] = bounds;

	if (!dict->GetBool("usePatches", "0", usePatches)) {
		common->Error("Missing 'usePatches' in entityDef '%s'", name);
	}

	if (!dict->GetBool("writeBrushMap", "0", writeBrushMap)) {
		common->Error("Missing 'writeBrushMap' in entityDef '%s'", name);
	}

	if (!dict->GetBool("playerFlood", "0", playerFlood)) {
		common->Error("Missing 'playerFlood' in entityDef '%s'", name);
	}

	if (!dict->GetBool("allowSwimReachabilities", "0", allowSwimReachabilities)) {
		common->Error("Missing 'allowSwimReachabilities' in entityDef '%s'", name);
	}

	if (!dict->GetBool("allowFlyReachabilities", "0", allowFlyReachabilities)) {
		common->Error("Missing 'allowFlyReachabilities' in entityDef '%s'", name);
	}

	if (!dict->GetString("fileExtension", "", fileExtension)) {
		common->Error("Missing 'fileExtension' in entityDef '%s'", name);
	}

	if (!dict->GetVector("gravity", "0 0 -1066", gravity)) {
		common->Error("Missing 'gravity' in entityDef '%s'", name);
	}

	gravityDir = gravity;
	gravityValue = gravityDir.Normalize();
	invGravityDir = -gravityDir;

	if (!dict->GetFloat("maxStepHeight", "0", maxStepHeight)) {
		common->Error("Missing 'maxStepHeight' in entityDef '%s'", name);
	}

	if (!dict->GetFloat("maxBarrierHeight", "0", maxBarrierHeight)) {
		common->Error("Missing 'maxBarrierHeight' in entityDef '%s'", name);
	}

	if (!dict->GetFloat("maxWaterJumpHeight", "0", maxWaterJumpHeight)) {
		common->Error("Missing 'maxWaterJumpHeight' in entityDef '%s'", name);
	}

	if (!dict->GetFloat("maxFallHeight", "0", maxFallHeight)) {
		common->Error("Missing 'maxFallHeight' in entityDef '%s'", name);
	}

	if (!dict->GetFloat("minFloorCos", "0", minFloorCos)) {
		common->Error("Missing 'minFloorCos' in entityDef '%s'", name);
	}

	if (!dict->GetInt("tt_barrierJump", "0", tt_barrierJump)) {
		common->Error("Missing 'tt_barrierJump' in entityDef '%s'", name);
	}

	if (!dict->GetInt("tt_startCrouching", "0", tt_startCrouching)) {
		common->Error("Missing 'tt_startCrouching' in entityDef '%s'", name);
	}

	if (!dict->GetInt("tt_waterJump", "0", tt_waterJump)) {
		common->Error("Missing 'tt_waterJump' in entityDef '%s'", name);
	}

	if (!dict->GetInt("tt_startWalkOffLedge", "0", tt_startWalkOffLedge)) {
		common->Error("Missing 'tt_startWalkOffLedge' in entityDef '%s'", name);
	}

	return true;
}


/*
============
idAASSettings::WriteToFile
============
*/
bool idAASSettings::WriteToFile(idFile *fp) const
{
	int i;

	fp->WriteFloatString("{\n");
	fp->WriteFloatString("\tbboxes\n\t{\n");

	for (i = 0; i < numBoundingBoxes; i++) {
		fp->WriteFloatString("\t\t(%f %f %f)-(%f %f %f)\n", boundingBoxes[i][0].x, boundingBoxes[i][0].y,
		                     boundingBoxes[i][0].z, boundingBoxes[i][1].x, boundingBoxes[i][1].y, boundingBoxes[i][1].z);
	}

	fp->WriteFloatString("\t}\n");
	fp->WriteFloatString("\tusePatches = %d\n", usePatches);
	fp->WriteFloatString("\twriteBrushMap = %d\n", writeBrushMap);
	fp->WriteFloatString("\tplayerFlood = %d\n", playerFlood);
	fp->WriteFloatString("\tallowSwimReachabilities = %d\n", allowSwimReachabilities);
	fp->WriteFloatString("\tallowFlyReachabilities = %d\n", allowFlyReachabilities);
#ifdef _RAVEN // quake4 aas file
// jmarshall - AAS 1.08
	//k fp->WriteFloatString("\tgenerateAllFaces = 0\n");
	fp->WriteFloatString("\tgenerateTacticalFeatures = 0\n");
// jmarshall end
#endif
	fp->WriteFloatString("\tfileExtension = \"%s\"\n", fileExtension.c_str());
	fp->WriteFloatString("\tgravity = (%f %f %f)\n", gravity.x, gravity.y, gravity.z);
	fp->WriteFloatString("\tmaxStepHeight = %f\n", maxStepHeight);
	fp->WriteFloatString("\tmaxBarrierHeight = %f\n", maxBarrierHeight);
	fp->WriteFloatString("\tmaxWaterJumpHeight = %f\n", maxWaterJumpHeight);
	fp->WriteFloatString("\tmaxFallHeight = %f\n", maxFallHeight);
	fp->WriteFloatString("\tminFloorCos = %f\n", minFloorCos);
	fp->WriteFloatString("\ttt_barrierJump = %d\n", tt_barrierJump);
	fp->WriteFloatString("\ttt_startCrouching = %d\n", tt_startCrouching);
	fp->WriteFloatString("\ttt_waterJump = %d\n", tt_waterJump);
	fp->WriteFloatString("\ttt_startWalkOffLedge = %d\n", tt_startWalkOffLedge);
	fp->WriteFloatString("}\n");
	return true;
}

/*
============
idAASSettings::ValidForBounds
============
*/
bool idAASSettings::ValidForBounds(const idBounds &bounds) const
{
	int i;

	for (i = 0; i < 3; i++) {
		if (bounds[0][i] < boundingBoxes[0][0][i]) {
			return false;
		}

		if (bounds[1][i] > boundingBoxes[0][1][i]) {
			return false;
		}
	}

	return true;
}

/*
============
idAASSettings::ValidEntity
============
*/
bool idAASSettings::ValidEntity(const char *classname) const
{
	idStr			use_aas;
	idVec3			size;
	idBounds		bounds;

	if (playerFlood) {
		if (!strcmp(classname, "info_player_start") || !strcmp(classname , "info_player_deathmatch") || !strcmp(classname, "func_teleporter")) {
			return true;
		}
	}

	const idDeclEntityDef *decl = static_cast<const idDeclEntityDef *>(declManager->FindType(DECL_ENTITYDEF, classname, false));

	if (decl && decl->dict.GetString("use_aas", NULL, use_aas) && !fileExtension.Icmp(use_aas)) {
		if (decl->dict.GetVector("mins", NULL, bounds[0])) {
			decl->dict.GetVector("maxs", NULL, bounds[1]);
		} else if (decl->dict.GetVector("size", NULL, size)) {
			bounds[ 0 ].Set(size.x * -0.5f, size.y * -0.5f, 0.0f);
			bounds[ 1 ].Set(size.x * 0.5f, size.y * 0.5f, size.z);
		}

		if (!ValidForBounds(bounds)) {
			common->Error("%s cannot use %s\n", classname, fileExtension.c_str());
		}

		return true;
	}

	return false;
}


/*
===============================================================================

	idAASFileLocal

===============================================================================
*/

#define AAS_LIST_GRANULARITY	1024
#define AAS_INDEX_GRANULARITY	4096
#define AAS_PLANE_GRANULARITY	4096
#define AAS_VERTEX_GRANULARITY	4096
#define AAS_EDGE_GRANULARITY	4096

/*
================
idAASFileLocal::idAASFileLocal
================
*/
idAASFileLocal::idAASFileLocal(void)
{
	planeList.SetGranularity(AAS_PLANE_GRANULARITY);
	vertices.SetGranularity(AAS_VERTEX_GRANULARITY);
	edges.SetGranularity(AAS_EDGE_GRANULARITY);
	edgeIndex.SetGranularity(AAS_INDEX_GRANULARITY);
	faces.SetGranularity(AAS_LIST_GRANULARITY);
	faceIndex.SetGranularity(AAS_INDEX_GRANULARITY);
	areas.SetGranularity(AAS_LIST_GRANULARITY);
	nodes.SetGranularity(AAS_LIST_GRANULARITY);
	portals.SetGranularity(AAS_LIST_GRANULARITY);
	portalIndex.SetGranularity(AAS_INDEX_GRANULARITY);
	clusters.SetGranularity(AAS_LIST_GRANULARITY);
#ifdef _RAVEN
	featureIndexes.SetGranularity(AAS_INDEX_GRANULARITY);
	features.SetGranularity(AAS_LIST_GRANULARITY);
#endif
}

/*
================
idAASFileLocal::~idAASFileLocal
================
*/
idAASFileLocal::~idAASFileLocal(void)
{
	int i;
	idReachability *reach, *next;

	for (i = 0; i < areas.Num(); i++) {
		for (reach = areas[i].reach; reach; reach = next) {
			next = reach->next;
#if !defined(_SPLASHDAMAGE) //karin: allocated by idList
			delete reach;
#endif
		}
	}
}

/*
================
idAASFileLocal::Clear
================
*/
void idAASFileLocal::Clear(void)
{
	planeList.Clear();
	vertices.Clear();
	edges.Clear();
	edgeIndex.Clear();
	faces.Clear();
	faceIndex.Clear();
	areas.Clear();
	nodes.Clear();
	portals.Clear();
	portalIndex.Clear();
	clusters.Clear();
#ifdef _RAVEN
	featureIndexes.Clear();
	features.Clear();
#endif
#ifdef _SPLASHDAMAGE
	obstaclePVS.Clear();
	reachabilityNames.Clear();
#endif
}

/*
================
idAASFileLocal::Write
================
*/
bool idAASFileLocal::Write(const idStr &fileName, unsigned int mapFileCRC)
{
	int i, num;
	idFile *aasFile;
	idReachability *reach;

	common->Printf("[Write AAS]\n");
	common->Printf("writing %s\n", fileName.c_str());

	name = fileName;
	crc = mapFileCRC;

	aasFile = fileSystem->OpenFileWrite(fileName, "fs_devpath");

	if (!aasFile) {
		common->Error("Error opening %s", fileName.c_str());
		return false;
	}

	aasFile->WriteFloatString("%s \"%s\"\n\n", AAS_FILEID, AAS_FILEVERSION);
	aasFile->WriteFloatString("%u\n\n", mapFileCRC);

	// write out the settings
	aasFile->WriteFloatString("settings\n");
	settings.WriteToFile(aasFile);

	// write out planes
	aasFile->WriteFloatString("planes %d {\n", planeList.Num());

	for (i = 0; i < planeList.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %f %f %f %f )\n", i,
		                          planeList[i].Normal().x, planeList[i].Normal().y, planeList[i].Normal().z, planeList[i].Dist());
	}

	aasFile->WriteFloatString("}\n");

	// write out vertices
	aasFile->WriteFloatString("vertices %d {\n", vertices.Num());

	for (i = 0; i < vertices.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %f %f %f )\n", i, vertices[i].x, vertices[i].y, vertices[i].z);
	}

	aasFile->WriteFloatString("}\n");

	// write out edges
	aasFile->WriteFloatString("edges %d {\n", edges.Num());

	for (i = 0; i < edges.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %d %d )\n", i, edges[i].vertexNum[0], edges[i].vertexNum[1]);
	}

	aasFile->WriteFloatString("}\n");

	// write out edgeIndex
	aasFile->WriteFloatString("edgeIndex %d {\n", edgeIndex.Num());

	for (i = 0; i < edgeIndex.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %d )\n", i, edgeIndex[i]);
	}

	aasFile->WriteFloatString("}\n");

	// write out faces
	aasFile->WriteFloatString("faces %d {\n", faces.Num());

	for (i = 0; i < faces.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %d %d %d %d %d %d )\n", i, faces[i].planeNum, faces[i].flags,
		                          faces[i].areas[0], faces[i].areas[1], faces[i].firstEdge, faces[i].numEdges);
	}

	aasFile->WriteFloatString("}\n");

	// write out faceIndex
	aasFile->WriteFloatString("faceIndex %d {\n", faceIndex.Num());

	for (i = 0; i < faceIndex.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %d )\n", i, faceIndex[i]);
	}

	aasFile->WriteFloatString("}\n");

	// write out areas
	aasFile->WriteFloatString("areas %d {\n", areas.Num());

	for (i = 0; i < areas.Num(); i++) {
		for (num = 0, reach = areas[i].reach; reach; reach = reach->next) {
			num++;
		}
#if HARM_AAS_SAFETY_RW //karin: only write max reach count per area
		if(num > MAX_REACH_PER_AREA)
		{
			common->Warning("Num reach in area(%d) is overflow(%d > %d) when writing AAS file '%s'", i, num, MAX_REACH_PER_AREA, fileName.c_str());
			if(harm_aas_areaReachSafety.GetInteger() & 2)
				num = MAX_REACH_PER_AREA;
		}
		int numWrite = 0;
#endif

#ifdef _RAVEN // quake4 aas file
// jmarshall: AAS 1.08 - numFeatures/firstFeature
		aasFile->WriteFloatString( "\t%d ( %d %d %d %d %d %d %d %d ) %d {\n", i, areas[i].flags, areas[i].contents,
						areas[i].firstFace, areas[i].numFaces, areas[i].cluster, areas[i].clusterAreaNum, /*areas[i].numFeatures*/ 0, /*areas[i].firstFeature*/ 0, num );
// jmarshall end
#elif defined(_SPLASHDAMAGE)
		aasFile->WriteFloatString("\t%d ( %d %d %d %d %d %d ) %d {\n", i, areas[i].flags, areas[i].contents,
								  areas[i].firstEdge, areas[i].numEdges, areas[i].cluster, areas[i].clusterAreaNum, num);
#else
		aasFile->WriteFloatString("\t%d ( %d %d %d %d %d %d ) %d {\n", i, areas[i].flags, areas[i].contents,
		                          areas[i].firstFace, areas[i].numFaces, areas[i].cluster, areas[i].clusterAreaNum, num);
#endif

		for (reach = areas[i].reach; reach; reach = reach->next) {
#if HARM_AAS_SAFETY_RW //karin: only write max reach count per area
			if(harm_aas_areaReachSafety.GetInteger() & 2)
			{
				if(numWrite < MAX_REACH_PER_AREA)
					numWrite++;
				else
				{
					if(numWrite == MAX_REACH_PER_AREA)
					{
						common->Printf("Writing num reach in area(%d) is overflow(%d) to AAS file '%s', don't write any more in this area.\n", i, MAX_REACH_PER_AREA, fileName.c_str());
						numWrite += 1;
					}
					continue;
				}
			}
#endif
			Reachability_Write(aasFile, reach);

#if !defined(_SPLASHDAMAGE)
			switch (reach->travelType) {
				case TFL_SPECIAL:
					Reachability_Special_Write(aasFile, static_cast<idReachability_Special *>(reach));
					break;
			}
#endif

			aasFile->WriteFloatString("\n");
		}

		aasFile->WriteFloatString("\t}\n");
	}

	aasFile->WriteFloatString("}\n");

	// write out nodes
	aasFile->WriteFloatString("nodes %d {\n", nodes.Num());

	for (i = 0; i < nodes.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %d %d %d )\n", i, nodes[i].planeNum, nodes[i].children[0], nodes[i].children[1]);
	}

	aasFile->WriteFloatString("}\n");

	// write out portals
	aasFile->WriteFloatString("portals %d {\n", portals.Num());

	for (i = 0; i < portals.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %d %d %d %d %d )\n", i, portals[i].areaNum, portals[i].clusters[0],
		                          portals[i].clusters[1], portals[i].clusterAreaNum[0], portals[i].clusterAreaNum[1]);
	}

	aasFile->WriteFloatString("}\n");

	// write out portalIndex
	aasFile->WriteFloatString("portalIndex %d {\n", portalIndex.Num());

	for (i = 0; i < portalIndex.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %d )\n", i, portalIndex[i]);
	}

	aasFile->WriteFloatString("}\n");

	// write out clusters
	aasFile->WriteFloatString("clusters %d {\n", clusters.Num());

	for (i = 0; i < clusters.Num(); i++) {
		aasFile->WriteFloatString("\t%d ( %d %d %d %d )\n", i, clusters[i].numAreas, clusters[i].numReachableAreas,
#ifdef _SPLASHDAMAGE //karin: numPortals is before firstPortal on ETQW
		                          clusters[i].numPortals, clusters[i].firstPortal
#else
		                          clusters[i].firstPortal, clusters[i].numPortals
#endif
		                          );
	}

	aasFile->WriteFloatString("}\n");

	// close file
	fileSystem->CloseFile(aasFile);

	common->Printf("done.\n");

	return true;
}

/*
================
idAASFileLocal::ParseIndex
================
*/
bool idAASFileLocal::ParseIndex(idLexer &src, idList<aasIndex_t> &indexes)
{
	int numIndexes, i;
	aasIndex_t index;

	numIndexes = src.ParseInt();
	indexes.Resize(numIndexes);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numIndexes; i++) {
		src.ParseInt();
		src.ExpectTokenString("(");
		index = src.ParseInt();
		src.ExpectTokenString(")");
		indexes.Append(index);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

	return true;
}

/*
================
idAASFileLocal::ParsePlanes
================
*/
bool idAASFileLocal::ParsePlanes(idLexer &src)
{
	int numPlanes, i;
	idPlane plane;
	idVec4 vec;

	numPlanes = src.ParseInt();
	planeList.Resize(numPlanes);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numPlanes; i++) {
		src.ParseInt();

		if (!src.Parse1DMatrix(4, vec.ToFloatPtr())) {
			return false;
		}

		plane.SetNormal(vec.ToVec3());
		plane.SetDist(vec[3]);
		planeList.Append(plane);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

	return true;
}

/*
================
idAASFileLocal::ParseVertices
================
*/
bool idAASFileLocal::ParseVertices(idLexer &src)
{
	int numVertices, i;
	idVec3 vec;

	numVertices = src.ParseInt();
	vertices.Resize(numVertices);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numVertices; i++) {
		src.ParseInt();

		if (!src.Parse1DMatrix(3, vec.ToFloatPtr())) {
			return false;
		}

		vertices.Append(vec);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

	return true;
}

/*
================
idAASFileLocal::ParseEdges
================
*/
bool idAASFileLocal::ParseEdges(idLexer &src)
{
	int numEdges, i;
	aasEdge_t edge;

	numEdges = src.ParseInt();
	edges.Resize(numEdges);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numEdges; i++) {
		src.ParseInt();
		src.ExpectTokenString("(");
		edge.vertexNum[0] = src.ParseInt();
		edge.vertexNum[1] = src.ParseInt();
		src.ExpectTokenString(")");
		edges.Append(edge);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

	return true;
}

/*
================
idAASFileLocal::ParseFaces
================
*/
bool idAASFileLocal::ParseFaces(idLexer &src)
{
	int numFaces, i;
	aasFace_t face;

	numFaces = src.ParseInt();
	faces.Resize(numFaces);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numFaces; i++) {
		src.ParseInt();
		src.ExpectTokenString("(");
		face.planeNum = src.ParseInt();
		face.flags = src.ParseInt();
		face.areas[0] = src.ParseInt();
		face.areas[1] = src.ParseInt();
		face.firstEdge = src.ParseInt();
		face.numEdges = src.ParseInt();
		src.ExpectTokenString(")");
		faces.Append(face);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

	return true;
}

/*
================
idAASFileLocal::ParseReachabilities
================
*/
bool idAASFileLocal::ParseReachabilities(idLexer &src, int areaNum)
{
	int num, j;
	aasArea_t *area;
	idReachability reach, *newReach;
	idReachability_Special *special;

	area = &areas[areaNum];

	num = src.ParseInt();
	src.ExpectTokenString("{");
	area->reach = NULL;
	area->rev_reach = NULL;
	area->travelFlags = AreaContentsTravelFlags(areaNum);

#if HARM_AAS_SAFETY_RW //karin: only load max reach count per area, else raise error in game library
		if(num > MAX_REACH_PER_AREA)
			common->Warning("Num reach in area(%d) is overflow(%d > %d) when loading AAS file '%s'", areaNum, num, MAX_REACH_PER_AREA, src.GetFileName());
#endif
	for (j = 0; j < num; j++) {
		Reachability_Read(src, &reach);

#ifdef _RAVEN //karin: aas1.08
		if (reach.travelType & TFL_SPECIAL) {
			newReach = special = new idReachability_Special();
			Reachability_Special_Read(src, special);
		}
		else
		{
			newReach = new idReachability();
			//karin: skip {...}
			idToken t;
			if(src.ReadToken(&t))
			{
				if(!t.Cmp("{"))
				{
					src.SkipBracedSection(false);
				}
				else
					src.UnreadToken(&t);
			}
		}
#elif defined(_SPLASHDAMAGE)
		newReach = new aasReachability_t();
#else
		switch (reach.travelType) {
			case TFL_SPECIAL:
				newReach = special = new idReachability_Special();
				Reachability_Special_Read(src, special);
				break;
			default:
				newReach = new idReachability();
				break;
		}
#endif

#if HARM_AAS_SAFETY_RW //karin: only load max reach count per area, else raise error in game library
		if(j >= MAX_REACH_PER_AREA && (harm_aas_areaReachSafety.GetInteger() & 1))
		{
			if(j == MAX_REACH_PER_AREA)
				common->Printf("Loading num reach in area(%d) is overflow(%d) to AAS file '%s', others will be skipped in this area.\n", areaNum, MAX_REACH_PER_AREA, src.GetFileName());
			delete newReach;
			continue;
		}
#endif
		newReach->CopyBase(reach);
		newReach->fromAreaNum = areaNum;
		newReach->next = area->reach;
		area->reach = newReach;
	}

	src.ExpectTokenString("}");
	return true;
}

/*
================
idAASFileLocal::LinkReversedReachability
================
*/
void idAASFileLocal::LinkReversedReachability(void)
{
	int i;
	idReachability *reach;

	// link reversed reachabilities
	for (i = 0; i < areas.Num(); i++) {
		for (reach = areas[i].reach; reach; reach = reach->next) {
			reach->rev_next = areas[reach->toAreaNum].rev_reach;
			areas[reach->toAreaNum].rev_reach = reach;
		}
	}
}

/*
================
idAASFileLocal::ParseAreas
================
*/
bool idAASFileLocal::ParseAreas(idLexer &src)
{
	int numAreas, i;
	aasArea_t area;

	numAreas = src.ParseInt();
	areas.Resize(numAreas);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numAreas; i++) {
		src.ParseInt();
		src.ExpectTokenString("(");
#ifdef _SPLASHDAMAGE
		area.travelFlags = src.ParseInt();
		area.flags = src.ParseInt();
		area.numEdges = src.ParseInt();
		area.firstEdge = src.ParseInt();
		area.clusterAreaNum = src.ParseInt();
		area.cluster = src.ParseInt();
		area.obstaclePVSOffset = src.ParseInt();
#else
		area.flags = src.ParseInt();
		area.contents = src.ParseInt();
		area.firstFace = src.ParseInt();
		area.numFaces = src.ParseInt();
		area.cluster = src.ParseInt();
		area.clusterAreaNum = src.ParseInt();
#endif
        area.reach = NULL;
        area.rev_reach = NULL;
        area.bounds.Zero();
        area.center.Zero();
#ifdef _SPLASHDAMAGE
		area.contents = area.flags;
#else
        area.travelFlags = 0;
#endif
#ifdef _RAVEN // quake4 aas file
// jmarshall - AAS 1.08 
		area.numFeatures = src.ParseInt();
		area.firstFeature = src.ParseInt();
// jmarshall end
#endif
		src.ExpectTokenString(")");
		areas.Append(area);
		ParseReachabilities(src, i);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

#if !defined(_SPLASHDAMAGE)
	LinkReversedReachability();
#endif

	return true;
}

/*
================
idAASFileLocal::ParseNodes
================
*/
bool idAASFileLocal::ParseNodes(idLexer &src)
{
	int numNodes, i;
	aasNode_t node;

	numNodes = src.ParseInt();
#ifdef _RAVEN // in quake4 paks, have some invalid aas files with node num is 0 or 1
// jmarshall - AAS 1.08
	if (numNodes <= 1) //k: at least 2
	{
		return false;
	}
// jmarshall end
#endif
	nodes.Resize(numNodes);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numNodes; i++) {
		src.ParseInt();
		src.ExpectTokenString("(");
		node.planeNum = src.ParseInt();
		node.children[0] = src.ParseInt();
		node.children[1] = src.ParseInt();
		src.ExpectTokenString(")");
		nodes.Append(node);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

	return true;
}

/*
================
idAASFileLocal::ParsePortals
================
*/
bool idAASFileLocal::ParsePortals(idLexer &src)
{
	int numPortals, i;
	aasPortal_t portal;

	numPortals = src.ParseInt();
	portals.Resize(numPortals);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numPortals; i++) {
		src.ParseInt();
		src.ExpectTokenString("(");
		portal.areaNum = src.ParseInt();
		portal.clusters[0] = src.ParseInt();
		portal.clusters[1] = src.ParseInt();
		portal.clusterAreaNum[0] = src.ParseInt();
		portal.clusterAreaNum[1] = src.ParseInt();
		src.ExpectTokenString(")");
		portals.Append(portal);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

	return true;
}

/*
================
idAASFileLocal::ParseClusters
================
*/
bool idAASFileLocal::ParseClusters(idLexer &src)
{
	int numClusters, i;
	aasCluster_t cluster;

	numClusters = src.ParseInt();
	clusters.Resize(numClusters);

	if (!src.ExpectTokenString("{")) {
		return false;
	}

	for (i = 0; i < numClusters; i++) {
		src.ParseInt();
		src.ExpectTokenString("(");
		cluster.numAreas = src.ParseInt();
		cluster.numReachableAreas = src.ParseInt();
#ifdef _SPLASHDAMAGE //karin: numPortals is before firstPortal on ETQW
		cluster.numPortals = src.ParseInt();
		cluster.firstPortal = src.ParseInt();
#else
		cluster.firstPortal = src.ParseInt();
		cluster.numPortals = src.ParseInt();
#endif
		src.ExpectTokenString(")");
		clusters.Append(cluster);
	}

	if (!src.ExpectTokenString("}")) {
		return false;
	}

	return true;
}

/*
================
idAASFileLocal::FinishAreas
================
*/
void idAASFileLocal::FinishAreas(void)
{
	int i;

	for (i = 0; i < areas.Num(); i++) {
		areas[i].center = AreaReachableGoal(i);
		areas[i].bounds = AreaBounds(i);
	}
}

/*
================
idAASFileLocal::Load
================
*/
bool idAASFileLocal::Load(const idStr &fileName, unsigned int mapFileCRC)
{
	idLexer src(LEXFL_NOFATALERRORS | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES);
	idToken token;
	int depth;
	unsigned int c;

#ifdef _SPLASHDAMAGE //karin: parse binary aasb file
	if (LoadBinary(fileName, mapFileCRC)) {
		return true;
	} else {
		common->Printf("idAASFileLocal::Load: unable to load binary aasb file '%s', try ascii aas file.\n", fileName.c_str());
	}
#endif

	name = fileName;
	crc = mapFileCRC;

	common->Printf("[Load AAS]\n");
	common->Printf("loading %s\n", name.c_str());

	if (!src.LoadFile(name)) {
		return false;
	}

	if (!src.ExpectTokenString(AAS_FILEID)) {
		common->Warning("Not an AAS file: '%s'", name.c_str());
		return false;
	}

	if (!src.ReadToken(&token) || token != AAS_FILEVERSION) {
		common->Warning("AAS file '%s' has version %s instead of %s", name.c_str(), token.c_str(), AAS_FILEVERSION);
		return false;
	}

	if (!src.ExpectTokenType(TT_NUMBER, TT_INTEGER, &token)) {
		common->Warning("AAS file '%s' has no map file CRC", name.c_str());
		return false;
	}

	c = token.GetUnsignedLongValue();

	if (mapFileCRC && c != mapFileCRC) {
		common->Warning("AAS file '%s' is out of date", name.c_str());
		return false;
	}

	// clear the file in memory
	Clear();

	// parse the file
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (token == "settings") {
			if (!settings.FromParser(src)) {
				return false;
			}
		} else if (token == "planes") {
			if (!ParsePlanes(src)) {
				return false;
			}
		} else if (token == "vertices") {
			if (!ParseVertices(src)) {
				return false;
			}
		} else if (token == "edges") {
			if (!ParseEdges(src)) {
				return false;
			}
		} else if (token == "edgeIndex") {
			if (!ParseIndex(src, edgeIndex)) {
				return false;
			}
		} else if (token == "faces") {
			if (!ParseFaces(src)) {
				return false;
			}
		} else if (token == "faceIndex") {
			if (!ParseIndex(src, faceIndex)) {
				return false;
			}
		} else if (token == "areas") {
			if (!ParseAreas(src)) {
				return false;
			}
		} else if (token == "nodes") {
			if (!ParseNodes(src)) {
				return false;
			}
		} else if (token == "portals") {
			if (!ParsePortals(src)) {
				return false;
			}
		} else if (token == "portalIndex") {
			if (!ParseIndex(src, portalIndex)) {
				return false;
			}
		} else if (token == "clusters") {
			if (!ParseClusters(src)) {
				return false;
			}
#ifdef _RAVEN // quake4 aas file
		}
// jmarshall - AAS 1.08
		else if (token == "featureIndex") {
			int numFeatureIndexes = src.ParseInt();
			featureIndexes.Resize(numFeatureIndexes);
			src.ExpectTokenString("{");

			for (int d = 0; d < numFeatureIndexes; d++)
			{
				src.ParseInt();
				src.ExpectTokenString("(");
				featureIndexes.Append(src.ParseInt());
				src.ExpectTokenString(")");
			}

			src.ExpectTokenString("}");
		}
		else if (token == "features") {
			int numFeatures = src.ParseInt();
			features.Resize(numFeatures);
			src.ExpectTokenString("{");

			for (int d = 0; d < numFeatures; d++)
			{
				aasFeature_t feature; // = { };

				src.ParseInt();
				src.ExpectTokenString("(");
				feature.flags = src.ParseInt();					// 2 Bytes
				feature.height = src.ParseInt();					// 2 Bytes
				feature.normalx = src.ParseInt();					// 2 Bytes
				feature.normaly = src.ParseInt();				// 2 Bytes
				feature.x = src.ParseInt();			// 1 Byte
				feature.y = src.ParseInt();			// 1 Byte
				feature.z = src.ParseInt();				// 1 Byte

				features.Append(feature);
				src.ExpectTokenString(")");
			}

			src.ExpectTokenString("}");
// jmarshall end
#endif

		} else {
			src.Error("idAASFileLocal::Load: bad token \"%s\"", token.c_str());
			return false;
		}
	}

#ifdef _SPLASHDAMAGE
	LinkReachability();

	FlagNoPushAreas();
#endif
	FinishAreas();

	depth = MaxTreeDepth();

	if (depth > MAX_AAS_TREE_DEPTH) {
		src.Error("idAASFileLocal::Load: tree depth = %d", depth);
	}

	common->Printf("done.\n");

	return true;
}

/*
================
idAASFileLocal::MemorySize
================
*/
#ifdef _SPLASHDAMAGE
size_t idAASFileLocal::MemorySize(void) const
#else
int idAASFileLocal::MemorySize(void) const
#endif
{
#ifdef _SPLASHDAMAGE
	size_t size;
#else
	int size;
#endif

	size = planeList.Size();
	size += vertices.Size();
	size += edges.Size();
	size += edgeIndex.Size();
	size += faces.Size();
	size += faceIndex.Size();
	size += areas.Size();
	size += nodes.Size();
	size += portals.Size();
	size += portalIndex.Size();
	size += clusters.Size();
#ifdef _RAVEN
	size += featureIndexes.Size();
	size += features.Size();
#endif
	size += sizeof(idReachability_Walk) * NumReachabilities();

	return size;
}

/*
================
idAASFileLocal::PrintInfo
================
*/
void idAASFileLocal::PrintInfo(void) const
{
#ifdef _SPLASHDAMAGE
	common->Printf("%6zd KB file size\n", MemorySize() >> 10);
#else
	common->Printf("%6d KB file size\n", MemorySize() >> 10);
#endif
	common->Printf("%6d areas\n", areas.Num());
	common->Printf("%6d max tree depth\n", MaxTreeDepth());
	ReportRoutingEfficiency();
}

/*
================
idAASFileLocal::NumReachabilities
================
*/
int idAASFileLocal::NumReachabilities(void) const
{
	int i, num;
	idReachability *reach;

	num = 0;

	for (i = 0; i < areas.Num(); i++) {
		for (reach = areas[i].reach; reach; reach = reach->next) {
			num++;
		}
	}

	return num;
}

/*
================
idAASFileLocal::ReportRoutingEfficiency
================
*/
void idAASFileLocal::ReportRoutingEfficiency(void) const
{
	int numReachableAreas, total, i, n;

	numReachableAreas = 0;
	total = 0;

	for (i = 0; i < clusters.Num(); i++) {
		n = clusters[i].numReachableAreas;
		numReachableAreas += n;
		total += n * n;
	}

	total += numReachableAreas * portals.Num();

	common->Printf("%6d reachable areas\n", numReachableAreas);
	common->Printf("%6d reachabilities\n", NumReachabilities());
	common->Printf("%6d KB max routing cache\n", (total * 3) >> 10);
}

/*
================
idAASFileLocal::DeleteReachabilities
================
*/
void idAASFileLocal::DeleteReachabilities(void)
{
	int i;
	idReachability *reach, *nextReach;

	for (i = 0; i < areas.Num(); i++) {
		for (reach = areas[i].reach; reach; reach = nextReach) {
			nextReach = reach->next;
			delete reach;
		}

		areas[i].reach = NULL;
		areas[i].rev_reach = NULL;
	}
}

/*
================
idAASFileLocal::DeleteClusters
================
*/
void idAASFileLocal::DeleteClusters(void)
{
	aasPortal_t portal;
	aasCluster_t cluster;

	portals.Clear();
	portalIndex.Clear();
	clusters.Clear();

	// first portal is a dummy
	memset(&portal, 0, sizeof(portal));
	portals.Append(portal);

	// first cluster is a dummy
	memset(&cluster, 0, sizeof(cluster));
	clusters.Append(cluster);
}

#ifdef _SPLASHDAMAGE //karin: parse binary aasb file
/*
============
idAASSettings::ReadFromFileBinary
============
*/
bool idAASSettings::ReadFromFileBinary(idFile *file)
{
	file->ReadInt(type);
#if 0
	if (type != AAS_PLAYER && type != AAS_VEHICLE)
	{
		common->Warning("AASB file '%s' has invalid type %d", name.c_str(), type);
		return false;
	}
#endif

	file->ReadString(fileExtension);

	numBoundingBoxes = 0;

	file->ReadVec3(boundingBox[0]);
	file->ReadVec3(boundingBox[1]);

	boundingBoxes[numBoundingBoxes++] = boundingBox;

	file->ReadInt(primitiveModeBrush);
	file->ReadInt(primitiveModePatch);
	file->ReadInt(primitiveModeModel);
	file->ReadInt(primitiveModeTerrain);

	file->ReadVec3(gravity);
	file->ReadVec3(gravityDir);
	file->ReadVec3(invGravityDir);

	file->ReadFloat(maxStepHeight);
	file->ReadFloat(maxBarrierHeight);
	file->ReadFloat(maxWaterJumpHeight);
	file->ReadFloat(maxFallHeight);
	file->ReadFloat(minFloorCos);
	file->ReadFloat(minHighCeiling);

	file->ReadFloat(groundSpeed);
	file->ReadFloat(waterSpeed);
	file->ReadFloat(ladderSpeed);

	file->ReadFloat(wallCornerEdgeRadius);
	file->ReadFloat(ledgeCornerEdgeRadius);
	file->ReadFloat(obstaclePVSRadius);

	file->ReadInt(tt_barrierJump);
	file->ReadInt(tt_waterJump);
	file->ReadInt(tt_startWalkOffLedge);
	file->ReadInt(tt_startLadderClimb);

	usePatches = false;
	writeBrushMap = false;
	playerFlood = false;
	allowSwimReachabilities = false;
	allowFlyReachabilities = false;
	if (fileExtension.IsEmpty()) {
		idStr fileName(file->GetName());
		idStr ext;
		fileName.ExtractFileExtension(ext);
		fileExtension = ext;
	}
	tt_startCrouching = false;

	return true;
}

/*
================
idAASFileLocal::LoadBinary
================
*/
bool idAASFileLocal::LoadBinary(const idStr &fileName, unsigned int mapFileCRC)
{
	idStr token;
	int depth;
	unsigned int c;

	name = fileName;
	crc = mapFileCRC;

	idStr binName(name);
	binName.Append("b");

	common->Printf("[Load AAS Binary]\n");
	common->Printf("loading %s\n", binName.c_str());

	idFile *file = fileSystem->OpenFileRead(binName.c_str());
	if (!file) {
		return false;
	}

	//karin: 1. read fileID
	file->ReadString(token);
	if (idStr::Icmp(token, AAS_FILE_ID_BINARY)) {
		common->Warning("Not an AASB file: '%s'", name.c_str());
		fileSystem->CloseFile(file);
		return false;
	}

	//karin: 2. read version
	idStr version;
	file->ReadString(version);
	if (version != AAS_FILE_VERSION)
	{
		common->Warning("AASB file '%s' has version %s instead of %s", name.c_str(), token.c_str(), AAS_FILEVERSION);
		fileSystem->CloseFile(file);
		return false;
	}

	file->ReadUnsignedInt(c);
#if 0
	if (mapFileCRC && c != mapFileCRC) {
		common->Warning("AASB file '%s' is out of date", name.c_str());
		return false;
	}
#endif

	// clear the file in memory
	Clear();

	//karin: 3. parse settings
	if (!settings.ReadFromFileBinary(file)) {
		fileSystem->CloseFile(file);
		return false;
	}

	//karin: 4. parse the file
	if (!ParsePlanesBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParseVerticesBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParseEdgesBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}
	if (!ParseIndexBinary(file, edgeIndex))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParseReachabilitiesBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParseAreasBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParseNodesBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParsePortalsBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}
	if (!ParseIndexBinary(file, portalIndex))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParseClustersBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParseObstaclePVSsBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	if (!ParseReachabilityNamesBinary(file))
	{
		fileSystem->CloseFile(file);
		return false;
	}

	fileSystem->CloseFile(file);

	LinkReachability();

	FlagNoPushAreas();

	FinishAreas();

	depth = MaxTreeDepth();

	if (depth > MAX_AAS_TREE_DEPTH) {
		common->Error("idAASFileLocal::Load: tree depth = %d", depth);
	}

	common->Printf("done.\n");

	return true;
}

/*
================
idAASFileLocal::ParsePlanesBinary
================
*/
bool idAASFileLocal::ParsePlanesBinary(idFile *file)
{
	int numPlanes, i;

	file->ReadInt(numPlanes);
	planeList.SetNum(numPlanes);

	for (i = 0; i < numPlanes; i++) {
		idPlane &plane = planeList[i];
		file->ReadFloat(plane[0]);
		file->ReadFloat(plane[1]);
		file->ReadFloat(plane[2]);
		file->ReadFloat(plane[3]);
	}

	return true;
}

/*
================
idAASFileLocal::ParseVerticesBinary
================
*/
bool idAASFileLocal::ParseVerticesBinary(idFile *file)
{
	int numVertices, i;

	file->ReadInt(numVertices);
	vertices.SetNum(numVertices);

	for (i = 0; i < numVertices; i++) {
		idVec3 &vec = vertices[i];
		file->ReadFloat(vec[0]);
		file->ReadFloat(vec[1]);
		file->ReadFloat(vec[2]);
	}

	return true;
}

/*
================
idAASFileLocal::ParseEdgesBinary
================
*/
bool idAASFileLocal::ParseEdgesBinary(idFile *file)
{
	int numEdges, i;

	file->ReadInt(numEdges);
	edges.SetNum(numEdges);

	for (i = 0; i < numEdges; i++) {
		aasEdge_t &edge = edges[i];
		file->ReadInt(edge.vertexNum[0]);
		file->ReadInt(edge.vertexNum[1]);
		file->ReadInt(edge.flags);
	}

	return true;
}

/*
================
idAASFileLocal::ParseIndexBinary
================
*/
bool idAASFileLocal::ParseIndexBinary(idFile *file, idList<aasIndex_t> &indexes)
{
	int numIndexes, i;

	file->ReadInt(numIndexes);
	indexes.SetNum(numIndexes);

	for (i = 0; i < numIndexes; i++) {
		file->ReadInt(indexes[i]);
	}

	return true;
}

/*
================
idAASFileLocal::ParseAreasBinary
================
*/
bool idAASFileLocal::ParseAreasBinary(idFile *file)
{
	int numAreas, i;

	file->ReadInt(numAreas);
	areas.SetNum(numAreas);

	for (i = 0; i < numAreas; i++) {
		aasArea_t &area = areas[i];
		file->ReadUnsignedShort(area.travelFlags);
		file->ReadUnsignedShort(area.flags);
		file->ReadInt(area.numEdges);
		file->ReadInt(area.firstEdge);
		file->ReadShort(area.cluster);
		file->ReadUnsignedShort(area.clusterAreaNum);
		file->ReadUnsignedInt(area.obstaclePVSOffset);
		file->Seek(4 * 2, FS_SEEK_CUR); // 2 32bits pointers
		area.reach = NULL;
		area.rev_reach = NULL;
		area.contents = area.flags;
		area.bounds.Zero();
		area.center.Zero();
		// compat for DOOM3
		area.firstFace = 0;
		area.numFaces = 0;
	}

	return true;
}

/*
================
idAASFileLocal::ParseNodesBinary
================
*/
bool idAASFileLocal::ParseNodesBinary(idFile *file)
{
	int numNodes, i;

	file->ReadInt(numNodes);
	//if (numNodes <= 1) //karin: at least 2
	//	return false;

	nodes.SetNum(numNodes);

	for (i = 0; i < numNodes; i++) {
		aasNode_t &node = nodes[i];
		file->ReadUnsignedShort(node.planeNum);
		file->ReadUnsignedShort(node.flags);
		file->ReadInt(node.children[0]);
		file->ReadInt(node.children[1]);
	}

	return true;
}

/*
================
idAASFileLocal::ParsePortalsBinary
================
*/
bool idAASFileLocal::ParsePortalsBinary(idFile *file)
{
	int numPortals, i;

	file->ReadInt(numPortals);
	portals.SetNum(numPortals);

	for (i = 0; i < numPortals; i++) {
		aasPortal_t &portal = portals[i];
		file->ReadUnsignedShort(portal.areaNum);
		file->ReadShort(portal.clusters[0]);
		file->ReadShort(portal.clusters[1]);
		file->ReadUnsignedShort(portal.clusterAreaNum[0]);
		file->ReadUnsignedShort(portal.clusterAreaNum[1]);
		file->ReadUnsignedShort(portal.maxAreaTravelTime);
	}

	return true;
}

/*
================
idAASFileLocal::ParseClustersBinary
================
*/
bool idAASFileLocal::ParseClustersBinary(idFile *file)
{
	int numClusters, i;

	file->ReadInt(numClusters);
	clusters.SetNum(numClusters);

	for (i = 0; i < numClusters; i++) {
		aasCluster_t &cluster = clusters[i];
		file->ReadInt(cluster.numAreas);
		file->ReadInt(cluster.numReachableAreas);
		//karin: numPortals is before firstPortal on ETQW
		file->ReadInt(cluster.numPortals);
		file->ReadInt(cluster.firstPortal);
	}

	return true;
}

/*
================
idAASFileLocal::ParseObstaclePVSsBinary
================
*/
bool idAASFileLocal::ParseObstaclePVSsBinary(idFile *file)
{
	int numIndexes, i;

	file->ReadInt(numIndexes);
	obstaclePVS.SetNum(numIndexes);

	for (i = 0; i < numIndexes; i++) {
		file->ReadUnsignedChar(obstaclePVS[i]);
	}

	return true;
}

/*
================
idAASFileLocal::ParseReachabilityNamesBinary
================
*/
bool idAASFileLocal::ParseReachabilityNamesBinary(idFile *file)
{
	int numNames, i;

	file->ReadInt(numNames);
	reachabilityNames.SetNum(numNames);

	for (i = 0; i < numNames; i++) {
		file->Read(reachabilityNames[i].name, sizeof(reachabilityNames[i].name));
		file->ReadInt(reachabilityNames[i].index);
	}

	return true;
}

/*
================
idAASFileLocal::ParseReachabilitiesBinary
================
*/
bool idAASFileLocal::ParseReachabilitiesBinary(idFile *file)
{
	int num, i;

	file->ReadInt(num);

	reachabilities.SetNum(num);
	for (i = 0; i < num; i++) {
		idReachability &reach = reachabilities[i];

		file->ReadUnsignedShort(reach.travelFlags);
		file->ReadUnsignedShort(reach.travelTime);
		file->ReadUnsignedShort(reach.fromAreaNum);
		file->ReadUnsignedShort(reach.toAreaNum);
		file->ReadShort(reach.start[0]);
		file->ReadShort(reach.start[1]);
		file->ReadShort(reach.start[2]);
		file->ReadShort(reach.end[0]);
		file->ReadShort(reach.end[1]);
		file->ReadShort(reach.end[2]);
		file->ReadUnsignedInt(reach.areaTTOfsAndNumber);

		file->Seek(4 * 2, FS_SEEK_CUR); // 2 32bit pointer

		reach.travelType = 0;
		reach.edgeNum = 0;

		reach.next = NULL;
		reach.rev_next = NULL;
	}

	return true;
}

int idAASFileLocal::FindReachabilityByName( const char *name ) const {
	for (int i = 0; i < reachabilityNames.Num(); i++) {
		if (!idStr::Icmp(reachabilityNames[i].name, name)) {
			return reachabilityNames[i].index;
		}
	}
	return -1;
}

void idAASFileLocal::LinkReachability(void)
{
  int i_v1; // edi
  int i_v2; // edx
  int i_v3; // eax
  int i_v4; // ebx
  aasReachability_t *reach_list; // eax
  int fromAreaNum; // esi
  aasReachability_t *reach_v7; // eax
  int toAreaNum; // edx

  i_v1 = 0;
  i_v2 = 0;
  if ( this->areas.Num() > 0 )
  {
    i_v3 = 0;
    do
    {
      this->areas[i_v3].reach = NULL;
      this->areas[i_v3].rev_reach = NULL;
      ++i_v2;
      ++i_v3;
    }
    while ( i_v2 < this->areas.Num() );
  }
  if ( this->reachabilities.Num() > 0 )
  {
    i_v4 = 0;
    do
    {
      reach_list = this->reachabilities.Ptr();
      fromAreaNum = reach_list[i_v4].fromAreaNum;
      reach_v7 = &reach_list[i_v4];
      reach_v7->next = this->areas[fromAreaNum].reach;
      this->areas[fromAreaNum].reach = reach_v7;
      toAreaNum = reach_v7->toAreaNum;
      reach_v7->rev_next = this->areas[toAreaNum].rev_reach;
      ++i_v1;
      this->areas[toAreaNum].rev_reach = reach_v7;
      ++i_v4;
    }
    while ( i_v1 < this->reachabilities.Num() );
  }
}

void idAASFileLocal::FlagNoPushAreas(void)
{
  aasArea_t *area_v1; // esi
  int i_v2; // edi
  int *edgeIndex_list; // ebx
  aasEdge_t *edge_v4; // ebp
//#define __int64 int64_t
//  __int64 v5; // rax
  int areaNum_v7; // [esp+8h] [ebp-1Ch]
  int areaNum_v8; // [esp+Ch] [ebp-18h]
  idVec3 *vertex_v10; // [esp+14h] [ebp-10h]
  float inv_v11; // [esp+14h] [ebp-10h]
  idVec3 center_v12; // v12 v13 v14

  idVec3 *_v5_1;
  areaNum_v7 = 0;
  if ( this->areas.Num() > 0 )
  {
    areaNum_v8 = 0;
    do
    {
      area_v1 = &this->areas[areaNum_v8];
      i_v2 = 0;
      center_v12 = vec3_zero;
      if ( area_v1->numEdges > 0 )
      {
        edgeIndex_list = this->edgeIndex.Ptr();
        edge_v4 = this->edges.Ptr();
        vertex_v10 = this->vertices.Ptr();
        do
        {
          int _v5_0 = edgeIndex_list[i_v2 + area_v1->firstEdge];
          //_v5 = &v10[i_v4[(HIDWORD(v5) ^ v5) - HIDWORD(v5)].vertexNum[(unsigned int)list[i_v2 + v1->firstEdge] >> 31]];
          _v5_1 = &vertex_v10[ edge_v4[abs(_v5_0)].vertexNum[(unsigned int)_v5_0 >> 31] ];
          ++i_v2;
          center_v12 = *_v5_1 + center_v12;
        }
        while ( i_v2 < area_v1->numEdges );
      }
      inv_v11 = 1.0f / (float)area_v1->numEdges;
      center_v12 = inv_v11 * center_v12;
      if ( this->PushPointIntoArea(areaNum_v7, center_v12) )
        area_v1->flags |= AAS_AREA_NOPUSH; //0x10u;
      ++areaNum_v8;
      ++areaNum_v7;
    }
    while ( areaNum_v7 < this->areas.Num() );
  }
}

#endif
