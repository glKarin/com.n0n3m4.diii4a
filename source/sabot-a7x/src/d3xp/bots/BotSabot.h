#ifndef __BOTSABOT_H__
#define __BOTSABOT_H__

/*
===============================================================================

	botSabot

===============================================================================
*/

class botSabot : public botAi {
// Variables
public:

// Functions
public:
	CLASS_PROTOTYPE( botSabot );

							botSabot();
							~botSabot();

	void					Think( void );
};

#endif /* !__BOTSABOT_H__ */