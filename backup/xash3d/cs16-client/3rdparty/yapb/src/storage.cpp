//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

#if defined(BOT_STORAGE_EXPLICIT_INSTANTIATIONS)

template <typename U> bool BotStorage::load (SmallArray <U> &data, ExtenHeader *exten, int32_t *outOptions) {
   auto type = guessType <U> ();
   String filename = buildPath (storageToBotFile (type.option), true);

   extern ConVar cv_debug, cv_graph_url;

   // graphs can be downloaded...
   const bool isGraph = !!(type.option & StorageOption::Graph);
   const bool isDebug = cv_debug || game.isDeveloperMode ();

   MemFile file (filename); // open the file
   data.clear ();

   // resize data to fit the stuff
   auto resizeData = [&] (const size_t length) {
      data.resize (length); // for non-graph data the graph should be already loaded
      data.shrink (); // free up memory to minimum

      // ensure we're have enough memory to decompress the data
      data.ensure (length + ULZ::Excess);
   };

   // if graph & attempted to load multiple times, bail out, we're failed
   if (isGraph && ++m_retries > 2) {
      resetRetries ();

      return error (isGraph, isDebug, file, "Unable to load %s (filename: '%s'). Download process has failed as well. No nodes has been found.", type.name, filename);
   }

   // downloader for graph
   auto download = [&] () -> bool {
      if (!graph.canDownload ()) {
         return false;
      }
      String lowercaseMapName = game.getMapName ();
      lowercaseMapName = lowercaseMapName.lowercase ();

      auto downloadAddress = cv_graph_url.as <StringRef> ();

      auto toDownload = buildPath (storageToBotFile (type.option), false);
      auto fromDownload = strings.format ("%s://%s/graph/%s.graph", product.httpScheme, downloadAddress, lowercaseMapName);

      // try to download
      if (http.downloadFile (fromDownload, toDownload)) {
         ctrl.msg ("%s file '%s' successfully downloaded. Processing...", type.name, filename);
         return true;
      }
      else {
         ctrl.msg ("Can't download '%s' from '%s' to '%s'... (%d).", filename, fromDownload, toDownload, http.getLastStatusCode ());
      }
      return false;
   };

   // tries to reload or open pwf file
   auto tryReload = [&] () -> bool {
      file.close ();

      if (!isGraph) {
         return false;
      }

      if (download ()) {
         return load (data, exten, outOptions);
      }

      if (graph.convertOldFormat ()) {
         return load (data, exten, outOptions);
      }
      return false;
   };

   // no open no fun
   if (!file) {
      if (tryReload ()) {
         return true;
      }
      return error (isGraph, isDebug, file, "Unable to open %s file for reading (filename: '%s').", type.name, filename);
   }

   // erase the current graph just in case
   auto unlinkIfGraph = [&] () {
      if (isGraph) {
         unlinkFromDisk (false, true);
      }
   };

   // read the header
   StorageHeader hdr {};
   file.read (&hdr, sizeof (StorageHeader));

   // check the magic
   if (hdr.magic != kStorageMagic && hdr.magic != kStorageMagicUB) {
      unlinkIfGraph ();

      if (tryReload ()) {
         return true;
      }
      return error (isGraph, isDebug, file, "Unable to read magic of %s (filename: '%s').", type.name, filename);
   }

   // check the path-numbers
   if (!isGraph && hdr.length != graph.length ()) {
      return error (isGraph, isDebug, file, "Damaged %s (filename: '%s'). Mismatch number of nodes (got: '%d', need: '%d').", type.name, filename, hdr.length, graph.length ());
   }

   // check the count
   if (hdr.length == 0 || hdr.length > kMaxNodes || hdr.length < kMaxNodeLinks) {
      unlinkIfGraph ();

      if (tryReload ()) {
         return true;
      }
      return error (isGraph, isDebug, file, "Damaged %s (filename: '%s'). Paths length is overflowed (got: '%d').", type.name, filename, hdr.length);
   }

   // check the version
   if (hdr.version > type.version && isGraph) {
      ctrl.msg ("Graph version mismatch %s (filename: '%s'). Version number differs (got: '%d', need: '%d') Please, upgrade %s.", type.name, filename, hdr.version, type.version, product.name);
   }
   else if (hdr.version != type.version && !isGraph) {
      return error (isGraph, isDebug, file, "Damaged %s (filename: '%s'). Version number differs (got: '%d', need: '%d').", type.name, filename, hdr.version, type.version);
   }

   // save graph version
   if (isGraph) {
      graph.setGraphHeader (&hdr);
   }

   // check the storage type
   if ((hdr.options & type.option) != type.option) {
      return error (isGraph, isDebug, file, "Incorrect storage format for %s (filename: '%s').", type.name, filename);
   }
   const auto compressedSize = static_cast <size_t> (hdr.compressed);
   const auto numberNodes = static_cast <size_t> (hdr.length);

   SmallArray <uint8_t> compressed (compressedSize + sizeof (uint8_t) * ULZ::Excess);

   // graph is not resized upon load
   if (isGraph) {
      resizeData (numberNodes);
   }
   else {
      resizeData (hdr.uncompressed / sizeof (U));
   }

   // read compressed data
   if (file.read (compressed.data (), sizeof (uint8_t), compressedSize) == compressedSize) {
      // try to uncompress
      if (m_ulz->uncompress (compressed.data (), hdr.compressed, reinterpret_cast <uint8_t *> (data.data ()), hdr.uncompressed) == ULZ::UncompressFailure) {
         return error (isGraph, isDebug, file, "Unable to decompress ULZ data for %s (filename: '%s').", type.name, filename);
      }
      else {

         if (outOptions) {
            outOptions = &hdr.options;
         }

         // author of graph.. save
         if ((hdr.options & StorageOption::Exten) && exten != nullptr) {
            const auto extenSize = sizeof (ExtenHeader);
            const auto actuallyRead = file.read (exten, extenSize) * extenSize;

            if (isGraph) {
               resetRetries ();

               ExtenHeader extenHeader {};
               strings.copy (extenHeader.author, exten->author, cr::bufsize (exten->author));

               if (extenSize <= actuallyRead) {
                  // write modified by, only if the name is different
                  if (!strings.isEmpty (extenHeader.author)
                     && strncmp (extenHeader.author, exten->modified, cr::bufsize (extenHeader.author)) != 0) {

                     strings.copy (extenHeader.modified, exten->modified, cr::bufsize (exten->modified));
                  }
               }
               else {
                  strings.copy (extenHeader.modified, "(none)", cr::bufsize (exten->modified));
               }
               extenHeader.mapSize = exten->mapSize;

               // tell graph about exten header
               graph.setExtenHeader (&extenHeader);
            }
         }

         // for visibility tables load counts of stand/count numbers
         if (type.option & StorageOption::Vistable) {
            for (auto &path : graph) {
               file.read (&path.vis, sizeof (PathVis));
            }
         }

         if (game.isDeveloperMode ()) {
            ctrl.msg ("Loaded Bots %s data v%d (Memory: %.2fMB).", type.name, hdr.version, static_cast <float> (data.capacity () * sizeof (U)) / 1024.0f / 1024.0f);
         }
         file.close ();

         return true;
      }
   }
   else {
      return error (isGraph, isDebug, file, "Unable to read ULZ data for %s (filename: '%s').", type.name, filename);
   }
   return false;
}

