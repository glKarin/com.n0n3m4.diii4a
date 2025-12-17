#ifdef BOTRIX_CHAT

#include <stdlib.h> // rand()

#include <good/string_buffer.h>

#include "chat.h"
#include "players.h"
#include "source_engine.h"
#include "type2string.h"


//----------------------------------------------------------------------------------------------------------------
extern char* szMainBuffer;
extern int iMainBufferSize;

good::string sComma(",");


//----------------------------------------------------------------------------------------------------------------
const good::string& PhraseToString( const CPhrase& cPhrase )
{
    static good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
    sbBuffer.erase();

    bool bPrevOptional = false;
    for ( int iWord = 0; iWord < cPhrase.aWords.size(); ++iWord )
    {
        const CPhraseWord& cWord = cPhrase.aWords[iWord];

        if ( cWord.bOptional && !bPrevOptional )
            sbBuffer << '(');
        if ( cWord.bCommaBefore )
            sbBuffer << ", ");
        sbBuffer <<  cWord.sWord );
        if ( cWord.bCommaAfter )
            sbBuffer << ',');
        if ( cWord.bOptional && !cWord.bNextOptional )
            sbBuffer << ')');

        sbBuffer << ' ');
        bPrevOptional = cWord.bOptional && cWord.bNextOptional;	// Check if optional belongs to same chunk.
    }

    sbBuffer[sbBuffer.size()-1] = cPhrase.chrPhraseEnd;
    return sbBuffer;
}



//----------------------------------------------------------------------------------------------------------------
TChatVariable CChat::iPlayerVar = EChatVariableInvalid;

good::vector<CPhrase> CChat::m_aMatchPhrases[EBotChatTotal]; // Phrases for commands used for matching.
good::vector<CPhrase> CChat::m_aPhrases[EBotChatTotal];      // Phrases for commands used for generation of commands.

good::vector<StringVector> CChat::m_aSynonims;               // Available synonims.

StringVector CChat::m_aVariables;                            // Available variable names ($player, $door, $button, etc).
good::vector<StringVector> CChat::m_aVariableValues;         // Available variable values (1, 2, opened, closed, weapon_...).


//----------------------------------------------------------------------------------------------------------------
void CChat::Init()
{
    iPlayerVar = EChatVariableInvalid;
}

//----------------------------------------------------------------------------------------------------------------
void CChat::Terminate()
{
    iPlayerVar = EChatVariableInvalid;
}

//----------------------------------------------------------------------------------------------------------------
const good::string& CChat::GetSynonim( const good::string& sSentence )
{
    for ( int iWord = 0; iWord < m_aSynonims.size(); ++iWord )
    {
        StringVector& aSynonims = m_aSynonims[iWord];

        // Check if sentence starts with some of this synonims.
        for ( int iSynonim = 0; iSynonim < aSynonims.size(); ++iSynonim )
        {
            const good::string& sSynonim = aSynonims[iSynonim];
            bool bBigger = sSentence.size() >  sSynonim.size();
            bool bEqual  = sSentence.size() == sSynonim.size();
            if ( ( (bBigger && (sSentence[sSynonim.size()] == ' ')) || bEqual ) && sSentence.starts_with(sSynonim) )
                return sSynonim;
        }
    }
    return sSentence;
}


//----------------------------------------------------------------------------------------------------------------
void CChat::AddSynonims( const good::string& sKey, const good::string& sValue )
{
    StringVector aSynonims;
    good::string sFirst = sKey.duplicate();
    sFirst.lower_case();

    aSynonims.push_back(sFirst);
    good::string sOthers(sValue, true);
    sOthers.escape();
    sOthers.lower_case();
    sOthers.split<good::vector>(aSynonims, '.', true);

    BLOG_D( "  %s", CTypeToString::StringArrayToString(aSynonims).c_str() );

    m_aSynonims.push_back(aSynonims);
}


