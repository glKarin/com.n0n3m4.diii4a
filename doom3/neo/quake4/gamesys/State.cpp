#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

const int HISTORY_COUNT = 50;

/*
=====================
stateParms_t::Save
=====================
*/
void stateParms_t::Save( idSaveGame *saveFile ) const {
	saveFile->WriteInt( blendFrames );
	saveFile->WriteInt( time );
	saveFile->WriteInt( stage );
}

/*
=====================
stateParms_t::Restore
=====================
*/
void stateParms_t::Restore( idRestoreGame *saveFile ) {
	saveFile->ReadInt( blendFrames );
	saveFile->ReadInt( time );
	saveFile->ReadInt( stage );
}

/*
=====================
stateCall_t::Save
=====================
*/
void stateCall_t::Save( idSaveGame *saveFile ) const {
	saveFile->WriteString( state->name );
	// TOSAVE: idLinkList<stateCall_t>		node;
	saveFile->WriteInt( flags );
	saveFile->WriteInt( delay );
	parms.Save( saveFile );
}

/*
=====================
stateCall_t::Save
=====================
*/
void stateCall_t::Restore( idRestoreGame *saveFile, const idClass* owner ) {
	idStr name;

	saveFile->ReadString( name );
	state = owner->FindState( name );

	saveFile->ReadInt( flags );
	saveFile->ReadInt( delay );
	parms.Restore( saveFile );
}

/*
=====================
rvStateThread::rvStateThread
=====================
*/
rvStateThread::rvStateThread ( void ) {
	owner		= NULL;
	insertAfter	= NULL;
	lastResult	= SRESULT_DONE;

	states.Clear ( );
	interrupted.Clear ( );
	
	memset ( &fl, 0, sizeof(fl) );
}

/*
=====================
rvStateThread::~rvStateThread
=====================
*/
rvStateThread::~rvStateThread ( void ) {
	Clear ( true );
}

/*
=====================
rvStateThread::SetOwner
=====================
*/
void rvStateThread::SetOwner ( idClass* _owner ) {
	owner = _owner;
}	

/*
=====================
rvStateThread::Post
=====================
*/
stateResult_t rvStateThread::PostState ( const char* name, int blendFrames, int delay, int flags ) {
	const rvStateFunc<idClass>* func;
	
	// Make sure the state exists before queueing it
	if ( NULL == (func = owner->FindState ( name ) ) ) {
		return SRESULT_ERROR;
	}

	stateCall_t* call;
	call = new stateCall_t;
	call->state				= func;
	call->delay				= delay;
	call->flags				= flags;
	call->parms.blendFrames = blendFrames;
	call->parms.time		= -1;
	call->parms.stage		= 0;

	call->node.SetOwner ( call );

	if ( fl.executing && insertAfter ) {
		call->node.InsertAfter ( insertAfter->node );
	} else {
		call->node.AddToEnd ( states );
	}

	insertAfter = call;
	
	return SRESULT_OK;
}

/*
=====================
rvStateThread::Set
=====================
*/
stateResult_t rvStateThread::SetState ( const char* name, int blendFrames, int delay, int flags ) {
	Clear ( );
	return PostState ( name, blendFrames, delay, flags );
}

/*
=====================
rvStateThread::InterruptState
=====================
*/
stateResult_t rvStateThread::InterruptState ( const char* name, int blendFrames, int delay, int flags ) {
	stateCall_t* call;
	
	// Move all states to the front of the interrupted list in the same order
	for ( call = states.Prev(); call; call = states.Prev() ) {
		call->node.Remove ( );
		call->node.AddToFront ( interrupted );
	}

	// Nothing to insert after anymore
	insertAfter = NULL;
	fl.stateInterrupted = true;

	// Post the state now
	return PostState ( name, blendFrames, delay, flags );
}

/*
=====================
rvStateThread::CurrentStateIs
=====================
*/
bool rvStateThread::CurrentStateIs( const char* name ) const {
	return ( !IsIdle() ) ? owner->FindState(name) == GetState()->state : false;
}

