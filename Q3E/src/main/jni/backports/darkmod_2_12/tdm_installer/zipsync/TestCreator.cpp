#include "TestCreator.h"
#include "StdString.h"
#include "StdFilesystem.h"
#include <string.h>
#include "minizip_extra.h"


namespace ZipSync {

bool InZipParams::operator==(const InZipParams &b) const {
    return (
        std::tie(method, level, dosDate, internalAttribs, externalAttribs) == 
        std::tie(b.method, b.level, b.dosDate, b.internalAttribs, b.externalAttribs)
    );
}

void TestCreator::SetSeed(int seed) {
    _rnd.seed(seed);
}
void TestCreator::SetUpdateType(UpdateType type) {
    _updateType = type;
}
void TestCreator::SetRemote(bool remote) {
    _remote = remote;
}

int TestCreator::RndInt(int low, int high) {
    return std::uniform_int_distribution<int>(low, high)(_rnd);
}
double TestCreator::RndDbl(double low, double high) {
    return std::uniform_real_distribution<double>(low, high)(_rnd);
}

template<class C> typename C::value_type & TestCreator::RandomFrom(C &cont) {
    int idx = RndInt(0, cont.size() - 1);
    auto iter = cont.begin();
    std::advance(iter, idx);
    return *iter;
}
template<class C> const typename C::value_type & TestCreator::RandomFrom(const C &cont) {
    return RandomFrom(const_cast<C&>(cont));
}

std::vector<int> TestCreator::GenPartition(int sum, int cnt, int minV) {
    sum -= cnt * minV;
    ZipSyncAssert(sum >= 0);
    std::vector<int> res;
    for (int i = 0; i < cnt; i++) {
        double avg = double(sum) / (cnt - i);
        int val = RndInt(0, int(2*avg));
        val = std::min(val, sum);
        if (i+1 == cnt)
            val = sum;
        res.push_back(minV + val);
        sum -= val;
    }
    std::shuffle(res.begin(), res.end(), _rnd);
    return res;
}

std::string TestCreator::GenExtension() {
    static const std::vector<std::string> extensions = {
        ".txt", ".bin", ".dat", ".jpg", ".png", ".mp4", ".md5mesh", ".lwo",
        ".exe", ".ini", ".zip", ".pk4"
    };
    return RandomFrom(extensions);
}

std::string TestCreator::GenName() {
    std::string res;
    int len = RndInt(0, 1) ? RndInt(3, 10) : RndInt(1, 3);
    for (int i = 0; i < len; i++) {
        int t = RndInt(0, 3);
        char ch = 0;
        if (t == 0) ch = RndInt('0', '9');
        if (t == 1) ch = RndInt('a', 'z');
        if (t == 2) ch = RndInt('A', 'Z');
        if (t == 3) ch = (RndInt(0, 1) ? ' ' : '_');
        //since recently, spaces in URLs don't work with libcurl
        //it probably started in 1.62.0:
        //  https://github.com/curl/curl/issues/3340
        if (_remote && ch == ' ') ch = '_';
        res += ch;
    }
    //trailing spaces don't work well in Windows
    if (res.front() == ' ') res.front() = '_';
    if (res.back() == ' ') res.back() = '_';
    //avoid Windows reserved names: CON, PRN, AUX, NUL, COM1, ...
    if ((res.size() == 3 || res.size() == 4) && isalpha(res[0]) && isalpha(res[1]) && isalpha(res[2]))
        res[0] = '_';
    return res;
}

std::vector<std::string> TestCreator::GenPaths(int number, const char *extension) {
    std::vector<std::string> res;
    std::set<std::string> used;
    while (res.size() < number) {
        std::string path;
        if (res.empty() || RndInt(0, 99) < 20) {
            int depth = RndInt(0, 2);
            for (int i = 0; i < depth; i++)
                path += GenName() + "/";
            path += GenName() + (extension ? extension : GenExtension());
        }
        else {
            std::string base = RandomFrom(res);
            std::vector<std::string> terms;
            stdext::split(terms, base, "/");
            int common = RndInt(0, terms.size()-1);
            terms.resize(common);
            int want = RndInt(0, 2);
            while (terms.size() < want)
                terms.push_back(GenName());
            path = stdext::join(terms, "/");
            if (!path.empty()) path += "/";
            path += GenName() + (extension ? extension : GenExtension());
        }
        if (used.insert(path).second)
            res.push_back(path);
    }
    std::shuffle(res.begin(), res.end(), _rnd);
    return res;
}

std::vector<uint8_t> TestCreator::GenFileContents() {
    int pwr = RndInt(0, 10);
    int size = RndInt((1<<pwr)-1, 2<<pwr);
    std::vector<uint8_t> res;
    int t = RndInt(0, 3);
    if (t == 0) {
        for (int i = 0; i < size; i++)
            res.push_back(RndInt(0, 255));
    }
    else if (t == 1) {
        for (int i = 0; i < size/4; i++) {
            int pwr = RndInt(0, 30);
            int value = RndInt((1<<pwr)-1, (2<<pwr)-1);
            for (int j = 0; j < 4; j++) {
                res.push_back(value & 0xFF);
                value >>= 8;
            }
        }
    }
    else if (t == 2) {
        std::string text;
        while (text.size() < size) {
            char buff[128];
            double x = RndDbl(-100.0, 100.0);
            double y = RndDbl(-10.0, 30.0);
            double z = RndDbl(0.0, 1.0);
            sprintf(buff, "%0.3lf %0.6lf %0.10lf\n", x, y, z);
            text += buff;
        }
        res.resize(text.size());
        memcpy(res.data(), text.data(), text.size());
    }
    else if (t == 3) {
        std::string source = R"(
Sample: top 60,000 lemmas and ~100,000 word forms (both sets included for the same price) 	Top 20,000 or 60,000 lemmas: simple word list, frequency by genre, or as an eBook. 	Top 100,000 word forms. Also contains information on COCA genres, and frequency in the BNC (British), SOAP (informal) and COHA (historical)
  	
rank 	  lemma / word 	PoS 	freq 	range 	range10
7371 	  brew 	v 	94904 	0.06 	0.01
17331 	  useable 	j 	17790 	0.02 	0.00
27381 	  uppercase 	n 	5959 	0.02 	0.00
37281 	  half-naked 	j 	2459 	0.00 	0.00
47381 	  bellhop 	n 	1106 	0.00 	0.00
57351 	  tetherball 	n 	425 	0.00 	0.00
	
rank 	  lemma / word 	PoS 	freq 	dispersion
7309 	  attic 	n 	2711 	0.91
17311 	  tearful 	j 	542 	0.93
27303 	  tailgate 	v 	198 	0.85
37310 	  hydraulically 	r 	78 	0.83
47309 	  unsparing 	j 	35 	0.83
57309 	  embryogenesis 	n 	22 	0.66
        )";
        std::string text;
        while (text.size() < size) {
            int l = RndInt(0, source.size());
            int r = RndInt(0, source.size());
            if (r < l) std::swap(l, r);
            int rem = size - text.size();
            r = std::min(r, l + rem);
            text += source.substr(l, r-l);
        }
        res.resize(text.size());
        memcpy(res.data(), text.data(), text.size());
    }
    return res;
}

