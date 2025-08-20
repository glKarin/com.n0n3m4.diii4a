//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_display_menu_text ("display_menu_text", "1", "Enables or disables display menu text, when players asks for menu. Useful only for Android.", true, 0.0f, 1.0f, Var::Xash3D);
ConVar cv_password ("password", "", "The value (password) for the setinfo key, if user sets correct password, he's gains access to bot commands and menus.", false, 0.0f, 0.0f, Var::Password);
ConVar cv_password_key ("password_key", "_ybpw", "The name of setinfo key used to store password to bot commands and menus.", false);
ConVar cv_bots_kill_on_endround ("bots_kill_on_endround", "0", "Allows to use classic bot kill on issuing end-round command in menus, instead of gamedll endround.", false);

int BotControl::cmdAddBot () {
   enum args { alias = 1, difficulty, personality, team, model, name, max };

   // this is duplicate error as in main bot creation code, but not to be silent
   if (!graph.length () || graph.hasChanged ()) {
      msg ("There is no graph found or graph is changed. Cannot create bot.");
      return BotCommandResult::Handled;
   }

   // give a chance to use additional args
   m_args.resize (max);

   // if team is specified, modify args to set team
   if (arg <StringRef> (alias).endsWith ("_ct")) {
      m_args.set (team, "2");
   }
   else if (arg <StringRef> (alias).endsWith ("_t")) {
      m_args.set (team, "1");
   }

   // if high-skilled bot is requested set personality to rusher and max-out difficulty
   if (arg <StringRef> (alias).contains ("addhs")) {
      m_args.set (difficulty, "4");
      m_args.set (personality, "1");
   }
   bots.addbot (arg <StringRef> (name), arg <StringRef> (difficulty), arg <StringRef> (personality), arg <StringRef> (team), arg <StringRef> (model), true);

   return BotCommandResult::Handled;
}

