//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <stdio.h>

#include <crlib/string.h>
#include <crlib/lambda.h>

CR_NAMESPACE_BEGIN

// simple stdio file wrapper
class File final : private NonCopyable {
private:
   FILE *handle_ = nullptr;
   size_t length_ {};

public:
   explicit File () = default;
   
   File (StringRef file, StringRef mode = "rt") {
      open (file, mode);
   }

   ~File () {
      close ();
   }

public:
   bool open (StringRef file, StringRef mode) {
      if (!openFile (file, mode)) {
         return false;
      }
      fseek (handle_, 0L, SEEK_END);
      length_ = ftell (handle_);
      fseek (handle_, 0L, SEEK_SET);

      return true;
   }

   void close () {
      if (handle_ != nullptr) {
         fclose (handle_);
         handle_ = nullptr;
      }
      length_ = 0;
   }

   bool eof () const {
      return !!feof (handle_);
   }

   bool flush () const {
      return !!fflush (handle_);
   }

   int get () const {
      return fgetc (handle_);
   }

   char *getString (char *buffer, int count) const {
      return fgets (buffer, count, handle_);
   }

   bool getLine (String &line) {
      int ch = 0;
      SmallArray <char> data (255);

      line.clear ();

      while ((ch = get ()) != EOF && !eof ()) {
         data.push (static_cast <char> (ch));

         if (ch == '\n') {
            break;
         }
      }
      if (!data.empty ()) {
         line.assign (data.data (), data.length ());
      }
      return !line.empty ();
   }

   template <typename ...Args> size_t puts (const char *fmt, Args &&...args) {
      if (!*this) {
         return 0;
      }
      return fputs (strings.format (fmt, cr::forward <Args> (args)...), handle_);
   }

   bool puts (const char *buffer) {
      if (!*this) {
         return 0;
      }
      if (fputs (buffer, handle_) < 0) {
         return false;
      }
      return true;
   }

   int putChar (int ch) {
      return fputc (ch, handle_);
   }

   size_t read (void *buffer, size_t size, size_t count = 1) {
      return fread (buffer, size, count, handle_);
   }

   size_t write (void *buffer, size_t size, size_t count = 1) {
      return fwrite (buffer, size, count, handle_);
   }

   bool seek (long offset, int origin) {
      if (fseek (handle_, offset, origin) != 0) {
         return false;
      }
      return true;
   }

   void rewind () {
      ::rewind (handle_);
   }

   size_t length () const {
      return length_;
   }

public:
   explicit operator bool () const {
      return handle_ != nullptr;
   }

public:
   static inline void makePath (const char  *path) {
      auto dir = const_cast <char *> (path) + sizeof (kPathSeparator);

      for (auto str = dir; *str != kNullChar; ++str) {
         if (*str ==  *kPathSeparator) {
            *str = kNullChar;
            plat.createDirectory (path);
            *str = *kPathSeparator;
         }
      }
      plat.createDirectory (path);
   }

private:
   bool openFile (StringRef filename, StringRef mode) {
      if (*this) {
         close ();
      }
      handle_ = plat.openStdioFile (filename.chars (), mode.chars ());

      return handle_ != nullptr;
   }
};

// wrapper for memory file for loading data into the memory
class MemFileStorage : public Singleton <MemFileStorage> {
private:
   using LoadFunction = Lambda <uint8_t * (const char *, int *)>;
   using FreeFunction = Lambda <void (void *)>;

private:
   LoadFunction loadFun_ = nullptr;
   FreeFunction freeFun_ = nullptr;

public:
   explicit MemFileStorage () = default;
   ~MemFileStorage () = default;

public:
   void initizalize (LoadFunction loader, FreeFunction unloader) {
      loadFun_ = cr::move (loader);
      freeFun_ = cr::move (unloader);
   }

public:
   uint8_t *load (StringRef file, int *size) {
      if (loadFun_) {
         return loadFun_ (file.chars (), size);
      }
      return nullptr;
   }

   void unload (void *buffer) {
      if (freeFun_) {
         freeFun_ (buffer);
      }
   }

public:
   static uint8_t *defaultLoad (const char *path, int *size) {
      File file (path, "rb");

      if (!file) {
         *size = 0;
         return nullptr;
      }
      *size = static_cast <int> (file.length ());
      auto data = Memory::get <uint8_t> (*size);

      file.read (data, *size);
      return data;
   }

   static void defaultUnload (void *buffer) {
      Memory::release (buffer);
   }

   static String loadToString (StringRef filename) {
      int32_t result = 0;
      auto buffer = defaultLoad (filename.chars (), &result);

      if (result > 0 && buffer) {
         String data (reinterpret_cast <char *> (buffer), result);
         defaultUnload (buffer);

         return data;
      }
      return "";
   }
};

class MemFile final : public NonCopyable {
private:
   enum : char {
      Eof = static_cast <char> (-1)
   };

private:
   uint8_t *contents_ = nullptr;
   size_t length_ {};
   size_t seek_ {};

public:
   explicit MemFile () = default;

   MemFile (StringRef file) {
      open (file);
   }

   ~MemFile () {
      close ();
   }

public:
   bool open (StringRef file) {
      length_ = 0;
      seek_ = 0;
      contents_ = MemFileStorage::instance ().load (file.chars (), reinterpret_cast <int *> (&length_));

      if (!contents_) {
         return false;
      }
      return true;
   }

