#include "Ini.h"
#include "Utils.h"
#include "StdString.h"
#include "Logging.h"
#include "ZipUtils.h"
#include "Hash.h"
#include <string.h>
#include "ChecksummedZip.h"


namespace ZipSync {

void WriteIniFile(const char *path, const IniData &data, IniMode mode) {
    ZipSyncAssertF(path[0], "Path to write INI file is empty");
    std::string text;
    for (const auto &pNS : data) {
        char buffer[SIZE_LINEBUFFER];
        sprintf(buffer, "[%s]\n", pNS.first.c_str());
        text.append(buffer);
        for (const auto &pKV : pNS.second) {
            sprintf(buffer, "%s=%s\n", pKV.first.c_str(), pKV.second.c_str());
            text.append(buffer);
        }
        text.append("\n");
    }
    if (mode == IniMode::Auto)
        mode = (path[strlen(path)-1] == 'z' ? IniMode::Zipped : IniMode::Plain);
    if (mode == IniMode::Zipped)
        WriteChecksummedZip(path, text.data(), text.size(), "data.ini");
    else {
        StdioFileHolder f(path, "wb");
        fwrite(text.data(), 1, text.size(), f);
    }
}

IniData ReadIniFile(const char *path, IniMode mode) {
    std::vector<char> text;
    if (mode == IniMode::Auto)
        mode = (path[strlen(path)-1] == 'z' ? IniMode::Zipped : IniMode::Plain);
    if (mode == IniMode::Zipped) {
        auto data = ReadChecksummedZip(path, "data.ini");
        text.assign(data.begin(), data.end());
    }
    else {
        StdioFileHolder f(path, "rb");
        char buffer[SIZE_FILEBUFFER];
        while (int bytes = fread(buffer, 1, SIZE_FILEBUFFER, f))
            text.insert(text.end(), buffer, buffer+bytes);
    }

    IniData ini;
    IniSect sec;
    std::string name;
    auto CommitSec = [&]() {
        if (!name.empty())
            ini.push_back(std::make_pair(std::move(name), std::move(sec)));
        name.clear();
        sec.clear();
    };
    size_t textPos = 0;
    std::string line;
    while (textPos < text.size()) {
        size_t eolPos = std::find(text.data() + textPos, text.data() + text.size(), '\n') - text.data();
        line.assign(text.data() + textPos, text.data() + eolPos);
        textPos = eolPos + 1;

        stdext::trim(line);
        if (line.empty())
            continue;
        if (line[0] == '#') //comment
            continue;
        if (line.front() == '[' && line.back() == ']') {
            CommitSec();
            name = line.substr(1, line.size() - 2);
        }
        else {
            size_t pos = line.find('=');
            ZipSyncAssertF(pos != std::string::npos, "Cannot parse ini line: %s", line.c_str());
            sec.emplace_back(line.substr(0, pos), line.substr(pos+1));
        }
    }
    CommitSec();
    return ini;
}

}
