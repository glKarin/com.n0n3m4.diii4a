// Bot_wieghts.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

idBotFuzzyWeightManager botFuzzyWeightManager;

/*
========================
idBotFuzzyWeightManager::Init
========================
*/
void idBotFuzzyWeightManager::Init( void )
{
	memset( &fuzzyseperators[0], 0, sizeof( fuzzyseperators ) );
}

/*
========================
idBotFuzzyWeightManager::AllocFuzzyWeight
========================
*/
fuzzyseperator_t* idBotFuzzyWeightManager::AllocFuzzyWeight( void )
{
	for( int i = 0; i < MAX_FUZZY_OPERATORS; i++ )
	{
		if( fuzzyseperators[i].inUse == false )
		{
			memset( &fuzzyseperators[i], 0, sizeof( fuzzyseperator_t ) );
			fuzzyseperators[i].inUse = true;
			return &fuzzyseperators[i];
		}
	}

	gameLocal.Error( "AllocFuzzyWeight: Not enough fuzzy weights\n" );
	return NULL;
}

/*
========================
idBotFuzzyWeightManager::ReadValue
========================
*/
bool idBotFuzzyWeightManager::ReadValue( idParser& source, float* value )
{
	idToken token;

	if( !source.ReadToken( &token ) )
	{
		return false;
	}

	if( token == "-" )
	{
		source.Warning( "negative value set to zero\n" );
		if( !source.ExpectTokenType( TT_NUMBER, 0, &token ) )
		{
			return false;
		}
	}

	if( token.type != TT_NUMBER )
	{
		source.Error( "invalid return value %s\n", token.c_str() );
		return false;
	}

	*value = token.GetFloatValue();

	return true;
}

/*
===================
idBotFuzzyWeightManager::ReadFuzzyWeight
===================
*/
int idBotFuzzyWeightManager::ReadFuzzyWeight( idParser& source, fuzzyseperator_t* fs )
{
	if( source.CheckTokenString( "balance" ) )
	{
		fs->type = WT_BALANCE;

		if( !source.ExpectTokenString( "(" ) )
		{
			return false;
		}

		if( !ReadValue( source, &fs->weight ) )
		{
			return false;
		}

		if( !source.ExpectTokenString( "," ) )
		{
			return false;
		}

		if( !ReadValue( source, &fs->minweight ) )
		{
			return false;
		}

		if( !source.ExpectTokenString( "," ) )
		{
			return false;
		}

		if( !ReadValue( source, &fs->maxweight ) )
		{
			return false;
		}

		if( !source.ExpectTokenString( ")" ) )
		{
			return false;
		}
	}
	else
	{
		fs->type = 0;

		if( !ReadValue( source, &fs->weight ) )
		{
			return false;
		}
		fs->minweight = fs->weight;
		fs->maxweight = fs->weight;
	}

	if( !source.ExpectTokenString( ";" ) )
	{
		return false;
	}

	return true;
}

/*
===================
idBotFuzzyWeightManager::FreeFuzzySeperators_r
===================
*/
void idBotFuzzyWeightManager::FreeFuzzySeperators_r( fuzzyseperator_t* fs )
{
	if( !fs )
	{
		return;
	}
	if( fs->child )
	{
		FreeFuzzySeperators_r( fs->child );
	}
	if( fs->next )
	{
		FreeFuzzySeperators_r( fs->next );
	}

	fs->inUse = false;
}

/*
===================
FreeWeightConfig2
===================
*/
void idBotFuzzyWeightManager::FreeWeightConfig2( weightconfig_t* config )
{
	int i;

	for( i = 0; i < config->numweights; i++ )
	{
		FreeFuzzySeperators_r( config->weights[i].firstseperator );
		// jmarshall - todo
		//if (config->weights[i].name)
		//	FreeMemory(config->weights[i].name);
		// jmarshall end
	}

}

/*
===================
idBotFuzzyWeightManager::FreeWeightConfig
===================
*/
void idBotFuzzyWeightManager::FreeWeightConfig( weightconfig_t* config )
{
	//if (!LibVarGetValue("bot_reloadcharacters"))
	//	return;

	FreeWeightConfig2( config );
}

