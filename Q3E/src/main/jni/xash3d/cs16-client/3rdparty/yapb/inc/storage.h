//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// storage file magic (podbot)
constexpr char kPodbotMagic[8] = { 'P', 'O', 'D', 'W', 'A', 'Y', '!', kNullChar };

constexpr int32_t kStorageMagic = 0x59415042; // storage magic for yapb-data files
constexpr int32_t kStorageMagicUB = 0x544f4255; // support also the fork format (merged back into yapb)

// storage header options
CR_DECLARE_SCOPED_ENUM (StorageOption,
   Practice = cr::bit (0), // this is practice (experience) file
   Matrix = cr::bit (1), // this is floyd warshal path & distance matrix
   Vistable = cr::bit (2), // this is vistable data
   Graph = cr::bit (3), // this is a node graph data
   Official = cr::bit (4), // this is additional flag for graph indicates graph are official
   Recovered = cr::bit (5), // this is additional flag indicates graph converted from podbot and was bad
   Exten = cr::bit (6), // this is additional flag indicates that there's extension info
   Analyzed = cr::bit (7) // this graph has been analyzed
)

// storage header versions
CR_DECLARE_SCOPED_ENUM (StorageVersion,
   Graph = 2,
   Practice = 2,
   Vistable = 4,
   Matrix = 2,
   Podbot = 7
)

CR_DECLARE_SCOPED_ENUM_TYPE (BotFile, uint32_t,
   Vistable = 0,
   LogFile = 1,
   Practice = 2,
   Graph = 3,
   Pathmatrix = 4,
   PodbotPWF = 5,
   EbotEWP = 6
)

class BotStorage final : public Singleton <BotStorage> {
private:
   struct SaveLoadData {
      String name {};
      int32_t option {};
      int32_t version {};

   public:
      SaveLoadData (StringRef name, int32_t option, int32_t version) : name (name), option (option), version (version) {}
   };

private:
   int m_retries {};
   ULZ *m_ulz {};

public:
   BotStorage () = default;
   ~BotStorage () = default;

public:
   // converts type to save/load options
   template <typename U> SaveLoadData guessType ();

   // loads the data and decompress ulz
   template <typename U> bool load (SmallArray <U> &data, ExtenHeader *exten = nullptr, int32_t *outOptions = nullptr);

   // saves the data and compress with ulz
   template <typename U> bool save (const SmallArray <U> &data, ExtenHeader *exten = nullptr, int32_t passOptions = 0);

   // report fatal error loading stuff
   template <typename ...Args> bool error (bool isGraph, bool isDebug, MemFile &file, const char *fmt, Args &&...args);

   // builds the filename to requested filename
   String buildPath (int32_t type, bool isMemoryLoad = false, bool withoutMapName = false);

   // get's relative path against bot library (bot library should reside in bin dir)
   StringRef getRunningPath ();

   // same as above, but with valve-specific filesystem paths (loadfileforme)
   StringRef getRunningPathVFS ();

   // converts storage option to storage filename
   int32_t storageToBotFile (int32_t options);

   // remove all bot related files from disk
   void unlinkFromDisk (bool onlyTrainingData, bool silenceMessages);

public:
   // loading the graph may attempt to recurse loading, with converting or download, reset retry counter
   void resetRetries () {
      m_retries = 0;
   }

   // set the compressor instance
   void setUlzInstance (ULZ *ulz) {
      m_ulz = ulz;
   }
};

#if !defined(BOT_STORAGE_EXPLICIT_INSTANTIATIONS)
#  define BOT_STORAGE_EXPLICIT_INSTANTIATIONS
#  include "../src/storage.cpp"
#endif

#undef BOT_STORAGE_EXPLICIT_INSTANTIATIONS

// expose global
CR_EXPOSE_GLOBAL_SINGLETON (BotStorage, bstor);

