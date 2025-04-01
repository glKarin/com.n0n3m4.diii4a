#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <random>
#include <chrono>

#include "Utils.h"
#include "StdFilesystem.h"
#include "ZipSync.h"
#include "Fuzzer.h"
#include "HttpServer.h"
#include "Downloader.h"
#include "TestCreator.h"
#include "ChecksummedZip.h"
#include "minizip_extra.h"
using namespace ZipSync;

#include <zip.h>
#include <blake2.h>
#include <curl/curl.h>


#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif
//TODO: move it somewhere
stdext::path GetCwd() {
    char buffer[4096];
    char *ptr = getcwd(buffer, sizeof(buffer));
    return ptr;
}
stdext::path GetTempDir() {
    int timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 % 1000000000;
    static stdext::path where = GetCwd() / "__temp__" / std::to_string(timestamp);
    return where;
}
struct LoggerTest : Logger {
    std::map<LogCode, int> counts;
    void Message(LogCode code, Severity severity, const char *message) override {
        counts[code]++;
    }
    void clear() {
        counts.clear();
    }
};
LoggerTest *g_testLogger = new LoggerTest();


TEST_CASE("Paths") {
    CHECK(PathAR::IsHttp("http://darkmod.taaaki.za.net/release") == true);
    CHECK(PathAR::IsHttp("http://tdmcdn.azureedge.net/") == true);
    CHECK(PathAR::IsHttp("C:\\TheDarkMod\\darkmod_207") == false);
    CHECK(PathAR::IsHttp("darkmod_207") == false);
    CHECK(PathAR::IsHttp("/usr/bin/darkmod_207") == false);

    const char *cases[][3] = {
        {"tdm_shared_stuff.zip", "C:/TheDarkMod/darkmod_207", "C:/TheDarkMod/darkmod_207/tdm_shared_stuff.zip"},
        {"tdm_shared_stuff.zip", "C:/TheDarkMod/darkmod_207/", "C:/TheDarkMod/darkmod_207/tdm_shared_stuff.zip"},
        {"a/b/c/x.pk4", "C:/TheDarkMod/darkmod_207/", "C:/TheDarkMod/darkmod_207/a/b/c/x.pk4"},
        {"tdm_shared_stuff.zip", "http://tdmcdn.azureedge.net/", "http://tdmcdn.azureedge.net/tdm_shared_stuff.zip"},
        {"a/b/c/x.pk4", "http://tdmcdn.azureedge.net/", "http://tdmcdn.azureedge.net/a/b/c/x.pk4"},
        {"linux_exe.zip", "/export/home/stgatilov", "/export/home/stgatilov/linux_exe.zip"}
    };
    for (int i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        CHECK(PathAR::IsAbsolute(cases[i][0]) == false);
        CHECK(PathAR::IsAbsolute(cases[i][1]) == true);
        CHECK(PathAR::IsAbsolute(cases[i][2]) == true);
        PathAR a = PathAR::FromRel(cases[i][0], cases[i][1]);
        PathAR b = PathAR::FromAbs(cases[i][2], cases[i][1]);
        CHECK(a.rel == cases[i][0]);
        CHECK(a.abs == cases[i][2]);
        CHECK(b.rel == cases[i][0]);
        CHECK(b.abs == cases[i][2]);
        std::string rootDir = cases[i][1];
        if (rootDir.back() == '/')
            rootDir.pop_back();
        CHECK(a.GetRootDir() == rootDir);
        CHECK(b.GetRootDir() == rootDir);
    }

    CHECK(PrefixFile("C:/TheDarkMod/stuff.zip", "__prefix__") == "C:/TheDarkMod/__prefix__stuff.zip");
}

HashDigest GenHash(int idx) {
    std::minstd_rand rnd(idx + 0xDEADBEEF);
    uint32_t data[8];
    for (int i = 0; i < 8; i++)
        data[i] = rnd();
    HashDigest res = Hasher().Update(data, sizeof(data)).Finalize();
    return res;
}
template<class T> const T &Search(const std::vector<std::pair<std::string, T>> &data, const std::string &key) {
    int pos = -1;
    int cnt = 0;
    for (int i = 0; i < data.size(); i++) {
        if (data[i].first == key) {
            pos = i;
            cnt++;
        }
    }
    REQUIRE(cnt == 1);
    return data[pos].second;
}

TEST_CASE("FileUtils") {
    auto dir = GetTempDir();
    stdext::create_directories(dir);

    CHECK(IfFileExists((dir / "non_existing_file.txt").string()) == false);
    CHECK_THROWS(GetFileSize((dir / "non_existing_file.txt").string()));
    {
        StdioFileHolder f(fopen((dir / "existing_file.txt").string().c_str(), "wt"));
        fprintf(f, "Test message!\n");
    }
    CHECK(IfFileExists((dir / "existing_file.txt").string()) == true);
    {
        StdioFileHolder f(fopen((dir / "existing_file.bin").string().c_str(), "wb"));
        fprintf(f, "Binary file\n");
    }
    CHECK(GetFileSize((dir / "existing_file.bin").string()) == 12);

    RenameFile((dir / "existing_file.txt").string(), (dir / "renamed_file.txt").string());
    CHECK(IfFileExists((dir / "existing_file.txt").string()) == false);
    CHECK(IfFileExists((dir / "renamed_file.txt").string()) == true);
    CHECK_THROWS(RenameFile((dir / "existing_file.txt").string(), (dir / "renamed_file.txt").string()));

    RemoveFile((dir / "renamed_file.txt").string());
    CHECK(IfFileExists((dir / "renamed_file.txt").string()) == false);
    CHECK_THROWS(RemoveFile((dir / "renamed_file.txt").string()));

    CHECK(CreateDir((dir / "test_dir").string()) == true);
    CHECK(CreateDir((dir / "test_dir").string()) == false);
    {
        StdioFileHolder f(fopen((dir / "test_dir" / "test_file").string().c_str(), "wt"));
    }

    CHECK(stdext::exists(dir / "test_dir") == true);
    CHECK(RemoveDirectoryIfEmpty((dir / "test_dir").string()) == false);
    CHECK(stdext::exists(dir / "test_dir") == true);
    RemoveFile((dir / "test_dir" / "test_file").string());
    CHECK(RemoveDirectoryIfEmpty((dir / "test_dir").string()) == true);
    CHECK(stdext::exists(dir / "test_dir") == false);
    CHECK_THROWS(RemoveDirectoryIfEmpty((dir / "test_dir").string()));

    CreateDirectoriesForFile((dir / "test_file").string(), dir.string());
    CreateDirectoriesForFile((dir / "test_dir" / "test_file").string(), dir.string());
    CreateDirectoriesForFile((dir / "test_dir" / "a" / "b" / "c" / "test_file").string(), dir.string());
    {
        StdioFileHolder f(fopen((dir / "test_dir" / "a" / "b" / "c" / "test_file").string().c_str(), "wt"));
    }
    CHECK(IfFileExists((dir / "test_dir" / "a" / "b" / "c" / "test_file").string()) == true);

    PruneDirectoriesAfterFileRemoval((dir / "test_dir" / "a" / "b" / "c" / "test_file").string(), (dir / "test_dir").string());
    CHECK(stdext::exists(dir / "test_dir" / "a" / "b" / "c") == true);
    RemoveFile((dir / "test_dir" / "a" / "b" / "c" / "test_file").string());
    PruneDirectoriesAfterFileRemoval((dir / "test_dir" / "a" / "b" / "c" / "test_file").string(), (dir / "test_dir").string());
    CHECK(stdext::exists(dir / "test_dir" / "a") == false);
    CHECK(stdext::exists(dir / "test_dir") == true);
}