/*
===================
idBotFuzzyWeightManager::ReadFuzzySeperators_r
===================
*/
fuzzyseperator_t* idBotFuzzyWeightManager::ReadFuzzySeperators_r( idParser& source )
{
	int newindent, index, founddefault;
	bool def;
	idToken token;
	fuzzyseperator_t* fs, * lastfs, * firstfs;

	founddefault = false;
	firstfs = NULL;
	lastfs = NULL;
	if( !source.ExpectTokenString( "(" ) )
	{
		return NULL;
	}

	if( !source.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) )
	{
		return NULL;
	}

	index = token.GetIntValue();

	if( !source.ExpectTokenString( ")" ) )
	{
		return NULL;
	}

	if( !source.ExpectTokenString( "{" ) )
	{
		return NULL;
	}

	if( !source.ExpectAnyToken( &token ) )
	{
		return NULL;
	}

	do
	{
		def = ( token == "default" ); //!strcmp(token.string, "default");
		if( def || ( token == "case" ) )
		{
			fs = AllocFuzzyWeight();
			fs->index = index;
			if( lastfs )
			{
				lastfs->next = fs;
			}
			else
			{
				firstfs = fs;
			}
			lastfs = fs;
			if( def )
			{
				if( founddefault )
				{
					FreeFuzzySeperators_r( firstfs );
					gameLocal.Error( "switch already has a default\n" );
					return NULL;
				}
				fs->value = MAX_INVENTORYVALUE;
				founddefault = true;
			}
			else
			{
				if( !source.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) )
				{
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}

				fs->value = token.GetIntValue();
			}

			if( !source.ExpectTokenString( ":" ) || !source.ExpectAnyToken( &token ) )
			{
				FreeFuzzySeperators_r( firstfs );
				return NULL;
			}

			newindent = false;
			if( token == "{" )
			{
				newindent = true;
				if( !source.ExpectAnyToken( &token ) )
				{
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}
			}

			if( token == "return" )
			{
				if( !ReadFuzzyWeight( source, fs ) )
				{
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}
			}
			else if( token == "switch" )
			{
				fs->child = ReadFuzzySeperators_r( source );
				if( !fs->child )
				{
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}
			}
			else
			{
				gameLocal.Error( "invalid name %s\n", token.c_str() );
				return NULL;
			}

			if( newindent )
			{
				if( !source.ExpectTokenString( "}" ) )
				{
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}
			}
		}
		else
		{
			gameLocal.Error( "invalid name %s\n", token.c_str() );
			FreeFuzzySeperators_r( firstfs );
			return NULL;
		}

		if( !source.ExpectAnyToken( &token ) )
		{
			FreeFuzzySeperators_r( firstfs );
			return NULL;
		}
	}
	while( token != "}" );

	if( !founddefault )
	{
		source.Warning( "switch without default\n" );
		fs = AllocFuzzyWeight();
		fs->index = index;
		fs->value = MAX_INVENTORYVALUE;
		fs->weight = 0;
		fs->next = NULL;
		fs->child = NULL;
		if( lastfs )
		{
			lastfs->next = fs;
		}
		else
		{
			firstfs = fs;
		}
		lastfs = fs;
	}

	return firstfs;
}

