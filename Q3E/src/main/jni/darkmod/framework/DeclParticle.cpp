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

#include "renderer/resources/Image.h"



struct ParticleParmDesc {
	const char *name;
	int count;
	const char *desc;
};

const ParticleParmDesc ParticleDistributionDesc[] = {
	{ "rect", 3, "" },
	{ "cylinder", 4, "" },
	{ "sphere", 3, "" }
};

const ParticleParmDesc ParticleDirectionDesc[] = {
	{ "cone", 1, "" },
	{ "outward", 1, "" },
};

const ParticleParmDesc ParticleOrientationDesc[] = {
	{ "view", 0, "" },
	{ "aimed", 2, "" },
	{ "x", 0, "" },
	{ "y", 0, "" },
	{ "z", 0, "" } 
};

const ParticleParmDesc ParticleCustomDesc[] = {
	{ "standard", 0, "Standard" },
	{ "helix", 5, "sizeX Y Z radialSpeed axialSpeed" },
	{ "flies", 3, "radialSpeed axialSpeed size" },
	{ "orbit", 2, "radius speed"},
	{ "drip", 2, "something something" }
};

const int CustomParticleCount = sizeof( ParticleCustomDesc ) / sizeof( const ParticleParmDesc );

/*
=================
idDeclParticle::Size
=================
*/
size_t idDeclParticle::Size( void ) const {
	return sizeof( idDeclParticle );
}

/*
================
idDeclParticle::ParseParms

Parses a variable length list of parms on one line
================
*/
void idDeclParticle::ParseParms( idLexer &src, float *parms, int maxParms ) {
	idToken token;

	memset( parms, 0, maxParms * sizeof( *parms ) );
	int	count = 0;
	while( 1 ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return;
		}
		if ( count == maxParms ) {
			src.Error( "too many parms on line" );
			return;
		}
		token.StripQuotes();
		parms[count] = atof( token );
		count++;
	}
}

/*
================
idDeclParticle::ParseParametric
================
*/
void idDeclParticle::ParseParametric( idLexer &src, idParticleParm *parm ) {
	idToken token;

	parm->table = NULL;
	parm->from = parm->to = 0.0f;

	if ( !src.ReadToken( &token ) ) {
		src.Error( "not enough parameters" );
		return;
	}

	if ( token.IsNumeric() ) {
		// can have a to + 2nd parm
		parm->from = parm->to = atof( token );
		if ( src.ReadToken( &token ) ) {
			if ( !token.Icmp( "to" ) ) {
				if ( !src.ReadToken( &token ) ) {
					src.Error( "missing second parameter" );
					return;
				}
				parm->to = atof( token );
			} else {
				src.UnreadToken( &token );
			}
		}
	} else {
		// table
		parm->table = static_cast<const idDeclTable *>( declManager->FindType( DECL_TABLE, token, false ) );
	}

}