int BotControl::cmdKickBot () {
   enum args { alias = 1, team };

   // if team is specified, kick from specified tram
   if (arg <StringRef> (alias).endsWith ("_ct") || arg <int> (team) == 2 || arg <StringRef> (team) == "ct") {
      bots.kickFromTeam (Team::CT);
   }
   else if (arg <StringRef> (alias).endsWith ("_t") || arg <int> (team) == 1 || arg <StringRef> (team) == "t") {
      bots.kickFromTeam (Team::Terrorist);
   }
   else {
      bots.balancedKickRandom (true);
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdKickBots () {
   enum args { alias = 1, instant, team };

   // check if we're need to remove bots instantly
   const auto kickInstant = arg <StringRef> (instant) == "instant";

   // if team is specified, kick from specified tram
   if (arg <StringRef> (alias).endsWith ("_ct") || arg <int> (team) == 2 || arg <StringRef> (team) == "ct") {
      bots.kickFromTeam (Team::CT, true);
   }
   else if (arg <StringRef> (alias).endsWith ("_t") || arg <int> (team) == 1 || arg <StringRef> (team) == "t") {
      bots.kickFromTeam (Team::Terrorist, true);
   }
   else {
      bots.kickEveryone (kickInstant);
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdKillBots () {
   enum args { alias = 1, team, silent, max };

   // do not issue any messages
   bool silentKill = hasArg (silent) && arg <StringRef> (silent).startsWith ("si");

   // if team is specified, kick from specified tram
   if (arg <StringRef> (alias).endsWith ("_ct") || arg <int> (team) == 2 || arg <StringRef> (team) == "ct") {
      bots.killAllBots (Team::CT, silentKill);
   }
   else if (arg <StringRef> (alias).endsWith ("_t") || arg <int> (team) == 1 || arg <StringRef> (team) == "t") {
      bots.killAllBots (Team::Terrorist, silentKill);
   }
   else {
      bots.killAllBots (Team::Invalid, silentKill);
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdFill () {
   enum args { alias = 1, team, count, difficulty, personality };

   if (!hasArg (team)) {
      return BotCommandResult::BadFormat;
   }
   bots.serverFill (arg <int> (team),
      hasArg (personality) ? arg <int> (personality) : -1,
      hasArg (difficulty) ? arg <int> (difficulty) : -1,
      hasArg (count) ? arg <int> (count) - 1 : -1);

   return BotCommandResult::Handled;
}

int BotControl::cmdVote () {
   enum args { alias = 1, mapid };

   if (!hasArg (mapid)) {
      return BotCommandResult::BadFormat;
   }
   const int mapID = arg <int> (mapid);

   // loop through all players
   for (const auto &bot : bots) {
      bot->m_voteMap = mapID;
   }
   msg ("All dead bots will vote for map #%d.", mapID);

   return BotCommandResult::Handled;
}

int BotControl::cmdWeaponMode () {
   enum args { alias = 1, type };

   if (!hasArg (type)) {
      return BotCommandResult::BadFormat;
   }
   static HashMap <String, int> modes {
      { "knife", 1 },
      { "pistol", 2 },
      { "shotgun", 3 },
      { "smg", 4 },
      { "rifle", 5 },
      { "sniper", 6 },
      { "standard", 7 }
   };
   auto mode = arg <StringRef> (type);

   // check if selected mode exists
   if (!modes.exists (mode)) {
      return BotCommandResult::BadFormat;
   }
   bots.setWeaponMode (modes[mode]);

   return BotCommandResult::Handled;
}

int BotControl::cmdVersion () {
   constexpr auto &bi = product.bi;

   msg ("%s v%s (ID %s)", product.name, product.version, bi.id);
   msg ("   by %s (%s)", product.author, product.email);
   msg ("   %s", product.url);
   msg ("compiled: %s on %s with %s", product.dtime, bi.machine, bi.compiler);

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeMenu () {
   enum args { alias = 1 };

   // graph editor is available only with editor
   if (!graph.hasEditor ()) {
      msg ("Unable to open graph editor without setting the editor player.");
      return BotCommandResult::Handled;
   }
   showMenu (Menu::NodeMainPage1);

   return BotCommandResult::Handled;
}

int BotControl::cmdMenu () {
   enum args { alias = 1, cmd };

   // reset the current menu
   closeMenu ();

   if (arg <StringRef> (cmd) == "cmd" && util.isAlive (m_ent)) {
      showMenu (Menu::Commands);
   }
   else {
      showMenu (Menu::Main);
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdList () {
   enum args { alias = 1 };

   bots.listBots ();
   return BotCommandResult::Handled;
}

int BotControl::cmdCvars () {
   enum args { alias = 1, pattern };

   auto match = arg <StringRef> (pattern);

   // stop printing if executed once more
   flushPrintQueue ();

   // revert all the cvars to their default values
   if (match == "defaults") {
      msg ("Bots cvars has been reverted to their default values.");

      for (const auto &cvar : game.getCvars ()) {
         if (!cvar.self || !cvar.self->ptr || cvar.type == Var::GameRef) {
            continue;
         }

         // set depending on cvar type
         if (cvar.bounded) {
            cvar.self->set (cvar.initial);
         }
         else {
            cvar.self->set (cvar.init.chars ());
         }
      }
      cv_quota.revert (); // quota should be reverted instead of regval

      return BotCommandResult::Handled;
   }

   const bool isSaveMain = match == "save";
   const bool isSaveMap = match == "save_map";

   const bool isSave = isSaveMain || isSaveMap;

   File cfg {};

   // if save requested, dump cvars to main config
   if (isSave) {
      auto cfgPath = strings.joinPath (bstor.getRunningPath (), folders.config, strings.format ("%s.%s", product.nameLower, kConfigExtension));

      if (isSaveMap) {
         cfgPath = strings.joinPath (bstor.getRunningPath (), folders.config, "maps", strings.format ("%s.%s", game.getMapName (), kConfigExtension));
      }
      cfg.open (cfgPath, "wt");
      cfg.puts ("// Configuration file for %s\n\n", product.name);
   }
   else {
      setRapidOutput (true);
   }

   for (const auto &cvar : game.getCvars ()) {
      if (cvar.info.empty () || !cvar.self || !cvar.self->ptr) {
         continue;
      }

      if (!isSave && !match.empty () && !strstr (cvar.reg.name, match.chars ())) {
         continue;
      }

      // prevent crash if file not accessible
      if (isSave && !cfg) {
         continue;
      }
      auto val = cvar.self->as <StringRef> ();

      // float value ?
      bool isFloat = !val.empty () && val.find (".") != StringRef::InvalidIndex;

      if (isSave) {
         cfg.puts ("//\n");
         cfg.puts ("// %s\n", String::join (cvar.info.split ("\n"), "\n//  "));
         cfg.puts ("// ---\n");

         if (cvar.bounded) {
            if (isFloat) {
               cfg.puts ("// Default: \"%.1f\", Min: \"%.1f\", Max: \"%.1f\"\n", cvar.initial, cvar.min, cvar.max);
            }
            else {
               cfg.puts ("// Default: \"%i\", Min: \"%i\", Max: \"%i\"\n", static_cast <int> (cvar.initial), static_cast <int> (cvar.min), static_cast <int> (cvar.max));
            }
         }
         else {
            cfg.puts ("// Default: \"%s\"\n", cvar.self->as <StringRef> ());
         }
         cfg.puts ("// \n");

         if (cvar.bounded) {
            if (isFloat) {
               cfg.puts ("%s \"%.1f\"\n", cvar.reg.name, cvar.self->as <float> ());
            }
            else {
               cfg.puts ("%s \"%i\"\n", cvar.reg.name, cvar.self->as <int> ());
            }
         }
         else {
            cfg.puts ("%s \"%s\"\n", cvar.reg.name, cvar.self->as <StringRef> ());
         }
         cfg.puts ("\n");
      }
      else {
         msg ("name: %s", cvar.reg.name);
         msg ("info: %s", conf.translate (cvar.info));

         msg (" ");
      }
   }
   setRapidOutput (false);

   if (isSave) {
      if (!cfg) {
         msg ("Unable to write cvars to config file. File not accessible");
         return BotCommandResult::Handled;
      }
      msg ("Bots cvars has been written to file.");
      cfg.close ();
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdShowCustom () {
   enum args { alias = 1 };

   conf.showCustomValues ();

   return BotCommandResult::Handled;
}

int BotControl::cmdExec () {
   enum args { alias = 1, target, command };

   if (!hasArg (target) || !hasArg (command)) {
      return BotCommandResult::BadFormat;
   }
   const auto userId = arg <int> (target);

   // find our little target
   auto bot = bots.findBotByIndex (userId);

   // bailout if not bot
   if (!bot) {
      msg ("Bot with #%d not found or not a bot.", userId);
      return BotCommandResult::Handled;
   }
   bot->issueCommand (arg <StringRef> (command).chars ());

   return BotCommandResult::Handled;
}

int BotControl::cmdNode () {
   enum args { root, alias, cmd, cmd2 };

   static Array <StringRef> allowedOnHLDS {
      "acquire_editor",
      "upload",
      "save",
      "load",
      "help",
      "erase",
      "erase_training",
      "fileinfo",
      "check"
   };

   // check if cmd is allowed on dedicated server
   auto isAllowedOnHLDS = [] (StringRef str) -> bool {
      for (const auto &test : allowedOnHLDS) {
         if (test == str) {
            return true;
         }
      }
      return false;
   };

   // graph editor supported only with editor
   if (game.isDedicated () && !graph.hasEditor () && !isAllowedOnHLDS (arg <StringRef> (cmd))) {
      msg ("Unable to use graph edit commands without setting graph editor player. Please use \"graph acquire_editor\" to acquire rights for graph editing.");
      return BotCommandResult::Handled;
   }

   // should be moved to class?
   static HashMap <String, BotCmd> commands {};
   static StringArray descriptions {};

   // fill only once
   if (descriptions.empty ()) {

      // separate function
      auto addGraphCmd = [&] (String cmd, String format, String help, Handler handler) -> void {
         BotCmd botCmd { cmd, cr::move (format), cr::move (help), cr::move (handler) };

         commands[cmd] = cr::move (botCmd);
         descriptions.push (cmd);
      };

      // add graph commands
      addGraphCmd ("on", "on [display|auto|noclip|models]", "Enables displaying of graph, nodes, noclip cheat", &BotControl::cmdNodeOn);
      addGraphCmd ("off", "off [display|auto|noclip|models]", "Disables displaying of graph, auto adding nodes, noclip cheat", &BotControl::cmdNodeOff);
      addGraphCmd ("menu", "menu [noarguments]", "Opens and displays bots graph editor.", &BotControl::cmdNodeMenu);
      addGraphCmd ("add", "add [noarguments]", "Opens and displays graph node add menu.", &BotControl::cmdNodeAdd);
      addGraphCmd ("addbasic", "menu [noarguments]", "Adds basic nodes such as player spawn points, goals and ladders.", &BotControl::cmdNodeAddBasic);
      addGraphCmd ("save", "save [noarguments]", "Save graph file to disk.", &BotControl::cmdNodeSave);
      addGraphCmd ("load", "load [noarguments]", "Load graph file from disk.", &BotControl::cmdNodeLoad);
      addGraphCmd ("erase", "erase [iamsure]", "Erases the graph file from disk.", &BotControl::cmdNodeErase);
      addGraphCmd ("erase_training", "erase_training", "Erases the training data leaving graph files.", &BotControl::cmdNodeEraseTraining);
      addGraphCmd ("delete", "delete [nearest|index]", "Deletes single graph node from map.", &BotControl::cmdNodeDelete);
      addGraphCmd ("check", "check [noarguments]", "Check if graph working correctly.", &BotControl::cmdNodeCheck);
      addGraphCmd ("cache", "cache [nearest|index]", "Caching node for future use.", &BotControl::cmdNodeCache);
      addGraphCmd ("clean", "clean [all|nearest|index]", "Clean useless path connections from all or single node.", &BotControl::cmdNodeClean);
      addGraphCmd ("setradius", "setradius [radius] [nearest|index]", "Sets the radius for node.", &BotControl::cmdNodeSetRadius);
      addGraphCmd ("flags", "flags [noarguments]", "Open and displays menu for modifying flags for nearest point.", &BotControl::cmdNodeSetFlags);
      addGraphCmd ("teleport", "teleport [index]", "Teleports player to specified node index.", &BotControl::cmdNodeTeleport);
      addGraphCmd ("upload", "upload", "Uploads created graph to graph database.", &BotControl::cmdNodeUpload);
      addGraphCmd ("stats", "stats [noarguments]", "Shows the stats about node types on the map.", &BotControl::cmdNodeShowStats);
      addGraphCmd ("fileinfo", "fileinfo [noarguments]", "Shows basic information about graph file.", &BotControl::cmdNodeFileInfo);
      addGraphCmd ("adjust_height", "adjust_height [height offset]", "Modifies all the graph nodes height (z-component) with specified offset.", &BotControl::cmdNodeAdjustHeight);

      // add path commands
      addGraphCmd ("path_create", "path_create [noarguments]", "Opens and displays path creation menu.", &BotControl::cmdNodePathCreate);
      addGraphCmd ("path_create_in", "path_create_in [noarguments]", "Creates incoming path connection from faced to nearest node.", &BotControl::cmdNodePathCreate);
      addGraphCmd ("path_create_out", "path_create_out [noarguments]", "Creates outgoing path connection from nearest to faced node.", &BotControl::cmdNodePathCreate);
      addGraphCmd ("path_create_both", "path_create_both [noarguments]", "Creates both-ways path connection between faced and nearest node.", &BotControl::cmdNodePathCreate);
      addGraphCmd ("path_create_jump", "path_create_jump [noarguments]", "Creates jumping path connection from nearest to faced node.", &BotControl::cmdNodePathCreate);
      addGraphCmd ("path_delete", "path_delete [noarguments]", "Deletes path from nearest to faced node.", &BotControl::cmdNodePathDelete);
      addGraphCmd ("path_set_autopath", "path_set_autopath [max_distance]", "Opens menu for setting autopath maximum distance.", &BotControl::cmdNodePathSetAutoDistance);
      addGraphCmd ("path_clean", "path_clean [index]", "Clears connections of all types from the node.", &BotControl::cmdNodePathCleanAll);

      // camp points iterator
      addGraphCmd ("iterate_camp", "iterate_camp [begin|end|next]", "Allows to go through all camp points on map.", &BotControl::cmdNodeIterateCamp);

      // remote graph editing stuff
      if (game.isDedicated ()) {
         addGraphCmd ("acquire_editor", "acquire_editor [noarguments]", "Acquires rights to edit graph on dedicated server.", &BotControl::cmdNodeAcquireEditor);
         addGraphCmd ("release_editor", "release_editor [noarguments]", "Releases graph editing rights.", &BotControl::cmdNodeReleaseEditor);
      }
   }
   if (commands.exists (arg <StringRef> (cmd))) {
      const auto &item = commands[arg <StringRef> (cmd)];

      // graph have only bad format return status
      int status = (this->*item.handler) ();

      if (status == BotCommandResult::BadFormat) {
         msg ("Incorrect usage of \"%s %s %s\" command. Correct usage is:", m_args[root], m_args[alias], item.name);
         msg ("\n\t%s\n", item.format);
         msg ("Please use correct format.");
      }
   }
   else {
      if (arg <StringRef> (cmd) == "help" && hasArg (cmd2) && commands.exists (arg <StringRef> (cmd2))) {
         auto &item = commands[arg <StringRef> (cmd2)];

         msg ("Command: \"%s %s %s\"", m_args[root], m_args[alias], item.name);
         msg ("Format: %s", item.format);
         msg ("Help: %s", conf.translate (item.help));
      }
      else {
         for (auto &desc : descriptions) {
            auto &item = commands[desc];
            msg ("   %s - %s", item.name, conf.translate (item.help));
         }
         msg ("Currently Graph Status %s", graph.hasEditFlag (GraphEdit::On) ? "Enabled" : "Disabled");
      }
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeOn () {
   enum args { alias = 1, cmd, option };

   // enable various features of editor
   if (arg <StringRef> (option).empty () || arg <StringRef> (option) == "display" || arg <StringRef> (option) == "models") {
      graph.setEditFlag (GraphEdit::On);
      enableDrawModels (true);

      msg ("Graph editor has been enabled.");
   }
   else if (arg <StringRef> (option) == "noclip") {
      m_ent->v.movetype = MOVETYPE_NOCLIP;

      if (graph.hasEditFlag (GraphEdit::On)) {
         graph.setEditFlag (GraphEdit::Noclip);

         msg ("Noclip mode enabled.");
      }
      else {
         graph.setEditFlag (GraphEdit::On | GraphEdit::Noclip);
         enableDrawModels (true);

         msg ("Graph editor has been enabled with noclip mode.");
      }
   }
   else if (arg <StringRef> (option) == "auto") {
      if (graph.hasEditFlag (GraphEdit::On)) {
         graph.setEditFlag (GraphEdit::Auto);

         msg ("Enabled auto nodes placement.");
      }
      else {
         graph.setEditFlag (GraphEdit::On | GraphEdit::Auto);
         enableDrawModels (true);

         msg ("Graph editor has been enabled with auto add node mode.");
      }
   }

   if (graph.hasEditFlag (GraphEdit::On)) {
      m_graphSaveVarValues.roundtime = mp_roundtime.as <float> ();
      m_graphSaveVarValues.freezetime = mp_freezetime.as <float> ();
      m_graphSaveVarValues.timelimit = mp_timelimit.as <float> ();

      mp_roundtime.set (9);
      mp_freezetime.set (0);
      mp_timelimit.set (0);

      if (game.is (GameFlags::ReGameDLL)) {
         ConVarRef mp_round_infinite ("mp_round_infinite");

         if (mp_round_infinite.exists ()) {
            mp_round_infinite.set ("1");
         }
      }
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeOff () {
   enum args { graph_cmd = 1, cmd, option };

   // enable various features of editor
   if (arg <StringRef> (option).empty () || arg <StringRef> (option) == "display") {
      graph.clearEditFlag (GraphEdit::On | GraphEdit::Auto | GraphEdit::Noclip);
      enableDrawModels (false);

      // revert cvars back to their values
      mp_roundtime.set (m_graphSaveVarValues.roundtime);
      mp_freezetime.set (m_graphSaveVarValues.freezetime);
      mp_timelimit.set (m_graphSaveVarValues.timelimit);

      if (game.is (GameFlags::ReGameDLL)) {
         ConVarRef mp_round_infinite ("mp_round_infinite");

         if (mp_round_infinite.exists ()) {
            mp_round_infinite.set ("0");
         }
      }
      msg ("Graph editor has been disabled.");
   }
   else if (arg <StringRef> (option) == "models") {
      enableDrawModels (false);

      msg ("Graph editor has disabled spawn points highlighting.");
   }
   else if (arg <StringRef> (option) == "noclip") {
      m_ent->v.movetype = MOVETYPE_WALK;
      graph.clearEditFlag (GraphEdit::Noclip);

      msg ("Graph editor has disabled noclip mode.");
   }
   else if (arg <StringRef> (option) == "auto") {
      graph.clearEditFlag (GraphEdit::Auto);
      msg ("Graph editor has disabled auto add node mode.");
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeAdd () {
   enum args { graph_cmd = 1, cmd };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   // show the menu
   showMenu (Menu::NodeType);
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeAddBasic () {
   enum args { graph_cmd = 1, cmd };
   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   graph.addBasic ();
   msg ("Basic graph nodes was added.");

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeSave () {
   enum args { graph_cmd = 1, cmd, option };

   // if no check is set save anyway
   if (arg <StringRef> (option) == "nocheck") {
      graph.saveGraphData ();

      msg ("All nodes has been saved and written to disk (IGNORING QUALITY CONTROL).");
   }
   else if (arg <StringRef> (option) == "old" || arg <StringRef> (option) == "oldformat") {
      if (graph.length () >= 1024) {
         msg ("Unable to save POD-Bot Format waypoint file. Number of nodes exceeds 1024.");

         return BotCommandResult::Handled;
      }
      graph.saveOldFormat ();

      msg ("All nodes has been saved and written to disk (POD-Bot Format (.pwf)).");
   }
   else {
      if (graph.checkNodes (false)) {
         graph.saveGraphData ();
         msg ("All nodes has been saved and written to disk.\n*** Please don't forget to share your work by typing \"%s g upload\". Thank you! ***", product.cmdPri);
      }
      else {
         msg ("Could not save nodes to disk. Graph check has failed.");
      }
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeLoad () {
   enum args { graph_cmd = 1, cmd };

   // just save graph on request
   if (graph.loadGraphData ()) {
      msg ("Graph successfully loaded.");
   }
   else {
      msg ("Could not load Graph. See console...");
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeErase () {
   enum args { graph_cmd = 1, cmd, iamsure };

   // prevent accidents when graph are deleted unintentionally
   if (arg <StringRef> (iamsure) == "iamsure") {
      bstor.unlinkFromDisk (false, false);
   }
   else {
      msg ("Please, append \"iamsure\" as parameter to get graph erased from the disk.");
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeEraseTraining () {
   enum args { graph_cmd = 1, cmd };

   bstor.unlinkFromDisk (true, false);

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeDelete () {
   enum args { graph_cmd = 1, cmd, nearest };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   // if "nearest" or nothing passed delete nearest, else delete by index
   if (arg <StringRef> (nearest).empty () || arg <StringRef> (nearest) == "nearest") {
      graph.erase (kInvalidNodeIndex);
   }
   else {
      const auto index = arg <int> (nearest);

      // check for existence
      if (graph.exists (index)) {
         graph.erase (index);
         msg ("Node %d has been deleted.", index);
      }
      else {
         msg ("Could not delete node %d.", index);
      }
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeCheck () {
   enum args { graph_cmd = 1, cmd };

   // check if nodes are ok
   if (graph.checkNodes (true)) {
      msg ("Graph seems to be OK.");
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeCache () {
   enum args { graph_cmd = 1, cmd, nearest };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   // if "nearest" or nothing passed delete nearest, else delete by index
   if (arg <StringRef> (nearest).empty () || arg <StringRef> (nearest) == "nearest") {
      graph.cachePoint (kInvalidNodeIndex);
   }
   else {
      const int index = arg <int> (nearest);

      // check for existence
      if (graph.exists (index)) {
         graph.cachePoint (index);
      }
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeClean () {
   enum args { graph_cmd = 1, cmd, option };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   // if "all" passed clean up all the paths
   if (arg <StringRef> (option) == "all") {
      int removed = 0;

      for (auto i = 0; i < graph.length (); ++i) {
         removed += graph.clearConnections (i);
      }
      msg ("Done. Processed %d nodes. %d useless paths was cleared.", graph.length (), removed);
   }
   else if (arg <StringRef> (option).empty () || arg <StringRef> (option) == "nearest") {
      int removed = graph.clearConnections (graph.getEditorNearest ());

      msg ("Done. Processed node %d. %d useless paths was cleared.", graph.getEditorNearest (), removed);
   }
   else {
      const int index = arg <int> (option);

      // check for existence
      if (graph.exists (index)) {
         const int removed = graph.clearConnections (index);

         msg ("Done. Processed node %d. %d useless paths was cleared.", index, removed);
      }
      else {
         msg ("Could not process node %d clearance.", index);
      }
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeSetRadius () {
   enum args { graph_cmd = 1, cmd, radius, index };

   // radius is a must
   if (!hasArg (radius)) {
      return BotCommandResult::BadFormat;
   }
   int radiusIndex = kInvalidNodeIndex;

   if (arg <StringRef> (index).empty () || arg <StringRef> (index) == "nearest") {
      radiusIndex = graph.getEditorNearest ();
   }
   else {
      radiusIndex = arg <int> (index);
   }
   graph.setRadius (radiusIndex, arg <StringRef> (radius).as <float> ());

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeSetFlags () {
   enum args { graph_cmd = 1, cmd };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   //show the flag menu
   showMenu (Menu::NodeFlag);
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeTeleport () {
   enum args { graph_cmd = 1, cmd, teleport_index };

   if (!hasArg (teleport_index)) {
      return BotCommandResult::BadFormat;
   }
   int index = arg <int> (teleport_index);

   // check for existence
   if (graph.exists (index)) {
      engfuncs.pfnSetOrigin (graph.getEditor (), graph[index].origin);

      msg ("You have been teleported to node %d.", index);

      // turn graph on
      graph.setEditFlag (GraphEdit::On | GraphEdit::Noclip);
   }
   else {
      msg ("Could not teleport to node %d.", index);
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodePathCreate () {
   enum args { graph_cmd = 1, cmd };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   // choose the direction for path creation
   if (arg <StringRef> (cmd).endsWith ("_jump")) {
      graph.pathCreate (PathConnection::Jumping);
   }
   else if (arg <StringRef> (cmd).endsWith ("_both")) {
      graph.pathCreate (PathConnection::Bidirectional);
   }
   else if (arg <StringRef> (cmd).endsWith ("_in")) {
      graph.pathCreate (PathConnection::Incoming);
   }
   else if (arg <StringRef> (cmd).endsWith ("_out")) {
      graph.pathCreate (PathConnection::Outgoing);
   }
   else {
      showMenu (Menu::NodePath);
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodePathDelete () {
   enum args { graph_cmd = 1, cmd };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   // delete the path
   graph.erasePath ();

   return BotCommandResult::Handled;
}

int BotControl::cmdNodePathSetAutoDistance () {
   enum args { graph_cmd = 1, cmd };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);
   showMenu (Menu::NodeAutoPath);

   return BotCommandResult::Handled;
}

int BotControl::cmdNodePathCleanAll () {
   enum args { graph_cmd = 1, cmd, index };

   auto requestedNode = kInvalidNodeIndex;

   if (hasArg (index)) {
      requestedNode = arg <int> (index);
   }
   graph.resetPath (requestedNode);

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeAcquireEditor () {
   enum args { graph_cmd = 1 };

   if (game.isNullEntity (m_ent)) {
      msg ("This command should not be executed from HLDS console.");
      return BotCommandResult::Handled;
   }

   if (graph.hasEditor ()) {
      msg ("Sorry, players \"%s\" already acquired rights to edit graph on this server.", graph.getEditor ()->v.netname.chars ());
      return BotCommandResult::Handled;
   }
   graph.setEditor (m_ent);
   msg ("You're acquired rights to edit graph on this server. You're now able to use graph commands.");

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeReleaseEditor () {
   enum args { graph_cmd = 1 };

   if (!graph.hasEditor ()) {
      msg ("No one is currently has rights to edit. Nothing to release.");
      return BotCommandResult::Handled;
   }
   graph.setEditor (nullptr);
   msg ("Graph editor rights freed. You're now not able to use graph commands.");

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeUpload () {
   enum args { graph_cmd = 1, cmd };

   // do not allow to upload analyzed graphs
   if (graph.isAnalyzed ()) {
      msg ("Sorry, unable to upload graph that was generated automatically.");
      return BotCommandResult::Handled;
   }

   // do not allow to upload bad graph
   if (!graph.checkNodes (false)) {
      msg ("Sorry, unable to upload graph file that contains errors. Please type \"graph check\" to verify graph consistency.");
      return BotCommandResult::Handled;
   }
   String uploadUrlAddress = cv_graph_url_upload.as <StringRef> ();

   // only allow to upload to non-https endpoint
   if (uploadUrlAddress.startsWith ("https")) {
      msg ("Value of \"%s\" cvar should not contain URL scheme, only the host name and path.", cv_graph_url_upload.name ());
      return BotCommandResult::Handled;
   }
   String uploadUrl = strings.format ("%s://%s", product.httpScheme, uploadUrlAddress);

   msg ("\n");
   msg ("WARNING!");
   msg ("Graph uploaded to graph database in synchronous mode. That means if graph is big enough");
   msg ("you may notice the game freezes a bit during upload and issue request creation. Please, be patient.");
   msg ("\n");

   // try to upload the file
   if (http.uploadFile (uploadUrl, bstor.buildPath (BotFile::Graph))) {
      msg ("Graph file was successfully validated and uploaded to the YaPB Graph DB (%s).", product.download);
      msg ("It will be available for download for all YaPB users in a few minutes.");
      msg ("\n");
      msg ("Thank you.");
      msg ("\n");
   }
   else {
      String status {};
      auto code = http.getLastStatusCode ();

      if (code == HttpClientResult::Forbidden) {
         status = "AlreadyExists";
      }
      else if (code == HttpClientResult::NotFound) {
         status = "AccessDenied";
      }
      else {
         status.assignf ("%d", code);
      }
      msg ("Something went wrong with uploading. Come back later. (%s)", status);
      msg ("\n");

      if (code == HttpClientResult::Forbidden) {
         msg ("You should create issue-request manually for this graph");
         msg ("as it's already exists in database, can't overwrite. Sorry...");
      }
      else {
         msg ("There is an internal error, or something is totally wrong with");
         msg ("your files, and they are not passed sanity checks. Sorry...");
      }
      msg ("\n");
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeIterateCamp () {
   enum args { graph_cmd = 1, cmd, option };

   // turn graph on
   graph.setEditFlag (GraphEdit::On);

   // get the option describing operation
   auto op = arg <StringRef> (option);

   if (op != "begin" && op != "end" && op != "next") {
      return BotCommandResult::BadFormat;
   }

   if ((op == "next" || op == "end") && m_campIterator.empty ()) {
      msg ("Before calling for 'next' / 'end' camp point, you should hit 'begin'.");
      return BotCommandResult::Handled;
   }
   else if (op == "begin" && !m_campIterator.empty ()) {
      msg ("Before calling for 'begin' camp point, you should hit 'end'.");
      return BotCommandResult::Handled;
   }

   if (op == "end") {
      m_campIterator.clear ();
   }
   else if (op == "next") {
      if (!m_campIterator.empty ()) {
         Vector origin = graph[m_campIterator.first ()].origin;

         if (graph[m_campIterator.first ()].flags & NodeFlag::Crouch) {
            origin.z += 23.0f;
         }
         engfuncs.pfnSetOrigin (m_ent, origin);

         // go to next
         m_campIterator.shift ();

         if (m_campIterator.empty ()) {
            msg ("Finished iterating camp spots.");
         }
      }
   }
   else if (op == "begin") {
      for (const auto &path : graph) {
         if (path.flags & NodeFlag::Camp) {
            m_campIterator.push (path.number);
         }
      }
      if (!m_campIterator.empty ()) {
         msg ("Ready for iteration. Type 'next' to go to first camp node.");
         return BotCommandResult::Handled;
      }
      msg ("Unable to begin iteration, camp points is not set.");
   }
   return BotCommandResult::Handled;
}

int BotControl::cmdNodeShowStats () {
   graph.showStats ();

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeFileInfo () {
   graph.showFileInfo ();

   return BotCommandResult::Handled;
}

int BotControl::cmdNodeAdjustHeight () {
   enum args { graph_cmd = 1, cmd, offset };

   if (!hasArg (offset)) {
      return BotCommandResult::BadFormat;
   }
   auto heightOffset = arg <float> (offset);

   // adjust the height for all the nodes (negative values possible)
   for (auto &path : graph) {
      path.origin.z += heightOffset;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuMain (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
      m_isMenuFillCommand = false;
      showMenu (Menu::Control);
      break;

   case 2:
      showMenu (Menu::Features);
      break;

   case 3:
      m_isMenuFillCommand = true;
      showMenu (Menu::TeamSelect);
      break;

   case 4:
      if (game.is (GameFlags::ReGameDLL) && !cv_bots_kill_on_endround) {
         game.serverCommand ("endround");
      }
      else {
         bots.killAllBots ();
      }
      break;

   case 10:
      closeMenu ();
      break;

   default:
      showMenu (Menu::Main);
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuFeatures (int item) {
   closeMenu (); // reset menu display

   auto autoAcquireEditorRights = [&] () {
      if (!graph.hasEditor ()) {
         graph.setEditor (m_ent);
      }
      return graph.hasEditor () && graph.getEditor () == m_ent ? Menu::NodeMainPage1 : Menu::Features;
   };

   switch (item) {
   case 1:
      showMenu (Menu::WeaponMode);
      break;

   case 2:
      showMenu (autoAcquireEditorRights ());
      break;

   case 3:
      showMenu (Menu::Personality);
      break;

   case 4:
      cv_debug.set (cv_debug.as <int> () ^ 1);

      showMenu (Menu::Features);
      break;

   case 5:
      if (util.isAlive (m_ent)) {
         showMenu (Menu::Commands);
      }
      else {
         closeMenu (); // reset menu display
         msg ("You're dead, and have no access to this menu");
      }
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuControl (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
      bots.createRandom (true);
      showMenu (Menu::Control);
      break;

   case 2:
      showMenu (Menu::Difficulty);
      break;

   case 3:
      bots.kickRandom ();
      showMenu (Menu::Control);
      break;

   case 4:
      bots.kickEveryone ();
      break;

   case 5:
      kickBotByMenu (1);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuWeaponMode (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
      bots.setWeaponMode (item);
      showMenu (Menu::WeaponMode);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuPersonality (int item) {
   if (m_isMenuFillCommand) {
      closeMenu (); // reset menu display

      switch (item) {
      case 1:
      case 2:
      case 3:
      case 4:
         bots.serverFill (m_menuServerFillTeam, item - 2, m_interMenuData[0]);
         closeMenu ();
         break;

      case 10:
         closeMenu ();
         break;
      }
      return BotCommandResult::Handled;
   }
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 3:
   case 4:
      m_interMenuData[3] = item - 2;
      showMenu (Menu::TeamSelect);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuDifficulty (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
      m_interMenuData[0] = 0;
      break;

   case 2:
      m_interMenuData[0] = 1;
      break;

   case 3:
      m_interMenuData[0] = 2;
      break;

   case 4:
      m_interMenuData[0] = 3;
      break;

   case 5:
      m_interMenuData[0] = 4;
      break;

   case 10:
      closeMenu ();
      break;
   }
   showMenu (Menu::Personality);

   return BotCommandResult::Handled;
}

int BotControl::menuTeamSelect (int item) {
   if (m_isMenuFillCommand) {
      closeMenu (); // reset menu display

      if (item < 3) {
         // turn off cvars if specified team
         mp_limitteams.set (0);
         mp_autoteambalance.set (0);
      }

      switch (item) {
      case 1:
      case 2:
      case 5:
         m_menuServerFillTeam = item;
         showMenu (Menu::Difficulty);
         break;

      case 10:
         closeMenu ();
         break;
      }
      return BotCommandResult::Handled;
   }
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 5:
      m_interMenuData[1] = item;

      if (item == 5) {
         m_interMenuData[2] = item;
         bots.addbot ("", m_interMenuData[0], m_interMenuData[3], m_interMenuData[1], m_interMenuData[2], true);
      }
      else if (game.is (GameFlags::ConditionZero)) {
         showMenu (item == 1 ? Menu::TerroristSelectCZ : Menu::CTSelectCZ);
      }
      else {
         showMenu (item == 1 ? Menu::TerroristSelect : Menu::CTSelect);
      }
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuClassSelect (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
      m_interMenuData[2] = item;
      bots.addbot ("", m_interMenuData[0], m_interMenuData[3], m_interMenuData[1], m_interMenuData[2], true);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuCommands (int item) {
   closeMenu (); // reset menu display
   Bot *nearest = nullptr;

   switch (item) {
   case 1:
   case 2:
      if (util.findNearestPlayer (reinterpret_cast <void **> (&m_djump), m_ent, 600.0f, true, true, true, true, false)
         && !m_djump->m_hasC4
         && !m_djump->m_hasHostage) {

         if (item == 1) {
            m_djump->startDoubleJump (m_ent);
         }
         else {
            if (m_djump) {
               m_djump->resetDoubleJump ();
               m_djump = nullptr;
            }
         }
      }
      showMenu (Menu::Commands);
      break;

   case 3:
   case 4:
      if (util.findNearestPlayer (reinterpret_cast <void **> (&nearest), m_ent, 600.0f, true, true, true, true, item == 4 ? false : true)) {
         nearest->dropWeaponForUser (m_ent, item == 4 ? false : true);
      }
      showMenu (Menu::Commands);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuGraphPage1 (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
      if (graph.hasEditFlag (GraphEdit::On)) {
         graph.clearEditFlag (GraphEdit::On);
         enableDrawModels (false);

         msg ("Graph editor has been disabled.");
      }
      else {
         graph.setEditFlag (GraphEdit::On);
         enableDrawModels (true);

         msg ("Graph editor has been enabled.");
      }
      showMenu (Menu::NodeMainPage1);
      break;

   case 2:
      graph.setEditFlag (GraphEdit::On);
      graph.cachePoint (kInvalidNodeIndex);

      showMenu (Menu::NodeMainPage1);
      break;

   case 3:
      graph.setEditFlag (GraphEdit::On);
      showMenu (Menu::NodePath);
      break;

   case 4:
      graph.setEditFlag (GraphEdit::On);
      graph.erasePath ();

      showMenu (Menu::NodeMainPage1);
      break;

   case 5:
      graph.setEditFlag (GraphEdit::On);
      showMenu (Menu::NodeType);
      break;

   case 6:
      graph.setEditFlag (GraphEdit::On);
      graph.erase (kInvalidNodeIndex);

      showMenu (Menu::NodeMainPage1);
      break;

   case 7:
      graph.setEditFlag (GraphEdit::On);
      showMenu (Menu::NodeAutoPath);
      break;

   case 8:
      graph.setEditFlag (GraphEdit::On);
      showMenu (Menu::NodeRadius);
      break;

   case 9:
      showMenu (Menu::NodeMainPage2);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuGraphPage2 (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
      graph.setEditFlag (GraphEdit::On);
      showMenu (Menu::NodeDebug);
      break;

   case 2:
      graph.setEditFlag (GraphEdit::On);

      if (graph.hasEditFlag (GraphEdit::Auto)) {
         graph.clearEditFlag (GraphEdit::Auto);
      }
      else {
         graph.setEditFlag (GraphEdit::Auto);
      }

      if (graph.hasEditFlag (GraphEdit::Auto)) {
         msg ("Enabled auto nodes placement.");
      }
      else {
         msg ("Disabled auto nodes placement.");
      }
      showMenu (Menu::NodeMainPage2);
      break;

   case 3:
      graph.setEditFlag (GraphEdit::On);
      showMenu (Menu::NodeFlag);
      break;

   case 4:
      if (graph.checkNodes (true)) {
         graph.saveGraphData ();
         msg ("Graph successfully saved.");
      }
      else {
         msg ("Graph not saved. There are errors, see console...");
      }
      showMenu (Menu::NodeMainPage2);
      break;

   case 5:
      if (graph.saveGraphData ()) {
         msg ("Graph successfully saved.");
      }
      else {
         msg ("Could not save Graph. See console...");
      }
      showMenu (Menu::NodeMainPage2);
      break;

   case 6:
      if (graph.loadGraphData ()) {
         msg ("Graph successfully loaded.");
      }
      else {
         msg ("Could not load Graph. See console...");
      }
      showMenu (Menu::NodeMainPage2);
      break;

   case 7:
      if (graph.checkNodes (true)) {
         msg ("Nodes works fine");
      }
      else {
         msg ("There are errors, see console");
      }
      showMenu (Menu::NodeMainPage2);
      break;

   case 8:
      graph.setEditFlag (GraphEdit::On);

      if (graph.hasEditFlag (GraphEdit::Noclip)) {
         graph.clearEditFlag (GraphEdit::Noclip);
         msg ("Noclip mode disabled.");
      }
      else {
         graph.setEditFlag (GraphEdit::Noclip);
         msg ("Noclip mode enabled.");
      }
      showMenu (Menu::NodeMainPage2);

      // update editor movetype based on flag
      m_ent->v.movetype = graph.hasEditFlag (GraphEdit::Noclip) ? MOVETYPE_NOCLIP : MOVETYPE_WALK;

      break;

   case 9:
      showMenu (Menu::NodeMainPage1);
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuGraphRadius (int item) {
   closeMenu (); // reset menu display
   graph.setEditFlag (GraphEdit::On); // turn graph on in case

   if (item >= 1 && item <= 9) {
      constexpr float kRadiusValues[] = { 0.0f, 8.0f, 16.0f, 32.0f, 48.0f, 64.0f, 80.0f, 96.0f, 128.0f };

      graph.setRadius (kInvalidNodeIndex, kRadiusValues[item - 1]);
      showMenu (Menu::NodeRadius);
   }
   return BotCommandResult::Handled;
}

int BotControl::menuGraphType (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
      graph.add (item - 1);
      showMenu (Menu::NodeType);
      break;

   case 8:
      graph.add (NodeAddFlag::Goal);
      showMenu (Menu::NodeType);
      break;

   case 9:
      graph.startLearnJump ();
      showMenu (Menu::NodeType);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuGraphDebug (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
      cv_debug_goal.set (graph.getEditorNearest ());
      if (cv_debug_goal.as <int> () != kInvalidNodeIndex) {
         msg ("Debug goal is set to node %d.", cv_debug_goal.as <int> ());
      }
      else {
         msg ("Cannot find the node. Debug goal is disabled.");
      }
      showMenu (Menu::NodeDebug);
      break;

   case 2:
      cv_debug_goal.set (graph.getFacingIndex ());
      if (cv_debug_goal.as <int> () != kInvalidNodeIndex) {
         msg ("Debug goal is set to node %d.", cv_debug_goal.as <int> ());
      }
      else {
         msg ("Cannot find the node. Debug goal is disabled.");
      }
      showMenu (Menu::NodeDebug);
      break;

   case 3:
      cv_debug_goal.set (kInvalidNodeIndex);
      msg ("Debug goal is disabled.");
      showMenu (Menu::NodeDebug);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuGraphFlag (int item) {
   closeMenu (); // reset menu display
   int nearest = graph.getEditorNearest ();

   switch (item) {
   case 1:
      graph.toggleFlags (NodeFlag::NoHostage);
      showMenu (Menu::NodeFlag);
      break;

   case 2:
      if (graph[nearest].flags & NodeFlag::CTOnly) {
         graph.toggleFlags (NodeFlag::CTOnly);
         graph.toggleFlags (NodeFlag::TerroristOnly);
      }
      else {
         graph.toggleFlags (NodeFlag::TerroristOnly);
      }
      showMenu (Menu::NodeFlag);
      break;

   case 3:
      if (graph[nearest].flags & NodeFlag::TerroristOnly) {
         graph.toggleFlags (NodeFlag::TerroristOnly);
         graph.toggleFlags (NodeFlag::CTOnly);
      }
      else {
         graph.toggleFlags (NodeFlag::CTOnly);
      }
      showMenu (Menu::NodeFlag);
      break;

   case 4:
      graph.toggleFlags (NodeFlag::Lift);
      showMenu (Menu::NodeFlag);
      break;

   case 5:
      graph.toggleFlags (NodeFlag::Sniper);
      showMenu (Menu::NodeFlag);
      break;

   case 6:
      graph.toggleFlags (NodeFlag::Goal);
      showMenu (Menu::NodeFlag);
      break;

   case 7:
      graph.toggleFlags (NodeFlag::Rescue);
      showMenu (Menu::NodeFlag);
      break;

   case 8:
      if (graph[nearest].flags != NodeFlag::Crouch) {
         graph.toggleFlags (NodeFlag::Crouch);
         graph[nearest].origin.z += -18.0f;
      }
      else {
         graph.toggleFlags (NodeFlag::Crouch);
         graph[nearest].origin.z += 18.0f;
      }

      showMenu (Menu::NodeFlag);
      break;

   case 9:
      // if the node doesn't have a camp flag, set it and open the camp directions selection menu
      if (!(graph[nearest].flags & NodeFlag::Camp)) {
         graph.toggleFlags (NodeFlag::Camp);
         showMenu (Menu::CampDirections);
         break;
      }
      // otherwise remove the flag, and don't show the camp directions selection menu
      else {
         graph.toggleFlags (NodeFlag::Camp);
         showMenu (Menu::NodeFlag);
         break;
      }
   }
   return BotCommandResult::Handled;
}

int BotControl::menuCampDirections (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
      graph.add (NodeAddFlag::Camp);
      showMenu (Menu::CampDirections);
      break;

   case 2:
      graph.add (NodeAddFlag::CampEnd);
      showMenu (Menu::CampDirections);
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuAutoPathDistance (int item) {
   closeMenu (); // reset menu display

   if (item >= 1 && item <= 7) {
      constexpr float kDistanceValues[] = { 0.0f, 100.0f, 130.0f, 160.0f, 190.0f, 220.0f, 250.0f };

      graph.setAutoPathDistance (kDistanceValues[item - 1]);
   }

   switch (item) {
   default:
      showMenu (Menu::NodeAutoPath);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuKickPage1 (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
   case 8:
      bots.kickBot (item - 1);
      kickBotByMenu (1);
      break;

   case 9:
      kickBotByMenu (2);
      break;

   case 10:
      showMenu (Menu::Control);
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuKickPage2 (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
   case 8:
      bots.kickBot (item + 8 - 1);
      kickBotByMenu (2);
      break;

   case 9:
      kickBotByMenu (3);
      break;

   case 10:
      kickBotByMenu (1);
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuKickPage3 (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
   case 8:
      bots.kickBot (item + 16 - 1);
      kickBotByMenu (3);
      break;

   case 9:
      kickBotByMenu (4);
      break;

   case 10:
      kickBotByMenu (2);
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuKickPage4 (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
   case 8:
      bots.kickBot (item + 24 - 1);
      kickBotByMenu (4);
      break;

   case 10:
      kickBotByMenu (3);
      break;
   }
   return BotCommandResult::Handled;
}

int BotControl::menuGraphPath (int item) {
   closeMenu (); // reset menu display

   switch (item) {
   case 1:
      graph.pathCreate (PathConnection::Outgoing);
      showMenu (Menu::NodePath);
      break;

   case 2:
      graph.pathCreate (PathConnection::Incoming);
      showMenu (Menu::NodePath);
      break;

   case 3:
      graph.pathCreate (PathConnection::Bidirectional);
      showMenu (Menu::NodePath);
      break;

   case 4:
      graph.pathCreate (PathConnection::Jumping);
      showMenu (Menu::NodePath);
      break;

   case 10:
      closeMenu ();
      break;
   }
   return BotCommandResult::Handled;
}

bool BotControl::executeCommands () {
   if (m_args.empty ()) {
      return false;
   }
   const auto &prefix = m_args.first ();

   // no handling if not for us
   if (prefix != product.cmdPri && prefix != product.cmdSec) {
      return false;
   }
   const auto &client = util.getClient (game.indexOfPlayer (m_ent));

   // do not allow to execute stuff for non admins
   if (m_ent != game.getLocalEntity () && !(client.flags & ClientFlags::Admin)) {
      msg ("Access to %s commands is restricted.", product.name);

      // reset issuer, but returns "true" to suppress "unknown command" message
      setIssuer (nullptr);

      return true;
   }

   auto aliasMatch = [] (String &test, const String &cmd, String &aliasName) -> bool {
      for (auto &alias : test.split ("/")) {
         if (alias == cmd) {
            aliasName = alias;
            return true;
         }
      }
      return false;
   };

   String cmd {};

   // give some help
   if (hasArg (1) && arg <StringRef> (1) == "help") {
      const auto hasSecondArg = hasArg (2);

      for (auto &item : m_cmds) {
         if (!hasSecondArg) {
            cmd = item.name.split ("/").first ();
         }

         if (!hasSecondArg || aliasMatch (item.name, arg <StringRef> (2), cmd)) {
            msg ("Command: \"%s %s\"", prefix, cmd);
            msg ("Format: %s", item.format);
            msg ("Help: %s", conf.translate (item.help));

            auto aliases = item.name.split ("/");

            if (aliases.length () > 1) {
               msg ("Aliases: %s", String::join (aliases, ", "));
            }

            if (hasSecondArg) {
               return true;
            }
            else {
               msg ("\n");
            }
         }
      }

      if (!hasSecondArg) {
         return true;
      }
      else {
         msg ("No help found for \"%s\"", arg <StringRef> (2));
      }
      return true;
   }
   cmd.clear ();

   // if no args passed just print all the commands
   if (m_args.length () == 1) {
      msg ("usage %s <command> [arguments]", prefix);
      msg ("valid commands are: ");

      for (auto &item : m_cmds) {
         if (!item.visible) {
            continue;
         }
         msg ("  %-14.11s - %s", item.name.split ("/").first (), String (conf.translate (item.help)).lowercase ());
      }
      return true;
   }

   // first search for a actual cmd
   for (auto &item : m_cmds) {
      if (aliasMatch (item.name, m_args[1], cmd)) {
         switch ((this->*item.handler) ()) {
         case BotCommandResult::Handled:
         default:
            break;

         case BotCommandResult::ListenServer:
            msg ("Command \"%s %s\" is only available from the listenserver console.", prefix, cmd);
            break;

         case BotCommandResult::BadFormat:
            msg ("Incorrect usage of \"%s %s\" command. Correct usage is:", prefix, cmd);
            msg ("\n\t%s\n", item.format);
            msg ("Please type \"%s help %s\" to get more information.", prefix, cmd);
            break;
         }

         m_isFromConsole = false;
         return true;
      }
   }
   msg ("Unknown command: %s", m_args[1]);

   // clear all the arguments upon finish
   m_args.clear ();

   return true;
}

bool BotControl::executeMenus () {
   if (!util.isPlayer (m_ent) || game.isBotCmd ()) {
      return false;
   }
   const auto &issuer = util.getClient (game.indexOfPlayer (m_ent));

   // check if it's menu select, and some key pressed
   if (arg <StringRef> (0) != "menuselect" || arg <StringRef> (1).empty () || issuer.menu == Menu::None) {
      return false;
   }

   // let's get handle
   for (auto &menu : m_menus) {
      if (menu.ident == issuer.menu) {
         return (this->*menu.handler) (arg <StringRef> (1).as <int> ()) == BotCommandResult::Handled;
      }
   }
   return false;
}

void BotControl::showMenu (int id) {
   static bool menusParsed = false;

   // make menus looks like we need only once
   if (!menusParsed) {
      m_ignoreTranslate = false; // always translate menus

      for (auto &parsed : m_menus) {
         StringRef translated = conf.translate (parsed.text);

         // translate all the things
         parsed.text = translated;

         // make menu looks best
         if (!game.is (GameFlags::Legacy)) {
            for (int j = 0; j < 10; ++j) {
               parsed.text.replace (strings.format ("%d.", j), strings.format ("\\r%d.\\w", j));
            }
         }
      }
      menusParsed = true;
   }

   if (!util.isPlayer (m_ent)) {
      return;
   }
   auto &client = util.getClient (game.indexOfPlayer (m_ent));

   auto sendMenu = [&] (int32_t slots, bool last, StringRef text) {
      MessageWriter (MSG_ONE, msgs.id (NetMsg::ShowMenu), nullptr, m_ent)
         .writeShort (slots)
         .writeChar (-1)
         .writeByte (last ? HLFalse : HLTrue)
         .writeString (text.chars ());
   };

   constexpr size_t kMaxMenuSentLength = 140;

   for (const auto &display : m_menus) {
      if (display.ident == id) {
         String text = (game.is (GameFlags::Xash3D | GameFlags::Mobility) && !cv_display_menu_text) ? " " : display.text.chars ();

         // split if needed
         if (text.length () > kMaxMenuSentLength) {
            auto chunks = text.split (kMaxMenuSentLength);

            // send in chunks
            for (size_t i = 0; i < chunks.length (); ++i) {
               sendMenu (display.slots, i == chunks.length () - 1, chunks[i]);
            }
         }
         else {
            sendMenu (display.slots, true, text);
         }

         client.menu = id;
         engfuncs.pfnClientCommand (m_ent, "speak \"player/geiger1\"\n"); // stops others from hearing menu sounds..

         break;
      }
   }
}

void BotControl::closeMenu () {
   if (!util.isPlayer (m_ent)) {
      return;
   }
   auto &client = util.getClient (game.indexOfPlayer (m_ent));

   // do not reset menu if already none
   if (client.menu == Menu::None) {
      return;
   }

   MessageWriter (MSG_ONE, msgs.id (NetMsg::ShowMenu), nullptr, m_ent)
      .writeShort (0)
      .writeChar (0)
      .writeByte (0)
      .writeString ("");

   client.menu = Menu::None;
}

void BotControl::kickBotByMenu (int page) {
   if (page > 4 || page < 1) {
      return;
   }

   static StringRef headerTitle = conf.translate ("Bot Removal Menu");
   static StringRef notABot = conf.translate ("Not a bot");
   static StringRef backKey = conf.translate ("Back");
   static StringRef moreKey = conf.translate ("More");

   String menus {};
   menus.assignf ("\\y%s (%d/4):\\w\n\n", headerTitle, page);

   int menuKeys = (page == 4) ? cr::bit (9) : (cr::bit (8) | cr::bit (9));
   int menuKey = (page - 1) * 8;

   for (int i = menuKey; i < page * 8; ++i) {
      auto bot = bots[i];

      // check for dormant bit, since we're adds it upon kick, but actual bot struct destroyed after client disconnected
      if (bot != nullptr && !(bot->pev->flags & FL_DORMANT)) {
         menuKeys |= cr::bit (cr::abs (i - menuKey));
         menus.appendf ("%1.1d. %s%s\n", i - menuKey + 1, bot->pev->netname.chars (), bot->m_team == Team::CT ? " \\y(CT)\\w" : " \\r(T)\\w");
      }
      else {
         menus.appendf ("\\d %1.1d. %s\\w\n", i - menuKey + 1, notABot);
      }
   }
   menus.appendf ("\n%s 0. %s", (page == 4) ? "" : strings.format (" 9. %s...\n", moreKey), backKey);

   // force to clear current menu
   closeMenu ();

   auto id = Menu::KickPage1 - 1 + page;

   for (auto &menu : m_menus) {
      if (menu.ident == id) {
         menu.slots = static_cast <int> (static_cast <uint32_t> (menuKeys) & static_cast <uint32_t> (-1));
         menu.text = menus;

         break;
      }
   }
   showMenu (id);
}

void BotControl::assignAdminRights (edict_t *ent, char *infobuffer) {
   if (!game.isDedicated () || util.isFakeClient (ent)) {
      return;
   }
   StringRef key = cv_password_key.as <StringRef> ();
   StringRef password = cv_password.as <StringRef> ();

   if (!key.empty () && !password.empty ()) {
      auto &client = util.getClient (game.indexOfPlayer (ent));

      if (password == engfuncs.pfnInfoKeyValue (infobuffer, key.chars ())) {
         client.flags |= ClientFlags::Admin;
      }
      else {
         client.flags &= ~ClientFlags::Admin;
      }
   }
}

void BotControl::maintainAdminRights () {
   if (!game.isDedicated ()) {
      return;
   }

   StringRef key = cv_password_key.as <StringRef> ();
   StringRef password = cv_password.as <StringRef> ();

   for (auto &client : util.getClients ()) {
      if (!(client.flags & ClientFlags::Used) || util.isFakeClient (client.ent)) {
         continue;
      }
      auto ent = client.ent;

      if (client.flags & ClientFlags::Admin) {
         if (key.empty () || password.empty ()) {
            client.flags &= ~ClientFlags::Admin;
         }
         else if (password != engfuncs.pfnInfoKeyValue (engfuncs.pfnGetInfoKeyBuffer (ent), key.chars ())) {
            client.flags &= ~ClientFlags::Admin;
            msg ("Player %s had lost remote access to %s.", ent->v.netname.chars (), product.name);
         }
      }
      else if (!(client.flags & ClientFlags::Admin) && !key.empty () && !password.empty ()) {
         if (password == engfuncs.pfnInfoKeyValue (engfuncs.pfnGetInfoKeyBuffer (ent), key.chars ())) {
            client.flags |= ClientFlags::Admin;
            msg ("Player %s had gained full remote access to %s.", ent->v.netname.chars (), product.name);
         }
      }
   }
}

void BotControl::flushPrintQueue () {
   if (m_printQueueFlushTimestamp > game.time () || m_printQueue.empty ()) {
      return;
   }
   auto printable = m_printQueue.popFront ();

   // send to needed destination
   if (printable.destination == PrintQueueDestination::ServerConsole) {
      game.print (printable.text.chars ());
   }
   else if (!game.isNullEntity (m_ent)) {
      game.clientPrint (m_ent, printable.text.chars ());
   }
   m_printQueueFlushTimestamp = game.time () + 0.05f;
}

BotControl::BotControl () {
   m_ent = nullptr;
   m_djump = nullptr;

   m_denyCommands = true;
   m_ignoreTranslate = false;
   m_isFromConsole = false;
   m_isMenuFillCommand = false;
   m_rapidOutput = false;
   m_menuServerFillTeam = 5;
   m_printQueueFlushTimestamp = 0.0f;

   m_cmds.emplace ("add/addbot/add_ct/addbot_ct/add_t/addbot_t/addhs/addhs_t/addhs_ct", "add [difficulty] [personality] [team] [model] [name]", "Adding specific bot into the game.", &BotControl::cmdAddBot);
   m_cmds.emplace ("kick/kickone/kick_ct/kick_t/kickbot_ct/kickbot_t", "kick [team]", "Kicks off the random bot from the game.", &BotControl::cmdKickBot);
   m_cmds.emplace ("removebots/kickbots/kickall/kickall_ct/kickall_t", "removebots [instant] [team]", "Kicks all the bots from the game.", &BotControl::cmdKickBots);
   m_cmds.emplace ("kill/killbots/killall/kill_ct/kill_t", "kill [team] [silent]", "Kills the specified team / all the bots.", &BotControl::cmdKillBots);
   m_cmds.emplace ("fill/fillserver", "fill [team] [count] [difficulty] [personality]", "Fill the server (add bots) with specified parameters.", &BotControl::cmdFill);
   m_cmds.emplace ("vote/votemap", "vote [map_id]", "Forces all the bot to vote to specified map.", &BotControl::cmdVote);
   m_cmds.emplace ("weapons/weaponmode", "weapons [knife|pistol|shotgun|smg|rifle|sniper|standard]", "Sets the bots weapon mode to use", &BotControl::cmdWeaponMode);
   m_cmds.emplace ("menu/botmenu", "menu [cmd]", "Opens the main bot menu, or command menu if specified.", &BotControl::cmdMenu);
   m_cmds.emplace ("version/ver/about", "version [no arguments]", "Displays version information about bot build.", &BotControl::cmdVersion);
   m_cmds.emplace ("graphmenu/wpmenu/wptmenu", "graphmenu [noarguments]", "Opens and displays bots graph editor.", &BotControl::cmdNodeMenu);
   m_cmds.emplace ("list/listbots", "list [noarguments]", "Lists the bots currently playing on server.", &BotControl::cmdList);
   m_cmds.emplace ("graph/g/w/wp/wpt/waypoint", "graph [help]", "Handles graph operations.", &BotControl::cmdNode);
   m_cmds.emplace ("cvars", "cvars [save|save_map|cvar|defaults]", "Display all the cvars with their descriptions.", &BotControl::cmdCvars);
   m_cmds.emplace ("show_custom", "show_custom [noarguments]", "Shows the current values from custom.cfg.", &BotControl::cmdShowCustom, false);
   m_cmds.emplace ("exec", "exec [user_id] [command]", "Executes a client command on bot entity.", &BotControl::cmdExec);

   // declare the menus
   createMenus ();
}

void BotControl::handleEngineCommands () {
   if (m_denyCommands) {
      return;
   }

   collectArgs ();
   setIssuer (game.getLocalEntity ());

   setFromConsole (true);
   executeCommands ();
}

bool BotControl::handleClientSideCommandsWrapper (edict_t *ent, bool isMenus) {
   if (m_denyCommands) {
      return false;
   }

   collectArgs ();
   setIssuer (ent);

   setFromConsole (!isMenus);
   auto result = isMenus ? executeMenus () : executeCommands ();

   if (!result) {
      setIssuer (nullptr);
   }
   return result;
}

bool BotControl::handleClientCommands (edict_t *ent) {
   return handleClientSideCommandsWrapper (ent, false);
}

bool BotControl::handleMenuCommands (edict_t *ent) {
   return handleClientSideCommandsWrapper (ent, true);
}

void BotControl::enableDrawModels (bool enable) {
   static StringArray entities {
      "info_player_start", "info_player_deathmatch", "info_vip_start"
   };

   if (enable) {
      game.setPlayerStartDrawModels ();
   }

   for (auto &entity : entities) {
      game.searchEntities ("classname", entity, [&enable] (edict_t *ent) {
         if (enable) {
            ent->v.effects &= ~EF_NODRAW;
         }
         else {
            ent->v.effects |= EF_NODRAW;
         }
         return EntitySearchResult::Continue;
      });
   }
}

void BotControl::createMenus () {
   auto keys = [] (int numKeys) -> int {
      int result = 0;

      for (int i = 0; i < numKeys; ++i) {
         result |= cr::bit (i);
      }
      result |= cr::bit (9);

      return result;
   };

   // bots main menu
   m_menus.emplace (
      Menu::Main, keys (4),
      "\\yMain Menu\\w\n\n"
      "1. Control bots\n"
      "2. Features\n\n"
      "3. Fill server\n"
      "4. End round\n\n"
      "0. Exit",
      &BotControl::menuMain);


   // bots features menu
   m_menus.emplace (
      Menu::Features, keys (5),
      "\\yBots Features\\w\n\n"
      "1. Weapon mode menu\n"
      "2. Graph editor\n"
      "3. Select personality\n\n"
      "4. Toggle debug mode\n"
      "5. Command menu\n\n"
      "0. Exit",
      &BotControl::menuFeatures);

   // bot control menu
   m_menus.emplace (
      Menu::Control, keys (5),
      "\\yBots Control Menu\\w\n\n"
      "1. Quick add bot\n"
      "2. Add specific bot\n\n"
      "3. Remove random bot\n"
      "4. Remove all bots\n\n"
      "5. Bot removal menu\n\n"
      "0. Exit",
      &BotControl::menuControl);

   // weapon mode select menu
   m_menus.emplace (
      Menu::WeaponMode, keys (7),
      "\\yBots Weapon Mode\\w\n\n"
      "1. Knives only\n"
      "2. Pistols only\n"
      "3. Shotguns only\n"
      "4. Machine guns only\n"
      "5. Rifles only\n"
      "6. Sniper weapons only\n"
      "7. All weapons\n\n"
      "0. Exit",
      &BotControl::menuWeaponMode);

   // personality select menu
   m_menus.emplace (
      Menu::Personality, keys (4),
      "\\yBots Personality\\w\n\n"
      "1. Random\n"
      "2. Normal\n"
      "3. Aggressive\n"
      "4. Careful\n\n"
      "0. Exit",
      &BotControl::menuPersonality);

   // difficulty select menu
   m_menus.emplace (
      Menu::Difficulty, keys (5),
      "\\yBots Difficulty Level\\w\n\n"
      "1. Newbie\n"
      "2. Average\n"
      "3. Normal\n"
      "4. Professional\n"
      "5. Godlike\n\n"
      "0. Exit",
      &BotControl::menuDifficulty);

   // team select menu
   m_menus.emplace (
      Menu::TeamSelect, keys (5),
      "\\ySelect a Team\\w\n\n"
      "1. Terrorist Force\n"
      "2. Counter-Terrorist Force\n\n"
      "5. Auto-select\n\n"
      "0. Exit",
      &BotControl::menuTeamSelect);

   // terrorist model select menu
   m_menus.emplace (
      Menu::TerroristSelect, keys (5),
      "\\ySelect an Appearance\\w\n\n"
      "1. Phoenix Connexion\n"
      "2. L337 Krew\n"
      "3. Arctic Avengers\n"
      "4. Guerilla Warfare\n\n"
      "5. Auto-select\n\n"
      "0. Exit",
      &BotControl::menuClassSelect);

   // counter-terrorist model select menu
   m_menus.emplace (
      Menu::CTSelect, keys (5),
      "\\ySelect an Appearance\\w\n\n"
      "1. Seal Team 6 (DEVGRU)\n"
      "2. German GSG-9\n"
      "3. UK SAS\n"
      "4. French GIGN\n\n"
      "5. Auto-select\n\n"
      "0. Exit",
      &BotControl::menuClassSelect);

   // condition zero terrorist model select menu
   m_menus.emplace (
      Menu::TerroristSelectCZ, keys (6),
      "\\ySelect an Appearance\\w\n\n"
      "1. Phoenix Connexion\n"
      "2. L337 Krew\n"
      "3. Arctic Avengers\n"
      "4. Guerilla Warfare\n"
      "5. Midwest Militia\n\n"
      "6. Auto-select\n\n"
      "0. Exit",
      &BotControl::menuClassSelect);

   // condition zero counter-terrorist model select menu
   m_menus.emplace (
      Menu::CTSelectCZ, keys (6),
      "\\ySelect an Appearance\\w\n\n"
      "1. Seal Team 6 (DEVGRU)\n"
      "2. German GSG-9\n"
      "3. UK SAS\n"
      "4. French GIGN\n"
      "5. Russian Spetsnaz\n\n"
      "6. Auto-select\n\n"
      "0. Exit",
      &BotControl::menuClassSelect);

   // command menu
   m_menus.emplace (
      Menu::Commands, keys (4),
      "\\yBot Command Menu\\w\n\n"
      "1. Make double jump\n"
      "2. Finish double jump\n\n"
      "3. Drop the C4 bomb\n"
      "4. Drop the weapon\n\n"
      "0. Exit",
      &BotControl::menuCommands);

   // main node menu
   m_menus.emplace (
      Menu::NodeMainPage1, keys (9),
      "\\yGraph Editor (Page 1)\\w\n\n"
      "1. Show/Hide nodes\n"
      "2. Cache node\n"
      "3. Create path\n"
      "4. Delete path\n"
      "5. Add node\n"
      "6. Delete node\n"
      "7. Set autopath distance\n"
      "8. Set radius\n\n"
      "9. Next...\n\n"
      "0. Exit",
      &BotControl::menuGraphPage1);

   // main node menu (page 2)
   m_menus.emplace (
      Menu::NodeMainPage2, keys (9),
      "\\yGraph Editor (Page 2)\\w\n\n"
      "1. Debug goal\n"
      "2. Auto node placement on/off\n"
      "3. Set flags\n"
      "4. Save graph\n"
      "5. Save without checking\n"
      "6. Load graph\n"
      "7. Check graph\n"
      "8. Noclip cheat on/off\n\n"
      "9. Previous...\n\n"
      "0. Exit",
      &BotControl::menuGraphPage2);

   // select nodes radius menu
   m_menus.emplace (
      Menu::NodeRadius, keys (9),
      "\\yNode Radius\\w\n\n"
      "1. 0 units\n"
      "2. 8 units\n"
      "3. 16 units\n"
      "4. 32 units\n"
      "5. 48 units\n"
      "6. 64 units\n"
      "7. 80 units\n"
      "8. 96 units\n"
      "9. 128 units\n\n"
      "0. Exit",
      &BotControl::menuGraphRadius);

   // nodes add menu
   m_menus.emplace (
      Menu::NodeType, keys (9),
      "\\yNode Type\\w\n\n"
      "1. Normal\n"
      "\\r2. Terrorist important\n"
      "3. Counter-Terrorist important\n"
      "\\w4. Block with hostage / Ladder\n"
      "\\y5. Rescue zone\n"
      "\\w6. Camping\n"
      "7. Camp end\n"
      "\\r8. Map goal\n"
      "\\w9. Jump\n\n"
      "0. Exit",
      &BotControl::menuGraphType);

   // debug goal menu
   m_menus.emplace (
      Menu::NodeDebug, keys (3),
      "\\yDebug Goal\\w\n\n"
      "1. Debug nearest node\n"
      "2. Debug facing node\n"
      "3. Stop debugging\n\n"
      "0. Exit",
      &BotControl::menuGraphDebug);

   // set node flag menu
   m_menus.emplace (
      Menu::NodeFlag, keys (9),
      "\\yToggle Node Flags\\w\n\n"
      "1. Block with hostage\n"
      "2. Terrorists specific\n"
      "3. CTs specific\n"
      "4. Use elevator\n"
      "5. Sniper point (\\yfor camp points only!\\w)\n"
      "6. Map goal\n"
      "7. Rescue zone\n"
      "8. Crouch down\n"
      "9. Camp point\n\n"
      "0. Exit",
      &BotControl::menuGraphFlag);

   // set camp directions menu
   m_menus.emplace (
      Menu::CampDirections, keys (2),
      "\\ySet Camp Point Directions\\w\n\n"
      "1. Camp start\n"
      "2. Camp end\n\n"
      "0. Exit",
      &BotControl::menuCampDirections);

   // auto-path max distance
   m_menus.emplace (
      Menu::NodeAutoPath, keys (7),
      "\\yAutoPath Distance\\w\n\n"
      "1. 0 units\n"
      "2. 100 units\n"
      "3. 130 units\n"
      "4. 160 units\n"
      "5. 190 units\n"
      "6. 220 units\n"
      "7. 250 units (default)\n\n"
      "0. Exit",
      &BotControl::menuAutoPathDistance);

   // path connections
   m_menus.emplace (
      Menu::NodePath, keys (4),
      "\\yCreate Path (Choose Direction)\\w\n\n"
      "1. Outgoing path\n"
      "2. Incoming path\n"
      "3. Bidirectional (both ways)\n"
      "4. Jumping path\n\n"
      "0. Exit",
      &BotControl::menuGraphPath);

   // kick menus
   m_menus.emplace (Menu::KickPage1, 0x0, "", &BotControl::menuKickPage1);
   m_menus.emplace (Menu::KickPage2, 0x0, "", &BotControl::menuKickPage2);
   m_menus.emplace (Menu::KickPage3, 0x0, "", &BotControl::menuKickPage3);
   m_menus.emplace (Menu::KickPage4, 0x0, "", &BotControl::menuKickPage4);
}
