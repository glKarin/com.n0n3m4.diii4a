#ifndef __HH_AISPEECH_H
#define __HH_AISPEECH_H


#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
//
// hhAISpeech
//
class hhAISpeech
{
public:	
	hhAISpeech()	{Clear(); next = NULL; prev = NULL; }
	
	void		Clear(void)			{speaker = NULL; expireTime = -1;}
	bool		IsCleared(void)		{return speaker == NULL;}	// Assumes speech will NEVER come from a 'null' speaker, NULL = empty		

	idAI*		speaker;
	idStr		snd;
	int			expireTime;			
	
	hhAISpeech	*next;
	hhAISpeech	*prev;
};
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

//
// hhAISpeechHandler
//
class hhAISpeechHandler
{
public:	
#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	enum
	{
		SpeechPoolSize		= 8, 
		MinSpeechWaitTime	= 2000,			// We have to wait at LEAST this long before we can play ANY more speech
	};

	hhAISpeechHandler();
	virtual ~hhAISpeechHandler() { }
	virtual void	Update();	// note: not named Think() on purpose, because we are NOT an entity	

	// Play speech from a given AI - returns true if speech was queued - DOES NOT mean it is guarenteed to play	
	bool			PlaySpeech(idAI *source, idStr sndShader, int expireDelay = 1000, bool allowQueue = true);

	// Finds the matching snd shader to the speechDesc given, as well as 0->1 of the frequency for this speech
	// Returns FALSE if no possible match could be found.
	bool			GetSpeechSoundShader(idAI *source, idStr speechDesc, idStr &outClosestSndShd);

	// From a sound shader name, return the frequency from 0->1
	float			GetSpeechFrequency(idStr sndShader);

	// From a sound shader name, return the delay in MS of how long it takes to 'comprehend' the given shader.
	// Example: Someone says "Take the shuttle!" and we want the person taking the shuttle to wait at least 1 second before they do it
	// so it looks like they comprehended the command
	int				GetSpeechResponseDelay(idStr sndShader);
	
	// Remove all queued speech
	void			ClearSpeech(); 			
	
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

protected:
	hhAISpeech		speechPool[SpeechPoolSize];		// Pool of all possible speech slots
	hhAISpeech*		GetFreeSpeech();				// Returns the first empty speech obj available in the pool	
	int				ClearExpiredSpeech();			// Clears out any expired speech in the pool	

	enum TokenType
	{
		TT_Desire = 0,		// The desire we are going to satisfy
		TT_How,				// How we are going to satisfy it
		TT_What,			// What entity we will use to satisfy it

		TT_Total,
	};

	struct SpeechTokens
	{
        idStr tokens[TT_Total];
	};

	static float TokenMatchWeight[TT_Total];
	static float TokenWildcardMatchWeight[TT_Total];

	// Returns a 0+ rating of 'how' close the potential speech matches the desired speech
	float			GetMatchRating(SpeechTokens &desired, SpeechTokens &potential);
	bool			ExtractSpeechTokens(idStr speechDesc, SpeechTokens &outTokens);

	// because I hate id's linked list
	hhAISpeech		*firstSpeech;	
	hhAISpeech		*lastSpeech;
	void			AddSpeechToQueue(hhAISpeech *speech);
	void			RemoveSpeechFromQueue(hhAISpeech *speech);
	int				GetPoolIndex(hhAISpeech *obj) const;

	int				nextSpeechTime;					// The time we are next allowed to play speech	
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};


#endif