/*
================
idDeclParticle::ParseParticleStage
================
*/
idParticleStage *idDeclParticle::ParseParticleStage( idLexer &src ) {
	idToken token;

	idParticleStage *stage = new idParticleStage;
	stage->Default();

	while (1) {
		if ( src.HadError() ) {
			break;
		}
		if ( !src.ReadToken( &token ) ) {
			break;
		}
		if ( !token.Icmp( "}" ) ) {
			break;
		}
		if ( !token.Icmp( "material" ) ) {
			src.ReadToken( &token );
			stage->material = declManager->FindMaterial( token.c_str() );
			continue;
		}
		if ( !token.Icmp( "count" ) ) {
			stage->totalParticles = src.ParseInt();
			continue;
		}
		if ( !token.Icmp( "time" ) ) {
			stage->particleLife = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "cycles" ) ) {
			stage->cycles = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "diversityPeriod" ) ) {
			int num = src.ParseInt();
			if (num > 1000)
				src.Warning("diversityPeriod %d is ignored (too large)", num);
			else if (num < 0)
				src.Warning("diversityPeriod %d is ignored (negative)", num);
			else
				stage->diversityPeriod = num;
			continue;
		}
		if ( !token.Icmp( "timeOffset" ) ) {
			stage->timeOffset = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "deadTime" ) ) {
			stage->deadTime = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "randomDistribution" ) ) {
			stage->randomDistribution = src.ParseBool();
			continue;
		}
		if ( !token.Icmp( "bunching" ) ) {
			stage->spawnBunching = src.ParseFloat();
			continue;
		}

		if ( !token.Icmp( "distribution" ) ) {
			src.ReadToken( &token );
			if ( !token.Icmp( "rect" ) ) {
				stage->distributionType = PDIST_RECT;
			} else if ( !token.Icmp( "cylinder" ) ) {
				stage->distributionType = PDIST_CYLINDER;
			} else if ( !token.Icmp( "sphere" ) ) {
				stage->distributionType = PDIST_SPHERE;
			} else {
				src.Error( "bad distribution type: %s\n", token.c_str() );
			}
			ParseParms( src, stage->distributionParms, sizeof( stage->distributionParms ) / sizeof( stage->distributionParms[0] ) );
			continue;
		}

		if ( !token.Icmp( "direction" ) ) {
			src.ReadToken( &token );
			if ( !token.Icmp( "cone" ) ) {
				stage->directionType = PDIR_CONE;
			} else if ( !token.Icmp( "outward" ) ) {
				stage->directionType = PDIR_OUTWARD;
			} else {
				src.Error( "bad direction type: %s\n", token.c_str() );
			}
			ParseParms( src, stage->directionParms, sizeof( stage->directionParms ) / sizeof( stage->directionParms[0] ) );
			continue;
		}

		if ( !token.Icmp( "orientation" ) ) {
			src.ReadToken( &token );
			if ( !token.Icmp( "view" ) ) {
				stage->orientation = POR_VIEW;
			} else if ( !token.Icmp( "aimed" ) ) {
				stage->orientation = POR_AIMED;
			} else if ( !token.Icmp( "x" ) ) {
				stage->orientation = POR_X;
			} else if ( !token.Icmp( "y" ) ) {
				stage->orientation = POR_Y;
			} else if ( !token.Icmp( "z" ) ) {
				stage->orientation = POR_Z;
			} else {
				src.Error( "bad orientation type: %s\n", token.c_str() );
			}
			ParseParms( src, stage->orientationParms, sizeof( stage->orientationParms ) / sizeof( stage->orientationParms[0] ) );
			continue;
		}

		if ( !token.Icmp( "customPath" ) ) {
			src.ReadToken( &token );
			if ( !token.Icmp( "standard" ) ) {
				stage->customPathType = PPATH_STANDARD;
			} else if ( !token.Icmp( "helix" ) ) {
				stage->customPathType = PPATH_HELIX;
			} else if ( !token.Icmp( "flies" ) ) {
				stage->customPathType = PPATH_FLIES;
			} else if ( !token.Icmp( "spherical" ) ) {
				stage->customPathType = PPATH_ORBIT;
			} else {
				src.Error( "bad path type: %s\n", token.c_str() );
			}
			ParseParms( src, stage->customPathParms, sizeof( stage->customPathParms ) / sizeof( stage->customPathParms[0] ) );
			continue;
		}

		if ( !token.Icmp( "speed" ) ) {
			ParseParametric( src, &stage->speed );
			continue;
		}
		if ( !token.Icmp( "rotation" ) ) {
			ParseParametric( src, &stage->rotationSpeed );
			continue;
		}
		if ( !token.Icmp( "angle" ) ) {
			stage->initialAngle = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "entityColor" ) ) { 
			stage->entityColor = src.ParseBool();
			continue;
		}
		if ( !token.Icmp( "size" ) ) {
			ParseParametric( src, &stage->size );
			continue;
		}
		if ( !token.Icmp( "aspect" ) ) {
			ParseParametric( src, &stage->aspect );
			continue;
		}
		if ( !token.Icmp( "fadeIn" ) ) {
			stage->fadeInFraction = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "fadeOut" ) ) {
			stage->fadeOutFraction = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "fadeIndex" ) ) {
			stage->fadeIndexFraction = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "color" ) ) {
			stage->color[0] = src.ParseFloat();
			stage->color[1] = src.ParseFloat();
			stage->color[2] = src.ParseFloat();
			stage->color[3] = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "fadeColor" ) ) {
			stage->fadeColor[0] = src.ParseFloat();
			stage->fadeColor[1] = src.ParseFloat();
			stage->fadeColor[2] = src.ParseFloat();
			stage->fadeColor[3] = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp("offset" ) ) {
			stage->offset[0] = src.ParseFloat();
			stage->offset[1] = src.ParseFloat();
			stage->offset[2] = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "animationFrames" ) ) {
			stage->animationFrames = src.ParseInt();
			continue;
		}
		if ( !token.Icmp( "animationRate" ) ) {
			stage->animationRate = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "boundsExpansion" ) ) {
			stage->boundsExpansion = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "gravity" ) ) {
			src.ReadToken( &token );
			if ( !token.Icmp( "world" ) ) {
				stage->worldGravity = true;
			} else {
				src.UnreadToken( &token );
			}
			stage->gravity = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "WorldAxis" ) ) {		// #3950
			stage->worldAxis = true;
			continue;
		}
		if ( !token.Icmp( "softeningRadius" ) ) {
			stage->softeningRadius = src.ParseFloat();
			continue;
		}
		if ( !token.Icmp( "cutoffTimeMap" ) ) {
			src.ReadToken( &token );
			stage->cutoffTimeMap = idParticleStage::LoadCutoffTimeMap( token.c_str(), false );
			stage->useCutoffTimeMap = true;
			continue;
		}
		if ( !token.Icmp( "mapLayout" ) ) {
			src.ReadToken( &token );
			if ( !token.Icmp("linear") ) {
				stage->mapLayoutType = PML_LINEAR;
				stage->mapLayoutSizes[0] = -1;
				stage->mapLayoutSizes[1] = -1;
			}
			else if ( !token.Icmp("texture") ) {
				stage->mapLayoutType = PML_TEXTURE;
				stage->mapLayoutSizes[0] = src.ParseInt();
				stage->mapLayoutSizes[1] = src.ParseInt();
			}
			else {
				src.Error( "unknown mapLayout type %s", token.c_str() );
			}
			continue;
		}
		if ( !token.Icmp( "collisionStatic" ) ) {
			stage->collisionStatic = true;
			stage->useCutoffTimeMap = true;
			continue;
		}
		if ( !token.Icmp( "collisionStaticWorldOnly" ) ) {
			stage->collisionStaticWorldOnly = true;
			continue;
		}
		if ( !token.Icmp( "collisionStaticTimeSteps" ) ) {
			int x = src.ParseInt();
			if (x <= 0 || x > 1000000) {
				src.Error("collisionStaticTimeSteps is negative (or too high)");
			}
			stage->collisionStaticTimeSteps = x;
			continue;
		}

		src.Error( "unknown token %s\n", token.c_str() );
	}

	// derive values
	stage->cycleMsec = ( stage->particleLife + stage->deadTime ) * 1000;

	if ( stage->cutoffTimeMap ) {
		if ( stage->collisionStatic ) {
			src.Warning( "'collisionStatic' is ignored in favor of 'cutoffTimeMap'" );
			stage->collisionStatic = false;
		}
		if ( stage->mapLayoutType != PML_TEXTURE ) {
			src.Warning( "'cutoffTimeMap' ignored: 'mapLayout' must be 'texture'" );
			stage->useCutoffTimeMap = false;
		}
		//stgatilov: image does not get loaded immediately =(
		/*else if ( stage->cutoffTimeMap->cpuData.width != stage->mapLayoutSizes[0] || stage->cutoffTimeMap->cpuData.height != stage->mapLayoutSizes[1] ) {
			src.Warning( "'cutoffTimeMap' ignored: dimensions must match specified in 'mapLayout'" );
			stage->useCutoffTimeMap = false;
		}*/
	}

	return stage;
}

