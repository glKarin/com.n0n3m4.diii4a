#include "Fuzzer.h"
#include "TestCreator.h"
#include <functional>
#include "HttpServer.h"
#include "StdString.h"
#include "StdFilesystem.h"


namespace ZipSync {

class Fuzzer : private TestCreator {
    struct SourcePath {
        std::string localDir;
        std::string urlDir;     //same as localDir if used locally
    };
    //directories where everything happens
    std::string _baseDir;
    std::string _rootTargetDir;
    std::string _rootInplaceDir;
    std::vector<SourcePath> _rootSources;       //includes local and remote directories

    //directory contents: generated randomly
    DirState _initialTargetState;
    DirState _initialInplaceState;
    DirState _initialAllSourcesState;
    std::vector<DirState> _initialSourceState;
    bool _shouldUpdateSucceed;

    //initial manifests passed to the updater
    Manifest _initialTargetMani;
    Manifest _initialProvidedMani;

    //http servers (serving content for remote directories)
    bool _remoteEnabled = false;
    std::vector<std::unique_ptr<HttpServer>> _httpServers;

    //the update class
    std::unique_ptr<UpdateProcess> _updater;

    //various counters (increased every run)
    int _numCasesGenerated = 0;         //generated
    int _numCasesValidated = 0;         //passed to updater (valid)
    int _numCasesShouldSucceed = 0;     //should succeed according to prior knowledge
    int _numCasesActualSucceed = 0;     //actually updated successfully

    //various manifests after update
    Manifest _finalComputedProvidedMani;    //computed by UpdateProcess during update
    Manifest _finalActualTargetMani;        //real contents of "inplace" dir (without "reduced" zips)
    Manifest _finalActualProvidedMani;      //real contents of "inplace" dir (includes "reduced" zips)

    void AssertManifestsSame(IniData &&iniDataA, std::string dumpFnA, IniData &&iniDataB, std::string dumpFnB, bool ignoreCompressedHash = false, bool ignoreByterange = false) const {
        auto ClearCompressedHash = [&](IniData &ini) {
            for (auto& pSect : ini) {
                for (auto &pProp : pSect.second) {
                    if (ignoreCompressedHash) {
                        if (pProp.first == "compressedHash" || pProp.first == "compressedSize")
                            pProp.second = "(removed)";
                    }
                    if (ignoreByterange) {
                        if (pProp.first == "byterange")
                            pProp.second = "(removed)";
                    }
                    if (pProp.first == "package")
                        pProp.second = "(removed)";
                }
            }
        };
        ClearCompressedHash(iniDataA);
        ClearCompressedHash(iniDataB);
        if (iniDataA != iniDataB) {
            WriteIniFile((_baseDir + "/" + dumpFnA).c_str(), iniDataA);
            WriteIniFile((_baseDir + "/" + dumpFnB).c_str(), iniDataB);
        }
        ZipSyncAssert(iniDataA == iniDataB);
    }

public:
    void SetRemoteEnabled(bool enabled) {
        _remoteEnabled = enabled;
        while (_remoteEnabled && _httpServers.size() < 3) {
            int idx = _httpServers.size();
            _httpServers.emplace_back(new HttpServer());
            _httpServers[idx]->SetPortNumber(HttpServer::PORT_DEFAULT + idx);
            _httpServers[idx]->SetBlockSize(30 + idx * 7);   //for more extensive testing 
            _httpServers[idx]->Start();
        }
        while (!_remoteEnabled && _httpServers.size() > 0)
            _httpServers.pop_back();
    }

    void GenerateInput(std::string baseDir, int seed) {
        _baseDir = baseDir;
        SetSeed(seed);

        _rootTargetDir = _baseDir + "/target";
        _rootInplaceDir = _baseDir + "/inplace";
        _rootSources.clear();
        _rootSources.push_back(SourcePath{_baseDir + "/local", _baseDir + "/local"});
        if (_remoteEnabled) {
            int k = RndInt(0, 2);
            for (int i = 0; i < k; i++) {
                SourcePath sp = {_baseDir + "/remote" + std::to_string(i), _httpServers[i]->GetRootUrl()};
                _rootSources.push_back(sp);
                _httpServers[i]->SetRootDir(sp.localDir);
            }
        }

        SetUpdateType(seed % 2 ? UpdateType::SameCompressed : UpdateType::SameContents);
        SetRemote(_remoteEnabled);

        _initialTargetState = GenTargetState(50, 10);
        _initialInplaceState = GenMutatedState(_initialTargetState);
        _initialAllSourcesState = GenMutatedState(_initialTargetState);
        _initialSourceState.assign(_rootSources.size(), {});
        SplitState(_initialAllSourcesState, _initialSourceState);
        std::vector<DirState*> provStates = {&_initialInplaceState};
        for (auto &s : _initialSourceState) provStates.push_back(&s);
        AddDuplicateFiles(_initialTargetState, provStates);
        TryAddFullZip(_initialTargetState, provStates);
        _shouldUpdateSucceed = AddMissingFiles(_initialTargetState, provStates, true);

        _numCasesGenerated++;
    }

    bool ValidateInput() {
        std::vector<DirState*> states;
        states.push_back(&_initialTargetState);
        states.push_back(&_initialInplaceState);
        for (auto &state : _initialSourceState) states.push_back(&state);

        for (int i = 0; i < states.size(); i++)
            for (int j = 0; j <= i; j++)
                if (CheckForCaseAliasing(*states[i], *states[j]))
                    return false;   //cannot ensure proper testing in case of any case collision 

        for (const auto &pdir : _initialTargetState) {
            const InZipState &zst = pdir.second;
            std::set<std::string> fileset;
            for (const auto &kv : zst) {
                const std::string &name = kv.first;
                if (!fileset.insert(name).second)
                    return false;   //same file paths inside target zip not allowed
            }
        }

        _numCasesValidated++;
        return true;
    }

