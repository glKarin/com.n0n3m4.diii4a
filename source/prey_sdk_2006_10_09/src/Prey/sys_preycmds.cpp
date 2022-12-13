// sys_preycmds.cpp
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


#if INGAME_PROFILER_ENABLED
void Profiler_ZoomOut_f( const idCmdArgs &args )			{	profiler->SubmitFrameCommand(PROFCOMMAND_TRAVERSEOUT);	}
void Profiler_Mode_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_TOGGLEMODE);	}
void Profiler_MSMode_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_TOGGLEMS);	}
void Profiler_Smooth_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_TOGGLESMOOTHING);	}
void Profiler_ToggleCapture_f( const idCmdArgs &args )		{	profiler->SubmitFrameCommand(PROFCOMMAND_TOGGLECAPTURE);	}
void Profiler_Toggle_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_TOGGLE);	}
void Profiler_Release_f( const idCmdArgs &args )			{	profiler->SubmitFrameCommand(PROFCOMMAND_RELEASE);	}
void Profiler_In0_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN0);	}
void Profiler_In1_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN1);	}
void Profiler_In2_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN2);	}
void Profiler_In3_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN3);	}
void Profiler_In4_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN4);	}
void Profiler_In5_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN5);	}
void Profiler_In6_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN6);	}
void Profiler_In7_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN7);	}
void Profiler_In8_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN8);	}
void Profiler_In9_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN9);	}
void Profiler_In10_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN10);	}
void Profiler_In11_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN11);	}
void Profiler_In12_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN12);	}
void Profiler_In13_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN13);	}
void Profiler_In14_f( const idCmdArgs &args )				{	profiler->SubmitFrameCommand(PROFCOMMAND_IN14);	}
#endif

void Cmd_GetMapName_f( const idCmdArgs &args ) {
	gameLocal.Printf( "%s\n", gameLocal.GetMapName() );
}

void Cmd_TestText_f( const idCmdArgs &args ) {
	char translated[1024];

	if ( args.Argc() < 2 ) {
		gameLocal.Printf("Usage: testtext <text>\n");
		return;
	}
	common->FixupKeyTranslations(args.Argv(1), translated, 1024);	// No passing idStr between game and engine
	gameLocal.Printf("%s\n", translated);
}

void Cmd_GetBinds_f( const idCmdArgs &args ) {
	const char	*entityname;

	if ( args.Argc() < 2 ) {
		gameLocal.Printf("Usage: getbinds <entityname>\n");
		return;
	}

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	// get name of entity
	entityname = args.Argv( 1 );

	// find entity by name or classname
	idEntity *master = gameLocal.FindEntity(entityname);
	if (master) {
		int count = 0;
		for (int i = 0; i < gameLocal.num_entities; i++) {
			idEntity *ent = gameLocal.entities[i];
			if ( ent && ent->GetBindMaster() == master ) {
				gameLocal.Printf("  %s bound (%s)\n", ent->GetName(), ent->GetClassname());
				count++;
			}
		}
		gameLocal.Printf("%d entities bound to %s\n", count, entityname);
	}
}

void Cmd_SpiritWalkMode_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		gameLocal.Printf("Usage: envirosuit <1|0>\n");
		return;
	}

	bool on = atoi(args.Argv( 1 )) != 0;
	gameLocal.SpiritWalkSoundMode( on );
}

void Cmd_DialogMode_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		gameLocal.Printf("Usage: dialogmode <1|0>\n");
		return;
	}

	bool on = atoi(args.Argv( 1 )) != 0;
	idPlayer *player = gameLocal.GetLocalPlayer();
	if (player && player->IsType(hhPlayer::Type)) {
		if (on) {
			static_cast<hhPlayer*>(player)->DialogStart(0, 1, 1);
		}
		else {
			static_cast<hhPlayer*>(player)->DialogStop();
		}
	}
}

void Cmd_EntitySize_f( const idCmdArgs &args ) {
	//TEMP: For memory optimization
	gameLocal.Printf("Sizeof(idDict):							%d\n", (int)sizeof(idDict));
	gameLocal.Printf("Sizeof(idList):							%d\n", 16);
	gameLocal.Printf("Sizeof(idMat3):							%d\n", (int)sizeof(idMat3));
	gameLocal.Printf("Sizeof(idVec3):							%d\n", (int)sizeof(idVec3));

	gameLocal.Printf("Sizeof(renderEntity_t):					%d\n", (int)sizeof(renderEntity_t));
	gameLocal.Printf("Sizeof(refSound_t):						%d\n", (int)sizeof(refSound_t));
	gameLocal.Printf("Sizeof(idAnimator):						%d\n", (int)sizeof(idAnimator));

	gameLocal.Printf("Sizeof(idClass):						%d\n", (int)sizeof(idClass));
	gameLocal.Printf("sizeof(idEntity):						%d\n", (int)sizeof(idEntity));
	gameLocal.Printf("Sizeof(idAI):							%d\n", (int)sizeof(idAI));
	gameLocal.Printf("Sizeof(hhPlayer):						%d\n", (int)sizeof(hhPlayer));

	gameLocal.Printf("Number entities:  %d\n", gameLocal.num_entities);
	gameLocal.Printf("sizeof(idEntity): %d\n", (int)sizeof(idEntity));
	gameLocal.Printf("Total mem cost:   %d\n", gameLocal.num_entities * (int)sizeof(idEntity));
}