InZipParams TestCreator::GenInZipParams() {
    InZipParams res;
    res.method = (RndInt(0, 2) > 0 ? 8 : 0);
    res.level = res.method ? RndInt(Z_BEST_SPEED, Z_BEST_COMPRESSION) : 0;
    res.dosDate = RndInt(INT_MIN, INT_MAX);
    res.internalAttribs = RndInt(INT_MIN, INT_MAX);
    res.externalAttribs = RndInt(INT_MIN, INT_MAX);
    res.externalAttribs &= ~0xFF;   //zero 0-th byte: it indicates directories
    return res;
}

bool TestCreator::DoFilesMatch(const InZipFile &a, const InZipFile &b) const {
    if (a.contents != b.contents)
        return false;
    if (_updateType == UpdateType::SameCompressed && !(a.params.method == b.params.method && a.params.level == b.params.level))
        return false;
    return true;
}

DirState TestCreator::GenTargetState(int numFiles, int numZips) {
    std::vector<std::string> zipPaths = GenPaths(numZips, ".zip");
    std::vector<int> fileCounts = GenPartition(numFiles, numZips);
    DirState state;
    for (int i = 0; i < numZips; i++) {
        int k = fileCounts[i];
        InZipState inzip;
        std::vector<std::string> filePaths = GenPaths(k);
        for (int j = 0; j < k; j++) {
            auto params = GenInZipParams();
            auto contents = GenFileContents();
            inzip.emplace_back(filePaths[j], InZipFile{std::move(params), std::move(contents)});
        }
        state[zipPaths[i]] = std::move(inzip);
    }
    return state;
}

