// sys_debugger.cpp
//
// HUMANHEAD: debugger functions
// 

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/* TODO:
	make dormant filter?
	make physics filter?
	Optimize:
		preallocate all the string ptrs in DisplayCell and don't free them every frame
*/

#if INGAME_DEBUGGER_ENABLED

hhDebugger debugger;

int hhDebugger::sortColumn = 1;
bool hhDebugger::sortAscending = false;

//=========================================================================
//
// hhDisplayCell
//
//=========================================================================

hhDisplayCell::hhDisplayCell()	{
	string = NULL;
	type = COLUMNTYPE_NUMERIC;
	value = 0.0f;
}
hhDisplayCell::~hhDisplayCell()	{
	if (string) {
		delete string;
		string = NULL;
	}
}
inline void hhDisplayCell::AllocateString() {
	if (!string) {
		string = new idStr;
	}
}
inline void hhDisplayCell::operator=( const hhDisplayCell &cell ) {
	if (cell.string) {
		AllocateString();
		*string = *cell.string;
	}
	type = cell.type;
	value = cell.value;
}
inline void hhDisplayCell::operator=(const idStr &text) {
	AllocateString();
	*string = text;
	type = COLUMNTYPE_ALPHA;
}
inline void hhDisplayCell::operator=(const char *text) {
	AllocateString();
	*string = text;
	type = COLUMNTYPE_ALPHA;
}
inline void hhDisplayCell::operator=(const int i) {
	value = (float)i;
	type = COLUMNTYPE_NUMERIC;
}
inline void hhDisplayCell::operator=(const float f) {
	value = f;
	type = COLUMNTYPE_NUMERIC;
}
inline hhDisplayCell & hhDisplayCell::operator+=(const int i) {
	if (type ==  COLUMNTYPE_ALPHA) {
		AllocateString();
		*this = atoi(String()) +  i;
	}
	else {
		value += (float)i;
	}
	type = COLUMNTYPE_NUMERIC;
	return *this;
}
inline hhDisplayCell & hhDisplayCell::operator+=(const float f) {
	if (type == COLUMNTYPE_ALPHA) {
		AllocateString();
		*this = (float)atof(String()) +  f;
	}
	else {
		value += f;
	}
	type = COLUMNTYPE_NUMERIC;
	return *this;
}
inline bool hhDisplayCell::IsInt() const {
	return ((type == COLUMNTYPE_NUMERIC) && (value == (int)value) );
}
inline int hhDisplayCell::StringLength() {
	if (type == COLUMNTYPE_NUMERIC) {
		return strlen(String());
	}
	AllocateString();
	return string->Length();
}
inline EColumnDataType hhDisplayCell::Type() const {
	return type;
}
inline float hhDisplayCell::Float() const {
	if (type == COLUMNTYPE_ALPHA) {
		return (float)atof(String());
	}
	return value;
}
inline int hhDisplayCell::Int() const {
	if (type == COLUMNTYPE_ALPHA) {
		return atoi(String());
	}
	return (int)value;
}
const char *hhDisplayCell::String() const {
	static idStr workStr;
	if (type == COLUMNTYPE_NUMERIC) {
		if (IsInt()) {
			workStr = idStr((int)value);
		}
		else {
			workStr = idStr(value);
		}
		return workStr.c_str();
	}
	if (!string) {
		return "";
	}
	return string->c_str();
}

//=========================================================================
//
// hhDebugger
//
//=========================================================================

hhDebugger::hhDebugger() {
	bInitialized = false;
	bClassCollapse = true;
	bDrawEntities = false;
	bPointer = false;
	includeFilters = excludeFilters = 0;
	filterClass = NULL;
	potentialSet.Clear();
	displayList.Clear();
	selectedEntity = NULL;
	sortColumn = 1;
	sortAscending = false;
	debugMode = DEBUGMODE_NONE;
	gui=NULL;
	displayColumnsUsed = 0;

	xaxis.Set(25,  0,  0);
	yaxis.Set( 0, 25,  0);
	zaxis.Set( 0,  0, 25);
}

hhDebugger::~hhDebugger() {
}

