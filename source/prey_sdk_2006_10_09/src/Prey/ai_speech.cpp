//
// ai_speech.cpp
//
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

extern idCVar	ai_skipSpeech;

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
//////////////////////////////////////////////////////////////////////
//
// hhAISpeech
//
//////////////////////////////////////////////////////////////////////

// jrm: none yet



//////////////////////////////////////////////////////////////////////
//
// hhAISpeechHandler
//
//////////////////////////////////////////////////////////////////////

float hhAISpeechHandler::TokenMatchWeight[]			= {3.0f, 2.0f, 1.0f};
float hhAISpeechHandler::TokenWildcardMatchWeight[] = {1.5f, 1.0f, 0.5f};
#define TokenWildCard	"*"
#define SpeechShaderPrefix "snd_speech_" // IF THIS CHANGES YOU MUST CHANGE THE LENGTH DEFINE!
#define SpeechShaderPrefixLen 11
#define FreqToken ':'
#define FreqTokenStr ":"

//
// Construction
//
hhAISpeechHandler::hhAISpeechHandler()
{
    firstSpeech		= NULL;
	lastSpeech		= NULL;
	nextSpeechTime	= 0;
};

//
// Update()
//
void hhAISpeechHandler::Update()
{
	hhAISpeech *currSpeech = firstSpeech;	
	hhAISpeech *nextSpeech = NULL;	

	if(ai_skipSpeech.GetBool())
		return;
	
	//for( currSpeech = speechQueue.Next(); currSpeech != NULL; currSpeech = nextSpeech ) 
	while(currSpeech != NULL)
	{	
		// We can't play any more speech yet! 
		if(gameLocal.GetTime() < nextSpeechTime)
			return;

		nextSpeech = currSpeech->next;
		
		// Has this speech expired? 		
		if(gameLocal.GetTime() > currSpeech->expireTime)
		{				
			RemoveSpeechFromQueue(currSpeech);
			currSpeech = nextSpeech;
			continue;
		}		
		
		// Play the speech!
		currSpeech->speaker->StartSound(currSpeech->snd, SND_CHANNEL_VOICE, 0, true, NULL);
		
		// Debug Start
		/*
		idStr speechStr;
		sprintf(speechStr, "\"%s\"", (const char*)currSpeech->snd);
		speechStr.ToLower();
		gameRenderWorld->DrawText(speechStr, currSpeech->speaker->GetOrigin() + idVec3(0.0f, 0.0f, 12.0f), 0.5f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(),1,1000);	
		gameLocal.Printf("\n%.01f, %s says %s", (float)MS2SEC(gameLocal.GetTime()), (const char*)currSpeech->speaker->name, (const char*)speechStr);
		*/
		// Debug End
		
		RemoveSpeechFromQueue(currSpeech);
		nextSpeechTime = gameLocal.GetTime() + MinSpeechWaitTime;		

		currSpeech = nextSpeech;		
	}
	

	
};

//
// PlaySpeech()
//
bool hhAISpeechHandler::PlaySpeech(idAI *source, idStr sndShd, int expireDelay, bool allowQueue)
{
	if(ai_skipSpeech.GetBool())
		return FALSE;

	if(!allowQueue) {
		// just play it and return
		HH_ASSERT(FALSE); // Code not done yet!
		return TRUE;
	}

	// Do we have more slots availble?
	hhAISpeech *freeSpeech = GetFreeSpeech();
	if(!freeSpeech)
		return FALSE;

	// Setup the new speech	
	freeSpeech->speaker		= source;
	freeSpeech->expireTime	= gameLocal.GetTime() + expireDelay;
	freeSpeech->snd			= sndShd;	
	AddSpeechToQueue(freeSpeech);
	return TRUE;
};
	
//
// ClearSpeech()
//
void hhAISpeechHandler::ClearSpeech()
{
	firstSpeech = NULL;
	lastSpeech  = NULL;

	for(int i=0;i<SpeechPoolSize;i++) {
		speechPool[i].Clear();			
	}
};
	

