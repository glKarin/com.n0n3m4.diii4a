
#pragma once

//VO categories, ordered by priority. Goes from MOST IMPORTANT to LEAST IMPORTANT, higher will override bottom.

#define VO_CATEGORY_DEATH		4
#define VO_CATEGORY_HITREACTION	3
#define VO_CATEGORY_NARRATIVE	2
#define VO_CATEGORY_BARK		1 //gameplay voice bark "I see them!" "moving in!" etc
#define VO_CATEGORY_GRUNT		0 //just a mouth sound, effort noise, breathing, etc

class idVOManager
{
public:
	idVOManager();
	~idVOManager();

	int		SayVO(idEntity *speaker, const char *soundname, int lineCategory);


//private:
	
};