TEST_CASE("ProvidedManifest: Read/Write") {
    Manifest mani;

    FileMetainfo pf;
    memset(&pf.props, 0, sizeof(pf.props));
    pf.location = FileLocation::Nowhere;
    pf.filename = "textures/model/darkmod/grass/grass01.jpg";
    pf.zipPath.rel = "subdir/win32/interesting_name456.pk4";
    pf.compressedHash = GenHash(1);
    pf.contentsHash = GenHash(2);
    pf.byterange[0] = 0;
    pf.byterange[1] = 123456;
    mani.AppendFile(pf);
    pf.filename = "models/darkmod/guards/head.lwo";
    pf.zipPath.rel = "basic_assets.pk4";
    pf.compressedHash = GenHash(5);
    pf.contentsHash = GenHash(6);
    pf.byterange[0] = 1000000000;
    pf.byterange[1] = 1000010000;
    mani.AppendFile(pf);
    pf.filename = "textures/model/standalone/menu.png";
    pf.zipPath.rel = "subdir/win32/interesting_name456.pk4";
    pf.compressedHash = GenHash(3);
    pf.contentsHash = GenHash(4);
    pf.byterange[0] = 123456;
    pf.byterange[1] = 987654;
    mani.AppendFile(pf);

    IniData savedIni = mani.WriteToIni();

    Manifest restored;
    restored.ReadFromIni(savedIni, "nowhere");

    std::vector<int> order = {1, 0, 2};
    for (int i = 0; i < order.size(); i++) {
        const FileMetainfo &src = mani[order[i]];
        const FileMetainfo &dst = restored[i];
        CHECK(src.zipPath.rel == dst.zipPath.rel);
        CHECK(src.filename == dst.filename);
        CHECK(src.compressedHash == dst.compressedHash);
        CHECK(src.contentsHash == dst.contentsHash);
        CHECK(src.byterange[0] == dst.byterange[0]);
        CHECK(src.byterange[1] == dst.byterange[1]);
    }

    for (int t = 0; t < 5; t++) {
        Manifest newMani;
        newMani.ReadFromIni(savedIni, "nowhere");
        IniData newIni = newMani.WriteToIni();
        CHECK(savedIni == newIni);
    }
}

TEST_CASE("TargetManifest: Read/Write") {
    Manifest mani;

    FileMetainfo tf;
    tf.byterange[0] = tf.byterange[1] = 0;
    memset(&tf.props, 0, sizeof(tf.props));
    tf.package = "interesting";
    tf.zipPath.rel = "subdir/win32/interesting_name456.pk4";
    tf.compressedHash = GenHash(1);
    tf.contentsHash = GenHash(2);
    tf.filename = "textures/model/darkmod/grass/grass01.jpg";
    tf.props.lastModTime = 1150921251;
    tf.props.compressionMethod = 8;
    tf.props.generalPurposeBitFlag = 2;
    tf.props.compressedSize = 171234;
    tf.props.contentsSize = 214567;
    tf.props.internalAttribs = 1234;
    tf.props.externalAttribs = 123454321;
    mani.AppendFile(tf);
    tf.package = "assets";
    tf.zipPath.rel = "basic_assets.pk4";
    tf.compressedHash = GenHash(5);
    tf.contentsHash = GenHash(6);
    tf.filename = "models/darkmod/guards/head.lwo";
    tf.props.lastModTime = 100000000;
    tf.props.compressionMethod = 0;
    tf.props.generalPurposeBitFlag = 0;
    tf.props.compressedSize = 4567891;
    tf.props.contentsSize = 4567891;
    tf.props.internalAttribs = 0;
    tf.props.externalAttribs = 4000000000U;
    mani.AppendFile(tf);
    tf.package = "assets";
    tf.zipPath.rel = "subdir/win32/interesting_name456.pk4";
    tf.compressedHash = GenHash(3);
    tf.contentsHash = GenHash(4);
    tf.filename = "textures/model/standalone/menu.png";
    tf.props.lastModTime = 4000000000U;
    tf.props.compressionMethod = 8;
    tf.props.generalPurposeBitFlag = 6;
    tf.props.compressedSize = 12012;
    tf.props.contentsSize = 12001;
    tf.props.internalAttribs = 7;
    tf.props.externalAttribs = 45;
    mani.AppendFile(tf);

    IniData savedIni = mani.WriteToIni();

    Manifest restored;
    restored.ReadFromIni(savedIni, "nowhere");

    std::vector<int> order = {1, 0, 2};
    for (int i = 0; i < order.size(); i++) {
        const FileMetainfo &src = mani[order[i]];
        const FileMetainfo &dst = restored[i];
        CHECK(src.zipPath.rel == dst.zipPath.rel);
        CHECK(src.package == dst.package);
        CHECK(src.compressedHash == dst.compressedHash);
        CHECK(src.contentsHash == dst.contentsHash);
        CHECK(src.filename == dst.filename);
        CHECK(src.props.lastModTime == dst.props.lastModTime);
        CHECK(src.props.compressionMethod == dst.props.compressionMethod);
        CHECK(src.props.generalPurposeBitFlag == dst.props.generalPurposeBitFlag);
        CHECK(src.props.compressedSize == dst.props.compressedSize);
        CHECK(src.props.contentsSize == dst.props.contentsSize);
    }

    for (int t = 0; t < 5; t++) {
        Manifest newMani;
        newMani.ReadFromIni(savedIni, "nowhere");
        IniData newIni = newMani.WriteToIni();
        CHECK(savedIni == newIni);
    }
}

TEST_CASE("Ini: Read/Write") {
    IniData ini;
    for (int i = 0; i < 5; i++) {
        IniSect sec;
        sec.emplace_back("index", std::to_string(i));
        sec.emplace_back("square", std::to_string(i*i));
        sec.emplace_back("index", std::to_string(i));
        sec.emplace_back("custom" + std::to_string(i*10), "10 x number");
        sec.emplace_back("index", std::to_string(i));
        ini.emplace_back("sec" + std::to_string(i), std::move(sec));
    }

    stdext::create_directories(GetTempDir());
    std::string testpath = (GetTempDir() / stdext::path("test.ini")).string();
    std::string testpathz = (GetTempDir() / stdext::path("test.iniz")).string();
    WriteIniFile(testpath.c_str(), ini, IniMode::Auto);
    WriteIniFile(testpathz.c_str(), ini, IniMode::Auto);

    IniData unpacked = ReadIniFile(testpath.c_str());
    IniData packed = ReadIniFile(testpathz.c_str());
    CHECK(unpacked == ini);
    CHECK(packed == ini);

    REQUIRE(IfFileExists(testpath));
    REQUIRE(IfFileExists(testpathz));
    CHECK_THROWS(UnzFileHolder(testpath.c_str()));
    UnzFileHolder zf(testpathz.c_str());
}

