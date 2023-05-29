#ifndef __SYS_STATE_H__
#define __SYS_STATE_H__

typedef enum {
	SRESULT_OK,				// Call was made successfully
	SRESULT_ERROR,			// An unrecoverable error occurred
	SRESULT_DONE,			// Done with current state, move to next
	SRESULT_DONE_WAIT,		// Done with current state, wait a frame then move to next
	SRESULT_WAIT,			// Wait a frame and re-run current state
	SRESULT_IDLE,			// State thread is currently idle (ie. no states)
	SRESULT_SETSTAGE,		// Sets the current stage of the current state and reruns the state
							// NOTE: this has to be the last result becuase the stage is added to
							//		 the result.
	SRESULT_SETDELAY = SRESULT_SETSTAGE + 20
} stateResult_t;

#define MAX_STATE_CALLS		50

#define SRESULT_STAGE(x)	((stateResult_t)((int)SRESULT_SETSTAGE + (int)(x)))
#define SRESULT_DELAY(x)	((stateResult_t)((int)SRESULT_SETDELAY + (int)(x)))

struct stateParms_t {
	int		blendFrames;
	int		time;
	int		stage;

	void	Save( idSaveGame *saveFile ) const;
	void	Restore( idRestoreGame *saveFile );
};

typedef stateResult_t ( idClass::*stateCallback_t )( const stateParms_t& parms );

template< class Type >
struct rvStateFunc {
	const char*			name;
	stateCallback_t		function;
};

/*
================
CLASS_STATES_PROTOTYPE

This macro must be included in the definition of any subclass of idClass that
wishes to have its own custom states.  Its prototypes variables used in the process
of managing states.
================
*/
#define CLASS_STATES_PROTOTYPE(nameofclass)						\
protected:														\
	static	rvStateFunc<nameofclass>		stateCallbacks[]

/*
================
CLASS_STATES_DECLARATION

This macro must be included in the code to properly initialize variables
used in state processing for a idClass dervied class
================
*/
#define CLASS_STATES_DECLARATION(nameofclass)				\
rvStateFunc<nameofclass> nameofclass::stateCallbacks[] = {
	
/*
================
STATE

This macro declares a single state.  It must be surrounded by the CLASS_STATES_DECLARATION 
and END_CLASS_STATES macros.
================
*/
#define STATE(statename,function)			{ statename, (stateCallback_t)( &function ) },

/*
================
END_CLASS_STATES

Terminates a state block
================
*/
#define END_CLASS_STATES					{ NULL, NULL } };

struct stateCall_t {
	const rvStateFunc<idClass>*	state;
	idLinkList<stateCall_t>		node;
	int							flags;
	int							delay;
	stateParms_t				parms;

	void						Save( idSaveGame *saveFile ) const;
	void						Restore( idRestoreGame *saveFile, const idClass* owner );
};

class idClass;

const int SFLAG_ONCLEAR			= BIT(0);			// Executes, even if the state queue is cleared
const int SFLAG_ONCLEARONLY		= BIT(1);			// Executes only if the state queue is cleared

class rvStateThread {
public:

	rvStateThread ( void );
	~rvStateThread ( void );
	
	void			SetName			( const char* name );
	void			SetOwner		( idClass* owner );
	
	bool			Interrupt		( void );

	stateResult_t	InterruptState	( const char* state, int blendFrames = 0, int delay = 0, int flags = 0 );	
	stateResult_t	PostState		( const char* state, int blendFrames = 0, int delay = 0, int flags = 0 );
	stateResult_t	SetState		( const char* state, int blendFrames = 0, int delay = 0, int flags = 0 );
	stateCall_t*	GetState		( void ) const;
	bool			CurrentStateIs	( const char* name ) const;
	
	stateResult_t	Execute			( void );
	
	void			Clear			( bool ignoreStateCalls = false );
	
	bool			IsIdle			( void ) const;
	bool			IsExecuting		( void ) const;

	void			Save( idSaveGame *saveFile ) const;
	void			Restore( idRestoreGame *saveFile, idClass* owner );
	
protected:

	struct flags {
		bool		stateCleared		:1;		// State list was cleared 
		bool		stateInterrupted	:1;		// State list was interrupted
		bool		executing			:1;		// Execute is currently processing states
	} fl;

	idStr						name;
	idClass*					owner;
	idLinkList<stateCall_t>		states;
	idLinkList<stateCall_t>		interrupted;
	stateCall_t*				insertAfter;
	stateResult_t				lastResult;
};

ID_INLINE void rvStateThread::SetName ( const char* _name ) {
	name = _name;
}

ID_INLINE stateCall_t* rvStateThread::GetState ( void ) const {
	return states.Next();
}

ID_INLINE bool rvStateThread::IsIdle ( void ) const {
	return !states.Next() && !interrupted.Next();
}

ID_INLINE bool rvStateThread::IsExecuting ( void ) const {
	return fl.executing;
}

#endif // __SYS_STATE_H__
