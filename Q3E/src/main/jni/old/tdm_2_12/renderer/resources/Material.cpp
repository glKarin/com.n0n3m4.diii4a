/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "renderer/tr_local.h"
#include "renderer/resources/CinematicFFMpeg.h"
#include "renderer/backend/GLSLProgramManager.h"

/*

Any errors during parsing just set MF_DEFAULTED and return, rather than throwing
a hard error. This will cause the material to fall back to default material,
but otherwise let things continue.

Each material may have a set of calculations that must be evaluated before
drawing with it.

Every expression that a material uses can be evaluated at one time, which
will allow for perfect common subexpression removal when I get around to
writing it.

Without this, scrolling an entire surface could result in evaluating the
same texture matrix calculations a half dozen times.

  Open question: should I allow arbitrary per-vertex color, texCoord, and vertex
  calculations to be specified in the material code?

  Every stage will definately have a valid image pointer.

  We might want the ability to change the sort value based on conditionals,
  but it could be a hassle to implement,

*/

// keep all of these on the stack, when they are static it makes material parsing non-reentrant
typedef struct mtrParsingData_s {
	bool			registerIsTemporary[MAX_EXPRESSION_REGISTERS];
	float			shaderRegisters[MAX_EXPRESSION_REGISTERS];
	expOp_t			shaderOps[MAX_EXPRESSION_OPS];
	shaderStage_t	parseStages[MAX_SHADER_STAGES];

	bool			registersAreConstant;
	bool			forceOverlays;
	bool			usesFrobParm;	// stgatilov #5427: is parm11 referenced in current stage?
} mtrParsingData_t;


/*
=============
idMaterial::CommonInit
=============
*/
void idMaterial::CommonInit() {
	desc = "<none>";
	renderBump = "";
	contentFlags = CONTENTS_SOLID;
	surfaceFlags = SURFTYPE_NONE;
	materialFlags = 0;
	sort = SS_BAD;
	coverage = MC_BAD;
	cullType = CT_FRONT_SIDED;
	deform = DFRM_NONE;
	numOps = 0;
	ops = NULL;
	numRegisters = 0;
	expressionRegisters = NULL;
	constantRegisters = NULL;
	numStages = 0;
	numAmbientStages = 0;
	stages = NULL;
	editorImage = NULL;
	lightFalloffImage = NULL;
	lightAmbientDiffuse = NULL;
	lightAmbientSpecular = NULL;
	shouldCreateBackSides = false;
	entityGui = 0;
	fogLight = false;
	blendLight = false;
	ambientLight = false;
	cubicLight = false;  //nbohr1more #3881: cubemap based lighting
	noFog = false;
	hasSubview = false;
	allowOverlays = true;
	isLightgemSurf = false; //nbohr1more: #4379 lightgem culling
	unsmoothedTangents = false;
	gui = NULL;
	memset( deformRegisters, 0, sizeof( deformRegisters ) );
	editorAlpha = 1.0f;
	spectrum = 0;
	polygonOffset = 0.0f;
	fogAlpha = 0;
	suppressInSubview = false;
	refCount = 0;
	portalSky = false;

	decalInfo.stayTime = 10000;
	decalInfo.fadeTime = 4000;
	decalInfo.start[0] = 1.0f;
	decalInfo.start[1] = 1.0f;
	decalInfo.start[2] = 1.0f;
	decalInfo.start[3] = 1.0f;
	decalInfo.end[0] = 0.0f;
	decalInfo.end[1] = 0.0f;
	decalInfo.end[2] = 0.0f;
	decalInfo.end[3] = 0.0f;
}

/*
=============
idMaterial::idMaterial
=============
*/
idMaterial::idMaterial() {
	CommonInit();

	// we put this here instead of in CommonInit, because
	// we don't want it cleared when a material is purged
	surfaceArea = 0.0f;
}

/*
=============
idMaterial::~idMaterial
=============
*/
idMaterial::~idMaterial() {
}

/*
===============
idMaterial::FreeData
===============
*/
void idMaterial::FreeData() {

	if ( stages ) {
		// delete any idCinematic textures
		for ( int i = 0; i < numStages; i++ ) {
			if ( stages[i].texture.cinematic != NULL ) {
				delete stages[i].texture.cinematic;
				stages[i].texture.cinematic = NULL;
			}
			if ( stages[i].newStage != NULL ) {
				Mem_Free( stages[i].newStage );
				stages[i].newStage = NULL;
			}
		}
		R_StaticFree( stages );
		stages = NULL;
	}
	if ( expressionRegisters != NULL ) {
		R_StaticFree( expressionRegisters );
		expressionRegisters = NULL;
	}
	if ( constantRegisters != NULL ) {
		R_StaticFree( constantRegisters );
		constantRegisters = NULL;
	}
	if ( ops != NULL ) {
		R_StaticFree( ops );
		ops = NULL;
	}
}

/*
==============
idMaterial::GetEditorImage
==============
*/
idImage *idMaterial::GetEditorImage( void ) const {
	if ( editorImage ) {
		return editorImage;
	}

	// if we don't have an editorImageName, use the first stage image
	if ( !editorImageName.Length()) {
		// _D3XP :: First check for a diffuse image, then use the first
		if ( numStages && stages ) {
			for( int i = 0; i < numStages; i++ ) {
				if ( stages[i].lighting == SL_DIFFUSE ) {
					editorImage = stages[i].texture.image;
					break;
				}
			}
			if ( !editorImage ) {
				editorImage = stages[0].texture.image;
			}
		} else {
			editorImage = globalImages->defaultImage;
		}
	} else {
		// look for an explicit one
		editorImage = globalImages->ImageFromFile( editorImageName, TF_DEFAULT, true, TR_REPEAT, TD_DEFAULT );
	}

	if ( !editorImage ) {
		editorImage = globalImages->defaultImage;
	}

	return editorImage;
}


// info parms
typedef struct {
	const char	*name;
	int		clearSolid, surfaceFlags, contents;
} infoParm_t;

static infoParm_t	infoParms[] = {
	// game relevant attributes
	{"solid",		0,	0,	CONTENTS_SOLID },		// may need to override a clearSolid
	{"water",		1,	0,	CONTENTS_WATER },		// used for water
	{"playerclip",	0,	0,	CONTENTS_PLAYERCLIP },	// solid to players
	{"monsterclip",	0,	0,	CONTENTS_MONSTERCLIP },	// solid to monsters
	{"moveableclip",0,	0,	CONTENTS_MOVEABLECLIP },// solid to moveable entities
	{"ikclip",		0,	0,	CONTENTS_IKCLIP },		// solid to IK
	{"blood",		0,	0,	CONTENTS_BLOOD },		// used to detect blood decals
	{"trigger",		0,	0,	CONTENTS_TRIGGER },		// used for triggers
	{"aassolid",	0,	0,	CONTENTS_AAS_SOLID },	// solid for AAS
	{"aasobstacle",	0,	0,	CONTENTS_AAS_OBSTACLE },// used to compile an obstacle into AAS that can be enabled/disabled
	{"flashlight_trigger",	0,	0,	CONTENTS_FLASHLIGHT_TRIGGER }, // used for triggers that are activated by the flashlight
	{"nonsolid",	1,	0,	0 },					// clears the solid flag
	{"nullNormal",	0,	SURF_NULLNORMAL,0 },		// renderbump will draw as 0x80 0x80 0x80

	// utility relevant attributes
	{"areaportal",	1,	0,	CONTENTS_AREAPORTAL },	// divides areas
	{"qer_nocarve",	1,	0,	CONTENTS_NOCSG},		// don't cut brushes in editor

	{"discrete",	1,	SURF_DISCRETE,	0 },		// surfaces should not be automatically merged together or
													// clipped to the world,
													// because they represent discrete objects like gui shaders
													// mirrors, or autosprites
	{"noFragment",	0,	SURF_NOFRAGMENT,	0 },

	{"slick",		0,	SURF_SLICK,		0 },
	{"collision",	0,	SURF_COLLISION,	0 },
	{"noimpact",	0,	SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
	{"nodamage",	0,	SURF_NODAMAGE,	0 },		// no falling damage when hitting
	{"ladder",		0,	SURF_LADDER,	0 },		// climbable
	{"nosteps",		0,	SURF_NOSTEPS,	0 },		// no footsteps

	// material types for particle, sound, footstep feedback
	{"metal",		0,  SURFTYPE_METAL,		0 },	// metal
	{"stone",		0,  SURFTYPE_STONE,		0 },	// stone
	{"flesh",		0,  SURFTYPE_FLESH,		0 },	// flesh
	{"wood",		0,  SURFTYPE_WOOD,		0 },	// wood
	{"cardboard",	0,	SURFTYPE_CARDBOARD,	0 },	// cardboard
	{"liquid",		0,	SURFTYPE_LIQUID,	0 },	// liquid
	{"glass",		0,	SURFTYPE_GLASS,		0 },	// glass
	{"plastic",		0,	SURFTYPE_PLASTIC,	0 },	// plastic
	{"ricochet",	0,	SURFTYPE_RICOCHET,	0 },	// behaves like metal but causes a ricochet sound

	// unassigned surface types
	{"surftype10",	0,	SURFTYPE_10,	0 },
	{"surftype11",	0,	SURFTYPE_11,	0 },
	{"surftype12",	0,	SURFTYPE_12,	0 },
	{"surftype13",	0,	SURFTYPE_13,	0 },
	{"surftype14",	0,	SURFTYPE_14,	0 },
	{"surftype15",	0,	SURFTYPE_15,	0 },
};

static const int numInfoParms = sizeof(infoParms) / sizeof (infoParms[0]);


/*
===============
idMaterial::CheckSurfaceParm

See if the current token matches one of the surface parm bit flags
===============
*/
bool idMaterial::CheckSurfaceParm( idToken *token ) {

	for ( int i = 0 ; i < numInfoParms ; i++ ) {
		if ( !token->Icmp( infoParms[i].name ) ) {
			if ( infoParms[i].surfaceFlags & SURF_TYPE_MASK ) {
				// ensure we only have one surface type set
				surfaceFlags &= ~SURF_TYPE_MASK;
			}
			surfaceFlags |= infoParms[i].surfaceFlags;
			contentFlags |= infoParms[i].contents;
			if ( infoParms[i].clearSolid ) {
				contentFlags &= ~CONTENTS_SOLID;
			}
			return true;
		}
	}
	return false;
}

/*
===============
idMaterial::MatchToken

Sets defaultShader and returns false if the next token doesn't match
===============
*/
bool idMaterial::MatchToken( idLexer &src, const char *match ) {
	if ( !src.ExpectTokenString( match ) ) {
		SetMaterialFlag( MF_DEFAULTED );
		return false;
	}
	return true;
}

/*
===============
idMaterial::ParseNumberOrVec3
===============
*/
idVec3 idMaterial::ParseNumberOrVec3( idLexer &src ) {
	if ( src.PeekTokenString( "(" ) ) {
		MatchToken( src, "(" );
		float red = src.ParseFloat();
		float green = src.ParseFloat();
		float blue = src.ParseFloat();
		MatchToken( src, ")" );
		return idVec3( red, green, blue );
	}
	else {
		float gray = src.ParseFloat();
		return idVec3( gray, gray, gray );
	}
}

/*
=================
idMaterial::ParseSort
=================
*/
void idMaterial::ParseSort( idLexer &src ) {
	idToken token;

	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Warning( "missing sort parameter" );
		SetMaterialFlag( MF_DEFAULTED );
		return;
	}

	else if ( !token.Icmp( "subview" ) ) {
		sort = SS_SUBVIEW;
	} else if ( !token.Icmp( "opaque" ) ) {
		sort = SS_OPAQUE;
	} else if ( !token.Icmp( "decal" ) ) {
		sort = SS_DECAL;
	} else if ( !token.Icmp( "far" ) ) {
		sort = SS_FAR;
	} else if ( !token.Icmp( "medium" ) ) {
		sort = SS_MEDIUM;
	} else if ( !token.Icmp( "close" ) ) {
		sort = SS_CLOSE;
	} else if ( !token.Icmp( "almostNearest" ) ) {
		sort = SS_ALMOST_NEAREST;
	} else if ( !token.Icmp( "nearest" ) ) {
		sort = SS_NEAREST;
	} else if ( !token.Icmp( "afterFog" ) ) {
		sort = SS_AFTER_FOG;
	} else if ( !token.Icmp( "postProcess" ) ) {
		sort = SS_POST_PROCESS;
	} else if ( !token.Icmp( "last" ) ) {
		sort = SS_LAST;
	} else if ( !token.Icmp( "portalSky" ) ) {
		sort = SS_PORTAL_SKY;
	} else {
		sort = atof( token );
	}
}

/*
=================
idMaterial::ParseDecalInfo
=================
*/
void idMaterial::ParseDecalInfo( idLexer &src ) {
	idToken token;

	decalInfo.stayTime = src.ParseFloat() * 1000;
	decalInfo.fadeTime = src.ParseFloat() * 1000;

	float	start[4], end[4];
	src.Parse1DMatrix( 4, start );
	src.Parse1DMatrix( 4, end );

	for ( int i = 0 ; i < 4 ; i++ ) {
		decalInfo.start[i] = start[i];
		decalInfo.end[i] = end[i];
	}
}