TEST_CASE("AppendManifestsFromLocalZip") {
    std::string rootDir = GetTempDir().string();
    std::string zipPath1 = (GetTempDir() / stdext::path("a/f1.zip")).string();
    std::string zipPath2 = (GetTempDir() / stdext::path("amt.pk4")).string();

    std::string fnPkgJson = "data/pkg.json";
    std::string fnRndDat = "rnd.dat";
    std::string fnSeqBin = "data/Seq.bin";
    std::string fnDoubleDump = "aRMy/Of/GoOd/WiLl/DoUbLe.dump";

    std::string cntPkgJson = R"(
# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/tdmsync2")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    )";
    std::vector<int> cntSeqBin;
    for (int i = 0; i < 10000; i++)
        cntSeqBin.push_back(i);
    std::vector<int> cntRndDat;
    std::mt19937 rnd;
    for (int i = 0; i < 1234; i++)
        cntRndDat.push_back(rnd());
    std::vector<double> cntDoubleDump;
    for (int i = 0; i < 1000; i++)
        cntDoubleDump.push_back(double(i) / double(1000));

    #define PACK_BUF(buf) zipWriteInFileInZip(zf, buf.data(), buf.size() * sizeof(buf[0]));
    stdext::create_directories(stdext::path(zipPath1).parent_path());
    zipFile zf = zipOpen(zipPath1.c_str(), 0);
    zipOpenNewFileInZip(zf, fnPkgJson.c_str(), NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    PACK_BUF(cntPkgJson);
    zipCloseFileInZip(zf);
    zipOpenNewFileInZip(zf, fnRndDat.c_str(), NULL, NULL, 0, NULL, 0, NULL, 0, 0);
    PACK_BUF(cntRndDat);
    zipCloseFileInZip(zf);
    zip_fileinfo info;
    info.dosDate = 123456789;
    info.internal_fa = 123;
    info.external_fa = 0xDEADBEEF;
    zipOpenNewFileInZip(zf, fnSeqBin.c_str(), &info, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);
    PACK_BUF(cntSeqBin);
    zipCloseFileInZip(zf);
    zipClose(zf, NULL);
    stdext::create_directories(stdext::path(zipPath2).parent_path());
    zf = zipOpen(zipPath2.c_str(), 0);
    zipOpenNewFileInZip(zf, fnDoubleDump.c_str(), NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_SPEED);
    PACK_BUF(cntDoubleDump);
    zipCloseFileInZip(zf);
    zipClose(zf, NULL);
    #undef PACK_BUF

    Manifest mani;
    AppendManifestsFromLocalZip(
        zipPath1, rootDir,
        FileLocation::Local, "default",
        mani
    );
    AppendManifestsFromLocalZip(
        zipPath2, rootDir,
        FileLocation::RemoteHttp, "chaos",
        mani
    );

    REQUIRE(mani.size() == 4);
    REQUIRE(mani[0].filename == fnPkgJson   );
    REQUIRE(mani[1].filename == fnRndDat    );
    REQUIRE(mani[2].filename == fnSeqBin    );
    REQUIRE(mani[3].filename == fnDoubleDump);

    CHECK(mani[0].zipPath.abs == zipPath1);
    CHECK(mani[1].zipPath.abs == zipPath1);
    CHECK(mani[2].zipPath.abs == zipPath1);
    CHECK(mani[3].zipPath.abs == zipPath2);
    CHECK(mani[0].location == FileLocation::Local);
    CHECK(mani[1].location == FileLocation::Local);
    CHECK(mani[2].location == FileLocation::Local);
    CHECK(mani[3].location == FileLocation::RemoteHttp);
    CHECK(mani[0].package == "default");
    CHECK(mani[1].package == "default");
    CHECK(mani[2].package == "default");
    CHECK(mani[3].package == "chaos"  );
    CHECK(mani[0].contentsHash.Hex() == "8ec061d20526f1e5ce56519f09bc1ee2ad065464e3e7cbbb94324865bca95a45"); //computed externally in Python
    CHECK(mani[1].contentsHash.Hex() == "75b25a4dd22ac100925e09d62016c0ffdb5998b470bc685773620d4f37458b69");
    CHECK(mani[2].contentsHash.Hex() == "54b97c474a60b36c16a5c6beea5b2a03a400096481196bbfe2202ef7a547408c");
    CHECK(mani[3].contentsHash.Hex() == "009c0860b467803040c61deb6544a3f515ac64c63d234e286d3e2fa352411e91");
    CHECK(mani[0].props.lastModTime == 0);
    CHECK(mani[1].props.lastModTime == 0);
    CHECK(mani[2].props.lastModTime == 123456789);
    CHECK(mani[3].props.lastModTime == 0);
    CHECK(mani[0].props.compressionMethod == Z_DEFLATED);
    CHECK(mani[1].props.compressionMethod == 0);
    CHECK(mani[2].props.compressionMethod == Z_DEFLATED);
    CHECK(mani[3].props.compressionMethod == Z_DEFLATED);
#define SIZE_BUF(buf) buf.size() * sizeof(buf[0])
    CHECK(mani[0].props.contentsSize == SIZE_BUF(cntPkgJson));
    CHECK(mani[1].props.contentsSize == SIZE_BUF(cntRndDat));
    CHECK(mani[2].props.contentsSize == SIZE_BUF(cntSeqBin));
    CHECK(mani[3].props.contentsSize == SIZE_BUF(cntDoubleDump));
#undef SIZE_BUF
    CHECK(mani[0].props.generalPurposeBitFlag == 0);     //"normal"
    CHECK(mani[1].props.generalPurposeBitFlag == 0);     //no compression (stored)
    CHECK(mani[2].props.generalPurposeBitFlag == 2);     //"maximum"
    CHECK(mani[3].props.generalPurposeBitFlag == 6);     //"super fast"
    CHECK(mani[0].props.internalAttribs == 1);        //Z_TEXT
    CHECK(mani[1].props.internalAttribs == 0);        //Z_BINARY
    CHECK(mani[2].props.internalAttribs == 123);      //custom (see above)
    CHECK(mani[3].props.internalAttribs == 0);        //Z_BINARY
    CHECK(mani[0].props.externalAttribs == 0);
    CHECK(mani[1].props.externalAttribs == 0);
    CHECK(mani[2].props.externalAttribs == 0xDEADBEEF);
    CHECK(mani[3].props.externalAttribs == 0);

    double RATIOS[4][2] = {
        {0.5, 0.75},        //text is rather compressible
        {1.0, 1.0},         //store: size must be exactly same
        {0.3, 0.4},         //integer sequence is well-compressible
        {0.3, 0.4},         //doubles are well-compressible
    };
    for (int i = 0; i < 4; i++) {
        double ratio = double(mani[i].props.compressedSize) / mani[i].props.contentsSize;
        CHECK(ratio >= RATIOS[i][0]);  CHECK(ratio <= RATIOS[i][1]);
    }

    for (int i = 0; i < 4; i++) {
        const uint32_t *br = mani[i].byterange;
        std::vector<char> fdata(br[1] - br[0]);
        int totalSize = mani[i].props.compressedSize + mani[i].filename.size() + 30;
        CHECK(fdata.size() == totalSize);

        FILE *f = fopen(mani[i].zipPath.abs.c_str(), "rb");
        REQUIRE(f);
        fseek(f, br[0], SEEK_SET);
        CHECK(fread(fdata.data(), 1, fdata.size(), f) == fdata.size());
        fclose(f);

        CHECK(*(int*)&fdata[0] == 0x04034b50);   //local file header signature
        CHECK(memcmp(&fdata[30], mani[i].filename.c_str(), mani[i].filename.size()) == 0);

        int offs = fdata.size() - mani[i].props.compressedSize, sz = mani[i].props.compressedSize;
        HashDigest digest = Hasher().Update(fdata.data() + offs, sz).Finalize();
        CHECK(mani[i].compressedHash == digest);
    }
}

TEST_CASE("BadZips") {
    std::string rootDir = GetTempDir().string();
    stdext::create_directories(stdext::path(rootDir));

    std::string zipPath[128];
    for (int i = 0; i < 128; i++)
        zipPath[i] = (GetTempDir() / stdext::path("badzip" + std::to_string(i) + ".zip")).string();

    #define TEST_BEGIN \
    { \
        zipFile zf = zipOpen(zipPath[testsCount++].c_str(), 0);
    #define TEST_END \
        zipWriteInFileInZip(zf, rootDir.data(), rootDir.size()); \
        zipCloseFileInZip(zf); \
        zipClose(zf, NULL); \
    }

    int testsCount = 0;
    TEST_BEGIN  //version made by
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_SPEED, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 63, 0);
    TEST_END
    TEST_BEGIN  //flags
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_SPEED, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, 8);
    TEST_END
    TEST_BEGIN  //flags
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_SPEED, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, 16);
    TEST_END
    TEST_BEGIN  //flags (encrypted)
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_SPEED, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, "password", 0, 0, 0);
    TEST_END
    TEST_BEGIN  //extra field (global)
        const char extra_field[] = "extra_field";
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, NULL, 0, extra_field, strlen(extra_field), NULL, Z_DEFLATED, Z_BEST_SPEED, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, 0);
    TEST_END