/*
=====================
idBotFuzzyWeightManager::ReadWeightConfig
=====================
*/
weightconfig_t* idBotFuzzyWeightManager::ReadWeightConfig( char* filename )
{
	int newindent, avail = 0, n;
	idToken token;
	idParser source;
	fuzzyseperator_t* fs;
	weightconfig_t* config = NULL;

	avail = -1;
	for( n = 0; n < MAX_WEIGHT_FILES; n++ )
	{
		config = &weightFileList[n];

		if( config->inUse )
		{
			continue;
		}

		if( config->filename == filename )
		{
			return config;
		}

		avail = n;
		break;
	}

	if( avail == -1 )
	{
		gameLocal.Error( "weightFileList was full trying to load %s\n", filename );
		return NULL;
	}

	rvmScopedLexerBaseFolder scopedBaseFolder( BOTFILESBASEFOLDER );

	if( !source.LoadFile( filename ) )
	{
		gameLocal.Error( "couldn't load %s\n", filename );
		return NULL;
	}

	config = &weightFileList[avail];
	config->Reset();

	config->filename = filename;

	//parse the item config file
	while( source.ReadToken( &token ) )
	{
		if( token == "weight" )
		{
			if( config->numweights >= MAX_WEIGHTS )
			{
				gameLocal.Error( "too many fuzzy weights\n" );
				break;
			}

			if( !source.ExpectTokenType( TT_STRING, 0, &token ) )
			{
				FreeWeightConfig( config );
				return NULL;
			}

			token.StripDoubleQuotes();
			config->weights[config->numweights].name = token.c_str();

			if( !source.ExpectAnyToken( &token ) )
			{
				FreeWeightConfig( config );
				return NULL;
			}

			newindent = false;
			if( token == "{" )
			{
				newindent = true;
				if( !source.ExpectAnyToken( &token ) )
				{
					FreeWeightConfig( config );
					return NULL;
				}
			}

			if( token == "switch" )
			{
				fs = ReadFuzzySeperators_r( source );
				if( !fs )
				{
					FreeWeightConfig( config );
					return NULL;
				}

				config->weights[config->numweights].firstseperator = fs;
			}
			else if( token == "return" )
			{
				fs = ( fuzzyseperator_t* )AllocFuzzyWeight();
				fs->index = 0;
				fs->value = MAX_INVENTORYVALUE;
				fs->next = NULL;
				fs->child = NULL;
				if( !ReadFuzzyWeight( source, fs ) )
				{
					FreeWeightConfig( config );
					return NULL;
				}
				config->weights[config->numweights].firstseperator = fs;
			}
			else
			{
				gameLocal.Error( "invalid name %s\n", token.c_str() );
				FreeWeightConfig( config );
				return NULL;
			}

			if( newindent )
			{
				if( !source.ExpectTokenString( "}" ) )
				{
					FreeWeightConfig( config );
					return NULL;
				}
			}
			config->numweights++;
		}
		else
		{
			gameLocal.Error( "invalid name %s\n", token.c_str() );
			FreeWeightConfig( config );
			return NULL;
		}
	}

	//if the file was located in a pak file
	common->Printf( "idBotFuzzyWeightManager::ReadWeightConfig: loaded %s\n", filename );
	config->inUse = true;
	return config;
}

/*
==================
idBotFuzzyWeightManager::FindFuzzyWeight
==================
*/
int idBotFuzzyWeightManager::FindFuzzyWeight( weightconfig_t* wc, char* name )
{
	int i;

	for( i = 0; i < wc->numweights; i++ )
	{
		if( wc->weights[i].name == name )
		{
			return i;
		}
	}
	return -1;
}

/*
==================
idBotFuzzyWeightManager::FuzzyWeight_r
==================
*/
float idBotFuzzyWeightManager::FuzzyWeight_r( int* inventory, fuzzyseperator_t* fs )
{
	float scale, w1, w2;

	if( inventory[fs->index] < fs->value )
	{
		if( fs->child )
		{
			return FuzzyWeight_r( inventory, fs->child );
		}
		else
		{
			return fs->weight;
		}
	}
	else if( fs->next )
	{
		if( inventory[fs->index] < fs->next->value )
		{
			//first weight
			if( fs->child )
			{
				w1 = FuzzyWeight_r( inventory, fs->child );
			}
			else
			{
				w1 = fs->weight;
			}

			//second weight
			if( fs->next->child )
			{
				w2 = FuzzyWeight_r( inventory, fs->next->child );
			}
			else
			{
				w2 = fs->next->weight;
			}

			//the scale factor
			scale = ( inventory[fs->index] - fs->value ) / ( fs->next->value - fs->value );

			//scale between the two weights
			return scale * w1 + ( 1 - scale ) * w2;
		}
		return FuzzyWeight_r( inventory, fs->next );
	}
	return fs->weight;
}

