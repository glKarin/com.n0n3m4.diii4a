//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#ifdef VERSION_GENERATED
#  define VERSION_HEADER <version.build.h>
#else
#  define VERSION_HEADER <version.h>
#endif

#include VERSION_HEADER

// compile time build string
#define CTS_BUILD_STR static inline constexpr StringRef

// simple class for bot internal information
static constexpr class Product final {
public:
   explicit constexpr Product () = default;
   ~Product () = default;

public:
   static constexpr struct BuildInfo {
      CTS_BUILD_STR hash { MODULE_COMMIT_HASH };
      CTS_BUILD_STR count { MODULE_COMMIT_COUNT };
      CTS_BUILD_STR author { MODULE_AUTHOR };
      CTS_BUILD_STR machine { MODULE_MACHINE };
      CTS_BUILD_STR compiler { MODULE_COMPILER };
      CTS_BUILD_STR id { MODULE_BUILD_ID };
   } bi {};

public:
   CTS_BUILD_STR name { "YaPB" };
   CTS_BUILD_STR nameLower { "yapb" };
   CTS_BUILD_STR year { &__DATE__[7] };
   CTS_BUILD_STR author { "YaPB Project" };
   CTS_BUILD_STR email { "yapb@jeefo.net" };
   CTS_BUILD_STR url { "https://yapb.jeefo.net/" };
   CTS_BUILD_STR download { "yapb.jeefo.net" };
   CTS_BUILD_STR upload { "yapb.jeefo.net/upload" };
   CTS_BUILD_STR httpScheme { "http" };
   CTS_BUILD_STR logtag { "YB" };
   CTS_BUILD_STR dtime { __DATE__ " " __TIME__ };
   CTS_BUILD_STR date { __DATE__ };
   CTS_BUILD_STR version { MODULE_VERSION "." MODULE_COMMIT_COUNT };
   CTS_BUILD_STR cmdPri { "yb" };
   CTS_BUILD_STR cmdSec { "yapb" };
} product {};

static constexpr class Folders final {
public:
   explicit constexpr Folders () = default;
   ~Folders () = default;

public:
   CTS_BUILD_STR bot { product.nameLower };
   CTS_BUILD_STR addons { "addons" };
   CTS_BUILD_STR config { "conf" };
   CTS_BUILD_STR data { "data" };
   CTS_BUILD_STR lang { "lang" };
   CTS_BUILD_STR logs { "logs" };
   CTS_BUILD_STR train { "train" };
   CTS_BUILD_STR graph { "graph" };
   CTS_BUILD_STR podbot { "pwf" };
   CTS_BUILD_STR bin { "bin" };
} folders {};

#undef CTS_BUILD_STR