template <typename U> bool BotStorage::save (const SmallArray <U> &data, ExtenHeader *exten, int32_t passOptions) {
   auto type = guessType <U> ();

   // append additional options
   if (passOptions != 0) {
      type.option |= passOptions;
   }
   const auto isGraph = !!(type.option & StorageOption::Graph);

   // hide some messages with debug cvar
   extern ConVar cv_debug;

   // do not allow to save graph with less than 8 nodes
   if (isGraph && graph.length () < kMaxNodeLinks) {
      ctrl.msg ("Can't save graph data with less than %d nodes. Please add some more before saving.", kMaxNodeLinks);
      return false;
   }
   String filename = buildPath (storageToBotFile (type.option));

   if (data.empty ()) {
      if (isGraph || cv_debug) {
         logger.error ("Unable to save %s file. Empty data. (filename: '%s').", type.name, filename);
      }
      return false;
   }
   else if (isGraph) {
      for (auto &path : graph) {
         path.display = 0.0f;
         path.light = illum.getLightLevel (path.origin);
      }
   }

   // open the file
   File file (filename, "wb");

   // no open no fun
   if (!file) {
      logger.error ("Unable to open %s file for writing (filename: '%s').", type.name, filename);
      return false;
   }
   const auto rawLength = data.length () * sizeof (U);
   SmallArray <uint8_t> compressed (rawLength + sizeof (uint8_t) * ULZ::Excess);

   // initialize compression
   ULZ ulz {};
   setUlzInstance (&ulz);

   // try to compress
   const auto compressedLength = static_cast <size_t> (m_ulz->compress (reinterpret_cast <uint8_t *> (data.data ()), static_cast <int32_t> (rawLength), reinterpret_cast <uint8_t *> (compressed.data ())));

   if (compressedLength > 0) {
      StorageHeader hdr {};

      hdr.magic = kStorageMagic;
      hdr.version = type.version;
      hdr.options = type.option;
      hdr.length = graph.length ();
      hdr.compressed = static_cast <int32_t> (compressedLength);
      hdr.uncompressed = static_cast <int32_t> (rawLength);

      file.write (&hdr, sizeof (StorageHeader));
      file.write (compressed.data (), sizeof (uint8_t), compressedLength);

      // for visibility tables save counts of stand/count numbers
      if (type.option & StorageOption::Vistable) {
         for (auto &path : graph) {
            file.write (&path.vis, sizeof (PathVis));
         }
      }

      // add extension
      if ((type.option & StorageOption::Exten) && exten != nullptr) {
         file.write (exten, sizeof (ExtenHeader));
      }

      // notify only about graph
      if (isGraph || cv_debug) {
         ctrl.msg ("Successfully saved Bots %s data.", type.name);
      }
   }
   else {
      logger.error ("Unable to compress %s data (filename: '%s').", type.name, filename);
      return false;
   }
   return true;
}