//----------------------------------------------------------------------------------------------------------------
bool CChat::AddChat( const good::string& sKey, const good::string& sValue )
{
    good::string sCommandName = sKey.duplicate();
    sCommandName.escape();
    sCommandName.lower_case();

    // Get command from key.
    int iCommand = CTypeToString::BotCommandFromString(sCommandName);
    if ( iCommand == -1 )
    {
        BLOG_E("Error, unknown bot chat: %s.", sKey.c_str());
        return false;
    }

    good::string_buffer sbCommands(szMainBuffer, iMainBufferSize, false);

    sbCommands = sValue;
    sbCommands.escape();
    sbCommands.lower_case();
    if ( !sbCommands.ends_with('.') )
        sbCommands.append('.'); // Force to end with '.'

    good::vector<CPhrase> aPhrases;
    good::vector<CPhrase> aMatchPhrases;

    // Example:  1 2 3/4 <5 6>/<7 8 (9)>
    int iBegin = 0, iFirstPhrase = 0;          // First frase index, for multiples '.'
    int iWordsBeforeParallelInFirstPhrase = 0; // When found '/' calculate how much words must be copied. In given example: =2 between 3-4 and =3 at 6 .
    int iWordsBeforeParallel = 0;              // Words before '/'. =3 for first '/', then it is reset, and it is 2 for the second '/'.
    int iWordsAfterParallel = 1;               // Words after '/'.
    int iParallelCount = 0;                    // How much phrases are now in parallel. In example =1 at 4 and =2 at 8.
    bool bCanBreak = true, bOptional = false, bPreviousParenthesis = false, bCommaBeforeWord = false;
    bool bParallel = false, bEnd = false;
    char cPrevChar = 0;

    for ( int iPos = 0; iPos < sbCommands.size(); ++iPos )
    {
        bool bQuestion = false, bExclamation = false;
        bool bWord = false, bComma = false;

        char cChar = sbCommands[iPos];

        // Before word is added to phrases.
        switch ( cChar ) // TODO: symbols from ini file.
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            bWord = true;
            break;

        case ',':
        case ';':
            bWord = bComma = true;
            break;

        case '.':
        case '?':
        case '!':
            bWord = bEnd = true;
            if ( !bCanBreak )
            {
                BLOG_E("Error for '%s' command: sentence can't end inside <>", sKey.c_str());
                return false;
            }
            if ( bParallel && (iWordsAfterParallel == 0) && (iPos-iBegin <= 0) )
            {
                BLOG_E("Error for '%s' command: sentence can't end at '/'", sKey.c_str());
                return false;
            }
            bExclamation = (sbCommands[iPos] == '!');
            bQuestion = (sbCommands[iPos] == '?');
            break;

        case '(':
            if ( bCanBreak && bParallel && (iWordsAfterParallel == 0) )
            {
                BLOG_E("Error for '%s' command: word can't be optional after separator.", sKey.c_str());
                return false;
            }
            bWord = true;
            break;

        case ')':
            bWord = true;
            break;

        case '<':
            if ( !bCanBreak )
            {
                BLOG_E("Error for '%s' command: invalid <> secuence.", sKey.c_str());
                return false;
            }
            bWord = true;
            break;

        case '>':
            if ( bCanBreak )
            {
                BLOG_E("Error for '%s' command: invalid <> secuence.", sKey.c_str());
                return false;
            }
            bWord = true;
            break;

        case '/':
            if ( !bCanBreak )
            {
                BLOG_E("Error for '%s' command: '/' can't occur inside <> secuence.", sKey.c_str());
                return false;
            }
            bWord = true;
            break;

        default:
            break;
        }

        if ( bWord )
        {
            if ( bParallel && (cPrevChar == '>') && (cChar != '/') )
                bParallel = false;

            // We have a separated word.
            if ( iPos-iBegin > 0 )
            {
                //if ( bParallel && bCanBreak && (cChar == '/') && (iWordsAfterParallel > 0) )
                //	bParallel = false;

                // Get word from string.
                good::string sWord = sbCommands.substr(iBegin, iPos - iBegin);

                // Check for synonims in matching phrases.
                good::string sMatchWord(sWord, true);
                sMatchWord.lower_case();
                const good::string& sSynonim = GetSynonim(sMatchWord);

                if ( aPhrases.size() == iFirstPhrase ) // Add new phrase after '.' character.
                {
                    aPhrases.push_back(CPhrase());
                    aMatchPhrases.push_back(CPhrase());
                }

                int iFirst = iFirstPhrase;
                int iLast = aPhrases.size();
                if ( bParallel )
                    iFirst = aPhrases.size() - iParallelCount;
                for ( int i = iFirst; i < iLast; ++i )
                {
                    aPhrases[i].aWords.push_back( CPhraseWord(sWord.duplicate(), bOptional, bOptional, bCommaBeforeWord, false ) );
                    aMatchPhrases[i].aWords.push_back( CPhraseWord(sSynonim.duplicate(), bOptional, bOptional, bCommaBeforeWord, false ) );
                }
                bCommaBeforeWord = bPreviousParenthesis = false;

                // Check if parallel processing can be terminated.
                if ( bCanBreak )
                {
                    iWordsBeforeParallel = 1;
                    if ( bParallel && (cChar != '/') )
                        bParallel = false;
                }
                else
                {
                    iWordsBeforeParallel++;
                    if ( bParallel )
                        iWordsAfterParallel++;
                }
            }
            iBegin = iPos+1;
        }

        if ( bComma )
        {
            if ( !bCommaBeforeWord )
            {
                int iFirst = iFirstPhrase;
                int iLast = aPhrases.size();
                if ( bParallel )
                {
                    iFirst = aPhrases.size() - iParallelCount;
                }
                for ( int i = iFirst; i < iLast; ++i )
                {
                    aPhrases[i].aWords.back().bCommaAfter = true;
                    aMatchPhrases[i].aWords.back().bCommaAfter = true;
                }
            }
            else
            {
                bCommaBeforeWord = bPreviousParenthesis;
                bPreviousParenthesis = false;
            }
        }

        // After word is added.
        switch ( cChar )
        {
        case '<':
            if ( bParallel && (iWordsAfterParallel > 0) )
                bParallel = false;
            iWordsBeforeParallel = iWordsAfterParallel = 0;
            bCanBreak = false;
            break;
        case '>':
            bPreviousParenthesis = false;
            bCanBreak = true;
            break;
        case '(':
            bOptional = bPreviousParenthesis = true;
            break;
        case ')':
            bPreviousParenthesis = bOptional = false;
            for ( int iPhrase = iFirstPhrase; iPhrase < aPhrases.size(); ++iPhrase )
            {
                aPhrases[iPhrase].aWords.back().bNextOptional = false;
                aMatchPhrases[iPhrase].aWords.back().bNextOptional = false;
            }
            break;
        case '/':
            {
                if ( !bParallel )
                {
                    bParallel = true;
                    iParallelCount = aPhrases.size() - iFirstPhrase;
                    if ( iParallelCount == 0 )
                    {
                        iParallelCount = 1;
                        iWordsBeforeParallelInFirstPhrase = 0;
                    }
                    else
                        iWordsBeforeParallelInFirstPhrase = aPhrases[iFirstPhrase].aWords.size() - iWordsBeforeParallel;
                }

                bPreviousParenthesis = false;

                // Copy all phrases we have so far.
                int iWordsBeforeSeparator = aPhrases[iFirstPhrase].aWords.size() - iWordsBeforeParallelInFirstPhrase;
                if ( iWordsBeforeSeparator == 0 ) // Case when there is a space before separator.
                    iWordsBeforeSeparator = 1;// TODO: error.

                for ( int iPhrase = iFirstPhrase; iPhrase < iFirstPhrase + iParallelCount; ++iPhrase )
                {
                    CPhrase& cPhrase = aPhrases[iPhrase];
                    CPhrase& cMatchPhrase = aMatchPhrases[iPhrase];

                    CPhrase& cPhraseCopy = *aPhrases.insert( aPhrases.end(), CPhrase() );
                    CPhrase& cMatchPhraseCopy = *aMatchPhrases.insert( aMatchPhrases.end(), CPhrase() );

                    for ( int iWord = 0; iWord < cPhrase.aWords.size() - iWordsBeforeSeparator; ++iWord )
                    {
                        CPhraseWord& cWord = cPhrase.aWords[iWord];
                        CPhraseWord cCopy = CPhraseWord(cWord.sWord.duplicate(), cWord.bOptional, cWord.bNextOptional, cWord.bCommaBefore, cWord.bCommaAfter);
                        cPhraseCopy.aWords.push_back( cCopy );
                        CPhraseWord cMatchCopy = CPhraseWord(cMatchPhrase.aWords[iWord].sWord.duplicate(), cWord.bOptional, cWord.bNextOptional, cWord.bCommaBefore, cWord.bCommaAfter);
                        cMatchPhraseCopy.aWords.push_back( cMatchCopy );
                    }
                }

                iWordsBeforeParallel = iWordsAfterParallel = 0;
                break;
            }
        }

        if ( bEnd )
        {
            if ( bParallel && (iWordsAfterParallel == 0) )
            {
                BLOG_E("Error for '%s' command: sentence can't end in separator.", sKey.c_str());
                return false;
            }
            for ( int iPhrase = iFirstPhrase; iPhrase < aPhrases.size(); ++iPhrase )
            {
                aPhrases[iPhrase].aWords.back().bNextOptional = false;
                aMatchPhrases[iPhrase].aWords.back().bNextOptional = false;

                aPhrases[iPhrase].chrPhraseEnd = sbCommands[iPos];
                aMatchPhrases[iPhrase].chrPhraseEnd = sbCommands[iPos];
            }
            iFirstPhrase = aPhrases.size();
            bParallel = bPreviousParenthesis = false;
            iWordsBeforeParallelInFirstPhrase = iParallelCount = 0;
            bEnd = false;
        }

        cPrevChar = cChar;
    }

