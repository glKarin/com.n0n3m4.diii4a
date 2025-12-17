#include <stdlib.h> // atoi

#include <good/file.h>
#include <good/string_buffer.h>
#include <good/string_utils.h>

#include "chat.h"
#include "config.h"
#include "mod.h"
#include "source_engine.h"
#include "type2string.h"
#include "types.h"
#include "weapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern char* szMainBuffer;
extern int iMainBufferSize;


//------------------------------------------------------------------------------------------------------------
good::ini_file CConfiguration::m_iniFile; // Ini file.
bool CConfiguration::m_bModified;         // True if something was modified (to save later).

//------------------------------------------------------------------------------------------------------------
TModId CConfiguration::Load( const good::string& sGameDir, const good::string& sModDir )
{
    m_bModified = false;
    if ( m_iniFile.load() >= good::IniFileNotFound )
    {
        BLOG_E("Error reading configuration file %s.", m_iniFile.name.c_str());
        return EModId_Invalid;
    }

    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);

    // Process general section.
    good::ini_file::const_iterator it = m_iniFile.find("General");
    if ( it == m_iniFile.end() )
        BLOG_E( "File \"%s\", missing [General] section.", m_iniFile.name.c_str() );
    else
        ProcessGeneralSection(it);

    TModId iModId = SearchMod( sGameDir, sModDir );
    if ( iModId == EModId_Invalid )
    {
        BLOG_E( "File \"%s\":", m_iniFile.name.c_str() );
        BLOG_E( "  There is no mod that matches current game (%s) & mod (%s) folders.", sGameDir.c_str(), sModDir.c_str() );
        BLOG_E( "  Using default mod 'HalfLife2Deathmatch'." );
        CMod::sModName = "HalfLife2Deathmatch";
        iModId = EModId_HL2DM;
    }
    else
        BLOG_D("Current mod: '%s'.", CMod::sModName.c_str());

    // First load default values, and then set mod vars that are present.
    CMod::LoadDefaults( iModId );
    LoadModVars();

    // Find section "<mod name>.mod".
    sbBuffer = CMod::sModName;
    sbBuffer << ".mod";
    it = m_iniFile.find( sbBuffer );
    if ( it == m_iniFile.end() )
        BLOG_E( "File \"%s\", missing [%s.mod] section.", m_iniFile.name.c_str(), CMod::sModName.c_str() );
    else
        ProcessModSection(it);

    LoadItemClasses();

    // Load weapons.
    sbBuffer = CMod::sModName;
    sbBuffer << ".weapons";
    it = m_iniFile.find( sbBuffer );
    if ( it == m_iniFile.end() )
        BLOG_E( "File \"%s\", missing [%s.weapons] section.", m_iniFile.name.c_str(), CMod::sModName.c_str() );
    else
        LoadWeapons(it);

#ifdef BOTRIX_CHAT
    // Load chat: synonims.
    BLOG_D("Loading chat synonims:");
    it = m_iniFile.find( "Chat.replace" );
    if ( it != m_iniFile.end() )
    {
        // Iterate throught key values.
        for ( good::ini_section::const_iterator wordIt = it->begin(); wordIt != it->end(); ++wordIt )
            CChat::AddSynonims( wordIt->key, wordIt->value );
    }

    // Load chat: commands.
    BLOG_D("Loading chat sentences:");
    it = m_iniFile.find( "Chat.sentences" );
    if ( it != m_iniFile.end() )
    {
        // Iterate throught key values.
        for ( good::ini_section::const_iterator wordIt = it->begin(); wordIt != it->end(); ++wordIt )
            CChat::AddChat( wordIt->key, wordIt->value );
    }
#endif // BOTRIX_CHAT

    CMod::Prepare();

    return iModId;
}


//------------------------------------------------------------------------------------------------------------
const good::string* CConfiguration::GetGeneralSectionValue( const good::string& sKey )
{
    good::ini_section& sect = m_iniFile["General"];
    good::ini_section::iterator kv = sect.find(sKey);
    return (kv == sect.end()) ? NULL : &kv->value;
}


//------------------------------------------------------------------------------------------------------------
void CConfiguration::SetGeneralSectionValue( const good::string& sKey, const good::string& sValue )
{
    good::ini_section& sect = m_iniFile["General"];
    sect[sKey] = sValue;
    m_bModified = true;
}


