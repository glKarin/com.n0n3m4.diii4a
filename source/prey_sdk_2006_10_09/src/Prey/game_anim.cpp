
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


/*
============
hhAnim::AddFrameCommandExtra
============
*/
bool hhAnim::AddFrameCommandExtra( idToken &token, frameCommand_t &fc, idLexer &src, idStr &errorText ) {
	errorText = "";


	if ( token == "stopSnd" ) {
		fc.type = FC_STOPSND;
		return( true );
	} else if ( token == "stopSnd_voice" ) {
		fc.type = FC_STOPSND_VOICE;
		return( true );
	} else if ( token == "stopSnd_voice2" ) {
		fc.type = FC_STOPSND_VOICE2;
		return( true );
	} else if ( token == "stopSnd_body" ) {
		fc.type = FC_STOPSND_BODY;
		return( true );
	} else if ( token == "stopSnd_body2" ) {
		fc.type = FC_STOPSND_BODY2;
		return( true );
	} else if ( token == "stopSnd_body3" ) {
		fc.type = FC_STOPSND_BODY3;
		return( true );
	} else if ( token == "stopSnd_weapon" ) {
		fc.type = FC_STOPSND_WEAPON;
		return( true );
	} else if ( token == "stopSnd_item" ) {
		fc.type = FC_STOPSND_ITEM;
		return( true );
	} else if ( token == "event_args" ) { 
		fc.type = FC_EVENT_ARGS;
		src.ParseRestOfLine( token );
		idStr error = InitFrameCommandEvent( fc, token );
		if( error.Length() ) {
			errorText = error.c_str();
			return( false );
		}

		return( true );
	} else if ( token == "launch_missile" ) {
		/* HUMANHEAD JRM - Because we parse out the joint when launched
		if ( !modelDef->FindJoint( cmd ) ) {
			return va( "Joint '%s' not found", cmd.c_str() );
		}
		*/
		src.ParseRestOfLine( token );
		/* HUMANHEAD JRM - need the rest of the line not just one token
		if( !src.ReadTokenOnLine( &token ) ) {
			errorText = va( "Unexpected end of line" );
			return( true );
		}
		*/
		fc.type = FC_LAUNCHMISSILE;
		fc.string = new idStr( token );

		return( true );
	} else if( token == "launch_altMissile" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			errorText = va( "Unexpected end of line" );
			return( true );
		}
		fc.type = FC_LAUNCHALTMISSILE;
		fc.string = new idStr( token );

		return( true );
	} else if ( token == "launch_missile_bonedir" ) {
		/* HUMANHEAD JRM - we want the whole token
		if( !src.ReadTokenOnLine( &token ) ) {
			errorText = va( "Unexpected end of line" );
			return( true );
		}		
		*/
		src.ParseRestOfLine( token );
		fc.type = FC_LAUNCHMISSILE_BONEDIR;
		fc.string = new idStr( token );

		return( true );
	} else if ( token == "leftfootprint"  ) {
		fc.type = FC_LEFTFOOTPRINT;

		return( true );
	} else if ( token == "rightfootprint" ) {
		fc.type = FC_RIGHTFOOTPRINT;

		return( true );
	} else if ( token == "mood" ) {
		fc.type = FC_MOOD;
		if( !src.ReadTokenOnLine( &token ) ) {
			fc.string = new idStr( "" );
			return( true );
		}		
		fc.string = new idStr( token );

		return( true );		
	} else if ( token == "kick_obstacle" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			errorText = va( "Unexpected end of line" );
			return( true );
		}
		fc.type = FC_KICK_OBSTACLE;
		fc.string = new idStr( token );

		return( true );
	} else if ( token == "trigger_anim_ent" ) {
		/*
		if( !src.ReadTokenOnLine( &token ) ) {
			errorText = va( "Unexpected end of line" );
			return( true );
		}
		*/
		fc.type = FC_TRIGGER_ANIM_ENT;
		return( true );
	} else if ( token == "set_key" ) {
		fc.type = FC_SETKEY;
		if( !src.ParseRestOfLine( token ) ) {
			errorText = va( "Unexpected end of line" );
			return ( true );
		}
		if( !fc.parmList ) {
			fc.parmList = new idList<idStr>;
		}
		hhUtils::SplitString( token, *fc.parmList, ' ' );
		if( fc.parmList->Num() != 2 ) {
			errorText = va( "Invalid number of arguments for setkey frame-command" );
			return ( true );
		}
		return ( true );
	}
	
	
	return( false );
}