/*
================
idDeclParticle::Parse
================
*/
bool idDeclParticle::Parse( const char *text, const int textLength ) {
	idLexer src;
	idToken	token;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	depthHack = 0.0f;

	while (1) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "{" ) ) {
			idParticleStage *stage = ParseParticleStage( src );
			if ( !stage ) {
				src.Warning( "Particle stage parse failed" );
				MakeDefault();
				return false;
			}
			stages.Append( stage );
			continue;
		}

		if ( !token.Icmp( "depthHack" ) ) {
			depthHack = src.ParseFloat();
			continue;
		}

		src.Warning( "bad token %s", token.c_str() );
		MakeDefault();
		return false;
	}

	//
	// precompute the "std" bounds of every stage
	//
	for( int i = 0; i < stages.Num(); i++ ) {
		stages[i]->stdBounds = idParticle_EstimateBoundsStdSys(*stages[i]);
	}
	return true;
}

idBounds idDeclParticle::GetStageBounds( const struct renderEntity_s *ent, idParticleStage *stage ) {
	return idParticle_GetStageBoundsModel(*stage, stage->stdBounds, ent->axis);
}

idBounds idDeclParticle::GetFullBounds( const struct renderEntity_s *ent ) const {
	idBounds res;
	res.Clear();
	for (int i = 0; i < stages.Num(); i++) {
		idBounds bounds = GetStageBounds(ent, stages[i]);
		res.AddBounds(bounds);
	}
	return res;
}

