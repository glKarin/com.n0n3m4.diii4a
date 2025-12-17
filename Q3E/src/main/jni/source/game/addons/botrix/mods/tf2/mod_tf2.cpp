#ifdef BOTRIX_TF2


#include <good/string_buffer.h>

#include "mods/tf2/bot_tf2.h"
#include "mods/tf2/mod_tf2.h"


extern char* szMainBuffer;
extern int iMainBufferSize;


//----------------------------------------------------------------------------------------------------------------
CPlayer* CModTF2::AddBot( const char* szName, TBotIntelligence iIntelligence, TTeam iTeam,
                              TClass iClass, int iParamsCount, const char **aParams )
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
        m_sLastError = "Error, can't add bot (no map or server full?).";
        return NULL;
    }

    return new CBot_TF2(pEdict, iIntelligence, iTeam, iClass);
}


#endif // BOTRIX_TF2