void Cmd_Assert_f( const idCmdArgs &args ) {
	assert(0);
}

void Cmd_Dormant_f( const idCmdArgs &args ) {
	const char	*classname;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() != 2 ) {
		gameLocal.Printf("Usage: dormant <classname>\n");
		return;
	}

	classname = args.Argv( 1 );
	const idTypeInfo *searchType = idClass::GetClass(classname);

	if (searchType) {
		// make all entities of this class: neverdormant
		int count = 0;
		for (int i = 0; i < gameLocal.num_entities; i++) {
			idEntity *ent = gameLocal.entities[i];
			if (ent && ent->IsType(*searchType)) {
				ent->fl.neverDormant = false;
				count++;
			}
		}
		gameLocal.Printf("%d %s entities unmarked neverdormant\n", count, classname);
	}
	else {
		gameLocal.Printf("class not found, make sure capitalization is correct\n");
	}
}

void Cmd_UnDormant_f( const idCmdArgs &args ) {
	const char	*classname;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() != 2 ) {
		gameLocal.Printf("Usage: undormant <classname>\n");
		return;
	}

	classname = args.Argv( 1 );
	const idTypeInfo *searchType = idClass::GetClass(classname);

	if (searchType) {
		// make all entities of this class: neverdormant
		int count = 0;
		for (int i = 0; i < gameLocal.num_entities; i++) {
			idEntity *ent = gameLocal.entities[i];
	//		if (ent && !idStr::Icmp(ent->GetClassname(), classname)) {
			if (ent && ent->IsType(*searchType)) {
				ent->fl.neverDormant = true;
				count++;
			}
		}

		gameLocal.Printf("%d %s entities marked neverdormant\n", count, classname);
	}
	else {
		gameLocal.Printf("class not found, make sure capitalization is correct\n");
	}
}

void Cmd_TestHealthPulse_f( const idCmdArgs &args ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if (player && player->hud) {
		player->healthPulse = true;
	}
}
void Cmd_TestSpiritPulse_f( const idCmdArgs &args ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if (player && player->hud) {
		player->spiritPulse = true;
	}
}

void Cmd_SpawnABunch_f( const idCmdArgs &args ) {
	const char *key, *value;
	float		yaw;
	idVec3		org;
	idPlayer	*player;
	idDict		dict;
	int			i;
	const char *classname;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return;
	}

	if ( args.Argc() < 2 || !args.Argc() & 1 ) {	// must always have an even number of arguments
		gameLocal.Printf( "usage: spawn classname <NumberToSpawn> [<key/value pairs>]\n" );
		return;
	}

	int numberToSpawn = atoi(args.Argv(2));

	classname = args.Argv( 1 );
	dict.Set( "classname", classname );

	for (int ix=0; ix<numberToSpawn; ix++) {
		yaw = player->viewAngles.yaw + gameLocal.random.CRandomFloat()*10;
		dict.Set( "angle", va( "%f", yaw + 180 ) );
		org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, 1 )*(10+gameLocal.random.RandomFloat()*50);
		dict.Set( "origin", org.ToString() );

		for( i = 3; i < args.Argc() - 1; i += 2 ) {
			key = args.Argv( i );
			value = args.Argv( i + 1 );
			dict.Set( key, value );
		}

		//HACK
		if (!idStr::Icmpn(classname, "movable_", 8)) {
			idVec3 vel = idAngles(0,yaw,0).ToForward() * gameLocal.random.RandomFloat()*150;
			dict.SetVector("init_velocity", vel);
		}

		gameLocal.SpawnEntityDef( dict );
	}
}

void Cmd_ListDictionary_f( const idCmdArgs &args ) {
	const idDict		*dict=NULL;
	idEntity			*ent=NULL;
	const char			*classname=NULL;
	idStr				key;
	const idKeyValue	*kv=NULL;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() < 2 ) {
		gameLocal.Printf("Usage: Dict <classname> [key]\n");
		return;
	}

	// get classname
	classname = args.Argv( 1 );
	dict = gameLocal.FindEntityDefDict( classname );
	if (!dict) {
		gameLocal.Printf("Unknown entity definition: %s\n", classname);
		return;
	}

	// handle specific key
	if (args.Argc()==3) {
		key = args.Argv( 2 );

		kv = dict->FindKey(key);
		if (kv) {
			gameLocal.Printf("  %30s %s\n", kv->GetKey().c_str(), kv->GetValue().c_str());
		}
	}
	else {
		int num = dict->GetNumKeyVals();
		for (int ix=0; ix<num; ix++) {
			kv = dict->GetKeyVal(ix);
			if (kv) {
				gameLocal.Printf("  %30s %s\n", kv->GetKey().c_str(), kv->GetValue().c_str());
			}
		}
	}
}

