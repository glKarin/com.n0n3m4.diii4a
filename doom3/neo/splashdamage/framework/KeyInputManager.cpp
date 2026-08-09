// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "framework/KeyInput.h"
#include "KeyInputManager_Local.h"

static const keyNum_t ModifierToKeynums[] = {
	K_CTRL,
	K_SHIFT,
	K_ALT,
	K_COMMAND,
	K_RIGHT_CTRL,
	K_RIGHT_SHIFT,
	K_RIGHT_ALT,
	K_COMMAND,
};

ID_INLINE static int ModifierKeyToEnum(int mod)
{
	for(int i = 0; i < KM_TOTAL; i++)
	{
		if(ModifierToKeynums[i] == mod)
			return i;
	}
	return -1;
}

ID_INLINE static int ModifierEnumToKey(int km)
{
	if(km >= 0 && km < KM_TOTAL)
		return ModifierToKeynums[km];
	return -1;
}

ID_INLINE static int FindModifierBindCommand(idStaticList< sdKeyBind::pair_t, sdKeyBind::MAX_MODIFIERS > &list, int modifier)
{
	for(int i = 0; i < list.Num(); i++)
	{
		if(list[i].first == modifier)
			return i;
	}

	return -1;
}

ID_INLINE static void DownsizeModifierBindCommand(idStaticList< sdKeyBind::pair_t, sdKeyBind::MAX_MODIFIERS > &list)
{
	int i;
	for(i == list.Num() - 1; i >= 0; i--)
	{
		if(list[i].first != K_INVALID)
			break;
	}
	if(i < list.Num() - 1)
		list.SetNum(i + 1);
}



sdKeyInputManagerLocal::sdKeyInputManagerLocal(void)
	: defaultContext(NULL),
	currentContext(NULL)
{
}

sdKeyInputManagerLocal::~sdKeyInputManagerLocal(void) {
	bindContexts.DeleteContents(true);
}

void sdKeyInputManagerLocal::SetBinding( sdBindContext* context, idKey& key, const char* binding, idKey* modifierKey ) {
	if(IsDefault(context))
	{
		idKeyInput::SetBinding(key.GetId(), binding);
	}
	//else
	{
		context->Bind(key.GetId(), modifierKey ? modifierKey->GetId() : -1, binding);
	}
}

const char* sdKeyInputManagerLocal::GetBinding( sdBindContext* context, idKey& key, idKey* modifierKey ) {
	if(IsDefault(context))
	{
		return idKeyInput::GetBinding(key.GetId());
	}
	else
	{
		sdKeyBind *bind = context->GetBind(key.GetId());
		if (bind) {
			if (modifierKey)
				return bind->GetCommand(modifierKey->GetId()).GetBinding();
			else
				return bind->GetCommand().GetBinding();
		}
		else
			return NULL;
	}
}

void sdKeyInputManagerLocal::UnbindBinding( sdBindContext* context, const char *bind ) {
	if(IsDefault(context))
	{
		idKeyInput::UnbindBinding(bind);
	}
	//else
	{
		context->UnBindBinding(bind);
	}
}

void sdKeyInputManagerLocal::KeysFromBinding( sdBindContext* context, const char* binding, bool useBindStrWhenEmpty, idWStr& keyName ) {
	if(IsDefault(context))
	{
		const char *name = idKeyInput::KeysFromBinding(binding);
		keyName = StrToWStr(name);
	}
	else
	{
		keyName = StrToWStr(context->GetName());
	}
}

//karin: _keys maybe null if only for get num keys
void sdKeyInputManagerLocal::KeysFromBinding( sdBindContext* context, const char* binding, int& numKeys, idKey** _keys ) {
	int max = numKeys;
	numKeys = 0;
	if (binding && *binding) {
		if(IsDefault(context))
		{
			for (int i = 0; i < MAX_KEYS; i++) {
				if (keys[i].binding.Icmp(binding) == 0) {
					if(_keys && numKeys < max)
						_keys[numKeys] = &keys[i];
					numKeys++;
				}
			}
		}
		else
		{
			for (int i = 0; i < context->keys.Num(); i++) {
				if (idStr::Icmp(context->keys[i].second->GetCommand().GetBinding(), binding) == 0)
				{
					if(_keys && numKeys < max)
						_keys[numKeys] = &keys[i];
					numKeys++;
				}
			}
		}
	}
}