/*
================
idDeclParticle::FreeData
================
*/
void idDeclParticle::FreeData( void ) {
	stages.DeleteContents( true );
}

/*
================
idDeclParticle::DefaultDefinition
================
*/
const char *idDeclParticle::DefaultDefinition( void ) const {
	return
		"{\n"
	"\t"	"{\n"
	"\t\t"		"material\t_default\n"
	"\t\t"		"count\t20\n"
	"\t\t"		"time\t\t1.0\n"
	"\t"	"}\n"
		"}";
}

/*
================
idDeclParticle::WriteParticleParm
================
*/
void idDeclParticle::WriteParticleParm( idFile *f, idParticleParm *parm, const char *name ) {

	f->WriteFloatString( "\t\t%s\t\t\t\t ", name );
	if ( parm->table ) {
		f->WriteFloatString( "%s\n", parm->table->GetName() );
	} else {
		f->WriteFloatString( "\"%.3f\" ", parm->from );
		if ( parm->from == parm->to ) {
			f->WriteFloatString( "\n" );
		} else {
			f->WriteFloatString( " to \"%.3f\"\n", parm->to );
		}
	}
}

/*
================
idDeclParticle::WriteStage
================
*/
void idDeclParticle::WriteStage( idFile *f, idParticleStage *stage ) {
	
	int i;

	f->WriteFloatString( "\t{\n" );
	f->WriteFloatString( "\t\tcount\t\t\t\t%i\n", stage->totalParticles );
	f->WriteFloatString( "\t\tmaterial\t\t\t%s\n", stage->material->GetName() );
	if ( stage->animationFrames ) {
		f->WriteFloatString( "\t\tanimationFrames \t%i\n", stage->animationFrames );
	}
	if ( stage->animationRate ) {
		f->WriteFloatString( "\t\tanimationRate \t\t%.3f\n", stage->animationRate );
	}
	f->WriteFloatString( "\t\ttime\t\t\t\t%.3f\n", stage->particleLife );
	f->WriteFloatString( "\t\tcycles\t\t\t\t%.3f\n", stage->cycles );
	if ( stage->timeOffset ) {
		f->WriteFloatString( "\t\ttimeOffset\t\t\t%.3f\n", stage->timeOffset );
	}
	if ( stage->deadTime ) {
		f->WriteFloatString( "\t\tdeadTime\t\t\t%.3f\n", stage->deadTime );
	}
	f->WriteFloatString( "\t\tbunching\t\t\t%.3f\n", stage->spawnBunching );
	
	f->WriteFloatString( "\t\tdistribution\t\t%s ", ParticleDistributionDesc[stage->distributionType].name );
	for ( i = 0; i < ParticleDistributionDesc[stage->distributionType].count; i++ ) {
		f->WriteFloatString( "%.3f ", stage->distributionParms[i] );
	}
	f->WriteFloatString( "\n" );

	f->WriteFloatString( "\t\tdirection\t\t\t%s ", ParticleDirectionDesc[stage->directionType].name );
	for ( i = 0; i < ParticleDirectionDesc[stage->directionType].count; i++ ) {
		f->WriteFloatString( "\"%.3f\" ", stage->directionParms[i] );
	}
	f->WriteFloatString( "\n" );

	f->WriteFloatString( "\t\torientation\t\t\t%s ", ParticleOrientationDesc[stage->orientation].name );
	for ( i = 0; i < ParticleOrientationDesc[stage->orientation].count; i++ ) {
		f->WriteFloatString( "%.3f ", stage->orientationParms[i] );
	}
	f->WriteFloatString( "\n" );

	if ( stage->customPathType != PPATH_STANDARD ) {
		f->WriteFloatString( "\t\tcustomPath %s ", ParticleCustomDesc[stage->customPathType].name );
		for ( i = 0; i < ParticleCustomDesc[stage->customPathType].count; i++ ) {
			f->WriteFloatString( "%.3f ", stage->customPathParms[i] );
		}
		f->WriteFloatString( "\n" );
	}

	if ( stage->entityColor ) {
		f->WriteFloatString( "\t\tentityColor\t\t\t1\n" );
	}

	WriteParticleParm( f, &stage->speed, "speed" );
	WriteParticleParm( f, &stage->size, "size" );
	WriteParticleParm( f, &stage->aspect, "aspect" );

	if ( stage->rotationSpeed.from ) {
		WriteParticleParm( f, &stage->rotationSpeed, "rotation" );
	}

	if ( stage->initialAngle ) {
		f->WriteFloatString( "\t\tangle\t\t\t\t%.3f\n", stage->initialAngle );
	}

	f->WriteFloatString( "\t\trandomDistribution\t\t\t\t%i\n", static_cast<int>( stage->randomDistribution ) );
	f->WriteFloatString( "\t\tboundsExpansion\t\t\t\t%.3f\n", stage->boundsExpansion );


	f->WriteFloatString( "\t\tfadeIn\t\t\t\t%.3f\n", stage->fadeInFraction );
	f->WriteFloatString( "\t\tfadeOut\t\t\t\t%.3f\n", stage->fadeOutFraction );
	f->WriteFloatString( "\t\tfadeIndex\t\t\t\t%.3f\n", stage->fadeIndexFraction );

	f->WriteFloatString( "\t\tcolor \t\t\t\t%.3f %.3f %.3f %.3f\n", stage->color.x, stage->color.y, stage->color.z, stage->color.w );
	f->WriteFloatString( "\t\tfadeColor \t\t\t%.3f %.3f %.3f %.3f\n", stage->fadeColor.x, stage->fadeColor.y, stage->fadeColor.z, stage->fadeColor.w );

	f->WriteFloatString( "\t\toffset \t\t\t\t%.3f %.3f %.3f\n", stage->offset.x, stage->offset.y, stage->offset.z );
	f->WriteFloatString( "\t\tgravity \t\t\t" );
	if ( stage->worldGravity ) {
		f->WriteFloatString( "world " );
	}
	f->WriteFloatString( "%.3f\n", stage->gravity );
	f->WriteFloatString( "\t}\n" );
}

