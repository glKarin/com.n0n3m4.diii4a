#pragma once

#include <vector>
#include <string>
#include <algorithm>

namespace ZipSync {

/// Contents of one section of ini file (note: ordered).
typedef std::vector<std::pair<std::string, std::string>> IniSect;
/// Contents of an ini file (note: ordered).
typedef std::vector<std::pair<std::string, IniSect>> IniData;

enum class IniMode {
    Auto = 0,           //auto-choose depending on extension (.ini vs .iniz)
    Plain,              //normal text
    Zipped,             //a zipfile with .ini inside
};

void WriteIniFile(const char *path, const IniData &data, IniMode mode = IniMode::Auto);
IniData ReadIniFile(const char *path, IniMode mode = IniMode::Auto);

}