//
// getFreeSpeech()
//
hhAISpeech* hhAISpeechHandler::GetFreeSpeech()
{
	for(int i=0;i<SpeechPoolSize;i++) {
		if(speechPool[i].IsCleared())
			return &speechPool[i];
	}

	return NULL;
};

//
// AddSpeechToQueue()
//
void hhAISpeechHandler::AddSpeechToQueue(hhAISpeech *speech)
{
	// First entry?
    if(!firstSpeech || !lastSpeech)
	{
		// this means empty list, so they both better be empty
		HH_ASSERT(lastSpeech == NULL);
		HH_ASSERT(firstSpeech == NULL);

		speech->next = NULL;
		speech->prev = NULL;
		firstSpeech = speech;		
		lastSpeech  = speech;
		return;
	}

	// Add it to the end of the list
	HH_ASSERT(lastSpeech && lastSpeech->next == NULL);
	lastSpeech->next = speech;
	speech->prev = lastSpeech;
	speech->next = NULL;
	lastSpeech = speech;
}
	
//
// RemoveSpeechFromQueue()
//
void hhAISpeechHandler::RemoveSpeechFromQueue(hhAISpeech *speech)
{
	speech->Clear();

	// Remove from one of the ends?
	if(speech == firstSpeech || speech == lastSpeech) {
		if(speech == firstSpeech) {
			firstSpeech = firstSpeech->next;
			if(firstSpeech)
				firstSpeech->prev = NULL;
		}

		if(speech == lastSpeech) {
			lastSpeech = lastSpeech->prev;
			if(lastSpeech)
				lastSpeech->next = NULL;
		}	
		return;
	}
	
	
	// Middle?
	if(speech->next)
		speech->next->prev = speech->prev;
	if(speech->prev)
		speech->prev->next = speech->next;	
}

//
// GetSpeechFrequency()
//
float hhAISpeechHandler::GetSpeechFrequency(idStr sndShader)
{	
	if(ai_skipSpeech.GetBool())
		return 0.0f;

	int firstBreak = sndShader.Find(FreqToken);
	if(firstBreak == -1)
		return 1.0f;

	int secondBreak = sndShader.Find(FreqTokenStr, true, firstBreak);
	if(secondBreak == -1)
		secondBreak = sndShader.Length();

	idStr freqStr  = sndShader.Mid(firstBreak, secondBreak - firstBreak);
	
	int f = atoi((const char*)freqStr);
	return idMath::ClampFloat(0.0f, 1.0f, (float(f) * 0.01f));
};

//
// GetSpeechResponseDelay()
//
int hhAISpeechHandler::GetSpeechResponseDelay(idStr sndShader)
{
	// TODO: Fill this code in when we actually need it. Does 1 second work all the time?

	return 500;
};

//
// GetSpeechSoundShader()
//
bool hhAISpeechHandler::GetSpeechSoundShader(idAI *source, idStr speechDesc, idStr &outClosestSndShd)
{
	HH_ASSERT(source != NULL);	

	const idKeyValue *kv = NULL;

	if(ai_skipSpeech.GetBool())
		return FALSE;

	// Quick out - do we have this EXACT snd shader? 
	idStr sndShader = SpeechShaderPrefix + speechDesc;
	kv = source->spawnArgs.MatchPrefix(sndShader);
	if(kv) {
		outClosestSndShd = kv->GetKey();
		return TRUE;
	}

	// Extrace the tokens for our 'desired' speech pattern
	SpeechTokens desired;	
	if(!ExtractSpeechTokens(speechDesc, desired))
		return FALSE;

	// Find our best potential speech pattern match
	float bestRating = 0.0f;
	float currRating = -1.0f;
	const idKeyValue *bestKey = NULL;
	SpeechTokens currPotential;

	kv = source->spawnArgs.MatchPrefix(SpeechShaderPrefix);	

	while(kv)
	{
		idStr speechStr;		
		speechStr = kv->GetKey().Mid(SpeechShaderPrefixLen, kv->GetKey().Length() - SpeechShaderPrefixLen);

		if(!ExtractSpeechTokens(speechStr, currPotential))
			currRating = -1.0f;
		else
			currRating = GetMatchRating(desired, currPotential);

		if(currRating > bestRating)
		{
			bestRating = currRating;
			bestKey = kv;
		}

		kv = source->spawnArgs.MatchPrefix(SpeechShaderPrefix,kv); 
	}

	// We've found a match to some degree
	if(bestKey && bestRating > 0.0f)
	{
		outClosestSndShd = bestKey->GetKey();
		return TRUE;
	}

	return FALSE;
};