/*
============================
idBotFuzzyWeightManager::FuzzyWeightUndecided_r
============================
*/
float idBotFuzzyWeightManager::FuzzyWeightUndecided_r( int* inventory, fuzzyseperator_t* fs )
{
	float scale, w1, w2;

	if( inventory[fs->index] < fs->value )
	{
		if( fs->child )
		{
			return FuzzyWeightUndecided_r( inventory, fs->child );
		}
		else
		{
			return fs->minweight + rvmBotUtil::random() * ( fs->maxweight - fs->minweight );
		}
	}
	else if( fs->next )
	{
		if( inventory[fs->index] < fs->next->value )
		{
			//first weight
			if( fs->child )
			{
				w1 = FuzzyWeightUndecided_r( inventory, fs->child );
			}
			else
			{
				w1 = fs->minweight + rvmBotUtil::random() * ( fs->maxweight - fs->minweight );
			}

			//second weight
			if( fs->next->child )
			{
				w2 = FuzzyWeight_r( inventory, fs->next->child );
			}
			else
			{
				w2 = fs->next->minweight + rvmBotUtil::random() * ( fs->next->maxweight - fs->next->minweight );
			}

			//the scale factor
			scale = ( inventory[fs->index] - fs->value ) / ( fs->next->value - fs->value );

			//scale between the two weights
			return scale * w1 + ( 1 - scale ) * w2;
		}
		return FuzzyWeightUndecided_r( inventory, fs->next );
	}
	return fs->weight;
}

/*
=================
idBotFuzzyWeightManager::FuzzyWeight
=================
*/
float idBotFuzzyWeightManager::FuzzyWeight( int* inventory, weightconfig_t* wc, int weightnum )
{
	return FuzzyWeight_r( inventory, wc->weights[weightnum].firstseperator );
}

/*
=================
idBotFuzzyWeightManager::FuzzyWeightUndecided
=================
*/
float idBotFuzzyWeightManager::FuzzyWeightUndecided( int* inventory, weightconfig_t* wc, int weightnum )
{
	return FuzzyWeightUndecided_r( inventory, wc->weights[weightnum].firstseperator );
}

/*
====================
idBotFuzzyWeightManager::EvolveFuzzySeperator_r
====================
*/
void idBotFuzzyWeightManager::EvolveFuzzySeperator_r( fuzzyseperator_t* fs )
{
	if( fs->child )
	{
		EvolveFuzzySeperator_r( fs->child );
	}
	else if( fs->type == WT_BALANCE )
	{
		//every once in a while an evolution leap occurs, mutation
		if( rvmBotUtil::random() < 0.01 )
		{
			fs->weight += rvmBotUtil::crandom() * ( fs->maxweight - fs->minweight );
		}
		else
		{
			fs->weight += rvmBotUtil::crandom() * ( fs->maxweight - fs->minweight ) * 0.5;
		}

		//modify bounds if necesary because of mutation
		if( fs->weight < fs->minweight )
		{
			fs->minweight = fs->weight;
		}
		else if( fs->weight > fs->maxweight )
		{
			fs->maxweight = fs->weight;
		}
	}
	if( fs->next )
	{
		EvolveFuzzySeperator_r( fs->next );
	}
}

/*
====================
idBotFuzzyWeightManager::EvolveWeightConfig
====================
*/
void idBotFuzzyWeightManager::EvolveWeightConfig( weightconfig_t* config )
{
	int i;

	for( i = 0; i < config->numweights; i++ )
	{
		EvolveFuzzySeperator_r( config->weights[i].firstseperator );
	}
}

/*
====================
idBotFuzzyWeightManager::ScaleWeight
====================
*/
void idBotFuzzyWeightManager::ScaleFuzzySeperator_r( fuzzyseperator_t* fs, float scale )
{
	if( fs->child )
	{
		ScaleFuzzySeperator_r( fs->child, scale );
	}
	else if( fs->type == WT_BALANCE )
	{
		fs->weight = ( fs->maxweight + fs->minweight ) * scale;

		//get the weight between bounds
		if( fs->weight < fs->minweight )
		{
			fs->weight = fs->minweight;
		}
		else if( fs->weight > fs->maxweight )
		{
			fs->weight = fs->maxweight;
		}
	}
	if( fs->next )
	{
		ScaleFuzzySeperator_r( fs->next, scale );
	}
}

/*
====================
idBotFuzzyWeightManager::ScaleWeight
====================
*/
void idBotFuzzyWeightManager::ScaleWeight( weightconfig_t* config, char* name, float scale )
{
	int i;

	if( scale < 0 )
	{
		scale = 0;
	}
	else if( scale > 1 )
	{
		scale = 1;
	}
	for( i = 0; i < config->numweights; i++ )
	{
		if( config->weights[i].name == name )
		{
			ScaleFuzzySeperator_r( config->weights[i].firstseperator, scale );
			break;
		}
	}
}