//------------------------------------------------------------------------------------------------------------
TCommandAccessFlags CConfiguration::ClientAccessLevel( const good::string& sSteamId )
{
    good::ini_section& sect = m_iniFile["User access"];
    good::ini_section::iterator kv = sect.find_escaped(sSteamId);

    TCommandAccessFlags iResult = 0;
    if ( kv != sect.end() )
    {
         iResult = CTypeToString::AccessFlagsFromString(kv->value);
         if ( iResult == -1 )
         {
             BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), sect.name.c_str() );
             BLOG_E( "  Invalid access for user %s: %s.", kv->key.c_str(), kv->value.c_str() );
             iResult = 0;
         }
    }

    return iResult;
}


//----------------------------------------------------------------------------------------------------------------
void CConfiguration::SetClientAccessLevel( const good::string& sSteamId, TCommandAccessFlags iAccess )
{
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
    sbBuffer.assign(sSteamId);
    if ( good::starts_with(sbBuffer, '[') )
        sbBuffer.insert("\\", 0);

    good::ini_section& sect = m_iniFile["User access"];
    sect[ sbBuffer.duplicate() ] = CTypeToString::AccessFlagsToString(iAccess).duplicate();
    m_bModified = true;
    Save();
}


//----------------------------------------------------------------------------------------------------------------
TModId CConfiguration::SearchMod( const good::string& sGameDir, const good::string& sModDir )
{
    TModId iModId = EModId_Invalid;
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);

    // Set current mod from mods sections.
    for ( good::ini_file::const_iterator it = m_iniFile.begin(); it != m_iniFile.end(); ++it )
    {
        if ( good::ends_with(it->name, ".mod") )
        {
            good::ini_section::iterator games = it->find("games");
            good::ini_section::iterator mods = it->find("mods");
            if ( (games != it->end()) && (mods != it->end()) )
            {
                sbBuffer = games->value;
                good::lower_case(sbBuffer);
                StringVector aGameDirs(4), aModsDirs(4);
                good::split((good::string)sbBuffer, aGameDirs, ',', true);
                for ( StringVector::const_iterator gameDirsIt = aGameDirs.begin(); gameDirsIt != aGameDirs.end(); ++gameDirsIt )
                {
                    if ( *gameDirsIt != sGameDir )
                        continue;

                    sbBuffer = mods->value;
                    good::escape(sbBuffer);
                    aModsDirs.clear();
                    good::split( (good::string)sbBuffer, aModsDirs, ',', true );
                    for ( StringVector::const_iterator modDirsIt = aModsDirs.begin(); modDirsIt != aModsDirs.end(); ++modDirsIt )
                    {
                        if ( *modDirsIt != sModDir )
                            continue;

                        // Get bot used in this mod.
                        good::ini_section::iterator bot = it->find("bot");
                        if ( bot != it->end() )
                        {
                            iModId = CTypeToString::ModFromString(bot->value);
                            if ( iModId == EModId_Invalid )
                            {
                                BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                                BLOG_E( "  Invalid bot: %s.", bot->value.c_str() );
                                BLOG_E("   Using default bot: hl2dm.");
                                iModId = EModId_HL2DM;
                            }
                        }
                        else
                        {
                            BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                            BLOG_E( "  No 'bot' key. Using default bot: hl2dm." );
                            iModId = EModId_HL2DM;
                        }

                        // Get mod name in sbBuffer.
                        sbBuffer = it->name;
                        sbBuffer.erase( sbBuffer.length() - 4, 4 ); // Erase ".mod" from section name.

                        CMod::sModName.assign(sbBuffer, true);
                        return iModId;
                    }
                }
            }
            else
            {
                BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                BLOG_E( "  Invalid mod section (missing games/mods keys)." );
            }
        }
    }
    return iModId;
}


