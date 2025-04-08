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
#pragma hdrstop



#include "IniFile.h"

#if defined(ID_DEBUG_MEMORY) && defined(ID_REDIRECT_NEWDELETE)
	#undef new
#endif

#include <set>
#include <fstream>
#include <sstream>

#include "StdString.h"

IniFile::IniFile()
{}

IniFile::IniFile(const std::string& str)
{
	ParseFromString(str);
}

IniFilePtr IniFile::Create()
{
	return IniFilePtr(new IniFile);
}

IniFilePtr IniFile::ConstructFromFile(const fs::path& filename)
{
	// Start parsing
	std::ifstream iniFile(filename.string().c_str());

	if (!iniFile)
    {
		return IniFilePtr();
    }

	return ConstructFromStream(iniFile);
}

IniFilePtr IniFile::ConstructFromStream(std::istream& stream)
{
	// Read the whole stream into a string
	std::string buffer(std::istreambuf_iterator<char>(stream), (std::istreambuf_iterator<char>()));

	return ConstructFromString(buffer);
}

IniFilePtr IniFile::ConstructFromString(const std::string& str)
{
	return IniFilePtr(new IniFile(str));
}

bool IniFile::IsEmpty() const
{
	return _settings.empty();
}

void IniFile::AddSection(const std::string& name)
{
	_settings.insert(SettingMap::value_type(name, KeyValues()));
}

std::string IniFile::GetValue(const std::string& section, const std::string& key) const
{
	SettingMap::const_iterator i = _settings.find(section);
	
	if (i == _settings.end()) return ""; // section not found
	
	KeyValues::const_iterator kv = i->second.find(KeyValuePair(key, ""));

	return (kv != i->second.end()) ? kv->second : "";
}

void IniFile::SetValue(const std::string& section, const std::string& key, const std::string& value)
{
	// Find the section, and create it if necessary
	SettingMap::iterator i = _settings.find(section);

	if (i == _settings.end())
	{
		AddSection(section);
	}

	// Section exists past this point

	KeyValues::iterator kv = _settings[section].find(KeyValuePair(key, ""));

	// Remove existing key value first
	if (kv != _settings[section].end())
	{
		_settings[section].erase(kv);
	}

	// Insert afresh
	_settings[section].insert(KeyValuePair(key, value));
}

bool IniFile::RemoveSection(const std::string& section)
{
	SettingMap::iterator i = _settings.find(section);

	if (i != _settings.end())
	{
		_settings.erase(i);
		return true;
	}

	return false; // not found
}

bool IniFile::RemoveKey(const std::string& section, const std::string& key)
{
	SettingMap::iterator i = _settings.find(section);

	if (i == _settings.end())
	{
		return false; // not found
	}

	KeyValues::iterator kv = i->second.find(KeyValuePair(key, ""));

	if (kv != i->second.end())
	{
		i->second.erase(kv);
		return true;
	}

	return false; // not found
}

IniFile::KeyValuePairList IniFile::GetAllKeyValues(const std::string& section) const
{
	SettingMap::const_iterator i = _settings.find(section);

	if (i == _settings.end())
	{
		return KeyValuePairList(); // not found
	}

	return KeyValuePairList(i->second.begin(), i->second.end());
}

void IniFile::ForeachSection(SectionVisitor& visitor) const
{
	for (SettingMap::const_iterator i = _settings.begin(); 
		 i != _settings.end(); /* in-loop increment */)
	{
		visitor.VisitSection(*this, (*i++).first);
	}
}

void IniFile::ExportToFile(const fs::path& file, const std::string& headerComments) const
{
	std::ofstream stream(file.string().c_str());

	if (!headerComments.empty())
	{
		// Split the header text into lines and export it as INI comment
		std::vector<std::string> lines;
		stdext::split(lines, headerComments, "\n");

		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			stream << "# " << lines[i] << std::endl;
		}

		// add some additional line break after the header
		stream << std::endl;
	}

	for (SettingMap::const_iterator i = _settings.begin(); i != _settings.end(); ++i)
	{
		stream << "[" << i->first << "]" << std::endl;

		for (KeyValues::const_iterator kv = i->second.begin(); kv != i->second.end(); ++kv)
		{
			stream << kv->first << " = " << kv->second << std::endl;
		}
		
		stream << std::endl;
	}
}

// Functor class adding INI sections and keyvalues
// Keeps track of the most recently added section and key
// as the Add* methods are called in the order of parsing without context.
class IniParser
{
private:
	IniFile& _self;
	
	// Most recently added section and key
	std::string _lastSection;
	std::string _lastKey;

    // To avoid a compiler warning, just define an inaccessible assignment operator
    IniParser& operator=(const IniParser& other);

public:
	IniParser(IniFile& self) :
		_self(self)
	{}

	void AddSection(const std::string &token)
	{
		// Remember this section name
		_lastSection = token;

		_self.AddSection(_lastSection);
	}

	void AddKey(const std::string &token)
	{
		assert(!_lastSection.empty()); // need to have parsed a section beforehand

		// Just remember the key name, an AddValue() call is imminent
		_lastKey = token;

		stdext::trim(_lastKey);
	}

	void AddValue(const std::string &token)
	{
		assert(!_lastSection.empty());
		assert(!_lastKey.empty());

		_self.SetValue(_lastSection, _lastKey, token);
	}
};

void IniFile::ParseFromString(const std::string& str)
{
	auto npos = std::string::npos;
	IniParser parser(*this);

	//split file into non-empty lines
	std::vector<std::string> lines;
	stdext::split(lines, str, "\r\n");
	for (auto line : lines) {
		//remove comment (if present)
		size_t commentStart = line.find_first_of("#;");
		if (commentStart != npos)
			line.resize(commentStart);
		//remove spaces at both ends
		stdext::trim(line);
		if (line.empty())
			continue;
		//check if it is section start
		if (line.front() == '[' && line.back() == ']') {
			auto sectionName = line.substr(1, line.length() - 2);
			parser.AddSection(sectionName);
			continue;
		}
		//check if it is "key=value" definition
		size_t equalCount = std::count(line.begin(), line.end(), '=');
		if (equalCount == 1) {
			size_t equalPos = line.find('=');
			std::string key = line.substr(0, equalPos), value = line.substr(equalPos + 1);
			stdext::trim(key);
			stdext::trim(value);
			if (key.empty()) {
				common->Warning("Empty key in INI file: '%s'", line.c_str());
				continue;
			}
			parser.AddKey(key);
			parser.AddValue(value);
			continue;
		}
		//check what sort of error it is
		if (equalCount > 1)
			common->Warning("%zu equality signs in a line of INI file: '%s'", equalCount, line.c_str());
		else
			common->Warning("Excessive line in INI file: '%s'", line.c_str());
	}
}