/*
=============
idMaterial::GetExpressionConstant
=============
*/
int idMaterial::GetExpressionConstant( float f ) {
	int		i;

	for ( i = EXP_REG_NUM_PREDEFINED ; i < numRegisters ; i++ ) {
		if ( !pd->registerIsTemporary[i] && pd->shaderRegisters[i] == f ) {
			return i;
		}
	}
	if ( numRegisters == MAX_EXPRESSION_REGISTERS ) {
		common->Warning( "GetExpressionConstant: material '%s' hit MAX_EXPRESSION_REGISTERS", GetName() );
		SetMaterialFlag( MF_DEFAULTED );
		return 0;
	}

	pd->registerIsTemporary[i] = false;
	pd->shaderRegisters[i] = f;
	numRegisters++;

	return i;
}

/*
=============
idMaterial::GetExpressionTemporary
=============
*/
int idMaterial::GetExpressionTemporary( void ) {
	if ( numRegisters >= MAX_EXPRESSION_REGISTERS ) {
		common->Warning( "GetExpressionTemporary: material '%s' hit MAX_EXPRESSION_REGISTERS", GetName() );
		SetMaterialFlag( MF_DEFAULTED );
		return 0;
	}

	pd->registerIsTemporary[numRegisters] = true;
	numRegisters++;

	return (numRegisters - 1);
}

/*
=============
idMaterial::GetExpressionOp
=============
*/
expOp_t	*idMaterial::GetExpressionOp( void ) {
	if ( numOps == MAX_EXPRESSION_OPS ) {
		common->Warning( "GetExpressionOp: material '%s' hit MAX_EXPRESSION_OPS", GetName() );
		SetMaterialFlag( MF_DEFAULTED );
		return &pd->shaderOps[0];
	}

	return &pd->shaderOps[numOps++];
}

/*
=================
idMaterial::EmitOp
=================
*/
int idMaterial::EmitOp( int a, int b, expOpType_t opType ) {
	expOp_t	*op;

	// optimize away identity operations
	if ( opType == OP_TYPE_ADD ) {
		if ( !pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 0 ) {
			return b;
		}
		else if ( !pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 0 ) {
			return a;
		}
		else if ( !pd->registerIsTemporary[a] && !pd->registerIsTemporary[b] ) {
			return GetExpressionConstant( pd->shaderRegisters[a] + pd->shaderRegisters[b] );
		}
	}
	else if ( opType == OP_TYPE_MULTIPLY ) {
		if ( !pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 1 ) {
			return b;
		}
		else if ( !pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 0 ) {
			return a;
		}
		else if ( !pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 1 ) {
			return a;
		}
		else if ( !pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 0 ) {
			return b;
		}
		else if ( !pd->registerIsTemporary[a] && !pd->registerIsTemporary[b] ) {
			return GetExpressionConstant( pd->shaderRegisters[a] * pd->shaderRegisters[b] );
		}
	}

	op = GetExpressionOp();
	op->opType = opType;
	op->a = a;
	op->b = b;
	op->c = GetExpressionTemporary();

	return op->c;
}

