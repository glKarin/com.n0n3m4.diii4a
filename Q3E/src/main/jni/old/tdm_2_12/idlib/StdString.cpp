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
#include "StdString.h"

namespace stdext {
	void split(std::vector<std::string> &tokens, const std::string &text, const char *delimiters) {
		tokens.clear();
		size_t pos = 0;
		while (true) {
			size_t left = text.find_first_not_of(delimiters, pos);
			if (left == std::string::npos)
				break;
			size_t right = text.find_first_of(delimiters, left);
			if (right == std::string::npos)
				right = text.length();
			tokens.push_back(text.substr(left, right - left));
			pos = right;
		}
	}

	std::string join(const std::vector<std::string> &tokens, const char *separator) {
		if (tokens.empty())
			return std::string();

		std::string res = tokens[0];
		for (size_t i = 1; i < tokens.size(); i++) {
			res += separator;
			res += tokens[i];
		}
		return res;
	}

	void trim(std::string &text, const char *delimiters) {
		size_t pos = text.find_first_not_of(delimiters);
		if (pos == std::string::npos) {
			text.clear();
			return;
		}
		text.erase(text.begin(), text.begin() + pos);
		pos = text.find_last_not_of(delimiters) + 1;
		text.erase(text.begin() + pos, text.end());
	}

	std::string to_lower_copy(const std::string &text) {
		idStr temp(text.c_str());
		temp.ToLower();
		return std::string(temp);
	}

	bool iends_with(const std::string &text, const std::string &suffix) {
		return idStr::IendsWith(text.c_str(), suffix.c_str());
	}
}