/*
================
idDeclParticle::RebuildTextSource
================
*/
bool idDeclParticle::RebuildTextSource( void ) {
	idFile_Memory f;

	f.WriteFloatString("\n\n/*\n"
		"\tGenerated by the Particle Editor.\n"
		"\tTo use the particle editor, launch the game and type 'editParticles' on the console.\n"
		"*/\n" );

	f.WriteFloatString( "particle %s {\n", GetName() );

	if ( depthHack ) {
		f.WriteFloatString( "\tdepthHack\t%f\n", depthHack );
	}

	for ( int i = 0; i < stages.Num(); i++ ) {
		WriteStage( &f, stages[i] );
	}

	f.WriteFloatString( "}" );

	SetText( f.GetDataPtr() );

	return true;
}

/*
================
idDeclParticle::Save
================
*/
bool idDeclParticle::Save( const char *fileName ) {
	RebuildTextSource();
	if ( fileName ) {
		declManager->CreateNewDecl( DECL_PARTICLE, GetName(), fileName );
	}
	ReplaceSourceFileText();
	return true;
}

/*
====================================================================================

idParticleStage

====================================================================================
*/

/*
================
idParticleStage::idParticleStage
================
*/
idParticleStage::idParticleStage( void ) {
	material = NULL;
	totalParticles = 0;
	cycles = 0.0f;
	diversityPeriod = 0;
	cycleMsec = 0;
	spawnBunching = 0.0f;
	particleLife = 0.0f;
	timeOffset = 0.0f;
	deadTime = 0.0f;
	distributionType = PDIST_RECT;
	distributionParms[0] = distributionParms[1] = distributionParms[2] = distributionParms[3] = 0.0f;
	directionType = PDIR_CONE;
	directionParms[0] = directionParms[1] = directionParms[2] = directionParms[3] = 0.0f;
	// idParticleParm		speed;
	gravity = 0.0f;
	worldGravity = false;
	worldAxis = false;
	customPathType = PPATH_STANDARD;
	customPathParms[0] = customPathParms[1] = customPathParms[2] = customPathParms[3] = 0.0f;
	customPathParms[4] = customPathParms[5] = customPathParms[6] = customPathParms[7] = 0.0f;
	offset.Zero();
	animationFrames = 0;
	animationRate = 0.0f;
	randomDistribution = true;
	entityColor = false;
	initialAngle = 0.0f;
	// idParticleParm		rotationSpeed;
	orientation = POR_VIEW;
	orientationParms[0] = orientationParms[1] = orientationParms[2] = orientationParms[3] = 0.0f;
	// idParticleParm		size
	// idParticleParm		aspect
	color.Zero();
	fadeColor.Zero();
	fadeInFraction = 0.0f;
	fadeOutFraction = 0.0f;
	fadeIndexFraction = 0.0f;
	hidden = false;
	boundsExpansion = 0.0f;
	softeningRadius = -2.0f;	// -2 means "auto"
}

