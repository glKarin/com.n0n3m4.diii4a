#include "CommandLine.h"
#include "ZipSync.h"
#include "ChecksummedZip.h"
#include "Utils.h"
#include "StdString.h"
#include "args.hxx"
#include <stdio.h>
#include <iostream>
#include <map>
#include <random>
#include <mutex>

using namespace ZipSync;


class ProgressIndicatorConsole : public ProgressIndicator {
    std::string content;
public:
    ~ProgressIndicatorConsole() override {
        Finish();
    }
    int Update(double globalRatio, std::string globalComment, double localRatio = -1.0, std::string localComment = "") override {
        auto PercentOf = [](double value) { return int(value * 100.0 + 0.5); };
        char buffer[1024];
        if (localRatio != -1.0 && localComment.size())
            sprintf(buffer, " %3d%% | %3d%% : %s : %s", PercentOf(globalRatio), PercentOf(localRatio), globalComment.c_str(), localComment.c_str());
        else
            sprintf(buffer, " %3d%%        : %s", PercentOf(globalRatio), globalComment.c_str());
        Erase();
        content = buffer;
        printf("%s", content.c_str());
        return 0;
    }
private:
    void Erase() {
        if (content.empty())
            return;
        printf("\r");
        for (int i = 0; i < content.size(); i++)
            printf(" ");
        printf("\r");
        content.clear();
    }
    void Finish() {
        if (content.empty())
            return;
        printf("\n");
    }
};

void CommandClean(args::Subparser &parser) {
    args::ValueFlag<std::string> argRootDir(parser, "root", "the root directory to clean after repack\n", {'r', "root"});
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"}, args::Options::HiddenFromDescription);
    parser.Parse();

    std::string root = GetCwd();
    if (argRootDir)
        root = argRootDir.Get();
    root = NormalizeSlashes(root);
    DoClean(root);
}

void CommandNormalize(args::Subparser &parser) {
    args::ValueFlag<std::string> argRootDir(parser, "root", "Relative paths to zips are based from this directory", {'r', "root"});
    args::PositionalList<std::string> argZips(parser, "zips", "List of files or globs specifying which zips in root directory to include", args::Options::Required);
    args::ValueFlag<std::string> argOutDir(parser, "output", "Write normalized zips to this directory (instead of modifying in-place)", {'o', "output"});
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"}, args::Options::HiddenFromDescription);
    parser.Parse();

    std::string root = GetCwd();
    if (argRootDir)
        root = argRootDir.Get();
    root = NormalizeSlashes(root);
    std::string outDir;
    if (argOutDir)
        outDir = NormalizeSlashes(argOutDir.Get());
    std::vector<std::string> zipPaths = CollectFilePaths(argZips.Get(), root);

    {
        ProgressIndicatorConsole progress;
        DoNormalize(root, outDir, zipPaths, &progress);
    }
}

void CommandAnalyze(args::Subparser &parser) {
    args::ValueFlag<std::string> argRootDir(parser, "root", "Manifests would contain paths relative to this root directory\n"
        "(all relative paths are based from the root directory)", {'r', "root"});
    args::Flag argClean(parser, "clean", "Run \"clean\" command before doing analysis", {'c', "clean"});
    args::Flag argNormalize(parser, "normalize", "Run \"normalize\" command before analysis (on demand)", {'n', "normalize"});
    args::ValueFlag<std::string> argManifest(parser, "mani", "Path where full manifest would be written (default: manifest.iniz)", {'m', "manifest"}, "manifest.iniz");
    args::ValueFlag<int> argThreads(parser, "threads", "Use this number of parallel threads to accelerate analysis (0 = max)", {'j', "threads"}, 1);
    args::PositionalList<std::string> argZips(parser, "zips", "List of files or globs specifying which zips in root directory to analyze", args::Options::Required);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"}, args::Options::HiddenFromDescription);
    parser.Parse();

    std::string root = GetCwd();
    if (argRootDir)
        root = argRootDir.Get();
    root = NormalizeSlashes(root);
    std::string maniPath = GetPath(argManifest.Get(), root);
    int threadsNum = argThreads.Get();

    if (argClean)
        DoClean(root);
    std::vector<std::string> zipPaths = CollectFilePaths(argZips.Get(), root);

    Manifest manifest;
    {
        ProgressIndicatorConsole progress;
        manifest = DoAnalyze(root, zipPaths, argNormalize, threadsNum, &progress);
    }
    g_logger->infof("Saving manifest to %s", maniPath.c_str());
    WriteIniFile(maniPath.c_str(), manifest.WriteToIni());
}

