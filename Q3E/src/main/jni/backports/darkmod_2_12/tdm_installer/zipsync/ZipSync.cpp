#include "ZipSync.h"
#include "StdString.h"
#include <algorithm>
#include <map>
#include <set>
#include "Logging.h"
#include "Utils.h"
#include "ZipUtils.h"
#include "Downloader.h"


namespace ZipSync {

void UpdateProcess::Init(const Manifest &targetMani_, const Manifest &providedMani_, const std::string &rootDir_) {
    _targetMani = targetMani_;
    _providedMani = providedMani_;
    _rootDir = rootDir_;

    _targetMani.ReRoot(_rootDir);

    _updateType = (UpdateType)0xDDDDDDDD;
    _matches.clear();

    //make sure every target zip is "managed"
    for (int i = 0; i < _targetMani.size(); i++) {
        _managedZips.insert(_targetMani[i].zipPath.abs);
    }
}

void UpdateProcess::AddManagedZip(const std::string &zipPath, bool relative) {
    PathAR path = relative ? PathAR::FromRel(zipPath, _rootDir) : PathAR::FromAbs(zipPath, _rootDir);
    auto pib = _managedZips.insert(path.abs);
}

bool UpdateProcess::DevelopPlan(UpdateType type) {
    _updateType = type;

    //build index of target files: by zip path + file path inside zip
    std::map<std::string, const FileMetainfo*> pathToTarget;
    for (int i = 0; i < _targetMani.size(); i++) {
        const FileMetainfo &tf = _targetMani[i];
        std::string fullPath = GetFullPath(tf.zipPath.abs, tf.filename);
        auto pib = pathToTarget.insert(std::make_pair(fullPath, &tf));
        ZipSyncAssertF(pib.second, "Duplicate target file at place %s", fullPath.c_str());
    }

    //find provided files which are already in-place
    for (int i = 0; i < _providedMani.size(); i++) {
        FileMetainfo &pf = _providedMani[i];
        if (pf.location != FileLocation::Local)
            continue;
        std::string fullPath = GetFullPath(pf.zipPath.abs, pf.filename);
        auto iter = pathToTarget.find(fullPath);
        if (iter != pathToTarget.end()) {
            //give this provided file priority when choosing where to take file from
            pf.location = FileLocation::Inplace;
        }
    }

    //build index of provided files (by hash on uncompressed file)
    std::map<HashDigest, std::vector<const FileMetainfo*>> pfIndex;
    for (int i = 0; i < _providedMani.size(); i++) {
        const FileMetainfo &pf = _providedMani[i];
        pfIndex[pf.contentsHash].push_back(&pf);
    }

    //find matching provided file for every target file
    _matches.clear();
    bool fullPlan = true;
    for (int i = 0; i < _targetMani.size(); i++) {
        const FileMetainfo &tf = _targetMani[i];

        const FileMetainfo *bestFile = nullptr;
        int bestScore = 1000000000;

        auto iter = pfIndex.find(tf.contentsHash);
        if (iter != pfIndex.end()) {
            const std::vector<const FileMetainfo*> &candidates = iter->second;

            for (const FileMetainfo *pf : candidates) {
                if (_updateType == UpdateType::SameCompressed && !(pf->compressedHash == tf.compressedHash))
                    continue;
                int score = int(pf->location) * 10 + 9;
                //more priority to same-file/same-range matches
                //this allows to avoid repacks on clean install
                if (pf->byterange[0] == tf.byterange[0])
                    score -= 2;
                if (pf->filename == tf.filename && pf->zipPath.rel == tf.zipPath.rel)
                    score -= 1;
                if (score < bestScore) {
                    bestScore = score;
                    bestFile = pf;
                }
            }
        }
        _matches.push_back(Match{ManifestIter(_targetMani, &tf), ManifestIter(_providedMani, bestFile)});
        if (!bestFile)
            fullPlan = false;
    }

    return fullPlan;
}

/**
 * Implementation class for UpdateProcess::RepackZips method.
 */
class UpdateProcess::Repacker {
public:
    UpdateProcess &_owner;