// id has a version of this as well, but ours is better
// Trigger all entities matching given name or class
void Cmd_HHTrigger_f( const idCmdArgs &args ) {
	idVec3		origin;
	idAngles	angles;
	idPlayer	*player;
	const char	*classname;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() != 2 ) {
		gameLocal.Printf( "usage: trigger <name/class of entity to trigger>\n" );
		return;
	}

	// get name/class of entity
	classname = args.Argv( 1 );

	// find entity by name or classname
	int count=0;
	for (int i = 0; i < gameLocal.num_entities; i++) {
		idEntity *ent = gameLocal.entities[i];
		if ( ent && (!idStr::Icmp(ent->name, classname) || !idStr::Icmp(ent->GetClassname(), classname)) ) {
			ent->Signal( SIG_TRIGGER );
			ent->ProcessEvent( &EV_Activate, player );
			ent->TriggerGuis();
			count++;
		}
	}

	// failed by name. try by entity number
	if ( count == 0 ) {
		int entityNumber = atoi(args.Argv( 1 ));
		if ( entityNumber < gameLocal.num_entities ) {
			idEntity *ent = gameLocal.entities[entityNumber];
			if ( ent ) {
				ent->Signal( SIG_TRIGGER );
				ent->ProcessEvent( &EV_Activate, player );
				ent->TriggerGuis();
			}
		}
	}

	gameLocal.Printf( "Triggered %d entities\n", count );
}

// CJRPERSISTENTMERGE:  id has a version of this as well, but ours is better
// Remove all of a specified class from the level
void Cmd_HHRemove_f( const idCmdArgs &args ) {
	idPlayer	*player;
	const char	*classname;

	if ( args.Argc() < 2 ) {
		gameLocal.Printf("Usage: remove <entityclass/entityname>\n");
		return;
	}

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	// get name/class of entity
	classname = args.Argv( 1 );

	// find entity by name or classname
	int count = 0;
	for (int i = 0; i < gameLocal.num_entities; i++) {
		idEntity *ent = gameLocal.entities[i];
		if ( ent && (!idStr::Icmp(ent->name, classname) || !idStr::Icmp(ent->GetClassname(), classname)) ) {
			ent->PostEventMS( &EV_Remove, 0 );
			count++;
		}
	}

	gameLocal.Printf("%d %s entities removed\n", count, classname);
}

// Hide all of a specified class or a specific name
void Cmd_Hide_f( const idCmdArgs &args ) {
	idPlayer	*player;
	idEntity	*ent = NULL;
	const char	*classname;

	if ( args.Argc() < 2 ) {
		gameLocal.Printf("Usage: hide <entityclass/entityname>\n");
		return;
	}

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	// get name/class of entity
	classname = args.Argv( 1 );

	// find entity by name or classname
	int count = 0;
	for (int i = 0; i < gameLocal.num_entities; i++) {
		idEntity *ent = gameLocal.entities[i];
		if ( ent && (!idStr::Icmp(ent->name, classname) || !idStr::Icmp(ent->GetClassname(), classname)) ) {
			ent->Hide();
			count++;
		}
	}

	gameLocal.Printf("%d %s entities hidden\n", count, classname);
}

// Show all of a specified class or a specific name
void Cmd_Show_f( const idCmdArgs &args ) {
	idPlayer	*player;
	idEntity	*ent = NULL;
	const char	*classname;

	if ( args.Argc() < 2 ) {
		gameLocal.Printf("Usage: show <entityclass/entityname>\n");
		return;
	}

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	// get name/class of entity
	classname = args.Argv( 1 );

	// find entity by name or classname
	int count = 0;
	for (int i = 0; i < gameLocal.num_entities; i++) {
		idEntity *ent = gameLocal.entities[i];
		if ( ent && (!idStr::Icmp(ent->name, classname) || !idStr::Icmp(ent->GetClassname(), classname)) ) {
			ent->Show();
			count++;
		}
	}

	gameLocal.Printf("%d %s entities shown\n", count, classname);
}

/*
============
Cmd_SpawnDebrisMass_f
============
*/
void Cmd_SpawnDebrisMass_f(const idCmdArgs &args) {
	const char *temp;


	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() < 2 ) {
		gameLocal.Printf( "Usage: SpawnDebrisMass entityDef\n" );
		return;
	}

	temp = args.Argv( 1 );

	hhUtils::SpawnDebrisMass( temp, vec3_origin );
	

}

void Cmd_SetPlayerGravity_f( const idCmdArgs &args ) {
	idVec3 gravDirection;
	idStr temp;
	hhPlayer	*player;
	int argIndex = 0;

	if ( args.Argc() < 3 ) {
		gameLocal.Printf("Usage: SetPlayerGravity <x_amount> <y_amount> <z_amount>\n");
		return;
	}

	player = static_cast<hhPlayer*>( gameLocal.GetLocalPlayer() );
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	for( int ix = 0; ix < 3; ++ix ) {
		temp = args.Argv( ++argIndex );
		if( temp.Icmp("-") == 0 ) {
			temp = args.Argv( ++argIndex );
			gravDirection[ix] = -1 * atof( temp.c_str() );
		} else {
			gravDirection[ix] = atof( temp.c_str() );
		}
	}

	if( player->GetPhysics() ) {
		player->OrientToGravity( true );
		player->GetPhysics()->SetGravity( gravDirection );
	}
}