#if defined(DEBUG) || defined(_DEBUG)
    // Show command name.
    BLOG_D( "  %s:", sCommandName.c_str() );

    for ( int iPhrase = 0; iPhrase < aPhrases.size(); ++iPhrase )
    {
        BLOG_D("    To generate: %s", PhraseToString(aPhrases[iPhrase]).c_str() );
        //BLOG_D("    To match:    %s", PhraseToString(aMatchPhrases[iPhrase]).c_str() );
    }
#endif

    m_aPhrases[iCommand] = aPhrases;
    m_aMatchPhrases[iCommand] = aMatchPhrases;

    return true;
}


//----------------------------------------------------------------------------------------------------------------
float CChat::ChatFromText( const good::string& sText, CBotChat& cCommand )
{
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);

    sbBuffer = sText;
    sbBuffer.trim();
    sbBuffer.lower_case();

    cCommand.iBotChat = EBotChatUnknown;
    cCommand.iDirectedTo = -1;
    if ( sbBuffer.size() == 0 )
        return 0.0f;

    if ( !sbBuffer.ends_with('.') && !sbBuffer.ends_with('!') && !sbBuffer.ends_with('?') )
        sbBuffer << '.');

    StringVector aPhrases[1]; // TODO: multiple commands in one sentence.
    int iCurrentPhrase = 0, iBegin = 0;
    char chrEnd;

    // Replace player's names and synonims.
    for ( int iPos = 0; iPos < sbBuffer.size(); ++iPos )
    {
        bool bWord = false, bEnd = false, bComma = false;
        switch ( sbBuffer[iPos] )
        {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            bWord = true;
            break;

        case ',':  // Don't mind commas.
        case ';':
            bWord = bComma = true;
            break;

        case '.':
        case '?':
        case '!':
            bWord = bEnd = true;
            chrEnd = sbBuffer[iPos];
            break;
        }

        if ( bWord )
        {
            if ( iPos - iBegin > 0 ) // At least one character in the word.
            {
                // Check for synonims.
                szMainBuffer[iPos] = 0;
                good::string sCurr( &szMainBuffer[iBegin], false, false, iPos - iBegin );
                const good::string& sSynonim = GetSynonim(sCurr);

                aPhrases[iCurrentPhrase].push_back( good::string(sSynonim) ); // Don't copy string, we will just use it for matching.
            }
            iBegin = iPos+1;
        }

        if ( bEnd )
            break;
    }

    StringVector& aWords = aPhrases[0]; // TODO: detect 2+ commands in one sentence.

    good::vector<bool> cFounded; // To know if word is in the matching phrase.
    cFounded.resize( aWords.size() );

    CChatVariablesMap cMatchMap(4);

    if ( aWords.size() == 0 )
        return 0.0f;

    // Search in all commands for match.
    int iBestFound, iBestRequired, iBestOrdered;
    int iBestPhrase, iBestTotalRequired;
    float fBestImportance = 0.0f; // Number from 0 to 10, represents how good matching is.

    for ( TBotChat iCommand = 0; iCommand < EBotChatTotal; ++iCommand )
    {
        for ( int iPhrase = 0; iPhrase < m_aMatchPhrases[iCommand].size(); ++iPhrase )
        {
            cMatchMap.clear();
            int iFound = 0, iRequired = 0, iOrdered = 0, iTotalRequired = 0, iPrevPos = 0;

            // Set all cFounded to false.
            memset( cFounded.data(), false, sizeof(bool) * cFounded.size() );

            CPhrase& currPhrase = m_aMatchPhrases[iCommand][iPhrase];
            for ( int iWord = 0; iWord < currPhrase.aWords.size(); ++iWord )
            {
                const CPhraseWord& cPhraseWord = currPhrase.aWords[iWord];

                // Check if word is a chat variable.
                TChatVariable iVar = EChatVariableInvalid;
                int iVarNumber = 0;
                if ( cPhraseWord.sWord.starts_with('$') )
                {
                    good::pair<TChatVariable, int> pair = GetVariableAndIndex( cPhraseWord.sWord );
                    iVar = pair.first;
                    iVarNumber = pair.second;
                    BASSERT( iVar != EChatVariableInvalid );
                }

                // Check if word in match phrase occurres in spoken sentence.
                int iWordIndex = -1;
                if ( iVar == EChatVariableInvalid )
                {
                    for ( int i = 0; i < aWords.size(); ++i )
                        if ( !cFounded[i] && (cPhraseWord.sWord == aWords[i]) )
                        {
                            iWordIndex = i;
                            break;
                        }
                }
                else
                {
                    const StringVector& cValues = m_aVariableValues[iVar];
                    TChatVariableValue iValue = EChatVariableValueInvalid;
                    for ( int i = 0; i < aWords.size(); ++i )
                        if ( !cFounded[i] )
                            for ( StringVector::const_iterator it = cValues.begin(); it != cValues.end(); ++it )
                                if ( *it == aWords[i] )
                                {
                                    iWordIndex = i;
                                    iValue = it - cValues.begin();
                                    goto found; // Break twice.
                                }
                found:
                    if ( iValue != EChatVariableValueInvalid )
                        cMatchMap.push_back( CChatVarValue(iVar, iVarNumber, iValue) );
                }

                if ( !cPhraseWord.bOptional )
                    iTotalRequired++; // Count amount of required words in this phrase.

                if ( iWordIndex != -1 )
                {
                    iFound++;
                    cFounded[iWordIndex] = true;

                    if ( !currPhrase.aWords[iWord].bOptional )
                        iRequired++;

                    if ( iPrevPos <= iWordIndex )
                        iOrdered++;

                    iPrevPos = iWordIndex;
                }
            }

            int iOptionals = iFound-iRequired;
            int iTotalOptionals = currPhrase.aWords.size() - iTotalRequired;
            int iExtra = aWords.size() - iFound;
            //int iLeft = currPhrase.aWords.size() - iFound;

            // Give more importance to requiered words (not optionals), and less to optionals and words order.
            float fImportance = 0.0f;
            //fImportance = (float)iFound/currPhrase.aWords.size() - (float)(iExtra+iLeft)/( aWords.size() + currPhrase.aWords.size() );
            //fImportance *= 10.0f;
            fImportance += 6*( (iTotalRequired) ? iRequired / (float)iTotalRequired : 0 );    // + Ratio of matched requered words.
            fImportance += 1*( (iTotalOptionals) ? iOptionals / (float)iTotalOptionals : 1 ); // + Ratio of matched optional words.
            fImportance += 3*( iOrdered / (float)currPhrase.aWords.size() );                  // + Ratio of matched ordered words.
            //fImportance += (chrEnd == currPhrase.chrPhraseEnd) ? 1 : 0;                       // + 1 if ends with same symbol as matching phrase (. ? !).
            fImportance -= 4*( iExtra / iTotalRequired );                                     // - Ratio of not matched words.

            if ( fBestImportance < fImportance )
            {
                fBestImportance = fImportance;
                iBestFound = iFound;
                iBestRequired = iRequired;
                iBestTotalRequired = iTotalRequired;
                iBestOrdered = iOrdered;
                cCommand.iBotChat = iCommand;
                iBestPhrase = iPhrase;

                cCommand.cMap.clear();
                for ( int i=0; i < cMatchMap.size(); ++i )
                    cCommand.cMap.push_back( cMatchMap[i] );
            }
        }
    }

    if ( fBestImportance > 0.0f )
    {
        //const CPhrase& cPhrase = m_aMatchPhrases[cCommand.iBotChat][iBestPhrase];
        //BLOG_D( "Chat match: %s", PhraseToString(cPhrase).c_str() );
        //BLOG_D( "Matching (from 0 to 10): %f.", fBestImportance );

        // Get $player/$player1 from chat variables, this is where chat is directed to.
        for ( int i=0; i < cCommand.cMap.size(); ++i )
        {
            CChatVarValue& cVar = cCommand.cMap[i];
            if ( cVar.iVar == iPlayerVar )
                if ( (cVar.iVarIndex == 0) || (cVar.iVarIndex == 1) ) // $player or $player1.
                    cCommand.iDirectedTo = cVar.iValue;
        }
    }
    else
    {
        BLOG_D( "No match found." );
    }
    return fBestImportance;
}