void hhDebugger::Shutdown() {
	// Debugger must be shutdown manually because of the dict memeber which
	// must be Cleared before the global string pool is emptied
	dict.Clear();
	potentialSet.Clear();
	displayList.Clear();
	// release gui resources
	bInitialized = false;
}

void hhDebugger::Initialize() {
	if (!gui) {
		gui = uiManager->FindGui("guis/debugger.gui", true);
	}

	SetMode(DEBUGMODE_STATS);
	SetClassCollapse(true);
	SetDrawEntities(false);
	SetPointer(false);
	ClearAllFilters();
	ExcludeFilter(FILTER_CLOSE);
	ExcludeFilter(FILTER_DISTANT);

	bInitialized = true;
}

void hhDebugger::Reset() {
	selectedEntity = NULL;
	filterClass = NULL;
	Initialize();
}


//=========================================================================
//
// hhDebugger::CaptureInput
//
//=========================================================================
void hhDebugger::CaptureInput(bool bCapture) {
	if (bCapture && !bInteractive) {
		// Just becoming interactive
		const char *cmds = gui->Activate(true, gameLocal.time);
		HandleGuiCommands(cmds);
	}
	else if (bInteractive && !bCapture) {
		// Just becoming non-interactive
		const char *cmds = gui->Activate(false, gameLocal.time);
		HandleGuiCommands(cmds);
	}

	bInteractive = bCapture;
}

//=========================================================================
//
// hhDebugger::ColorForIndex
//
//=========================================================================
idVec4 hhDebugger::ColorForIndex(int index, float alpha) {
	idVec4 color;
	int bitmask = (index % 7) + 1;
	color.x = (bitmask & 1) ? 1.0f : 0.0f;
	color.y = (bitmask & 2) ? 1.0f : 0.0f;
	color.z = (bitmask & 4) ? 1.0f : 0.0f;
	color.w = alpha;
	return color;
}

//=========================================================================
//
// hhDebugger::ColorForEntity
//
//=========================================================================
idVec4 hhDebugger::ColorForEntity(idEntity *ent, float alpha) {
	// Choose color based on editor_color if possible
	idVec3 color3;
	idVec4 color;
	if (ent->spawnArgs.GetVector("editor_color", "0 0 1", color3)) {
		color.x = color3.x;
		color.y = color3.y;
		color.z = color3.z;
		color.w = alpha;
	}
	else {
		color = ColorForIndex(ent->entityNumber, alpha);
	}
	return color;
}

//=========================================================================
//
// hhDebugger::SetSortToColumn
//
//=========================================================================
void hhDebugger::SetSortToColumn(int col) {
	if (hhDebugger::sortColumn == col) {
		hhDebugger::sortAscending ^= 1;
	}
	else {
		hhDebugger::sortColumn = col;
	}
}

//=========================================================================
//
// Filter routines
//
//=========================================================================
int hhDebugger::IndexForFilter(int filter) {	//OBS
	int index = hhMath::ILog2(filter);
	return index;
}

int hhDebugger::FilterForIndex(int index) {
	return 1<<index;
}

bool hhDebugger::FilterIncluded(int filterIndex) {
	int filter = FilterForIndex(filterIndex);
	return (includeFilters & filter) != 0;
}

bool hhDebugger::FilterExcluded(int filterIndex) {
	int filter = FilterForIndex(filterIndex);
	return (excludeFilters & filter) != 0;
}

void hhDebugger::IncludeFilter(int filterIndex) {
	int filter = FilterForIndex(filterIndex);
	includeFilters |= filter;
	excludeFilters &= ~filter;
	if (gui) {
		gui->SetStateInt(va("filter%i", filterIndex), FILTERSTATE_INCLUDED);
		gui->HandleNamedEvent("SetFilter");
	}
}

void hhDebugger::ExcludeFilter(int filterIndex) {
	int filter = FilterForIndex(filterIndex);
	excludeFilters |= filter;
	includeFilters &= ~filter;
	if (gui) {
		gui->SetStateInt(va("filter%i", filterIndex), FILTERSTATE_EXCLUDED);
		gui->HandleNamedEvent("SetFilter");
	}
}

