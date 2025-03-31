#include "Path.h"
#include "Logging.h"
#include "StdString.h"
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <cstdint>
#include <sys/stat.h>
#endif


namespace ZipSync {

bool PathAR::IsHttp(const std::string &path) {
    return stdext::starts_with(path, "http://");
}

bool PathAR::IsAbsolute(const std::string &path) {
    if (path.empty())
        return false;   //wrong path, actually
    if (path[0] == '/')
        return true;
    if (path.find(':') != std::string::npos)
        return true;
    return false;
}

static void CheckPath(const std::string &path, bool relative) {
    for (int i = 0; i < path.size(); i++)
        ZipSyncAssertF(uint8_t(path[i]) >= 32, "Non-printable character %d in path", int(path[i]));
    ZipSyncAssertF(!path.empty() && path != "/", "Empty path [%s]", path.c_str());
    ZipSyncAssertF(path.find_first_of("\\|[]=?&") == std::string::npos, "Forbidden symbol in path %s", path.c_str());
    ZipSyncAssertF(path[0] != '.', "Path must not start with dot: %s", path.c_str());
    if (relative) {
        ZipSyncAssertF(path.find_first_of(":") == std::string::npos, "Colon in relative path %s", path.c_str());
        ZipSyncAssertF(path[0] != '/', "Relative path starts with slash: %s", path.c_str())
    }
}

PathAR PathAR::FromAbs(std::string absPath, std::string rootDir) {
    CheckPath(rootDir, false);
    CheckPath(absPath, false);
    bool lastSlash = (rootDir.back() == '/');
    int len = rootDir.size() - (lastSlash ? 1 : 0);
    ZipSyncAssertF(strncmp(absPath.c_str(), rootDir.c_str(), len) == 0, "Abs path %s is not within root dir %s", absPath.c_str(), rootDir.c_str());
    ZipSyncAssertF(absPath.size() > len && absPath[len] == '/', "Abs path %s is not within root dir %s", absPath.c_str(), rootDir.c_str());
    PathAR res;
    res.abs = absPath;
    res.rel = absPath.substr(len+1);
    return res;
}
PathAR PathAR::FromRel(std::string relPath, std::string rootDir) {
    CheckPath(rootDir, false);
    CheckPath(relPath, true);
    bool lastSlash = (rootDir.back() == '/');
    PathAR res;
    res.rel = relPath;
    res.abs = rootDir + (lastSlash ? "" : "/") + relPath;
    return res;
}

std::string PathAR::GetRootDir() const {
    ZipSyncAssertF(abs.size() > rel.size() && rel == abs.substr(abs.size() - rel.size()), "Absolute path %s doesn't have relative path %s as prefix", abs.c_str(), rel.c_str());
    return abs.substr(0, abs.size() - rel.size() - 1);
}

std::string PrefixFile(std::string absPath, std::string prefix) {
    size_t pos = absPath.find_last_of('/');
    if (pos != std::string::npos)
        pos++;
    else
        pos = 0;
    absPath.insert(absPath.begin() + pos, prefix.begin(), prefix.end());
    return absPath;
}

std::string UnPrefixFile(std::string absPath, std::string prefix) {
    size_t pos = absPath.rfind('/' + prefix);
    if (pos != std::string::npos)
        absPath.erase(pos + 1, prefix.size());
    else if (stdext::starts_with(absPath, prefix))
        absPath.erase(0, prefix.size());
    return absPath;
}

std::string GetDirPath(std::string somePath) {
    size_t pos = somePath.find_last_of('/');
    ZipSyncAssert(pos != std::string::npos);
    return somePath.substr(0, pos);
}
std::string GetFilename(std::string somePath) {
    size_t pos = somePath.find_last_of('/');
    if (pos == std::string::npos)
        return somePath;
    ZipSyncAssert(pos + 1 < somePath.size());
    return somePath.substr(pos + 1);
}

std::string GetFullPath(const std::string &zipPath, const std::string &filename) {
    return zipPath + "||" + filename;
}
void ParseFullPath(const std::string &fullPath, std::string &zipPath, std::string &filename) {
    size_t pos = fullPath.find("||");
    ZipSyncAssertF(pos != std::string::npos, "Cannot split fullname into zip path and filename: %s", fullPath.c_str());
    zipPath = fullPath.substr(0, pos);
    filename = fullPath.substr(pos + 2);
}


bool IfFileExists(const std::string &path) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return false;
    fclose(f);
    return true;
}

void RemoveFile(const std::string &path) {
    int res = remove(path.c_str());
    ZipSyncAssertF(res == 0, "Failed to remove file %s (error %d)", path.c_str(), res);
}

void RenameFile(const std::string &oldPath, const std::string &newPath) {
    int res = rename(oldPath.c_str(), newPath.c_str());
    ZipSyncAssertF(res == 0, "Failed to rename file %s to %s (error %d)", oldPath.c_str(), newPath.c_str(), res);
}

bool CreateDir(const std::string &dirPath) {
    int res = 0;
#ifdef _WIN32
    res = _mkdir(dirPath.c_str());
#else
    res = mkdir(dirPath.c_str(), ACCESSPERMS);
#endif
    if (res == -1)
        res = errno;
    if (res == EEXIST)
        return false;
    ZipSyncAssertF(res == 0, "Failed to create directory %s (error %d)", dirPath.c_str(), res);
    return true;
}

void CreateDirectoriesForFile(const std::string &filePath, const std::string &rootPath) {
    PathAR p = PathAR::FromAbs(filePath, rootPath);
    std::vector<std::string> components;
    stdext::split(components, p.rel, "/");
    std::string curr = rootPath;
    for (int i = 0; i+1 < components.size(); i++) {
        curr += "/";
        curr += components[i];
        CreateDir(curr);
    }
}

}
