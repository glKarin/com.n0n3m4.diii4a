#include "ZipUtils.h"
#include "Utils.h"
#include "Path.h"
#include <algorithm>
#include "minizip_extra.h"
#include <string.h>


namespace ZipSync {

int zipCloseNoComment(zipFile zf) {
    return zipClose(zf, NULL);
}

UnzFileHolder::~UnzFileHolder()
{}
UnzFileHolder::UnzFileHolder(unzFile zf)
    : UnzFileUniquePtr(zf, unzClose)
{}
UnzFileHolder::UnzFileHolder(const char *path)
    : UnzFileUniquePtr(unzOpen(path), unzClose)
{
    if (!get())
        g_logger->errorf(lcCantOpenFile, "Failed to open zip file \"%s\"", path);
}

ZipFileHolder::~ZipFileHolder()
{}
ZipFileHolder::ZipFileHolder(zipFile zf)
    : ZipFileUniquePtr(zf, zipCloseNoComment) 
{}
ZipFileHolder::ZipFileHolder(const char *path)
    : ZipFileUniquePtr(nullptr, zipCloseNoComment)
{
    //allow to overwrite
    if (IfFileExists(path))
        RemoveFile(path);
    reset(zipOpen(path, 0));
    if (!get())
        g_logger->errorf(lcCantOpenFile, "Failed to open zip file \"%s\"", path);
}

void unzGetCurrentFilePosition(unzFile zf, uint32_t *localHeaderStart, uint32_t *fileDataStart, uint32_t *fileDataEnd) {
    unz_file_info info;
    SAFE_CALL(unzGetCurrentFileInfo(zf, &info, NULL, 0, NULL, 0, NULL, 0));
    SAFE_CALL(unzOpenCurrentFile(zf));
    int64_t pos = unzGetCurrentFileZStreamPos64(zf);
    int localHeaderSize = 30 + info.size_filename + info.size_file_extra;
    if (localHeaderStart)
        *localHeaderStart = pos - localHeaderSize;
    if (fileDataStart)
        *fileDataStart = pos;
    if (fileDataEnd)
        *fileDataEnd = pos + info.compressed_size;
    SAFE_CALL(unzCloseCurrentFile(zf));
}

bool UnzFileIndexed::Entry::operator< (const UnzFileIndexed::Entry &b) const {
    return byterangeStart < b.byterangeStart;
}
UnzFileIndexed::~UnzFileIndexed() {}
UnzFileIndexed::UnzFileIndexed() : _zfHandle(0, unzClose) {}
void UnzFileIndexed::Clear() {
    _zfHandle.reset();
    _sortedEntries.clear();
}
void UnzFileIndexed::Open(const char *path) {
    unzFile zf = unzOpen(path);
    if (!zf)
        g_logger->errorf(lcCantOpenFile, "Failed to open zip file \"%s\"", path);
    _zfHandle.reset(zf);
    _sortedEntries.clear();
    SAFE_CALL(unzGoToFirstFile(zf));
    while (1) {
        char currFilename[SIZE_PATH];
        SAFE_CALL(unzGetCurrentFileInfo(zf, NULL, currFilename, sizeof(currFilename), NULL, 0, NULL, 0));
        uint32_t from, to;
        unzGetCurrentFilePosition(zf, &from, NULL, &to);
        Entry e;
        e.byterangeStart = from;
        SAFE_CALL(unzGetFilePos(zf, &e.unzPos));
        _sortedEntries.push_back(e);
        int res = unzGoToNextFile(zf);
        if (res == UNZ_END_OF_LIST_OF_FILE)
            break;   //finished
        SAFE_CALL(res);
    }
    if (!std::is_sorted(_sortedEntries.begin(), _sortedEntries.end()))
        std::sort(_sortedEntries.begin(), _sortedEntries.end());
}
void UnzFileIndexed::LocateByByterange(uint32_t start, uint32_t end) {
    Entry aux;
    aux.byterangeStart = start;
    int idx = std::lower_bound(_sortedEntries.begin(), _sortedEntries.end(), aux) - _sortedEntries.begin();
    ZipSyncAssertF(idx < _sortedEntries.size() && _sortedEntries[idx].byterangeStart == start, "Failed to find file at byterange [%u..%u]", start, end);
    SAFE_CALL(unzGoToFilePos(_zfHandle.get(), &_sortedEntries[idx].unzPos));
}


bool unzLocateFileAtBytes(unzFile zf, const char *filename, uint32_t from, uint32_t to) {
    SAFE_CALL(unzGoToFirstFile(zf));
    while (1) {
        char currFilename[SIZE_PATH];
        SAFE_CALL(unzGetCurrentFileInfo(zf, NULL, currFilename, sizeof(currFilename), NULL, 0, NULL, 0));
        if (strcmp(filename, currFilename) == 0) {
            uint32_t currFrom, currTo;
            unzGetCurrentFilePosition(zf, &currFrom, NULL, &currTo);
            if (currFrom == from && currTo == to)
                return true;    //hit
            //miss: only happens if two files have same name (e.g. tdm_update_2.06_to_2.07.zip)
            currFrom = currFrom; //noop
        }
        int res = unzGoToNextFile(zf);
        if (res == UNZ_END_OF_LIST_OF_FILE)
            return false;   //not found
        SAFE_CALL(res);
    }
}


int CompressionLevelFromGpFlags(int flags) {
    int compressionLevel = Z_DEFAULT_COMPRESSION;
    if (flags == 2)
        compressionLevel = Z_BEST_COMPRESSION;  //minizip: 8,9
    if (flags == 4)
        compressionLevel = 2;                   //minizip: 2
    if (flags == 6)
        compressionLevel = Z_BEST_SPEED;        //minizip: 1
    return compressionLevel;
}
void minizipCopyFile(unzFile zf, zipFile zfOut, const char *filename, int method, int flags, uint16_t internalAttribs, uint32_t externalAttribs, uint32_t dosDate, bool copyRaw, uint32_t crc, uint32_t contentsSize) {
    //copy provided file data into target file
    SAFE_CALL(unzOpenCurrentFile2(zf, NULL, NULL, copyRaw));
    zip_fileinfo info;
    info.internal_fa = internalAttribs;
    info.external_fa = externalAttribs;
    info.dosDate = dosDate;
    int level = CompressionLevelFromGpFlags(flags);
    SAFE_CALL(zipOpenNewFileInZip2(zfOut, filename, &info, NULL, 0, NULL, 0, NULL, method, level, copyRaw));
    char buffer[SIZE_FILEBUFFER];
    if (copyRaw) {
        //faster than minizip copy: no CRC32 calculation, large buffer
        minizipCopyDataRaw(zf, zfOut, buffer, sizeof(buffer));
    }
    else {
        while (1) {
            int bytes = unzReadCurrentFile(zf, buffer, sizeof(buffer));
            if (bytes < 0)
                SAFE_CALL(bytes);
            if (bytes == 0)
                break;
            SAFE_CALL(zipWriteInFileInZip(zfOut, buffer, bytes));
        }
    }
    if (!copyRaw)
        SAFE_CALL(zipForceDataType(zfOut, info.internal_fa));
    SAFE_CALL(copyRaw ? zipCloseFileInZipRaw(zfOut, contentsSize, crc) : zipCloseFileInZip(zfOut));
    SAFE_CALL(unzCloseCurrentFile(zf));
}

#pragma pack(push, 1)
struct ZipLocalHeader {
    uint32_t magic;
    uint16_t versionNeeded;
    uint16_t flag;
    uint16_t compMethod;
    uint32_t timeDate;
    uint32_t crc32;
    uint32_t compSize;
    uint32_t uncompSize;
    uint16_t filenameLen;
    uint16_t extraLen;
};
struct ZipCentralHeader {
    uint32_t magic;
    uint16_t versionMade;
    uint16_t versionNeeded;
    uint16_t flag;
    uint16_t compMethod;
    uint32_t timeDate;
    uint32_t crc32;
    uint32_t compSize;
    uint32_t uncompSize;
    uint16_t filenameLen;
    uint16_t extraLen;
    uint16_t commentLen;
    uint16_t diskNum;
    uint16_t internalAttr;
    uint32_t externalAttr;
    uint32_t offset;
};
struct ZipEndOfCentral {
    uint32_t magic;
    uint16_t thisDiskNum;
    uint16_t startDiskNum;
    uint16_t numCentralHeaders;
    uint16_t totalCentralHeaders;
    uint32_t centralDirSize;
    uint32_t offset;
    uint16_t commentLen;
};
#pragma pack(pop)
void minizipAddCentralDirectory(const char *zipFilename, std::vector<FileAttribInfo> attribs) {
    std::sort(attribs.begin(), attribs.end(), [](const FileAttribInfo &a, const FileAttribInfo &b) { return a.offset < b.offset; });
    StdioFileHolder f(zipFilename, "r+b");
    std::vector<ZipCentralHeader> headers;
    std::vector<std::string> filenames;
    ZipLocalHeader lh;
    int attrIdx = 0;
    while (fread(&lh, sizeof(lh), 1, f) == 1) {
        uint32_t offset = ftell(f) - sizeof(lh);
        ZipSyncAssert(lh.magic == 0x04034b50);
        ZipSyncAssert(lh.extraLen == 0);
        std::unique_ptr<char[]> filename(new char[lh.filenameLen]);
        ZipSyncAssert(fread(filename.get(), lh.filenameLen, 1, f) == 1);
        ZipCentralHeader ch = {0};
        ch.magic = 0x02014b50;
        memcpy(&ch.versionNeeded, &lh.versionNeeded, sizeof(ZipLocalHeader) - offsetof(ZipLocalHeader, versionNeeded));
        ch.offset = offset;
        while (attrIdx < attribs.size() && attribs[attrIdx].offset < offset)
            attrIdx++;
        if (attrIdx < attribs.size() && attribs[attrIdx].offset == offset) {
            ch.externalAttr = attribs[attrIdx].externalAttribs;
            ch.internalAttr = attribs[attrIdx].internalAttribs;
        }
        headers.push_back(ch);
        filenames.push_back(std::string(filename.get(), filename.get() + lh.filenameLen));
        ZipSyncAssert(fseek(f, lh.compSize, SEEK_CUR) == 0);
    }
    fseek(f, 0, SEEK_END);
    size_t centralOffset = ftell(f);
    for (int i = 0; i < headers.size(); i++) {
        fwrite(&headers[i], sizeof(headers[i]), 1, f);
        fwrite(filenames[i].c_str(), 1, filenames[i].size(), f);
    }
    ZipEndOfCentral eocd = {0};
    eocd.magic = 0x06054b50;
    eocd.numCentralHeaders = eocd.totalCentralHeaders = headers.size();
    eocd.centralDirSize = ftell(f) - centralOffset;
    eocd.offset = centralOffset;
    fwrite(&eocd, sizeof(eocd), 1, f);
}

//note: see AnalyzeCurrentFile in Manifest.cpp for exact requirements
void minizipNormalize(const char *srcFilename, const char *dstFilename) {
    if (!dstFilename)
        dstFilename = srcFilename;

    std::string tempFilename = PrefixFile(dstFilename, "__normalized__");
    struct FileLocation {
        std::string filename;
        uint32_t range[2];
        bool operator< (const FileLocation &b) const {
            return std::make_pair(filename, range[0]) < std::make_pair(b.filename, b.range[0]);
        }
    };
    std::vector<FileLocation> files;

    UnzFileIndexed zfIn;
    zfIn.Open(srcFilename);
    SAFE_CALL(unzGoToFirstFile(zfIn));
    while (1) {
        char filename[SIZE_PATH];
        unz_file_info info;
        SAFE_CALL(unzGetCurrentFileInfo(zfIn, &info, filename, SIZE_PATH, NULL, 0, NULL, 0));
        FileLocation floc;
        uint32_t temp;
        unzGetCurrentFilePosition(zfIn, &floc.range[0], &temp, &floc.range[1]);
        floc.filename = filename;
        int len = strlen(filename);
        bool isDirectory = info.uncompressed_size == 0 && info.compression_method == 0 && ((info.external_fa & 16) || (len > 0 && filename[len-1] == '/'));
        if (!isDirectory)
            files.push_back(floc);

        int err = unzGoToNextFile(zfIn);
        if (err == UNZ_END_OF_LIST_OF_FILE)
            break;
        SAFE_CALL(err);
    }

    std::stable_sort(files.begin(), files.end());

    if (IfFileExists(tempFilename))
        RemoveFile(tempFilename);
    ZipFileHolder zfOut(tempFilename.c_str());
    for (const FileLocation &f : files) {
        zfIn.LocateByByterange(f.range[0], f.range[1]);

        unz_file_info infoIn;
        SAFE_CALL(unzGetCurrentFileInfo(zfIn, &infoIn, NULL, 0, NULL, 0, NULL, 0));
        ZipSyncAssertF(infoIn.compression_method == 0 || infoIn.compression_method == 8, "File %s has compression %d (not supported)", f.filename.c_str(), infoIn.compression_method);
        ZipSyncAssertF((infoIn.flag & (~0x06)) == 0, "File %s has flags %d (not supported)", f.filename.c_str(), infoIn.flag);
        ZipSyncAssertF((infoIn.internal_fa & (~0x01)) == 0, "File %s has internal attribs %d (not supported)", f.filename.c_str(), infoIn.internal_fa);

        SAFE_CALL(unzOpenCurrentFile2(zfIn, NULL, NULL, true));
        zip_fileinfo infoOut;
        infoOut.dosDate = infoIn.dosDate;
        infoOut.internal_fa = infoIn.internal_fa;
        infoOut.external_fa = infoIn.external_fa & 0xFF;    //drop anything except for lower byte (which has MS-DOS attribs)
        int level = CompressionLevelFromGpFlags(infoIn.flag);
        SAFE_CALL(zipOpenNewFileInZip2(zfOut, f.filename.c_str(), &infoOut, NULL, 0, NULL, 0, NULL, infoIn.compression_method, level, true));
        char buffer[SIZE_FILEBUFFER];
        //faster than minizip copy: no CRC32 calculation, large buffer
        minizipCopyDataRaw(zfIn, zfOut, buffer, sizeof(buffer));
        SAFE_CALL(zipCloseFileInZipRaw(zfOut, infoIn.uncompressed_size, infoIn.crc));
        SAFE_CALL(unzCloseCurrentFile(zfIn));
    }
    zfIn.Clear();
    zfOut.reset();

    if (IfFileExists(dstFilename))
        RemoveFile(dstFilename);
    RenameFile(tempFilename, dstFilename);
}

}
