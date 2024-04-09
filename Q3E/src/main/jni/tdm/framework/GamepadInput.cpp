/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#include "GamepadInput.h"

idCVar in_padLongPressTimeMs("in_padLongPressTimeMs", "250", CVAR_GAME|CVAR_INTEGER|CVAR_ARCHIVE, "Amount of time a gamepad button must be held to activate a long press action");
idCVar in_padTwoButtonMaxTimeMs("in_padTwoButtonMaxTimeMs", "50", CVAR_GAME|CVAR_INTEGER|CVAR_ARCHIVE, "Amount of time two gamepad buttons can be pressed apart to be registered as a two-button action");
idCVar in_padL3HoldWhileDeflected("in_padL3HoldWhileDeflected", "1", CVAR_GAME|CVAR_BOOL|CVAR_ARCHIVE, "If enabled, an L3 click will be held until the left stick is released");

namespace {
	enum padBindingType_t {
		BIND_PRESS,
		BIND_LONG_PRESS,
		BIND_MOD_PRESS,
		BIND_MOD_LONG_PRESS,
		BIND_NUM,

		BIND_MODIFIER,
	};

	struct padCmd_t {
		idStr cmd;
		int action;
	};

	enum padStatus_t {
		STATUS_NONE,
		STATUS_PRESSED,
		STATUS_RELEASED,
	};

	struct padButtonState_t {
		padStatus_t status;
		uint64_t lastPressed;
		uint64_t lastReleased;

		padCmd_t bindings[BIND_NUM];
		int activeBinding;

		padButtonState_t() : status(STATUS_NONE), lastPressed(0), lastReleased(0), activeBinding(-1) {
			for ( int i = 0; i < BIND_NUM; ++i ) {
				bindings[i].action = -1;
			}
		}
	};

	padButtonState_t buttonState[PAD_NUM_BUTTONS] = {};
	int modifierButton;

	int axisState[PAD_NUM_AXES];

	struct nameMapping_t {
		const char *name;
		int value;
	};

	nameMapping_t buttonNames[] = {
		{ "PAD_A", PAD_A },
		{ "PAD_B", PAD_B },
		{ "PAD_X", PAD_X },
		{ "PAD_Y", PAD_Y },
		{ "PAD_L1", PAD_L1 },
		{ "PAD_R1", PAD_R1 },
		{ "PAD_L2", PAD_L2 },
		{ "PAD_R2", PAD_R2 },
		{ "PAD_L3", PAD_L3 },
		{ "PAD_R3", PAD_R3 },
		{ "PAD_BACK", PAD_BACK },
		{ "PAD_START", PAD_START },
		{ "PAD_LEFT", PAD_LEFT },
		{ "PAD_RIGHT", PAD_RIGHT },
		{ "PAD_UP", PAD_UP },
		{ "PAD_DOWN", PAD_DOWN },
		{ nullptr, -1 },
	};

	nameMapping_t typeNames[] = {
		{ "PRESS", BIND_PRESS },
		{ "LONG_PRESS", BIND_LONG_PRESS },
		{ "MOD_PRESS", BIND_MOD_PRESS },
		{ "MOD_LONG_PRESS", BIND_MOD_LONG_PRESS },
		{ "MODIFIER", BIND_MODIFIER },
		{ nullptr, -1 },
	};

	bool IsModButtonActiveFor( int button ) {
		if ( modifierButton == -1 ) {
			return false;
		}
		const padButtonState_t &ms = buttonState[modifierButton];
		const padButtonState_t &bs = buttonState[button];
		return ms.lastPressed <= bs.lastPressed && (ms.status == STATUS_PRESSED || ms.lastReleased >= bs.lastPressed);
	}

	uint64_t leanLeftPressed;
	uint64_t leanRightPressed;
	bool forwardLeanActive;
}

void Gamepad_BindButton_f( const idCmdArgs &args ) {
	int numArgs = args.Argc();

	if ( numArgs < 3 ) {
		common->Printf( "bindPadButton <type> <button> [command] : attach a command to a gamepad button\n" );
		return;
	}

	int type = idGamepadInput::StringToType( args.Argv( 1 ) );
	if ( type == -1 ) {
		common->Printf( "\"%s\" isn't a valid binding type\n", args.Argv(1) );
	}

	int button = idGamepadInput::StringToButton( args.Argv( 2 ) );
	if ( button == -1 || button >= PAD_NUM_BUTTONS ) {
		common->Printf( "\"%s\" isn't a valid gamepad button\n", args.Argv(2) );
		return;
	}

	// copy the rest of the command line
	idStr cmd;
	for ( int i = 3; i < numArgs; i++ ) {
		cmd += args.Argv( i );
		if ( i != (numArgs-1) ) {
			cmd += " ";
		}
	}

	idGamepadInput::SetBinding( button, type, cmd );
}

void ArgCompletion_PadButtonName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	for ( auto kn = buttonNames; kn->name; kn++ ) {
		callback( va( "%s %s", args.Argv( 0 ), kn->name ) );
	}
}

