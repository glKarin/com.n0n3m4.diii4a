#pragma once

#include "Manifest.h"
#include <set>
#include <functional>


namespace ZipSync {

class LocalCache;

//called to report progress: returning nonzero value interrupts processing
typedef std::function<int(double, const char*)> GlobalProgressCallback;

enum class UpdateType {
    SameContents,       //uncompressed contents of every file must match (and compression settings too)
    SameCompressed,     //compressed contents and local file header must be bitwise the same
};

/**
 * Represents the whole updating process.
 */
class UpdateProcess {
public:
    struct Match {
        //target file (surely not NULL)
        ManifestIter target;
        //provided file which will fulfill it (NULL if no match found)
        ManifestIter provided;
    };

private:
    //the target manifest being the goal of this update
    Manifest _targetMani;
    //the provided manifest showing the current state of installation
    //note: it is changed during the update process
    Manifest _providedMani;
    //the root directory of installation being updated
    //all target zip paths are treated relative to it
    std::string _rootDir;
    //which type of "sameness" we want to achieve
    UpdateType _updateType;
    //set of local zips which should be removed/replaced by update
    //note: every zip with a target file gets managed automatically
    std::set<std::string> _managedZips;

    //the best matching provided file for every target file
    std::vector<Match> _matches;

    class Repacker;
    friend class Repacker;

public:
    //must be called prior to any usage of an instance
    void Init(const Manifest &targetMani, const Manifest &providedMani, const std::string &rootDir);

    //mark this zip as being owned by updater
    //it means that updater must delete it if it's not mentioned on target manifest
    void AddManagedZip(const std::string &zipPath, bool relative = false);

    //decide how to execute the update (which files to find where)
    bool DevelopPlan(UpdateType type);

    //download all remote files which are necessary for update
    //returns total number of bytes downloaded
    uint64_t DownloadRemoteFiles(
        const GlobalProgressCallback &progressDownloadCallback = GlobalProgressCallback(),
        const GlobalProgressCallback &progressPostprocessCallback = GlobalProgressCallback(),
        const char *useragent = nullptr,
        bool blockMultipart = false
    );

    //having all matches available locally, perform the update
    void RepackZips(const GlobalProgressCallback &progressCallback = GlobalProgressCallback());

    //TODO: local cache for reduced zips?
    void RemoveOldZips(const LocalCache *cache);


    const Manifest &GetProvidedManifest() const { return _providedMani; }

    int MatchCount() const { return _matches.size(); }
    const Match &GetMatch(int idx) const { return _matches[idx]; }
};

}