//=========================================================================
// Cmd_ClosePortal_f
//
// Closes a portal if it intersects my current bounds (for testing)
//=========================================================================
void Cmd_ClosePortal_f(const idCmdArgs &args) {
	if (!gameLocal.CheatsOk()) {
		return;
	}
	idPlayer *player = gameLocal.GetLocalPlayer();
	qhandle_t areaPortal = gameRenderWorld->FindPortal( player->GetPhysics()->GetAbsBounds() );
	if ( areaPortal ) {
		gameLocal.SetPortalState( areaPortal, PS_BLOCK_ALL );
		gameLocal.Printf("Portal closed\n");
	}
}

//=========================================================================
// Cmd_OpenPortal_f
//
// Opens a portal if it intersects my current bounds (for testing)
//=========================================================================
void Cmd_OpenPortal_f(const idCmdArgs &args) {
	if (!gameLocal.CheatsOk()) {
		return;
	}
	idPlayer *player = gameLocal.GetLocalPlayer();
	qhandle_t areaPortal = gameRenderWorld->FindPortal( player->GetPhysics()->GetAbsBounds() );
	if ( areaPortal ) {
		gameLocal.SetPortalState( areaPortal, PS_BLOCK_NONE );
		gameLocal.Printf("Portal opened\n");
	}
}

//=========================================================================
//
// Cmd_CallScriptFunc_f
//
//=========================================================================
void Cmd_CallScriptFunc_f( const idCmdArgs &args ) {
	if( !gameLocal.CheatsOk() ) {
		return;
	}

	if( args.Argc() <= 1 ) {
		gameLocal.Printf( "Usage: call <retKey> <script function name> <param1> <param2> ....\n" );
		return;
	}

	hhFuncParmAccessor info;
	idDict returnValue;

	hhUtils::SplitString( args, info.GetParms() );

	const char *funcName = info.GetParms()[0].c_str();
	if( funcName && *funcName ) {
		if( gameLocal.program.FindFunction( funcName ) == NULL ) {
			gameLocal.Printf( "Unknown function '%s'.\n", funcName );
			return;
		}
	}
	else {
		gameLocal.Printf( "Usage: call <retKey> <script function name> <param1> <param2> ....\n" );
		return;
	}

	info.ParseFunctionKeyValue( info.GetParms() );

	if (info.GetFunction()==NULL) {
		gameLocal.Printf( "Invalid function\n" );
		return;
	}

	if( info.GetFunction()->eventdef ) {
		gameLocal.Printf( "Can't call eventDefs (%s) from console\n", info.GetFunctionName() );
		return;
	}

	info.CallFunction( returnValue );
	if( info.GetReturnKey() && info.GetReturnKey()[0] ) {
		gameLocal.Printf( "%s: %s\n", info.GetReturnKey(), returnValue.GetString(info.GetReturnKey()) );
	}
}

//=========================================================================
//
// Cmd_PrintDDA_f
//
// Prints out the DDA information for the player
//=========================================================================
void Cmd_PrintDDA_f(const idCmdArgs &args) {
	if (!gameLocal.CheatsOk()) {
		return;
	}

	idPlayer *player = gameLocal.GetLocalPlayer();
	if( player && player->IsType(hhPlayer::Type)) {
		gameLocal.GetDDA()->PrintDDA();
	}
}

//=========================================================================
//
// Cmd_DDAExport_f
//
// Forces the dda system to export the dda information currently.
//=========================================================================
void Cmd_DDAExport_f(const idCmdArgs &args) {
	const char* filename = NULL;
	if( args.Argc() > 1 )
		filename = args.Argv(1);
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player && player->IsType(hhPlayer::Type) ) {
		gameLocal.GetDDA()->Export(filename);
	}
}

//=========================================================================
//
// Cmd_SetDDA_f
//
//=========================================================================

void Cmd_SetDDA_f(const idCmdArgs &args) {
	if( !gameLocal.CheatsOk() ) {
		return;
	}

	if( args.Argc() < 2 ) {
		gameLocal.Printf( "Usage: setdda <value>\n" );
		return;
	}

	float oldDDA = 0.5f;
	float newDDA = atof( args.Argv(1) );

	if ( gameLocal.GetDDA() ) {
		oldDDA = gameLocal.GetDDA()->GetDifficulty();
		gameLocal.GetDDA()->ForceDifficulty( newDDA );
	}

	gameLocal.Printf( "DDA changed from %0.4f to %0.4f\n", oldDDA, newDDA );
}


//=========================================================================
//
// Cmd_ToggleTalonTargets_f
//
//=========================================================================