bool sdKeyInputManagerLocal::IsDown( const idKey& key ) {
	return key.IsDown();
}

bool sdKeyInputManagerLocal::IsDown( keyNum_e key ) {
	return idKeyInput::IsDown( key );
}

idKey* sdKeyInputManagerLocal::GetKey( const char* name ) {
	int id = idKeyInput::StringToKeyNum(name);
	return id != -1 ? &keys[id] : NULL;
}

idKey* sdKeyInputManagerLocal::GetKeyForEvent( const sdSysEvent& evt, bool& down ) {
	if (evt.evType == SE_KEY) {
		if (evt.evValue >= 0 && evt.evValue < MAX_KEYS) {
			idKey *key = &keys[evt.evValue];
			down = key->IsDown();
			return key;
		}
	}
	return NULL;
}

void sdKeyInputManagerLocal::ProcessUserCmdEvent( const sdSysEvent& event ) {
	if (event.evType == SE_KEY)
	{
		idKeyInput::PreliminaryKeyEvent(event.evValue, event.evValue2);
		if(currentContext && event.evValue2 == 1)
		{
			sdKeyBind *bind = currentContext->GetBind(event.evValue);
			if(bind)
			{
				const sdKeyBind::pair_t *p;
				int i;

				for(i = 0; i < bind->modifierCommands.Num(); i++)
				{
					p = &bind->modifierCommands[i];
					if(p->first == K_INVALID)
						continue;

					if(idKeyInput::IsDown(p->first)
							&& (p->second.GetType() == B_LOCAL_IMPULSE && p->second.GetAction())
							)
					{
						sysEvent_t ev;
						ev.Memset();
						ev.evType = SE_GUI;
						ev.evValue = p->second.GetAction();
						game->HandleGuiEvent(&ev);
						break;
					}
				}

				if(i == bind->modifierCommands.Num())
				{
					const sdKeyCommand *cmd = &bind->GetCommand();
					if(cmd->GetType() == B_LOCAL_IMPULSE && cmd->GetAction())
					{
						sysEvent_t ev;
						ev.Memset();
						ev.evType = SE_GUI;
						ev.evValue = cmd->GetAction();
						game->HandleGuiEvent(&ev);
					}
				}
			}
		}
	}

	currentContext = NULL;
}

sdKeyCommand* sdKeyInputManagerLocal::GetCommand( sdBindContext* context, const idKey& key ) {
	currentContext = context;
	if(IsDefault(context))
	{
		return &const_cast<idKey &>(key).command;
	}
	else
	{
		sdKeyBind *binding = context->GetBind(key.GetId());
		if (binding) {
			return &binding->GetCommand();
		}
		else
			return NULL;
	}
}

sdBindContext* sdKeyInputManagerLocal::AllocBindContext( const char* context ) {
	for (int i = 0; i < bindContexts.Num(); ++i) {
		if (!idStr::Icmp(bindContexts[i]->GetName(), context))
			return bindContexts[i];
	}
	int index = bindContexts.Append(new sdBindContext(context));
	//karin: add HACK: ignore menu, bindmenu, radialmenu
	const bool isMenu = !idStr::Icmp(context, "menu") || !idStr::Icmp(context, "radialmenu") || !idStr::Icmp(context, "bindmenu");
	const bool isDefault = !idStr::Icmp(context, "default");
	int flags = 0;
	if(isMenu)
		flags |= BCF_MENU;
	if(isDefault)
		flags |= BCF_DEFAULT;
	contextFlags.Append(flags);
	if(isDefault)
		defaultContext = bindContexts[index];
	return bindContexts[index];
}

void sdKeyInputManagerLocal::UnbindKey(  sdBindContext* context, idKey& key, idKey* modifier ) {
	if(IsDefault(context))
	{
		idKeyInput::SetBinding(key.GetId(), "");
	}
	//else
	{
		if (modifier)
			context->UnBind(key.GetId(), modifier->GetId());
		else
			context->UnBind(key.GetId(), 0);
	}
}

bool sdKeyInputManagerLocal::AnyKeysDown( void ) {
	for (int i = 0; i < MAX_KEYS; i++) {
		if (keys[i].IsDown())
			return true;
	}
	return false;
}

