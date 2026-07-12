// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "SDNetUser_local.h"
#include "SDNet_local.h"

extern void RemoveOSDir(const char *OSPath, int type = 0);
extern void	ReplacePathSeparators(idStr &path, char sep = PATHSEPERATOR_CHAR);

sdNetUser_Local::sdNetUser_Local()
    : userState(US_INACTIVE),
	noPrint(false)
{
}

sdNetUser_Local::~sdNetUser_Local() {
}

sdNetUser::userState_e sdNetUser_Local::GetState() const {
    return userState;
}

const char* sdNetUser_Local::GetUsername() const {
    return account.GetUsername();
}

const char* sdNetUser_Local::GetRawUsername() const {
    return rawUsername.c_str();
}

sdNetProfile& sdNetUser_Local::GetProfile() {
    return profile;
}

const sdNetProfile& sdNetUser_Local::GetProfile() const {
    return profile;
}

// Make this user the currently active one
void sdNetUser_Local::Activate() {
	Restore(SI_PROFILE | SI_CVARS | SI_BINDINGS);
	profile.SetUsername(rawUsername);
	userState = US_ACTIVE;
	networkServiceLocal.SetActiveUser(this);
}

// Deactivate the user, effectively logging them out
void sdNetUser_Local::Deactivate() {
	userState = US_INACTIVE;
	Save(SI_PROFILE | SI_CVARS | SI_BINDINGS);
	networkServiceLocal.SetActiveUser(NULL);
}

// Write user to permanent storage
bool sdNetUser_Local::Save( int saveItems ) const {
	int res = 0;
	if(saveItems & SI_PROFILE)
	{
		if(SaveOffline(SI_PROFILE))
			res |= SI_PROFILE;
	}
	if(saveItems & SI_CVARS)
	{
		if(SaveOffline(SI_CVARS))
			res |= SI_CVARS;
	}
	if(saveItems & SI_BINDINGS)
	{
		if(SaveOffline(SI_BINDINGS))
			res |= SI_BINDINGS;
	}
	return res == saveItems;
}

// Get online account
sdNetAccount& sdNetUser_Local::GetAccount() {
    return account;
}

void sdNetUser_Local::SetRawUsername(const char *name)
{
	rawUsername = name;
}

void sdNetUser_Local::Init(void)
{
	account.SetUsername(rawUsername);
	Restore(SI_PROFILE);
	profile.SetUsername(rawUsername);
}

void sdNetUser_Local::Create(void)
{
	common->Printf("SDNet: create user: %s\n", rawUsername.c_str());
	account.SetUsername(rawUsername);
	profile.SetUsername(rawUsername);
	Save(SI_PROFILE | SI_CVARS | SI_BINDINGS);
}

void sdNetUser_Local::ProfilePath(idStr &out, const char *type) const
{
	out.Append(fileSystem->GetUserPath());
	out.AppendPath("sdnet");
	out.AppendPath(rawUsername);
	const char *fs_game = cvarSystem->GetCVarString("fs_game");
	if(fs_game && fs_game)
		out.AppendPath(fs_game);
	else
		out.AppendPath(BASE_GAMEDIR);
	out.AppendPath(type);
	out.SetFileExtension(".cfg");
	ReplacePathSeparators(out);
}

bool sdNetUser_Local::SaveCVarsOffline(idFile *file) const
{
	if(!noPrint)
		common->Printf("Save user '%s' offline cvars\n", rawUsername.c_str());
	cvarSystem->WriteFlaggedVariables(CVAR_ARCHIVE, "seta", file);
	return true;
}

bool sdNetUser_Local::SaveBindingsOffline(idFile *file) const
{
	if(!noPrint)
		common->Printf("Save user '%s' offline bindings\n", rawUsername.c_str());
	idKeyInput::WriteBindings(file);
	return true;
}

bool sdNetUser_Local::SaveProfileOffline(idFile *file) const
{
	if(!noPrint)
		common->Printf("Save user '%s' offline profile\n", rawUsername.c_str());
	int num = profile.GetProperties().GetNumKeyVals();
	for(int i = 0; i < num; i++)
	{
		const idKeyValue *kv = profile.GetProperties().GetKeyVal(i);
		const char *line;
		if(kv->GetValue().Find(' ') != -1)
			line = va("\"%s\" \"%s\"\n", kv->GetKey().c_str(), kv->GetValue().c_str());
		else
			line = va("\"%s\" %s\n", kv->GetKey().c_str(), kv->GetValue().c_str());
		file->Write(line, strlen(line));
	}
	return true;
}