/*
=====================
rvStateThread::Clear
=====================
*/
void rvStateThread::Clear ( bool ignoreStateCalls ) {
	stateCall_t* call;
	
	// Clear all states from the main state list
	for( call = states.Next(); call != NULL; call = states.Next() ) {
		if ( !ignoreStateCalls && (call->flags & (SFLAG_ONCLEAR|SFLAG_ONCLEARONLY) ) ) {
			owner->ProcessState ( call->state, call->parms );
		}
		call->node.Remove();
		delete call;
	}		
	
	// Clear all interrupted states
	for( call = interrupted.Next(); call != NULL; call = interrupted.Next() ) {
		if ( !ignoreStateCalls && (call->flags & (SFLAG_ONCLEAR|SFLAG_ONCLEARONLY) ) ) {
			owner->ProcessState ( call->state, call->parms );
		}
		call->node.Remove();
		delete call;
	}		

	insertAfter		= NULL;
	fl.stateCleared	= true;
	
	states.Clear ( );
	interrupted.Clear ( );	
}

/*
=====================
rvStateThread::Execute
=====================
*/
stateResult_t rvStateThread::Execute ( void ) {
	stateCall_t*	call = NULL;
	int				count;
	const char*		stateName;
	int				stateStage;
	const char*		historyState[HISTORY_COUNT];
	int				historyStage[HISTORY_COUNT];
	int				historyStart;
	int				historyEnd;

	// If our main state loop is empty copy over any states in the interrupted state		
	if ( !states.Next ( ) ) {
		for ( call = interrupted.Next(); call; call = interrupted.Next() ) {
			call->node.Remove ( );
			call->node.AddToEnd ( states );
		}
		assert ( !interrupted.Next ( ) );
	}
	
	// State thread is idle if there are no states
	if ( !states.Next() ) {
		return SRESULT_IDLE;
	}
	
	fl.executing = true;
	
	// Run through the states until there are no more or one of them tells us to wait
	count		 = 0;
	historyStart = 0;
	historyEnd	 = 0;
				
	for( call = states.Next(); call && count < HISTORY_COUNT; call = states.Next(), ++count ) {
		insertAfter			= call;
		fl.stateCleared		= false;
		fl.stateInterrupted	= false;

		// If this state is only called when being cleared then just skip it
		if ( call->flags & SFLAG_ONCLEARONLY ) {
			call->node.Remove ( );
			delete call;
			continue;
		}		

		// If the call has a delay on it the time will be set to negative initially and then
		// converted to game time.
		if ( call->parms.time <= 0 ) {
			call->parms.time = gameLocal.time;
		}
		
		// Check for delayed states
		if ( call->delay && gameLocal.time < call->parms.time + call->delay ) {
			fl.executing = false;
			return SRESULT_WAIT;
		}

		// Debugging
		if ( lastResult != SRESULT_WAIT ) {
			if ( *g_debugState.GetString ( ) && (*g_debugState.GetString ( ) == '*' || !idStr::Icmp ( g_debugState.GetString ( ), name ) ) ) {
				if ( call->parms.stage ) {
					gameLocal.Printf ( "%s: %s (%d)\n", name.c_str(), call->state->name, call->parms.stage );
				} else { 
					gameLocal.Printf ( "%s: %s\n", name.c_str(), call->state->name );
				}
			}
		
			// Keep a history of the called states so we can dump them on an overflow
			historyState[historyEnd] = call->state->name;
			historyStage[historyEnd] = call->parms.stage;
			historyEnd = (historyEnd+1) % HISTORY_COUNT;
			if ( historyEnd == historyStart ) {
				historyStart = (historyEnd+1) % HISTORY_COUNT;
			}
		}

		// Cache name and stage for error messages
		stateName  = call->state->name;
		stateStage = call->parms.stage;
		
		// Actually call the state function 		
		lastResult = owner->ProcessState ( call->state, call->parms );		
		switch ( lastResult ) {
			case SRESULT_WAIT:
				fl.executing = false;
				return SRESULT_WAIT;
				
			case SRESULT_ERROR:
				gameLocal.Error ( "rvStateThread: error reported by state '%s (%d)'", stateName, stateStage );
				fl.executing = false;
				return SRESULT_ERROR;				
		}

#ifdef _QUAKE4 // bot
		if (lastResult == SRESULT_DONE) {
			owner->StateThreadChanged();
		}
#endif

		// Dont remove the node if it was interrupted or cleared in the last process
		if ( !fl.stateCleared && !fl.stateInterrupted ) {
			if( lastResult >= SRESULT_SETDELAY ) {
				call->delay = lastResult - SRESULT_SETDELAY;
				call->parms.time = gameLocal.GetTime();
				continue;
			} else if ( lastResult >= SRESULT_SETSTAGE ) {
				call->parms.stage = lastResult - SRESULT_SETSTAGE;
				continue;
			}

			// Done with state so remove it from list
			call->node.Remove ( );
			delete call;		
		}
		
		// Finished the last state but wait a frame for next one
		if ( lastResult == SRESULT_DONE_WAIT ) {
			fl.executing = false;
			return SRESULT_WAIT;
		}	
	}
	
	// Runaway state loop?
#ifdef _QUAKE4 //k: for map game/convoy1
	if(0) //k: now fixed, do not need it no longer
#endif
	if ( count >= HISTORY_COUNT ) {
		idFile *file;

		fileSystem->RemoveFile ( "statedump.txt" );
		file = fileSystem->OpenFileWrite( "statedump.txt" );
		
		for ( ; historyStart != historyEnd; historyStart = (historyStart + 1) % HISTORY_COUNT ) {
			if ( historyStage[historyStart] ) {
				gameLocal.Printf ( "rvStateThread: %s (%d)\n", historyState[historyStart], historyStage[historyStart] );
			} else {
				gameLocal.Printf ( "rvStateThread: %s\n", historyState[historyStart] );
			}
			if ( file ) {
				if ( historyStage[historyStart] ) {
					file->Printf ( "rvStateThread: %s (%d)\n", historyState[historyStart], historyStage[historyStart] );
				} else {
					file->Printf ( "rvStateThread: %s\n", historyState[historyStart] );
				}
			}
		}
		if ( file ) {
			fileSystem->CloseFile( file );	
		}

		gameLocal.Error ( "rvStateThread: run away state loop '%s'", name.c_str() );
	}

	insertAfter  = NULL;
	fl.executing = false;

	// Move interrupted states back into the main state list when the main state list is empty
	if ( !states.Next() && interrupted.Next ( ) ) {
		return Execute ( );
	}
				
	return lastResult;
}

