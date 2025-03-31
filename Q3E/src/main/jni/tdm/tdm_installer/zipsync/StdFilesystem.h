#ifndef STD_FILESYSTEM_H_
#define STD_FILESYSTEM_H_

#include <stdint.h>
#include <ctime>
#include <string>
#include <memory>
#include <vector>
#include <system_error>

namespace stdext {
    //all of these types and functions are equivalents to boost::filesystem namespace contents
    //also, they neatly wrap std::filesystem code

    struct path_impl;

    class path {
    public:
        ~path();
        path();
        path(const path& path);
        path& operator= (const path &path);
        path(path&& path);
        path& operator= (path &&path);

        path(const path_impl& impl);
        path(const char *source);
        path(const std::string &source);
        std::string string() const;     //generic_string --- in generic format
        bool empty() const;
        path parent_path() const;
        path filename() const;
        path extension() const;
        path stem() const;
        bool is_absolute() const;
        path& remove_filename();

    private:
        std::unique_ptr<path_impl> d;
        friend path_impl &get(path &path);
        friend const path_impl &get(const path &path);
    };

    path operator/(const path& lhs, const path& rhs);
    path& operator/=(path& lhs, const path& rhs);
    bool operator==(const path& lhs, const path& rhs);
    bool operator!=(const path& lhs, const path& rhs);
    bool operator<(const path& lhs, const path& rhs);

    bool is_directory(const path &path);
    bool is_regular_file(const path &path);
    bool create_directory(const path &path);
    bool exists(const path &path);
    bool create_directories(const path &path);
    bool remove(const path &path);
    uint64_t file_size(const path &path);
    std::time_t last_write_time(const path& p);
    uint64_t remove_all(const path& path);
    void copy_file(const path &from, const path &to);
    void rename(const path &from, const path &to);
    std::string extension(const path &path);
    path current_path();
    void current_path(const path& to);
    path canonical(const path& path);
    bool equivalent(const path &pathA, const path &pathB);

    struct space_info {
        std::uintmax_t capacity;
        std::uintmax_t free;
        std::uintmax_t available;
    };
    space_info space(const path& p);

    //below is some replacement for C++ iterator mess
    //(cost of memory allocation should be negligible compare to cost of filesystem access)

    //replacement for directory_iterator
    std::vector<path> directory_enumerate(const path &path);
    //replacement for recursive_directory_iterator
    std::vector<path> recursive_directory_enumerate(const path &path);

    class filesystem_error : public std::system_error {
    public:
        filesystem_error(const char *what_arg, std::error_code ec);
    };
}

#endif