template <typename ...Args> bool BotStorage::error (bool isGraph, bool isDebug, MemFile &file, const char *fmt, Args &&...args) {
   auto result = strings.format (fmt, cr::forward <Args> (args)...);

   // display error only for graph file
   if (isGraph || isDebug) {
      logger.error (result);
   }

   // if graph reset paths
   if (isGraph) {
      bots.kickEveryone (true);
      graph.reset ();
   }
   file.close ();

   return false;
}

template <typename U> BotStorage::SaveLoadData BotStorage::guessType () {
   if constexpr (cr::is_same <U, FloydWarshallAlgo::Matrix>::value) {
      return { "Pathmatrix", StorageOption::Matrix, StorageVersion::Matrix };
   }
   else if constexpr (cr::is_same <U, BotPractice::DangerSaveRestore>::value) {
      return { "Practice", StorageOption::Practice, StorageVersion::Practice };
   }
   else if constexpr (cr::is_same <U, GraphVistable::VisStorage>::value) {
      return { "Vistable", StorageOption::Vistable, StorageVersion::Vistable };
   }
   else if constexpr (cr::is_same <U, Path>::value) {
      return { "Graph", StorageOption::Graph, StorageVersion::Graph };
   }
}

#else 

String BotStorage::buildPath (int32_t file, bool isMemoryLoad, bool withoutMapName) {
   using FilePath = Twin <String, String>;

   static HashMap <int32_t, FilePath> paths = {
      { BotFile::Vistable, FilePath (folders.train, "vis")},
      { BotFile::Practice, FilePath (folders.train, "prc")},
      { BotFile::Pathmatrix, FilePath (folders.train, "pmx")},
      { BotFile::LogFile, FilePath (folders.logs, "txt")},
      { BotFile::Graph, FilePath (folders.graph, "graph")},
      { BotFile::PodbotPWF, FilePath (folders.podbot, "pwf")}
   };

   static StringArray path {};
   path.clear ();

   // if not memory file we're don't need game dir
   if (isMemoryLoad) {
      path.emplace (getRunningPathVFS ());
   }
   else {
      path.emplace (getRunningPath ());
   }

   // the datadir
   path.emplace (folders.data);

   // append real filepath
   path.emplace (paths[file].first);

   // if file is logfile use correct logfile name with date
   if (file == BotFile::LogFile) {
      time_t ticks = time (&ticks);
      tm timeinfo {};

      plat.loctime (&timeinfo, &ticks);
      auto timebuf = strings.chars ();

      strftime (timebuf, StringBuffer::StaticBufferSize, "L%d%m%Y", &timeinfo);
      path.emplace (strings.format ("%s_%s.%s", product.nameLower, timebuf, paths[file].second));
   }
   else if (!withoutMapName) {
      String mapName = game.getMapName ();
      path.emplace (strings.format ("%s.%s", mapName.lowercase (), paths[file].second));
   }

   // finally use correct path separators for us
   return String::join (path, kPathSeparator);
}

