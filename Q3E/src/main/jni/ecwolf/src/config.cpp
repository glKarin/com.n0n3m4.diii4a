/*
** config.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>

#include "filesys.h"
#include "zstring.h"
#include "config.h"
#include "scanner.h"
#include "version.h"

typedef TMap<FName, TUniquePtr<SettingsData> > SettingsMap;

Config config;

Config::Config() : firstRun(false)
{
}

Config::~Config()
{
}

#ifndef LIBRETRO
void Config::LocateConfigFile(int argc, char* argv[])
{
	// Look for --config parameter
	for(int i = 0;i < argc-1;++i)
	{
		if(strcmp(argv[i], "--config") == 0)
		{
			configFile = argv[i+1];
			ReadConfig();
			return;
		}
	}

	configDir = FileSys::GetDirectoryPath(FileSys::DIR_Configuration);

#ifdef _WIN32
	configFile = configDir + "\\" BINNAME ".cfg";
#else
	configFile = configDir + "/" BINNAME ".cfg";
#endif

	ReadConfig();
}

void Config::ReadConfig()
{
	// Check to see if we have located the config file.
	if(configFile.IsEmpty())
		return;

	FILE *stream = File(configFile).open("rb");
	if(stream)
	{
		if(fseek(stream, 0, SEEK_END))
			return;
		unsigned int size = static_cast<unsigned int>(ftell(stream));
		if(fseek(stream, 0, SEEK_SET))
			return;
		char* data = new char[size];
		fread(data, 1, size, stream);
		// The eof flag seems to trigger fail on windows.
		if(!feof(stream) && ferror(stream))
		{
			delete[] data;
			return;
		}
		fclose(stream);

		Scanner sc(data, size);
		sc.SetScriptIdentifier("Configuration");
		while(sc.TokensLeft())  // Go until there is nothing left to read.
		{
			sc.MustGetToken(TK_Identifier);
			FString index = sc->str;
			sc.MustGetToken('=');
			if(sc.CheckToken(TK_StringConst))
			{
				CreateSetting(index, "");
				GetSetting(index)->SetValue(sc->str);
			}
			else
			{
				bool negative = sc.CheckToken('-');
				if(sc.CheckToken(TK_IntConst))
				{
					CreateSetting(index, 0);
					GetSetting(index)->SetValue(negative ? -sc->number : sc->number);
				}
				else
				{
					sc.MustGetToken(TK_FloatConst);
					CreateSetting(index, 0.0f);
					GetSetting(index)->SetValue(negative ? -sc->decimal : sc->decimal);
				}
			}
			sc.MustGetToken(';');
		}

		delete[] data;
	}

	if(settings.CountUsed() == 0)
		firstRun = true;
}

void Config::SaveConfig()
{
	// Check to see if we're saving the settings.
	if(configFile.IsEmpty())
		return;

	FILE *stream = File(configFile).open("wb");
	if(stream)
	{
		SettingsMap::Pair *pair;
		for(SettingsMap::Iterator it(settings);it.NextPair(pair);)
		{
			fwrite(pair->Key, 1, strlen(pair->Key), stream);
			if(ferror(stream))
				return;
			SettingsData *data = pair->Value;
			if(data->GetType() == SettingsData::ST_INT)
			{
				// Determine size of number.
				unsigned int intLength = 1;
				while(data->GetInteger()/static_cast<int>(pow(10.0, static_cast<double>(intLength))) != 0)
					intLength++;

				char* value = new char[intLength + 7];
				sprintf(value, " = %d;\n", data->GetInteger());
				fwrite(value, 1, strlen(value), stream);
				delete[] value;
				if(ferror(stream))
					return;
			}
			else if(data->GetType() == SettingsData::ST_FLOAT)
			{
				FString value;
				value.Format(" = %f;\n", data->GetFloat());
				fwrite(value.GetChars(), 1, value.Len(), stream);
				if(ferror(stream))
					return;
			}
			else
			{
				FString str = data->GetString(); // Make a non const copy of the string.
				Scanner::Escape(str);
				char* value = new char[str.Len() + 8];
				sprintf(value, " = \"%s\";\n", str.GetChars());
				fwrite(value, 1, str.Len() + 7, stream);
				delete[] value;
				if(ferror(stream))
					return;
			}
		}
		fclose(stream);
	}
}
#endif

void Config::CreateSetting(const FName index, int defaultInt)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultInt);
		settings[index] = data;
	}
}

void Config::CreateSetting(const FName index, double defaultFloat)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultFloat);
		settings[index] = data;
	}
}

void Config::CreateSetting(const FName index, FString defaultString)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultString);
		settings[index] = data;
	}
}

void Config::DeleteSetting(const FName index)
{
	settings.Remove(index);
}

SettingsData *Config::GetSetting(const FName index)
{
	SettingsData *data;
	if(FindIndex(index, data))
		return data;
	return NULL;
}

bool Config::FindIndex(const FName index, SettingsData *&data)
{
	TUniquePtr<SettingsData> *setting = settings.CheckKey(index);
	if(setting == NULL)
		return false;
	data = *setting;
	return true;
}