    struct ZipInfo {
        std::string _zipPath;
        bool _managed = false;

        std::vector<ManifestIter> _target;
        std::vector<ManifestIter> _provided;
        std::vector<int> _matchIds;

        std::string _zipPathRepacked;
        std::string _zipPathReduced;

        //number of provided files in this zip still needed in future
        int _usedCnt = 0;
        bool _repacked = false;
        bool _reduced = false;

        //for progress indicator
        uint64_t _totalTargetSize = 0;

        //cached during repacking to avoid reopening
        mutable UnzFileIndexed zfCache;

        UnzFileIndexed &UnzFileCached() const {
            if (!zfCache)
                zfCache.Open(_zipPath.c_str());
            return zfCache;
        }
        bool operator< (const ZipInfo &b) const {
            return _zipPath < b._zipPath;
        }
    };
    std::vector<ZipInfo> _zips;
    ZipInfo& FindZip(const std::string &zipPath) {
        for (auto &zip : _zips)
            if (zip._zipPath == zipPath)
                return zip;
        ZipSyncAssert(false);
    }

    //indexed as matches: false if provided file was copied in "raw" mode, true if in recompressing mode
    std::vector<bool> _recompressed;
    //how many (local) provided files have specified compressed hash
    //note: includes files from repacked and reduced zips
    std::map<HashDigest, int> _hashProvidedCnt;

    //the manifest containing provided files created by repacking process
    Manifest _repackedMani;
    //the manifest containing no-longer-needed files from target zips
    Manifest _reducedMani;

    //calling back to report current progress
    GlobalProgressCallback _progress;

    Repacker(UpdateProcess &owner) : _owner(owner) {}

    void CheckPreconditions() const {
        //verify that we are ready to do repacking
        ZipSyncAssertF(_owner._matches.size() == _owner._targetMani.size(), "RepackZips: DevelopPlan not called yet");
        for (Match m : _owner._matches) {
            std::string fullPath = GetFullPath(m.target->zipPath.abs, m.target->filename);
            ZipSyncAssertF(m.provided, "RepackZips: target file %s is not provided", fullPath.c_str());
            ZipSyncAssertF(m.provided->location == FileLocation::Inplace || m.provided->location == FileLocation::Local, "RepackZips: target file %s is not available locally", fullPath.c_str());
            ZipSyncAssert(_owner._managedZips.count(m.target->zipPath.abs));
        }
    }

    void ClassifyMatchesByTargetZip() {
        //create ZipInfo structure for every zip involved
        std::set<std::string> zipPaths = _owner._managedZips;
        for (int i = 0; i < _owner._providedMani.size(); i++) {
            const FileMetainfo &pf = _owner._providedMani[i];
            if (pf.location == FileLocation::Inplace || pf.location == FileLocation::Local) {
                _hashProvidedCnt[pf.compressedHash]++;
                zipPaths.insert(pf.zipPath.abs);
            }
        }
        for (const std::string &zp : zipPaths) {
            ZipInfo zip;
            zip._zipPath = zp;
            zip._zipPathRepacked = PrefixFile(zp, "__repacked__");
            zip._zipPathReduced = PrefixFile(zp, "__reduced__");
            _zips.push_back(std::move(zip));
        }

        //fill zips with initial info
        for (const std::string &zipPath : _owner._managedZips)
            FindZip(zipPath)._managed = true;
        for (int i = 0; i < _owner._targetMani.size(); i++) {
            const FileMetainfo &tf = _owner._targetMani[i];
            ZipInfo &zi = FindZip(tf.zipPath.abs);
            zi._target.push_back(ManifestIter(_owner._targetMani, i));
            zi._totalTargetSize += tf.props.compressedSize;
        }
        for (int i = 0; i < _owner._providedMani.size(); i++) {
            const FileMetainfo &pf = _owner._providedMani[i];
            if (pf.location == FileLocation::RemoteHttp)
                continue;
            FindZip(pf.zipPath.abs)._provided.push_back(ManifestIter(_owner._providedMani, i));
        }
        for (int i = 0; i < _owner._matches.size(); i++) {
            const Match &m = _owner._matches[i];
            FindZip(m.target->zipPath.abs)._matchIds.push_back(i);
            FindZip(m.provided->zipPath.abs)._usedCnt++;
        }
    }

