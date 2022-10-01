// Bot_weights.h
//

#define MAX_INVENTORYVALUE			999999
#define EVALUATERECURSIVELY

#define MAX_WEIGHT_FILES			128
#define MAX_FUZZY_OPERATORS			8192

class idBotFuzzyWeightManager
{
public:
	// Init the fuzzy weight manager.
	void Init( void );

	// Parses a weight config file.
	weightconfig_t* ReadWeightConfig( char* filename );

	void BotShutdownWeights( void );

	// Fuzzy Weight simulation functions.
	int FindFuzzyWeight( weightconfig_t* wc, char* name );
	float FuzzyWeight( int* inventory, weightconfig_t* wc, int weightnum );
	float FuzzyWeightUndecided( int* inventory, weightconfig_t* wc, int weightnum );
	void EvolveFuzzySeperator_r( fuzzyseperator_t* fs );
	void EvolveWeightConfig( weightconfig_t* config );
	void ScaleWeight( weightconfig_t* config, char* name, float scale );
	void ScaleFuzzyBalanceRange( weightconfig_t* config, float scale );
	void InterbreedWeightConfigs( weightconfig_t* config1, weightconfig_t* config2, weightconfig_t* configout );
	void FreeWeightConfig( weightconfig_t* config );
private:
	fuzzyseperator_t* AllocFuzzyWeight( void );
	bool ReadValue( idParser& source, float* value );
	int ReadFuzzyWeight( idParser& source, fuzzyseperator_t* fs );

	int InterbreedFuzzySeperator_r( fuzzyseperator_t* fs1, fuzzyseperator_t* fs2, fuzzyseperator_t* fsout );

	fuzzyseperator_t* ReadFuzzySeperators_r( idParser& source );

	void FreeWeightConfig2( weightconfig_t* config );
	void FreeFuzzySeperators_r( fuzzyseperator_t* fs );

	void ScaleFuzzySeperatorBalanceRange_r( fuzzyseperator_t* fs, float scale );

	void ScaleFuzzySeperator_r( fuzzyseperator_t* fs, float scale );
	float FuzzyWeight_r( int* inventory, fuzzyseperator_t* fs );
	float FuzzyWeightUndecided_r( int* inventory, fuzzyseperator_t* fs );

	weightconfig_t weightFileList[MAX_WEIGHT_FILES];
	fuzzyseperator_t fuzzyseperators[MAX_FUZZY_OPERATORS];
};

extern idBotFuzzyWeightManager botFuzzyWeightManager;