/*
====================
idBotFuzzyWeightManager::ScaleFuzzySeperatorBalanceRange_r
====================
*/
void idBotFuzzyWeightManager::ScaleFuzzySeperatorBalanceRange_r( fuzzyseperator_t* fs, float scale )
{
	if( fs->child )
	{
		ScaleFuzzySeperatorBalanceRange_r( fs->child, scale );
	}
	else if( fs->type == WT_BALANCE )
	{
		float mid = ( fs->minweight + fs->maxweight ) * 0.5;
		//get the weight between bounds
		fs->maxweight = mid + ( fs->maxweight - mid ) * scale;
		fs->minweight = mid + ( fs->minweight - mid ) * scale;
		if( fs->maxweight < fs->minweight )
		{
			fs->maxweight = fs->minweight;
		}
	}
	if( fs->next )
	{
		ScaleFuzzySeperatorBalanceRange_r( fs->next, scale );
	}
}

/*
====================
idBotFuzzyWeightManager::ScaleFuzzyBalanceRange
====================
*/
void idBotFuzzyWeightManager::ScaleFuzzyBalanceRange( weightconfig_t* config, float scale )
{
	int i;

	if( scale < 0 )
	{
		scale = 0;
	}
	else if( scale > 100 )
	{
		scale = 100;
	}
	for( i = 0; i < config->numweights; i++ )
	{
		ScaleFuzzySeperatorBalanceRange_r( config->weights[i].firstseperator, scale );
	}
}

/*
====================
idBotFuzzyWeightManager::InterbreedFuzzySeperator_r
====================
*/
int idBotFuzzyWeightManager::InterbreedFuzzySeperator_r( fuzzyseperator_t* fs1, fuzzyseperator_t* fs2, fuzzyseperator_t* fsout )
{
	if( fs1->child )
	{
		if( !fs2->child || !fsout->child )
		{
			gameLocal.Error( "cannot interbreed weight configs, unequal child\n" );
			return false;
		}
		if( !InterbreedFuzzySeperator_r( fs2->child, fs2->child, fsout->child ) )
		{
			return false;
		}
	}
	else if( fs1->type == WT_BALANCE )
	{
		if( fs2->type != WT_BALANCE || fsout->type != WT_BALANCE )
		{
			gameLocal.Error( "cannot interbreed weight configs, unequal balance\n" );
			return false;
		}
		fsout->weight = ( fs1->weight + fs2->weight ) / 2;
		if( fsout->weight > fsout->maxweight )
		{
			fsout->maxweight = fsout->weight;
		}
		if( fsout->weight > fsout->minweight )
		{
			fsout->minweight = fsout->weight;
		}
	}
	if( fs1->next )
	{
		if( !fs2->next || !fsout->next )
		{
			gameLocal.Error( "cannot interbreed weight configs, unequal next\n" );
			return false;
		}
		if( !InterbreedFuzzySeperator_r( fs1->next, fs2->next, fsout->next ) )
		{
			return false;
		}
	}
	return true;
}

/*
====================
idBotFuzzyWeightManager::InterbreedWeightConfigs
====================
*/
void idBotFuzzyWeightManager::InterbreedWeightConfigs( weightconfig_t* config1, weightconfig_t* config2, weightconfig_t* configout )
{
	int i;

	if( config1->numweights != config2->numweights ||
			config1->numweights != configout->numweights )
	{
		gameLocal.Error( "cannot interbreed weight configs, unequal numweights\n" );
		return;
	}

	for( i = 0; i < config1->numweights; i++ )
	{
		InterbreedFuzzySeperator_r( config1->weights[i].firstseperator,
									config2->weights[i].firstseperator,
									configout->weights[i].firstseperator );
	}
}


/*
=======================
idBotFuzzyWeightManager::BotShutdownWeights
=======================
*/
void idBotFuzzyWeightManager::BotShutdownWeights( void )
{
	int i;

	for( i = 0; i < MAX_WEIGHT_FILES; i++ )
	{
		weightFileList[i].inUse = false;
		//if (weightFileList[i])
		//{
		//	FreeWeightConfig2(weightFileList[i]);
		//	weightFileList[i] = NULL;
		//}
	}
}
