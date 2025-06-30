//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <time.h>

#include <crlib/files.h>
#include <crlib/lambda.h>

CR_NAMESPACE_BEGIN

class SimpleLogger final : public Singleton <SimpleLogger> {
public:
   using PrintFunction = Lambda <void (const char *)>;

private:
   String filename_;
   PrintFunction printFun_;
   bool disableLogWrite_ = false;

public:
   explicit SimpleLogger () = default;
   ~SimpleLogger () = default;

public:
   class LogFile final {
   private:
      File handle_;

   public:
      LogFile (StringRef filename) {
         handle_.open (filename, "at");
      }

      ~LogFile () {
         handle_.close ();
      }

   public:
      void print (StringRef msg) {
         if (!handle_) {
            return;
         }
         handle_.puts (msg.chars ());
      }
   };

private:
   void logToFile (const char *level, const char *msg) {
      if (disableLogWrite_) {
         return;
      }

      time_t ticks = time (&ticks);
      tm timeinfo {};

      plat.loctime (&timeinfo, &ticks);

      auto timebuf = strings.chars ();
      strftime (timebuf, StringBuffer::StaticBufferSize, "%Y-%m-%d %H:%M:%S", &timeinfo);

      LogFile lf (filename_);
      lf.print (strings.format ("%s (%s): %s\n", timebuf, level, msg));
   }

public:
   template <typename ...Args> void fatal (const char *fmt, Args &&...args) {
      auto msg = strings.format (fmt, cr::forward <Args> (args)...);

      logToFile ("FATAL", msg);
      plat.abort (msg);
   }

   template <typename ...Args> void error (const char *fmt, Args &&...args) {
      auto msg = strings.format (fmt, cr::forward <Args> (args)...);

      logToFile ("ERROR", msg);

      if (printFun_) {
         printFun_ (msg);
      }
   }

   template <typename ...Args> void message (const char *fmt, Args &&...args) {
      auto msg = strings.format (fmt, cr::forward <Args> (args)...);

      logToFile ("INFO", msg);

      if (printFun_) {
         printFun_ (msg);
      }
   }

public:
   void initialize (StringRef filename, PrintFunction printFunction) {
      printFun_ = cr::move (printFunction);
      filename_ = filename;
      disableLogWrite_ = false;
   }

   void disableLogWrite (bool enable) {
      disableLogWrite_ = enable;
   }
};

// expose global instance
CR_EXPOSE_GLOBAL_SINGLETON (SimpleLogger, logger);

CR_NAMESPACE_END
