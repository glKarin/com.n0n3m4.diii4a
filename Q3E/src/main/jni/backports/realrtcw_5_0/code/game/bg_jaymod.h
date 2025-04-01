#ifndef GAME_JAYMOD_H
#define GAME_JAYMOD_H

///////////////////////////////////////////////////////////////////////////////

// Shotgun
// Animation time = 1000 / fps * numFrames
// moved all those timings to .weap files
//#define   M97_RLT_RELOAD1			400		// Reload normal shell start
//#define	M97_RLT_RELOAD2			900		// Reload normal shell loop
//#define	M97_RLT_RELOAD3			550		// Reload normal shell end
//#define	M97_RLT_ALTSWITCHFROM	2000	// Reload first shell and pump start
//#define	M97_RLT_ALTSWITCHTO		300		// Reload first shell and pump to loop
//#define	M97_RLT_DROP2			375		// Reload first shell and pump end

//#define AUTO5_RLT_RELOAD1		400		// Reload normal shell start
//#define	AUTO5_RLT_RELOAD2		900		// Reload normal shell loop
//#define	AUTO5_RLT_RELOAD3		550		// Reload normal shell end
//#define	AUTO5_RLT_ALTSWITCHFROM	2000	// Reload first shell and pump start
//#define	AUTO5_RLT_ALTSWITCHTO	300		// Reload first shell and pump to loop
//#define	AUTO5_RLT_DROP2			375		// Reload first shell and pump end

// Shotgun reload states
typedef enum {
	M97_READY,							// Not reloading
	M97_RELOADING_BEGIN,				// Reload normal shell start
	M97_RELOADING_BEGIN_PUMP,			// Reload first shell and pump start
	M97_RELOADING_AFTER_PUMP,			// Reload first shell and pump to loop
	M97_RELOADING_LOOP,					// Reload normal shell loop
} m97state_t;


// Shotgun reload states
typedef enum {
	AUTO5_READY,						// Not reloading
	AUTO5_RELOADING_BEGIN,				// Reload normal shell start
	AUTO5_RELOADING_BEGIN_PUMP,			// Reload first shell and pump start
	AUTO5_RELOADING_AFTER_PUMP,			// Reload first shell and pump to loop
	AUTO5_RELOADING_LOOP,			    // Reload normal shell loop
} auto5state_t;

// Functions
void PM_BeginM97Reload ( );
void PM_M97Reload      ( );

void PM_BeginAuto5Reload ( );
void PM_Auto5Reload      ( );

#endif // GAME_JAYMOD_H