   void close () {
      if (!contents_) {
         return;
      }
      MemFileStorage::instance ().unload (contents_);

      length_ = 0;
      seek_ = 0;
      contents_ = nullptr;
   }

   char get () {
      if (!contents_ || seek_ >= length_) {
         return Eof;
      }
      return static_cast <char> (contents_[seek_++]);
   }

   char *getString (char *buffer, size_t count) {
      if (!contents_ || seek_ >= length_) {
         return nullptr;
      }
      size_t index = 0;
      buffer[0] = 0;

      for (; index < count - 1;) {
         if (seek_ < length_) {
            buffer[index] = contents_[seek_++];

            if (buffer[index++] == '\n') {
               break;
            }
         }
         else {
            break;
         }
      }
      buffer[index] = 0;
      return index ? buffer : nullptr;
   }

   bool getLine (String &line) {
      char ch;
      SmallArray <char> data (255);

      line.clear ();

      while ((ch = get ()) != Eof && !eof ()) {
         data.push (ch);

         if (ch == '\n') {
            break;
         }
      }

      if (!data.empty ()) {
         line.assign (data.data (), data.length ());
      }
      return !line.empty ();
   }

   size_t read (void *buffer, size_t size, size_t count = 1) {
      if (!contents_ || length_ <= seek_ || !buffer || !size || !count) {
         return 0;
      }
      size_t blocksRead = size * count <= length_ - seek_ ? size * count : length_ - seek_;

      memcpy (buffer, &contents_[seek_], blocksRead);
      seek_ += blocksRead;

      return blocksRead / size;
   }

   bool seek (size_t offset, int origin) {
      if (!contents_ || seek_ >= length_) {
         return false;
      }
      if (origin == SEEK_SET) {
         if (offset >= length_) {
            return false;
         }
         seek_ = offset;
      }
      else if (origin == SEEK_END) {
         if (offset >= length_) {
            return false;
         }
         seek_ = length_ - offset;
      }
      else {
         if (seek_ + offset >= length_) {
            return false;
         }
         seek_ += offset;
      }
      return true;
   }

   size_t length () const {
      return length_;
   }

   bool eof () const {
      return seek_ >= length_;
   }

   void rewind () {
      seek_ = 0;
   }

public:
   explicit operator bool () const {
      return !!contents_ && length_ > 0;
   }
};

namespace detail {
   struct FileEnumeratorEntry : public NonCopyable {
      FileEnumeratorEntry () = default;
      ~FileEnumeratorEntry () = default;

#if defined(CR_WINDOWS)
      bool next {};
      String path {};
      HANDLE handle {};
      WIN32_FIND_DATAA data {};
#else
      String path {};
      String mask {};
      DIR *dir {};
      dirent *entry {};
#endif
   };
}

// file enumerator
class FileEnumerator : public NonCopyable {
private:
   UniquePtr <detail::FileEnumeratorEntry> entries_;

public:
   FileEnumerator (StringRef mask) : entries_ (cr::makeUnique <detail::FileEnumeratorEntry> ()) {
      start (mask);
   }

   ~FileEnumerator () {
      close ();
   }

public:
   void start (StringRef mask) {
      auto lastSeperatorPos = mask.findLastOf (kPathSeparator);
      Twin <String, String> pam {};

      if (lastSeperatorPos != StringRef::InvalidIndex) {
         pam = {
            mask.substr (0, lastSeperatorPos),
            mask.substr (1 + lastSeperatorPos)
         };
      }
      else {
         pam = { ".", mask };
      }
      entries_->path = pam.first;

#if defined(CR_WINDOWS)
      entries_->handle = FindFirstFileA (mask.chars (), &entries_->data);
      entries_->next = entries_->handle != INVALID_HANDLE_VALUE;
#else
      entries_->dir = opendir (entries_->path.chars ());
      entries_->mask = pam.second;

      if (entries_->dir != nullptr) {
         next ();
      }
#endif
   }

   void close () {
#if defined(CR_WINDOWS)
      if (entries_->handle != INVALID_HANDLE_VALUE) {
         FindClose (entries_->handle);

         entries_->handle = INVALID_HANDLE_VALUE;
         entries_->next = false;
      }
#else
      if (entries_->dir != nullptr) {
         closedir (entries_->dir);
         entries_->dir = nullptr;
      }
#endif
   }

   bool next () {
#if defined(CR_WINDOWS)
      entries_->next = !!FindNextFileA (entries_->handle, &entries_->data);
      return entries_->next;
#else
      while ((entries_->entry = readdir (entries_->dir)) != nullptr) {
         if (!fnmatch (entries_->mask.chars (), entries_->entry->d_name, FNM_CASEFOLD | FNM_NOESCAPE | FNM_PERIOD)) {
            return true;
         }
      }
      return false;
#endif
   }

   bool stillValid () const {
#if defined(CR_WINDOWS)
      return entries_->next;
#else
      return entries_->dir != nullptr && entries_->entry != nullptr;
#endif
   }

   String getMatch () const {
      StringRef match {};

#if defined(CR_WINDOWS)
      match = entries_->data.cFileName;
#else
      match = entries_->entry->d_name;
#endif
      return String::join ({ entries_->path, match }, kPathSeparator);
   }

public:
   operator bool () const {
      return stillValid ();
   }
};

CR_NAMESPACE_END