#if 0   //not verified yet: minizip ignores it
    TEST_BEGIN //extra field (local)
        const char extra_field[] = "extra_field";
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, extra_field, strlen(extra_field), NULL, 0, NULL, Z_DEFLATED, Z_BEST_SPEED, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, 0);
    TEST_END
#endif
    TEST_BEGIN  //comment
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, NULL, 0, NULL, 0, "comment", Z_DEFLATED, Z_BEST_SPEED, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, 0);
    TEST_END
    {           //crc
        zipFile zf = zipOpen(zipPath[testsCount++].c_str(), 0);
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, NULL, 0, NULL, 0, NULL, 0, 0, 1, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, 0);
        zipWriteInFileInZip(zf, rootDir.data(), rootDir.size());
        zipCloseFileInZipRaw(zf, rootDir.size(), 0xDEADBEEF);
        zipClose(zf, NULL);
    }
    {           //uncompressed size
        zipFile zf = zipOpen(zipPath[testsCount++].c_str(), 0);
        zipOpenNewFileInZip4(zf, "temp.txt", NULL, NULL, 0, NULL, 0, NULL, 0, 0, 1, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, 0);
        zipCloseFileInZipRaw(zf, 123456789, 0);
        zipClose(zf, NULL);
    }
    //TODO: check other cases (which cannot be generated with minizip) ?

    #undef TEST_BEGIN
    #undef TEST_END

    for (int i = 0; i < testsCount; i++) {
        CHECK_THROWS(Manifest().AppendLocalZip(zipPath[i], rootDir, ""));
    }
}

TEST_CASE("UpdateProcess::DevelopPlan") {
    Manifest provided;
    Manifest target;

    struct MatchAnswer {
        int bestLocation = int(FileLocation::Nowhere);
        std::vector<std::string> filenames;
    };
    //for each target filename: list of provided filenames which match
    std::map<std::string, MatchAnswer> correctMatching[2];

    //generate one file per every possible combination of availability:
    //  target file is: [missing/samecontents/samecompressed] on [inplace/local/remote]
    int mode[3] = {0};
    for (mode[0] = 0; mode[0] < 3; mode[0]++) {
        for (mode[1] = 0; mode[1] < 5; mode[1]++) {
            for (mode[2] = 0; mode[2] < 5; mode[2]++) {
                int targetIdx = target.size();
                FileMetainfo tf;
                tf.contentsHash = GenHash(targetIdx);
                tf.compressedHash = GenHash(targetIdx + 1000);
                tf.zipPath = PathAR::FromRel("target" + std::to_string(targetIdx % 4) + ".zip", "nowhere");
                tf.filename = "file" + std::to_string(targetIdx) + ".dat";
                //note: other members not used

                MatchAnswer *answer[2] = {&correctMatching[0][tf.filename], &correctMatching[1][tf.filename]};
                for (int pl = 0; pl < 3; pl++) {
                    int modes[2] = {mode[pl], -1};
                    if (modes[0] >= 3) {
                        modes[1] = mode[0] - 2;
                        modes[0] = 1;
                    }

                    for (int j = 0; j < 2; j++) if (modes[j] >= 0) {
                        int providedIdx = provided.size();
                        int m = modes[j];

                        FileMetainfo pf;
                        pf.byterange[0] = (providedIdx+0) * 100000;
                        pf.byterange[1] = (providedIdx+1) * 100000;
                        pf.contentsHash = (m >= 1 ? tf.contentsHash : GenHash(providedIdx + 2000));
                        pf.compressedHash = (m == 2 ? tf.compressedHash : GenHash(providedIdx + 3000));
                        if (pl == 0) {
                            pf.location = FileLocation::Local;     //Inplace must be set of DevelopPlan
                            pf.zipPath = tf.zipPath;
                            pf.filename = tf.filename;
                        }
                        else if (pl == 1) {
                            pf.location = FileLocation::Local;
                            pf.zipPath = PathAR::FromRel("other" + std::to_string(providedIdx % 4) + ".zip", "nowhere");
                            pf.filename = "some_file" + std::to_string(providedIdx);
                        }
                        else if (pl == 2) {
                            pf.location = FileLocation::RemoteHttp;
                            pf.zipPath = PathAR::FromRel("other" + std::to_string(providedIdx % 4) + ".zip", "http://localhost:7123");
                            pf.filename = "some_file" + std::to_string(providedIdx);
                        }
                        provided.AppendFile(pf);

                        for (int t = 0; t < 2; t++) {
                            bool matches = (t == 0 ? pf.contentsHash == tf.contentsHash : pf.compressedHash == tf.compressedHash);
                            if (!matches)
                                continue;
                            if (pl < int(answer[t]->bestLocation)) {
                                answer[t]->filenames.clear();
                                answer[t]->bestLocation = pl;
                            }
                            if (pl == int(answer[t]->bestLocation)) {
                                answer[t]->filenames.push_back(pf.filename);
                            }
                        }
                    }
                }

                target.AppendFile(tf);
            }
        }
    }

    std::mt19937 rnd;
    for (int attempt = 0; attempt < 10; attempt++) {

        //try to develop update plan in both modes
        for (int t = 0; t < 2; t++) {

            Manifest targetCopy = target;
            Manifest providedCopy = provided;
            if (attempt > 0) {
                //note: here we assume that files are stored in a vector =(
                std::shuffle(&targetCopy[0], &targetCopy[0] + targetCopy.size(), rnd);
                std::shuffle(&providedCopy[0], &providedCopy[0] + providedCopy.size(), rnd);
            }

            UpdateProcess update;
            update.Init(std::move(targetCopy), std::move(providedCopy), "nowhere");
            update.DevelopPlan(t == 0 ? UpdateType::SameContents : UpdateType::SameCompressed);

            for (int i = 0; i < update.MatchCount(); i++) {
                UpdateProcess::Match match = update.GetMatch(i);
                const MatchAnswer &answer = correctMatching[t][match.target->filename];
                if (answer.filenames.empty())
                    CHECK(!match.provided);
                else {
                    CHECK(match.provided);
                    std::string fn = match.provided->filename;
                    CHECK(std::find(answer.filenames.begin(), answer.filenames.end(), fn) != answer.filenames.end());
                    if (attempt == 0) {
                        //non-shuffled manifests: check that first optimal match is chosen (to be deterministic)
                        CHECK(fn == answer.filenames.front());
                    }
                }
            }
        }
    }
}

static std::string CurlSimple(const std::string &url, const std::string &ranges = "", std::vector<int> wantedHttpCode = {200}) {
    auto WriteCallback = [](char *buffer, size_t size, size_t nitems, void *outstream) -> size_t {
        int bytes = size * nitems;
        std::string &data = *(std::string*)outstream;
        if (data.size() + bytes > data.capacity())
            data.reserve(data.capacity() * 2);
        data.append(buffer, bytes);
        return bytes;
    };
    std::string data;
    std::unique_ptr<CURL, void (*)(CURL*)> curl(curl_easy_init(), curl_easy_cleanup);
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    if (!ranges.empty())
        curl_easy_setopt(curl.get(), CURLOPT_RANGE, ranges.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, (curl_write_callback)WriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &data);
    CURLcode res = curl_easy_perform(curl.get());
    CHECK(res == CURLE_OK);
    long httpRes = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_HTTP_CODE, &httpRes);
    for (int i = 0; i < wantedHttpCode.size(); i++)
        if (wantedHttpCode[i] == httpRes)
            return data;
    CHECK(httpRes == wantedHttpCode[0]);
    return data;
}