/*
=================
idMaterial::ParseTerm

Returns a register index
=================
*/
int idMaterial::ParseTerm( idLexer &src ) {
	idToken token;

	src.ReadToken( &token );

	if ( token == "(" ) {
		int a = ParseExpression( src );
		MatchToken( src, ")" );
		return a;
	}

	else if ( !token.Icmp( "time" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_TIME;
	}
	else if ( !token.Icmp( "parm0" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM0;
	}
	else if ( !token.Icmp( "parm1" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM1;
	}
	else if ( !token.Icmp( "parm2" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM2;
	}
	else if ( !token.Icmp( "parm3" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM3;
	}
	else if ( !token.Icmp( "parm4" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM4;
	}
	else if ( !token.Icmp( "parm5" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM5;
	}
	else if ( !token.Icmp( "parm6" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM6;
	}
	else if ( !token.Icmp( "parm7" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM7;
	}
	else if ( !token.Icmp( "parm8" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM8;
	}
	else if ( !token.Icmp( "parm9" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM9;
	}
	else if ( !token.Icmp( "parm10" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM10;
	}
	else if ( !token.Icmp( "parm11" ) ) {
		pd->registersAreConstant = false;
		pd->usesFrobParm = true;
		return EXP_REG_PARM11;
	}
	else if ( !token.Icmp( "global0" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL0;
	}
	else if ( !token.Icmp( "global1" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL1;
	}
	else if ( !token.Icmp( "global2" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL2;
	}
	else if ( !token.Icmp( "global3" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL3;
	}
	else if ( !token.Icmp( "global4" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL4;
	}
	else if ( !token.Icmp( "global5" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL5;
	}
	else if ( !token.Icmp( "global6" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL6;
	}
	else if ( !token.Icmp( "global7" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL7;
	}
	else if ( !token.Icmp( "min" ) || !token.Icmp( "max" ) ) {
		expOpType_t op = ( !token.Icmp( "min" ) ? OP_TYPE_MIN : OP_TYPE_MAX );
		MatchToken( src, "(" );
		int a = ParseExpression( src );
		do {
			MatchToken( src, "," );
			int b = ParseExpression( src );
			a = EmitOp( a, b, op );
		} while ( src.PeekTokenString(",") );
		MatchToken( src, ")" );
		return a;
	}
	else if ( !token.Icmp( "fragmentPrograms" ) ) {
		return GetExpressionConstant( 1 );
	}
	else if ( !token.Icmp( "sound" ) ) {
		pd->registersAreConstant = false;
		return EmitOp( 0, 0, OP_TYPE_SOUND );
	}

	// parse negative numbers
	else if ( token == "-" ) {
		src.ReadToken( &token );
		if ( token.type == TT_NUMBER || token == "." ) {
			return GetExpressionConstant( -(float) token.GetFloatValue() );
		}
		src.Warning( "Bad negative number '%s'", token.c_str() );
		SetMaterialFlag( MF_DEFAULTED );
		return 0;
	}

	else if ( token.type == TT_NUMBER || token == "." || token == "-" ) {
		return GetExpressionConstant( (float) token.GetFloatValue() );
	}

	// see if it is a table name
	const idDeclTable *table = static_cast<const idDeclTable *>( declManager->FindType( DECL_TABLE, token.c_str(), false ) );
	if ( !table ) {
		src.Warning( "Bad term '%s'", token.c_str() );
		SetMaterialFlag( MF_DEFAULTED );
		return 0;
	}

	// parse a table expression
	MatchToken( src, "[" );

	int b = ParseExpression( src );

	MatchToken( src, "]" );

	return EmitOp( table->Index(), b, OP_TYPE_TABLE );
}

/*
=================
idMaterial::ParseExpressionPriority

Returns a register index
=================
*/
#define	TOP_PRIORITY 6
int idMaterial::ParseExpressionPriority( idLexer &src, int priority ) {
	idToken token;

	if ( priority == 0 ) {
		return ParseTerm( src );
	}

	int a = ParseExpressionPriority( src, priority - 1 );

	if ( TestMaterialFlag( MF_DEFAULTED ) ) {	// we have a parse error
		return 0;
	}

	if ( !src.ReadToken( &token ) ) {
		// we won't get EOF in a real file, but we can
		// when parsing from generated strings
		return a;
	}

	auto CheckOperationType = [priority](const idToken &token) -> expOpType_t {
		if ( priority == 1 && token == "*" ) {
			return OP_TYPE_MULTIPLY;
		}
		if ( priority == 1 && token == "/" ) {
			return OP_TYPE_DIVIDE;
		}
		if ( priority == 1 && token == "%" ) {	// implied truncate both to integer
			return OP_TYPE_MOD;
		}
		if ( priority == 2 && token == "+" ) {
			return OP_TYPE_ADD;
		}
		if ( priority == 2 && token == "-" ) {
			return OP_TYPE_SUBTRACT;
		}
		if ( priority == 3 && token == ">" ) {
			return OP_TYPE_GT;
		}
		if ( priority == 3 && token == ">=" ) {
			return OP_TYPE_GE;
		}
		if ( priority == 3 && token == "<" ) {
			return OP_TYPE_LT;
		}
		if ( priority == 3 && token == "<=" ) {
			return OP_TYPE_LE;
		}
		if ( priority == 4 && token == "==" ) {
			return OP_TYPE_EQ;
		}
		if ( priority == 4 && token == "!=" ) {
			return OP_TYPE_NE;
		}
		if ( priority == 5 && token == "&&" ) {
			return OP_TYPE_AND;
		}
		if ( priority == 6 && token == "||" ) {
			return OP_TYPE_OR;
		}
		return OP_TYPE_INVALID;
	};

	expOpType_t operType = CheckOperationType( token );
	while ( operType != OP_TYPE_INVALID ) {
		// stgatilov: build tree with left-to-right evaluation order
		// (original D3 code applied right-to-left order)
		intptr_t b = ParseExpressionPriority( src, priority - 1 );
		a = EmitOp( a, b, operType );

		if ( !src.ReadToken( &token ) ) {
			return a;
		}
		operType = CheckOperationType( token );
	}

	// assume that anything else terminates the expression
	// not too robust error checking...
	src.UnreadToken( &token );

	return a;
}

/*
=================
idMaterial::ParseExpression

Returns a register index
=================
*/
int idMaterial::ParseExpression( idLexer &src ) {
	return ParseExpressionPriority( src, TOP_PRIORITY );
}


static GLSLProgram* GLSL_LoadMaterialStageProgram(const char *name) {
	return programManager->Load( name );
}


/*
===============
idMaterial::ClearStage
===============
*/
void idMaterial::ClearStage( shaderStage_t *ss ) {
	ss->drawStateBits = 0;
	ss->conditionRegister = GetExpressionConstant( 1.0f );
	ss->color.registers[0] =
	ss->color.registers[1] =
	ss->color.registers[2] =
	ss->color.registers[3] = GetExpressionConstant( 1.0f );
}

/*
===============
idMaterial::NameToSrcBlendMode
===============
*/
int idMaterial::NameToSrcBlendMode( const idStr &name ) {
	if ( !name.Icmp( "GL_ONE" ) ) {
		return GLS_SRCBLEND_ONE;
	} else if ( !name.Icmp( "GL_ZERO" ) ) {
		return GLS_SRCBLEND_ZERO;
	} else if ( !name.Icmp( "GL_DST_COLOR" ) ) {
		return GLS_SRCBLEND_DST_COLOR;
	} else if ( !name.Icmp( "GL_ONE_MINUS_DST_COLOR" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	} else if ( !name.Icmp( "GL_SRC_ALPHA" ) ) {
		return GLS_SRCBLEND_SRC_ALPHA;
	} else if ( !name.Icmp( "GL_ONE_MINUS_SRC_ALPHA" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !name.Icmp( "GL_DST_ALPHA" ) ) {
		return GLS_SRCBLEND_DST_ALPHA;
	} else if ( !name.Icmp( "GL_ONE_MINUS_DST_ALPHA" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !name.Icmp( "GL_SRC_ALPHA_SATURATE" ) ) {
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	common->Warning( "unknown blend mode '%s' in material '%s'", name.c_str(), GetName() );
	SetMaterialFlag( MF_DEFAULTED );

	return GLS_SRCBLEND_ONE;
}

/*
===============
idMaterial::NameToDstBlendMode
===============
*/
int idMaterial::NameToDstBlendMode( const idStr &name ) {
	if ( !name.Icmp( "GL_ONE" ) ) {
		return GLS_DSTBLEND_ONE;
	} else if ( !name.Icmp( "GL_ZERO" ) ) {
		return GLS_DSTBLEND_ZERO;
	} else if ( !name.Icmp( "GL_SRC_ALPHA" ) ) {
		return GLS_DSTBLEND_SRC_ALPHA;
	} else if ( !name.Icmp( "GL_ONE_MINUS_SRC_ALPHA" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !name.Icmp( "GL_DST_ALPHA" ) ) {
		return GLS_DSTBLEND_DST_ALPHA;
	} else if ( !name.Icmp( "GL_ONE_MINUS_DST_ALPHA" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !name.Icmp( "GL_SRC_COLOR" ) ) {
		return GLS_DSTBLEND_SRC_COLOR;
	} else if ( !name.Icmp( "GL_ONE_MINUS_SRC_COLOR" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	common->Warning( "unknown blend mode '%s' in material '%s'", name.c_str(), GetName() );
	SetMaterialFlag( MF_DEFAULTED );

	return GLS_DSTBLEND_ONE;
}

/*
================
idMaterial::ParseBlend
================
*/
void idMaterial::ParseBlend( idLexer &src, shaderStage_t *stage ) {
	idToken token;
	int		srcBlend, dstBlend;

	if ( !src.ReadToken( &token ) ) {
		return;
	}

	// blending combinations
	else if ( !token.Icmp( "blend" ) ) {
		stage->drawStateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		return;
	}
	else if ( !token.Icmp( "add" ) ) {
		stage->drawStateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
		return;
	}
	else if ( !token.Icmp( "filter" ) || !token.Icmp( "modulate" ) ) {
		stage->drawStateBits = GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		return;
	}
	else if (  !token.Icmp( "none" ) ) {
		// none is used when defining an alpha mask that doesn't draw
		stage->drawStateBits = GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE;
		return;
	}
	else if ( !token.Icmp( "bumpmap" ) ) {
		stage->lighting = SL_BUMP;
		return;
	}
	else if ( !token.Icmp( "diffusemap" ) ) {
		stage->lighting = SL_DIFFUSE;
		return;
	}
	else if ( !token.Icmp( "specularmap" ) ) {
		stage->lighting = SL_SPECULAR;
		return;
	}

	srcBlend = NameToSrcBlendMode( token );

	MatchToken( src, "," );
	if ( !src.ReadToken( &token ) ) {
		return;
	}
	dstBlend = NameToDstBlendMode( token );

	stage->drawStateBits = srcBlend | dstBlend;
}

/*
================
idMaterial::ParseVertexParm

If there is a single value, it will be repeated across all elements
If there are two values, 3 = 0.0, 4 = 1.0
if there are three values, 4 = 1.0
================
*/
void idMaterial::ParseVertexParm( idLexer &src, newShaderStage_t *newStage ) {
	idToken				token;

	src.ReadTokenOnLine( &token );
	int	parm = token.GetIntValue();
	if ( !token.IsNumeric() || parm < 0 || parm >= MAX_VERTEX_PARMS ) {
		common->Warning( "bad vertexParm number" );
		SetMaterialFlag( MF_DEFAULTED );
		return;
	}
	else if ( parm >= newStage->numVertexParms ) {
		newStage->numVertexParms = parm+1;
	}

	newStage->vertexParms[parm][0] = ParseExpression( src );

	src.ReadTokenOnLine( &token );
	if ( !token[0] || token.Icmp( "," ) ) {
		newStage->vertexParms[parm][1] =
		newStage->vertexParms[parm][2] =
		newStage->vertexParms[parm][3] = newStage->vertexParms[parm][0];
		return;
	}

	newStage->vertexParms[parm][1] = ParseExpression( src );

	src.ReadTokenOnLine( &token );
	if ( !token[0] || token.Icmp( "," ) ) {
		newStage->vertexParms[parm][2] = GetExpressionConstant( 0.0f );
		newStage->vertexParms[parm][3] = GetExpressionConstant( 1.0f );
		return;
	}

	newStage->vertexParms[parm][2] = ParseExpression( src );

	src.ReadTokenOnLine( &token );
	if ( !token[0] || token.Icmp( "," ) ) {
		newStage->vertexParms[parm][3] = GetExpressionConstant( 1.0f );
		return;
	}

	newStage->vertexParms[parm][3] = ParseExpression( src );
}


/*
================
idMaterial::ParseFragmentMap
================
*/
void idMaterial::ParseFragmentMap( idLexer &src, newShaderStage_t *newStage ) {
	const char			*str;
	idToken				token;

	textureFilter_t tf = TF_DEFAULT;
	textureRepeat_t trp = TR_REPEAT;
	textureDepth_t td = TD_DEFAULT;
	cubeFiles_t cubeMap = CF_2D;
	bool allowPicmip = true;

	src.ReadTokenOnLine( &token );
	int	unit = token.GetIntValue();
	if ( !token.IsNumeric() || unit < 0 || unit >= MAX_FRAGMENT_IMAGES ) {
		common->Warning( "bad fragmentMap number" );
		SetMaterialFlag( MF_DEFAULTED );
		return;
	}

	// unit 1 is the normal map.. make sure it gets flagged as the proper depth
	if ( unit == 1 ) {
		td = TD_BUMP;
	}

	if ( unit >= newStage->numFragmentProgramImages ) {
		newStage->numFragmentProgramImages = unit+1;
	}

	while( 1 ) {
		src.ReadTokenOnLine( &token );

		if ( !token.Icmp( "cubeMap" ) ) {
			cubeMap = CF_NATIVE;
			continue;
		}
		else if ( !token.Icmp( "cameraCubeMap" ) ) {
			// note: you can use instead:
			//   cubeMap cameraLayout(textures/env/mycubemap) 
			cubeMap = CF_CAMERA;
			continue;
		}
		else if ( !token.Icmp( "nearest" ) ) {
			tf = TF_NEAREST;
			continue;
		}
		else if ( !token.Icmp( "linear" ) ) {
			tf = TF_LINEAR;
			continue;
		}
		else if ( !token.Icmp( "clamp" ) ) {
			trp = TR_CLAMP;
			continue;
		}
		else if ( !token.Icmp( "noclamp" ) ) {
			trp = TR_REPEAT;
			continue;
		}
		else if ( !token.Icmp( "zeroclamp" ) ) {
			trp = TR_CLAMP_TO_ZERO;
			continue;
		}
		else if ( !token.Icmp( "alphazeroclamp" ) ) {
			trp = TR_CLAMP_TO_ZERO_ALPHA;
			continue;
		}
		else if ( !token.Icmp( "forceHighQuality" ) ) {
			td = TD_HIGH_QUALITY;
			continue;
		}

		else if ( !token.Icmp( "uncompressed" ) || !token.Icmp( "highquality" ) ) {
			if ( !globalImages->image_ignoreHighQuality.GetInteger() ) {
				td = TD_HIGH_QUALITY;
			}
			continue;
		}
		else if ( !token.Icmp( "nopicmip" ) ) {
			allowPicmip = false;
			continue;
		}
		else {
			// assume anything else is the image name
			src.UnreadToken( &token );
			break;
		}
	}

	str = R_ParsePastImageProgram( src );

	idImage* &image = newStage->fragmentProgramImages[unit];
	image = nullptr;
	if ( idImage *existingImg = globalImages->GetImage( str ) ) {
		// allow using scratch textures (e.g. _currentRender)
		if ( existingImg->GetType() == IT_SCRATCH )
			image = existingImg;
	}
	if ( !image ) {
		// asset image may be created or reused
		image = globalImages->ImageFromFile( str, tf, allowPicmip, trp, td, cubeMap );
	}
	if ( !image ) {
		image = globalImages->defaultImage;
	}
}

/*
===============
idMaterial::MultiplyTextureMatrix
===============
*/
void idMaterial::MultiplyTextureMatrix( textureStage_t *ts, int registers[2][3] ) {
	int		old[2][3];

	if ( !ts->hasMatrix ) {
		ts->hasMatrix = true;
		memcpy( ts->matrix, registers, sizeof( ts->matrix ) );
		return;
	}

	memcpy( old, ts->matrix, sizeof( old ) );

	// multiply the two maticies
	ts->matrix[0][0] = EmitOp(
							EmitOp( old[0][0], registers[0][0], OP_TYPE_MULTIPLY ),
							EmitOp( old[0][1], registers[1][0], OP_TYPE_MULTIPLY ), OP_TYPE_ADD );
	ts->matrix[0][1] = EmitOp(
							EmitOp( old[0][0], registers[0][1], OP_TYPE_MULTIPLY ),
							EmitOp( old[0][1], registers[1][1], OP_TYPE_MULTIPLY ), OP_TYPE_ADD );
	ts->matrix[0][2] = EmitOp( 
							EmitOp(
								EmitOp( old[0][0], registers[0][2], OP_TYPE_MULTIPLY ),
								EmitOp( old[0][1], registers[1][2], OP_TYPE_MULTIPLY ), OP_TYPE_ADD ),
							old[0][2], OP_TYPE_ADD );

	ts->matrix[1][0] = EmitOp(
							EmitOp( old[1][0], registers[0][0], OP_TYPE_MULTIPLY ),
							EmitOp( old[1][1], registers[1][0], OP_TYPE_MULTIPLY ), OP_TYPE_ADD );
	ts->matrix[1][1] = EmitOp(
							EmitOp( old[1][0], registers[0][1], OP_TYPE_MULTIPLY ),
							EmitOp( old[1][1], registers[1][1], OP_TYPE_MULTIPLY ), OP_TYPE_ADD );
	ts->matrix[1][2] = EmitOp( 
							EmitOp(
								EmitOp( old[1][0], registers[0][2], OP_TYPE_MULTIPLY ),
								EmitOp( old[1][1], registers[1][2], OP_TYPE_MULTIPLY ), OP_TYPE_ADD ),
							old[1][2], OP_TYPE_ADD );

}

/*
=================
idMaterial::ParseStage

An open brace has been parsed


{
	if <expression>
	map <imageprogram>
	"nearest" "linear" "clamp" "zeroclamp" "uncompressed" "highquality" "nopicmip"
	scroll, scale, rotate
}

=================
*/
void idMaterial::ParseStage( idLexer &src, const textureRepeat_t trpDefault ) {
	idToken				token;
	const char			*str;
	shaderStage_t		*ss;
	textureStage_t		*ts;
	textureFilter_t		tf;
	textureRepeat_t		trp;
	textureDepth_t		td;
	cubeFiles_t			cubeMap;
	bool				allowPicmip;
	char				imageName[MAX_IMAGE_NAME];
	int					a, b;
	int					matrix[2][3];
	newShaderStage_t	newStage;

	if ( numStages >= MAX_SHADER_STAGES ) {
		SetMaterialFlag( MF_DEFAULTED );
		common->Warning( "material '%s' exceeded %i stages", GetName(), MAX_SHADER_STAGES );
	}

	tf = TF_DEFAULT;
	td = TD_DEFAULT;
	trp = trpDefault;
	allowPicmip = true;
	cubeMap = CF_2D;

	imageName[0] = 0;

	memset( &newStage, 0, sizeof( newStage ) );

	ss = &pd->parseStages[numStages];
	ts = &ss->texture;
	pd->usesFrobParm = false;

	ClearStage( ss );

	while ( 1 ) {
		if ( TestMaterialFlag( MF_DEFAULTED ) ) {	// we have a parse error
			return;
		}
		else if ( !src.ExpectAnyToken( &token ) ) {
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}

		// the close brace for the entire material ends the draw block
		else if ( token == "}" ) {
			break;
		}

		//BSM Nerve: Added for stage naming in the material editor
		else if(  !token.Icmp( "name") ) {
			src.SkipRestOfLine();	// Serp - I am not sure that this should be used here, materials may not be folded.
									//        left it in for safety.
			//src.ReadTokenOnLine( &token ); // replace with?
			continue;
		}

		// image options
		else if ( !token.Icmp( "blend" ) ) {
			ParseBlend( src, ss );
			continue;
		}

		else if (  !token.Icmp( "map" ) ) {
			str = R_ParsePastImageProgram( src );
			idStr::Copynz( imageName, str, sizeof( imageName ) );
			continue;
		}

		else if ( !token.Icmp( "remoteRenderMap" ) ) {
			ts->dynamic = DI_REMOTE_RENDER;
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			continue;
		}

		else if ( !token.Icmp( "portalRenderMap" ) ) {
			ts->dynamic = DI_PORTAL_RENDER;
			continue;
		}

		else if ( !token.Icmp( "mirrorRenderMap" ) ) {
			ts->dynamic = DI_MIRROR_RENDER;
			//ts->width = src.ParseInt();
			//ts->height = src.ParseInt();
			src.SkipRestOfLine();
			ts->texgen = TG_SCREEN;
			continue;
		}

		else if (  !token.Icmp( "xrayRenderMap" ) ) {
			ts->dynamic = DI_XRAY_RENDER;
			//ts->width = src.ParseInt();
			//ts->height = src.ParseInt();
			ts->width = 0;
			if ( src.ReadTokenOnLine( &token ) ) {
				if ( !token.Icmp( "inclusive" ) ) {
					ts->width = 1;
				}
			}
			src.SkipRestOfLine();
			ts->texgen = TG_SCREEN;
			continue;
		}
		else if (  !token.Icmp( "screen" ) ) {
			ts->texgen = TG_SCREEN;
			continue;
		}
		else if (  !token.Icmp( "screen2" ) ) {
			ts->texgen = TG_SCREEN; //TG_SCREEN2; duzenko: treated by renderer same as TG_SCREEN, simplified
			continue;
		}

		else if ( !token.Icmp( "videomap" ) ) {
			bool withAudio = false;
			bool loop = false;
			bool error = false;
			while (true) {
				// note that videomaps will always be in clamp mode, so texture
				// coordinates had better be in the 0 to 1 range
				if ( !src.ReadToken( &token ) ) {
					common->Warning( "missing parameter for 'videoMap' keyword in material '%s'", GetName() );
					error = true;
					break;
				}
				if ( !token.Icmp( "loop" ) )
					loop = true;
				else if ( !token.Icmp( "withAudio" ) )
					withAudio = true;		//#4534
				else
					break;
			}
			if (loop && withAudio) {
					common->Warning( "Enabling both 'loop' and 'withAudio' for 'videoMap' is not implemented (material '%s')", GetName() );
					loop = false;
			}
			if (!error) {
				ts->cinematic = idCinematic::Alloc( token.c_str() );
				ts->cinematic->InitFromFile( token.c_str(), loop, withAudio );
			}
			continue;
		}
		else if ( !token.Icmp( "soundmap" ) ) {
			if ( !src.ReadToken( &token ) ) {
				common->Warning( "missing parameter for 'soundmap' keyword in material '%s'", GetName() );
				continue;
			}
			ts->cinematic = new idSndWindow();
			ts->cinematic->InitFromFile( token.c_str(), true );
			continue;
		}

		else if ( !token.Icmp( "cubeMap" ) ) {
			str = R_ParsePastImageProgramCubeMap( src );
			idStr::Copynz( imageName, str, sizeof( imageName ) );
			cubeMap = CF_NATIVE;
			continue;
		}

		else if ( !token.Icmp( "cameraCubeMap" ) ) {
			str = R_ParsePastImageProgramCubeMap( src );
			idStr::Copynz( imageName, str, sizeof( imageName ) );
			cubeMap = CF_CAMERA;
			continue;
		}

		else if ( !token.Icmp( "ignoreAlphaTest" ) ) {
			ss->ignoreAlphaTest = true;
			continue;
		}
		else if ( !token.Icmp( "nearest" ) ) {
			tf = TF_NEAREST;
			continue;
		}
		else if ( !token.Icmp( "linear" ) ) {
			tf = TF_LINEAR;
			continue;
		}
		else if ( !token.Icmp( "clamp" ) ) {
			trp = TR_CLAMP;
			continue;
		}
		else if ( !token.Icmp( "noclamp" ) ) {
			trp = TR_REPEAT;
			continue;
		}
		else if ( !token.Icmp( "zeroclamp" ) ) {
			trp = TR_CLAMP_TO_ZERO;
			continue;
		}
		else if ( !token.Icmp( "alphazeroclamp" ) ) {
			trp = TR_CLAMP_TO_ZERO_ALPHA;
			continue;
		}
		else if ( !token.Icmp( "uncompressed" ) || !token.Icmp( "highquality" ) ) {
			if ( !globalImages->image_ignoreHighQuality.GetInteger() ) {
				td = TD_HIGH_QUALITY;
			}
			continue;
		}
		else if ( !token.Icmp( "forceHighQuality" ) ) {
			td = TD_HIGH_QUALITY;
			continue;
		}
		else if ( !token.Icmp( "nopicmip" ) ) {
			allowPicmip = false;
			continue;
		}
		else if ( !token.Icmp( "vertexColor" ) ) {
			ss->vertexColor = SVC_MODULATE;
			continue;
		}
		else if ( !token.Icmp( "inverseVertexColor" ) ) {
			ss->vertexColor = SVC_INVERSE_MODULATE;
			continue;
		}

		// privatePolygonOffset
		else if ( !token.Icmp( "privatePolygonOffset" ) ) {
			if ( !src.ReadTokenOnLine( &token ) ) {
				ss->privatePolygonOffset = 1.0f;
				continue;
			}
			// explict larger (or negative) offset
			src.UnreadToken( &token );
			ss->privatePolygonOffset = src.ParseFloat();
			continue;
		}

		// texture coordinate generation
		else if ( !token.Icmp( "texGen" ) ) {
			src.ExpectAnyToken( &token );
			/*if ( !token.Icmp( "normal" ) ) {
				ts->texgen = TG_DIFFUSE_CUBE;
			} else */if ( !token.Icmp( "reflect" ) ) {
				ts->texgen = TG_REFLECT_CUBE;
			} else if ( !token.Icmp( "skybox" ) ) {
				ts->texgen = TG_SKYBOX_CUBE;
			} else if ( !token.Icmp( "wobbleSky" ) ) {
				ts->texgen = TG_WOBBLESKY_CUBE;
				texGenRegisters[0] = ParseExpression( src );
				texGenRegisters[1] = ParseExpression( src );
				texGenRegisters[2] = ParseExpression( src );
			} else {
				common->Warning( "bad texGen '%s' in material %s", token.c_str(), GetName() );
				SetMaterialFlag( MF_DEFAULTED );
			}
			continue;
		}
		else if ( !token.Icmp( "scroll" ) || !token.Icmp( "translate" ) ) {
			a = ParseExpression( src );
			MatchToken( src, "," );
			b = ParseExpression( src );
			matrix[0][0] = GetExpressionConstant( 1.0f );
			matrix[0][1] = GetExpressionConstant( 0.0f );
			matrix[0][2] = a;
			matrix[1][0] = GetExpressionConstant( 0.0f );
			matrix[1][1] = GetExpressionConstant( 1.0f );
			matrix[1][2] = b;

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}
		else if ( !token.Icmp( "scale" ) ) {
			a = ParseExpression( src );
			MatchToken( src, "," );
			b = ParseExpression( src );
			// this just scales without a centering
			matrix[0][0] = a;
			matrix[0][1] = GetExpressionConstant( 0.0f );
			matrix[0][2] = GetExpressionConstant( 0.0f );
			matrix[1][0] = GetExpressionConstant( 0.0f );
			matrix[1][1] = b;
			matrix[1][2] = GetExpressionConstant( 0.0f );

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}
		else if ( !token.Icmp( "centerScale" ) ) {
			a = ParseExpression( src );
			MatchToken( src, "," );
			b = ParseExpression( src );
			// this subtracts 0.5, then scales, then adds 0.5
			matrix[0][0] = a;
			matrix[0][1] = GetExpressionConstant( 0.0f );
			matrix[0][2] = EmitOp( GetExpressionConstant( 0.5f ), EmitOp( GetExpressionConstant( 0.5f ), a, OP_TYPE_MULTIPLY ), OP_TYPE_SUBTRACT );
			matrix[1][0] = GetExpressionConstant( 0.0f );
			matrix[1][1] = b;
			matrix[1][2] = EmitOp( GetExpressionConstant( 0.5f ), EmitOp( GetExpressionConstant( 0.5f ), b, OP_TYPE_MULTIPLY ), OP_TYPE_SUBTRACT );

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}
		else if ( !token.Icmp( "shear" ) ) {
			a = ParseExpression( src );
			MatchToken( src, "," );
			b = ParseExpression( src );
			// this subtracts 0.5, then shears, then adds 0.5
			matrix[0][0] = GetExpressionConstant( 1.0f );
			matrix[0][1] = a;
			matrix[0][2] = EmitOp( GetExpressionConstant( -0.5f ), a, OP_TYPE_MULTIPLY );
			matrix[1][0] = b;
			matrix[1][1] = GetExpressionConstant( 1.0f );
			matrix[1][2] = EmitOp( GetExpressionConstant( -0.5f ), b, OP_TYPE_MULTIPLY );

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}
		else if ( !token.Icmp( "rotate" ) ) {
			const idDeclTable *table;
			// in cycles
			a = ParseExpression( src );

			table = static_cast<const idDeclTable *>( declManager->FindType( DECL_TABLE, "sinTable", false ) );
			if ( !table ) {
				common->Warning( "no sinTable for rotate defined" );
				SetMaterialFlag( MF_DEFAULTED );
				return;
			}
			const int sinReg = EmitOp( table->Index(), a, OP_TYPE_TABLE );

			table = static_cast<const idDeclTable *>( declManager->FindType( DECL_TABLE, "cosTable", false ) );
			if ( !table ) {
				common->Warning( "no cosTable for rotate defined" );
				SetMaterialFlag( MF_DEFAULTED );
				return;
			}
			const int cosReg = EmitOp( table->Index(), a, OP_TYPE_TABLE );

			// this subtracts 0.5, then rotates, then adds 0.5
			matrix[0][0] = cosReg;
			matrix[0][1] = EmitOp( GetExpressionConstant( 0.0f ), sinReg, OP_TYPE_SUBTRACT );
			matrix[0][2] = EmitOp( EmitOp( EmitOp( GetExpressionConstant( -0.5f ), cosReg, OP_TYPE_MULTIPLY ), 
										EmitOp( GetExpressionConstant( 0.5f ), sinReg, OP_TYPE_MULTIPLY ), OP_TYPE_ADD ),
										GetExpressionConstant( 0.5f ), OP_TYPE_ADD );

			matrix[1][0] = sinReg;
			matrix[1][1] = cosReg;
			matrix[1][2] = EmitOp( EmitOp( EmitOp( GetExpressionConstant( -0.5f ), sinReg, OP_TYPE_MULTIPLY ), 
										EmitOp( GetExpressionConstant( -0.5f ), cosReg, OP_TYPE_MULTIPLY ), OP_TYPE_ADD ),
										GetExpressionConstant( 0.5f ), OP_TYPE_ADD );

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}

		// color mask options
		else if ( !token.Icmp( "maskRed" ) ) {
			ss->drawStateBits |= GLS_REDMASK;
			continue;
		}		
		else if ( !token.Icmp( "maskGreen" ) ) {
			ss->drawStateBits |= GLS_GREENMASK;
			continue;
		}		
		else if ( !token.Icmp( "maskBlue" ) ) {
			ss->drawStateBits |= GLS_BLUEMASK;
			continue;
		}		
		else if ( !token.Icmp( "maskAlpha" ) ) {
			ss->drawStateBits |= GLS_ALPHAMASK;
			continue;
		}		
		else if ( !token.Icmp( "maskColor" ) ) {
			ss->drawStateBits |= GLS_COLORMASK;
			continue;
		}		
		else if ( !token.Icmp( "maskDepth" ) ) {
			ss->drawStateBits |= GLS_DEPTHMASK;
			continue;
		}
		else if ( !token.Icmp( "ignoreDepth" ) ) {		// Added in #3877.
			ss->drawStateBits |= GLS_DEPTHFUNC_ALWAYS;
			continue;
		}
		else if ( !token.Icmp( "alphaTest" ) ) {
			ss->hasAlphaTest = true;
			ss->alphaTestRegister = ParseExpression( src );
			coverage = MC_PERFORATED;
			continue;
		}		

		// shorthand for 2D modulated
		else if ( !token.Icmp( "colored" ) ) {
			ss->color.registers[0] = EXP_REG_PARM0;
			ss->color.registers[1] = EXP_REG_PARM1;
			ss->color.registers[2] = EXP_REG_PARM2;
			ss->color.registers[3] = EXP_REG_PARM3;
			pd->registersAreConstant = false;
			continue;
		}
		else if ( !token.Icmp( "color" ) ) {
			ss->color.registers[0] = ParseExpression( src );
			MatchToken( src, "," );
			ss->color.registers[1] = ParseExpression( src );
			MatchToken( src, "," );
			ss->color.registers[2] = ParseExpression( src );
			MatchToken( src, "," );
			ss->color.registers[3] = ParseExpression( src );
			continue;
		}
		else if ( !token.Icmp( "red" ) ) {
			ss->color.registers[0] = ParseExpression( src );
			continue;
		}
		else if ( !token.Icmp( "green" ) ) {
			ss->color.registers[1] = ParseExpression( src );
			continue;
		}
		else if ( !token.Icmp( "blue" ) ) {
			ss->color.registers[2] = ParseExpression( src );
			continue;
		}
		else if ( !token.Icmp( "alpha" ) ) {
			ss->color.registers[3] = ParseExpression( src );
			continue;
		}
		else if ( !token.Icmp( "rgb" ) ) {
			ss->color.registers[0] = ss->color.registers[1] = 
				ss->color.registers[2] = ParseExpression( src );
			continue;
		}
		else if ( !token.Icmp( "rgba" ) ) {
			ss->color.registers[0] = ss->color.registers[1] = 
				ss->color.registers[2] = ss->color.registers[3] = ParseExpression( src );
			continue;
		}
		else if ( !token.Icmp( "if" ) ) {
			ss->conditionRegister = ParseExpression( src );
			continue;
		}
		else if ( !token.Icmp( "program" ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				idStr fileExt;
				token.ExtractFileExtension( fileExt );
				token.StripFileExtension();
				newStage.glslProgram = GLSL_LoadMaterialStageProgram( token );
			}
			continue;
		}
		else if ( !token.Icmp( "fragmentProgram" ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				token.StripFileExtension();
				newStage.glslProgram = GLSL_LoadMaterialStageProgram( token );
			}
			continue;
		}
		else if ( !token.Icmp( "vertexProgram" ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				token.StripFileExtension();
				newStage.glslProgram = GLSL_LoadMaterialStageProgram( token );
			}
			continue;
		}
		/*else if ( !token.Icmp( "megaTexture" ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				newStage.megaTexture = new idMegaTexture;
				if ( !newStage.megaTexture->InitFromMegaFile( token.c_str() ) ) {
					delete newStage.megaTexture;
					SetMaterialFlag( MF_DEFAULTED );
					continue;
				}
				newStage.vertexProgram = R_FindARBProgram( GL_VERTEX_PROGRAM_ARB, "megaTexture.vfp" );
				newStage.fragmentProgram = R_FindARBProgram( GL_FRAGMENT_PROGRAM_ARB, "megaTexture.vfp" );
				continue;
			}
		}*/

		else if ( !token.Icmp( "vertexParm" ) ) {
			ParseVertexParm( src, &newStage );
			continue;
		}

		else if (  !token.Icmp( "fragmentMap" ) ) {	
			ParseFragmentMap( src, &newStage );
			continue;
		} else {
			common->Warning( "unknown token '%s' in material '%s'", token.c_str(), GetName() );
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}
	}

	// if we are using newStage, allocate a copy of it
	if ( newStage.glslProgram ) {
		ss->newStage = (newShaderStage_t *)Mem_Alloc( sizeof( newStage ) );
		*(ss->newStage) = newStage;
	}

	if ( pd->usesFrobParm ) {
		// stgatilov #5427: using parm11 outside "frob stage" is wrong
		if ( !IsFrobStage( numStages ) )
			common->Warning( "Material '%s' stage %d uses parm11 without frobstage condition", GetName(), numStages );
	}

	// successfully parsed a stage
	numStages++;

	// select a compressed depth based on what the stage is
	if ( td == TD_DEFAULT ) {
		switch( ss->lighting ) {
		case SL_BUMP:
			td = TD_BUMP;
			break;
		case SL_DIFFUSE:
			td = TD_DIFFUSE;
			break;
		case SL_SPECULAR:
			td = TD_SPECULAR;
			break;
		default:
			break;
		}
	}

	// now load the image with all the parms we parsed
	assert( !ts->image );
	if ( imageName[0] ) {
		if ( idImage *existingImg = globalImages->GetImage( imageName ) ) {
			// allow using scratch textures (e.g. _currentRender)
			if ( existingImg->GetType() == IT_SCRATCH )
				ts->image = existingImg;
		}
		if ( !ts->image ) {
			// asset image may be created or reused
			ts->image = globalImages->ImageFromFile( imageName, tf, allowPicmip, trp, td, cubeMap );
		}
		if ( !ts->image ) {
			ts->image = globalImages->defaultImage;
		}
	} else if ( !ts->cinematic && !ts->dynamic && !ss->newStage ) {
		common->Warning( "material '%s' had stage with no image", GetName() );
		ts->image = globalImages->defaultImage;
	}
}

/*
===============
idMaterial::ParseDeform
===============
*/
void idMaterial::ParseDeform( idLexer &src ) {
	idToken token;

	if ( !src.ExpectAnyToken( &token ) ) {
		return;
	}

	else if ( !token.Icmp( "sprite" ) ) {
		deform = DFRM_SPRITE;
		cullType = CT_TWO_SIDED;
		SetMaterialFlag( MF_NOSHADOWS );
		return;
	}
	else if ( !token.Icmp( "tube" ) ) {
		deform = DFRM_TUBE;
		cullType = CT_TWO_SIDED;
		SetMaterialFlag( MF_NOSHADOWS );
		return;
	}
	else if ( !token.Icmp( "flare" ) ) {
		deform = DFRM_FLARE;
		cullType = CT_TWO_SIDED;
		deformRegisters[0] = ParseExpression( src );
		SetMaterialFlag( MF_NOSHADOWS );
		return;
	}
	else if ( !token.Icmp( "expand" ) ) {
		deform = DFRM_EXPAND;
		deformRegisters[0] = ParseExpression( src );
		return;
	}
	else if ( !token.Icmp( "move" ) ) {
		deform = DFRM_MOVE;
		deformRegisters[0] = ParseExpression( src );
		return;
	}
	else if ( !token.Icmp( "turbulent" ) ) {
		deform = DFRM_TURB;

		if ( !src.ExpectAnyToken( &token ) ) {
			src.Warning( "deform particle missing particle name" );
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}
		deformDecl = declManager->FindType( DECL_TABLE, token.c_str(), true );

		deformRegisters[0] = ParseExpression( src );
		deformRegisters[1] = ParseExpression( src );
		deformRegisters[2] = ParseExpression( src );
		return;
	}
	else if ( !token.Icmp( "eyeBall" ) ) {
		deform = DFRM_EYEBALL;
		return;
	}
	else if ( !token.Icmp( "particle" ) ) {
		deform = DFRM_PARTICLE;
		if ( !src.ExpectAnyToken( &token ) ) {
			src.Warning( "deform particle missing particle name" );
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}
		deformDecl = declManager->FindType( DECL_PARTICLE, token.c_str(), true );
		return;
	}
	else if ( !token.Icmp( "particle2" ) ) {
		deform = DFRM_PARTICLE2;
		if ( !src.ExpectAnyToken( &token ) ) {
			src.Warning( "deform particle missing particle name" );
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}
		deformDecl = declManager->FindType( DECL_PARTICLE, token.c_str(), true );
		return;
	} else {
		src.Warning( "Bad deform type '%s'", token.c_str() );
		SetMaterialFlag( MF_DEFAULTED );
	}
}

idCVar r_frobDefaultEnable("r_frobDefaultEnable", "1", CVAR_RENDERER | CVAR_BOOL, "Should we add implicit frobstages if none are present?");
idCVar r_frobDefaultAdd("r_frobDefaultAdd", "0.4", CVAR_RENDERER | CVAR_FLOAT, "Plain color value added in implicitly generated material frobstage");
idCVar r_frobDefaultMult("r_frobDefaultMult", "0.15", CVAR_RENDERER | CVAR_FLOAT, "Multiplier of diffuse texture color added in implicitly generated material frobstage");

/*
==============
idMaterial::AddImplicitStages

If a material has diffuse or specular stages without any
bump stage, add an implicit _flat bumpmap stage.

It is valid to have either a diffuse or specular without the other.

It is valid to have a reflection map and a bump map for bumpy reflection
==============
*/
void idMaterial::AddImplicitStages( const textureRepeat_t trpDefault /* = TR_REPEAT  */ ) {
	char	buffer[1024];
	idLexer		newSrc;
	bool hasDiffuse = false;
	idImage *diffuseTex = nullptr;
	bool hasSpecular = false;
	bool hasBump = false;
	bool hasReflection = false;
	bool hasFrobStage = false, isFrobStandard = false;

	for ( int i = 0 ; i < numStages ; i++ ) {
		if ( pd->parseStages[i].lighting == SL_BUMP ) {
			hasBump = true;
		}
		if ( pd->parseStages[i].lighting == SL_DIFFUSE ) {
			diffuseTex = pd->parseStages[i].texture.image;
			hasDiffuse = true;
		}
		if ( pd->parseStages[i].lighting == SL_SPECULAR ) {
			hasSpecular = true;
		}
		if ( pd->parseStages[i].texture.texgen == TG_REFLECT_CUBE ) {
			hasReflection = true;
		}
		if ( IsFrobStage( i, &isFrobStandard) ) {
			// stgatilov #5427: warn about nonstandard frobstage condition
			if ( !isFrobStandard )
				common->Warning( "Material '%s' frobstage %d: bad condition, should be 'if (parm11 > 0)'", GetName(), i );
			hasFrobStage = true;
		}
	}

	if ( hasBump || hasDiffuse || hasSpecular ) {
		// if it has any interaction, then it should have bumpmap
		if ( !hasBump ) {
			idStr::snPrintf( buffer, sizeof( buffer ),
				"blend bumpmap\n"
				"map _flat\n"
				"}\n"	// finishes stage
			);
			newSrc.LoadMemory(buffer, static_cast<int>(strlen(buffer)), "bumpmap");
			newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
			ParseStage( newSrc, trpDefault );
			newSrc.FreeSource();
		}

		// stgatilov #5427: no frobstage found -> add default stages implicitly
		if ( !hasFrobStage && r_frobDefaultEnable.GetBool() ) {
			AddFrobStages(
				idVec3( r_frobDefaultAdd.GetFloat() ),
				diffuseTex ? diffuseTex->imgName.c_str() : nullptr,
				idVec3( r_frobDefaultMult.GetFloat() ),
				trpDefault
			);
		}
	}
}

/*
===============
idMaterial::SortInteractionStages

The renderer expects bump, then diffuse, then specular
There can be multiple bump maps, followed by additional
diffuse and specular stages, which allows cross-faded bump mapping.

Ambient stages can be interspersed anywhere, but they are
ignored during interactions, and all the interaction
stages are ignored during ambient drawing.
===============
*/
void idMaterial::SortInteractionStages() {
	int		j;

	for ( int i = 0 ; i < numStages ; i = j ) {
		// find the next bump map
		for ( j = i + 1 ; j < numStages ; j++ ) {
			if ( pd->parseStages[j].lighting == SL_BUMP ) {
				// if the very first stage wasn't a bumpmap,
				// this bumpmap is part of the first group
				if ( pd->parseStages[i].lighting != SL_BUMP ) {
					continue;
				}
				break;
			}
		}

		// bubble sort everything bump / diffuse / specular
		for ( int l = 1 ; l < j-i ; l++ ) {
			for ( int k = i ; k < j-l ; k++ ) {
				if ( pd->parseStages[k].lighting > pd->parseStages[k+1].lighting ) {
					shaderStage_t	temp;

					temp = pd->parseStages[k];
					pd->parseStages[k] = pd->parseStages[k+1];
					pd->parseStages[k+1] = temp;
				}
			}
		}
	}
}

/*
=================
idMaterial::CheckAlphaColorDependencies

stgatilov #6340: The output color at the end of material rendering should not depend on destination alpha at the start of rendering.
Because framebuffer alpha contains some invisible trash values leftover from previous rendering of independent objects.

This defect can only appear due to using DST_ALPHA blending modes.
The only valid usage of these modes is to read intermediate alpha image generated by a prior stage of the same material.
=================
*/
void idMaterial::CheckAlphaColorDependencies() const {
	// We track dependencies symbolically.
	// A matrix M[i][j] = true means that i-th component of output might depend on j-th component of input.
	// M[i][j] = false means that there is surely no such dependency.
	// There are only two components:
	//   idx = 0: color   (red, green, blue --- all considered to be single value)
	//   idx = 1: alpha

	// how output after the current stage depends on the initial input
	// initialize with identity matrix
	bool currentDependsOnInitial[2][2] = {{true, false}, {false, true}};

	for ( int i = 0; i < numStages; i++ ) {
		const shaderStage_t& stage = pd->parseStages[i];
		// bump/diffuse/specular are special interaction stages, their blending equation is hardcoded
		if ( stage.lighting != SL_AMBIENT )
			continue;
		// output = dstFactor * input + srcFactor * texture
		int srcFactor = stage.drawStateBits & GLS_SRCBLEND_BITS;
		int dstFactor = stage.drawStateBits & GLS_DSTBLEND_BITS;

		// how new value after this stage depends on old value before this stage
		bool depends[2][2] = {{false}};
		if ( srcFactor == GLS_SRCBLEND_DST_COLOR || srcFactor == GLS_SRCBLEND_ONE_MINUS_DST_COLOR )
			depends[0][0] = depends[1][1] = true;
		if ( srcFactor == GLS_SRCBLEND_DST_ALPHA || srcFactor == GLS_SRCBLEND_ONE_MINUS_DST_ALPHA || srcFactor == GLS_SRCBLEND_ALPHA_SATURATE )
			depends[0][1] = depends[1][1] = true;
		if ( dstFactor != GLS_DSTBLEND_ZERO )	// note: gl_zero dst mode is the only way to break dependency chain
			depends[0][0] = depends[1][1] = true;
		if ( dstFactor == GLS_DSTBLEND_DST_ALPHA || dstFactor == GLS_DSTBLEND_ONE_MINUS_DST_ALPHA )
			depends[0][1] = true;

		// multiply 2 x 2 dependency matrices: [initial -> current] * [current -> next]
		bool nextDependsOnInitial[2][2] = {{false}};
		for ( int a = 0; a < 2; a++ )
			for ( int b = 0; b < 2; b++ )
				for ( int c = 0; c < 2; c++ )
					if ( depends[a][b] && currentDependsOnInitial[b][c] )
						nextDependsOnInitial[a][c] = true;

		// if color/alpha is masked during stage, then old contents remain (and old dependency)
		if ( (stage.drawStateBits & GLS_COLORMASK) != GLS_COLORMASK )	// unless color is fully masked
			memcpy( currentDependsOnInitial[0], nextDependsOnInitial[0], sizeof(currentDependsOnInitial[0]) );
		if ( (stage.drawStateBits & GLS_ALPHAMASK) != GLS_ALPHAMASK )	// unless alpha is masked
			memcpy( currentDependsOnInitial[1], nextDependsOnInitial[1], sizeof(currentDependsOnInitial[1]) );
	}

	// can final color after rendering material depend on framebuffer alpha before rendering it?
	if ( currentDependsOnInitial[0][1] ) {
		// complain! more likely gl_dst_alpha should be replaced with gl_one
		common->Warning( "material '%s' output color depends on input alpha (#6340)", GetName() );
	}
}

/*
=================
idMaterial::ParseMaterial

The current text pointer is at the explicit text definition of the
Parse it into the global material variable. Later functions will optimize it.

If there is any error during parsing, defaultShader will be set.
=================
*/
void idMaterial::ParseMaterial( idLexer &src ) {
	idToken		token;
	char		buffer[1024];
	const char	*str;
	idLexer		newSrc;

	numOps = 0;
	numRegisters = EXP_REG_NUM_PREDEFINED;	// leave space for the parms to be copied in
	numStages = 0;
	for ( int i = 0 ; i < numRegisters ; i++ ) {
		pd->registerIsTemporary[i] = true;		// they aren't constants that can be folded
	}

	textureRepeat_t	trpDefault = TR_REPEAT;		// allow a global setting for repeat

	while ( 1 ) {
		if ( TestMaterialFlag( MF_DEFAULTED ) ) {	// we have a parse error
			return;
		}
		else if ( !src.ExpectAnyToken( &token ) ) {
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}

		// end of material definition
		else if ( token == "}" ) {
			break;
		}
		else if ( !token.Icmp( "qer_editorimage") ) {
			src.ReadTokenOnLine( &token );
			editorImageName = token.c_str();
			//src.SkipRestOfLine(); // Serp - I am not sure that this should be used here, materials may not be folded.
			continue;
		}
		// description
		else if ( !token.Icmp( "description") ) {
			src.ReadTokenOnLine( &token );
			desc = token.c_str();
			continue;
		}
		// check for the surface / content bit flags
		else if ( CheckSurfaceParm( &token ) ) {
			continue;
		}
		else if ( !token.Icmp( "materialImage" ) ) {
			src.ReadTokenOnLine( &token );
			materialImage = token.c_str();
			continue;
		}
		// polygonOffset
		else if ( !token.Icmp( "polygonOffset" ) ) {
			SetMaterialFlag( MF_POLYGONOFFSET );
			if ( !src.ReadTokenOnLine( &token ) ) {
				polygonOffset = 1.0f;
				continue;
			}
			// explict larger (or negative) offset
			polygonOffset = token.GetFloatValue();
			continue;
		}
		// fogAlpha
		else if ( !token.Icmp( "fogAlpha" ) ) {
			if ( !src.ReadTokenOnLine( &token ) ) {
				fogAlpha = 1.0f;
				continue;
			}
			fogAlpha = token.GetFloatValue();
			continue;
		}
		// noshadow
		else if ( !token.Icmp( "noShadows" ) ) {
			SetMaterialFlag( MF_NOSHADOWS );
			continue;
		}
		else if ( !token.Icmp( "suppressInSubview" ) ) {
			suppressInSubview = true;
			continue;
		}
		else if ( !token.Icmp( "portalSky" ) ) {
			portalSky = true;
			continue;
		}
		// noSelfShadow
		else if ( !token.Icmp( "noSelfShadow" ) ) {
			SetMaterialFlag( MF_NOSELFSHADOW );
			continue;
		}
		// noPortalFog
		else if ( !token.Icmp( "noPortalFog" ) ) {
			SetMaterialFlag( MF_NOPORTALFOG );
			continue;
		}
		// forceShadows allows nodraw surfaces to cast shadows
		else if ( !token.Icmp( "forceShadows" ) ) {
			SetMaterialFlag( MF_FORCESHADOWS );
			continue;
		}
		// overlay / decal suppression
		else if ( !token.Icmp( "noOverlays" ) ) {
			allowOverlays = false;
			continue;
		}
		// nbohr1more: #4379 lightgem culling
		else if ( !token.Icmp( "islightgemsurf" ) ) {
			isLightgemSurf = true;
			continue;
		}
		// moster blood overlay forcing for alpha tested or translucent surfaces
		else if ( !token.Icmp( "forceOverlays" ) ) {
			pd->forceOverlays = true;
			continue;
		}
		// translucent
		else if ( !token.Icmp( "translucent" ) ) {
			coverage = MC_TRANSLUCENT;
			continue;
		}
		// global zero clamp
		else if ( !token.Icmp( "zeroclamp" ) ) {
			trpDefault = TR_CLAMP_TO_ZERO;
			continue;
		}
		// global clamp
		else if ( !token.Icmp( "clamp" ) ) {
			trpDefault = TR_CLAMP;
			continue;
		}
		// global clamp
		else if ( !token.Icmp( "alphazeroclamp" ) ) {
			trpDefault = TR_CLAMP_TO_ZERO;
			continue;
		}
		// forceOpaque is used for skies-behind-windows
		else if ( !token.Icmp( "forceOpaque" ) ) {
			coverage = MC_OPAQUE;
			continue;
		}
		// twoSided
		else if ( !token.Icmp( "twoSided" ) ) {
			cullType = CT_TWO_SIDED;
			// twoSided implies no-shadows, because the shadow
			// volume would be coplanar with the surface, giving depth fighting
			// we could make this no-self-shadows, but it may be more important
			// to receive shadows from no-self-shadow monsters
			SetMaterialFlag( MF_NOSHADOWS );
		}
		// backSided
		else if ( !token.Icmp( "backSided" ) ) {
			cullType = CT_BACK_SIDED;
			// the shadow code doesn't handle this, so just disable shadows.
			// We could fix this in the future if there was a need.
			SetMaterialFlag( MF_NOSHADOWS );
		}
		// foglight
		else if ( !token.Icmp( "fogLight" ) ) {
			fogLight = true;
			continue;
		}
		// blendlight
		else if ( !token.Icmp( "blendLight" ) ) {
			blendLight = true;
			continue;
		}
		// ambientLight
		else if ( !token.Icmp( "ambientLight" ) ) {
			ambientLight = true;
			continue;
		}
		// nbohr1more #3881: cubicLight
		else if ( !token.Icmp( "cubicLight" ) ) {
			cubicLight = true;
			continue;
		}
		// mirror
		else if ( !token.Icmp( "mirror" ) ) {
			sort = SS_SUBVIEW;
			coverage = MC_OPAQUE;
			continue;
		}
		// noFog
		else if ( !token.Icmp( "noFog" ) ) {
			noFog = true;
			continue;
		}
		// unsmoothedTangents
		else if ( !token.Icmp( "unsmoothedTangents" ) ) {
			unsmoothedTangents = true;
			continue;
		}
		// lightFalloffImage <imageprogram>
		// specifies the image to use for the third axis of projected
		// light volumes
		else if ( !token.Icmp( "lightFalloffImage" ) ) {
			str = R_ParsePastImageProgram( src );
			idStr copy = str;	// so other things don't step on it
			lightFalloffImage = globalImages->ImageFromFile( copy, TF_DEFAULT, false, TR_CLAMP /* TR_CLAMP_TO_ZERO */, TD_DEFAULT );
			continue;
		}
		// lightAmbientDiffuse <imageprogram>
		// specifies the image indexed by surface normal in world space
		// contains ambient-diffuse light from environment (stgatilov #6090)
		else if ( !token.Icmp( "lightAmbientDiffuse" ) ) {
			str = R_ParsePastImageProgramCubeMap( src );
			idStr copy = str;	// so other things don't step on it
			lightAmbientDiffuse = globalImages->ImageFromFile( copy, TF_DEFAULT, false, TR_CLAMP /* TR_CLAMP_TO_ZERO */, TD_DEFAULT, CF_NATIVE );
			continue;
		}
		// lightAmbientSpecular <imageprogram>
		// specifies the image indexed by surface normal in world space
		// contains ambient-specular light from environment (stgatilov #6090)
		else if ( !token.Icmp( "lightAmbientSpecular" ) ) {
			str = R_ParsePastImageProgramCubeMap( src );
			idStr copy = str;	// so other things don't step on it
			lightAmbientSpecular = globalImages->ImageFromFile( copy, TF_DEFAULT, false, TR_CLAMP /* TR_CLAMP_TO_ZERO */, TD_DEFAULT, CF_NATIVE );
			continue;
		}
		// guisurf <guifile> | guisurf entity
		// an entity guisurf must have an idUserInterface
		// specified in the renderEntity
		else if ( !token.Icmp( "guisurf" ) ) {
			src.ReadTokenOnLine( &token );
			if ( !token.Icmp( "entity" ) ) {
				entityGui = 1;
			} else if ( !token.Icmp( "entity2" ) ) {
				entityGui = 2;
			} else if ( !token.Icmp( "entity3" ) ) {
				entityGui = 3;
			} else {
				gui = uiManager->FindGui( token.c_str(), true );
			}
			continue;
		}
		// sort
		else if ( !token.Icmp( "sort" ) ) {
			ParseSort( src );
			continue;
		}
		// spectrum <integer>
		else if ( !token.Icmp( "spectrum" ) ) {
			src.ReadTokenOnLine( &token );
			spectrum = atoi( token.c_str() );
			continue;
		}
		// deform < sprite | tube | flare >
		else if ( !token.Icmp( "deform" ) ) {
			ParseDeform( src );
			continue;
		}
		// decalInfo <staySeconds> <fadeSeconds> ( <start rgb> ) ( <end rgb> )
		else if ( !token.Icmp( "decalInfo" ) ) {
			ParseDecalInfo( src );
			continue;
		}
		// renderbump <args...>
		else if ( !token.Icmp( "renderbump") ) {
			src.ParseRestOfLine( renderBump );
			continue;
		}
		// diffusemap for stage shortcut
		else if ( !token.Icmp( "diffusemap" ) ) {
			str = R_ParsePastImageProgram( src );
			idStr::snPrintf( buffer, sizeof( buffer ), "blend diffusemap\nmap %s\n}\n", str );
            newSrc.LoadMemory(buffer, static_cast<int>(strlen(buffer)), "diffusemap");
			newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
			ParseStage( newSrc, trpDefault );
			newSrc.FreeSource();
			continue;
		}
		// specularmap for stage shortcut
		else if ( !token.Icmp( "specularmap" ) ) {
			str = R_ParsePastImageProgram( src );
			idStr::snPrintf( buffer, sizeof( buffer ), "blend specularmap\nmap %s\n}\n", str );
            newSrc.LoadMemory(buffer, static_cast<int>(strlen(buffer)), "specularmap");
			newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
			ParseStage( newSrc, trpDefault );
			newSrc.FreeSource();
			continue;
		}
		// normalmap for stage shortcut
		else if ( !token.Icmp( "bumpmap" ) ) {
			str = R_ParsePastImageProgram( src );
			idStr::snPrintf( buffer, sizeof( buffer ), "blend bumpmap\nmap %s\n}\n", str );
            newSrc.LoadMemory(buffer, static_cast<int>(strlen(buffer)), "bumpmap");
			newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
			ParseStage( newSrc, trpDefault );
			newSrc.FreeSource();
			continue;
		}
		// stgatilov #5427: handle macros for typical frobstages generation
		else if ( !token.Icmp( "frobstage_texture" ) || !token.Icmp( "frobstage_diffuse" ) ) {

			idStr str;
			if ( !token.Icmp( "frobstage_texture" ) ) {
				// frobstage_texture: texture must be specified as argument
				str = R_ParsePastImageProgram( src );
			} else {
				// frobstage_diffuse: take first diffuse map (must be added beforehand)
				for ( int i = 0; i < numStages; i++ )
					if ( pd->parseStages[i].lighting == SL_DIFFUSE )
						if ( idImage *image = pd->parseStages[i].texture.image ) {
							str = image->imgName.c_str();
							break;
						}
				if ( str.IsEmpty() )
					common->Warning( "material '%s' frobstage_diffuse: cannot find diffuse texture", GetName() );
			}

			idVec3 mult = ParseNumberOrVec3( src );
			idVec3 add = ParseNumberOrVec3( src );

			AddFrobStages( add, str.c_str(), mult, trpDefault );
			continue;
		}
		else if ( !token.Icmp( "frobstage_none" ) ) {
			// add frobstage which has no effect
			// the goal is to block implicit frobstages from being added
			AddFrobStages( idVec3(0.0f), nullptr, idVec3(0.0f), trpDefault );
			continue;
		}
		// DECAL_MACRO for backwards compatibility with the preprocessor macros
		else if ( !token.Icmp( "DECAL_MACRO" ) ) {
			SetMaterialFlag( MF_POLYGONOFFSET );				// polygonOffset
			polygonOffset = 1.0f;

			sort = SS_DECAL;									// sort decal
			surfaceFlags |= SURF_DISCRETE;						// discrete
			contentFlags &= ~CONTENTS_SOLID;					// nonsolid

			SetMaterialFlag( MF_NOSHADOWS );					// noShadows
			continue;
		}
		// TWOSIDED_DECAL_MACRO to shorten some definitions
		else if ( !token.Icmp( "TWOSIDED_DECAL_MACRO" ) ) {
			SetMaterialFlag( MF_POLYGONOFFSET );				// polygonOffset
			polygonOffset = 1.0f;

			sort = SS_DECAL;									// sort decal
			surfaceFlags |= ( SURF_DISCRETE | SURF_NOIMPACT );	// discrete and noimpact
			contentFlags &= ~CONTENTS_SOLID;					// nonsolid
			cullType = CT_TWO_SIDED;							// twosided

			// twoSided implies no-shadows, because the shadow
			// volume would be coplanar with the surface, giving depth fighting
			// we could make this no-self-shadows, but it may be more important
			// to receive shadows from no-self-shadow monsters
			SetMaterialFlag( MF_NOSHADOWS | MF_NOSELFSHADOW );	// noShadows and noSelfShadows

			// translucent
			coverage = MC_TRANSLUCENT;
			continue;
		}
		// PARTICLE_MACRO to shorten some definitions
		else if ( !token.Icmp( "PARTICLE_MACRO" ) ) {
			surfaceFlags |= ( SURF_DISCRETE | SURF_NOIMPACT );	// discrete and noimpact
			contentFlags &= ~CONTENTS_SOLID;					// nonsolid
			SetMaterialFlag( MF_NOSHADOWS | MF_NOSELFSHADOW );	// noShadows and noSelfShadows
			
			coverage = MC_TRANSLUCENT;							// translucent
			continue;
		}
		// GLASS_MACRO to shorten some definitions
		else if ( !token.Icmp( "GLASS_MACRO" ) ) {
			cullType = CT_TWO_SIDED;
			// twoSided implies no-shadows, because the shadow
			// volume would be coplanar with the surface, giving depth fighting
			// we could make this no-self-shadows, but it may be more important
			// to receive shadows from no-self-shadow monsters
			SetMaterialFlag( MF_NOSHADOWS | MF_NOSELFSHADOW );	// noShadows and noSelfShadows

			coverage = MC_TRANSLUCENT;							// translucent
			continue;
		}
		else if ( token == "{" ) {
			// create the new stage
			ParseStage( src, trpDefault );
			continue;
		}
		else {
			common->Warning( "Unknown general material parameter '%s' in '%s'", token.c_str(), GetName() );
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}
	}

	// add _flat or _white stages if needed
	AddImplicitStages();

	// order the diffuse / bump / specular stages properly
	SortInteractionStages();

	// stgatilov #6340: warn if output color might depend on input alpha
	CheckAlphaColorDependencies();

	// if we need to do anything with normals (lighting or environment mapping)
	// and two sided lighting was asked for, flag
	// shouldCreateBackSides() and change culling back to single sided,
	// so we get proper tangent vectors on both sides

	// we can't just call ReceivesLighting(), because the stages are still
	// in temporary form
	if ( cullType == CT_TWO_SIDED ) {
		for ( int l = 0 ; l < numStages ; l++ ) {
			if ( pd->parseStages[l].lighting != SL_AMBIENT || pd->parseStages[l].texture.texgen != TG_EXPLICIT ) {
				if ( cullType == CT_TWO_SIDED ) {
					cullType = CT_FRONT_SIDED;
					shouldCreateBackSides = true;
				}
				break;
			}
		}
	}

	// currently a surface can only have one unique texgen for all the stages on old hardware
	texgen_t firstGen = TG_EXPLICIT;
	for ( int k = 0; k < numStages; k++ ) {
		if ( pd->parseStages[k].texture.texgen != TG_EXPLICIT ) {
			if ( firstGen == TG_EXPLICIT ) {
				firstGen = pd->parseStages[k].texture.texgen;
			} else if ( firstGen != pd->parseStages[k].texture.texgen ) {
				common->Warning( "material '%s' has multiple stages with a texgen", GetName() );
				break;
			}
		}
	}
}

/*
=========================
idMaterial::SetGui
=========================
*/
void idMaterial::SetGui( const char *_gui ) const {
	gui = uiManager->FindGui( _gui, true, false, true );
}

/*
=========================
idMaterial::Parse

Parses the current material definition and finds all necessary images.
=========================
*/
bool idMaterial::Parse( const char *text, const int textLength ) {
	idLexer	src;
	idToken	token;
	mtrParsingData_t parsingData;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	// reset to the unparsed state
	CommonInit();

	memset( &parsingData, 0, sizeof( parsingData ) );

	pd = &parsingData;	// this is only valid during parse

	// parse it
	ParseMaterial( src );

	//
	// count non-lit stages
	numAmbientStages = 0;
	int i;
	for ( i = 0 ; i < numStages ; i++ ) {
		if ( pd->parseStages[i].lighting == SL_AMBIENT ) {
			numAmbientStages++;
		}
	}

	// see if there is a subview stage
	if ( sort == SS_SUBVIEW ) {
		hasSubview = true;
	} else {
		hasSubview = false;
		for ( i = 0 ; i < numStages ; i++ ) {
			if ( pd->parseStages[i].texture.dynamic ) {
				hasSubview = true;
			}
		}
	}

	// automatically determine coverage if not explicitly set
	if ( coverage == MC_BAD ) {
		// automatically set MC_TRANSLUCENT if we don't have any interaction stages and 
		// the first stage is blended and not an alpha test mask or a subview
		if ( !numStages ) {
			// non-visible
			coverage = MC_TRANSLUCENT;
		} else if ( numStages != numAmbientStages ) {
			// we have an interaction draw
			coverage = MC_OPAQUE;
		} else if ( 
			( pd->parseStages[0].drawStateBits & GLS_DSTBLEND_BITS ) != GLS_DSTBLEND_ZERO ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_COLOR ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_ALPHA ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
			) {
			// blended with the destination
			coverage = MC_TRANSLUCENT;
		} else {
			coverage = MC_OPAQUE;
		}
	}

	// translucent automatically implies noshadows
	if ( coverage == MC_TRANSLUCENT ) {
		SetMaterialFlag( MF_NOSHADOWS );
	} else {
		// mark the contents as opaque
		contentFlags |= CONTENTS_OPAQUE;
	}

	// if we are translucent, draw with an alpha in the editor
	if ( coverage == MC_TRANSLUCENT ) {
		editorAlpha = 0.5f;
	} else {
		editorAlpha = 1.0f;
	}

	// the sorts can make reasonable defaults
	if ( sort == SS_BAD ) {
		if ( TestMaterialFlag(MF_POLYGONOFFSET) ) {
			sort = SS_DECAL;
		} else if ( coverage == MC_TRANSLUCENT ) {
			sort = SS_MEDIUM;
		} else {
			sort = SS_OPAQUE;
		}
	}

	// anything that references _currentRender will automatically get sort = SS_POST_PROCESS
	// and coverage = MC_TRANSLUCENT

	for ( i = 0 ; i < numStages ; i++ ) {
		shaderStage_t	*pStage = &pd->parseStages[i];
		if ( pStage->texture.image == globalImages->currentRenderImage ) {
			if ( sort != SS_PORTAL_SKY ) {
				sort = SS_POST_PROCESS;
				coverage = MC_TRANSLUCENT;
			}
			break;
		}
		if ( pStage->newStage ) {
			for ( int j = 0 ; j < pStage->newStage->numFragmentProgramImages ; j++ ) {
				if ( pStage->newStage->fragmentProgramImages[j] == globalImages->currentRenderImage ) {
					if ( sort != SS_PORTAL_SKY ) {
						sort = SS_POST_PROCESS;
						coverage = MC_TRANSLUCENT;
					}
					i = numStages;
					break;
				}
			}
		}
	}

	// set the drawStateBits depth flags
	for ( i = 0 ; i < numStages ; i++ ) {
		shaderStage_t	*pStage = &pd->parseStages[i];
		if ( sort == SS_POST_PROCESS ) {
			// post-process effects fill the depth buffer as they draw, so only the
			// topmost post-process effect is rendered
			pStage->drawStateBits |= GLS_DEPTHFUNC_LESS;
		} else if ( coverage == MC_TRANSLUCENT || pStage->ignoreAlphaTest ) {
			// translucent surfaces can extend past the exactly marked depth buffer
			pStage->drawStateBits |= GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK;
		} else {
			// opaque and perforated surfaces must exactly match the depth buffer,
			// which gets alpha test correct
			pStage->drawStateBits |= GLS_DEPTHFUNC_EQUAL | GLS_DEPTHMASK;
		}
	}

	// determine if this surface will accept overlays / decals

	if ( pd->forceOverlays ) {
		// explicitly flaged in material definition
		allowOverlays = true;
	} else {
		if ( !IsDrawn() ) {
			allowOverlays = false;
		}
		if ( Coverage() != MC_OPAQUE ) {
			allowOverlays = false;
		}
		if ( GetSurfaceFlags() & SURF_NOIMPACT ) {
			allowOverlays = false;
		}
	}

	// add a tiny offset to the sort orders, so that different materials
	// that have the same sort value will at least sort consistantly, instead
	// of flickering back and forth
/* this messed up in-game guis
	if ( sort != SS_SUBVIEW ) {
		int	hash, l;

		l = name.Length();
		hash = 0;
		for ( int i = 0 ; i < l ; i++ ) {
			hash ^= name[i];
		}
		sort += hash * 0.01;
	}
*/

	if (numStages) {
		stages = (shaderStage_t *)R_StaticAlloc( numStages * sizeof( stages[0] ) );
		memcpy( stages, pd->parseStages, numStages * sizeof( stages[0] ) );
	}

	if ( numOps ) {
		ops = (expOp_t *)R_StaticAlloc( numOps * sizeof( ops[0] ) );
		memcpy( ops, pd->shaderOps, numOps * sizeof( ops[0] ) );
	}

	if ( numRegisters ) {
		expressionRegisters = (float *)R_StaticAlloc( numRegisters * sizeof( expressionRegisters[0] ) );
		memcpy( expressionRegisters, pd->shaderRegisters, numRegisters * sizeof( expressionRegisters[0] ) );
	}

	// see if the registers are completely constant, and don't need to be evaluated
	// per-surface
	CheckForConstantRegisters();

	pd = NULL;	// the pointer will be invalid after exiting this function

	// finish things up
	if ( TestMaterialFlag( MF_DEFAULTED ) ) {
		MakeDefault();
		return false;
	}
	return true;
}

/*
===================
idMaterial::Print
===================
*/
const char *opNames[] = {
	"OP_TYPE_INVALID",
	"OP_TYPE_ADD",
	"OP_TYPE_SUBTRACT",
	"OP_TYPE_MULTIPLY",
	"OP_TYPE_DIVIDE",
	"OP_TYPE_MOD",
	"OP_TYPE_TABLE",
	"OP_TYPE_GT",
	"OP_TYPE_GE",
	"OP_TYPE_LT",
	"OP_TYPE_LE",
	"OP_TYPE_EQ",
	"OP_TYPE_NE",
	"OP_TYPE_AND",
	"OP_TYPE_OR",
	"OP_TYPE_MIN",
	"OP_TYPE_MAX",
	"OP_TYPE_SOUND"
};

void idMaterial::Print() const {
	int i;

	for ( i = EXP_REG_NUM_PREDEFINED ; i < GetNumRegisters() ; i++ ) {
		common->Printf( "register %i: %f\n", i, expressionRegisters[i] );
	}

	for ( i = 0 ; i < numOps ; i++ ) {
		const expOp_t *op = &ops[i];
		if ( op->opType == OP_TYPE_TABLE ) {
			common->Printf( "%i = %s[ %i ]\n", op->c, declManager->DeclByIndex( DECL_TABLE, op->a )->GetName(), op->b );
		} else {
			common->Printf( "%i = %i %s %i\n", op->c, op->a, opNames[ op->opType ], op->b );
		}
	}
}

/*
===============
idMaterial::Save
===============
*/
bool idMaterial::Save( const char *fileName ) {
	return ReplaceSourceFileText();
}

/*
===============
idMaterial::AddReference
===============
*/
void idMaterial::AddReference() {
	refCount++;

	for ( int i = 0; i < numStages; i++ ) {
		shaderStage_t *s = &stages[i];

		if ( s->texture.image ) {
			s->texture.image->AddReference();
		}
	}
}

/*
===============
idMaterial::EvaluateRegisters

Parameters are taken from the localSpace and the renderView,
then all expressions are evaluated, leaving the material registers
set to their apropriate values.
===============
*/
void idMaterial::EvaluateRegisters( float *registers, const float shaderParms[MAX_ENTITY_SHADER_PARMS],
									const viewDef_t *view, idSoundEmitter *soundEmitter ) const {
	int		i, b;

	expOp_t	*op = ops;

	if ( !op && numOps ) {
		common->FatalError( "R_EvaluateExpression: NULL operators pointer" );
		return;
	}

	// copy the material constants
	for ( i = EXP_REG_NUM_PREDEFINED ; i < numRegisters ; i++ ) {
		registers[i] = expressionRegisters[i];
	}

	// copy the local and global parameters
	registers[EXP_REG_TIME] = view->floatTime;
	registers[EXP_REG_PARM0] = shaderParms[0];
	registers[EXP_REG_PARM1] = shaderParms[1];
	registers[EXP_REG_PARM2] = shaderParms[2];
	registers[EXP_REG_PARM3] = shaderParms[3];
	registers[EXP_REG_PARM4] = shaderParms[4];
	registers[EXP_REG_PARM5] = shaderParms[5];
	registers[EXP_REG_PARM6] = shaderParms[6];
	registers[EXP_REG_PARM7] = shaderParms[7];
	registers[EXP_REG_PARM8] = shaderParms[8];
	registers[EXP_REG_PARM9] = shaderParms[9];
	registers[EXP_REG_PARM10] = shaderParms[10];
	registers[EXP_REG_PARM11] = shaderParms[11];
	registers[EXP_REG_GLOBAL0] = view->renderView.shaderParms[0];
	registers[EXP_REG_GLOBAL1] = view->renderView.shaderParms[1];
	registers[EXP_REG_GLOBAL2] = view->renderView.shaderParms[2];
	registers[EXP_REG_GLOBAL3] = view->renderView.shaderParms[3];
	registers[EXP_REG_GLOBAL4] = view->renderView.shaderParms[4];
	registers[EXP_REG_GLOBAL5] = view->renderView.shaderParms[5];
	registers[EXP_REG_GLOBAL6] = view->renderView.shaderParms[6];
	registers[EXP_REG_GLOBAL7] = view->renderView.shaderParms[7];

	for ( i = 0 ; i < numOps ; i++, op++ ) {
		switch( op->opType ) {
		case OP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case OP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case OP_TYPE_DIVIDE:
			registers[op->c] = ( registers[op->b] != 0.0f ) ? registers[op->a] / registers[op->b] : 0.0f;
			break;
		case OP_TYPE_MOD:
			b = (int)registers[op->b];
			b = ( b != 0 ) ? b : 1;
			registers[op->c] = (int)registers[op->a] % b;
			break;
		case OP_TYPE_TABLE:
			{
				const idDeclTable *table = static_cast<const idDeclTable *>( declManager->DeclByIndex( DECL_TABLE, op->a ) );
				registers[op->c] = table->TableLookup( registers[op->b] );
			}
			break;
		case OP_TYPE_GT:
			registers[op->c] = registers[ op->a ] > registers[op->b];
			break;
		case OP_TYPE_GE:
			registers[op->c] = registers[ op->a ] >= registers[op->b];
			break;
		case OP_TYPE_LT:
			registers[op->c] = registers[ op->a ] < registers[op->b];
			break;
		case OP_TYPE_LE:
			registers[op->c] = registers[ op->a ] <= registers[op->b];
			break;
		case OP_TYPE_EQ:
			registers[op->c] = registers[ op->a ] == registers[op->b];
			break;
		case OP_TYPE_NE:
			registers[op->c] = registers[ op->a ] != registers[op->b];
			break;
		case OP_TYPE_AND:
			registers[op->c] = registers[ op->a ] && registers[op->b];
			break;
		case OP_TYPE_OR:
			registers[op->c] = registers[ op->a ] || registers[op->b];
			break;
		case OP_TYPE_MIN:
			registers[op->c] = idMath::Fmin( registers[ op->a ], registers[op->b] );
			break;
		case OP_TYPE_MAX:
			registers[op->c] = idMath::Fmax( registers[ op->a ], registers[op->b] );
			break;
		case OP_TYPE_SOUND:
			if ( soundEmitter && soundEmitter->CurrentlyPlaying() ) {
				registers[op->c] = soundEmitter->CurrentAmplitude();
			} else {
				registers[op->c] = 0.0f;
			}
			break;
		default:
			common->FatalError( "R_EvaluateExpression: bad opcode" );
		}
	}
}

/*
=============
idMaterial::Texgen
=============
*/
texgen_t idMaterial::Texgen() const {
	if ( stages ) {
		for ( int i = 0; i < numStages; i++ ) {
			if ( stages[ i ].texture.texgen != TG_EXPLICIT ) {
				return stages[ i ].texture.texgen;
			}
		}
	}
	
	return TG_EXPLICIT;
}

/*
=============
idMaterial::GetImageWidth
=============
*/
int idMaterial::GetImageWidth( void ) const {
	assert( GetStage(0) && GetStage(0)->texture.image );
	return GetStage(0)->texture.image->uploadWidth;
}

/*
=============
idMaterial::GetImageHeight
=============
*/
int idMaterial::GetImageHeight( void ) const {
	assert( GetStage(0) && GetStage(0)->texture.image );
	return GetStage(0)->texture.image->uploadHeight;
}

/*
=============
idMaterial::CinematicLength
=============
*/
int	idMaterial::CinematicLength() const {
	if ( !stages || !stages[0].texture.cinematic ) {
		return 0;
	}
	return stages[0].texture.cinematic->AnimationLength();
}

/*
=============
idMaterial::UpdateCinematic
=============
*/
void idMaterial::UpdateCinematic( int time ) const {
	if ( !stages || !stages[0].texture.cinematic || !backEnd.viewDef ) {
		return;
	}
	stages[0].texture.cinematic->ImageForTime( tr.primaryRenderView.time );
}

/*
=============
idMaterial::CloseCinematic
=============
*/
void idMaterial::CloseCinematic( void ) const {
	for( int i = 0; i < numStages; i++ ) {
		if ( stages[i].texture.cinematic ) {
			stages[i].texture.cinematic->Close();
			delete stages[i].texture.cinematic;
			stages[i].texture.cinematic = NULL;
		}
	}
}

/*
=============
idMaterial::ResetCinematicTime
=============
*/
void idMaterial::ResetCinematicTime( int time ) const {
	for( int i = 0; i < numStages; i++ ) {
		if ( stages[i].texture.cinematic ) {
			stages[i].texture.cinematic->ResetTime( time );
		}
	}
}

idCinematic *idMaterial::GetCinematic() const {
	if ( !stages || !stages[0].texture.cinematic ) {
		return NULL;
	}
	return stages[0].texture.cinematic;
}

/*
=============
idMaterial::ConstantRegisters
=============
*/
const float *idMaterial::ConstantRegisters() const {
	if ( !r_useConstantMaterials.GetBool() ) {
		return NULL;
	}
	return constantRegisters;
}

/*
==================
idMaterial::CheckForConstantRegisters

As of 5/2/03, about half of the unique materials loaded on typical
maps are constant, but 2/3 of the surface references are.
This is probably an optimization of dubious value.
==================
*/
void idMaterial::CheckForConstantRegisters() {
	if ( !pd->registersAreConstant ) {
		return;
	}

	// evaluate the registers once, and save them 
	constantRegisters = (float *)R_ClearedStaticAlloc( GetNumRegisters() * sizeof( float ) );

	float shaderParms[MAX_ENTITY_SHADER_PARMS];
	memset( shaderParms, 0, sizeof( shaderParms ) );
	viewDef_t	viewDef;
	memset( &viewDef, 0, sizeof( viewDef ) );

	EvaluateRegisters( constantRegisters, shaderParms, &viewDef, 0 );
}

/*
===================
idMaterial::ImageName
===================
*/
const char *idMaterial::ImageName( void ) const {
	if ( numStages == 0 ) {
		return "_scratch";
	}
	idImage	*image = stages[0].texture.image;
	if ( image ) {
		return image->imgName;
	}
	return "_scratch";
}

/*
===================
idMaterial::SetImageClassifications

Just for image resource tracking.
===================
*/
void idMaterial::SetImageClassifications( int tag ) const {
	for ( int i = 0 ; i < numStages ; i++ ) {
		idImage	*image = stages[i].texture.image;
		if ( image ) {
			if ( idImageAsset *asset = image->AsAsset() ) {
				asset->SetClassification( tag );
			}
		}
	}
}

/*
=================
idMaterial::Size
=================
*/
size_t idMaterial::Size( void ) const {
	return sizeof( idMaterial );
}

/*
===================
idMaterial::SetDefaultText
===================
*/
bool idMaterial::SetDefaultText( void ) {
	// if there exists an image with the same name
	//if ( 1 ) { //fileSystem->ReadFile( GetName(), NULL ) != -1 ) {
	char generated[2048];
	idStr::snPrintf( generated, sizeof( generated ), 
					"material %s // IMPLICITLY GENERATED\n"
					"{\n"
					"\t"	"{\n"
					"\t"	"blend blend\n"
					"\t"	"colored\n"
					"\t"	"map \"%s\"\n"
					"\t"	"clamp\n"
					"\t"	"}\n"
					"}\n", GetName(), GetName() );
	SetText( generated );

	return true;
}

/*
===================
idMaterial::DefaultDefinition
===================
*/
const char *idMaterial::DefaultDefinition() const {
	return
	"{\n"
	"\t"	"{\n"
	"\t\t"	"blend\tblend\n"
	"\t\t"	"map\t\t_default\n"
	"\t"	"}\n"
	"}";
}


/*
===================
idMaterial::GetBumpStage
===================
*/
const shaderStage_t *idMaterial::GetBumpStage( void ) const {
	for ( int i = 0 ; i < numStages ; i++ ) {
		if ( stages[i].lighting == SL_BUMP ) {
			return &stages[i];
		}
	}
	return NULL;
}

/*
===================
idMaterial::ReloadImages
===================
*/
void idMaterial::ReloadImages( bool force ) const {
	for ( int i = 0 ; i < numStages ; i++ ) {
		if ( stages[i].newStage ) {
			for ( int j = 0 ; j < stages[i].newStage->numFragmentProgramImages ; j++ ) {
				if ( idImage *img = stages[i].newStage->fragmentProgramImages[j] ) {
					if ( idImageAsset *asset = img->AsAsset() ) {
						asset->Reload( false, force );
					}
				}
			}
		} else if ( idImage *img = stages[i].texture.image ) {
			if ( idImageAsset *asset = img->AsAsset() ) {
				asset->Reload( false, force );
			}
		}
	}
}

/*
===================
idMaterial::HasMirrorLikeStage
===================
*/
bool idMaterial::HasMirrorLikeStage() const {
	if (!HasSubview())
		return false;
	for (int i = 0; i < numStages; i++) {
		dynamicidImage_t dyn = stages[i].texture.dynamic;
		if (dyn == DI_MIRROR_RENDER || dyn == DI_XRAY_RENDER || dyn == DI_REMOTE_RENDER)
			return true;
	}
	return false;
}

/*
===================
idMaterial::IsFrobStage
===================
*/
bool idMaterial::IsFrobStage(int stageIdx, bool *isStandard) const {
	const shaderStage_t *stage = &pd->parseStages[stageIdx];

	// expression operations are executed sequentally
	// so we need to trace operations backwards to see which of them write to "active" register we are interested in
	// this way we can walk through the whole dependency subgraph of an "if" statement
	const expOp_t *lastOp = nullptr;
	idList<int> activeRegisters;
	activeRegisters.Append(	stage->conditionRegister );

	for ( int i = numOps - 1; i >= 0; i-- ) {
		const expOp_t &op = pd->shaderOps[i];
		if ( int *ptr = activeRegisters.Find( op.c ) ) {
			activeRegisters.Remove( *ptr );
			activeRegisters.AddUnique( op.a );
			activeRegisters.AddUnique( op.b );
			if ( !lastOp )
				lastOp = &op;
		}
	}

	// we only have leaf registers of dependency subgraph, check if parm11 is among them
	if ( !activeRegisters.Find( EXP_REG_PARM11 ) )
		return false;

	if ( isStandard ) {
		// standard format of frobstage condition is:
		//    if (parm11 > 0)
		// we should encourage mappers to write this exact condition

		if ( lastOp &&
			lastOp->a == EXP_REG_PARM11 && lastOp->opType == OP_TYPE_GT &&
			!pd->registerIsTemporary[lastOp->b] && pd->shaderRegisters[lastOp->b] == 0.0f
		) {
			*isStandard = true;
		} else {
			*isStandard = false;
		}
	}

	return true;
}

/*
===================
idMaterial::AddFrobStages
===================
*/
void idMaterial::AddFrobStages(const idVec3 &rgbAdd, const char *imageName, const idVec3 &rgbMult, const textureRepeat_t trpDefault) {
	idLexer newSrc;
	char buffer[1024];

	idStr::snPrintf( buffer, sizeof( buffer ),
		"if ( parm11 > 0 )\n"
		"blend  gl_dst_color, gl_one\n"
		"map    _white\n"
		"red    %f * parm11\n"
		"green  %f * parm11\n"
		"blue   %f * parm11\n"
		"}\n",	// finishes stage
		rgbAdd.x, rgbAdd.y, rgbAdd.z
	);
	newSrc.LoadMemory(buffer, static_cast<int>(strlen(buffer)), "frobstage_const");
	newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	ParseStage( newSrc, trpDefault );
	newSrc.FreeSource();

	if (imageName) {
		idStr::snPrintf( buffer, sizeof( buffer ),
			"if ( parm11 > 0 )\n"
			"blend  add\n"
			"map    %s\n"
			"red    %f * parm11\n"
			"green  %f * parm11\n"
			"blue   %f * parm11\n"
			"}\n",	// finishes stage
			imageName,
			rgbMult.x, rgbMult.y, rgbMult.z
		);
		newSrc.LoadMemory(buffer, static_cast<int>(strlen(buffer)), "frobstage_mult");
		newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
		ParseStage( newSrc, trpDefault );
		newSrc.FreeSource();
	}
}
