#pragma once

#include <stdint.h>
#include <vector>
#include <functional>
#include "Path.h"
#include "Hash.h"
#include "Ini.h"
#include "ZipUtils.h"

namespace ZipSync {

/**
 * Information about file properties inside zip.
 * Enough to exactly reproduce the zip file header.
 */
struct FileZipProps {
    //(contents of zip file central header follows)
    //  version made by                 2 bytes  (minizip: 0)
    //  version needed to extract       2 bytes  (minizip: 20 --- NO zip64!)
    //  general purpose bit flag        2 bytes  ???  [0|2|4|6]
    //  compression method              2 bytes  ???  [0|8]
    //  last mod file time              2 bytes  ???
    //  last mod file date              2 bytes  ???
    //  crc-32                          4 bytes  (defined from contents --- checked by minizip)
    //  compressed size                 4 bytes  (defined from contents --- checked by me)
    //  uncompressed size               4 bytes  (defined from contents --- checked by me)
    //  filename length                 2 bytes  ???
    //  extra field length              2 bytes  (minizip: 0)
    //  file comment length             2 bytes  (minizip: 0)
    //  disk number start               2 bytes  (minizip: 0)
    //  internal file attributes        2 bytes  ???
    //  external file attributes        4 bytes  ???
    //  relative offset of local header 4 bytes  (dependent on file layout)
    //  filename (variable size)        ***      ???
    //  extra field (variable size)     ***      (minizip: empty)
    //  file comment (variable size)    ***      (minizip: empty)

    //last modification time in DOS format
    uint32_t lastModTime;
    //compression method
    uint16_t compressionMethod;
    //compression settings for DEFLATE algorithm
    uint16_t generalPurposeBitFlag;
    //internal attributes
    uint16_t internalAttribs;
    //external attributes
    uint32_t externalAttribs;
    //size of compressed file (excessive)
    //note: local file header EXcluded
    uint32_t compressedSize;
    //size of uncompressed file (needed when writing in RAW mode)
    uint32_t contentsSize;
    //CRC32 checksum of uncompressed file (needed when writing in RAW mode)
    uint32_t crc32;
};

/**
 * Where is the file described by metainfo located.
 */
enum class FileLocation {
    Inplace = 0,    //local zip file in the place where it should be
    Local = 1,      //local zip file (e.g. inside local cache of old versions)
    RemoteHttp = 2, //file remotely available via HTTP 1.1+
    Nowhere,        //file data is not available
    Repacked,       //internal: file is on its place in "repacked" zip (not yet renamed back)
    Reduced,        //internal: file is in "reduced" zip, to be moved to cache later
};

/**
 * Metainformation about a file, which is manipulated by updater.
 * It serves two purposes:
 *   1. in target manifest: describes a file which must be present according to selected target package after update
 *   2. in provided manifest: describes a file provided at some location, available for reading/downloading
 */
struct FileMetainfo {
    //file/url path to the zip archive
    PathAR zipPath;
    //filename inside zip
    std::string filename;

    //in target manifest: Nowhere
    //in provided manifest: Local/RemoteHttp
    //the update algorithm uses other values
    FileLocation location;
    //range of bytes in the zip representing the file
    //makes sense only if the file is actually provided
    //note: local file header INcluded
    uint32_t byterange[2];

    //name of the target package it belongs to
    //"target package" = a set of files which must be installed (several packages may be chosen)
    std::string package;
    //what should be written to the file header in zip
    FileZipProps props;

    //hash of the contents of uncompressed file
    HashDigest contentsHash;
    //hash of the compressed file
    //note: local file header EXcluded
    HashDigest compressedHash;

    static bool IsLess_ByZip(const FileMetainfo &a, const FileMetainfo &b);
    void Nullify();
    void DontProvide();
};

/**
 * Manifest describes a set of files, and stores metainfo for each of these files.
 * For update, one manifest is chosen as "target" (desired result), and several manifests "provide" files for copy/download.
 * Update is possible if provided manifests together cover the requirements of the target manifest.
 * While single manifest file can represent both target set and provided set, they are always split before update.
 */
class Manifest {
    //arbitrary text attached to the manifest (only for debugging)
    std::string _comment;

    //the set of files described by this manifest
    std::vector<FileMetainfo> _files;

public:
    const std::string &GetComment() const { return _comment; }
    void SetComment(const std::string &text) { _comment = text; }

    int size() const { return _files.size(); }
    const FileMetainfo &operator[](int index) const { return _files[index]; }
    FileMetainfo &operator[](int index) { return _files[index]; }

    void Clear() { _files.clear(); }
    void AppendFile(const FileMetainfo &file) { _files.push_back(file); }
    void AppendManifest(const Manifest &other);
    void AppendLocalZip(const std::string &zipPath, const std::string &rootDir, const std::string &packageName);

    void ReadFromIni(const IniData &data, const std::string &rootDir);
    IniData WriteToIni() const;

    void ReRoot(const std::string &rootDir);
    Manifest Filter(const std::function<bool(const FileMetainfo&)> &ifCopy) const;
};

/**
 * Iterator to a file in a manifest.
 * Appends do NOT invalidate it.
 */
struct ManifestIter {
    Manifest *_manifest;
    int _index;

    ManifestIter() : _manifest(nullptr), _index(0) {}
    ManifestIter(Manifest &manifest, int index) : _manifest(&manifest), _index(index) {}
    ManifestIter(Manifest &manifest, const FileMetainfo *file) {
        if (file) {
            _manifest = &manifest;
            _index = file - &manifest[0];
            ZipSyncAssert(_index >= 0 && _index < manifest.size());
        }
        else {
            _manifest = nullptr;
            _index = 0;
        }
    }
    FileMetainfo& operator*() const { return (*_manifest)[_index]; }
    FileMetainfo* operator->() const { return &(*_manifest)[_index]; }
    explicit operator bool() const { return _manifest != nullptr; }
    FileMetainfo *get () const { return &(*_manifest)[_index]; }
};


//sets all properties except for:
//  zipPath
//  location
//  package
//  contentsHash (if hashContents = false)
//  compressedHash (if hashCompressed = false)
void AnalyzeCurrentFile(unzFile zf, FileMetainfo &target, bool hashContents = true, bool hashCompressed = true);

//creates manifest for local zip, serving both as target and provided
void AppendManifestsFromLocalZip(
    const std::string &zipPath, const std::string &rootDir,             //path to local zip (both absolute?)
    FileLocation location,                                              //for provided
    const std::string &packageName,                                     //for target
    Manifest &mani                                                      //output
);

}
