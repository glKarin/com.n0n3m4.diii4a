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

#pragma once

#include "StdString.h"

#include "TraceLog.h"
#include <map>
#include <sstream>
#undef max
#include <algorithm>


namespace tdm
{

class ProgramOptions
{
public:
	struct Option {
		std::string key;
		bool needsValue;				//if false, then it is "key" argument
		std::string defValue;			//if empty => no default value
		std::string description;

		static Option Flag(const std::string &key, const std::string &desc) {
			Option res;
			res.key = key;
			res.needsValue = false;
			res.description = desc;
			return res;
		}
		static Option Str(const std::string &key, const std::string &desc, const std::string &defValue = std::string()) {
			Option res;
			res.key = key;
			res.needsValue = true;
			res.description = desc;
			res.defValue = defValue;
			return res;
		}
	};

protected:

	std::vector<Option> _desc;
	std::map<std::string, std::string> _vm;

	// The command line arguments for reference
	std::vector<std::string> _cmdLineArgs;

public:
	virtual ~ProgramOptions()
	{}

	void ParseFromCommandLine(int argc, char* argv[])
	{
		auto check = [](bool condition, const std::string &message, const std::string &arg) {
			if (condition)
				return;
			throw std::runtime_error(message + ": " + arg);
		};

		try {
			for (int i = 1; i < argc; i++) {
				std::string arg = argv[i];
				bool handled = false;
				for (const Option &opt : _desc) {
					if (arg == "--" + opt.key) {
						if (opt.needsValue) {
							check(i+1 < argc && !stdext::starts_with(argv[i+1], "--"), "Missing value for parameter", arg);
							_vm[opt.key] = argv[i+1];
							i++;
						}
						else {
							_vm[opt.key] = "";
						}
						handled = true;
						break;
					}
					if (stdext::starts_with(arg, "--" + opt.key + "=")) {
						check(opt.needsValue, "Specified value for key parameter", arg);
						std::string value = arg.substr(opt.key.size() + 3);
						_vm[opt.key] = value;
						handled = true;
						break;
					}
				}
				check(handled, "Unknown argument", arg);
			}

			//set default values for non-set options (where appropriate)
			for (const Option &opt : _desc) {
				if (opt.needsValue && !opt.defValue.empty() && !IsSet(opt.key)) {
					_vm[opt.key] = opt.defValue;
				}
			}
		}
		catch (std::runtime_error &e) {
			TraceLog::WriteLine(LOG_STANDARD, " " + std::string(e.what()));
		}
	}

	void Set(const std::string& key)
	{
		_vm.insert(std::make_pair(key, ""));
		_cmdLineArgs.push_back("--" + key);
	}

	void Set(const std::string& key, const std::string& value)
	{
		_vm.insert(std::make_pair(key, value));
		_cmdLineArgs.push_back("--" + key + "=" + value);
	}

	void Unset(const std::string& key)
	{
		_vm.erase(key);

		for (std::vector<std::string>::iterator i = _cmdLineArgs.begin(); 
			 i != _cmdLineArgs.end(); ++i)
		{
			if (stdext::starts_with(*i, "--" + key))
			{
				_cmdLineArgs.erase(i);
				break;
			}
		}
	}

	bool Empty() const
	{
		return _vm.empty();
	}

	bool IsSet(const std::string& key) const
	{
		return _vm.count(key) > 0;
	}

	std::string Get(const std::string& key) const
	{
		return _vm.count(key) > 0 ? _vm.find(key)->second : "";
	}

	const std::vector<std::string>& GetRawCmdLineArgs() const
	{
		return _cmdLineArgs;
	}

	virtual void PrintHelp()
	{
		std::ostringstream stream;
		stream << "\n";

		//format list of options into a two-column table
		std::vector<std::pair<std::string, std::string>> table;
		int leftLength = 0;
		for (const Option &opt : _desc) {
			std::string leftS = std::string("--") + opt.key;
			if (opt.needsValue)
				leftS += " arg";
			if (opt.defValue.size())
				leftS += " (=" + opt.defValue + ")";
			std::string rightS = opt.description;
			rightS = stdext::replace_all_copy(rightS, "\n", " ");
			table.emplace_back(leftS, rightS);
			leftLength = std::max(leftLength, (int)leftS.size());
		}
		//print the table
		for (auto line : table) {
			std::string leftS = line.first;
			leftS += std::string(leftLength - leftS.size(), ' ');
			stream << "  " << leftS << "  " << line.second << "\n";
		}

		TraceLog::WriteLine(LOG_STANDARD, " " + stream.str());
	}

protected:
	/**
	 * Subclasses should implement this method to populate the available options
	 * and call it in their constructors.
	 */
	virtual void SetupDescription() = 0;
};

}
