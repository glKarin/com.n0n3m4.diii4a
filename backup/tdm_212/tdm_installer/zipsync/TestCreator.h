#pragma once

#include <stdint.h>
#include <string>
#include <algorithm>
#include <map>
#include <random>
#include "ZipSync.h"


namespace ZipSync {

struct InZipParams {
    int method;
    int level;
    uint32_t dosDate;
    uint16_t internalAttribs;   //warning: minizip usually changes this value when writing zip!
    uint32_t externalAttribs;
    bool operator==(const InZipParams &b) const;
};
struct InZipFile {
    InZipParams params;
    std::vector<uint8_t> contents;
};
typedef std::vector<std::pair<std::string, InZipFile>> InZipState;
typedef std::map<std::string, InZipState> DirState;


class TestCreator {
protected:
    std::mt19937 _rnd;
    UpdateType _updateType = UpdateType::SameCompressed;
    bool _remote = false;

public:
    void SetSeed(int seed);
    void SetUpdateType(UpdateType type);
    void SetRemote(bool remote);

    int RndInt(int low, int high);
    double RndDbl(double low, double high);
    template<class C> typename C::value_type & RandomFrom(C &cont);
    template<class C> const typename C::value_type & RandomFrom(const C &cont);

    std::vector<int> GenPartition(int sum, int cnt, int minV = 0);
    std::string GenExtension();
    std::string GenName();
    std::vector<std::string> GenPaths(int number, const char *extension = nullptr);
    std::vector<uint8_t> GenFileContents();

    InZipParams GenInZipParams();
    bool DoFilesMatch(const InZipFile &a, const InZipFile &b) const;

    DirState GenTargetState(int numFiles, int numZips);
    DirState GenMutatedState(const DirState &source);
    void TryAddFullZip(DirState &target, std::vector<DirState*> provided);
    void AddDuplicateFiles(DirState &target, std::vector<DirState*> provided);

    bool AddMissingFiles(const DirState &target, std::vector<DirState*> provided, bool leaveMisses = false);
    void SplitState(const DirState &full, std::vector<DirState> &parts);

    static bool ArePathsCaseAliased(std::string pathA, std::string pathB);
    bool CheckForCaseAliasing(const DirState &state1, const DirState &state2);

    static void WriteState(const std::string &localRoot, const std::string &remoteRoot, const DirState &state, Manifest *mani);
};

}