    double ComputeProgressRatio() const {
        uint64_t totalBytes = 0;
        uint64_t doneBytes = 0;
        for (const ZipInfo &zi : _zips) {
            totalBytes += zi._totalTargetSize;
            if (zi._repacked)
                doneBytes += zi._totalTargetSize;
        }
        if (totalBytes == 0)
            return 0.0;
        return double(doneBytes) / totalBytes;
    }

    void ProcessZipsWithoutRepacking() {
        //two important use-cases when no repacking should be done:
        //  1. existing zip did not change and should not be updated
        //  2. clean install: downloaded zip should be renamed without repacking
        for (ZipInfo &dstZip : _zips) {
            if (dstZip._matchIds.empty())
                continue;       //nothing to put into this zip
            int k = dstZip._matchIds.size();

            //find source zip candidate
            const PathAR &srcZipPath = _owner._matches[dstZip._matchIds[0]].provided->zipPath;
            ZipInfo &srcZip = FindZip(srcZipPath.abs);
            if (srcZip._provided.size() != k)
                continue;       //number of files is different

            //check that "match" mapping maps into source zip and is surjective
            std::set<const FileMetainfo *> providedSet;
            for (int midx : dstZip._matchIds) {
                Match m = _owner._matches[midx];
                if (m.provided->zipPath.abs != srcZip._zipPath)
                    break;
                providedSet.insert(m.provided.get());
            }
            if (providedSet.size() != k)
                continue;       //some matches map outside (or not surjective)

            if (!srcZip._managed)
                continue;       //non-managed zip: cannot rename
            if (srcZip._usedCnt != k)
                continue;       //every file inside zip must be used exactly once

            { //check that filenames and header data are same (TODO: do it without scanning file)
                std::map<uint32_t, ManifestIter> bytestartToTarget;
                for (int midx : dstZip._matchIds) {
                    Match m = _owner._matches[midx];
                    bytestartToTarget[m.provided->byterange[0]] = m.target;
                }
                UnzFileHolder zf(srcZip._zipPath.c_str());
                bool allSame = true;
                for (int i = 0; i < k; i++) {
                    SAFE_CALL(i == 0 ? unzGoToFirstFile(zf) : unzGoToNextFile(zf));
                    FileMetainfo tf;
                    AnalyzeCurrentFile(zf, tf, false, false);
                    auto iter = bytestartToTarget.find(tf.byterange[0]);
                    if (iter == bytestartToTarget.end()) {
                        allSame = false;
                        break;
                    }
                    const FileMetainfo &want = *iter->second;
                    if (want.filename != tf.filename ||
                        want.props.lastModTime != tf.props.lastModTime ||
                        want.props.compressionMethod != tf.props.compressionMethod ||
                        want.props.generalPurposeBitFlag != tf.props.generalPurposeBitFlag ||
                        want.props.internalAttribs != tf.props.internalAttribs ||
                        want.props.externalAttribs != tf.props.externalAttribs ||
                        want.props.compressedSize != tf.props.compressedSize ||
                        want.props.contentsSize != tf.props.contentsSize ||
                        want.props.crc32 != tf.props.crc32
                    ) {
                        allSame = false;
                        break;
                    }
                }
                if (!allSame)
                    continue;   //filenames of metadata differ
            }

            //note: we can rename source zip into target zip directly
            //this would substitute both repacking and reducing
            g_logger->infof(lcRenameZipWithoutRepack, "Renaming %s to %s without repacking...", srcZip._zipPath.c_str(), dstZip._zipPathRepacked.c_str());

            //do the physical action
            CreateDirectoriesForFile(dstZip._zipPath, _owner._rootDir);
            RenameFile(srcZip._zipPath, dstZip._zipPathRepacked);

            //update all the data structures
            dstZip._repacked = true;
            srcZip._usedCnt = 0;
            srcZip._reduced = true;
            std::map<uint32_t, ManifestIter> filesMap;
            for (ManifestIter pf : srcZip._provided) {
                pf->location = FileLocation::Repacked;
                _repackedMani.AppendFile(*pf);
                filesMap[pf->byterange[0]] = ManifestIter(_repackedMani, _repackedMani.size() - 1);
            }
            for (int midx : dstZip._matchIds) {
                _recompressed.resize(midx + 1, false);
                _recompressed[midx] = false;
                ManifestIter &pf = _owner._matches[midx].provided;
                ManifestIter newIter = filesMap.at(pf->byterange[0]);
                pf->Nullify();
                pf = newIter;
            }

            if (_progress)
                _progress(ComputeProgressRatio(), formatMessage("Renamed %s to %s", srcZip._zipPath.c_str(), dstZip._zipPathRepacked.c_str()).c_str());
        }
    }