void hhDebugger::ClearFilter(int filterIndex) {
	int filter = FilterForIndex(filterIndex);
	includeFilters &= (~filter);
	excludeFilters &= (~filter);
	if (gui) {
		gui->SetStateInt(va("filter%i", filterIndex), FILTERSTATE_NONE);
		gui->HandleNamedEvent("SetFilter");
	}
}

void hhDebugger::ClearAllFilters() {
	for (int ix=0; ix<NUM_FILTERS; ix++) {
		ClearFilter(ix);
	}
}

// Toggles between: Ignore, Include, Exclude
void hhDebugger::Toggle3wayFilter(int filterIndex) {
	if (FilterIncluded(filterIndex)) {
		assert(!FilterExcluded(filterIndex));
		ExcludeFilter(filterIndex);
	}
	else if (FilterExcluded(filterIndex)) {
		assert(!FilterIncluded(filterIndex));
		ClearFilter(filterIndex);
	}
	else {
		IncludeFilter(filterIndex);
	}
}

//=========================================================================
//
// hhDebugger::ClassForRow
//
//=========================================================================
idTypeInfo *hhDebugger::ClassForRow(int row) {
	const char *classname = NULL;
	if (row < displayList.Num()) {
		classname = displayList[row].column[0].String();
		return idClass::GetClass(classname);
	}
	return NULL;
}

//=========================================================================
//
// hhDebugger::EntityForRow
//
//=========================================================================
idEntity *hhDebugger::EntityForRow(int row) {
	const char *name = NULL;
	if (row < displayList.Num()) {
		name = displayList[row].column[0].String();
		return gameLocal.FindEntity( name );
	}
	return NULL;
}

void hhDebugger::SetMode(EDebugMode mode) {
	debugMode = mode;
	if (gui) {
		// Clear list
		for (int row=0; row<DEBUG_NUM_ROWS; row++) {
			gui->DeleteStateVar( va( "listStats_item_%i", row ) );
		}

		gui->SetStateInt("mode", debugMode);
		gui->HandleNamedEvent("SetMode");
	}
}

void hhDebugger::SetClassCollapse(bool on) {
	bClassCollapse = on;
	if (gui) {
		gui->SetStateBool("classcollapse", bClassCollapse);
		gui->HandleNamedEvent("SetClassCollapse");
	}
}

void hhDebugger::SetDrawEntities(bool on) {
	bDrawEntities = on;
	if (gui) {
		gui->SetStateBool("drawEntities", bDrawEntities);
		gui->HandleNamedEvent("SetDrawEntities");
	}
}

void hhDebugger::SetPointer(bool on) {
	bPointer = on;
	if (gui) {
		gui->SetStateBool("pointer", bPointer);
		gui->HandleNamedEvent("SetPointer");
	}
}

void hhDebugger::SetSelectedEntity(idEntity *ent) {
	selectedEntity = ent;
}


//=========================================================================
//
// hhDebugger::HandleGuiCommands
//
//=========================================================================
bool hhDebugger::HandleGuiCommands(const char *commands) {
	bool ret = false;

	if (commands && commands[0]) {
		idLexer src;
		src.LoadMemory( commands, strlen( commands ), "guiCommands" );

		if (HandleSingleGuiCommand(gameLocal.GetLocalPlayer(), &src)) {
			ret = true;
		}
	}
	return ret;
}