bool sdNetUser_Local::RestoreCVarsOffline(const char *data)
{
	common->Printf("Restore user '%s' offline cvars\n", rawUsername.c_str());
	cmdSystem->BufferCommandText(CMD_EXEC_INSERT, data);
	return true;
}

bool sdNetUser_Local::RestoreProfileOffline(const char *data)
{
	common->Printf("Restore user '%s' offline profile\n", rawUsername.c_str());
	idParser src;
	idStr str;
	str.Append("{\n");
	str.Append(data);
	str.Append("}\n");
	src.LoadMemory(str.c_str(), str.Length(), "<userprofile>");
	src.SetFlags(LEXFL_NOFATALERRORS | LEXFL_NOERRORS);
	profile.Clear();
	profile.GetProperties().Parse(src);
	profile.SetUsername(rawUsername);
	return true;
}

bool sdNetUser_Local::RestoreBindingsOffline(const char *data)
{
	common->Printf("Restore user '%s' offline bindings\n", rawUsername.c_str());
	cmdSystem->BufferCommandText(CMD_EXEC_INSERT, data);
	return true;
}

bool sdNetUser_Local::RestoreOffline(int item)
{
	idStr path;
	const char *filename;
	switch(item)
	{
		case SI_PROFILE:
			filename = "property";
			break;
		case SI_CVARS:
			filename = "profile";
			break;
		case SI_BINDINGS:
			filename = "bindings";
			break;
		default:
			return false;
	}
	ProfilePath(path, filename);

	idFile *file = fileSystem->OpenExplicitFileRead(path);
	if(!file)
		return false;

	int length = file->Length();
	if(length == 0)
	{
		fileSystem->CloseFile(file);
		return false;
	}

	char *data = (char *)Mem_Alloc(length + 1);
	file->Read(data, length);
	data[length] = '\0';
	fileSystem->CloseFile(file);

	bool res;
	switch(item)
	{
		case SI_PROFILE:
			res = RestoreProfileOffline(data);
			break;
		case SI_CVARS:
			res = RestoreCVarsOffline(data);
			break;
		case SI_BINDINGS:
			res = RestoreBindingsOffline(data);
			break;
		default:
			res = false;
			break;
	}

	Mem_Free(data);

    return res;
}

bool sdNetUser_Local::SaveOffline( int saveItem ) const {
	idStr path;
	const char *filename;
	switch(saveItem)
	{
		case SI_PROFILE:
			filename = "property";
			break;
		case SI_CVARS:
			filename = "profile";
			break;
		case SI_BINDINGS:
			filename = "bindings";
			break;
		default:
			return false;
	}
	ProfilePath(path, filename);

	idFile *file = fileSystem->OpenExplicitFileWrite(path);
	if(!file)
		return false;

	const char header[] = 
"// *********************************************************\n"
"// This file is managed by ETQW and will be\n"
"// overwritten when logging in and out\n"
"// Any custom commands or bindings should be put into 'autoexec.cfg'\n"
"// in the same folder as this file\n"
"// *********************************************************\n\n";
	file->Write(header, strlen(header));

	bool res;
	switch(saveItem)
	{
		case SI_PROFILE:
			res = SaveProfileOffline(file);
			break;
		case SI_CVARS:
			res = SaveCVarsOffline(file);
			break;
		case SI_BINDINGS:
			res = SaveBindingsOffline(file);
			break;
		default:
			res = false;
			break;
	}

	fileSystem->CloseFile(file);
    return res;
}

bool sdNetUser_Local::Restore( int saveItems ) {
	int res = 0;
	if(saveItems & SI_PROFILE)
	{
		if(RestoreOffline(SI_PROFILE))
			res |= SI_PROFILE;
	}
	if(saveItems & SI_CVARS)
	{
		if(RestoreOffline(SI_CVARS))
			res |= SI_CVARS;
	}
	if(saveItems & SI_BINDINGS)
	{
		if(RestoreOffline(SI_BINDINGS))
			res |= SI_BINDINGS;
	}
	return res == saveItems;
}

void sdNetUser_Local::Remove(void)
{
	if(rawUsername.IsEmpty())
		return;

	idStr path = fileSystem->GetUserPath();
	path.AppendPath("sdnet");
	path.AppendPath(rawUsername);
	RemoveOSDir(path);
}

void sdNetUser_Local::SaveModified(void)
{
	noPrint = true;
	Save(sdNetUser::SI_CVARS | sdNetUser::SI_BINDINGS);
	noPrint = false;
}