    void WriteInput() {
        _initialTargetMani.Clear();
        _initialProvidedMani.Clear();
        WriteState(_rootTargetDir, _rootTargetDir, _initialTargetState, &_initialTargetMani);
        WriteState(_rootInplaceDir, _rootInplaceDir, _initialInplaceState, &_initialProvidedMani);
        for (int i = 0; i < _rootSources.size(); i++)
            WriteState(_rootSources[i].localDir, _rootSources[i].urlDir, _initialSourceState[i], &_initialProvidedMani);
    }

    bool DoUpdate() {
        _updater.reset(new UpdateProcess());

        _updater->Init(_initialTargetMani, _initialProvidedMani, _rootInplaceDir);
        for (const auto &zipPair : _initialInplaceState)
            _updater->AddManagedZip(_rootInplaceDir + "/" + zipPair.first);

        bool success = _updater->DevelopPlan(_updateType);
        _numCasesShouldSucceed += _shouldUpdateSucceed;
        _numCasesActualSucceed += success;

        if (!success) {
            ZipSyncAssert(!_shouldUpdateSucceed);
            return false;
        }
        //sometimes same file with different compression level is packed into same compressed data
        //hence our "should succeed" detection is not perfect, so we allow a bit of errors of one type
        double moreSuccessRatio = double(_numCasesActualSucceed - _numCasesShouldSucceed) / std::max(_numCasesValidated, 200);
        ZipSyncAssert(moreSuccessRatio <= 0.05);    //actually, this ratio is smaller than 1%
        
        if (_remoteEnabled)
            _updater->DownloadRemoteFiles();

        _updater->RepackZips();

        return true;
    }

    void CheckOutput() {
        _finalComputedProvidedMani = _updater->GetProvidedManifest();

        //analyze the actual state of inplace/update directory
        auto resultPaths = stdext::recursive_directory_enumerate(_rootInplaceDir);
        _finalActualTargetMani.Clear();
        _finalActualProvidedMani.Clear();
        Manifest actualProvidedMani;
        for (stdext::path filePath : resultPaths) {
            if (!stdext::is_regular_file(filePath))
                continue;
            if (!stdext::starts_with(filePath.filename().string(), "__reduced__") && !stdext::starts_with(filePath.filename().string(), "__download")) {
                _finalActualTargetMani.AppendLocalZip(filePath.string(), _rootInplaceDir, "default");
            }
            _finalActualProvidedMani.AppendLocalZip(filePath.string(), _rootInplaceDir, "default");
        }

        //check: the actual state exactly matches what we wanted to obtain (compressed hash may differ depending on options)
        AssertManifestsSame(
            _initialTargetMani.WriteToIni(), "target_expected.ini",
            _finalActualTargetMani.WriteToIni(), "target_obtained.ini",
            _updateType == UpdateType::SameContents,    //compressed data may not match unless requested explicitly
            _updateType == UpdateType::SameContents     //byterange may not match if recompression happened
        );

        //check: the provided manifest computed by updater exactly represents the current state
        Manifest inplaceComputedProvidedMani = _finalComputedProvidedMani.Filter([&](const FileMetainfo &f) {
            return f.zipPath.GetRootDir() == _rootInplaceDir;
        });
        AssertManifestsSame(
            inplaceComputedProvidedMani.WriteToIni(), "provided_computed.ini",
            _finalActualProvidedMani.WriteToIni(), "provided_actual.ini"
            //note: both compressed hashes and byteranges must be exactly equal!
        );

        //check: all files previously available are still available, if we take reduced zips into account
        for (int i = 0; i < _initialProvidedMani.size(); i++) {
            const FileMetainfo &oldFile = _initialProvidedMani[i];
            bool available = false;
            for (int j = 0; j < _finalComputedProvidedMani.size() && !available; j++) {
                const FileMetainfo &newFile = _finalComputedProvidedMani[j];
                if (oldFile.compressedHash == newFile.compressedHash)
                    available = true;
            }
            ZipSyncAssertF(available, "File %s with hash %s is no longer available", GetFullPath(oldFile.zipPath.abs, oldFile.filename).c_str(), oldFile.compressedHash.Hex().c_str());
        }
    }
};

//==========================================================================================

void Fuzz(std::string where, int casesNum, bool enableRemote) {
    static const int SPECIAL_SEEDS[] = {0};
    int SK = sizeof(SPECIAL_SEEDS) / sizeof(SPECIAL_SEEDS[0]);
    if (casesNum < 0)
        casesNum = 1000000000;  //infinite
    Fuzzer fuzz;
    fuzz.SetRemoteEnabled(enableRemote);
    for (int attempt = -SK; attempt < casesNum; attempt++) {
        int seed = (attempt < 0 ? SPECIAL_SEEDS[attempt + SK] : attempt);
        bool duplicate = false;
        for (int i = 0; i < SK; i++)
            if (SPECIAL_SEEDS[i] == seed)
                duplicate = true;
        if (attempt >= 0 && duplicate)
            continue;

        fuzz.GenerateInput(where + "/" + std::to_string(seed), seed);
        if (!fuzz.ValidateInput())
            continue;
        fuzz.WriteInput();
        if (fuzz.DoUpdate())
            fuzz.CheckOutput();
    }
}

}
