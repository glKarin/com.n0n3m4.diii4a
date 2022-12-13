#ifndef __PREY_AI_CRAWLER_H__
#define __PREY_AI_CRAWLER_H__

/***********************************************************************
  hhCrawler.
	Crawler AI.
***********************************************************************/
class hhCrawler : public hhMonsterAI {

public:	
	CLASS_PROTOTYPE(hhCrawler);

public:
	void				Spawn();

protected:
	void				Event_Touch( idEntity *other, trace_t *trace );
	void				Pickup( hhPlayer *player );
	bool				GiveToPlayer( hhPlayer* player );
	void				Think();
	bool				UpdateAnimationControllers();
};

#endif //__PREY_AI_CRAWLER_H__