//=========================================================================
//
// hhDebugger::HandleSingleGuiCommand
//
//=========================================================================
bool hhDebugger::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {
	idToken token;

	if (!src->ReadToken(&token)) {
		return false;
	}
	else if (token == ";") {
		return false;
	}
	else if (token.IcmpPrefix("guicmd_title_") == 0) {			// "title_cNN"
		token.ToLower();
		token.Strip("guicmd_title_c");							// "NN"
		int col = atoi(token.c_str());
		SetSortToColumn(col);
		return true;
	}
	else if (token.IcmpPrefix("guicmd_select") == 0) {			// "guicmd_select"
		int row = gui->State().GetInt("listStats_sel_0", "-1");
		if (row >= 0) {
			TranslateRowCommand(row);
		}
	}
	else if (token.IcmpPrefix("guicmd_cyclemode") == 0) {		// "guicmd_cyclemode"
		SetMode( (EDebugMode)((GetMode() + 1) % NUM_DEBUGMODES) );
	}
	else if (token.IcmpPrefix("guicmd_drawentities") == 0) {	// "guicmd_drawentities"
		SetDrawEntities(!GetDrawEntities());
	}
	else if (token.IcmpPrefix("guicmd_classcollapse") == 0) {	// "guicmd_classcollapse"
		SetClassCollapse(!GetClassCollapse());
	}
	else if (token.IcmpPrefix("guicmd_pointer") == 0) {			// "guicmd_pointer"
		SetPointer(!GetPointer());
	}
	else if (token.IcmpPrefix("guicmd_filter") == 0) {			// "guicmd_filter"
		token.ToLower();
		token.Strip("guicmd_filter");							// "NN"
		int filterIndex = atoi(token.c_str());

		// toggle 3-way state
		Toggle3wayFilter(filterIndex);

		// If turning off the class filter, revert back to ClassCollapse
		if (filterIndex == FILTER_CLASS && !FilterIncluded(FILTER_CLASS)) {
			SetClassCollapse(true);
		}
	}

	src->UnreadToken(&token);
	return false;
}

//=========================================================================
//
// hhDebugger::TranslateRowCommand
//
//=========================================================================
void hhDebugger::TranslateRowCommand(int row) {
	if (GetMode() != DEBUGMODE_STATS) {
		return;
	}

	// clicked on a row
	if (GetClassCollapse()) {
		idTypeInfo *clickedClass = ClassForRow(row);
		if (clickedClass) {
			IncludeFilter(FILTER_CLASS);
			filterClass = clickedClass;
			SetClassCollapse(false);
		}
	}
	else {
		idEntity *clickedEntity = EntityForRow(row);
		if (clickedEntity) {
			if (!selectedEntity.IsValid() || clickedEntity != selectedEntity.GetEntity()) {
				selectedEntity = clickedEntity;
			}
			else {
				if (GetMode()==DEBUGMODE_STATS) {
					SetMode(DEBUGMODE_GAMEINFO1);
				}
			}
		}
	}
}