    void RepackZip(ZipInfo &zip) {
        g_logger->infof(lcRepackZip, "Repacking %s...", zip._zipPathRepacked.c_str());
        if (_progress)
            _progress(ComputeProgressRatio(), formatMessage("Repacking %s...", zip._zipPathRepacked.c_str()).c_str());

        //ensure all directories are created if missing
        CreateDirectoriesForFile(zip._zipPath, _owner._rootDir);
        //create new zip archive (it will contain results of repacking)
        ZipFileHolder zfOut(zip._zipPathRepacked.c_str());

        //copy all target files one-by-one
        for (int midx : zip._matchIds) {
            const Match &m = _owner._matches[midx];

            //find provided file
            UnzFileIndexed &zf = FindZip(m.provided->zipPath.abs).UnzFileCached();
            zf.LocateByByterange(m.provided->byterange[0], m.provided->byterange[1]);

            //can we avoid recompressing the file?
            unz_file_info info;
            SAFE_CALL(unzGetCurrentFileInfo(zf, &info, NULL, 0, NULL, 0, NULL, 0));
            bool copyRaw = false;
            if (m.provided->compressedHash == m.target->compressedHash)
                copyRaw = true;  //bitwise same
            if (_owner._updateType == UpdateType::SameContents && m.target->props.compressionMethod == info.compression_method && m.target->props.generalPurposeBitFlag == info.flag)
                copyRaw = true;  //same compression level

            //copy the file to the new zip
            minizipCopyFile(zf, zfOut,
                m.target->filename.c_str(),
                m.target->props.compressionMethod, m.target->props.generalPurposeBitFlag,
                m.target->props.internalAttribs, m.target->props.externalAttribs, m.target->props.lastModTime,
                copyRaw, m.target->props.crc32, m.target->props.contentsSize
            );
            //remember whether we repacked or not --- to be used in AnalyzeRepackedZip
            _recompressed.resize(midx+1, false);
            _recompressed[midx] = !copyRaw;
        }

        //flush and close new zip
        zfOut.reset();
        zip._repacked = true;

        if (_progress)
            _progress(ComputeProgressRatio(), formatMessage("Repacking %s...", zip._zipPathRepacked.c_str()).c_str());
    }