std::string ReadWholeFileAsStr(const std::string &filename) {
    auto data = ReadWholeFile(filename);
    return std::string((char*)data.data(), data.size());
}
static void PrepareFilesForHttpServer() {
    stdext::create_directories(GetTempDir());
    StdioFileHolder test((GetTempDir() / "test.txt").string().c_str(), "wb");
    fprintf(test, "Hello, microhttpd!\n");
    StdioFileHolder identity((GetTempDir() / "identity.bin").string().c_str(), "wb");
    for (int i = 0; i < 1000000; i++)
        fwrite(&i, 1, 1, identity);
    stdext::create_directories(GetTempDir() / "subdir");
    StdioFileHolder numbers((GetTempDir() / "subdir" / "squares.txt").string().c_str(), "wb");
    for (int i = 0; i < 100000; i++)
        fprintf(numbers, "%d-th square is %d\n", i, i*i);
}
TEST_CASE("HttpServer") {
    PrepareFilesForHttpServer();
    std::string DataTestTxt = ReadWholeFileAsStr((GetTempDir() / "test.txt").string());
    std::string DataIdentityBin = ReadWholeFileAsStr((GetTempDir() / "identity.bin").string());
    std::string DataSquaresTxt = ReadWholeFileAsStr((GetTempDir() / "subdir" / "squares.txt").string());

    for (int blk = 0; blk < 2; blk++) {
        HttpServer server;
        if (blk == 0)
            server.SetBlockSize(13);    //small block size to test stuff better
        server.SetRootDir(GetTempDir().string());
        server.Start();
        server.Start();

        CHECK(CurlSimple(server.GetRootUrl() + "test.txt") == DataTestTxt);
        CurlSimple(server.GetRootUrl() + "badfilename.txt", "", {404});
        CHECK(CurlSimple(server.GetRootUrl() + "test.txt", "3-9", {206}) == DataTestTxt.substr(3, 7));
        CHECK(CurlSimple(server.GetRootUrl() + "test.txt", "3-", {206}) == DataTestTxt.substr(3));
        CHECK(CurlSimple(server.GetRootUrl() + "test.txt", "13-13", {206}) == DataTestTxt.substr(13, 1));
        CHECK(DataTestTxt.size() == 19);
        CHECK(CurlSimple(server.GetRootUrl() + "test.txt", "0-18", {200,206}) == DataTestTxt);

        std::string mpResp = CurlSimple(server.GetRootUrl() + "test.txt", "2-5,7-10,14-16", {206});
        std::string mpRespExp = R"(
--********72FFC411326F7C93
Content-Range: bytes 2-5/19

llo,
--********72FFC411326F7C93
Content-Range: bytes 7-10/19

micr
--********72FFC411326F7C93
Content-Range: bytes 14-16/19

tpd
--********72FFC411326F7C93--
)";     //note: must start and end with one EOL
        std::string mpRespExp2;
        for (int i = 0; i < mpRespExp.size(); i++) {
            if (mpRespExp[i] == '\n')
                mpRespExp2.push_back('\r');
            mpRespExp2.push_back(mpRespExp[i]);
        }
        CHECK(mpResp == mpRespExp2);

        CurlSimple(server.GetRootUrl() + "test.txt", "5-2", {416});
        CurlSimple(server.GetRootUrl() + "test.txt", "-3-2", {416});
        CurlSimple(server.GetRootUrl() + "test.txt", "-2", {416});
        CurlSimple(server.GetRootUrl() + "test.txt", "23", {416});
        CurlSimple(server.GetRootUrl() + "test.txt", "2fg", {416});
        CurlSimple(server.GetRootUrl() + "test.txt", "0-100", {416});
        CurlSimple(server.GetRootUrl() + "test.txt", "0-3,10-7", {416});
        CurlSimple(server.GetRootUrl() + "test.txt", "0-5,5-7", {416});
        CurlSimple(server.GetRootUrl() + "test.txt", "low-high", {416});

        if (blk == 1) CHECK(CurlSimple(server.GetRootUrl() + "subdir/squares.txt") == DataSquaresTxt);
        CHECK(CurlSimple(server.GetRootUrl() + "subdir/squares.txt", "1000000-1000100", {206}) == DataSquaresTxt.substr(1000000, 101));
        if (blk == 1) CHECK(CurlSimple(server.GetRootUrl() + "subdir/squares.txt", "500000-1500000", {206}) == DataSquaresTxt.substr(500000, 1000001));

        if (blk == 1) CHECK(CurlSimple(server.GetRootUrl() + "identity.bin") == DataIdentityBin);
        CHECK(CurlSimple(server.GetRootUrl() + "identity.bin", "256-512", {206}) == DataIdentityBin.substr(256, 257));
        CHECK(CurlSimple(server.GetRootUrl() + "identity.bin", "5000-15678", {206}) == DataIdentityBin.substr(5000, 10679));
        if (blk == 1) CHECK(CurlSimple(server.GetRootUrl() + "identity.bin", "123456-654321", {206}) == DataIdentityBin.substr(123456, 654321-123456+1));

        if (blk == 0) server.Stop();
    }
}