//
// GetMatchRating() 
//
float hhAISpeechHandler::GetMatchRating(SpeechTokens &desired, SpeechTokens &potential)
{
	float rating = 0.0f;

	for(int i=0;i<TT_Total;i++)
	{
		if(potential.tokens[i] == desired.tokens[i])
			rating += TokenMatchWeight[i];
		else if(potential.tokens[i] == TokenWildCard)
			rating += TokenWildcardMatchWeight[i];
	}
    
	return rating;
};

//
// ExtractSpeechTokens()
//
bool hhAISpeechHandler::ExtractSpeechTokens(idStr speechDesc, SpeechTokens &outTokens)
{		
	int firstBreak = speechDesc.Find('_');
	if(firstBreak < 0)
		return FALSE;

	int lastBreak  = speechDesc.Find("_", true, firstBreak+1);
	if(lastBreak < 0)
		return FALSE;
	
	outTokens.tokens[TT_Desire]	= speechDesc.Mid(0, firstBreak-1);
	outTokens.tokens[TT_How]	= speechDesc.Mid(firstBreak, lastBreak-(firstBreak));
	outTokens.tokens[TT_What]   = speechDesc.Mid(lastBreak+1, (speechDesc.Length()-1) - lastBreak);
	int findFreqBreak = outTokens.tokens[TT_What].Find(FreqToken);
	if(findFreqBreak >= 0)
		outTokens.tokens[TT_What] = outTokens.tokens[TT_What].Mid(0,  outTokens.tokens[TT_What].Length() - (findFreqBreak+1));

	return true;
};

/*
================
hhAISpeechHandler::Save
================
*/
void hhAISpeechHandler::Save(idSaveGame *savefile) const {
	for (int i = 0; i < SpeechPoolSize; i++) {
		savefile->WriteObject(speechPool[i].speaker);
		savefile->WriteString(speechPool[i].snd);
		savefile->WriteInt(speechPool[i].expireTime);
		savefile->WriteInt(GetPoolIndex(speechPool[i].next));
		savefile->WriteInt(GetPoolIndex(speechPool[i].prev));
	}

	savefile->WriteInt(GetPoolIndex(firstSpeech));
	savefile->WriteInt(GetPoolIndex(lastSpeech));
	savefile->WriteInt(nextSpeechTime);
}

/*
================
hhAISpeechHandler::Restore
================
*/
void hhAISpeechHandler::Restore(idRestoreGame *savefile) {
	int tmp;
	for (int i = 0; i < SpeechPoolSize; i++) {
		savefile->ReadObject(reinterpret_cast<idClass *&> (speechPool[i].speaker));
		savefile->ReadString(speechPool[i].snd);
		savefile->ReadInt(speechPool[i].expireTime);
		savefile->ReadInt(tmp);
		speechPool[i].next = (tmp == -1 ? NULL : &speechPool[tmp]);
		savefile->ReadInt(tmp);
		speechPool[i].prev = (tmp == -1 ? NULL : &speechPool[tmp]);
	}

	savefile->ReadInt(tmp);
	firstSpeech = (tmp == -1 ? NULL : &speechPool[tmp]);
	savefile->ReadInt(tmp);
	lastSpeech = (tmp == -1 ? NULL : &speechPool[tmp]);
	savefile->ReadInt(nextSpeechTime);
}


int hhAISpeechHandler::GetPoolIndex(hhAISpeech *obj) const {
	if (!obj) {
		return -1;
	}

	for (int i = 0; i < SpeechPoolSize; i++) {
		if (obj == &speechPool[i]) {
			return i;
		}
	}

	HH_ASSERT(!"Couldn't find pool index!");
	return -1;
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build