    void ValidateFile(const FileMetainfo &want, const FileMetainfo &have) const {
        std::string fullPath = GetFullPath(have.zipPath.abs, have.filename);
        //zipPath is different while repacking
        //package does not need to be checked
        ZipSyncAssertF(want.filename == have.filename, "Wrong filename of %s after repack: need %s", fullPath.c_str(), want.filename.c_str());
        ZipSyncAssertF(want.contentsHash == have.contentsHash, "Wrong contents hash of %s after repack", fullPath.c_str());
        ZipSyncAssertF(want.props.contentsSize == have.props.contentsSize, "Wrong contents size of %s after repack", fullPath.c_str());
        ZipSyncAssertF(want.props.crc32 == have.props.crc32, "Wrong crc32 of %s after repack", fullPath.c_str());
        if (_owner._updateType == UpdateType::SameCompressed) {
            ZipSyncAssertF(want.compressedHash == have.compressedHash, "Wrong compressed hash of %s after repack", fullPath.c_str());
            ZipSyncAssertF(want.props.compressedSize == have.props.compressedSize, "Wrong compressed size of %s after repack", fullPath.c_str());
        }
        ZipSyncAssertF(want.props.compressionMethod == have.props.compressionMethod, "Wrong compression method of %s after repack", fullPath.c_str());
        ZipSyncAssertF(want.props.generalPurposeBitFlag == have.props.generalPurposeBitFlag, "Wrong flags of %s after repack", fullPath.c_str());
        ZipSyncAssertF(want.props.lastModTime == have.props.lastModTime, "Wrong modification time of %s after repack", fullPath.c_str());
        ZipSyncAssertF(want.props.internalAttribs == have.props.internalAttribs, "Wrong internal attribs of %s after repack", fullPath.c_str());
        ZipSyncAssertF(want.props.externalAttribs == have.props.externalAttribs, "Wrong external attribs of %s after repack", fullPath.c_str());
    }

    void AnalyzeRepackedZip(const ZipInfo &zip) {
        //analyze the repacked new zip
        UnzFileHolder zf(zip._zipPathRepacked.c_str());
        SAFE_CALL(unzGoToFirstFile(zf));
        for (int i = 0; i < zip._matchIds.size(); i++) {
            int midx = zip._matchIds[i];
            Match &m = _owner._matches[midx];
            if (i > 0) SAFE_CALL(unzGoToNextFile(zf));

            //analyze current file
            bool needsRehashCompressed = _recompressed[midx];
            FileMetainfo metaNew;
            metaNew.zipPath = PathAR::FromAbs(zip._zipPathRepacked, _owner._rootDir);
            metaNew.location = FileLocation::Repacked;
            metaNew.package = m.target->package;
            metaNew.contentsHash = m.provided->contentsHash;
            metaNew.compressedHash = m.provided->compressedHash;   //will be recomputed if needsRehashCompressed
            AnalyzeCurrentFile(zf, metaNew, false, needsRehashCompressed);
            //check that it indeed matches the target
            ValidateFile(*m.target, metaNew);

            //decrement ref count on zip (which might allow to "reduce" it in ReduceOldZips)
            int &usedCnt = FindZip(m.provided->zipPath.abs)._usedCnt;
            ZipSyncAssert(usedCnt >= 0);
            usedCnt--;
            //increment ref count on compressed hash
            _hashProvidedCnt[metaNew.compressedHash]++;

            //add info about file to special manifest
            _repackedMani.AppendFile(metaNew);
            //switch the match for the target file to this new file
            m.provided = ManifestIter(_repackedMani, _repackedMani.size() - 1);
        }
        zf.reset();
    }