void CommandDiff(args::Subparser &parser) {
    args::ValueFlag<std::string> argRootDir(parser, "root", "The set of zips is located in this root directory\n"
        "(all relative paths are based from it)", {'r', "root"});
    args::ValueFlag<std::string> argManifest(parser, "mani", "Path to full manifest of the zips set", {'m', "manifest"}, "manifest.iniz");
    args::ValueFlagList<std::string> argSubtractedMani(parser, "subm", "Paths or URLs of provided manifests being subtracted", {'s', "subtract"}, {}, args::Options::Required);
    args::ValueFlag<std::string> argOutDir(parser, "output", "Difference zips and manifest will be written to this directory", {'o', "output"}, args::Options::Required);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"}, args::Options::HiddenFromDescription);
    parser.Parse();

    std::string root = GetCwd();
    if (argRootDir)
        root = argRootDir.Get();
    root = NormalizeSlashes(root);
    std::string outRoot = root;
    if (argOutDir)
        outRoot = argOutDir.Get();
    outRoot = NormalizeSlashes(outRoot);
    std::string maniPath = GetPath(argManifest.Get(), root);
    std::string outManiPath = GetPath(argManifest.Get(), outRoot);
    if (EnumerateFilesInDirectory(outRoot).size() > 0)
        throw std::runtime_error("Output directory is not empty: " + outRoot);
    CreateDirectories(outRoot);

    Manifest fullMani;
    fullMani.ReadFromIni(ReadIniFile(maniPath.c_str()), root);
    printf("Subtracting from %s containing %d files of size %0.3lf MB:\n", 
        maniPath.c_str(), TotalCount(fullMani), TotalCompressedSize(fullMani) * 1e-6
    );
    std::set<HashDigest> subtractedHashes;
    for (std::string path : argSubtractedMani.Get()) {
        path = NormalizeSlashes(path);
        std::string localPath = path;
        if (PathAR::IsHttp(path))
            localPath = DownloadSimple(path, outRoot, "  ");
        std::string providedRoot = GetDirPath(path);
        Manifest mani;
        mani.ReadFromIni(ReadIniFile(localPath.c_str()), providedRoot);
        printf("   %s containing %d files of size %0.3lf MB\n", 
            path.c_str(), TotalCount(mani), TotalCompressedSize(mani) * 1e-6
        );
        for (int i = 0; i < mani.size(); i++)
            subtractedHashes.insert(mani[i].compressedHash);
    }

    Manifest filteredMani;
    Manifest subtractedMani;
    for (int i = 0; i < fullMani.size(); i++) {
        auto &pf = fullMani[i];
        if (subtractedHashes.count(pf.compressedHash))
            subtractedMani.AppendFile(pf);
        else
            filteredMani.AppendFile(pf);
    }
    printf("Result will be written to %s containing %d files of size %0.3lf MB\n", 
        outRoot.c_str(), TotalCount(filteredMani), TotalCompressedSize(filteredMani) * 1e-6
    );

    UpdateProcess update;
    update.Init(filteredMani, filteredMani, outRoot);
    bool ok = update.DevelopPlan(UpdateType::SameCompressed);
    if (!ok)
        throw std::runtime_error("Internal error: DevelopPlan failed");
    update.RepackZips();
    Manifest provMani = update.GetProvidedManifest();

    fullMani = provMani.Filter([](const FileMetainfo &f) {
        return f.location == FileLocation::Inplace;
    });
    for (int i = 0; i < subtractedMani.size(); i++) {
        auto &pf = subtractedMani[i];
        pf.DontProvide();
        fullMani.AppendFile(pf);
    }
    printf("Saving manifest of the diff to %s\n", outManiPath.c_str());
    WriteIniFile(outManiPath.c_str(), fullMani.WriteToIni());
}