TEST_CASE("Downloader") {
    PrepareFilesForHttpServer();
    std::string DataTestTxt = ReadWholeFileAsStr((GetTempDir() / "test.txt").string());
    std::string DataIdentityBin = ReadWholeFileAsStr((GetTempDir() / "identity.bin").string());
    std::string DataSquaresTxt = ReadWholeFileAsStr((GetTempDir() / "subdir" / "squares.txt").string());
    auto CreateDownloadCallback = [](std::string &buffer) -> DownloadFinishedCallback {
        return [&buffer](const void *ptr, uint32_t bytes) -> void {
            buffer.assign((char*)ptr, (char*)ptr + bytes);
        };
    };

    for (int blk = 0; blk < 2; blk++) {
        HttpServer server;
        if (blk == 0)
            server.SetBlockSize(17);
        server.SetRootDir(GetTempDir().string());
        server.Start();

        { //download all
            Downloader down;
            std::string data1, data2, data3;
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt"), CreateDownloadCallback(data1));
            if (blk == 1) {
                down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "identity.bin"), CreateDownloadCallback(data2));
                down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt"), CreateDownloadCallback(data3));
            }
            double progressRatio = -1.0;
            int progressCnt = 0;
            GlobalProgressCallback progressCallback = [&](double ratio, const char *message) -> int {
                if (progressCnt == 0)
                    CHECK(ratio == 0.0);
                CHECK(ratio >= progressRatio);
                progressRatio = ratio;
                progressCnt++;
                return 0;
            };
            down.SetProgressCallback(progressCallback);
            down.DownloadAll();
            CHECK(progressRatio == 1.0);
            CHECK(data1 == DataTestTxt);
            int totalDownloaded = down.TotalBytesDownloaded();
            if (blk == 1) {
                CHECK(data2 == DataIdentityBin);
                CHECK(data3 == DataSquaresTxt);
                CHECK(progressCnt >= 50);   // number of calls depends on CURL default settings...
                int sumSize = DataTestTxt.size() + DataIdentityBin.size() + DataSquaresTxt.size();
                CHECK(totalDownloaded == sumSize);
            }
        }

        { //download ranges
            Downloader down;
            std::string data1, data2, data3;
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt", 2, 6), CreateDownloadCallback(data1));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt", 7, 11), CreateDownloadCallback(data2));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt", 14, 17), CreateDownloadCallback(data3));
            down.DownloadAll();
            CHECK(data1 == DataTestTxt.substr(2, 4));
            CHECK(data2 == DataTestTxt.substr(7, 4));
            CHECK(data3 == DataTestTxt.substr(14, 3));
            int totalDownloaded = down.TotalBytesDownloaded();
            CHECK(totalDownloaded >= 11);
        }

        { //check overlapping and single-range download
            Downloader down;
            std::string data[8];
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt", 2, 10), CreateDownloadCallback(data[0]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt", 7, 13), CreateDownloadCallback(data[1]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "identity.bin", 5000, 10000), CreateDownloadCallback(data[2]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "identity.bin", 6000, 7000), CreateDownloadCallback(data[3]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 1000, 5000), CreateDownloadCallback(data[4]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 3000, 4000), CreateDownloadCallback(data[5]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 2000, 3000), CreateDownloadCallback(data[6]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 6000, 7000), CreateDownloadCallback(data[7]));
            down.DownloadAll();
            CHECK(data[0] == DataTestTxt.substr(2, 8));
            CHECK(data[1] == DataTestTxt.substr(7, 6));
            CHECK(data[2] == DataIdentityBin.substr(5000, 5000));
            CHECK(data[3] == DataIdentityBin.substr(6000, 1000));
            CHECK(data[4] == DataSquaresTxt.substr(1000, 4000));
            CHECK(data[5] == DataSquaresTxt.substr(3000, 1000));
            CHECK(data[6] == DataSquaresTxt.substr(2000, 1000));
            CHECK(data[7] == DataSquaresTxt.substr(6000, 1000));
        }

        { //download ranges from many files
            Downloader down;
            std::string data[10];
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt", 2, 10), CreateDownloadCallback(data[0]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt", 7, 13), CreateDownloadCallback(data[1]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "identity.bin", 10000, 11000), CreateDownloadCallback(data[2]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt", 16, 19), CreateDownloadCallback(data[3]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "identity.bin", 100000, 102000), CreateDownloadCallback(data[4]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 12345, 54321), CreateDownloadCallback(data[5]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "identity.bin", 101000, 103000), CreateDownloadCallback(data[6]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 50000, 60000), CreateDownloadCallback(data[7]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 30000, 40000), CreateDownloadCallback(data[8]));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "identity.bin", 50000, 55000), CreateDownloadCallback(data[9]));
            down.DownloadAll();
            CHECK(data[0] == DataTestTxt.substr(2, 8));
            CHECK(data[1] == DataTestTxt.substr(7, 6));
            CHECK(data[2] == DataIdentityBin.substr(10000, 1000));
            CHECK(data[3] == DataTestTxt.substr(16, 3));
            CHECK(data[4] == DataIdentityBin.substr(100000, 2000));
            CHECK(data[5] == DataSquaresTxt.substr(12345, 54321-12345));
            CHECK(data[6] == DataIdentityBin.substr(101000, 2000));
            CHECK(data[7] == DataSquaresTxt.substr(50000, 10000));
            CHECK(data[8] == DataSquaresTxt.substr(30000, 10000));
            CHECK(data[9] == DataIdentityBin.substr(50000, 5000));
        }

        { //download empty
            Downloader down;
            down.DownloadAll();
            int totalDownloaded = down.TotalBytesDownloaded();
            CHECK(totalDownloaded == 0);
        }

        { //download interruption
            Downloader down;
            std::string data1, data2, data3;
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "test.txt"), CreateDownloadCallback(data1));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "identity.bin"), CreateDownloadCallback(data2));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt"), CreateDownloadCallback(data3));
            double progressRatio = -1.0;
            int progressCnt = 0;
            int interruptCnt = 0;
            GlobalProgressCallback progressCallback = [&](double ratio, const char *message) -> int {
                progressRatio = ratio;
                progressCnt++;
                if (ratio >= 0.3) {
                    CHECK(interruptCnt == 0);
                    interruptCnt = progressCnt;
                    return 1;   //interrupt in the middle
                }
                return 0;
            };
            down.SetProgressCallback(progressCallback);
            try {
                down.DownloadAll();
                CHECK(false);
            } catch(const ErrorException &e) {
                CHECK(e.code() == lcUserInterrupt);
            }
            CHECK(progressRatio >= 0.3);
            CHECK(progressRatio < 1.0);
            CHECK(interruptCnt == progressCnt);
            int cntEmpty = int(data1.empty()) + int(data2.empty()) + int(data3.empty());
            CHECK(cntEmpty > 0);
        }
    }

    {   //test Downloader::SetMultipartBlocked
        HttpServer server;
        server.SetDropMultipart(true);
        server.SetRootDir(GetTempDir().string());
        server.Start();
        for (int nomultipart = 0; nomultipart < 2; nomultipart++) {
            Downloader down;
            std::string data1, data2, data3, data4;
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 10000, 10100), CreateDownloadCallback(data1));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 30000, 30200), CreateDownloadCallback(data2));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 20000, 20500), CreateDownloadCallback(data3));
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subdir/squares.txt", 10100, 10345), CreateDownloadCallback(data4));
            down.SetMultipartBlocked(nomultipart);
            if (nomultipart) {
                down.DownloadAll();
                CHECK(data1 == DataSquaresTxt.substr(10000, 100));
                CHECK(data2 == DataSquaresTxt.substr(30000, 200));
                CHECK(data3 == DataSquaresTxt.substr(20000, 500));
                CHECK(data4 == DataSquaresTxt.substr(10100, 245));
            }
            else {
                CHECK_THROWS(down.DownloadAll());
            }
        }
    }
}

//this bug was noticed after adding subtasks in Downloader
//i.e. when I made it possible to split single download into smaller chunks
TEST_CASE("DownloaderSubtasks") {
    char pattern[256];
    for (int i = 0; i < 256; i++)
        pattern[i] = i;
    static const int FILESIZE = 100<<20;
    {
        stdext::create_directories(GetTempDir());
        StdioFileHolder identity((GetTempDir() / "subtasks.bin").string().c_str(), "wb");
        for (int i = 0; i < FILESIZE; i += 256)
            fwrite(pattern, 256, 1, identity);
    }

    auto CreateDownloadCallback = [](std::string &buffer) -> DownloadFinishedCallback {
        return [&buffer](const void *ptr, uint32_t bytes) -> void {
            buffer.assign((char*)ptr, (char*)ptr + bytes);
        };
    };

    HttpServer server;
    server.SetRootDir(GetTempDir().string());
    server.Start();

    std::vector<std::string> data;
    data.reserve(1<<20);
    std::mt19937 rnd;

    for (int downloadSize = 10<<10; downloadSize <= 20<<20; ) {
        data.clear();
        Downloader down;
        for (int pos = 0; pos < FILESIZE; ) {
            int size = downloadSize;
            if (rnd() & 1)
                size += rnd() % (downloadSize * 2);
            else
                size -= rnd() % (2 * downloadSize / 3);
            size = std::min(size, FILESIZE - pos);
            data.push_back("");
            down.EnqueueDownload(DownloadSource(server.GetRootUrl() + "subtasks.bin", pos, pos + size), CreateDownloadCallback(data.back()));
            pos += size;
        }
        down.DownloadAll();
        std::string all;
        for (int i = 0; i < data.size(); i++)
            all += data[i];
        for (int i = 0; i < FILESIZE; i += 256)
            if (memcmp(pattern, all.data() + i, 256) != 0)
                CHECK(false);

        if (downloadSize < 5<<20)
            downloadSize *= 2;
        else {
            //the most important thing is to test ~10MB size
            //since that is the chunk size in the main speed profile of Downloader
            downloadSize += downloadSize / 3;
        }
    }
    //don't like 100MB files lying around...
    RemoveFile((GetTempDir() / "subtasks.bin").string());
}