//=========================================================================
//
// hhDebugger::DeterminePotentialSet
//
//=========================================================================
void hhDebugger::DeterminePotentialSet() {

	float maxDistSquared = g_maxShowDistance.GetFloat()*g_maxShowDistance.GetFloat();
	float minDistSquared = DEBUG_MIN_ENT_DIST*DEBUG_MIN_ENT_DIST;
	idPlayer *player = gameLocal.GetLocalPlayer();
	idVec3 playerPosition = player->GetEyePosition();
	idEntity *ent;

	potentialSet.Clear();
/*FIXME: Should use this format instead for speed
for ( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
    if ( ent->IsType( idLight::Type ) ) {
        idLight *light = static_cast<idLight *>(ent);
    }
}*/
	//FIXME: Use gameEdit->FindEntity ???
	for (int ix=0; ix<gameLocal.num_entities; ix++) {
		ent = gameLocal.entities[ix];
		if (!ent) {
			continue;
		}

		// Apply all filters to entity list, doing the cheap ones first

		if (FilterIncluded(FILTER_CLASS) && (ent->GetType() != filterClass) ) {
			continue;
		}
		if (FilterExcluded(FILTER_CLASS) && (ent->GetType() == filterClass) ) {
			continue;
		}

		if (FilterIncluded(FILTER_VISIBLE) && ent->IsHidden()) {	// worthless, replace with more specific active filters (THINK,ANIM,UPDATEVISUALS,PHYSICS)
			continue;
		}
		if (FilterExcluded(FILTER_VISIBLE) && !ent->IsHidden()) {
			continue;
		}

		if (FilterIncluded(FILTER_ACTIVE) && !ent->IsActive()) {
			continue;
		}
		if (FilterExcluded(FILTER_ACTIVE) && ent->IsActive()) {
			continue;
		}

		if (FilterIncluded(FILTER_MONSTERS) && !ent->IsType(idAI::Type)) {
			continue;
		}
		if (FilterExcluded(FILTER_MONSTERS) && ent->IsType(idAI::Type)) {
			continue;
		}

		if (FilterIncluded(FILTER_HASMODEL) && ent->GetModelDefHandle() == -1) {
			continue;
		}
		if (FilterExcluded(FILTER_HASMODEL) && ent->GetModelDefHandle() != -1) {
			continue;
		}

		if (FilterIncluded(FILTER_EXPENSIVE) && ent->thinkMS < g_expensiveMS.GetFloat()) {
			continue;
		}
		if (FilterExcluded(FILTER_EXPENSIVE) && ent->thinkMS > g_expensiveMS.GetFloat()) {
			continue;
		}

		if (FilterIsActive(FILTER_ANIMATING)) {
			bool isAnimating = ent->GetAnimator() && ent->GetAnimator()->IsAnimating( gameLocal.time );
			if (FilterIncluded(FILTER_ANIMATING) && !isAnimating) {
				continue;
			}
			if (FilterExcluded(FILTER_ANIMATING) && isAnimating) {
				continue;
			}
		}

		if (FilterIsActive(FILTER_DORMANT)) {
			
			bool isDormant = ent->fl.isDormant;
			if (FilterIncluded(FILTER_DORMANT) && !isDormant) {
				continue;
			}
			if (FilterExcluded(FILTER_DORMANT) && isDormant) {
				continue;
			}
		}

		if (FilterIsActive(FILTER_ACTIVESOUNDS)) {
			bool playingSounds = ent->GetSoundEmitter() && ent->GetSoundEmitter()->CurrentlyPlaying();
			if (FilterIncluded(FILTER_ACTIVESOUNDS) && !playingSounds) {
				continue;
			}
			if (FilterExcluded(FILTER_ACTIVESOUNDS) && playingSounds) {
				continue;
			}
		}

		if (FilterIsActive(FILTER_CLOSE) || FilterIsActive(FILTER_DISTANT)) {
			idVec3 entPos = ent->GetPhysics()->GetOrigin();
			idVec3 toEnt = entPos - playerPosition;
			float distsqr = toEnt.LengthSqr();
			if (FilterIncluded(FILTER_CLOSE) && (distsqr > minDistSquared)) {
				continue;
			}
			if (FilterExcluded(FILTER_CLOSE) && (distsqr < minDistSquared)) {
				continue;
			}
			if (FilterIncluded(FILTER_DISTANT) && (distsqr < maxDistSquared)) {
				continue;
			}
			if (FilterExcluded(FILTER_DISTANT) && (distsqr > maxDistSquared)) {
				continue;
			}
		}

		potentialSet.Append(ent);
	}
}

//=========================================================================
//
// hhDebugger::DetermineSelectionSet
//
//=========================================================================
void hhDebugger::DetermineSelectionSet() {

	// Clear selectedEntity if it has been removed
//	if (selectedEntity && !gameLocal.entities[selectedEntity->entityNumber]) {
//		selectedEntity = NULL;
//	}

	if (!GetPointer()) {
		return;
	}

	idPlayer *player = gameLocal.GetLocalPlayer();
	idVec3 playerPosition = player->GetEyePosition();
	idVec4 color;
	idVec3 entPos;
	idVec3 toEnt;
	idEntity *ent = NULL;
	idEntity *bestEnt = NULL;
	float bestScore = 0.0f;

	int num = potentialSet.Num();
	for (int ix=0; ix<num; ix++) {
		ent = potentialSet[ix];

		// Judge "most pointed at" entity based on player view axis and distance
		entPos = ent->GetPhysics()->GetOrigin();
		toEnt = entPos - playerPosition;
		float dist = toEnt.Length();
		float score = (toEnt * (player->viewAngles.ToMat3()[0])) / dist;
		if (score > bestScore) {
			bestScore = score;
			bestEnt = ent;
		}
	}

	selectedEntity = bestEnt;
}

