#include "CommandLine.h"
#include "ZipSync.h"
#include "Downloader.h"
#include "Wildcards.h"
#include "Utils.h"
#include <thread>
#include <mutex>

#include "StdFilesystem.h"


namespace ZipSync {

ProgressIndicator::~ProgressIndicator() {}

std::function<int(double, const char*)> ProgressIndicator::GetDownloaderCallback() {
    return [this](double ratio, const char *comment) -> int {
        return Update(ratio, comment);
    };
}


std::vector<std::string> EnumerateFilesInDirectory(const std::string &root, bool skipErrors) {
    using ZipSync::PathAR;
    std::vector<std::string> res;
    std::vector<stdext::path> allPaths = stdext::recursive_directory_enumerate(stdext::path(root));
    for (auto& entry : allPaths) {
        try {
            if (stdext::is_regular_file(entry)) {
                std::string absPath = entry.string();   //.generic_string()
                std::string relPath = PathAR::FromAbs(absPath, root).rel;
                res.push_back(relPath);
            }
        } catch(...) {
            //exception example: name of user's file contains bad characters
            if (!skipErrors)
                throw;
        }
    }
    return res;
}
std::string GetCwd() {
    return stdext::current_path().string(); //.generic_string();
}
size_t SizeOfFile(const std::string &path) {
    return stdext::file_size(path);
}
void CreateDirectories(const std::string &path) {
    stdext::create_directories(path);
}

std::string NormalizeSlashes(std::string path) {
    for (char &ch : path)
        if (ch == '\\')
            ch = '/';
    if (path.size() > 1 && path.back() == '/')
        path.pop_back();
    return path;
}
bool StartsWith(const std::string &text, const std::string &prefix) {
    return text.size() > prefix.size() && text.substr(0, prefix.size()) == prefix;
}

std::string GetPath(std::string path, const std::string &root) {
    using ZipSync::PathAR;
    path = NormalizeSlashes(path);
    if (PathAR::IsAbsolute(path))
        return path;
    else
        return PathAR::FromRel(path, root).abs;
}

std::vector<std::string> CollectFilePaths(const std::vector<std::string> &elements, const std::string &root) {
    using ZipSync::PathAR;
    std::vector<std::string> resPaths;
    std::vector<std::string> wildcards;
    for (std::string str : elements) {
        str = NormalizeSlashes(str);
        if (PathAR::IsAbsolute(str))
            resPaths.push_back(str);
        else {
            if (str.find_first_of("*?") == std::string::npos)
                resPaths.push_back(PathAR::FromRel(str, root).abs);
            else
                wildcards.push_back(str);
        }
    }
    if (!wildcards.empty()) {
        auto allFiles = EnumerateFilesInDirectory(root);
        for (const std::string &path : allFiles) {
            bool matches = false;
            for (const std::string &wild : wildcards)
                if (WildcardMatch(wild.c_str(), path.c_str()))
                    matches = true;
            if (matches)
                resPaths.push_back(PathAR::FromRel(path, root).abs);
        }
    }

    //deduplicate
    std::set<std::string> resSet;
    int k = 0;
    for (int i = 0; i < resPaths.size(); i++) {
        auto pib = resSet.insert(resPaths[i]);
        if (pib.second)
            resPaths[k++] = resPaths[i];
    }
    resPaths.resize(k);

    return resPaths;
}

std::string DownloadSimple(const std::string &url, const std::string &rootDir, const char *printIndent) {
    std::string filepath;
    for (int i = 0; i < 100; i++) {
        filepath = rootDir + "/__download" + std::to_string(i) +  + "__" + ZipSync::GetFilename(url);
        if (!ZipSync::IfFileExists(filepath))
            break;
    }
    if (printIndent) {
        printf("%sDownloading %s to %s\n", printIndent, url.c_str(), filepath.c_str());
    }
    ZipSync::Downloader downloader;
    auto DataCallback = [&filepath](const void *data, int len) {
        ZipSync::StdioFileHolder f(filepath.c_str(), "wb");
        int res = fwrite(data, 1, len, f);
        if (res != len)
            throw std::runtime_error("Failed to write " + std::to_string(len) + " bytes downloaded from " + filepath);
    };
    downloader.EnqueueDownload(ZipSync::DownloadSource(url), DataCallback);
    downloader.DownloadAll();
    return filepath;
}

void ParallelFor(int from, int to, const std::function<void(int)> &body, int thrNum, int blockSize) {
    if (thrNum == 1) {
        for (int i = from; i < to; i++)
            body(i);
    }
    else {
        if (thrNum <= 0)
            thrNum = std::thread::hardware_concurrency();
        //int blockNum = (to - from + blockSize-1) / blockSize;

        std::vector<std::thread> threads(thrNum);
        int lastAssigned = from;
        std::exception_ptr workerException;
        std::mutex mutex;

        for (int t = 0; t < thrNum; t++) {
            auto ThreadFunc = [&,t]() {
                while (1) {
                    int left, right;
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        if (workerException || lastAssigned == to)
                            break;
                        left = lastAssigned;
                        right = std::min(lastAssigned + blockSize, to);
                        lastAssigned = right;
                    }
                    try {
                        for (int i = left; i < right; i++) {
                            body(i);
                        }
                    } catch(...) {
                        std::lock_guard<std::mutex> lock(mutex);
                        workerException = std::current_exception();
                        break;
                    }
                }
            };
            threads[t] = std::thread(ThreadFunc);
        }
        for (int t = 0; t < thrNum; t++)
            threads[t].join();

        if (workerException)
            std::rethrow_exception(workerException);
    }
}