DirState TestCreator::GenMutatedState(const DirState &source) {
    DirState state;

    std::vector<std::string> appendableZips;

    int sameZips = RndInt(0, source.size() * 2/3);
    for (int i = 0; i < sameZips; i++) {
        bool samePath = (RndInt(0, 99) < 75);
        bool appendable = (RndInt(0, 99) < 50);
        bool incomplete = (RndInt(0, 99) < 30);
        const auto &pPZ = RandomFrom(source);
        std::string filename = (samePath ? pPZ.first : GenPaths(1)[0]);
        InZipState inzip = pPZ.second;
        if (incomplete) {
            int removeCnt = RndInt(0, inzip.size()/2);
            for (int j = 0; j < removeCnt; j++)
                inzip.erase(inzip.begin() + RndInt(0, inzip.size()-1));
        }
        if (appendable)
            appendableZips.push_back(filename);
        state[filename] = std::move(inzip);
    }

    std::vector<InZipState::const_iterator> sourceFiles;
    std::vector<std::string> candidatePaths;
    for (const auto &zipPair : source) {
        const auto &files = zipPair.second;
        for (auto iter = files.cbegin(); iter != files.cend(); iter++) {
            sourceFiles.push_back(iter);
            candidatePaths.push_back(iter->first);
        }
    }
    {
        auto np = GenPaths(candidatePaths.size() + 1);
        candidatePaths.insert(candidatePaths.end(), np.begin(), np.end());
    }

    std::vector<InZipFile> appendFiles;
    int sameFiles = RndInt(0, sourceFiles.size());
    for (int i = 0; i < sameFiles; i++) {
        auto iter = RandomFrom(sourceFiles);
        InZipParams params = RndInt(0, 1) ? iter->second.params : GenInZipParams();
        appendFiles.push_back(InZipFile{params, iter->second.contents});
    }
    int rndFiles = RndInt(0, sourceFiles.size());
    for (int i = 0; i < rndFiles; i++) {
        auto params = GenInZipParams();
        auto contents = GenFileContents();
        appendFiles.push_back(InZipFile{params, std::move(contents)});
    }

    {
        auto np = GenPaths(appendableZips.size() + 1, ".zip");
        appendableZips.insert(appendableZips.end(), np.begin(), np.end());
    }
    for (auto& f : appendFiles) {
        std::string zipPath = RandomFrom(appendableZips);
        InZipState &inzip = state[zipPath];
        std::string path = RandomFrom(candidatePaths);
        int pos = RndInt(0, 1) || inzip.empty() ? inzip.size() : RndInt(0, inzip.size()-1);
        inzip.insert(inzip.begin() + pos, std::make_pair(std::move(path), std::move(f)));
    }

    return state;
}

void TestCreator::TryAddFullZip(DirState &target, std::vector<DirState*> provided) {
    if (RndInt(0, 99) >= 40)
        return;
    int numZips = RndInt(1, 2);
    DirState added = GenTargetState(RndInt(numZips, 4), numZips);

    std::vector<std::string> zipnames;
    for (const auto &zipPair : added)
        zipnames.push_back(zipPair.first);
    for (const auto &zipPair : target)
        zipnames.push_back(zipPair.first);
    for (auto *prov : provided)
        for (const auto &zipPair : *prov)
            zipnames.push_back(zipPair.first);

    if (RndInt(0, 99) < 50) {
        DirState ns;
        for (auto &zipPair : added) {
            std::string newName = RandomFrom(zipnames);
            ns[newName] = zipPair.second;
        }
        added = ns;
    }

    for (const auto &zipPair : added) {
        for (const auto &filePair : zipPair.second)
            target[zipPair.first].push_back(filePair);
        int mult = RndInt(1, 2);
        for (int i = 0; i < mult; i++) {
            DirState &other = *RandomFrom(provided);
            std::string zipName = (RndInt(0, 99) < 50 ? GenPaths(1, ".zip")[0] : RandomFrom(zipnames));
            for (const auto &filePair : zipPair.second)
                other[zipName].push_back(filePair);
        }
    }
}

void TestCreator::AddDuplicateFiles(DirState &target, std::vector<DirState*> provided) {
    int cnt = RndInt(0, 3);
    if (cnt == 0)
        return;
    for (int idx = 0; idx < cnt; idx++) {
        DirState tmpState = GenTargetState(1, 1);
        std::string origZfn = tmpState.begin()->first;
        auto origInzf = tmpState.begin()->second[0];
        int tMult = RndInt(1, 2);
        int pMult = RndInt(1, 2);
        for (int t = 0; t < tMult; t++) {
            if (t == 0 || target.empty())
                target[origZfn].push_back(origInzf);
            else {
                auto inzf = origInzf;
                if (RndInt(0, 99) < 70);
                    inzf.first = GenPaths(1)[0];
                RandomFrom(target).second.push_back(inzf);
            }
        }
        for (int p = 0; p < pMult; p++) {
            auto inzf = origInzf;
            if (RndInt(0, 1)) {
                InZipParams rndParams = GenInZipParams();
                if (RndInt(0, 99) < 20)
                    inzf.first = GenPaths(1)[0];
                if (RndInt(0, 99) < 20)
                    inzf.second.params.externalAttribs = rndParams.externalAttribs;
                if (RndInt(0, 99) < 20) {
                    inzf.second.params.method = rndParams.method;
                    inzf.second.params.level = rndParams.level;
                }
            }
            DirState *prov = RandomFrom(provided);
            if (p == 0 || prov->empty())
                (*prov)[origZfn].push_back(inzf);
            else
                RandomFrom(*prov).second.push_back(inzf);
        }
    }
}