/*
============
hhAnim::CallFrameCommandsExtra
============
*/
bool hhAnim::CallFrameCommandsExtra( const frameCommand_t &command, idEntity *ent ) const {
	switch( command.type ) {
		case FC_EVENT_ARGS: {
			if ( command.function && command.parmList && command.function->eventdef ) {
				ent->ProcessEvent( command.function->eventdef, (int)command.parmList );
			}	
			return( true );
		}
		case FC_LAUNCHALTMISSILE: {
			//ent->ProcessEvent( &AI_AttackAltMissile, command.string->c_str(), NULL );
			return( true );
		}				
		case FC_STOPSND: 
		case FC_STOPSND_VOICE:	  
		case FC_STOPSND_VOICE2:
		case FC_STOPSND_BODY:
		case FC_STOPSND_BODY2:
		case FC_STOPSND_BODY3:
		case FC_STOPSND_WEAPON:
		case FC_STOPSND_ITEM: {
			ent->StopSound( s_channelType(command.type - FC_STOPSND), false ); //rww - do not broadcast
			return( true );
		}
		case FC_HIDE: {
			ent->ProcessEvent( &EV_Hide );
			return( true );
		}
		case FC_MOOD: {
			if(command.string) {
				ent->ProcessEvent( &AI_SetAnimPrefix, command.string->c_str());
			} else {
				ent->ProcessEvent( &AI_SetAnimPrefix, " ");
			}
			return( true );
		}
	  	case FC_LEFTFOOTPRINT: {
			ent->ProcessEvent( &EV_FootprintLeft );
			return( true );
		}
		case FC_RIGHTFOOTPRINT: {
			ent->ProcessEvent( &EV_FootprintRight );
			return( true );
		}
		case FC_LAUNCHMISSILE: { // JRM: changed to AttackMissileEx (so we can pass in projectile)
			ent->ProcessEvent( &MA_AttackMissileEx, command.string->c_str(),0 );
			return( true );
		}			   
		case FC_LAUNCHMISSILE_BONEDIR: { // JRM: need so we can launch missiles
			ent->ProcessEvent( &MA_AttackMissileEx, command.string->c_str(),1 );
			return( true );
		}
		case FC_SETKEY:	{//mdc: added to be able to set keys from frame commands
			if( command.parmList && command.parmList->Num() == 2 ) {
				ent->spawnArgs.Set( (*command.parmList)[0].c_str(), (*command.parmList)[1].c_str() );
			}
			return ( true );
		}
	}

	return( false );

}


/*
=====================
hhAnim::InitFrameCommandEvent
=====================
*/
idStr hhAnim::InitFrameCommandEvent( frameCommand_t &command, const idStr& cmdStr ) const {
	idStr error;

	if( !command.parmList ) {
		command.parmList = new idList<idStr>;
	}

	hhUtils::SplitString( cmdStr, *command.parmList, ' ' );

	// Find the function
	command.function = gameLocal.program.FindFunction( (*command.parmList)[0].c_str(), gameLocal.program.FindType((*command.parmList)[0].c_str()) );
	if( !command.function || !command.function->eventdef || command.function->eventdef->GetNumArgs() != 1 ) {
		error = va("Invalid event('%s') for frameCommands event or event_args\n", (*command.parmList)[0].c_str() );
	}

	//Remove function name from the list
	command.parmList->RemoveIndex( 0 );

	//If no parms are needed, no need to waste memory
	if( command.parmList->Num() <= 0 ) {
		SAFE_DELETE_PTR( command.parmList );
	}

	return error;
}

