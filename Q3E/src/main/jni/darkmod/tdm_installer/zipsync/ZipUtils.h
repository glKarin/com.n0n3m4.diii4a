#pragma once

#include <memory>
#include <vector>
#include "Logging.h"

#include <minizip/unzip.h>
#include <minizip/zip.h>


namespace ZipSync {

typedef std::unique_ptr<std::remove_pointer<unzFile>::type, int (*)(unzFile)> UnzFileUniquePtr;
/**
 * RAII wrapper around unzFile from minizip.h.
 * Automatically closes file on destruction.
 */
class UnzFileHolder : public UnzFileUniquePtr {
public:
    UnzFileHolder(unzFile zf);
    UnzFileHolder(const char *path);
    ~UnzFileHolder();
    operator unzFile() const { return get(); }
};

typedef std::unique_ptr<std::remove_pointer<zipFile>::type, int (*)(zipFile)> ZipFileUniquePtr;
/**
 * RAII wrapper around zipFile from minizip.h.
 * Automatically closes file on destruction.
 */
class ZipFileHolder : public ZipFileUniquePtr {
public:
    ZipFileHolder(zipFile zf);
    ZipFileHolder(const char *path);
    ~ZipFileHolder();
    operator zipFile() const { return get(); }
};


/**
 * Performs whatever call you wrap into it and checks its return code.
 * If the return code is nonzero, then exception is thrown.
 */
#define SAFE_CALL(...) \
    do { \
        int mz_errcode = __VA_ARGS__; \
        if (mz_errcode != 0) g_logger->errorf(lcMinizipError, "Minizip error %d", mz_errcode); \
    } while (0)


//note: file must be NOT opened
void unzGetCurrentFilePosition(unzFile zf, uint32_t *localHeaderStart, uint32_t *fileDataStart, uint32_t *fileDataEnd);

class UnzFileIndexed {
    struct Entry {
        uint32_t byterangeStart;
        unz_file_pos unzPos;
        bool operator< (const Entry &b) const;
    };
    UnzFileUniquePtr _zfHandle;
    std::vector<Entry> _sortedEntries;
public:
    ~UnzFileIndexed();
    UnzFileIndexed();
    UnzFileIndexed(UnzFileIndexed &&) = default;
    operator unzFile() const { return _zfHandle.get(); }
    void Clear();
    void Open(const char *path);
    void LocateByByterange(uint32_t start, uint32_t end);
};

/*
//like unzLocateFile, but also checks exact match by byterange (which includes local file header)
bool unzLocateFileAtBytes(unzFile zf, const char *filename, uint32_t from, uint32_t to);
*/

void minizipCopyFile(unzFile zf, zipFile zfOut, const char *filename, int method, int flags, uint16_t internalAttribs, uint32_t externalAttribs, uint32_t dosDate, bool copyRaw, uint32_t crc, uint32_t contentsSize);

struct FileAttribInfo {
    uint32_t offset;
    uint32_t externalAttribs;
    uint16_t internalAttribs;
};
//given a tightly packed zip file without central directory, rebuilds it and appends it to the end of file
void minizipAddCentralDirectory(const char *filename, std::vector<FileAttribInfo> attribs = {});

//repack given zip file so that it gets accepted by ZipSync
void minizipNormalize(const char *srcFilename, const char *dstFilename = NULL);

}
