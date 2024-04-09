#include "StdFilesystem.h"

#if defined(_MSC_VER) && _MSC_VER < 1910
    //STL-based implementation for MSVC2013
    #include <filesystem>
    namespace stdfsys = std::tr2::sys;
    //TODO: support later versions of MSVC
#else
    //it should be here for both GCC and Clang
    //MSVC2017 is also OK with it
    #if _HAS_CXX17
        #include <filesystem>
        namespace stdfsys = std::filesystem;
    #else
        #define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
        #include <experimental/filesystem>
        namespace stdfsys = std::experimental::filesystem;
    #endif
#endif

namespace stdext {
    struct path_impl : public stdfsys::path {
        path_impl() : stdfsys::path() {}
        path_impl(const char* str) : stdfsys::path(str) {}
        path_impl(const std::string &str) : stdfsys::path(str) {}
        path_impl(const stdfsys::path &path) : stdfsys::path(path) {}
    };
    path_impl &get(path &path) {
        return *path.d;
    }
    const path_impl &get(const path &path) {
        return *path.d;
    }

    path::~path() {}
    path::path() : d(new path_impl()) {}
    path::path(const path& path) : d(new path_impl(*path.d)) {}
    path& path::operator= (const path &path) {
        if (this != &path)
            d.reset(new path_impl(*path.d));
        return *this;
    }
    path::path(path&& path) : d(std::move(path.d)) {}
    path& path::operator= (path &&path) {
        d = std::move(path.d);
        return *this;
    }
    path::path(const path_impl& impl) : d(new path_impl(impl)) {}
    path::path(const char *source) : d(new path_impl(source)) {}
    path::path(const std::string &source) : d(new path_impl(source)) {}
    std::string path::string() const {
        std::string res = d->string();
#ifdef _MSC_VER
        std::replace(res.begin(), res.end(), '\\', '/');
#endif
        return res;
    }
    bool path::empty() const {
        return d->empty();
    }
    path path::parent_path() const {
        return path_impl(d->parent_path());
    }
    path path::filename() const {
        return path_impl(d->filename());
    }
    path path::extension() const {
        return path_impl(d->extension());
    }
    path path::stem() const {
        return path_impl(d->stem());
    }
    bool path::is_absolute() const {
        return d->is_absolute();
    }
    path& path::remove_filename() {
        d->remove_filename();
        return *this;
    }
    path operator/(const path& lhs, const path& rhs) {
        return path_impl(get(lhs) / get(rhs));
    }
    path& operator/=(path& lhs, const path& rhs) {
        get(lhs) /= get(rhs);
        return lhs;
    }
    bool operator==(const path& lhs, const path& rhs) {
        return get(lhs) == get(rhs);
    }
    bool operator!=(const path& lhs, const path& rhs) {
        return get(lhs) != get(rhs);
    }
    bool operator<(const path& lhs, const path& rhs) {
        return get(lhs) < get(rhs);
    }

    filesystem_error::filesystem_error(const char *what_arg, std::error_code ec) : std::system_error(ec, what_arg) {}

    bool is_directory(const path &path) {
        bool res;
        try {   res = stdfsys::is_directory(get(path)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    bool is_regular_file(const path &path) {
        bool res;
        try {   res = stdfsys::is_regular_file(get(path)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    bool create_directory(const path &path) {
        bool res;
        try {   res = stdfsys::create_directory(get(path)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    bool exists(const path &path) {
        bool res;
        try {   res = stdfsys::exists(get(path)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    bool create_directories(const path &path) {
        bool res;
        try {   res = stdfsys::create_directories(get(path)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    bool remove(const path &path) {
        bool res;
        try {   res = stdfsys::remove(get(path)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    uint64_t file_size(const path &path) {
        uint64_t res;
        try {   res = stdfsys::file_size(get(path)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    std::time_t last_write_time(const path& p) {
        std::time_t res;
        try {
            #if defined(_MSC_VER) && _MSC_VER <= 1800
                res = stdfsys::last_write_time(get(p));
            #else
                auto tt = stdfsys::last_write_time(get(p));
                res = stdfsys::file_time_type::clock::to_time_t(tt);
            #endif
        }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    uint64_t remove_all(const path& path) {
        uint64_t res;
        try {   res = stdfsys::remove_all(get(path)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }
    void copy_file(const path &from, const path &to) {
        try {   stdfsys::copy_file(get(from), get(to)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
    }
    void rename(const path &from, const path &to) {
        try {   stdfsys::rename(get(from), get(to)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
    }
    std::string extension(const path &path) {
        return path.extension().string();
    }
    path current_path() {
        path_impl res;
        try {
            #if defined(_MSC_VER) && _MSC_VER <= 1800
                res = stdfsys::current_path<stdfsys::path>();
            #else
                res = stdfsys::current_path();
            #endif
        }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return path(res);
    }
    void current_path(const path& to) {
        try { stdfsys::current_path(get(to)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
    }
    path canonical(const path& p) {
        path_impl res;
        try { res = stdfsys::canonical(get(p)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return path(res);
    }
    bool equivalent(const path &pathA, const path &pathB) {
        bool res;
        try { res = stdfsys::equivalent(get(pathA), get(pathB)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        return res;
    }

    space_info space(const path& p) {
        stdfsys::space_info stdres;
        try { stdres = stdfsys::space(get(p)); }
        catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
        space_info res;
        res.capacity = stdres.capacity;
        res.free = stdres.free;
        res.available = stdres.available;
        return res;
    }

    std::vector<path> directory_enumerate(const path &rootPath) {
        std::vector<path> res;
        try {
            for (auto iter = stdfsys::directory_iterator(get(rootPath)); iter != stdfsys::directory_iterator(); ++iter)
                res.push_back(path_impl(iter->path()));
        }
        catch(stdfsys::filesystem_error &e) {
            throw filesystem_error(e.what(), e.code());
        }
        return res;
    }
    std::vector<path> recursive_directory_enumerate(const path &rootPath) {
        std::vector<path> res;
        try {
            for (auto iter = stdfsys::recursive_directory_iterator(get(rootPath)); iter != stdfsys::recursive_directory_iterator(); ++iter)
                res.push_back(path_impl(iter->path()));
        }
        catch(stdfsys::filesystem_error &e) {
            throw filesystem_error(e.what(), e.code());
        }
        return res;
    }
}
