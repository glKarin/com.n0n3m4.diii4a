#include "Manifest.h"
#include "Logging.h"
#include "StdString.h"
#include "Utils.h"
#include "ZipUtils.h"
#include "Path.h"
#include <tuple>
#include <map>
#include <functional>
#include <string.h>

#include "minizip_extra.h"


namespace ZipSync {

bool FileMetainfo::IsLess_ByZip(const FileMetainfo &a, const FileMetainfo &b) {
    #define WRAP_TUPLE(m) std::tie(                                                         \
        /*the main properties to sort by*/                                                  \
        m.zipPath.rel, m.filename,                                                          \
        /*the above props should always differ under normal use*/                           \
        /*but better provide tie-breaker to make order deterministic*/                      \
        /*and we need this in fuzzy tests (not a normal use of course)*/                    \
        m.contentsHash, m.compressedHash, m.package,                                        \
        m.props.lastModTime, m.props.compressionMethod, m.props.generalPurposeBitFlag,      \
        m.props.internalAttribs, m.props.externalAttribs, m.props.compressedSize,           \
        m.props.contentsSize, m.props.crc32,                                                \
        /*these come last: they are missing anyway in target manifests*/                    \
        m.byterange[0], m.byterange[1]                                                      \
    )
    return WRAP_TUPLE(a) < WRAP_TUPLE(b);
}

void FileMetainfo::Nullify() {
    byterange[0] = byterange[1] = 0;
    location = FileLocation::Nowhere;
    compressedHash.Clear();
    contentsHash.Clear();
    memset(&props, 0, sizeof(props));
}

void FileMetainfo::DontProvide() {
    byterange[0] = byterange[1] = 0;
    location = FileLocation::Nowhere;
}


void AnalyzeCurrentFile(unzFile zf, FileMetainfo &filemeta, bool hashContents, bool hashCompressed) {
    char filename[SIZE_PATH];
    unz_file_info info;
    SAFE_CALL(unzGetCurrentFileInfo(zf, &info, filename, sizeof(filename), NULL, 0, NULL, 0));

    ZipSyncAssertF(info.version == 0, "File %s has made-by version %d (not supported)", filename, info.version);
    ZipSyncAssertF(info.version_needed == 20, "File %s needs zip version %d (not supported)", filename, info.version_needed);
    ZipSyncAssertF((info.flag & 0x08) == 0, "File %s has data descriptor (not supported)", filename);
    ZipSyncAssertF((info.flag & 0x01) == 0, "File %s is encrypted (not supported)", filename);
    ZipSyncAssertF((info.flag & (~0x06)) == 0, "File %s has flags %d (not supported)", filename, info.flag);
    ZipSyncAssertF(info.compression_method == 0 || info.compression_method == 8, "File %s has compression %d (not supported)", filename, info.compression_method);
    ZipSyncAssertF(info.size_file_extra == 0, "File %s has extra field in header (not supported)", filename);
    ZipSyncAssertF(info.size_file_comment == 0, "File %s has comment in header (not supported)", filename);
    ZipSyncAssertF(info.disk_num_start == 0, "File %s has disk nonzero number (not supported)", filename);
    //TODO: check that extra field is empty in local file header?...

    filemeta.filename = filename;
    filemeta.props.crc32 = info.crc;
    filemeta.props.compressedSize = info.compressed_size;
    filemeta.props.contentsSize = info.uncompressed_size;
    filemeta.props.compressionMethod = info.compression_method;
    filemeta.props.generalPurposeBitFlag = info.flag;
    filemeta.props.lastModTime = info.dosDate;
    filemeta.props.internalAttribs = info.internal_fa;
    filemeta.props.externalAttribs = info.external_fa;
    unzGetCurrentFilePosition(zf, &filemeta.byterange[0], NULL, &filemeta.byterange[1]);

    for (int mode = 0; mode < 2; mode++) {
        if (!(mode == 0 ? hashCompressed : hashContents))
            continue;
        SAFE_CALL(unzOpenCurrentFile2(zf, NULL, NULL, !mode));

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
        HashDigest cmpHash = hasher.Finalize();

        SAFE_CALL(unzCloseCurrentFile(zf));

        if (mode == 0) {
            ZipSyncAssertF(processedBytes == filemeta.props.compressedSize, "File %s has wrong compressed size: %d instead of %d", filename, filemeta.props.compressedSize, processedBytes);
            filemeta.compressedHash = cmpHash;
        }
        else {
            ZipSyncAssertF(processedBytes == filemeta.props.contentsSize, "File %s has wrong uncompressed size: %d instead of %d", filename, filemeta.props.contentsSize, processedBytes);
            filemeta.contentsHash = cmpHash;
        }
    }
}

void AppendManifestsFromLocalZip(
    const std::string &zipPathAbs, const std::string &rootDir,
    FileLocation location,
    const std::string &packageName,
    Manifest &mani
) {
    PathAR zipPath = PathAR::FromAbs(zipPathAbs, rootDir);

    UnzFileHolder zf(zipPath.abs.c_str());
    ZipSyncAssertF(!unzIsZip64(zf), "Zip64 is not supported!");
    SAFE_CALL(unzGoToFirstFile(zf));
    while (1) {
        FileMetainfo filemeta;
        filemeta.zipPath = zipPath;
        filemeta.location = location;
        filemeta.package = packageName;

        AnalyzeCurrentFile(zf, filemeta);

        mani.AppendFile(filemeta);

        int err = unzGoToNextFile(zf);
        if (err == UNZ_END_OF_LIST_OF_FILE)
            break;
        SAFE_CALL(err);
    }
    zf.reset();
}
void Manifest::AppendLocalZip(const std::string &zipPath, const std::string &rootDir, const std::string &packageName) {
    ZipSyncAssert(PathAR::IsHttp(rootDir) == false);
    AppendManifestsFromLocalZip(zipPath, rootDir, FileLocation::Local, packageName, *this);
}

