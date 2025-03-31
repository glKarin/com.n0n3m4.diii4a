#include "Utils.h"
#include "Logging.h"


namespace ZipSync {

StdioFileHolder::~StdioFileHolder()
{}
StdioFileHolder::StdioFileHolder(FILE *f)
    : StdioFileUniquePtr(f, fclose)
{}
StdioFileHolder::StdioFileHolder(const char *path, const char *mode)
    : StdioFileUniquePtr(fopen(path, mode), fclose)
{
    if (!get())
        g_logger->errorf(lcCantOpenFile, "Failed to open file \"%s\"", path);
}

std::vector<uint8_t> ReadWholeFile(const std::string &filename) {
    StdioFileHolder f(filename.c_str(), "rb");
    fseek(f.get(), 0, SEEK_END);
    int size = ftell(f.get());
    fseek(f.get(), 0, SEEK_SET);
    std::vector<uint8_t> res;
    res.resize(size);
    int numRead = fread(res.data(), 1, size, f.get());
    ZipSyncAssert(numRead == size);
    return res;
}

int GetFileSize(const std::string &filename) {
    StdioFileHolder f(filename.c_str(), "rb");
    fseek(f.get(), 0, SEEK_END);
    int size = ftell(f.get());
    return size;
}

}
