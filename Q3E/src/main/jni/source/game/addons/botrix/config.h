#ifndef __BOTRIX_CONFIG_H__
#define __BOTRIX_CONFIG_H__


#include "types.h"

#include <good/ini_file.h>


//****************************************************************************************************************
/// Class for storing configuration in ini file format.
//****************************************************************************************************************
class CConfiguration
{

public: // Methods.


    /// Set ini file name.
    static void SetFileName( const good::string& sFileName ) { m_iniFile.name.assign(sFileName, true); }

    static const good::ini_file& GetIniFile() { return m_iniFile; }

    /// Load configuration file. You need also provide game and mod folders in order to detect mod to use.
    static TModId Load( const good::string& sGameDir, const good::string& sModDir );

    /// Save configuration file (must be loaded first).
    static void Save()
    {
        if ( m_bModified )
        {
            m_iniFile.save();
            m_bModified = false;
        }
    }

    /// Unload configuration file, free all memory.
    static void Unload() { m_iniFile.clear(); }

    /// Get value for sKey of [General] section. Could be NULL.
    static const good::string* GetGeneralSectionValue( const good::string& sKey );

    /// Set value for sKey of [General] section.
    static void SetGeneralSectionValue( const good::string& sKey, const good::string& sValue );

    /// Get client access bits to execute commands on server (@see ). Section [User Access] is used.
    static TCommandAccessFlags ClientAccessLevel( const good::string& sSteamId );

    /// Set client access bits to execute commands on server (@see ).
    static void SetClientAccessLevel( const good::string& sSteamId, TCommandAccessFlags iAccess );


protected: // Members.
    /// Process section [General].
    static void ProcessGeneralSection( good::ini_file::const_iterator it );

    /// Search for mod in configuration file that matches current game/mod folders.
    static TModId SearchMod( const good::string& sGameDir, const good::string& sModDir );

    /// Process section [<mod-name>.mod] to get available teams/classes.
    static void ProcessModSection( good::ini_file::const_iterator it );

    /// Load MOD dependent variables such as velocity / player's hull, etc.
    static void LoadModVars();

    /// Load classes names for items.
    static void LoadItemClasses();

    /// Process section [<mod-name>.weapons].
    static void LoadWeapons( good::ini_file::const_iterator itSection );

    friend class CBotrixPlugin;      ///< Access to m_iniFile. TODO: something better.

    static good::ini_file m_iniFile; ///< Ini file.
    static bool m_bModified;         ///< True if something was modified (to save later).

};


#endif // __BOTRIX_CONFIG_H__