/*
================
idParticleStage::Default

Sets the stage to a default state
================
*/
void idParticleStage::Default() {
	material = declManager->FindMaterial( "_default" );
	totalParticles = 100;
	spawnBunching = 1.0f;
	particleLife = 1.5f;
	timeOffset = 0.0f;
	deadTime = 0.0f;
	distributionType = PDIST_RECT;
	distributionParms[0] = 8.0f;
	distributionParms[1] = 8.0f;
	distributionParms[2] = 8.0f;
	distributionParms[3] = 0.0f;
	directionType = PDIR_CONE;
	directionParms[0] = 90.0f;
	directionParms[1] = 0.0f;
	directionParms[2] = 0.0f;
	directionParms[3] = 0.0f;
	orientation = POR_VIEW;
	orientationParms[0] = 0.0f;
	orientationParms[1] = 0.0f;
	orientationParms[2] = 0.0f;
	orientationParms[3] = 0.0f;
	speed.from = 150.0f;
	speed.to = 150.0f;
	speed.table = NULL;
	gravity = 1.0f;
	worldGravity = false;
	worldAxis = false;
	customPathType = PPATH_STANDARD;
	customPathParms[0] = 0.0f;
	customPathParms[1] = 0.0f;
	customPathParms[2] = 0.0f;
	customPathParms[3] = 0.0f;
	customPathParms[4] = 0.0f;
	customPathParms[5] = 0.0f;
	customPathParms[6] = 0.0f;
	customPathParms[7] = 0.0f;
	offset.Zero();
	animationFrames = 0;
	animationRate = 0.0f;
	initialAngle = 0.0f;
	rotationSpeed.from = 0.0f;
	rotationSpeed.to = 0.0f;
	rotationSpeed.table = NULL;
	size.from = 4.0f;
	size.to = 4.0f;
	size.table = NULL;
	aspect.from = 1.0f;
	aspect.to = 1.0f;
	aspect.table = NULL;
	color.x = 1.0f;
	color.y = 1.0f;
	color.z = 1.0f;
	color.w = 1.0f;
	fadeColor.x = 0.0f;
	fadeColor.y = 0.0f;
	fadeColor.z = 0.0f;
	fadeColor.w = 0.0f;
	fadeInFraction = 0.1f;
	fadeOutFraction = 0.25f;
	fadeIndexFraction = 0.0f;
	boundsExpansion = 0.0f;
	randomDistribution = true;
	entityColor = false;
	cycleMsec = ( particleLife + deadTime ) * 1000;
	softeningRadius = -2.0f; // -2 means "auto"
	useCutoffTimeMap = false;
	cutoffTimeMap = nullptr;
	collisionStatic = false;
	collisionStaticWorldOnly = false;
	mapLayoutType = PML_LINEAR;
	mapLayoutSizes[0] = -1;
	mapLayoutSizes[1] = -1;
	collisionStaticTimeSteps = 0;
}