int32_t BotStorage::storageToBotFile (int32_t options) {
   // converts storage option to storage filename

   if (options & StorageOption::Graph) {
      return BotFile::Graph;
   }
   else if (options & StorageOption::Matrix) {
      return BotFile::Pathmatrix;
   }
   else if (options & StorageOption::Vistable) {
      return BotFile::Vistable;
   }
   else if (options & StorageOption::Practice) {
      return BotFile::Practice;
   }
   return BotFile::Graph;
}

void BotStorage::unlinkFromDisk (bool onlyTrainingData, bool silenceMessages) {
   // this function removes graph file from the hard disk

   StringArray unlinkable {};
   bots.kickEveryone (true);

   // if we're delete graph, delete all corresponding to it files
   if (!onlyTrainingData) {
      unlinkable.emplace (buildPath (BotFile::Graph)); // graph itself
   }
   unlinkable.emplace (buildPath (BotFile::Practice)); // corresponding to practice
   unlinkable.emplace (buildPath (BotFile::Vistable)); // corresponding to vistable
   unlinkable.emplace (buildPath (BotFile::Pathmatrix)); // corresponding to matrix

   for (const auto &item : unlinkable) {
      if (plat.fileExists (item.chars ())) {
         plat.removeFile (item.chars ());

         if (!silenceMessages) {
            ctrl.msg ("File %s, has been deleted from the hard disk", item);
         }
      }
      else if (!silenceMessages) {
         logger.error ("Unable to open %s", item);
      }
   }
   graph.reset (); // re-initialize points

   if (onlyTrainingData) {
      graph.loadGraphData ();

      // take bots  back to game
      cv_quota.revert ();
   }
}

StringRef BotStorage::getRunningPath () {
   // this function get's relative path against bot library (bot library should reside in bin dir)

   static String path {};

   // we're do not do relative (against bot's library) paths on specific cases
   if (m_useNonRelativePaths) {
      if (path.empty ()) {
         path = strings.joinPath (game.getRunningModName (), folders.addons, folders.bot);
      }
      return path;
   }

   // compute the full path to the our folder
   if (path.empty ()) {
      path = SharedLibrary::path (&bstor);

      if (path.empty ()) {
         logger.fatal ("Unable to detect library path. Giving up...");
      }

      // remove dot prefix
      if (path.startsWith (".")) {
         path.ltrim (".\\/");
      }
      auto parts = path.substr (1).split (kPathSeparator);

      parts.pop (); // remove library name
      parts.pop (); // remove bin directory

      path = path.substr (0, 1) + String::join (parts, kPathSeparator);
   }
   return path;
}

StringRef BotStorage::getRunningPathVFS () {
   static String path {};

   // we're do not do relative (against bot's library) paths on android 
   if (m_useNonRelativePaths) {
      if (path.empty ()) {
         path = strings.joinPath (folders.addons, folders.bot);
      }
      return path;
   }

   if (path.empty ()) {
      path = getRunningPath ();

      path = path.substr (path.rfind (game.getRunningModName ())); // skip to the game dir
      path = path.substr (path.find (kPathSeparator) + 1); // skip the game dir
   }
   return path;
}

void BotStorage::checkInstallLocation () {
   if (plat.android || plat.emscripten) {
      m_useNonRelativePaths = true;
      return;
   }
   String path = SharedLibrary::path (&bstor);

   if (path.empty ()) {
      m_useNonRelativePaths = true;
      return;
   }
   String dpath = strings.joinPath (folders.addons, folders.bot, folders.bin);
   path = path.substr (0, path.rfind (kPathSeparator));

   m_useNonRelativePaths = !path.lowercase ().endsWith (dpath.lowercase ());
}

#endif // BOT_STORAGE_EXPLICIT_INSTANTIATIONS

