#pragma once

#include <memory>
#include <vector>
#include <string>


namespace ZipSync {

static const int SIZE_PATH = 4<<10;
static const int SIZE_FILEBUFFER = 64<<10;
static const int SIZE_LINEBUFFER = 16<<10;


template<class T> void AppendVector(std::vector<T> &dst, const std::vector<T> &src) {
    dst.insert(dst.end(), src.begin(), src.end());
}


typedef std::unique_ptr<FILE, int (*)(FILE*)> StdioFileUniquePtr;
/**
 * RAII wrapper around FILE* from stdio.h.
 * Automatically closes file on destruction.
 */
class StdioFileHolder : public StdioFileUniquePtr {
public:
    StdioFileHolder(FILE *f);
    StdioFileHolder(const char *path, const char *mode);    //checks that file is opened successfully
    ~StdioFileHolder();
    StdioFileHolder(StdioFileHolder&&) = default;
    StdioFileHolder& operator=(StdioFileHolder&&) = default;
    operator FILE*() const { return get(); }
};

std::vector<uint8_t> ReadWholeFile(const std::string &filename);
int GetFileSize(const std::string &filename);

}