//----------------------------------------------------------------------------------------------------------------
void CConfiguration::ProcessModSection( good::ini_file::const_iterator it )
{
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);

    // Get player teams.
    good::ini_section::const_iterator itSect = it->find("teams");
    if ( itSect != it->end() )
    {
        sbBuffer = itSect->value;
        good::escape(sbBuffer);
        good::split( (good::string)sbBuffer, CMod::aTeamsNames, ',', true );

        BLOG_D("Teams names:");
        for ( int i = 0; i < CMod::aTeamsNames.size(); ++i )
        {
            if ( CMod::aTeamsNames[i] == "unassigned" )
                CMod::iUnassignedTeam = i;
            else if ( CMod::aTeamsNames[i] == "spectators" )
                CMod::iSpectatorTeam = i;
            BLOG_D( "  %s", CMod::aTeamsNames[i].c_str() );
        }
    }
    else
    {
        BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
        BLOG_E( "  Missing team information." );
        BLOG_E( "  Using default teams (unassigned, spectator, unknown1, unknown2)." );
        CMod::aTeamsNames.push_back("unassigned");
        CMod::aTeamsNames.push_back("spectator");
        CMod::aTeamsNames.push_back("unknown1");
        CMod::aTeamsNames.push_back("unknown2");
    }

    // Get player classes.
    itSect = it->find("classes");
    if ( itSect != it->end() )
    {
        sbBuffer = itSect->value;
        good::escape(sbBuffer);
        good::split( (good::string)sbBuffer, CMod::aClassNames, ',', true );

        BLOG_D("Classes names:");
        for ( int i = 0; i < CMod::aClassNames.size(); ++i )
            BLOG_D( "  %s", CMod::aClassNames[i].c_str() );
    }
}


//----------------------------------------------------------------------------------------------------------------
void CConfiguration::ProcessGeneralSection( good::ini_file::const_iterator it )
{
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);

    // Check log file level.
    good::ini_section::iterator kv = it->find("console_log_level");
    if ( kv != it->end() )
    {
        int iLevel = CTypeToString::LogLevelFromString(kv->value);
        if ( iLevel == -1 )
        {
            BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
            BLOG_E( "  Invalid log level %s.", kv->value.c_str() );
        }
        else
        {
            CUtil::iLogLevel = iLevel;
            BLOG_I("Console log level: %s.", kv->value.c_str());
        }
    }

    // Check if need to log to a file.
    kv = it->find("file_log");
    if ( kv != it->end() )
    {
        if ( good::file::absolute(kv->value.c_str()) )
            sbBuffer = kv->value;
        else
        {
            sbBuffer = CBotrixPlugin::instance->sBotrixPath;
            sbBuffer << PATH_SEPARATOR << kv->value;
        }
        if ( good::log::start_log_to_file( sbBuffer.c_str(), false ) )
            BLOG_I("Log to file: %s.", kv->value.c_str());
        else
        {
            BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
            BLOG_E( "  Can't open log file %s.", kv->value.c_str() );
        }
    }

    // Check log file level.
    kv = it->find("file_log_level");
    if ( kv != it->end() )
    {
        int iLevel = CTypeToString::LogLevelFromString(kv->value);
        if ( iLevel == -1 )
        {
            BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
            BLOG_E( "  Invalid log level %s.", kv->value.c_str() );
        }
        else
        {
            good::log::iFileLogLevel = iLevel;
            BLOG_I("File log level: %s.", kv->value.c_str());
        }
    }

    // Check if need to show intelligence in it's name.
    kv = it->find("name_with_intelligence");
    if ( kv != it->end() )
    {
        int iValue = CTypeToString::BoolFromString(kv->value);
        if ( iValue == -1 )
        {
            BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
            BLOG_E( "  Invalid parameter %s for %s.", kv->value.c_str(), kv->key.c_str() );
        }
        else
        {
            CMod::bIntelligenceInBotName = (iValue != 0);
            BLOG_D("Intelligence in bot name: %s.", kv->value.c_str());
        }
    }

    // Get bot names.
    kv = it->find("names");
    if ( kv == it->end() )
    {
        BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
        BLOG_E( "  Missing bot_names." );
    }
    else
    {
        if ( sbBuffer.capacity() <= kv->value.size() )
        {
            BLOG_W( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
            BLOG_W( "  bot_names value is too long (max %d characters), truncating.", iMainBufferSize );
        }
        sbBuffer = kv->value;
        good::escape(sbBuffer);
        good::split((good::string)sbBuffer, CMod::aBotNames, ',', true);

        BLOG_D("Bot names:");
        for ( int i = 0; i < CMod::aBotNames.size(); ++i )
            BLOG_D( "  %s", CMod::aBotNames[i].c_str() );
    }

}