void CommandPatch(args::Subparser &parser) {
    args::ValueFlag<std::string> argRootDir(parser, "root", "The set of zips is located in this root directory\n"
        "(all relative paths are based from it)", {'r', "root"});
    args::ValueFlag<std::string> argPatchMani(parser, "mani", "Path to manifest describing added set of zips", {'m', "manifest"}, "manifest.iniz");
    args::ValueFlag<std::string> argBaseMani(parser, "base", "Path or URL of manifest, which is the base being patched", {'b', "base"}, args::Options::Required);
    args::ValueFlag<std::string> argOutMani(parser, "outMani", "Path to new manifest describing the whole patched version", {'o', "output"}, "manifest.iniz");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"}, args::Options::HiddenFromDescription);
    parser.Parse();

    std::string root = GetCwd();
    if (argRootDir)
        root = argRootDir.Get();
    root = NormalizeSlashes(root);
    std::string patchManiPath = GetPath(argPatchMani.Get(), root);
    std::string baseManiPath = GetPath(argBaseMani.Get(), root);
    std::string outManiPath = GetPath(argOutMani.Get(), root);
    CreateDirectories(root);

    printf("Reading patch manifest %s and base manifest %s\n", patchManiPath.c_str(), baseManiPath.c_str());
    Manifest patchMani, baseMani;
    patchMani.ReadFromIni(ReadIniFile(patchManiPath.c_str()), root);
    if (PathAR::IsHttp(baseManiPath))
        baseManiPath = DownloadSimple(baseManiPath, root, "  ");
    baseMani.ReadFromIni(ReadIniFile(baseManiPath.c_str()), root);

    std::map<std::string, ManifestIter> fullnameToIter;
    for (int i = 0; i < patchMani.size(); i++) {
        const FileMetainfo &mf = patchMani[i];
        ZipSyncAssertF(mf.location != FileLocation::Nowhere, "Patch manifest must provide all files it decribes");
        std::string fullname = GetFullPath(mf.zipPath.rel, mf.filename);
        fullnameToIter[fullname] = ManifestIter(patchMani, &mf);
    }

    printf("Applying patch with %d files of size %0.3lf MB to base set with %d files of size %0.3lf MB\n", 
        TotalCount(patchMani), TotalCompressedSize(patchMani) * 1e-6,
        TotalCount(baseMani, false), TotalCompressedSize(baseMani, false) * 1e-6
    );

    std::vector<bool> usedPatchIds(patchMani.size(), false);
    Manifest outMani = patchMani;
    for (int i = 0; i < baseMani.size(); i++) {
        FileMetainfo mf = baseMani[i];
        std::string fullname = GetFullPath(mf.zipPath.rel, mf.filename);
        auto mit = fullnameToIter.find(fullname);
        if (mit != fullnameToIter.end()) {
            //overriden by patch
            ManifestIter iter = mit->second;
            usedPatchIds[iter._index] = true;
            printf("  Replaced %s: size %u -> %u  hash %s -> %s\n",
                fullname.c_str(), iter->props.contentsSize, mf.props.contentsSize,
                iter->contentsHash.Hex().c_str(), mf.contentsHash.Hex().c_str()
            );
        }
        else {
            //provided by base
            mf.DontProvide();
            outMani.AppendFile(mf);
        }
    }

    for (int i = 0; i < patchMani.size(); i++) {
        if (usedPatchIds[i])
            continue;
        const FileMetainfo &mf = patchMani[i];
        std::string fullname = GetFullPath(mf.zipPath.rel, mf.filename);
        printf("  Added %s: size %u  hash %s\n",
            fullname.c_str(), mf.props.contentsSize,
            mf.contentsHash.Hex().c_str()
        );
    }

    printf("Saving resulting manifest to %s\n", outManiPath.c_str());
    WriteIniFile(outManiPath.c_str(), outMani.WriteToIni());
}