//=========================================================================
//
// idListDefaultCompare<hhDisplayItem>
//
// Compares two pointers to hhDisplayItem.  Used to sort.
//=========================================================================
template<>
ID_INLINE int idListSortCompare<hhDisplayItem>( const hhDisplayItem *a, const hhDisplayItem *b ) {
	int sortColumn = hhDebugger::sortColumn;

	if (hhDebugger::sortAscending) {
		if (a->column[sortColumn].Type() == COLUMNTYPE_ALPHA) {
			return ( idStr::Icmp(b->column[sortColumn].String(), a->column[sortColumn].String() ) );
		}
		else {	// Sort numerically - need to multiply up because we're comparing integers
			return (int)(1000.0 * (	a->column[sortColumn].Float() -
									b->column[sortColumn].Float() ));	// sort by percentage
		}
	}
	else {
		if (a->column[sortColumn].Type() == COLUMNTYPE_ALPHA) {
			return ( idStr::Icmp(a->column[sortColumn].String(), b->column[sortColumn].String() ) );
		}
		else {	// Sort numerically - need to multiply up because we're comparing integers
			return (int)(1000.0 * (	b->column[sortColumn].Float() -
									a->column[sortColumn].Float() ));	// sort by percentage
		}
	}
}

const char *GetThinkFlags(idEntity *ent) {
	static char buffer[20];

	buffer[0] = 0;
	if (ent->thinkFlags & TH_THINK) {
		strcat(buffer, "T");
	}
	if (ent->thinkFlags & TH_ANIMATE) {
		strcat(buffer, "A");
	}
	if (ent->thinkFlags & TH_PHYSICS) {
		strcat(buffer, "P");
	}
	if (ent->thinkFlags & TH_UPDATEVISUALS) {
		strcat(buffer, "R");
	}
	if (ent->thinkFlags & TH_UPDATEPARTICLES) {
		strcat(buffer, "U");
	}
	if (ent->thinkFlags &  TH_TICKER) {
		strcat(buffer, "K");
	}

	return buffer;
}

//=========================================================================
//
// hhDebugger::FillDisplayList_Stats
//
//=========================================================================
void hhDebugger::FillDisplayList_Stats() {
	idEntity *ent;
	int slot, ix, num;
	float totalThinkMS = 0.0f;

	num = potentialSet.Num();
	for (ix=0; ix<num; ix++) {
		ent = potentialSet[ix];

		// Create or retrieve a slot for this item
		hhDisplayItem item;
		if (GetClassCollapse()) {
			item.column[0] = ent->GetClassname();
			slot = displayList.AddUnique(item);
		}
		else {
			item.column[0] = ent->name;
			slot = displayList.Append(item);	// All entity names should be unique but just in case
		}

		// Fill columns with appropriate stats
		hhDisplayItem *row = &displayList[slot];
		if (GetClassCollapse()) {
		    //class count dormant (unused) active time % events
			row->column[1] += 1;						// Count - number in this class
			if (ent->fl.isDormant) {					// HUMANHEAD JRM - changed to fl.isDormant
				row->column[2] += 1;					// dormant
			}

			// Column 3 is unused
			row->column[3] = 0;

			if (ent->IsActive()) {
				row->column[4] += 1;					// active - number that are active
				row->column[5] += ent->thinkMS;			// thinkMS
				totalThinkMS += ent->thinkMS;			// column 6 is percentage
			}
			row->column[7] += idEvent::NumQueuedEvents(ent);
			displayColumnsUsed = 8;
		}
		else {
		    //entity entitynum thFlags/active time % events
			row->column[1] = ent->entityNumber;
			if (ent->fl.isDormant) {					// HUMANHEAD JRM - Changed to fl.isDormant
				row->column[2] = 1;						// dormant
			}

			// Column 3 is unused
			row->column[3] = 0;

			if (ent->IsActive()) {
				row->column[4] = GetThinkFlags(ent);	// active - number that are active
				row->column[5] = ent->thinkMS;			// thinkMS
				totalThinkMS += ent->thinkMS;			// column 6 is percentage
			}
			row->column[7] += idEvent::NumQueuedEvents(ent);
			displayColumnsUsed = 8;
		}
	}

	// divide by totalMS to get percentage
	num = displayList.Num();
	float recip = totalThinkMS > 0.0f ? (1.0f / totalThinkMS) : 0.0f;
	for (ix=0; ix<num; ix++) {
		displayList[ix].column[6] = (int)(100 * (displayList[ix].column[5].Float() * recip));
	}
}

