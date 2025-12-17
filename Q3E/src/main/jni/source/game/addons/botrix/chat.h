#ifndef __BOTRIX_CHAT_H__
#define __BOTRIX_CHAT_H__

#ifdef BOTRIX_CHAT


#include "types.h"


/// Enum to represent invalid chat variable.
enum TChatVariables
{
    EChatVariableInvalid = -1,             ///< Invalid chat variable.
};
typedef int TChatVariable;                 ///< Number that represents chat variable (the one that starts with $ symbol).


/// Enum to represent invalid chat variable value.
enum TChatVariableValues
{
    EChatVariableValueInvalid = -1,        ///< Invalid chat variable value.
};
typedef int TChatVariableValue;            ///< Number that represents chat variable value.



/// Obtained value for chat variable.
/**
  * For example said sentence "hey Jonh" matches "hey, $player1" with one variable:
  * iVar = index of $player var, iVarIndex to 1 (what left of $player1 without $player) and iValue to John index (player index).
  */
class CChatVarValue
{
public:
    CChatVarValue( TChatVariable iVar, int iVarIndex, TChatVariableValue iValue ):
        iVar(iVar), iVarIndex(iVarIndex), iValue(iValue) {}

    TChatVariable iVar;
    int iVarIndex;
    TChatVariableValue iValue;
};

typedef good::vector< CChatVarValue > CChatVariablesMap; ///< Map from chat variables to their values.



/// Class that hold information about player's request.
class CBotChat
{
public:
    CBotChat( TBotChat iBotChat = EBotChatUnknown, int iSpeaker = -1, int iDirectedTo = -1 ):
        iBotChat(iBotChat), iSpeaker(iSpeaker), iDirectedTo(iDirectedTo), cMap(4) {}

    TBotChat iBotChat;                  ///< Request type.
    TPlayerIndex iSpeaker;                 ///< Index of player, that talked this request.
    TPlayerIndex iDirectedTo;              ///< Index of player, request is directed to. It is $player or $player1.
    CChatVariablesMap cMap;                ///< Map from variables to value.
};



/// Word from a phrase.
class CPhraseWord
{
public:
    CPhraseWord( good::string sWord, bool bOptional = false, bool bNextOptional = false, bool bCommaBefore = false, bool bCommaAfter = false):
        sWord(sWord), bOptional(bOptional), bNextOptional(bNextOptional), bCommaBefore(bCommaBefore), bCommaAfter(bCommaAfter) {}

    good::string sWord;                    ///< Word itself.
    //bool bVerb:1;                          ///< True, if word is verb (important).
    bool bOptional:1;                      ///< True, if word is optional in the phrase.
    bool bNextOptional:1;                  ///< True, if next word belongs to same optional subphrase in the phrase.
    bool bCommaBefore:1;                   ///< True, if there is a comma before this word. Case when comma is first in optional. Not used for matching.
    bool bCommaAfter:1;                    ///< True, if there is a comma after this word. Not used for matching.
};



/// Sentence of words.
class CPhrase
{
public:
    CPhrase(): aWords(16), chrPhraseEnd('.') {}

    good::vector<CPhraseWord> aWords;      ///< Array of words.
    char chrPhraseEnd;                     ///< One of '.', '?' or '!'.
};



/// Class that process what player said and transforms it to commands and viceversa.
class CChat
{

public: // Members.
    static TChatVariable iPlayerVar;       ///< Chat variable $player.

public: // Methods.

    static void Init();
    static void Terminate();

    /// Add synonims for a word.
    static void AddSynonims( const good::string& sKey, const good::string& sValue );

    /// Add phrase with command meaning.
    static bool AddChat( const good::string& sKey, const good::string& sValue );

    /// Get command from text, returning number from 0 to 10 which represents matching.
    static float ChatFromText( const good::string& sText, CBotChat& cCommand );

    /// Get text from chat.
    static const good::string& ChatToText( const CBotChat& cCommand );


    /// Remove all chat variable values.
    static void CleanVariableValues()
    {
        for ( int i=0; i < m_aVariableValues.size(); ++i )
            m_aVariableValues[i].clear();
    }

    /// Get chat variable index from string.
    static TChatVariable GetVariable( const good::string& sVar )
    {
        StringVector::const_iterator it = good::find(m_aVariables.begin(), m_aVariables.end(), sVar);
        return ( it == m_aVariables.end() )  ?  EChatVariableInvalid  :  ( it - m_aVariables.begin() );
    }

    /// Get chat variable index and it's index from string (i.e. for $player1 returns <index of $player, 1>).
    static good::pair<TChatVariable, int> GetVariableAndIndex( const good::string& sVarIndex )
    {
        for ( StringVector::const_iterator it = m_aVariables.begin(); it != m_aVariables.end(); ++it )
        {
            if ( sVarIndex.starts_with( *it ) )
            {
                const char* szRest = sVarIndex.c_str() + it->size();
                if ( (*szRest == 0) || ( ('0' <= *szRest) && (*szRest <= '9') ) ) // Must follow a number.
                {
                    int iIndex = atoi(szRest);
                    return good::pair<TChatVariable, int>( it - m_aVariables.begin(), iIndex );
                }
            }
        }
        return good::pair<TChatVariable, int>( EChatVariableInvalid, -1 );
    }

    /// Get variable value for variable index iVar, value index iValue.
    static const good::string& GetVariableValue( TChatVariable iVar, TChatVariableValue iValue )
    {
        return m_aVariableValues[iVar][iValue];
    }

    /// Add chat variable. Returns variable index.
    static TChatVariable AddVariable( const good::string& sVar, int iValuesSize = 0 )
    {
        if ( sVar == "$player" )
            iPlayerVar = m_aVariables.size();
        m_aVariables.push_back(sVar);
        m_aVariableValues.push_back( StringVector(iValuesSize) );
        return m_aVariables.size() - 1;
    }

    /// Add value for variable index iVar. Must be lower case to match. Returns value index.
    static TChatVariableValue AddVariableValue( TChatVariable iVar, const good::string& sValue )
    {
        StringVector& cValues = m_aVariableValues[iVar];
        cValues.push_back( sValue );
        return cValues.size() - 1;
    }

    /// Set string for given variable and value indexes. Must be lower case to match.
    static void SetVariableValue( TChatVariable iVar, TChatVariableValue iValue, const good::string& sValue )
    {
        m_aVariableValues[iVar][iValue] = sValue;
    }


    /// Get possible answers to a chat request.
    static const good::vector<TBotChat>& PossibleAnswers( TBotChat iTalk );

protected:
    static const good::string& GetSynonim( const good::string& sWord );


    static good::vector<CPhrase> m_aMatchPhrases[EBotChatTotal]; // Phrases for commands used for matching.
    static good::vector<CPhrase> m_aPhrases[EBotChatTotal];      // Phrases for commands used for generation of commands.

    static good::vector<StringVector> m_aSynonims;               // Available synonims.

    static StringVector m_aVariables;                            // Available variable names ($player, $door, $button, etc).
    static good::vector<StringVector> m_aVariableValues;         // Available variable values (1, 2, opened, closed, weapon_...).

};


#endif // BOTRIX_CHAT

#endif // __BOTRIX_CHAT_H__