void sdKeyInputManagerLocal::BindDefault(void)
{
#define K_BINDING(key, cmd, mod, ctx) "bind \"" key "\" \"" cmd "\" \"" mod "\" \"" ctx "\";\n"
	const char etqwbinds[] = "unbindall;\n"
		K_BINDING("w", "_forward", "", "default")
		K_BINDING("s", "_back", "", "default")
		K_BINDING("a", "_moveleft", "", "default")
		K_BINDING("d", "_moveright", "", "default")
		K_BINDING("x", "_prone", "", "default")
		K_BINDING("SPACE", "_moveup", "", "default")
		K_BINDING("c", "_movedown", "", "default")
		K_BINDING("q", "_leanleft", "", "default")
		K_BINDING("e", "_leanright", "", "default")
		K_BINDING("SHIFT", "_sprint", "", "default")
		K_BINDING("CTRL", "_speed", "", "default")
		K_BINDING("1", "_weapon0", "", "default")
		K_BINDING("2", "_weapon1", "", "default")
		K_BINDING("3", "_weapon2", "", "default")
		K_BINDING("4", "_weapon3", "", "default")
		K_BINDING("5", "_weapon4", "", "default")
		K_BINDING("6", "_weapon5", "", "default")
		K_BINDING("7", "useWeapon weapon_binocs", "", "default")
		K_BINDING("MOUSE1", "_attack", "", "default")
		K_BINDING("MOUSE2", "_altattack", "", "default")
		K_BINDING("MWHEELDOWN", "_weapnext", "", "default")
		K_BINDING("MWHEELUP", "_weapprev", "", "default")
		K_BINDING("r", "_reload", "", "default")
		K_BINDING("f", "_activate", "", "default")
		K_BINDING("MOUSE4", "_activate", "", "default")
		K_BINDING("b", "useWeapon weapon_binocs", "", "default")
		K_BINDING("g", "_usevehicle", "", "default")
		K_BINDING("HOME", "_vehicleCamera", "", "default")
		K_BINDING("CAPSLOCK", "_tophat", "", "default")
		K_BINDING("h", "_modeswitch", "", "default")
		K_BINDING("-", "_stroyDown", "", "default")
		K_BINDING("+", "_stroyUp", "", "default")
		K_BINDING("F1", "vote y", "", "default")
		K_BINDING("F2", "vote n", "", "default")
		K_BINDING(",", "zoomOutCommandMap", "", "default")
		K_BINDING(".", "zoomInCommandMap", "", "default")
		K_BINDING("o", "_showWayPoints", "", "default")
		K_BINDING("p", "toggle g_showWayPoints 0 1", "", "default")
		K_BINDING("ALT", "_showFireTeam", "", "default")
		K_BINDING("t", "clientMessageMode", "", "default")
		K_BINDING("y", "clientMessageMode 1", "", "default")
		K_BINDING("u", "clientMessageMode 2", "", "default")
		K_BINDING("v", "_context", "", "default")
		K_BINDING("MOUSE3", "_quickchat", "", "default")
		K_BINDING("F3", "_ready", "", "default")
		K_BINDING("F4", "_showScores", "", "default")
		K_BINDING("TAB", "_showScores", "", "default")
		K_BINDING("m", "_taskmenu", "", "default")
		K_BINDING("n", "_commandmap", "", "default")
		K_BINDING("l", "_limbomenu", "", "default")
		K_BINDING("k", "_votemenu", "", "default")
		K_BINDING("KP_ENTER", "_fireteam", "", "default")
		K_BINDING("z", "_fireteamVoice", "", "default")
		K_BINDING("i", "_fireteamVoice", "", "default")
		K_BINDING("F5", "clientTeam gdf", "", "default")
		K_BINDING("F6", "clientTeam strogg", "", "default")
		K_BINDING("F7", "clientTeam spectator", "", "default")
		K_BINDING("F11", "screenshot", "", "default")
		K_BINDING("F12", "toggleNetDemo", "", "default")
		K_BINDING("TAB", "_menuNavForward", "", "menu")
		K_BINDING("TAB", "_menuNavBackward", "SHIFT", "menu")
		K_BINDING("TAB", "_menuNavBackward", "RIGHTSHIFT", "menu")
		K_BINDING("ESCAPE", "_menuCancel", "", "menu")
		K_BINDING("ENTER", "_menuAccept", "", "menu")
		K_BINDING("ENTER", "_menuNewline", "CTRL", "menu")
		K_BINDING("ENTER", "_menuNewline", "RIGHTCTRL", "menu")
		K_BINDING("KP_ENTER", "_menuAccept", "", "menu")
		K_BINDING("ESCAPE", "_menuCancel", "", "bindmenu")
		;
#undef K_BINDING
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, etqwbinds);
}