void Gamepad_Unbind_f( const idCmdArgs &args ) {
	idGamepadInput::UnbindAll();
}

void idGamepadInput::Init() {
	memset(axisState, 0, sizeof(axisState));
	modifierButton = -1;
	leanLeftPressed = 0;
	leanRightPressed = 0;
	forwardLeanActive = false;

	cmdSystem->AddCommand( "bindPadButton", Gamepad_BindButton_f, CMD_FL_SYSTEM, "binds a command to a gamepad button", ArgCompletion_PadButtonName );
	cmdSystem->AddCommand( "unbindPad", Gamepad_Unbind_f, CMD_FL_SYSTEM, "unbinds all gamepad buttons" );
}

void idGamepadInput::Shutdown() {}

int idGamepadInput::StringToButton( const idStr &str ) {
	for ( nameMapping_t *n = buttonNames; n->name; n++ ) {
		if ( !idStr::Icmp( str, n->name ) ) {
			return n->value;
		}
	}

	return -1;
}

idStr idGamepadInput::ButtonToString( int button ) {
	for ( nameMapping_t *n = buttonNames; n->name; n++ ) {
		if ( n->value == button ) {
			return n->name;
		}
	}

	return "";
}

int idGamepadInput::StringToType( const idStr &str ) {
	for ( nameMapping_t *n = typeNames; n->name; n++ ) {
		if ( !idStr::Icmp( str, n->name ) ) {
			return n->value;
		}
	}

	return -1;
}

idStr idGamepadInput::TypeToString( int type ) {
	for ( nameMapping_t *n = typeNames; n->name; n++ ) {
		if ( n->value == type ) {
			return n->name;
		}
	}

	return "";
}

void idGamepadInput::UnbindAll() {
	usercmdGen->Clear();
	for ( int i = 0; i < PAD_NUM_BUTTONS; ++i ) {
		buttonState[i].status = STATUS_NONE;
		buttonState[i].lastPressed = 0;
		buttonState[i].lastReleased = 0;
		buttonState[i].activeBinding = -1;

		for ( int j = 0; j < BIND_NUM; ++j ) {
			buttonState[i].bindings[j].action = -1;
			buttonState[i].bindings[j].cmd = "";
		}
	}
	modifierButton = -1;

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
}

void idGamepadInput::SetBinding( int button, int type, const idStr &binding ) {
	if ( button < 0 || button >= PAD_NUM_BUTTONS || type < 0 || (type >= BIND_NUM && type != BIND_MODIFIER) ) {
		common->Printf("Invalid gamepad button binding\n");
		return;
	}

	// Clear out all button states so we aren't stuck forever thinking this key is held down
	usercmdGen->Clear();
	for ( int i = 0; i < PAD_NUM_BUTTONS; ++i ) {
		buttonState[i].status = STATUS_NONE;
		buttonState[i].lastPressed = 0;
		buttonState[i].lastReleased = 0;
		buttonState[i].activeBinding = -1;
	}

	if ( type == BIND_MODIFIER ) {
		// setting the modifier key unbinds all other actions for that button
		for ( int i = 0; i < BIND_NUM; ++i ) {
			buttonState[button].bindings[i].action = -1;
		}
		modifierButton = button;
		common->Printf("Gamepad modifier button assigned to %d\n", button);
	} else {
		padCmd_t &bind = buttonState[button].bindings[type];
		bind.cmd = binding;
		bind.action = usercmdGen->CommandStringUsercmdData( binding );
		if ( modifierButton == button ) {
			modifierButton = -1;
		}
	}

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
}

void idGamepadInput::SetButtonState( int button, bool pressed ) {
	if ( button < 0 || button >= PAD_NUM_BUTTONS ) {
		return;
	}

	padButtonState_t &st = buttonState[button];

	if ( pressed ) {
		st.status = STATUS_PRESSED;
		st.lastPressed = Sys_GetTimeMicroseconds();
	} else {
		st.status = STATUS_RELEASED;
		st.lastReleased = Sys_GetTimeMicroseconds();
	}
}

void idGamepadInput::SetAxisState( int axis, int value ) {
	if ( axis < PAD_MIN_AXIS || axis > PAD_MAX_AXIS ) {
		return;
	}

	axisState[axis - PAD_MIN_AXIS] = value;
}

void idGamepadInput::UpdateAxisState( int gameAxis[6] ) {
	gameAxis[AXIS_SIDE] = axisState[PAD_AXIS_LX - PAD_MIN_AXIS];
	gameAxis[AXIS_FORWARD] = axisState[PAD_AXIS_LY - PAD_MIN_AXIS];
	gameAxis[AXIS_YAW] = axisState[PAD_AXIS_RX - PAD_MIN_AXIS];
	gameAxis[AXIS_PITCH] = axisState[PAD_AXIS_RY - PAD_MIN_AXIS];
}

