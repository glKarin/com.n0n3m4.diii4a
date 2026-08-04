#pragma once

#include <vector>
#include <string>
#include <functional>
#include "Manifest.h"

//note: this is a set of utilities extracted from zipsync command line tool

namespace ZipSync {

class ProgressIndicator {
public:
    virtual ~ProgressIndicator();
    virtual int Update(double globalRatio, std::string globalComment, double localRatio = -1.0, std::string localComment = "") = 0;
    std::function<int(double, const char*)> GetDownloaderCallback();
};

std::vector<std::string> EnumerateFilesInDirectory(const std::string &root, bool skipErrors = true);
std::string GetCwd();
size_t SizeOfFile(const std::string &path);
void CreateDirectories(const std::string &path);

std::string NormalizeSlashes(std::string path);
bool StartsWith(const std::string &text, const std::string &prefix);

std::string GetPath(std::string path, const std::string &root);
std::vector<std::string> CollectFilePaths(const std::vector<std::string> &elements, const std::string &root);

std::string DownloadSimple(const std::string &url, const std::string &rootDir, const char *printIndent = "");

void ParallelFor(int from, int to, const std::function<void(int)> &body, int thrNum = -1, int blockSize = 1);
double TotalCompressedSize(const ZipSync::Manifest &mani, bool providedOnly = true);
int TotalCount(const ZipSync::Manifest &mani, bool providedOnly = true);

void DoClean(std::string root);
void DoNormalize(std::string root, std::string outDir, std::vector<std::string> zipPaths, ProgressIndicator *progress = nullptr);
Manifest DoAnalyze(std::string root, std::vector<std::string> zipPaths, bool autoNormalize, int threadsNum, ProgressIndicator *progress = nullptr);

}