void Cmd_ToggleTalonTargets_f(const idCmdArgs &args) {
	// Determine if the targets are visible (check the first talon target)
	if ( gameLocal.talonTargets.Num() > 0 && gameLocal.talonTargets[0]->fl.hidden ) { // one of them is hidden, so show all
		hhTalonTarget::ShowTargets();
	} else { // Visible, so hide them
		hhTalonTarget::HideTargets();
	}
}

//=========================================================================
//
// Cmd_DebugPointer_F
//
//=========================================================================
void Cmd_DebugPointer_f(const idCmdArgs &args) {
	if (!gameLocal.CheatsOk()) {
		return;
	}

#if INGAME_DEBUGGER_ENABLED
	g_debugger.SetBool( true );
	debugger.SetPointer(true);
	debugger.SetClassCollapse(false);
	debugger.SetMode(DEBUGMODE_GAMEINFO1);
#endif
}

//=========================================================================
//
// Cmd_Debug
//
//=========================================================================
void Cmd_Debug_f(const idCmdArgs &args) {
	if (!gameLocal.CheatsOk()) {
		return;
	}

	if (args.Argc() < 2) {
		gameLocal.Printf("Usage: Debug <EntityName>\n");
		g_debugger.SetBool( false );
		return;
	}

#if INGAME_DEBUGGER_ENABLED
	// find entity
	const char *entityName = args.Argv( 1 );
	idEntity *ent = gameLocal.FindEntity( entityName );

	g_debugger.SetBool( true );
	debugger.SetSelectedEntity(ent);
	debugger.SetPointer(false);
	debugger.SetClassCollapse(false);
	debugger.SetMode(DEBUGMODE_SPAWNARGS);
#endif

}

void Cmd_PossessPlayer_f(const idCmdArgs &args) {
	if (!gameLocal.CheatsOk()) {
		return;
	}

	idPlayer* pPlayer = gameLocal.GetLocalPlayer();
	if( pPlayer && pPlayer->IsType(hhPlayer::Type) ) {
		static_cast<hhPlayer*>(pPlayer)->Possess( NULL );
	}
}

void Cmd_UnpossessPlayer_f(const idCmdArgs &args) {
	if (!gameLocal.CheatsOk()) {
		return;
	}

	idPlayer* pPlayer = gameLocal.GetLocalPlayer();
	if( pPlayer && pPlayer->IsType(hhPlayer::Type) ) {
		static_cast<hhPlayer*>(pPlayer)->Unpossess();
	}
}

// save position to sys_SavedPosition
void Cmd_SavePosition_f( const idCmdArgs &args ) {
	if (!gameLocal.CheatsOk()) {
		return;
	}

	if (!gameLocal.GetLocalPlayer()) { //HUMANHEAD rww
		return;
	}

	idPlayer *player = gameLocal.GetLocalPlayer();
	idStr str;
	sprintf(str, "%s %.2f", player->GetPhysics()->GetOrigin().ToString(), player->viewAngles.yaw);
	sys_SavedPosition.SetString( str.c_str() );
}

// restore position from sys_SavedPosition
void Cmd_RestorePosition_f( const idCmdArgs &args ) {
	idVec3		origin;
	idAngles	angles;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if (!gameLocal.GetLocalPlayer()) { //HUMANHEAD rww
		return;
	}

	// Parse x y z yaw from sys_SavedPosition
	idParser src;
	idToken token;
	if (strlen(sys_SavedPosition.GetString()) > 0) {
		src.LoadMemory( sys_SavedPosition.GetString(), strlen( sys_SavedPosition.GetString() ), "position" );

		origin.x = src.ParseFloat();
		origin.y = src.ParseFloat();
		origin.z = src.ParseFloat();
		angles.Zero();
		angles.yaw = src.ParseFloat();

		gameLocal.Printf("Setting player position to: [%s] [%s]\n", origin.ToString(), angles.ToString());
		idPlayer *player = gameLocal.GetLocalPlayer();
		player->Teleport( origin, angles, NULL );
	}
}

void Cmd_PrintHandInfo_f( const idCmdArgs &args ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	
	hhHand::PrintHandInfo( player );

}

void Cmd_ToggleShuttle_f( const idCmdArgs &args ) {
	hhPlayer * player = static_cast<hhPlayer*>( gameLocal.GetLocalPlayer() );
	if ( !player || !gameLocal.CheatsOk() || !player->GetVehicleInterface() ) {
		return;
	}

	if ( player->GetVehicleInterface()->GetVehicle() ) {
		player->GetVehicleInterface()->GetVehicle()->EjectPilot();
	} else {
		idDict		dict;
		float yaw = player->viewAngles.yaw;
		dict.Set( "angle", va( "%f", yaw ) );
		dict.Set( "nodrop", va( "%d", 1 ) );
		idVec3 org = player->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 30 + idVec3( 0, 0, 1 ) * 150;
		dict.Set( "origin", org.ToString() );

		hhVehicle *shuttle = static_cast<hhVehicle*>(gameLocal.SpawnObject( "vehicle_shuttle", &dict ));
		if ( shuttle ) {
			shuttle->SetOrigin( org );
			shuttle->GetPhysics()->PutToRest();
			player->EnterVehicle( static_cast<hhVehicle*>(shuttle) );
		}
	}
}