void CommandReplace(args::Subparser &parser) {
    args::ValueFlag<std::string> argRootDir(parser, "root", "The root directory with the set of zips to modify", {'r', "root"});
    args::ValueFlag<std::string> argReplacements(parser, "replacements", "Path to ini file describing replacements", {"replacements"}, "replacements.ini", args::Options::Required);
    args::ValueFlag<std::string> argModifiedMani(parser, "mani", "Path to manifest describing the modified set of zips", {'m', "manifest"}, "manifest.iniz");
    args::ValueFlagList<std::string> argProvidedMani(parser, "provMani", "Path to additional provided manifests describing where to take files from", {'p', "provided"}, {});
    args::Flag argClean(parser, "clean", "Run \"clean\" command before and after update", {'c', "clean"});
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"}, args::Options::HiddenFromDescription);
    parser.Parse();

    std::string root = GetCwd();
    if (argRootDir)
        root = argRootDir.Get();
    root = NormalizeSlashes(root);
    std::string replacementsPath = GetPath(argReplacements.Get(), root);
    std::string modifiedManiPath = GetPath(argModifiedMani.Get(), root);
    if (argClean.Get())
        DoClean(root);

    std::vector<std::string> providManiPaths = CollectFilePaths(argProvidedMani.Get(), root);

    Manifest mainManifest;
    mainManifest.ReadFromIni(ReadIniFile(modifiedManiPath.c_str()), root);
    Manifest mainProvidedManifest, mainTargetOnlyManifest;
    for (int i = 0; i < mainManifest.size(); i++) {
        if (mainManifest[i].location == FileLocation::Nowhere)
            mainTargetOnlyManifest.AppendFile(mainManifest[i]);
        else
            mainProvidedManifest.AppendFile(mainManifest[i]);
    }

    printf("State of modified directory %s:\n", root.c_str());
    printf("  target has %d files of size %0.3lf MB\n",
        TotalCount(mainManifest, false), TotalCompressedSize(mainManifest, false) * 1e-6
    );
    printf("  provides %d files of size %0.3lf MB\n",
        TotalCount(mainProvidedManifest, false), TotalCompressedSize(mainProvidedManifest, false) * 1e-6
    );

    printf("Additional %d provided manifests:\n", (int)providManiPaths.size());
    Manifest providedManifest = mainProvidedManifest;
    for (std::string provManiPath : providManiPaths) {
        std::string srcDir = GetDirPath(provManiPath);
        std::string provManiLocalPath = provManiPath;
        if (PathAR::IsHttp(provManiPath))
            provManiLocalPath = DownloadSimple(provManiPath, root, "  ");
        Manifest mani;
        mani.ReadFromIni(ReadIniFile(provManiLocalPath.c_str()), srcDir);
        mani = mani.Filter([](const FileMetainfo &f) {
            return f.location != FileLocation::Nowhere;
        });
        printf("  %s contains %d files of size %0.3lf MB\n",
            provManiPath.c_str(), TotalCount(mani), TotalCompressedSize(mani) * 1e-6
        );
        providedManifest.AppendManifest(mani);
    }

    std::string replacementsLocalPath = replacementsPath;
    if (PathAR::IsHttp(replacementsLocalPath))
        replacementsLocalPath = DownloadSimple(replacementsPath, root, "");
    std::map<HashDigest, ManifestIter> replacementMap;
    {   //read replacements ini
        IniData ini = ReadIniFile(replacementsLocalPath.c_str());
        printf("Replacements file %s:\n", replacementsPath.c_str());
        for (const auto &pSV : ini) {
            const std::string &secName = pSV.first;
            if (!stdext::starts_with(secName, "replacement"))
                continue;
            for (const auto &pKV : pSV.second) {
                const std::string &srcStr = pKV.first;
                const std::string &dstStr = pKV.second;
                HashDigest srcHash, dstHash;
                srcHash.Parse(srcStr.c_str());
                dstHash.Parse(dstStr.c_str());
                ZipSyncAssertF(replacementMap.count(srcHash) == 0, "Duplicate replacement for hash %s", srcHash.Hex().c_str());
                ManifestIter dstIter;
                for (int i = 0; i < providedManifest.size(); i++) {
                    const FileMetainfo &pf = providedManifest[i];
                    if (pf.contentsHash == dstHash) {
                        ZipSyncAssertF(!dstIter, "Destination file with contents hash %s provided twice", dstHash.Hex().c_str());
                        dstIter = ManifestIter(providedManifest, i);
                    }
                }
                ZipSyncAssertF(dstIter, "Destination file with contents hash %s not provided", dstHash.Hex().c_str());
                printf("  %s -> %s\n    (size: %u / %u;  hashes: %s / %s)\n",
                    srcHash.Hex().c_str(), GetFullPath(dstIter->zipPath.abs, dstIter->filename).c_str(),
                    dstIter->props.contentsSize, dstIter->props.compressedSize,
                    dstIter->contentsHash.Hex().c_str(), dstIter->compressedHash.Hex().c_str()
                );
                replacementMap[srcHash] = dstIter;
            }
        }
    }

    //do replacements in the main manifest (both of its parts)
    for (int i = 0; i < mainProvidedManifest.size() + mainTargetOnlyManifest.size(); i++) {
        FileMetainfo &f = (i < mainProvidedManifest.size() ? mainProvidedManifest[i] : mainTargetOnlyManifest[i - mainProvidedManifest.size()]);
        auto iter = replacementMap.find(f.contentsHash);
        if (iter == replacementMap.end())
            continue;
        const FileMetainfo &replFile = *iter->second;
        f.DontProvide();
        f.contentsHash = replFile.contentsHash;
        f.compressedHash = replFile.compressedHash;
        f.props = replFile.props;
    }

    UpdateProcess update;
    update.Init(mainProvidedManifest, providedManifest, root);
    bool ok = update.DevelopPlan(UpdateType::SameCompressed);
    ZipSyncAssertF(ok, "DevelopPlan failed: provided manifests not enough");
    printf("Update plan developed\n");
    printf("Downloading missing files...\n");
    {
        ProgressIndicatorConsole progress;
        update.DownloadRemoteFiles([&progress](double ratio, const char *comment) -> int {
            return progress.Update(ratio, comment);
        });
        progress.Update(1.0, "All downloads complete");
    }
    printf("Repacking zips...\n");
    update.RepackZips();
    Manifest provMani = update.GetProvidedManifest();

    mainManifest = provMani.Filter([](const FileMetainfo &f) {
        return f.location == FileLocation::Inplace;
    });
    mainManifest.AppendManifest(mainTargetOnlyManifest);
    printf("Saving resulting manifest to %s\n", modifiedManiPath.c_str());
    WriteIniFile(modifiedManiPath.c_str(), mainManifest.WriteToIni());

    if (argClean.Get())
        DoClean(root);
}