double TotalCompressedSize(const ZipSync::Manifest &mani, bool providedOnly) {
    double size = 0.0;
    for (int i = 0; i < mani.size(); i++) {
        if (providedOnly && mani[i].location == ZipSync::FileLocation::Nowhere)
            continue;
        size += mani[i].props.compressedSize;
    }
    return size;
}
int TotalCount(const ZipSync::Manifest &mani, bool providedOnly) {
    int cnt = 0;
    for (int i = 0; i < mani.size(); i++) {
        if (providedOnly && mani[i].location == ZipSync::FileLocation::Nowhere)
            continue;
        cnt++;
    }
    return cnt;
}

void DoClean(std::string root) {
    static std::string DELETE_PREFIXES[] = {"__reduced__", "__download", "__repacked__"};
    static std::string RESTORE_PREFIX = "__repacked__";

    std::vector<std::string> allFiles = EnumerateFilesInDirectory(root);
    for (std::string filename : allFiles) {
        std::string fn = ZipSync::GetFilename(filename);

        bool shouldDelete = false;
        for (const std::string &p : DELETE_PREFIXES)
            if (StartsWith(fn, p))
                shouldDelete = true;
        if (!shouldDelete)
            continue;

        if (StartsWith(fn, RESTORE_PREFIX)) {
            std::string shouldRestore = UnPrefixFile(filename, RESTORE_PREFIX);
            std::string fullOldPath = root + '/' + filename;
            std::string fullNewPath = root + '/' + shouldRestore;
            if (!ZipSync::IfFileExists(fullNewPath)) {
                g_logger->infof("Restoring %s...", shouldRestore.c_str());
                ZipSync::RenameFile(fullOldPath, fullNewPath);
                continue;
            }
        }

        std::string fullPath = root + '/' + filename;
        g_logger->infof("Deleting %s...", filename.c_str());
        ZipSync::RemoveFile(fullPath);
    }
}

void DoNormalize(std::string root, std::string outDir, std::vector<std::string> zipPaths, ProgressIndicator *progress) {
    double totalSize = 1.0, doneSize = 0.0;
    for (auto zip : zipPaths)
        totalSize += SizeOfFile(zip);
    g_logger->infof("Going to normalize %d zips in %s%s of total size %0.3lf MB", int(zipPaths.size()), (root.empty() ? "nowhere" : root.c_str()), (outDir.empty() ? " inplace" : ""), totalSize * 1e-6);

    {
        for (std::string zip : zipPaths) {
            std::string zipRel = PathAR::FromAbs(zip, root).rel;
            if (progress) progress->Update(doneSize / totalSize, ("Normalizing \"" + zipRel + "\"...").c_str());
            doneSize += SizeOfFile(zip);
            if (!outDir.empty()) {
                std::string zipOut = ZipSync::PathAR::FromRel(zipRel, outDir).abs;
                ZipSync::CreateDirectoriesForFile(zipOut, outDir);
                ZipSync::minizipNormalize(zip.c_str(), zipOut.c_str());
            }
            else
                ZipSync::minizipNormalize(zip.c_str());
        }
        if (progress) progress->Update(1.0, "Normalizing done");
    }
}

Manifest DoAnalyze(std::string root, std::vector<std::string> zipPaths, bool autoNormalize, int threadsNum, ProgressIndicator *progress) {
    double totalSize = 1.0, doneSize = 0.0;
    for (auto zip : zipPaths)
        totalSize += SizeOfFile(zip);
    g_logger->infof("Going to analyze %d zips in %s of total size %0.3lf MB in %d threads", int(zipPaths.size()), root.c_str(), totalSize * 1e-6, threadsNum);

    std::vector<Manifest> zipManis(zipPaths.size());
    {
        std::mutex mutex;
        ParallelFor(0, zipPaths.size(), [&](int index) {
            std::string zipPath = zipPaths[index];
            std::string zipPathRel = PathAR::FromAbs(zipPath, root).rel;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (progress) progress->Update(doneSize / totalSize, "Analysing \"" + zipPathRel + "\"...");
            }

            if (autoNormalize) {
                try {
                    //try to analyze "as is"
                    zipManis[index].AppendLocalZip(zipPath, root, "");
                }
                catch(const ErrorException &e) {
                    zipManis[index].Clear();
                    //failed: normalize and retry
                    ZipSync::minizipNormalize(zipPath.c_str());
                    zipManis[index].AppendLocalZip(zipPath, root, "");
                }
            }
            else {
                zipManis[index].AppendLocalZip(zipPath, root, "");
            }

            {
                std::lock_guard<std::mutex> lock(mutex);
                doneSize += SizeOfFile(zipPath);
                if (progress) progress->Update(doneSize / totalSize, "Analysed  \"" + zipPathRel + "\"");
            }
        }, threadsNum);
        if (progress) progress->Update(1.0, "Analysing done");
    }

    Manifest manifest;
    for (const auto &tm : zipManis)
        manifest.AppendManifest(tm);
    return manifest;
}

}