void Cmd_KillThis_f( const idCmdArgs &args ) {
	hhPlayer * player = static_cast<hhPlayer*>( gameLocal.GetLocalPlayer() );
	if ( !player || !player->weapon.IsValid() || !gameLocal.CheatsOk() ) {
		return;
	}
	idEntity *ent = gameLocal.GetTraceEntity( player->weapon->GetEyeTraceInfo() );
	if ( ent ) {
		ent->Damage( player, player, vec3_zero, "damage_instantdeath", 9999.0f, 0 );
	}
}

void Cmd_PrintThis_f( const idCmdArgs &args ) {
	hhPlayer * player = static_cast<hhPlayer*>( gameLocal.GetLocalPlayer() );
	if ( !player || !player->weapon.IsValid() || !gameLocal.CheatsOk() ) {
		return;
	}
	idEntity *ent = gameLocal.GetTraceEntity( player->weapon->GetEyeTraceInfo() );
	idMapFile *mapFile = gameLocal.GetLevelMap();

	// trace didn't find an entity.  try 
	if ( !ent ) {
		int entityNumber = atoi(args.Argv( 1 ));
		if ( entityNumber < gameLocal.num_entities ) {
			idEntity *ent = gameLocal.entities[entityNumber];
			if ( ent ) {
				ent->Signal( SIG_TRIGGER );
				ent->ProcessEvent( &EV_Activate, player );
				ent->TriggerGuis();
			}
		}
	}

	if ( ent && !ent->IsType( idWorldspawn::Type ) && mapFile ) {
		idMapEntity * mapEnt = mapFile->FindEntity( ent->GetName() );
		if ( mapEnt ) {
			gameLocal.Printf( "[%s]\n", ent->GetName() );
			for ( int i=0;i<mapEnt->epairs.GetNumKeyVals();i++) {
				gameLocal.Printf( "  \"%s\" \"%s\"\n", mapEnt->epairs.GetKeyVal(i)->GetKey().c_str(), mapEnt->epairs.GetKeyVal(i)->GetValue().c_str());
			}
		} else {
			gameLocal.Printf( "  No non-default map keys.\n" );
		}
		if ( ent->IsType( hhMonsterAI::Type ) ) {
			hhMonsterAI *entAI = static_cast<hhMonsterAI*>(ent);
			if ( entAI ) {
				entAI->PrintDebug();
			}
		}
	}
}

void Cmd_TriggerThis_f( const idCmdArgs &args ) {
	hhPlayer * player = static_cast<hhPlayer*>( gameLocal.GetLocalPlayer() );
	if ( !player || !player->weapon.IsValid() || !gameLocal.CheatsOk() ) {
		return;
	}
	idEntity *ent = gameLocal.GetTraceEntity( player->weapon->GetEyeTraceInfo() );
	if ( ent ) {
		ent->PostEventMS( &EV_Activate, 0, player );
	}
}

void Cmd_CheckNodes_f( const idCmdArgs &args ) {
	bool found = false;
	if ( !gameLocal.CheatsOk() ) {
		return;
	}
	//loop through and find the next valid aas
	idAASLocal *aas = static_cast<idAASLocal*>(gameLocal.GetAAS( 0 ));
	if ( !aas ) {
		gameLocal.Printf( "No aas48 found on this map\n" );
		return;
	}

	idEntity *ent;
	idBounds bounds;
	idVec3 size;
	gameLocal.Printf( "Checking cover node placement...\n" );
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( !ent || !ent->spawnArgs.GetBool( "ainode", "0" ) ) {
			continue;
		}
		found = true;
		size = aas->GetSettings()->boundingBoxes[0][1];
		bounds[0] = -size;
		size.z = 32.0f;
		bounds[1] = size;

		//check reachability 
		if ( !aas->PointReachableAreaNum( ent->GetOrigin(), bounds, AREA_REACHABLE_WALK ) ) {
			common->Printf( "    %s: " S_COLOR_RED "NOT REACHABLE\n", ent->GetName() );
		} else {
			common->Printf( "    %s: " S_COLOR_GREEN " REACHABLE\n", ent->GetName() );
		}
	}
	if ( !found ) {
		gameLocal.Printf( "    None found.\n" );
	}
}

void Cmd_NextAas_f( const idCmdArgs &args ) {
	bool found = false;
	if ( !gameLocal.CheatsOk() ) {
		return;
	}
	//loop through and find the next valid aas
	int current = aas_test.GetInteger() + 1;
	for ( int i=0;i<gameLocal.NumAAS()+1;i++ ) {
		if ( gameLocal.GetAAS( current ) ) {
			found = true;
			break;
		}
		if ( current >= gameLocal.NumAAS() ) {
			current = 0;
		} else {
			current++;
		}
	}
	if ( found ) {
		aas_test.SetInteger( current );
	} else {
		gameLocal.Printf( "No aas found for any monsters\n" );
		return;
	}

	//print out aas name
	if ( aas_test.GetInteger() <= gameLocal.NumAAS() ) {
		idAAS *aas = gameLocal.GetAAS( aas_test.GetInteger() );
		if ( !aas ) {
			gameLocal.Printf( "No aas #%d loaded\n", aas_test.GetInteger() );
		} else {
			idAASLocal *aasLocal = static_cast<idAASLocal*>(aas);
			if ( aasLocal && aasLocal->GetName() ) {
				gameLocal.Printf( "Testing aas #%i: %s\n", aas_test.GetInteger(), aasLocal->GetName() );
			}
		}
	} else {
		gameLocal.Printf( "Unable to find aas for #%i\n", aas_test.GetInteger() );
	}
}