    void ReduceOldZips() {
        //see which target zip-s have contents no longer needed
        for (ZipInfo &zip : _zips) {
            if (!zip._managed)
                continue;       //no targets, not repacked, don't remove
            if (zip._reduced)
                continue;       //already reduced
            if (zip._usedCnt > 0)
                continue;       //original zip still needed as source
            zip.zfCache.Clear();

            if (IfFileExists(zip._zipPath)) {
                UnzFileHolder zf(zip._zipPath.c_str());
                ZipFileHolder zfOut(zip._zipPathReduced.c_str());

                //go over files and copy unique ones to reduced zip
                std::vector<FileMetainfo> copiedFiles;
                SAFE_CALL(unzGoToFirstFile(zf));
                while (1) {
                    //find current file in provided manifest
                    uint32_t range[2];
                    unzGetCurrentFilePosition(zf, &range[0], NULL, &range[1]);
                    ManifestIter found;
                    for (ManifestIter pf : zip._provided) {
                        if (pf->byterange[0] == range[0] && pf->byterange[1] == range[1]) {
                            ZipSyncAssertF(!found, "Provided manifest of %s has duplicate byteranges", zip._zipPath.c_str());
                            found = pf;
                        }
                    }
                    //check whether we should retain the file or remove it
                    unz_file_info info;
                    char filename[SIZE_PATH];
                    SAFE_CALL(unzGetCurrentFileInfo(zf, &info, filename, sizeof(filename), NULL, 0, NULL, 0));
                    if (found) {
                        int &usedCnt = _hashProvidedCnt.at(found->compressedHash);
                        if (usedCnt == 1) {
                            //not available otherwise -> repack
                            minizipCopyFile(zf, zfOut,
                                filename,
                                info.compression_method, info.flag,
                                info.internal_fa, info.external_fa, info.dosDate,
                                true, info.crc, info.uncompressed_size
                            );
                            copiedFiles.push_back(*found);
                        }
                        else {
                            //drop it: it will still be available
                            usedCnt--;
                        }
                    }
                    else {
                        //not present in provided manifests, but in managed zip -> drop it
                        //note that this file gets completely lost at this moment!
                        (void)0;    //nop
                    }
                    int res = unzGoToNextFile(zf);
                    if (res == UNZ_END_OF_LIST_OF_FILE)
                        break;
                    SAFE_CALL(res);
                }

                zf.reset();
                zfOut.reset();

                if (copiedFiles.empty()) {
                    //empty reduced zip -> remove it
                    RemoveFile(zip._zipPathReduced);
                }
                else {
                    //analyze reduced zip, add all files to manifest
                    UnzFileHolder zf(zip._zipPathReduced.c_str());
                    SAFE_CALL(unzGoToFirstFile(zf));
                    for (int i = 0; i < copiedFiles.size(); i++) {
                        FileMetainfo pf;
                        AnalyzeCurrentFile(zf, pf, false, false);
                        pf.package = copiedFiles[i].package;
                        pf.zipPath = PathAR::FromAbs(zip._zipPathReduced, _owner._rootDir);
                        pf.location = FileLocation::Reduced;
                        pf.contentsHash = copiedFiles[i].contentsHash;
                        pf.compressedHash = copiedFiles[i].compressedHash;
                        _reducedMani.AppendFile(pf);
                        if (i+1 < copiedFiles.size())
                            SAFE_CALL(unzGoToNextFile(zf));
                    }
                }

                //remove the old file
                RemoveFile(zip._zipPath);
                //nullify all provided files from the removed zip
                for (ManifestIter pf : zip._provided) {
                    pf->Nullify();
                }
            }

            zip._reduced = true;
        }
    }

    void RenameRepackedZips() {
        //see which target zip-s have contents no longer needed
        for (ZipInfo &zip : _zips) {
            if (!zip._repacked)
                continue;       //not repacked (yet?)
            if (!zip._reduced)
                continue;       //not reduced original yet

            if (zip._matchIds.empty()) {
                //we don't create empty zips (minizip support issue)
                RemoveFile(zip._zipPathRepacked);
            }
            else {
                ZipSyncAssertF(!IfFileExists(zip._zipPath), "Zip %s exists immediately before renaming repacked file", zip._zipPath.c_str());
                RenameFile(zip._zipPathRepacked, zip._zipPath);
            }
            //update provided files in repacked zip (all of them must be among matches by now)
            for (int midx : zip._matchIds) {
                FileMetainfo &pf = *_owner._matches[midx].provided;
                pf.zipPath = PathAR::FromAbs(zip._zipPath, _owner._rootDir);
                pf.location = FileLocation::Inplace;
            }
        }
    }

    void RewriteProvidedManifest() {
        Manifest newProvidedMani;

        for (int i = 0; i < _repackedMani.size(); i++)
            newProvidedMani.AppendFile(std::move(_repackedMani[i]));
        _repackedMani.Clear();
        for (int i = 0; i < _reducedMani.size(); i++)
            newProvidedMani.AppendFile(std::move(_reducedMani[i]));
        _reducedMani.Clear();
        for (int i = 0; i < _owner._providedMani.size(); i++) {
            FileMetainfo &pf = _owner._providedMani[i];
            if (pf.location == FileLocation::Nowhere)
                continue;       //was removed during zip-reduce
            newProvidedMani.AppendFile(std::move(pf));
        }
        _owner._providedMani.Clear();

        _owner._providedMani = std::move(newProvidedMani);
        //TODO: do we need matches afterwards?
        _owner._matches.clear();
    }

