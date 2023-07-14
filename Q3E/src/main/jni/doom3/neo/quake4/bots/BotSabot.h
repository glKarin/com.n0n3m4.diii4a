#ifndef __BOTSABOT_H__
#define __BOTSABOT_H__

#ifdef MOD_BOTS

/*
===============================================================================

	botSabot

===============================================================================
*/

class botSabot : public botAi
{
// Variables
public:

// Functions
public:
    CLASS_PROTOTYPE( botSabot );

    botSabot();
    ~botSabot();

    void					Think( void );
};

#endif

#endif /* !__BOTSABOT_H__ */
