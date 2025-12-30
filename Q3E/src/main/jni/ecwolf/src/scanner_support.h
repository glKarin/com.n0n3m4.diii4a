#ifndef __SCANNER_SUPPORT_H__
#define __SCANNER_SUPPORT_H__

// Scanner support file to allow easy translation between frameworks

#if 0
// STD C++ Library

#include <string>

typedef std::string	SCString;
typedef size_t		SCString_Index;

inline SCString &SCString_AppendChar(SCString &obj, const char chr) { return obj.append(1, chr); }
inline int SCString_Compare(const SCString &obj, const char *other) { return obj.compare(other); }
inline const char* SCString_GetChars(const SCString &obj) { return obj.c_str(); }
inline SCString_Index SCString_IndexOf(const SCString &obj, const char chr, SCString_Index spos) { return obj.find_first_of(chr, spos); }
inline SCString_Index SCString_IndexOfSeq(const SCString &obj, const SCString &seq, SCString_Index spos) { return obj.find(seq, spos); }
inline void SCString_InsertChar(SCString &obj, SCString_Index pos, const char chr) { obj.insert(pos, 1, chr); }
inline SCString_Index SCString_Len(const SCString &obj) { return obj.length(); }
inline SCString_Index SCString_NPos(const SCString &obj) { return std::string::npos; }
inline void SCString_Unescape(SCString &obj, SCString_Index pos, const char chr) { obj.replace(pos, 2, 1, chr); }

#elif 1
// ZDoom Library

#include "zstring.h"

typedef FString	SCString;
typedef long	SCString_Index;

inline SCString &SCString_AppendChar(SCString &obj, const char chr) { return obj += chr; }
// This is normally case sensitive, but for ZDoom stuff we're usually case insensitive.
// Hopefully I don't end up needing this for something strictly case sensitive and forget about this! :P
inline int SCString_Compare(const SCString &obj, const char *other) { return obj.CompareNoCase(other); }
inline const char* SCString_GetChars(const SCString &obj) { return obj; }
inline SCString_Index SCString_IndexOf(const SCString &obj, const char chr, SCString_Index spos) { return obj.IndexOf(chr, spos); }
inline SCString_Index SCString_IndexOfSeq(const SCString &obj, const SCString &seq, SCString_Index spos) { return obj.IndexOf(seq, spos); }
inline void SCString_InsertChar(SCString &obj, SCString_Index pos, const char chr) { obj.Insert(pos, &chr, 1); }
inline SCString_Index SCString_Len(const SCString &obj) { return static_cast<SCString_Index>(obj.Len()); }
inline SCString_Index SCString_NPos(const SCString &obj) { return -1; }
inline void SCString_Unescape(SCString &obj, SCString_Index pos, const char chr) { obj = obj.Left(pos) + chr + obj.Mid(pos+2); }

#endif

#endif /* __SCANNER_SUPPORT_H__ */