//karin: must after idDeclManager::Init
void sdKeyInputManagerLocal::Init(void)
{
	for (int i = 0; i < MAX_KEYS; i++) {
		idKey &key = keys[i];
		// check locName exists
		const idDecl *decl = declManager->FindType(DECL_LOCSTR, key.locName, false);
		if(decl)
		{
			const sdDeclLocStr *locStr = static_cast<const sdDeclLocStr *>(decl);
			key.fixedText = locStr->GetText();
		}
		else
		{
			key.fixedText = StrToWStr(idKeyInput::KeyNumToString(i, true));
			key.locName.Clear(); // clear if not exists, it will using fixedText
		}

		if (key.binding.IsEmpty())
			continue;
		usercmdbuttonType_t type = game->SetupBinding(key.binding, key.usercmdAction);
		key.type = type;

		key.command.Set(key.binding);
	}

	for (int i = 0; i < bindContexts.Num(); ++i) {
		//if (!IsDefault(bindContexts[i]))
			bindContexts[i]->SetupBinds();
	}
}

void sdKeyInputManagerLocal::UnBindAll(void)
{
	for (int i = 0; i < bindContexts.Num(); ++i) {
		bindContexts[i]->UnBindAll();
	}
}

void sdKeyInputManagerLocal::Write(idFile *f, bool unbindall) const
{
	if(unbindall)
		f->Printf("unbindall\n");

	for (int i = 0; i < bindContexts.Num(); ++i) {
		if(!IsDefault(bindContexts[i]))
			bindContexts[i]->WriteBindings(f);
	}
}

sdBindContext * sdKeyInputManagerLocal::GetBindContext(const char *context)
{
	if(!context || !context[0])
		return NULL;

	for (int i = 0; i < bindContexts.Num(); ++i) {
		if (!idStr::Icmp(bindContexts[i]->GetName(), context))
			return bindContexts[i];
	}

	return NULL;
}

const idKey& sdKeyInputManagerLocal::GetKeyByNum( int keynum ) const {
	return keys[keynum];
}

bool sdKeyInputManagerLocal::IsMenu(sdBindContext *context) const
{
	int index = bindContexts.FindIndex(context);
	if(index < 0)
		return false;
	return contextFlags[index] & BCF_MENU;
}



sdKeyCommand::sdKeyCommand( void )
	: action(0),
	type(B_COMMAND)
{
}

void sdKeyCommand::Set( const char* _binding ) {
	binding = _binding;
	FixupBind();
}

void sdKeyCommand::FixupBind( void ) {
	if(game && !binding.IsEmpty())
		type = game->SetupBinding(binding, action);
	else
	{
		action = 0;
		type = B_COMMAND;
	}
}




void sdKeyBind::ClearCommand( int modifier ) {
	if (modifier < 0)
		defaultCommand.Set("");
	else
	{
		int i = FindModifierBindCommand(modifierCommands, modifier);
		if(i != -1)
		{
			pair_t *p = &modifierCommands[i];
			p->first = K_INVALID;
			p->second.Set("");
			DownsizeModifierBindCommand(modifierCommands);
		}
	}
}

void sdKeyBind::SetCommand( int modifier, const char* command ) {
	if (modifier < 0)
		defaultCommand.Set(command);
	else
	{
		int i = FindModifierBindCommand(modifierCommands, modifier);
		if(i != -1)
			modifierCommands[i].second.Set(command);
		else
		{
			int km = ModifierKeyToEnum(modifier);
			if(km != -1)
			{
				if(modifierCommands.Num() <= km)
					modifierCommands.SetNum(km + 1);
				sdKeyBind::pair_t *p = &modifierCommands[km];
				p->first = modifier;
				p->second.Set(command);
			}
		}
	}
}

sdKeyCommand& sdKeyBind::GetCommand( void ) {
	return defaultCommand;
}

sdKeyCommand& sdKeyBind::GetCommand( int modifier ) {
	if (modifier < 0)
		return defaultCommand;
	else
	{
		int i = FindModifierBindCommand(modifierCommands, modifier);
		if(i != -1)
			return modifierCommands[i].second;
		else
		{
			int km = ModifierKeyToEnum(modifier);
			if(km != -1)
			{
				sdKeyBind::pair_t *p;
				while(modifierCommands.Num() <= km)
				{
					p = modifierCommands.Alloc();
					p->first = 0;
					p->second.Set("");
				}
				p = &modifierCommands[km];
				p->first = modifier;
				p->second.Set("");
				return p->second;
			}
			else
				return defaultCommand;
		}
	}
}