//----------------------------------------------------------------------------------------------------------------
void CConfiguration::LoadModVars()
{
    // Get mod vars section, i.e. [HalfLife2Deathmatch.items.vars].
    good::string_buffer sbBuffer( szMainBuffer, iMainBufferSize, false );
    sbBuffer = CMod::sModName;
    sbBuffer << ".vars";

    good::ini_file::const_iterator it = m_iniFile.find( sbBuffer );
    if ( it == m_iniFile.end() )
    {
        BLOG_W( "File \"%s\", no section [%s], using defaults.", m_iniFile.name.c_str(), sbBuffer.c_str() );
        return;
    }

    // Iterate throught section key values.
    bool aFound[ EModVarTotal ] = { false };
    good::vector<float> aVars[ EModVarTotal ];

    for ( good::ini_section::const_iterator itemIt = it->begin(); itemIt != it->end(); ++itemIt )
    {
        TModVar iVar = CTypeToString::ModVarFromString( itemIt->key );
        if ( iVar == -1 )
        {
            BLOG_W( "File \"%s\", section [%s], no such variable \"%s\".", m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str() );
            continue;
        }

        aFound[ iVar ] = true;

        // Get var values (for each class).
        StringVector aArguments;
        good::split( itemIt->value, aArguments, ',', true );

        if ( aArguments.size() != 1 && CMod::aClassNames.size() != aArguments.size() )
        {
            BLOG_W( "File \"%s\", section [%s], invalid arguments for %s = %s (should have same parameters quantity as classes names).", 
                    m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(), itemIt->value.c_str() );
            continue;
        }

        for ( int i = 0; i < aArguments.size(); ++i )
        {
            float fValue = 0;
            if ( sscanf( aArguments[ i ].c_str(), "%f", &fValue ) != 1 )
            {
                BLOG_W( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                BLOG_W( "  Invalid argument %s for %s, should be float value.", aArguments[ i ].c_str(), itemIt->key.c_str() );
            }
            else
            {
                aVars[ iVar ].push_back( fValue );
            }
        }
    }

    for ( int i = 0; i < EModVarTotal; ++i )
        if ( !aFound[ i ] )
            BLOG_W( "File \"%s\", section [%s]: missing var %s, using default.", m_iniFile.name.c_str(), it->name.c_str(), 
                    CTypeToString::ModVarToString( i ).c_str() );

    CMod::SetVars( aVars );
}


//----------------------------------------------------------------------------------------------------------------
void CConfiguration::LoadItemClasses()
{
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
    sbBuffer = CMod::sModName;

    // Load health /armor / object entity classes.
    for ( TItemType iType = 0; iType < EItemTypeKnownTotal; ++iType )
    {
        // TODO: shouldn't load weapons/ammo.
        // Get mod item section, i.e. [HalfLife2Deathmatch.items.health].
        sbBuffer.erase( CMod::sModName.size() );
        sbBuffer << ".items.";
        sbBuffer <<  CTypeToString::EntityTypeToString(iType);

        good::ini_file::const_iterator it = m_iniFile.find( sbBuffer );
        if ( it == m_iniFile.end() )
            continue;

        // Iterate throught key values.
        for ( good::ini_section::const_iterator itemIt = it->begin(); itemIt != it->end(); ++itemIt )
        {
            // String instances will live until plugin is unloaded.
            CItemClass cEntityClass;
            cEntityClass.sClassName.assign( itemIt->key.c_str(), itemIt->key.size() );

            // Get item flags.
            StringVector aArguments;
            good::split(itemIt->value, aArguments, ',', true);
            for ( int i=0; i < aArguments.size(); ++i )
            {
                StringVector aCurrent;
                good::split<good::vector>(aArguments[i], aCurrent);

                int iFlag = CTypeToString::EntityClassFlagsFromString(aCurrent[0]);

                if ( iFlag == -1 )
                {
                    BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                    BLOG_E( "  Invalid entity flag %s for %s.",  aArguments[i].c_str(), itemIt->key.c_str());
                }
                else
                {
                    FLAG_SET(iFlag, cEntityClass.iFlags);
                    /*if ( iFlag == FEntityRespawnable ) // Check respawn time.
                    {
                        if ( aCurrent.size() == 2 )
                        {
                            int iValue = -1;
                            sscanf(aCurrent[1].c_str(), "%d", &iValue);
                            if ( iValue > 0 )
                                SET_2ND_WORD(iValue, cEntityClass.iFlags);
                            else
                                BLOG_E("File \"%s\", section [%s], invalid respawn time for: %s.",  m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str());
                        }
                        else if ( aCurrent.size() > 2 )
                            BLOG_E("File \"%s\", section [%s], invalid parameters count for: %s.",  m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str());
                    }*/
                }
            }

            CItems::AddItemClassFor( iType, cEntityClass );
        }
    }

    // Load object models.
    sbBuffer = CMod::sModName;
    sbBuffer << ".items.object.models";
    good::ini_file::const_iterator it = m_iniFile.find( sbBuffer );
    if ( it != m_iniFile.end() )
    {
        // Iterate throught key values.
        for ( good::ini_section::const_iterator itemIt = it->begin(); itemIt != it->end(); ++itemIt )
        {
            int iValue = CTypeToString::EntityClassFlagsFromString(itemIt->value);
            if ( iValue > 0 )
                CItems::SetObjectFlagForModel(iValue, itemIt->key);
            else
            {
                BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                BLOG_E( "  Invalid object model flag: %s.", itemIt->value.c_str() );
                BLOG_E( "  Can be one of: %s", CTypeToString::EntityClassFlagsToString(FEntityAll).c_str() );
            }
        }
    }
}


//----------------------------------------------------------------------------------------------------------------
void CConfiguration::LoadWeapons( good::ini_file::const_iterator it )
{
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
    CWeapons::Clear();

    // Iterate throught key-values of weapons section.
    BLOG_D("Weapons:");
    for ( good::ini_section::const_iterator itemIt = it->begin(); itemIt != it->end(); ++itemIt )
    {
        sbBuffer = itemIt->value;
        good::escape(sbBuffer);

        StringVector aParams;
        good::split((good::string)sbBuffer, aParams, ',', true);
        good::vector<good::string> aAmmos[2];
        good::vector<int> aAmmoBullets[2];

        CWeapon* pWeapon = new CWeapon();
        StringVector aCurrent;
        aCurrent.reserve(4);

        bool bError = false;
        int iSecondary = 0;
        for ( StringVector::iterator paramsIt = aParams.begin(); paramsIt != aParams.end(); ++paramsIt )
        {
            int iValue = -1;
            bool bProcessed = true;

            aCurrent.clear();
            good::split(*paramsIt, aCurrent);
            BASSERT( aCurrent.size() > 0, exit(1) );

            if ( aCurrent[0] == "class" )
            {
                if ( aCurrent.size() > 1 )
                {
                    for ( int i=1; i<aCurrent.size(); ++i )
                    {
                        TClass iClass = CTypeToString::ClassFromString(aCurrent[i]);
                        if ( iClass == -1 )
                        {
                            BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                            BLOG_E( "  Weapon %s, invalid class: %s.", itemIt->key.c_str(), aCurrent[1].c_str() );
                            bError = true;
                            break;
                        }
                        FLAG_SET(1 << iClass, pWeapon->iClass);
                    }
                    if ( bError )
                        break;
                }
                else
                {
                    BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                    BLOG_E( "  Weapon %s, class not specified.", itemIt->key.c_str() );
                    bError = true;
                    break;
                }
            }
            else if ( aCurrent.size() == 1 )
            {
                if ( aCurrent[0] == "secondary" )
                    iSecondary = CWeapon::SECONDARY;
                else
                {
                    TWeaponFlags iFlag = CTypeToString::WeaponFlagsFromString(aCurrent[0]);
                    if ( iFlag == -1 )
                        bProcessed = false;
                    else
                        FLAG_SET(iFlag, pWeapon->iFlags[iSecondary]);
                }
            }
            else if ( aCurrent.size() == 2 )
            {
                if ( aCurrent[0] == "type" )
                {
                    int iType = CTypeToString::WeaponTypeFromString(aCurrent[1]);
                    if ( iType == -1 )
                    {
                        BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                        BLOG_E( "  Weapon %s, invalid type: %s.", itemIt->key.c_str(), aCurrent[1].c_str() );
                        bError = true;
                        break;
                    }
                    pWeapon->iType = iType;

                    // Set weapon default parameters.
                    switch (iType)
                    {
                    case EWeaponMelee:
                    case EWeaponPhysics:
                        pWeapon->iAttackBullets[0] = pWeapon->iAttackBullets[1] = 0;
                        break;
                    case EWeaponRocket:
                    case EWeaponGrenade:
                    case EWeaponRemoteDetonation:
                        pWeapon->iClipSize[0] = 1;
                        break;
                    }
                }
                else if ( aCurrent[0] == "preference" )
                {
                    iValue = CTypeToString::PreferenceFromString(aCurrent[1]);
                    if ( iValue == -1 )
                        bProcessed = false;
                    else
                        pWeapon->iBotPreference = iValue;
                }
                else if ( aCurrent[0] == "team" )
                {
                    iValue = CMod::GetTeamIndex(aCurrent[1]);
                    if ( iValue == -1 )
                    {
                        BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                        BLOG_E( "  Weapon %s, invalid team: %s.", itemIt->key.c_str(), aCurrent[1].c_str() );
                        bError = true;
                        break;
                    }
                    pWeapon->iTeam = 1 << iValue;
                }
                else if ( aCurrent[0] == "aim" )
                {
                    TWeaponAim iAim = CTypeToString::WeaponAimFromString(aCurrent[1]);
                    if ( iAim == -1 )
                        bProcessed = false;
                    else
                        pWeapon->iAim[iSecondary] = iAim;
                }
                else
                {
                    sscanf(aCurrent[1].c_str(), "%d", &iValue);
                    if ( iValue < 0 )
                    {
                        BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                        BLOG_E( "   Weapon %s, invalid number: %s for parameter %s.", itemIt->key.c_str(), aCurrent[1].c_str(), aCurrent[0].c_str() );
                        bError = true;
                        break;
                    }

                    if ( aCurrent[0] == "clip" )
                        pWeapon->iClipSize[iSecondary] = iValue;

                    else if ( aCurrent[0] == "damage" )
                        pWeapon->fDamage[iSecondary] = iValue;

                    else if ( aCurrent[0] == "delay" )
                        pWeapon->fShotTime[iSecondary] = iValue / 1000.0f;

                    else if ( aCurrent[0] == "hold" )
                        pWeapon->fHoldTime[iSecondary] = iValue / 1000.0f;

                    else if ( aCurrent[0] == "reload_by" )
                        pWeapon->iReloadBy[iSecondary] = iValue;

                    else if ( aCurrent[0] == "reload" )
                        pWeapon->fReloadTime[iSecondary] = iValue / 1000.0f;

                    else if ( aCurrent[0] == "reload_start" )
                        pWeapon->fReloadStartTime[iSecondary] = iValue / 1000.0f;

                    else if ( aCurrent[0] == "holster" )
                        pWeapon->fHolsterTime = iValue / 1000.0f;

                    else if ( aCurrent[0] == "default_ammo" )
                        pWeapon->iDefaultAmmo[iSecondary] = iValue;

                    else if ( aCurrent[0] == "max_ammo" )
                        pWeapon->iMaxAmmo[iSecondary] = iValue;

                    else if ( aCurrent[0] == "bullets" )
                        pWeapon->iAttackBullets[iSecondary] = iValue;

                    else if ( aCurrent[0] == "default_ammo" )
                        pWeapon->iDefaultAmmo[iSecondary] = iValue;

                    else if ( aCurrent[0] == "zoom_distance" )
                        pWeapon->fMinDistanceSqr[1] = SQR(iValue);

                    else if ( aCurrent[0] == "zoom_time" )
                        pWeapon->fShotTime[1] = iValue / 1000.0f;

                    else
                        bProcessed = false;
                }
            }
            else if ( aCurrent.size() == 3 )
            {
                if ( aCurrent[0] == "ammo" )
                {
                    aAmmos[iSecondary].reserve(4);
                    int iValue = -1;
                    sscanf(aCurrent[2].c_str(), "%d", &iValue);
                    if ( iValue <= 0 ) // Ammo count can't be 0.
                    {
                        BLOG_E( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                        BLOG_E( "  Weapon %s, invalid parameter for '%s' ammo's count: %s.",
                                itemIt->key.c_str(), aCurrent[1].c_str(), aCurrent[2].c_str());
                        bError = true;
                        break;
                    }

                    good::string sAmmo(aCurrent[1], true);
                    aAmmos[iSecondary].push_back(sAmmo);
                    aAmmoBullets[iSecondary].push_back(iValue);
                }
                else
                {
                    int iValue1 = -1, iValue2 = -1;
                    sscanf(aCurrent[1].c_str(), "%d", &iValue1);
                    sscanf(aCurrent[2].c_str(), "%d", &iValue2);
                    if ( (iValue1 < 0) || (iValue2 < 0) || (iValue1 >= CUtil::iMaxMapSize) || (iValue1 >= CUtil::iMaxMapSize) )
                        bProcessed = false;
                    else
                    {
                        if ( aCurrent[0] == "parabolic" )
                        {
                            pWeapon->iParabolicDistance0[iSecondary] = iValue1;
                            pWeapon->iParabolicDistance45[iSecondary] = iValue2;
                        }
                        else if ( aCurrent[0] == "range" )
                        {
                            pWeapon->fMinDistanceSqr[iSecondary] = SQR(iValue1);
                            if ( iValue2 == 0 )
                                pWeapon->fMaxDistanceSqr[iSecondary] = CUtil::iMaxMapSizeSqr;
                            else
                                pWeapon->fMaxDistanceSqr[iSecondary] = SQR(iValue2);
                        }
                        else
                            bProcessed = false;
                    }
                }
            }
            else
                bProcessed = false;

            if ( !bProcessed )
            {
                BLOG_W( "File \"%s\", section [%s]:", m_iniFile.name.c_str(), it->name.c_str() );
                BLOG_W( "  Weapon %s, unknown keyword %s or invalid parameters, skipping.",
                        itemIt->key.c_str(), aCurrent[0].c_str() );
            }
        }

        if ( bError )
            delete pWeapon;
        else
        {
			pWeapon->iId = CWeapons::Size();
			BLOG_D( "  %s, id %d", itemIt->key.c_str(), pWeapon->iId );

            if ( pWeapon->iTeam )
            {
                BLOG_D( "    team %s", CTypeToString::TeamFlagsToString(pWeapon->iTeam).c_str() );
                //if ( FLAGS_SOME_SET(FDeathmatchTeamAllWeapons, CMod::iDeathmatchFlags) )
                pWeapon->iTeam |= 1 << CMod::iUnassignedTeam;
            }
            else
                pWeapon->iTeam = -1; // Mark to use by any flag.

            if ( CMod::aClassNames.size() )
                BLOG_D( "    class %s", CTypeToString::ClassFlagsToString(pWeapon->iClass).c_str() );
            else
                pWeapon->iClass = -1; // Mark to use by any flag.

            // If reload_by is not specified, assume reload refill the clip.
            for ( int i=0; i < 2; ++i )
                if ( pWeapon->iReloadBy[i] == 0 )
                    pWeapon->iReloadBy[i] = pWeapon->iClipSize[i];

            // Add weapon class.
            CItemClass cEntityClass;
            cEntityClass.sClassName = itemIt->key;
            pWeapon->pWeaponClass = CItems::AddItemClassFor( EItemTypeWeapon, cEntityClass );

            // Add ammo classes.
            pWeapon->aAmmos[0].reserve(aAmmos[0].size());
            pWeapon->aAmmos[1].reserve(aAmmos[1].size());
            for ( int iSec = CWeapon::PRIMARY; iSec <= CWeapon::SECONDARY; ++iSec )
                for ( int i = 0; i < aAmmos[iSec].size(); ++i )
                {
                    const good::string& sAmmo = aAmmos[iSec][i];
                    const CItemClass* pAmmoClass = CItems::GetItemClass( EItemTypeAmmo, sAmmo );
                    if ( !pAmmoClass )
                    {
                        CItemClass cAmmoClass;
                        cAmmoClass.sClassName = sAmmo;
                        pAmmoClass = CItems::AddItemClassFor( EItemTypeAmmo, cAmmoClass );
                    }
                    pWeapon->aAmmos[iSec].push_back( pAmmoClass );
                    pWeapon->aAmmoBullets[iSec].push_back( aAmmoBullets[iSec][i] );
                    BLOG_D( "    ammo %s (%u bullets)",
                            pWeapon->aAmmos[iSec][i]->sClassName.c_str(), pWeapon->aAmmoBullets[iSec][i] );
                }

            CWeaponWithAmmo cWeapon(pWeapon);
            CWeapons::Add(cWeapon);
        }
    }
}