void Manifest::AppendManifest(const Manifest &other) {
    AppendVector(_files, other._files);
}

IniData Manifest::WriteToIni() const {
    //sort files by INI order
    std::vector<const FileMetainfo*> order;
    for (const auto &f : _files)
        order.push_back(&f);
    std::sort(order.begin(), order.end(), [](const FileMetainfo *a, const FileMetainfo *b) {
        return FileMetainfo::IsLess_ByZip(*a, *b);
    });

    IniData ini;
    for (const FileMetainfo *pf : order) {
        IniSect section;
        section.push_back(std::make_pair("contentsHash", pf->contentsHash.Hex()));
        section.push_back(std::make_pair("compressedHash", pf->compressedHash.Hex()));
        section.push_back(std::make_pair("byterange", std::to_string(pf->byterange[0]) + "-" + std::to_string(pf->byterange[1])));
        section.push_back(std::make_pair("package", pf->package));
        section.push_back(std::make_pair("crc32", std::to_string(pf->props.crc32)));
        section.push_back(std::make_pair("lastModTime", std::to_string(pf->props.lastModTime)));
        section.push_back(std::make_pair("compressionMethod", std::to_string(pf->props.compressionMethod)));
        section.push_back(std::make_pair("gpbitFlag", std::to_string(pf->props.generalPurposeBitFlag)));
        section.push_back(std::make_pair("compressedSize", std::to_string(pf->props.compressedSize)));
        section.push_back(std::make_pair("contentsSize", std::to_string(pf->props.contentsSize)));
        section.push_back(std::make_pair("internalAttribs", std::to_string(pf->props.internalAttribs)));
        section.push_back(std::make_pair("externalAttribs", std::to_string(pf->props.externalAttribs)));

        std::string secName = "File " + GetFullPath(pf->zipPath.rel, pf->filename);
        ini.push_back(std::make_pair(secName, std::move(section)));
    }

    return ini;
}
void Manifest::ReadFromIni(const IniData &data, const std::string &rootDir) {
    bool remote = PathAR::IsHttp(rootDir);

    for (const auto &pNS : data) {
        FileMetainfo pf;
        pf.location = (remote ? FileLocation::RemoteHttp : FileLocation::Local);

        std::string name = pNS.first;
        if (!stdext::starts_with(name, "File "))
            continue;
        name = name.substr(5);
        const IniSect &sec = pNS.second;

        ParseFullPath(name, pf.zipPath.rel, pf.filename);
        pf.zipPath = PathAR::FromRel(pf.zipPath.rel, rootDir);

        //note: since nobody would ever write manifest by hand
        //here we rely on order of properties as written in WriteToIni
        int propIdx = 0;
        auto ReadProperty = [&sec,&propIdx](const char *key) -> const std::string& {
            ZipSyncAssertF(propIdx < sec.size(), "No property while %s expected", key);
            ZipSyncAssertF(sec[propIdx].first == key, "Expected property %s, got %s", key, sec[propIdx].first.c_str());
            return sec[propIdx++].second;
        };

        pf.contentsHash.Parse(ReadProperty("contentsHash").c_str());
        pf.compressedHash.Parse(ReadProperty("compressedHash").c_str());

        const std::string &byterange = ReadProperty("byterange");
        size_t pos = byterange.find('-');
        ZipSyncAssertF(pos != std::string::npos, "Byterange %s has no hyphen", byterange.c_str());
        pf.byterange[0] = std::stoul(byterange.substr(0, pos));
        pf.byterange[1] = std::stoul(byterange.substr(pos+1));
        if (pf.byterange[0] || pf.byterange[1]) {
            ZipSyncAssert(pf.byterange[0] < pf.byterange[1]);
        }
        else
            pf.location = FileLocation::Nowhere;
        pf.package = ReadProperty("package");
        pf.props.crc32 = std::stoul(ReadProperty("crc32"));
        pf.props.lastModTime = std::stoul(ReadProperty("lastModTime"));
        pf.props.compressionMethod = std::stoul(ReadProperty("compressionMethod"));
        pf.props.generalPurposeBitFlag = std::stoul(ReadProperty("gpbitFlag"));
        pf.props.compressedSize = std::stoul(ReadProperty("compressedSize"));
        pf.props.contentsSize = std::stoul(ReadProperty("contentsSize"));
        pf.props.internalAttribs = std::stoul(ReadProperty("internalAttribs"));
        pf.props.externalAttribs = std::stoul(ReadProperty("externalAttribs"));

        AppendFile(pf);
    }
}

void Manifest::ReRoot(const std::string &rootDir) {
    bool remote = PathAR::IsHttp(rootDir);
    for (FileMetainfo &filemeta : _files) {
        filemeta.zipPath = PathAR::FromRel(filemeta.zipPath.rel, rootDir);
        if (filemeta.location != FileLocation::Nowhere)
            filemeta.location = (remote ? FileLocation::RemoteHttp : FileLocation::Local);
    }
}

Manifest Manifest::Filter(const std::function<bool(const FileMetainfo&)> &ifCopy) const {
    Manifest res;
    for (int i = 0; i < _files.size(); i++)
        if (ifCopy(_files[i]))
            res.AppendFile(_files[i]);
    return res;
}

}