TEST_CASE("DownloaderTimeout"
    * doctest::skip()   //takes hours due to repeated pauses
) {
    stdext::create_directories(GetTempDir());
    {
        StdioFileHolder identity((GetTempDir() / "identity_large.bin").string().c_str(), "wb");
        for (uint64_t i = 0; i < 1000000; i++)
            fwrite(&i, 8, 1, identity);
        StdioFileHolder numbers((GetTempDir() / "squares_large.txt").string().c_str(), "wb");
        for (int i = 0; i < 1000000; i++)
            fprintf(numbers, "%d-th square is %d\n", i, i*i);
    }
    std::string DataIdentityBin = ReadWholeFileAsStr((GetTempDir() / "identity_large.bin").string());
    std::string DataSquaresTxt = ReadWholeFileAsStr((GetTempDir() / "squares_large.txt").string());
    auto CreateDownloadCallback = [](std::string &buffer) -> DownloadFinishedCallback {
        return [&buffer](const void *ptr, uint32_t bytes) -> void {
            buffer.assign((char*)ptr, (char*)ptr + bytes);
        };
    };

    {   //connection timeout
        HttpServer server;
        server.SetRootDir(GetTempDir().string());
        server.StartButIgnoreConnections();
        Downloader down;
        std::string res;
        down.EnqueueDownload(
            DownloadSource(server.GetRootUrl() + "identity_large.bin", 0, 1000),
            CreateDownloadCallback(res)
        );
        CHECK_THROWS(down.DownloadAll());
    }

    for (int mode = 0; mode < 4; mode++) {
        HttpServer server;
        server.SetBlockSize(3 * 1024);
        server.SetRootDir(GetTempDir().string());

        //note: this tests depends a lot on SPEED_PROFILES in Downloader.cpp
        //the wait time and byte intervals should be kept in sync with them
        if (mode == 0) {
            //pause for 1 second every 1 MB
            //download should not be stopped (timeout = 10 seconds)
            server.SetPauseModel(HttpServer::PauseModel{1<<20, 1});
        }
        if (mode == 1) {
            //pause for 20 seconds every 2 MB
            //download should be retried with second profile a few times
            server.SetPauseModel(HttpServer::PauseModel{2<<20, 20});
        }
        if (mode == 2) {
            //pause for 20 seconds every 200 KB
            //download will proceed with the third profile (having timeout = 30 seconds)
            server.SetPauseModel(HttpServer::PauseModel{200<<10, 20});
        }
        if (mode == 3) {
            //pause for 100 seconds every 1 KB
            //download should be stopped, no profile supports it
            server.SetPauseModel(HttpServer::PauseModel{1<<10, 100});
        }

        server.Start();

        { //retry due to hanging connection
            Downloader down;
            static const int CHUNK_SIZE = 100<<10;
            std::vector<std::string> resId, resSq;
            resId.reserve(100000);
            resSq.reserve(100000);
            for (int req = 0; (req + 1) * CHUNK_SIZE <= DataIdentityBin.size(); req++) {
                resId.push_back({});
                down.EnqueueDownload(DownloadSource(
                    server.GetRootUrl() + "identity_large.bin",
                    CHUNK_SIZE * req, CHUNK_SIZE * (req + 1)),
                    CreateDownloadCallback(resId.back())
                );
            }
            for (int req = 0; (req + 1) * CHUNK_SIZE <= DataSquaresTxt.size(); req++) {
                resSq.push_back({});
                down.EnqueueDownload(DownloadSource(
                    server.GetRootUrl() + "squares_large.txt",
                    CHUNK_SIZE * req, CHUNK_SIZE * (req + 1)),
                    CreateDownloadCallback(resSq.back())
                );
            }

            g_testLogger->clear();
            if (mode < 3)
                down.DownloadAll();
            else {
                CHECK_THROWS(down.DownloadAll());
                continue;
            }
            int tooSlowCount = g_testLogger->counts[lcDownloadTooSlow];
            if (mode < 1)
                CHECK(tooSlowCount == 0);
            else
                CHECK(tooSlowCount > 0);

            for (int req = 0; req < resId.size(); req++)
                CHECK(resId[req] == DataIdentityBin.substr(CHUNK_SIZE * req, CHUNK_SIZE));
            for (int req = 0; req < resSq.size(); req++)
                CHECK(resSq[req] == DataSquaresTxt.substr(CHUNK_SIZE * req, CHUNK_SIZE));
        }
    }
}

TEST_CASE("CleanInstall") {
    //ensure no unnecessary zip repacks on clean install of something
    //even if some files are present in several provided locations / are duplicates
    for (int canRename = 0; canRename < 2; canRename++) {
        auto tempDir = GetTempDir() / ("ci" + std::to_string(canRename));
        TestCreator tc;
        auto params = tc.GenInZipParams();
        std::vector<std::vector<uint8_t>> fileContents;
        for (int i = 0; i < 100; i++) {
            fileContents.push_back(tc.GenFileContents());
            fileContents.back().push_back((uint8_t)i);
        }
        DirState state;
        for (int z = 0; z < 3; z++) {
            InZipState &zf = state["arch" + std::to_string(z) + ".zip"];
            for (int i = 0; i < 100; i++) {
                zf.emplace_back("file" + std::to_string(i) + ".tmp", InZipFile{params, fileContents[i]});
                //some duplicate files
                if (i % 11 == 10)
                    zf.emplace_back("11added" + std::to_string(i) + ".tmp", InZipFile{params, fileContents[i-z-5]});
                if (i % 9 == 7)
                    zf.emplace_back("9added" + std::to_string(i) + ".tmp", InZipFile{params, fileContents[i-2*z-2]});
            }
        }

        std::vector<DirState> prov(2);
        if (canRename)
            prov[0] = prov[1] = state;
        else
            tc.SplitState(state, prov);

        HttpServer servers[2];
        servers[0].SetRootDir((tempDir / "srcA").string());
        servers[1].SetRootDir((tempDir / "srcB").string());
        servers[0].SetPortNumber(8123);
        servers[0].Start();
        servers[1].Start();

        Manifest targetMani;
        TestCreator::WriteState((tempDir / "current").string(), "", state, &targetMani);
        Manifest providedMani;
        TestCreator::WriteState((tempDir / "srcA").string(), servers[0].GetRootUrl(), prov[0], &providedMani);
        TestCreator::WriteState((tempDir / "srcB").string(), servers[1].GetRootUrl(), prov[1], &providedMani);

        UpdateProcess updater;
        updater.Init(targetMani, providedMani, (tempDir / "current").string());
        bool ok = updater.DevelopPlan(UpdateType::SameContents);
        REQUIRE(ok);
        g_testLogger->clear();
        updater.DownloadRemoteFiles();
        updater.RepackZips();
        CHECK(g_testLogger->counts[lcRenameZipWithoutRepack] == (canRename ? 3 : 0));
        CHECK(g_testLogger->counts[lcRepackZip] == (canRename ? 0 : 3));

        Manifest resMani = updater.GetProvidedManifest().Filter([](const FileMetainfo &f) {
            return f.location == FileLocation::Inplace;
        });
        for (int i = 0; i < resMani.size(); i++) {
            CHECK(resMani[i].props.externalAttribs == params.externalAttribs);
            //CHECK(resMani[i].props.internalAttribs == params.internalAttribs); //changed when zip is being written
            CHECK(resMani[i].props.lastModTime == params.dosDate);
            CHECK(resMani[i].props.compressionMethod == params.method);
        }
    }
}