void CommandUpdate(args::Subparser &parser) {
    args::ValueFlag<std::string> argRootDir(parser, "root", "The update should create/update the set of zips in this root directory\n"
        "(all relative paths are based from the root directory)", {'r', "root"});
    args::ValueFlag<std::string> argTargetMani(parser, "trgMani", "Path to the target manifest to update to", {'t', "target"}, "manifest.iniz", args::Options::Required);
    args::ValueFlagList<std::string> argProvidedMani(parser, "provMani", "Path to additional provided manifests describing where to take files from", {'p', "provided"}, {});
    args::Flag argClean(parser, "clean", "Run \"clean\" command before and after update", {'c', "clean"});
    args::PositionalList<std::string> argManagedZips(parser, "managed", "List of files or globs specifying which zips must be updated");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"}, args::Options::HiddenFromDescription);
    parser.Parse();

    std::string root = GetCwd();
    if (argRootDir)
        root = argRootDir.Get();
    root = NormalizeSlashes(root);
    std::string targetManiPath = GetPath(argTargetMani.Get(), root);
    CreateDirectories(root);
    if (argClean.Get())
        DoClean(root);

    std::vector<std::string> providManiPaths = CollectFilePaths(argProvidedMani.Get(), root);
    std::vector<std::string> managedZips = CollectFilePaths(argManagedZips.Get(), root);

    Manifest targetManifest;
    Manifest providedManifest;
    std::string targetManiLocalPath = targetManiPath;
    if (PathAR::IsHttp(targetManiPath))
        targetManiLocalPath = DownloadSimple(targetManiPath, root, "");
    targetManifest.ReadFromIni(ReadIniFile(targetManiLocalPath.c_str()), root);
    printf("Updating directory %s to target %s with %d files of size %0.3lf MB\n",
        root.c_str(), targetManiPath.c_str(), TotalCount(targetManifest, false), TotalCompressedSize(targetManifest, false) * 1e-6
    );
    printf("Provided manifests:\n");
    {
        std::string srcDir = GetDirPath(targetManiPath);
        Manifest mani = targetManifest.Filter([](const FileMetainfo &f) {
            return f.location != FileLocation::Nowhere;
        });
        mani.ReRoot(srcDir);
        printf("  %s containing %d files of size %0.3lf MB\n",
            targetManiPath.c_str(), TotalCount(mani), TotalCompressedSize(mani) * 1e-6
        );
        providedManifest.AppendManifest(mani);
    }
    for (std::string provManiPath : providManiPaths) {
        std::string srcDir = GetDirPath(provManiPath);
        std::string provManiLocalPath = provManiPath;
        if (PathAR::IsHttp(provManiPath))
            provManiLocalPath = DownloadSimple(provManiPath, root, "  ");
        Manifest mani;
        mani.ReadFromIni(ReadIniFile(provManiLocalPath.c_str()), srcDir);
        mani = mani.Filter([](const FileMetainfo &f) {
            return f.location != FileLocation::Nowhere;
        });
        printf("  %s containing %d files of size %0.3lf MB\n",
            provManiPath.c_str(), TotalCount(mani), TotalCompressedSize(mani) * 1e-6
        );
        providedManifest.AppendManifest(mani);
    }

    UpdateProcess update;
    update.Init(targetManifest, providedManifest, root);
    if (managedZips.size())
        printf("Managing %d zip files\n", (int)managedZips.size());
    for (int i = 0; i < managedZips.size(); i++)
        update.AddManagedZip(managedZips[i]);
    bool ok = update.DevelopPlan(UpdateType::SameCompressed);
    if (!ok) {
        int n = update.MatchCount();
        std::vector<ManifestIter> misses;
        for (int i = 0; i < n; i++) {
            const auto &m = update.GetMatch(i);
            if (!m.provided)
                misses.push_back(m.target);
        }
        std::shuffle(misses.begin(), misses.end(), std::mt19937(time(0)));
        n = misses.size();
        int k = std::min(n, 10);
        printf("Here are some of the missing files (%d out of %d):\n", k, n);
        for (int i = 0; i < k; i++) {
            printf("  %s||%s of size = %d/%d with hash = %s/%s\n",
                misses[i]->zipPath.rel.c_str(),
                misses[i]->filename.c_str(),
                misses[i]->props.compressedSize,
                misses[i]->props.contentsSize,
                misses[i]->compressedHash.Hex().c_str(),
                misses[i]->contentsHash.Hex().c_str()
            );
        }
        throw std::runtime_error("DevelopPlan failed: provided manifests not enough");
    }
    printf("Update plan developed\n");

    uint64_t bytesTotal = 0, bytesRemote = 0;
    int numTotal = 0, numRemote = 0;
    for (int i = 0; i < update.MatchCount(); i++) {
        const auto &m = update.GetMatch(i);
        uint32_t size = m.provided->byterange[1] - m.provided->byterange[0];
        if (m.provided->location == FileLocation::RemoteHttp) {
            numRemote++;
            bytesRemote += size;
        }
        numTotal++;
        bytesTotal += size;
    }
    printf("To be downloaded:\n");
    printf("  %d/%d files of size %0.0lf/%0.0lf MB (%0.2lf%%)\n", numRemote, numTotal, 1e-6 * bytesRemote, 1e-6 * bytesTotal, 100.0 * bytesRemote/bytesTotal);

    printf("Downloading missing files...\n");
    {
        ProgressIndicatorConsole progress;
        update.DownloadRemoteFiles([&progress](double ratio, const char *comment) -> int {
            return progress.Update(ratio, comment);
        });
        progress.Update(1.0, "All downloads complete");
    }
    printf("Repacking zips...\n");
    update.RepackZips();
    Manifest provMani = update.GetProvidedManifest();

    provMani = provMani.Filter([](const FileMetainfo &f) {
        return f.location == FileLocation::Inplace;
    });
    std::string resManiPath = GetPath("manifest.iniz", root);
    printf("Saving resulting manifest to %s\n", resManiPath.c_str());
    WriteIniFile(resManiPath.c_str(), provMani.WriteToIni());

    if (argClean.Get())
        DoClean(root);
}