bool TestCreator::AddMissingFiles(const DirState &target, std::vector<DirState*> provided, bool leaveMisses) {
    std::vector<const InZipFile*> targetFiles;
    for (const auto &zipPair : target)
        for (const auto &filePair : zipPair.second)
            targetFiles.push_back(&filePair.second);

    std::vector<const InZipFile*> providedFiles;
    for (const DirState *dir : provided) {
        for (const auto &zipPair : *dir)
            for (const auto &filePair : zipPair.second)
                providedFiles.push_back(&filePair.second);
    }

    int k = 0;
    for (const InZipFile *tf : targetFiles) {
        bool present = false;
        for (const InZipFile *pf : providedFiles) {
            if (DoFilesMatch(*tf, *pf))
                present = true;
        }
        if (!present)
            targetFiles[k++] = tf;
    }
    targetFiles.resize(k);

    if (targetFiles.empty())
        return true;
    bool surelySucceed = true;
    if (leaveMisses && RndInt(0, 1)) {
        targetFiles.resize(RndInt(k/2, k-1));
        surelySucceed = false;
    }

    auto filePaths = GenPaths(targetFiles.size());
    for (int i = 0; i < targetFiles.size(); i++) {
        DirState *dst = RandomFrom(provided);
        std::string zipPath = GenPaths(1, ".zip")[0];
        InZipState &inzip = (*dst)[zipPath];
        std::string path = filePaths[i];
        int pos = RndInt(0, inzip.size());
        inzip.insert(inzip.begin() + pos, std::make_pair(path, *targetFiles[i]));
    }
    return surelySucceed;
}

void TestCreator::SplitState(const DirState &full, std::vector<DirState> &parts) {
    int n = parts.size();
    for (int i = 0; i < n; i++)
         parts[i].clear();
    std::vector<char> used;
    for (const auto &zipPair : full)
        for (const auto &filePair : zipPair.second) {
            int q = (RndInt(0, 99) < 25 ? 2 : 1);
            used.assign(n, false);
            for (int j = 0; j < q; j++) {
                int x = RndInt(0, n-1);
                if (used[x]) continue;
                used[x] = true;
                parts[x][zipPair.first].push_back(filePair);
            }
        }
}

bool TestCreator::ArePathsCaseAliased(std::string pathA, std::string pathB) {
#ifndef _WIN32
    return false;  //case-sensitive
#else
    pathA += '/';
    pathB += '/';
    int k = 0;
    //find common case-insensitive prefix
    while (k < pathA.size() && k < pathB.size() && tolower(pathA[k]) == tolower(pathB[k]))
        k++;
    //trim end up to last slash
    while (k > 0 && pathA[k-1] != '/')
        k--;
    //check that prefixes are same
    std::string prefixA = pathA.substr(0, k);
    std::string prefixB = pathB.substr(0, k);
    if (prefixA != prefixB)
        return true;
    return false;
#endif
}
bool TestCreator::CheckForCaseAliasing(const DirState &state1, const DirState &state2) {
    for (const auto &zp1 : state1) {
        for (const auto &zp2 : state2)
            if (ArePathsCaseAliased(zp1.first, zp2.first))
                return true;
    }
    return false;
}

void TestCreator::WriteState(const std::string &localRoot, const std::string &rr, const DirState &state, Manifest *mani) {
    std::string remoteRoot = rr;
    if (remoteRoot.empty())
        remoteRoot = localRoot;
    Manifest addedMani;
    for (const auto &zipPair : state) {
        PathAR zipPath = PathAR::FromRel(zipPair.first, localRoot);
        stdext::create_directories(stdext::path(zipPath.abs).parent_path());

        //note: minizip's unzip cannot work with empty zip files
        if (zipPair.second.empty())
            continue;

        ZipFileHolder zf(zipPath.abs.c_str());
        for (const auto &filePair : zipPair.second) {
            const auto &params = filePair.second.params;
            const auto &contents = filePair.second.contents;
            zip_fileinfo info;
            info.dosDate = params.dosDate;
            info.internal_fa = params.internalAttribs;
            info.external_fa = params.externalAttribs;
            SAFE_CALL(zipOpenNewFileInZip(zf, filePair.first.c_str(), &info, NULL, 0, NULL, 0, NULL, params.method, params.level));
            SAFE_CALL(zipWriteInFileInZip(zf, contents.data(), contents.size()));
            SAFE_CALL(zipForceDataType(zf, info.internal_fa));
            SAFE_CALL(zipCloseFileInZip(zf));
        }
        zf.reset();

        addedMani.AppendLocalZip(zipPath.abs, localRoot, "default");
    }
    addedMani.ReRoot(remoteRoot);
    if (mani)
        mani->AppendManifest(addedMani);
}

}