//=========================================================================
//
// hhDebugger::FillDisplayList_SpawnArgs
//
//=========================================================================
void hhDebugger::FillDisplayList_SpawnArgs() {
	const idKeyValue *kv;
	// Fill our lists with spawnargs, weeding out the undesirables
	if (selectedEntity.IsValid()) {
		int num = selectedEntity->spawnArgs.GetNumKeyVals();
		for (int ix=0; ix<num; ix++) {
			kv = selectedEntity->spawnArgs.GetKeyVal(ix);
			if (!kv->GetKey().IcmpPrefix("editor_")) {
				continue;
			}

			// Create a row with these two columns
			hhDisplayItem item;
			item.column[0] = kv->GetKey();
			item.column[1] = kv->GetValue();
			displayList.Append(item);
		}
		displayColumnsUsed = 2;
	}
}

//=========================================================================
//
// hhDebugger::FillDisplayList_GameInfo
//
//=========================================================================
void hhDebugger::FillDisplayList_GameInfo(int page) {
	const idKeyValue *kv;

	if (selectedEntity.IsValid()) {
		// Retrieve dictionary of variables from entity
		dict.Clear();

		selectedEntity->FillDebugVars(&dict, page);

		// Create a cleared item
		hhDisplayItem item;

		// Fill item with dictionary entries
		int num = dict.GetNumKeyVals();
		for (int ix=0; ix<num; ix++) {
			kv = dict.GetKeyVal(ix);
			item.column[0] = kv->GetKey();
			item.column[1] = kv->GetValue();
			displayList.Append(item);
		}

		displayColumnsUsed = 2;
	}
}

//=========================================================================
//
// hhDebugger::UpdateGUI_Entity
//
//=========================================================================
void hhDebugger::UpdateGUI_Entity() {

   // Fill display list depending on mode
	displayList.Clear();
	displayColumnsUsed = 0;
	switch(GetMode()) {
		case DEBUGMODE_STATS:		FillDisplayList_Stats();		break;
		case DEBUGMODE_SPAWNARGS:	FillDisplayList_SpawnArgs();	break;
		case DEBUGMODE_GAMEINFO1:	FillDisplayList_GameInfo(1);	break;
		case DEBUGMODE_GAMEINFO2:	FillDisplayList_GameInfo(2);	break;
		case DEBUGMODE_GAMEINFO3:	FillDisplayList_GameInfo(3);	break;
		case DEBUGMODE_NONE:											break;
	}

	// Sort display list based on sortColumn
	displayList.Sort();

	FillGUI();
	gui->StateChanged(gameLocal.time);	// Notify the gui that variables have changed
}

//=========================================================================
//
// hhDebugger::FillGUI
//
//=========================================================================
void hhDebugger::FillGUI() {

	int row,col;
	int numrows = displayList.Num();
	int numcols = displayColumnsUsed;
	char rowText[1024];

	for (row=0; row<DEBUG_NUM_ROWS; row++) {
		if (row < numrows) {
			rowText[0] = '\0';
			for (col=0; col<numcols; col++) {
				strcat(rowText, displayList[row].column[col].String());
				if (col+1 < numcols) {
					strcat(rowText, "\t");
				}
			}
			gui->SetStateString( va("listStats_item_%i", row ), rowText );
		}
		else {
			gui->DeleteStateVar( va( "listStats_item_%i", row ) );
		}
	}
}

//=========================================================================
//
// hhDebugger::DrawPotentialSet
//
//=========================================================================
void hhDebugger::DrawPotentialSet() {
	idVec4 color;
	idVec3 entPos;
	idEntity *ent;

	if (!GetDrawEntities()) {
		return;
	}

	// Draw potential entities as crosses
	int num = potentialSet.Num();
	for (int ix=0; ix<num; ix++) {
		ent = potentialSet[ix];
		if (!ent->GetPhysics()) {
			continue;
		}

		// Draw cross at all entities and label
		color = ColorForEntity(ent, 0.5f);
		entPos = ent->GetPhysics()->GetOrigin();

		hhUtils::DebugCross(color, entPos, 25);
		gameRenderWorld->DrawText(ent->name.c_str(), entPos, 0.15f, color, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 0);
	}
}