//----------------------------------------------------------------------------------------------------------------
const good::string& CChat::ChatToText( const CBotChat& cCommand )
{
    static good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
    sbBuffer.erase();

    BASSERT( (EBotChatUnknown < cCommand.iBotChat) && (cCommand.iBotChat < EBotChatTotal), return sbBuffer );

    if ( m_aPhrases[cCommand.iBotChat].size() == 0 )
    {
        BLOG_E( "No phrases provided to generate chat message for '%s'.", CTypeToString::BotCommandToString(cCommand.iBotChat).c_str() );
        return sbBuffer; // Empty string.
    }

    // Get random phrase from possible set of phrases.
    int iRand = rand() % m_aPhrases[cCommand.iBotChat].size();
    const CPhrase& cPhrase = m_aPhrases[cCommand.iBotChat][iRand];

    BASSERT( cPhrase.aWords.size(), return sbBuffer );

    // Get phrase into buffer replacing variables by their values.
    bool bNextOptional = false, bUseOptional = false;
    for ( int i=0; i < cPhrase.aWords.size(); ++i )
    {
        const CPhraseWord& cWord = cPhrase.aWords[i];
        bool bAdd = !cWord.bOptional;

        if ( cWord.bOptional )
        {
            if ( bNextOptional ) // Previous word was optional.
            {
                bAdd = bUseOptional;
            }
            else
            {
                bUseOptional = (rand()&1); // First optional.
                if ( bUseOptional )
                {
                    bAdd = true;
                }
            }
            bNextOptional = cWord.bNextOptional;
        }

        if ( bAdd )
        {
            if ( cWord.bCommaBefore )
                sbBuffer << ", ");

            if ( cWord.sWord.starts_with('$') )
            {
                good::pair<TChatVariable, int> iVarIndex = GetVariableAndIndex( cWord.sWord );
                BASSERT( iVarIndex.first != EChatVariableInvalid );

                bool bFound = false;
                for ( int i=0; i < cCommand.cMap.size(); ++i )
                    if ( (cCommand.cMap[i].iVar == iVarIndex.first) && (cCommand.cMap[i].iVarIndex == iVarIndex.second) )
                    {
                        bFound = true;
                        sbBuffer << GetVariableValue(iVarIndex.first, cCommand.cMap[i].iValue) );
                    }
                BASSERT(bFound); // You need to add pair <var, value> to the map.
            }
            else
                sbBuffer << cWord.sWord);

            if ( cWord.bCommaAfter )
                sbBuffer << ',');
            sbBuffer << ' ');
        }
    }

    sbBuffer[sbBuffer.size() - 1] = cPhrase.chrPhraseEnd;
    sbBuffer[0] = sbBuffer[0] - 'a' + 'A'; // Uppercase first letter.
    return sbBuffer;
}