void Cmd_GiveEnergy_f( const idCmdArgs &args ) {
	if ( !gameLocal.CheatsOk() ) {
		return;
	}
	if( args.Argc() < 2 ) {
		gameLocal.Printf( "Usage: giveEnergy <type> (railgun,sunbeam,freeze,plasma)\n" );
		return;
	}
	if ( !args.Argv( 1 ) || !args.Argv( 1 )[0] ) {
		return;
	}

	if ( !gameLocal.GetLocalPlayer() || !gameLocal.GetLocalPlayer()->weapon.IsValid() ) {
		return;
	}
	if ( !gameLocal.GetLocalPlayer()->weapon->IsType( hhWeaponSoulStripper::Type ) ) {
		return;
	}
	hhWeaponSoulStripper *weapon = static_cast<hhWeaponSoulStripper*>(gameLocal.GetLocalPlayer()->weapon.GetEntity());
	weapon->GiveEnergy( va( "fireinfo_%s", args.Argv( 1 ) ), true );
}

//=========================================================================
//
// Cmd_AI_ReactionSet_f
//
// sets the reaction that we want to inspect.  Used with Cmd_AI_ReactionCheck_f
//=========================================================================
void Cmd_AI_ReactionSet_f(const idCmdArgs &args ) {
	idEntity*		entity = NULL;
	idPlayer*		player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() ) {
		return;
	}
	if( args.Argc() < 2 ) {
		gameLocal.Printf( "usage: ai_reactionset <entity>\n" );
		return;
	}

	entity = gameLocal.FindEntity( args.Argv(1) );
	if( !entity ) {
		gameLocal.Printf( "Unable to locate entity '%s' specified\n", args.Argv(1) );
		return;
	}

	//restart the inspector if he is going
	if( gameLocal.inspector != NULL ) {
		hhAIInspector::RestartInspector( entity, player );
	}
	else {
		gameLocal.Printf( "Need to set a inspector before setting the reaction\n" );
		return;
	}
}

//=========================================================================
//
// Cmd_AI_ReactionCheck_f
//
// checks the reaction by spawning a monster that attempts to use it.  Used with Cmd_AI_ReactionSet_f (optionally)
//=========================================================================
void Cmd_AI_ReactionCheck_f(const idCmdArgs &args ) {
	idEntity*		react_entity = NULL;
	idPlayer*		player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if( args.Argc() < 2 ) {
		gameLocal.Printf( "Usage: ai_reactioncheck <entity> [optional: <reaction>] \n" );
		return;
	}

	if( args.Argc() >= 3 ) {
		react_entity = gameLocal.FindEntity( args.Argv( 2 ) );
		if( !react_entity ) {
			gameLocal.Printf( "Invalid entity '%s' specified as reaction\n", args.Argv( 2 ) );
			return;
		}
	}

	hhAIInspector::RestartInspector( args.Argv(1), react_entity, player );
}

//=========================================================================
//
// P_InitConsoleCommands
//
// Let the system know about all of our commands
// so it can perform tab completion
//=========================================================================