//=========================================================================
//
// hhDebugger::DrawSelectionSet
//
//=========================================================================
void hhDebugger::DrawSelectionSet() {
	if (selectedEntity.IsValid() && selectedEntity->GetPhysics()) {
		idVec3 entPos = selectedEntity->GetPhysics()->GetOrigin();
		idVec4 color = ColorForEntity(selectedEntity.GetEntity(), 1.0f);

		// Draw selected entities as highlighted
		if (GetMode() != DEBUGMODE_GAMEINFO2) {
			hhUtils::DebugCross(color*2, entPos, 10, 0);
		}

		gameRenderWorld->DebugBox(color*2,
			idBox(selectedEntity->GetPhysics()->GetBounds(), entPos, selectedEntity->GetPhysics()->GetAxis()), 0);

		// Allow entity to do custom debug drawing
		switch(GetMode()) {
			case DEBUGMODE_GAMEINFO1:
				selectedEntity->DrawDebug(1);
				break;
			case DEBUGMODE_GAMEINFO2:
				selectedEntity->DrawDebug(2);
				break;
			case DEBUGMODE_GAMEINFO3:
				selectedEntity->DrawDebug(3);
				break;
			default:
				selectedEntity->DrawDebug(0);
				break;
		}
	}
}

//=========================================================================
//
// hhDebugger::DrawGUI
//
//=========================================================================
void hhDebugger::DrawGUI() {
	PROFILE_SCOPE("Profilers", PROFMASK_NORMAL);
	if (gui) {
		gui->Redraw(gameLocal.time);
	}
}

//=========================================================================
//
// hhDebugger::UpdateDebugger
//
//=========================================================================
void hhDebugger::UpdateDebugger() {
	PROFILE_SCOPE("Profilers", PROFMASK_NORMAL);

	if (!gameLocal.GetLocalPlayer()) {
		return;
	}

	uiManager->SetDebuggerInteractive(IsInteractive());

	// HUMANHEAD PCF pdm 05/14/06: Don't initialize/load the gui unless it's actually being used.
	if (g_debugger.GetInteger() && !bInitialized) {
		Initialize();
	}

	CaptureInput(gui && g_debugger.GetInteger() == 2);

	if (gui && g_debugger.GetInteger()) {
		HandleFrameEvents();

		DeterminePotentialSet();
		DetermineSelectionSet();

		UpdateGUI_Entity();

		DrawPotentialSet();
		DrawSelectionSet();

		DrawGUI();

		if (IsInteractive()) {
			gui->DrawCursor();
		}
	}
}

// This code stolen from GuiFrameEvents()
void hhDebugger::HandleFrameEvents() {
	const char	*cmd;
	sysEvent_t  ev;
	static int	oldMouseX=0;
	static int	oldMouseY=0;
	static int	oldButton=0;
	static bool oldDown=false;
	int		newX, newY;
	int		newButton;
	bool    buttonDown;

	memset( &ev, 0, sizeof( ev ) );

	if (bInteractive) {
		// fake up a mouse event based on the deltas tracked
		// by the async usercmd generation
		uiManager->GetMouseState( &newX, &newY, &newButton, &buttonDown );
		if ( newX != oldMouseX || newY != oldMouseY ) {
			ev.evType = SE_MOUSE;
			ev.evValue = newX - oldMouseX;
			ev.evValue2 = newY - oldMouseY;
			cmd = gui->HandleEvent( &ev, gameLocal.time );
			if ( cmd && cmd[0] ) {
				HandleGuiCommands( cmd );
			}

			oldMouseX = newX;
			oldMouseY = newY;
		}

		if ( newButton != oldButton || buttonDown != oldDown) {
			ev.evType = SE_KEY;
			ev.evValue = newButton;
			ev.evValue2 = buttonDown;
			cmd = gui->HandleEvent( &ev, gameLocal.time );
			if ( cmd && cmd[0] ) {
				HandleGuiCommands( cmd );
			}
			oldButton = newButton;
			oldDown = buttonDown;
		}
	}

	ev.evType = SE_NONE;
	cmd = gui->HandleEvent( &ev, gameLocal.time );
	if ( cmd && cmd[0] ) {
		gameLocal.Printf( "frame event returned: '%s'\n", cmd );
	}
}

#endif	//INGAME_DEBUGGER_ENABLED