//----------------------------------------------------------------------------------------------------------------
class CAnswers
{
public:
    CAnswers()
    {
        aTalkAnswers[EBotChatGreeting].push_back(EBotChatGreeting);

        aTalkAnswers[EBotChatBye].push_back(EBotChatBye);

        aTalkAnswers[EBotChatCall].push_back(EBotChatCallResponse);
        aTalkAnswers[EBotChatCall].push_back(EBotChatGreeting);

        aTalkAnswers[EBotChatHelp].push_back(EBotChatAffirmative);
        aTalkAnswers[EBotChatHelp].push_back(EBotChatNegative);
        aTalkAnswers[EBotChatHelp].push_back(EBotChatAffirm);
        aTalkAnswers[EBotChatHelp].push_back(EBotChatNegate);

        for ( TBotChat i = EBotChatStop; i <= EBotChatJump; ++i )
        {
            aTalkAnswers[i].push_back(EBotChatAffirm);
            aTalkAnswers[i].push_back(EBotChatNegate);
        }

        aTalkAnswers[EBotChatLeave].push_back(EBotChatBye);
    };

    good::vector<TBotChat> aTalkAnswers[EBotChatTotal];
};
/*
    // EBotChatError         { -1 },
    // EBotChatGreeting      { EBotChatGreeting, EBotChatBusy, -1 },
    // EBotChatBye           { -1 },
    // EBotChatBusy          { -1 },
    // EBotChatAffirmative   { -1 },
    // EBotChatNegative      { -1 },
    // EBotChatAffirm        { -1 },
    // EBotChatNegate        { -1 },
    // EBotChatCall          { EBotChatCallResponse, EBotChatBusy, -1 },
    // EBotChatCallResponse  { -1 },
    // EBotChatHelp          { EBotChatAffirmative, EBotChatNegative, EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatStop          { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatCome          { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatFollow        { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatAttack        { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatNoKill        { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatSitDown       { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatStandUp       { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatJump          { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
    // EBotChatLeave         { EBotChatBye, -1 },
};
*/

const good::vector<TBotChat>& CChat::PossibleAnswers( TBotChat iTalk )
{
    // TODO: finish
    static CAnswers cAnswers;
    BASSERT( (0 <= iTalk) && (iTalk < EBotChatTotal) );
    return cAnswers.aTalkAnswers[iTalk];
}

#endif // BOTRIX_CHAT