void CommandHashzip(args::Subparser &parser) {
    args::Positional<std::string> argInputFile(parser, "input", "Name of file which should be put into checksummed zip", args::Options::Required);
    args::ValueFlag<std::string> argOutputFile(parser, "output", "Name of output zip file to be created (default: same as input with .zip appended)", {'o', "output"}, "");
    parser.Parse();

    std::string inPath = NormalizeSlashes(argInputFile.Get());
    std::string outPath = inPath + ".zip";
    if (argOutputFile)
        outPath = argOutputFile.Get();

    int pos = inPath.find_last_of('/');
    std::string filename = inPath.substr(pos + 1);

    auto data = ReadWholeFile(inPath);
    WriteChecksummedZip(outPath.c_str(), data.data(), data.size(), filename.c_str());
}

int main(int argc, char **argv) {
    args::ArgumentParser parser("ZipSync command line tool.");
    parser.helpParams.programName = "zipsync";
    parser.helpParams.width = 120;
    parser.helpParams.flagindent = 4;
    parser.helpParams.showTerminator = false;
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::Command clean(parser, "clean", "Delete temporary and intermediate files after repacking", CommandClean);
    args::Command normalize(parser, "normalize", "Normalize specified set of zips (on local machine)", CommandNormalize);
    args::Command analyze(parser, "analyze", "Create manifest for specified set of zips (on local machine)", CommandAnalyze);
    args::Command diff(parser, "diff", "Produce differential package, containing only those files from the set of zips which are not provided by given manifests", CommandDiff);
    args::Command patch(parser, "patch", "Create differential package from zipped set of file, which are to be added/overwriten on top of specified target", CommandPatch);
    args::Command update(parser, "update", "Update the set of zips to the specified target", CommandUpdate);
    args::Command hashzip(parser, "hashzip", "Put specified file into \"checksummed\" zip (with hash of its contents at the beginning of the file)", CommandHashzip);
    args::Command replace(parser, "replace", "Replace specified files in package, assuming it is also replaced in all dependency packages", CommandReplace);

    try {
        parser.ParseCLI(argc, argv);
    }
    catch (const args::Help&) {
        std::cout << parser;
        return 0;
    }
    catch (const args::Error& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (const std::exception &e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return 2;
    }
    return 0;
}