    void DoAll() {
        if (_progress)
            _progress(0.0, "Repacking started");

        CheckPreconditions();
        ClassifyMatchesByTargetZip();

        //prepare manifest for repacked files
        _repackedMani.Clear();
        _reducedMani.Clear();

        ProcessZipsWithoutRepacking();

        //iterate over all zips and repack them
        ReduceOldZips();
        for (ZipInfo &zip : _zips) {
            if (!zip._managed)
                continue;   //no targets, no need to remove
            if (zip._matchIds.empty())
                continue;   //minizip doesn't support empty zip
            if (zip._repacked)
                continue;   //renamed in ProcessZipsWithoutRepacking
            RepackZip(zip);
            AnalyzeRepackedZip(zip);
            ReduceOldZips();
        }
        for (ZipInfo &zip : _zips)
            zip.zfCache.Clear();

        RenameRepackedZips();
        RewriteProvidedManifest();

        if (_progress)
            _progress(1.0, "Repacking finished");
    }
};

void UpdateProcess::RepackZips(const GlobalProgressCallback &progressCallback) {
    Repacker impl(*this);
    impl._progress = progressCallback;
    impl.DoAll();
}


uint64_t UpdateProcess::DownloadRemoteFiles(
    const GlobalProgressCallback &progressDownloadCallback,
    const GlobalProgressCallback &progressPostprocessCallback,
    const char *useragent,
    bool blockMultipart
) {
    struct UrlData {
        PathAR path;
        StdioFileHolder file;
        int finishedCount = 0, totalCount = 0;
        std::map<uint32_t, int> baseToProvIdx;
        UrlData() : file(nullptr) {}
    };
    std::map<std::string, UrlData> urlStates;
    Downloader downloader;
    downloader.SetUserAgent(useragent);
    downloader.SetMultipartBlocked(blockMultipart);
    std::map<int, std::vector<int>> provIdxToMatchIds;

    std::set<std::string> downloadedFilenames;
    for (int midx = 0; midx < _matches.size(); midx++) {
        const Match &m = _matches[midx];
        if (m.provided->location != FileLocation::RemoteHttp)
            continue;
        const std::string &url = m.provided->zipPath.abs;

        int provIdx = m.provided._index;
        bool alreadyScheduled = (provIdxToMatchIds.count(provIdx) > 0);
        provIdxToMatchIds[provIdx].push_back(midx);
        if (alreadyScheduled)
            continue;   //same file wanted by many targets: download once

        PathAR &fn = urlStates[url].path;
        if (fn.abs.empty()) {
            for (int t = 0; t < 100; t++) {
                fn = PathAR::FromRel(PrefixFile(m.provided->zipPath.rel, "__download" + std::to_string(t) + "__"), _rootDir);
                if (downloadedFilenames.count(fn.abs))
                    continue;
                if (!IfFileExists(fn.abs))
                    break;
            }
            ZipSyncAssertF(!fn.abs.empty(), "too many \"__download??__%s\" files", m.provided->zipPath.rel.c_str());
            downloadedFilenames.insert(fn.abs);
        }
        urlStates[url].totalCount++;

        DownloadSource src;
        src.url = url;
        src.byterange[0] = m.provided->byterange[0];
        src.byterange[1] = m.provided->byterange[1];
        downloader.EnqueueDownload(src, [this,&urlStates,url,provIdx](const void *data, uint32_t bytes) {
            UrlData &state = urlStates[url];
            if (!state.file) {
                CreateDirectoriesForFile(state.path.abs, _rootDir);
                state.file = StdioFileHolder(state.path.abs.c_str(), "wb");
            }

            size_t base = ftell(state.file);
            state.baseToProvIdx[base] = provIdx;
            size_t written = fwrite(data, 1, bytes, state.file);
            ZipSyncAssert(written == bytes);

            if (++state.finishedCount == state.totalCount)
                state.file.reset();
        });
    }

    downloader.SetProgressCallback([this,&progressDownloadCallback](double ratio, const char *message) -> int {
        if (progressDownloadCallback)
            return progressDownloadCallback(ratio, message);
        return 0;
    });
    downloader.DownloadAll();

    double totalBytesToPostprocess = 1e-20;
    for (const auto &pKV : urlStates)
        totalBytesToPostprocess += GetFileSize(pKV.second.path.abs);
    double bytesPostprocessed = 0.0;
    if (progressPostprocessCallback)
        progressPostprocessCallback(0.0, "Verifying started");

    for (const auto &pKV : urlStates) {
        const std::string &url = pKV.first;
        const UrlData &state = pKV.second;
        double rawZipSize = GetFileSize(state.path.abs);

        if (progressPostprocessCallback) {
            int code = progressPostprocessCallback(bytesPostprocessed / totalBytesToPostprocess, formatMessage("Verifying \"%s\"...", state.path.rel.c_str()).c_str());
            if (code != 0)
                g_logger->errorf(lcUserInterrupt, "Interrupted by user");
        }

        std::vector<FileAttribInfo> fileAttribs;
        for (const auto &pOI : state.baseToProvIdx) {
            ManifestIter provided(_providedMani, pOI.second);
            fileAttribs.push_back(FileAttribInfo{pOI.first, provided->props.externalAttribs, provided->props.internalAttribs});
        }

        minizipAddCentralDirectory(state.path.abs.c_str(), fileAttribs);
        UnzFileIndexed zf;
        zf.Open(state.path.abs.c_str());

        for (const auto &pOI : state.baseToProvIdx) {
            uint32_t offset = pOI.first;
            int provIdx = pOI.second;
            std::vector<int> matchIds = provIdxToMatchIds[provIdx];
            ManifestIter provided(_providedMani, provIdx);

            //verify hash of the downloaded file (we must be sure that it is correct)
            //TODO: what if bad mirror changes file local header?...
            uint32_t size = provided->byterange[1] - provided->byterange[0];
            zf.LocateByByterange(offset, offset + size);
            SAFE_CALL(unzOpenCurrentFile2(zf, NULL, NULL, true));
            Hasher hasher;
            char buffer[SIZE_FILEBUFFER];
            int processedBytes = 0;
            while (1) {
                int bytes = unzReadCurrentFile(zf, buffer, sizeof(buffer));
                if (bytes < 0)
                    SAFE_CALL(bytes);
                if (bytes == 0)
                    break;
                hasher.Update(buffer, bytes);
                processedBytes += bytes;
            } 
            HashDigest obtainedHash = hasher.Finalize();
            SAFE_CALL(unzCloseCurrentFile(zf));

            const HashDigest &expectedHash = provided->compressedHash;
            std::string fullPath = GetFullPath(url, provided->filename);
            ZipSyncAssertF(obtainedHash == expectedHash, "Hash of \"%s\" after download is %s instead of %s", fullPath.c_str(), obtainedHash.Hex().c_str(), expectedHash.Hex().c_str());

            FileMetainfo pf = *provided;
            pf.zipPath = state.path;
            pf.byterange[0] = offset;
            pf.byterange[1] = offset + size;
            pf.location = FileLocation::Local;

            int pi = _providedMani.size();
            _providedMani.AppendFile(pf);
            for (int midx : matchIds)
                _matches[midx].provided = ManifestIter(_providedMani, pi);
        }

        //mark downloaded file as "managed", meaning that repacking can remove it if it likes
        //this also enables fast path: rename the zip without any repacking
        //note that we have to ensure that zip file is perfectly good in its current state!
        //otherwise user will get bad zip, and no repacking would happen to fix it...
        AddManagedZip(state.path.abs);

        bytesPostprocessed += rawZipSize;
    }

    if (progressPostprocessCallback)
        progressPostprocessCallback(1.0, "Verifying finished");

    return downloader.TotalBytesDownloaded();
}

}