void P_InitConsoleCommands( void ) {
	cmdSystem->AddCommand("testtext",				Cmd_TestText_f,				CMD_FL_SYSTEM|CMD_FL_CHEAT,	"fixup in-game text codes" );
	cmdSystem->AddCommand("getbinds",				Cmd_GetBinds_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"print entities bound to an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand("spiritwalkmode",			Cmd_SpiritWalkMode_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"spiritwalk sound mode" );
	cmdSystem->AddCommand("dialogmode",				Cmd_DialogMode_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"dialog sound mode" );
	cmdSystem->AddCommand("healthPulse",			Cmd_TestHealthPulse_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"healthpulse" );
	cmdSystem->AddCommand("spiritPulse",			Cmd_TestSpiritPulse_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"spiritpulse" );
	cmdSystem->AddCommand("entitysize",				Cmd_EntitySize_f,			CMD_FL_SYSTEM|CMD_FL_CHEAT,	"print size taken by entities" );
	cmdSystem->AddCommand("assert",					Cmd_Assert_f,				CMD_FL_SYSTEM,				"assert so you can use debugger" );
	cmdSystem->AddCommand("dict",					Cmd_ListDictionary_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"print a dictionary", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand("trigger",				Cmd_HHTrigger_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"trigger all entities or a class or name", idGameLocal::ArgCompletion_EntityOrClassName ); // CJRPERSISTENTMERGE:  id has a version of this, but ours is better
	cmdSystem->AddCommand("remove",					Cmd_HHRemove_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"remove all entities of a class or name", idGameLocal::ArgCompletion_EntityOrClassName ); // CJRPERSISTENTMERGE:  id has a version of this, but ours is better
	cmdSystem->AddCommand("hide",					Cmd_Hide_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"hide all entities of a class or name", idGameLocal::ArgCompletion_EntityOrClassName );
	cmdSystem->AddCommand("show",					Cmd_Show_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"show all entities of a class or name", idGameLocal::ArgCompletion_EntityOrClassName );
	cmdSystem->AddCommand("spawnDebrisMass",		Cmd_SpawnDebrisMass_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"spawn a debris mass"  );
	cmdSystem->AddCommand("setPlayerGravity",		Cmd_SetPlayerGravity_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"set player gravity" );
	cmdSystem->AddCommand("OpenPortal", 			Cmd_OpenPortal_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"open a portal" );
	cmdSystem->AddCommand("ClosePortal", 			Cmd_ClosePortal_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"close a portal" );
	cmdSystem->AddCommand("debug_pointer",			Cmd_DebugPointer_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"switch to debugger in pointer mode" );
	cmdSystem->AddCommand("debug",					Cmd_Debug_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"debug a specific entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand("putpos",					Cmd_SavePosition_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"save location" );
	cmdSystem->AddCommand("getpos",					Cmd_RestorePosition_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"teleport to saved location" );
	cmdSystem->AddCommand("printHandInfo",			Cmd_PrintHandInfo_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"print info about hand" );
	cmdSystem->AddCommand("dormant",				Cmd_Dormant_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"mark entities of a class as NOT neverdormant", idGameLocal::ArgCompletion_ClassName );
	cmdSystem->AddCommand("neverdormant",			Cmd_UnDormant_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"mark entities of a class as neverdormant", idGameLocal::ArgCompletion_ClassName );
	cmdSystem->AddCommand("spawnabunch",			Cmd_SpawnABunch_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"spawn a specified number of entities" );
	cmdSystem->AddCommand("getmapname",				Cmd_GetMapName_f,			CMD_FL_SYSTEM,				"print current map name to console" );
 
#if INGAME_PROFILER_ENABLED
 	cmdSystem->AddCommand("Prof_out", 				Profiler_ZoomOut_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_mode",				Profiler_Mode_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_ms",				Profiler_MSMode_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_smooth",			Profiler_Smooth_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_capture", 			Profiler_ToggleCapture_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_toggle",			Profiler_Toggle_f,			CMD_FL_TOOL|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_release",			Profiler_Release_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in0",				Profiler_In0_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in1",				Profiler_In1_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in2",				Profiler_In2_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in3",				Profiler_In3_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in4",				Profiler_In4_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in5",				Profiler_In5_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in6",				Profiler_In6_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in7",				Profiler_In7_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in8",				Profiler_In8_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in9",				Profiler_In9_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in10",				Profiler_In10_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in11",				Profiler_In11_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in12",				Profiler_In12_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in13",				Profiler_In13_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
	cmdSystem->AddCommand("Prof_in14",				Profiler_In14_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Profiler command" );
#endif

	cmdSystem->AddCommand( "printDDA",				Cmd_PrintDDA_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"print dda info" );
	cmdSystem->AddCommand( "exportDDA",				Cmd_DDAExport_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"export dda info to file" );
	cmdSystem->AddCommand( "possessPlayer",			Cmd_PossessPlayer_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"possess the player" );
	cmdSystem->AddCommand( "unpossessPlayer",		Cmd_UnpossessPlayer_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"unpossess the player" );
	cmdSystem->AddCommand( "toggleTalonTargets",	Cmd_ToggleTalonTargets_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"toggle talon targets" );
	cmdSystem->AddCommand( "call",					Cmd_CallScriptFunc_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"call a script function from console" );
	cmdSystem->AddCommand( "setdda",				Cmd_SetDDA_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"set dda difficulty to specific value" );

	cmdSystem->AddCommand( "ai_reactionset",		Cmd_AI_ReactionSet_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"sets the target reaction which will be used for ai_reactioncheck",	idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "ai_reactioncheck",		Cmd_AI_ReactionCheck_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"spawns a creature and forces him to use a specific reaction", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand( "toggleShuttle",			Cmd_ToggleShuttle_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"magically enter or exit a shuttle" );
	cmdSystem->AddCommand( "killThis",				Cmd_KillThis_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"kill thing that player is pointing at" );
	cmdSystem->AddCommand( "triggerThis",			Cmd_TriggerThis_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"trigger thing that player is pointing at" );
	cmdSystem->AddCommand( "printThis",			Cmd_PrintThis_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"print extra map keys of the thing that player is pointing at" );
	cmdSystem->AddCommand( "nextaas",				Cmd_NextAas_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"increment aas_test and print out info" );
	cmdSystem->AddCommand( "giveEnergy",			Cmd_GiveEnergy_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"give energy gun energy" );
	cmdSystem->AddCommand( "checkNodes",			Cmd_CheckNodes_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Check reachability of ai_generic_nodes" );
}
