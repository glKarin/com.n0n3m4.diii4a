// Bot_goal.h
//

#define MAX_BOT_ITEM_INFOS		2
#define MAX_BOT_ITEM_INFO		256
#define MAX_BOT_LEVEL_ITEMS		256

//#define DEBUG_AI_GOAL
#ifdef RANDOMIZE
	#define UNDECIDEDFUZZY
#endif //RANDOMIZE
#define DROPPEDWEIGHT
//minimum avoid goal time
#define AVOID_MINIMUM_TIME		10
//default avoid goal time
#define AVOID_DEFAULT_TIME		30
//avoid dropped goal time
#define AVOID_DROPPED_TIME		10
//
#define TRAVELTIME_SCALE		0.01
//item flags
#define IFL_NOTFREE				1		//not in free for all
#define IFL_NOTTEAM				2		//not in team play
#define IFL_NOTSINGLE			4		//not in single player
#define IFL_NOTBOT				8		//bot should never go for this
#define IFL_ROAM				16		//bot roam goal

//a bot goal
struct bot_goal_t
{
	bot_goal_t()
	{
		Reset();
	}

	void Reset()
	{
		origin.Zero();
		areanum = 0;
		mins.Zero();
		maxs.Zero();
		entitynum = 0;
		number = 0;
		flags = 0;
		iteminfo = 0;
		framenum = -1;
	}

	int framenum;
	idVec3 origin;				//origin of the goal
	int areanum;				//area number of the goal
	idVec3 mins, maxs;			//mins and maxs of the goal
	int entitynum;				//number of the goal entity
	int number;					//goal number
	int flags;					//goal flags
	int iteminfo;				//item information
};

//location in the map "target_location"
struct maplocation_t
{
	maplocation_t()
	{
		Reset();
	}

	void Reset()
	{
		origin.Zero();
		areanum = 0;
		name = "";
		next = 0;
	}

	idVec3 origin;
	int areanum;
	idStr name;
	maplocation_t* next;
};

//camp spots "info_camp"
struct campspot_t
{
	campspot_t()
	{
		Reset();
	}

	void Reset()
	{
		origin.Zero();
		areanum = 0;
		name = "";
		range = 0;
		weight = 0;
		random = 0;
	}

	idVec3 origin;
	int areanum;
	idStr name;
	float range;
	float weight;
	float wait;
	float random;
};

struct levelitem_t
{
	levelitem_t()
	{
		Reset();
	}

	void Reset()
	{
		number = 0;
		iteminfo = 0;
		flags = 0;
		weight = 0;
		origin.Zero();
		goalorigin.Zero();
		item = 0;
		timeout = 0;
		prev = 0;
		next = 0;
	}

	idStr name;
	int number;							//number of the level item
	int iteminfo;						//index into the item info
	int flags;							//item flags
	float weight;						//fixed roam weight
	idVec3 origin;						//origin of the item
	idVec3 goalorigin;					//goal origin within the area
	//int entitynum;						//entity number
	idItem* item;
	float timeout;						//item is removed after this time
	levelitem_t* prev, * next;
};

struct iteminfo_t
{
	iteminfo_t()
	{
		Reset();
	}

	void Reset()
	{
		classname = "";
		name = "";
		model = "";
		modelindex = 0;
		type = 0;
		index = 0;
		respawntime = 0;
		mins.Zero();
		maxs.Zero();
		number = 0;
	}

	idStr classname;					//classname of the item
	idStr name;							//name of the item
	idStr model;						//model of the item
	int modelindex;						//model index
	int type;							//item type
	int index;							//index in the inventory
	float respawntime;					//respawn time
	idVec3 mins;						//mins of the item
	idVec3 maxs;						//maxs of the item
	int number;							//number of the item info
};

//
// itemconfig_t
//
struct itemconfig_t
{
	itemconfig_t()
	{
		Reset();
	}

	void Reset( void )
	{
		numiteminfo = 0;
		for( int i = 0; i < MAX_BOT_ITEM_INFO; i++ )
		{
			iteminfo[i].Reset();
		}
	}

	int numiteminfo;
	iteminfo_t iteminfo[MAX_BOT_ITEM_INFO];
};

//goal state
struct bot_goalstate_t
{
	bot_goalstate_t()
	{
		itemweightindex = NULL;
		Reset();
	}

