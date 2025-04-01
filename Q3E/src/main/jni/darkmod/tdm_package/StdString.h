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

#include <vector>
#include <string>

namespace stdext {
	//alternative to boost::algorithm::split
	void split(std::vector<std::string> &tokens, const std::string &text, const char *delimiters = " \n\r\t\f\v");
	//alternative to boost::algorithm::join
	std::string join(const std::vector<std::string> &tokens, const char *separator);
	//alternative to boost::algorithm::trim(_if)
	void trim(std::string &text, const char *delimiters = " \n\r\t\f\v");
	//alternative to boost::algorithm::trim_copy
	std::string trim_copy(const std::string &text);
	//alternative to boost::to_lower_copy
	std::string to_lower_copy(const std::string &text);
	//alternative to boost::ends_with
	bool ends_with(const std::string &text, const std::string &suffix);
	//alternative to boost::iends_with
	bool iends_with(const std::string &text, const std::string &suffix);
	//alternative to boost::starts_with
	bool starts_with(const std::string &text, const std::string &prefix);
	//alternative to boost::istarts_with
	bool istarts_with(const std::string &text, const std::string &prefix);
	//sort of alternative to boost::replace_all
	std::string replace_all_copy(const std::string &text, const std::string &from, const std::string &to);
}
