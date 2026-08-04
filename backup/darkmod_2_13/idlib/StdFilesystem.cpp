/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#include "StdFilesystem.h"

#ifdef _MSC_VER
	#if _MSC_VER < 1910
		//STL-based implementation for MSVC2013
		#include <filesystem>
		namespace stdfsys = std::tr2::sys;
	#else // MSVC 2017+
		#if _HAS_CXX17
			#include <filesystem>
			namespace stdfsys = std::filesystem;
		#else
			#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
			#include <experimental/filesystem>
			namespace stdfsys = std::experimental::filesystem;
		#endif
	#endif 
#else
	//it should be here for both GCC and Clang
	#include <experimental/filesystem>
	namespace stdfsys = std::experimental::filesystem::v1;
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
		return d->string();
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
	bool operator<(const path& lhs, const path& rhs) {
		return get(lhs) < get(rhs);
	}

	filesystem_error::filesystem_error(const char *what_arg, std::error_code ec) : std::system_error(ec, what_arg) {}

	bool is_directory(const path &path) {
		bool res;
		try {	res = stdfsys::is_directory(get(path)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
		return res;
	}
	bool create_directory(const path &path) {
		bool res;
		try {	res = stdfsys::create_directory(get(path)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
		return res;
	}
	bool exists(const path &path) {
		bool res;
		try {	res = stdfsys::exists(get(path)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
		return res;
	}
	bool create_directories(const path &path) {
		bool res;
		try {	res = stdfsys::create_directories(get(path)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
		return res;
	}
	bool remove(const path &path) {
		bool res;
		try {	res = stdfsys::remove(get(path)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
		return res;
	}
	uint64_t file_size(const path &path) {
		uint64_t res;
		try {	res = stdfsys::file_size(get(path)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
		return res;
	}
	uint64_t remove_all(const path& path) {
		uint64_t res;
		try {	res = stdfsys::remove_all(get(path)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
		return res;
	}
	void copy_file(const path &from, const path &to) {
		try {	stdfsys::copy_file(get(from), get(to)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
	}
	void rename(const path &from, const path &to) {
		try {	stdfsys::rename(get(from), get(to)); }
		catch(stdfsys::filesystem_error &e) { throw filesystem_error(e.what(), e.code()); }
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