idList<padActionChange_t> idGamepadInput::GetActionStateChange() {
	idList<padActionChange_t> actionChanges;

	const uint64_t longPressTime = in_padLongPressTimeMs.GetInteger() * 1000;
	const uint64_t twoButtonTime = in_padTwoButtonMaxTimeMs.GetInteger() * 1000;

	const uint64_t currentTime = Sys_GetTimeMicroseconds();

	// double-lean hack
	if ( !forwardLeanActive ) {
		if ( leanLeftPressed > 0 && leanRightPressed > 0 ) {
			actionChanges.AddGrow( { UB_LEAN_FORWARD, "", true } );
			forwardLeanActive = true;
		} else if ( leanLeftPressed > 0 && currentTime - leanLeftPressed >= twoButtonTime ) {
			actionChanges.AddGrow( { UB_LEAN_LEFT, "", true } );
			leanLeftPressed = 0;
		} else if ( leanRightPressed > 0 && currentTime - leanRightPressed >= twoButtonTime ) {
			actionChanges.AddGrow( { UB_LEAN_RIGHT, "", true } );
			leanRightPressed = 0;
		}
	}
	
	for ( int btn = 0; btn < PAD_NUM_BUTTONS; ++btn ) {
		padButtonState_t &bs = buttonState[btn];
		if ( btn == modifierButton ) {
			continue;
		}

		if ( bs.activeBinding != -1 ) {
			padCmd_t &bind = bs.bindings[bs.activeBinding];
			// check if the current action should be released
			if ( bs.status == STATUS_RELEASED || bs.status == STATUS_NONE ) {
				if ( btn == PAD_L3 && in_padL3HoldWhileDeflected.GetBool() ) {
					if ( axisState[PAD_AXIS_LX - PAD_MIN_AXIS] != 0 || axisState[PAD_AXIS_LY - PAD_MIN_AXIS] != 0 ) {
						// don't release while the stick is deflected
						continue;
					}
				}
				if ( (bind.action == UB_LEAN_LEFT || bind.action == UB_LEAN_RIGHT) && forwardLeanActive ) {
					// hack for forward lean
					actionChanges.AddGrow( { UB_LEAN_FORWARD, "", false } );
				} else {
					actionChanges.AddGrow( { bind.action, bind.cmd, false } );
				}
				bs.activeBinding = -1;
				bs.status = STATUS_NONE;

				if ( bind.action == UB_LEAN_LEFT || bind.action == UB_LEAN_RIGHT ) {
					leanLeftPressed = 0;
					leanRightPressed = 0;
					forwardLeanActive = false;
				}
			}
		} else {
			bool modActive = IsModButtonActiveFor(btn);
			bool hasLongBinding = modActive
				? bs.bindings[BIND_MOD_LONG_PRESS].action != -1
				: bs.bindings[BIND_LONG_PRESS].action != -1;
			bool hasShortBinding = modActive
				? bs.bindings[BIND_MOD_PRESS].action != -1
				: bs.bindings[BIND_PRESS].action != -1;
			bool isLongPressed = bs.status == STATUS_PRESSED && currentTime - bs.lastPressed >= longPressTime;
			bool isShortPressed = bs.status == STATUS_RELEASED || (bs.status == STATUS_PRESSED && !hasLongBinding);

			if ( isLongPressed && hasLongBinding ) {
				bs.activeBinding = modActive ? BIND_MOD_LONG_PRESS : BIND_LONG_PRESS;
			}
			else if ( isShortPressed && hasShortBinding ) {
				bs.activeBinding = modActive ? BIND_MOD_PRESS : BIND_PRESS;
			}

			if ( bs.activeBinding != -1 ) {
				padCmd_t &bind = bs.bindings[bs.activeBinding];
				// hack to enable forward leaning when both lean left/right are pressed:
				// need to delay sending the left/right lean impulses to allow for both to be pressed
				if ( bind.action == UB_LEAN_LEFT ) {
					leanLeftPressed = currentTime;
				} else if ( bind.action == UB_LEAN_RIGHT ) {
					leanRightPressed = currentTime;
				} else {
					actionChanges.AddGrow( { bind.action, bind.cmd, true } );
				}
			}
		}
	}

	return actionChanges;
}

void idGamepadInput::WriteBindings( idFile *f ) {
	f->Printf( "unbindPad\n" );

	if ( modifierButton != -1 ) {
		f->Printf( "bindPadButton MODIFIER %s\n", ButtonToString( modifierButton ).c_str() );
	}

	for ( int btn = 0; btn < PAD_NUM_BUTTONS; btn++ ) {
		for ( int type = 0; type < BIND_NUM; ++type ) {
			if ( buttonState[btn].bindings[type].action != -1 ) {
				f->Printf( "bindPadButton %s %s \"%s\"\n", TypeToString( type ).c_str(), ButtonToString( btn ).c_str(), buttonState[btn].bindings[type].cmd.c_str() );
			}
		}
	}
}