TEST_CASE("ChecksummedZip") {
    static const int NUM = 10;
    auto tempDir = GetTempDir() / "chkZip";
    stdext::create_directories(tempDir);

    TestCreator creator;
    std::vector<std::vector<uint8_t>> datas;
    for (int i = 0; i < NUM; i++) {
        int k = creator.RndInt(100000, 200000);
        int seed = creator.RndInt(0, INT_MAX);
        std::vector<uint8_t> data(k, 0);
        for (int j = 0; j < k; j++) {
            data[j] = ((seed >> 16) & 255);
            seed = seed * 1103515245 + 12345;
        }
        datas.push_back(std::move(data));
    }

    //write local file
    std::vector<PathAR> filePaths;
    for (int i = 0; i < NUM; i++) {
        std::string fn = "data" + std::to_string(i) + ".zip";
        filePaths.push_back(PathAR::FromRel(fn, tempDir.string()));
        WriteChecksummedZip(filePaths[i].abs.c_str(), datas[i].data(), datas[i].size(), "trashData.bin");
    }

    //read local file (correct)
    for (int i = 0; i < NUM; i++) {
        std::vector<uint8_t> readData = ReadChecksummedZip(filePaths[i].abs.c_str(), "trashData.bin");
        CHECK(readData == datas[i]);
    }

    //read checksums only
    std::vector<HashDigest> hashes;
    for (int i = 0; i < NUM; i++) {
        HashDigest hash = GetHashOfChecksummedZip(filePaths[i].abs.c_str());
        HashDigest expected = Hasher().Update(datas[i].data(), datas[i].size()).Finalize();
        CHECK(hash == expected);
        hashes.push_back(hash);
    }

    for (int t = 0; t < 2; t++) {
        //read local file (wrong)
        std::string path = (tempDir / ("wrongData0.zip")).string();
        {
            ZipFileHolder zf(path.c_str());
            std::string str = CHECKSUMMED_HASH_PREFIX + hashes[0].Hex();
            if (t == 1) {
                for (int i = strlen(CHECKSUMMED_HASH_PREFIX); i < str.size(); i++) {
                    char &ch = str[i];
                    ch = (isdigit(ch) ? 'a' : '0');
                }
            }
            SAFE_CALL(zipOpenNewFileInZip(zf, CHECKSUMMED_HASH_FILENAME, NULL, NULL, 0, NULL, 0, NULL, Z_NO_COMPRESSION, Z_NO_COMPRESSION));
            SAFE_CALL(zipWriteInFileInZip(zf, str.data(), str.size()));
            SAFE_CALL(zipCloseFileInZip(zf));
            SAFE_CALL(zipOpenNewFileInZip(zf, "trashData.bin", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION));
            SAFE_CALL(zipWriteInFileInZip(zf, datas[0].data(), datas[0].size()));
            SAFE_CALL(zipCloseFileInZip(zf));
        }
        if (t == 0)
            CHECK(ReadChecksummedZip(path.c_str(), "trashData.bin") == datas[0]);
        else
            CHECK_THROWS(ReadChecksummedZip(path.c_str(), "trashData.bin"));
    }

    HttpServer server;
    server.SetRootDir(tempDir.string());
    server.Start();
    std::string rootUrl = server.GetRootUrl();

    std::vector<std::string> allUrls;
    for (int i = 0; i < NUM; i++)
        allUrls.push_back(PathAR::FromRel(filePaths[i].rel, rootUrl).abs);

    //check remote hashes retrieval
    Downloader downloader0;
    std::vector<HashDigest> remoteHashes = GetHashesOfRemoteChecksummedZips(downloader0, allUrls);
    CHECK(remoteHashes == hashes);
    int downTotal0 = downloader0.TotalBytesDownloaded();
    CHECK(downTotal0 <= NUM * 256);

    std::vector<std::string> cacheFilePaths;
    std::vector<int> expectedMatching(NUM, -1);
    int sumDownloadSizes = 0;
    for (int i = 0; i < NUM; i++) {
        bool inCache = !creator.RndInt(0, 1);
        if (inCache || i == 3)
            cacheFilePaths.push_back(filePaths[i].abs);
        if (inCache && i != 3)
            expectedMatching[i] = cacheFilePaths.size() - 1;
        else
            sumDownloadSizes += datas[i].size();
    }
    std::vector<std::string> outPaths, outPathsOrig;
    for (int i = 0; i < NUM; i++)
        outPaths.push_back(PathAR::FromRel("out" + filePaths[i].rel, tempDir.string()).abs);
    outPathsOrig = outPaths;

    //check optimized download
    Downloader downloader1;
    remoteHashes[3].Clear();    //let's suppose it has no hash
    std::vector<int> matching = DownloadChecksummedZips(downloader1, allUrls, remoteHashes, cacheFilePaths, outPaths);
    CHECK(matching == expectedMatching);
    for (int i = 0; i < NUM; i++) {
        if (matching[i] < 0)
            CHECK(outPaths[i] == outPathsOrig[i]);
        else
            CHECK(outPaths[i] == cacheFilePaths[matching[i]]);
        std::vector<uint8_t> obtainedData = ReadChecksummedZip(outPaths[i].c_str(), "trashData.bin");
        CHECK(obtainedData == datas[i]);
    }
    int downTotal1 = downloader1.TotalBytesDownloaded();
    CHECK(downTotal1 <= sumDownloadSizes + NUM * 1024);
}

TEST_CASE("BugAsciiOrBinary") {
    TestCreator rnd;
    for (int dataAscii = 0; dataAscii < 2; dataAscii++) {
        for (int attrAscii = 0; attrAscii < 2; attrAscii++) {
            auto tempDir = GetTempDir() / "ascii" / (std::to_string(dataAscii) + std::to_string(attrAscii));
            stdext::create_directories(tempDir);

            zip_fileinfo info = {0};
            info.internal_fa = attrAscii;

            {
                ZipFileHolder zf((tempDir / "z.zip").string().c_str());
                SAFE_CALL(zipOpenNewFileInZip(zf, (dataAscii ? "temp.txt" : "temp.bin"), &info, 0, 0, 0, 0, 0, Z_DEFLATED, Z_BEST_COMPRESSION));
                char buff[64<<10] = {0};
                for (int i = 0; i < 50000; i++) {
                    if (dataAscii)
                        buff[i] = rnd.RndInt('a', 'z');
                    else
                        buff[i] = rnd.RndInt(0, 3) == 0 ? rnd.RndInt(0, 255) : rnd.RndInt(0, 100);
                }
                SAFE_CALL(zipWriteInFileInZip(zf, buff, 50000));
                //note: here is the fix for the problem --- force ASCII/binary type in zlib stream
                SAFE_CALL(zipForceDataType(zf, info.internal_fa));
                SAFE_CALL(zipCloseFileInZip(zf));
            }

            {
                UnzFileHolder zf((tempDir / "z.zip").string().c_str());
                SAFE_CALL(unzLocateFile(zf, (dataAscii ? "temp.txt" : "temp.bin"), 0));
                unz_file_info info = {0};
                SAFE_CALL(unzGetCurrentFileInfo(zf, &info, 0, 0, 0, 0, 0, 0));
                CHECK(info.internal_fa == attrAscii);
            }
        }
    }

}


TEST_CASE("FuzzLocal50") {
    Fuzz((GetTempDir() / "FL50").string(), 50, false);
}

TEST_CASE("FuzzRemote50") {
    Fuzz((GetTempDir() / "FR50").string(), 50, true);
}

TEST_CASE("FuzzLocalInfinite"
    * doctest::skip()
) {
    Fuzz((GetTempDir()).string(), -1, false);
}

TEST_CASE("FuzzRemoteInfinite"
    * doctest::skip()
) {
    Fuzz((GetTempDir()).string(), -1, true);
}

//=============================================================

int main(int argc, char** argv) {
    doctest::Context ctx(argc, argv);
    g_logger = g_testLogger;
    return ctx.run();
}