void sdKeyBind::Write( idFile* f, const char* context, const char* keyName ) {
	if(idStr::Length(defaultCommand.GetBinding()) > 0)
	{
		if (!strcmp(keyName, "\\"))
			f->Printf("bind \"\\\" \"%s\" \"\" \"%s\"\n", defaultCommand.GetBinding(), context);
		else
			f->Printf("bind \"%s\" \"%s\" \"\" \"%s\"\n", keyName, defaultCommand.GetBinding(), context);
	}

	const pair_t *p;
	for(int i = 0; i < modifierCommands.Num(); i++)
	{
		p = &modifierCommands[i];
		if(p->first == K_INVALID)
			continue;
		const char *modKeyName = idKeyInput::KeyNumToString(p->first, false);
		if (!strcmp(keyName, "\\"))
			f->Printf("bind \"\\\" \"%s\" \"%s\" \"%s\"\n", defaultCommand.GetBinding(), modKeyName, context);
		else
			f->Printf("bind \"%s\" \"%s\" \"%s\" \"%s\"\n", keyName, p->second.GetBinding(), modKeyName, context);
	}
}

void sdKeyBind::UnBindBinding( const char* binding ) {
	if (binding && *binding) {
		if (!idStr::Icmp(defaultCommand.GetBinding(), binding))
			defaultCommand.Set("");

		pair_t *p;
		for(int i = 0; i < modifierCommands.Num(); i++) {
			p = &modifierCommands[i];
			if (idStr::Icmp(p->second.GetBinding(), binding) == 0) {
				p->second.Set("");
				p->first = K_INVALID;
			}
		}
		DownsizeModifierBindCommand(modifierCommands);
	}
}

void sdKeyBind::SetupBinds( void ) {
	defaultCommand.FixupBind();
	for(int i = 0; i < modifierCommands.Num(); i++)
	{
		if(modifierCommands[i].first != K_INVALID)
			modifierCommands[i].second.FixupBind();
	}
}



sdKeyBind* sdBindContext::AllocBind( int key ) {
	int index = keyHash.GetFirst(key);
	if (index == idHashIndexInt::NULL_INDEX) {
		index     = keys.Append(pair_t());
		pair_t &p = keys[index];
		p.first   = key;
		p.second  = new sdKeyBind;
		keyHash.Add(key, index);
		return p.second;
	}
	else
		return keys[index].second;
}

sdKeyBind* sdBindContext::GetBind( int key ) {
	int index = keyHash.GetFirst(key);
	if (index == idHashIndexInt::NULL_INDEX)
		return NULL;
	else
		return keys[index].second;
}

sdKeyCommand* sdBindContext::GetCommand( int key ) {
	sdKeyBind* binding = GetBind(key);
	return binding ? &binding->GetCommand() : NULL;
}

void sdBindContext::WriteBindings( idFile* f ) {
	for (int i = 0; i < keys.Num(); i++) {
		const char *keyName = idKeyInput::KeyNumToString(keys[i].first, false);
		keys[i].second->Write(f, name, keyName);
	}
}

void sdBindContext::Bind( int key, int modifierKey, const char* binding ) {
	sdKeyBind* b = AllocBind(key);
	b->SetCommand(modifierKey, binding);
}

void sdBindContext::UnBind( int key, int modifierKey ) {
	sdKeyBind* binding = GetBind(key);
	if (!binding)
		return;
	binding->ClearCommand(modifierKey);
}

void sdBindContext::UnBindAll( void ) {
	keyHash.Clear();
	for (int i = 0; i < keys.Num(); i++) {
		delete keys[i].second;
	}
	keys.Clear();
}

void sdBindContext::UnBindBinding( const char* binding ) {
	for (int i = 0; i < keys.Num(); i++) {
		keys[i].second->UnBindBinding(binding);
	}
}

void sdBindContext::SetupBinds( void ) {
	for (int i = 0; i < keys.Num(); i++) {
		keys[i].second->SetupBinds();
	}
}

sdKeyInputManagerLocal keyInputManagerLocal;

sdKeyInputManager* keyInputManager = &keyInputManagerLocal;