	void Reset()
	{
		itemweightconfig = NULL;

		if( itemweightindex != NULL )
		{
			delete itemweightindex;
		}
		itemweightindex = NULL;
		client = -1;
		lastreachabilityarea = 0;
		goalstacktop = 0;

		for( int i = 0; i < MAX_AVOIDGOALS; i++ )
		{
			avoidgoals[i] = 0;
			avoidgoaltimes[i] = 0;
		}
	}

	~bot_goalstate_t()
	{
		if( itemweightindex != NULL )
		{
			delete itemweightindex;
		}

		itemweightindex = NULL;
	}

	bool InUse()
	{
		return client != -1;
	}

	weightconfig_t*			itemweightconfig;	//weight config
	int* itemweightindex;						//index from item to weight

	int client;									//client using this goal state
	int lastreachabilityarea;					//last area with reachabilities the bot was in

	bot_goal_t goalstack[MAX_GOALSTACK];		//goal stack
	int goalstacktop;							//the top of the goal stack

	int avoidgoals[MAX_AVOIDGOALS];				//goals to avoid
	float avoidgoaltimes[MAX_AVOIDGOALS];		//times to avoid the goals
};

class idBotGoalManager
{
public:
	idBotGoalManager();

	int BotSetupGoalAI( void );

	void InitLevelItems( void );
	void UpdateEntityItems( void );

	void BotPushGoal( int goalstate, bot_goal_t* goal );
	void BotPopGoal( int goalstate );
	void BotEmptyGoalStack( int goalstate );

	int BotLoadItemWeights( int goalstate, char* filename );
	void BotResetGoalState( int goalstate );

	int BotItemGoalInVisButNotVisible( int viewer, idVec3 eye, idAngles viewangles, bot_goal_t* goal );

	int BotChooseLTGItem( int goalstate, idVec3 origin, int* inventory, int travelflags );
	int BotChooseNBGItem( int goalstate, idVec3 origin, int* inventory, int travelflags, bot_goal_t* ltg, float maxtime );

	int BotTouchingGoal( idVec3 origin, bot_goal_t* goal );

	int BotAllocGoalState( int client );

	void BotGoalName( int number, char* name, int size );

	void BotFreeGoalState( int handle );
	void BotShutdownGoalAI( void );
	void BotFreeItemWeights( int goalstate );
public:
	bool BotNearGoal( idVec3 p1, idVec3 p2 );
	int BotGetTopGoal( int goalstate, bot_goal_t* goal );
	int BotGetSecondGoal( int goalstate, bot_goal_t* goal );
public:
	void BotDumpGoalStack( int goalstate );
public:
	int BotGetLevelItemGoal( int index, char* name, bot_goal_t* goal );
	void BotSetAvoidGoalTime( int goalstate, int number, float avoidtime );
	float BotAvoidGoalTime( int goalstate, int number );
	void BotResetAvoidGoals( int goalstate );
	void BotDumpAvoidGoals( int goalstate );
	void BotAddToAvoidGoals( bot_goalstate_t* gs, int number, float avoidtime );
	void BotRemoveFromAvoidGoals( int goalstate, int number );
	int BotGetMapLocationGoal( char* name, bot_goal_t* goal );
	int BotGetNextCampSpotGoal( int num, bot_goal_t* goal );
	void BotFindEntityForLevelItem( levelitem_t* li );
private:

	levelitem_t* AllocLevelItem( void );
	void FreeLevelItem( levelitem_t* li );

	void AddLevelItemToList( levelitem_t* li );
	void RemoveLevelItemFromList( levelitem_t* li );

	void BotFreeInfoEntities( void );

	void BotInitInfoEntities( void );

	void InitLevelItemHeap( void );
	int* ItemWeightIndex( weightconfig_t* iwc, itemconfig_t* ic );
	itemconfig_t* LoadItemConfig( char* filename );
	void BotSaveGoalFuzzyLogic( int goalstate, char* filename );
	void BotMutateGoalFuzzyLogic( int goalstate, float range );
	bot_goalstate_t* BotGoalStateFromHandle( int handle );
	void BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child );
private:
	void ParseItemInfo( idParser& parser, iteminfo_t* itemInfo );
private:
	bot_goalstate_t botgoalstates[MAX_CLIENTS + 1];

	//item configuration
	itemconfig_t		itemconfiglocal;
	itemconfig_t*		itemconfig;

	//level items
	levelitem_t levelitemheap[MAX_BOT_LEVEL_ITEMS];
	levelitem_t* freelevelitems;
	levelitem_t* levelitems;
	int numlevelitems;

	//map locations
	idList<maplocation_t> maplocations;
	idList<campspot_t> campspots;
};

extern idBotGoalManager botGoalManager;
