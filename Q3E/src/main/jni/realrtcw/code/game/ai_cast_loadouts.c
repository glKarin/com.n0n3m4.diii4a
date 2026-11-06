//===========================================================================
//
// Name:			ai_cast_loadouts.c
//===========================================================================


#include "g_local.h"
#include "../qcommon/q_shared.h"
#include "../botlib/botlib.h"      //bot lib interface
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/botai.h"          //bot ai interface

#include "ai_cast.h"


// Forward declarations of script actions implemented in ai_cast_script_actions.c
qboolean AICast_ScriptAction_GiveWeapon( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SetAmmo( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SetClip( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SetArmor( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GiveInventory( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SelectWeapon( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_TakeWeapon( cast_state_t *cs, char *params ) ;
qboolean AICast_ScriptAction_GiveScore( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GivePerk( cast_state_t *cs, char *params );

#define LOADOUT_MANIFEST "loadouts/loadouts.cfg"
#define LOADOUT_MANIFEST_BUFSIZE ( 8 * 1024 )
#define LOADOUT_FILE_BUFSIZE     ( 64 * 1024 )

static qboolean AICast_Loadouts_ReadFile( const char *path, char *out, int outSize ) {
    fileHandle_t f;
    int len = trap_FS_FOpenFile( path, &f, FS_READ );
    if ( len <= 0 || !f ) {
        return qfalse;
    }
    if ( len >= outSize ) {
        trap_FS_FCloseFile( f );
        return qfalse;
    }
    trap_FS_Read( out, len, f );
    out[len] = '\0';
    trap_FS_FCloseFile( f );
    return qtrue;
}

static void AICast_Loadouts_ParseArgsOnLine( char **p, char *out, int outSize ) {
    char *tok;

    out[0] = '\0';

    // read tokens until newline (COM_ParseExt returns "" if allowLineBreaks==qfalse and it hits a newline)
    while ( 1 ) {
        tok = COM_ParseExt( p, qfalse );
        if ( !tok[0] ) {
            break;
        }

        if ( out[0] ) {
            Q_strcat( out, outSize, " " );
        }
        Q_strcat( out, outSize, tok );
    }
}

static qboolean AICast_Loadouts_RunCommand( cast_state_t *pcs, const char *cmd, char *args ) {
    while ( *args && *args <= ' ' ) args++;

    if ( !Q_stricmp( cmd, "giveweapon" ) ) {
        return AICast_ScriptAction_GiveWeapon( pcs, args );
    }
    if ( !Q_stricmp( cmd, "givescore" ) ) {
        return AICast_ScriptAction_GiveScore( pcs, args );
    }
    if ( !Q_stricmp( cmd, "giveperk" ) ) {
        return AICast_ScriptAction_GivePerk( pcs, args );
    }
    if ( !Q_stricmp( cmd, "takeweapon" ) ) {
        return AICast_ScriptAction_TakeWeapon( pcs, args );
    }
    if ( !Q_stricmp( cmd, "setammo" ) ) {
        return AICast_ScriptAction_SetAmmo( pcs, args );
    }
    if ( !Q_stricmp( cmd, "setclip" ) ) {
        return AICast_ScriptAction_SetClip( pcs, args );
    }
    if ( !Q_stricmp( cmd, "setarmor" ) ) {
        return AICast_ScriptAction_SetArmor( pcs, args );
    }
    if ( !Q_stricmp( cmd, "giveinventory" ) ) {
        return AICast_ScriptAction_GiveInventory( pcs, args );
    }
    if ( !Q_stricmp( cmd, "selectweapon" ) ) {
        return AICast_ScriptAction_SelectWeapon( pcs, args );
    }

    if ( g_cheats.integer ) {
        G_Printf( "Loadout: unknown cmd '%s'\n", cmd );
    }
    return qfalse;
}

static qboolean AICast_Loadouts_ApplyFromFile( cast_state_t *cs, gentity_t *target, const char *loadoutName, const char *path ) {
    static char fileBuf[LOADOUT_FILE_BUFSIZE];
    char *p, *tok;
    char name[64];
    cast_state_t pcs;

    if ( !target || !target->client ) return qfalse;
    if ( !loadoutName || !loadoutName[0] ) return qfalse;

    Q_strncpyz( name, loadoutName, sizeof(name) );

    if ( !AICast_Loadouts_ReadFile( path, fileBuf, sizeof(fileBuf) ) ) {
        // silent fail is fine here (manifest may list optional files)
        return qfalse;
    }

    pcs = *cs;
    pcs.entityNum   = (int)( target - g_entities );
    pcs.aiCharacter = target->aiCharacter;

    p = fileBuf;
    COM_BeginParseSession( path );

    while ( 1 ) {
        tok = COM_ParseExt( &p, qtrue );
        if ( !tok[0] ) break;

        if ( Q_stricmp( tok, "loadout" ) ) {
            continue;
        }

        {
            char blockNameBuf[64];
            char *blockNameTok = COM_ParseExt( &p, qfalse );

            if ( !blockNameTok[0] ) {
                COM_ParseError( "expected loadout name after 'loadout'" );
                return qfalse;
            }

            Q_strncpyz( blockNameBuf, blockNameTok, sizeof(blockNameBuf) );

            tok = COM_ParseExt( &p, qtrue );
            if ( tok[0] != '{' ) {
                COM_ParseError( "expected '{' after loadout name '%s'", blockNameBuf );
                return qfalse;
            }

            if ( Q_stricmp( blockNameBuf, name ) ) {
                if ( !SkipBracedSection( &p, 1 ) ) {
                    COM_ParseError( "unterminated '{' in loadout '%s'", blockNameBuf );
                    return qfalse;
                }
                continue;
            }

            while ( 1 ) {
                char args[1024];
                char cmdBuf[64];

                tok = COM_ParseExt( &p, qtrue );
                if ( !tok[0] ) {
                    COM_ParseError( "EOF while parsing loadout '%s' in %s (missing '}')", name, path );
                    return qfalse;
                }

                if ( tok[0] == '}' ) {
                    return qtrue; // found & applied
                }

                Q_strncpyz( cmdBuf, tok, sizeof(cmdBuf) );

                AICast_Loadouts_ParseArgsOnLine( &p, args, sizeof(args) );
                AICast_Loadouts_RunCommand( &pcs, cmdBuf, args );
            }
        }
    }

    return qfalse; // not found in this file
}

qboolean AICast_Loadouts_ApplyToEnt( cast_state_t *cs, gentity_t *target, const char *loadoutName ) {
    static char manifestBuf[LOADOUT_MANIFEST_BUFSIZE];
    char *p, *line;

    if ( !target || !target->client ) return qfalse;
    if ( !loadoutName || !loadoutName[0] ) return qfalse;

    if ( !AICast_Loadouts_ReadFile( LOADOUT_MANIFEST, manifestBuf, sizeof(manifestBuf) ) ) {
        G_Printf( "applyloadout: couldn't read %s\n", LOADOUT_MANIFEST );
        return qfalse;
    }

    p = manifestBuf;

    while ( 1 ) {
        // manual line splitting
        line = p;
        while ( *p && *p != '\n' && *p != '\r' ) p++;
        while ( *p == '\n' || *p == '\r' ) { *p = '\0'; p++; }

        // end
        if ( !line[0] && !*p ) break;

        // trim leading spaces
        while ( *line && (unsigned char)*line <= ' ' ) line++;

        // strip // comments
        {
            char *c = strstr( line, "//" );
            if ( c ) *c = '\0';
        }

        // trim again
        while ( *line && (unsigned char)*line <= ' ' ) line++;
        if ( !line[0] ) {
            if ( !*p ) break;
            continue;
        }

        // line is a file path
        if ( AICast_Loadouts_ApplyFromFile( cs, target, loadoutName, line ) ) {
            return qtrue;
        }

        if ( !*p ) break;
    }

    G_Printf( "applyloadout: loadout '%s' not found (manifest %s)\n", loadoutName, LOADOUT_MANIFEST );
    return qfalse;
}