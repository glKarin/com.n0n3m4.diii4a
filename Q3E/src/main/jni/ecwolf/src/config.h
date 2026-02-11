/*
** config.h
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "tarray.h"
#include "tmemory.h"
#include "zstring.h"

struct SettingsData
{
	public:
		enum SettingType
		{
			ST_INT,
			ST_FLOAT,
			ST_STR
		};

		SettingsData(int integer=0) : type(ST_INT), str("") { SetValue(integer); }
		SettingsData(unsigned int integer) : type(ST_INT), str("") { SetValue(integer); }
		SettingsData(double decimal) : type(ST_FLOAT), str("") { SetValue(decimal); }
		SettingsData(FString str) : type(ST_STR), str("") { SetValue(str); integer = 0; }

		int					GetInteger() { return integer; }
		double				GetFloat() { return decimal; }
		FString				GetString()	{ return str; }
		SettingType			GetType() { return type; }
		void				SetValue(int integer) { this->integer = integer;this->type = ST_INT; }
		void				SetValue(unsigned int integer) { SetValue((int)integer); }
		void				SetValue(double decimal) { this->decimal = decimal;this->type = ST_FLOAT; }
		void				SetValue(FString str) { this->str = str;this->type = ST_STR; }

	protected:
		SettingType		type;
		union
		{
			double		decimal;
			int			integer;
		};
		FString			str;
};

class Config
{  
	public:
		Config();
		~Config();

		/**
		 * Creates the specified setting if it hasn't been made already.  It 
		 * will be set to the default value.
		 */
		void			CreateSetting(const FName index, int defaultInt);
		void			CreateSetting(const FName index, unsigned int defaultInt) { CreateSetting(index, (int)defaultInt); }
		void			CreateSetting(const FName index, double defaultFloat);
		void			CreateSetting(const FName index, FString defaultString);
		void			DeleteSetting(const FName index);

		FString			GetConfigDir() const { return configDir; }
		/**
		 * Gets the specified setting.  Will return NULL if the setting does 
		 * not exist.
		 */
		SettingsData	*GetSetting(const FName index);
		/**
		 * Returns if this is an entirely new configuration file.  This can be 
		 * used to see if a first time set up wizard should be run.
		 */
		bool			IsNewConfig() { return firstRun; }
		/**
		 * Looks for the config file and creates the directory if needed.  This 
		 * will call ReadConfig if there is a file to be read.
		 * @see SaveConfig
		 * @see ReadConfig
		 */
		void			LocateConfigFile(int argc, char* argv[]);
		/**
		 * Reads the configuration file for settings.  This is ~/.sde/sde.cfg 
		 * on unix systems.
		 * @see LocateConfigFile
		 * @see SaveConfig
		 */
		void			ReadConfig();
		/**
		 * Saves the configuration to a file.  ~/.sde/sde.cfg on unix systems.
		 * @see LocateConfigFile
		 * @see ReadConfig
		 */
		void			SaveConfig();

	protected:
		bool			FindIndex(const FName index, SettingsData *&data);

		bool firstRun;
		FString configDir;
		FString configFile;
		TMap<FName, TUniquePtr<SettingsData> > settings;
};

extern Config config;

#endif /* __CONFIG_HPP__ */