/*
================
idParticleStage::NumQuadsPerParticle

includes trails and cross faded animations
================
*/
int idParticleStage::NumQuadsPerParticle() const {
	int	count = 1;

	if ( orientation == POR_AIMED ) {
		int	trails = idMath::FtoiTrunc( orientationParms[0] );
		// each trail stage will add an extra quad
		count *= ( 1 + trails );
	}

	// if we are doing strip-animation, we need to double the number and cross fade them
	if ( animationFrames > 1 ) {
		count *= 2;
	}

	return count;
}

/*
==================
idParticleStage::GetCustomPathName
==================
*/
const char* idParticleStage::GetCustomPathName() {
	int index = ( customPathType < CustomParticleCount ) ? customPathType : 0;
	return ParticleCustomDesc[index].name;
}

/*
==================
idParticleStage::GetCustomPathDesc
==================
*/
const char* idParticleStage::GetCustomPathDesc() {
	int index = ( customPathType < CustomParticleCount ) ? customPathType : 0;
	return ParticleCustomDesc[index].desc;
}

/*
==================
idParticleStage::NumCustomPathParms
==================
*/
int idParticleStage::NumCustomPathParms() {
	int index = ( customPathType < CustomParticleCount ) ? customPathType : 0;
	return ParticleCustomDesc[index].count;
}

/*
==================
idParticleStage::SetCustomPathType
==================
*/
void idParticleStage::SetCustomPathType( const char *p ) {
	customPathType = PPATH_STANDARD;
	for ( int i = 0; i < CustomParticleCount; i ++ ) {
		if ( idStr::Icmp( p, ParticleCustomDesc[i].name ) == 0 ) {
			customPathType = static_cast<prtCustomPth_t>( i );
			break;
		}
	}
}

const char *idParticleStage::GetCollisionStaticDirectory() {
	return "textures/_prt_gen";
}
idStr idParticleStage::GetCollisionStaticImagePath(const idPartSysEmitterSignature &signature) {
	idStr imageName;
	sprintf(imageName, "%s/cstm__%s%s__%d_%d.tga",
		GetCollisionStaticDirectory(),
		signature.mainName.c_str(),
		signature.modelSuffix.c_str(),
		signature.surfaceIndex,
		signature.particleStageIndex
	);
	return imageName;
}
idImageAsset *idParticleStage::LoadCutoffTimeMap(const char *imagePath, bool defer) {
	// check if image is already loaded and CPU data is present
	if ( idImage *baseImg = globalImages->GetImage( imagePath ) ) {
		if ( idImageAsset *image = baseImg->AsAsset() )
			if ( ( image->residency & IR_CPU ) && image->cpuData.IsValid() )
				return image;
	}

	if ( defer ) {
		// make sure string is copied
		idStr pathStr = imagePath;

		globalImages->ExecuteWhenSingleThreaded( [pathStr]() {
			idImageAsset *image = globalImages->ImageFromFile( pathStr, TF_NEAREST, false, TR_CLAMP, TD_HIGH_QUALITY, CF_2D, IR_CPU );
		} );
		return nullptr;	// not loaded yet, wait until next frame
	} 

	idImageAsset *image = globalImages->ImageFromFile( imagePath, TF_NEAREST, false, TR_CLAMP, TD_HIGH_QUALITY, CF_2D, IR_CPU );
	return image;
}
