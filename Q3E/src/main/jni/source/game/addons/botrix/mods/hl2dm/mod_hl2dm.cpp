#ifdef BOTRIX_HL2DM


#include <good/string_buffer.h>
#include <good/string_utils.h>

#include "mods/hl2dm/bot_hl2dm.h"
#include "mods/hl2dm/mod_hl2dm.h"

#include "iplayerinfo.h"
#include "type2string.h" 


extern char* szMainBuffer;
extern int iMainBufferSize;


//----------------------------------------------------------------------------------------------------------------
CModHL2DM::CModHL2DM()
{
    m_aModels.resize( CMod::aTeamsNames.size() );
}


//----------------------------------------------------------------------------------------------------------------
bool CModHL2DM::ProcessConfig( const good::ini_file& cIni )
{
    // Find section "<mod name>.models".
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
    sbBuffer = CMod::sModName;
    sbBuffer << ".models";

    good::ini_file::const_iterator it = cIni.find( sbBuffer );
    if ( it != cIni.end() )
    {
        StringVector aModels;
        m_aModels.resize( CMod::aTeamsNames.size() );
		
		good::ini_section::const_iterator models = it->find("use models");
		if (models != it->end())
		{
			int value = CTypeToString::BoolFromString(models->value);
			CMod::bUseModels = value == 0 ? false : true;
		}

        // Get player models.
        for ( int i = 0; i < CMod::aTeamsNames.size(); ++i )
        {
            sbBuffer = "models ";
            sbBuffer << CMod::aTeamsNames[i];

            good::ini_section::const_iterator models = it->find(sbBuffer);
            if ( models != it->end() )
            {
                sbBuffer = models->value;
                good::escape(sbBuffer);
                good::split( (good::string)sbBuffer, m_aModels[i], ',', true );

                BLOG_D("Model names for team %s:", CMod::aTeamsNames[i].c_str());
                for ( int j = 0; j < m_aModels[i].size(); ++j )
                    BLOG_D( "  %s", m_aModels[i][j].c_str() );
            }
        }
    }

    return true;
}


//----------------------------------------------------------------------------------------------------------------
CPlayer* CModHL2DM::AddBot( const char* szName, TBotIntelligence iIntelligence, TTeam iTeam,
                                TClass /*iClass*/, int iParamsCount, const char **aParams )
{
    if ( iParamsCount > 0 )
    {
        good::string_buffer sb(szMainBuffer, iMainBufferSize, false);
        sb << "Unknown parameter: " << aParams[0];
        m_sLastError = sb;
        return NULL;
    }

    edict_t* pEdict = CBotrixPlugin::pBotManager->CreateBot( szName );
    if ( !pEdict )
    {
        m_sLastError = "Error, couldn't add bot (no map or server full?).";
        return NULL;
    }

	CBot_HL2DM *result = new CBot_HL2DM( pEdict, iIntelligence );
	result->ChangeModel( iTeam );
	return result;
}


#endif // BOTRIX_HL2DM