/*
=====================
rvStateThread::Save
=====================
*/
void rvStateThread::Save( idSaveGame *saveFile ) const {
	saveFile->WriteString( name.c_str() );

	// No need to save owner, its setup in restore

	saveFile->WriteInt( lastResult );
	saveFile->Write ( &fl, sizeof(fl) );

	saveFile->WriteInt( states.Num() );
	for( idLinkList<stateCall_t>* node = states.NextNode(); node; node = node->NextNode() ) {
		node->Owner()->Save( saveFile );
	}

	saveFile->WriteInt( interrupted.Num() );
	for( idLinkList<stateCall_t>* node = interrupted.NextNode(); node; node = node->NextNode() ) {
		node->Owner()->Save( saveFile );
	}

	// TOSAVE: 	stateCall_t*				insertAfter;
	// TOSAVE: 	stateResult_t				lastResult;
}

/*
=====================
rvStateThread::Restore
=====================
*/
void rvStateThread::Restore( idRestoreGame *saveFile, idClass* owner ) {
	int numStates;
	stateCall_t* call = NULL;

	saveFile->ReadString( name );

	this->owner = owner;

	saveFile->ReadInt( (int&)lastResult );
	saveFile->Read ( &fl, sizeof(fl) );

	saveFile->ReadInt( numStates );
	for( ; numStates > 0; numStates-- ) {
		call = new stateCall_t;
		assert( call );

		call->Restore( saveFile, owner );

		call->node.SetOwner ( call );
		call->node.AddToEnd ( states );
	}

	saveFile->ReadInt( numStates );
	for( ; numStates > 0; numStates-- ) {
		call = new stateCall_t;
		assert( call );

		call->Restore( saveFile, owner );

		call->node.SetOwner ( call );
		call->node.AddToEnd ( interrupted );
	}
}
