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
#pragma hdrstop



#include "Script_Doc_Export.h"
#include "pugixml.hpp"

namespace
{
	inline void Write(idFile& out, const idStr& str)
	{
		out.Write(str.c_str(), str.Length());
	}

	inline void Writeln(idFile& out, const idStr& str)
	{
		out.Write((str + "\n").c_str(), str.Length() + 1);
	}

	idStr GetEventArgumentString(const idEventDef& ev)
	{
		idStr out;

		static const char* gen = "abcdefghijklmnopqrstuvwxyz";
		int g = 0;

		const EventArgs& args = ev.GetArgs();

		for ( int i = 0; i < args.size(); i++ )
		{
			out += out.IsEmpty() ? "" : ", ";

			idTypeDef* type = idCompiler::GetTypeForEventArg(args[i].type);

			// Use a generic variable name "a", "b", "c", etc. if no name present
			out += va("%s %s", type->Name(), strlen(args[i].name) > 0 ? args[i].name : idStr(gen[g++]).c_str());
		}

		return out;
	}

	inline bool EventIsPublic(const idEventDef& ev)
	{
		const char* eventName = ev.GetName();

		if (eventName != NULL && (eventName[0] == '<' || eventName[0] == '_'))
		{
			return false; // ignore all event names starting with '<', these mark internal events
		}

		const char* argFormat = ev.GetArgFormat();
        int numArgs = static_cast<int>(strlen(argFormat));

		// Check if any of the argument types is invalid before allocating anything
		for (int arg = 0; arg < numArgs; ++arg)
		{
			idTypeDef* argType = idCompiler::GetTypeForEventArg(argFormat[arg]);

			if (argType == NULL)
			{
				return false;
			}
		}

		return true;
	}

	idList<idTypeInfo*> GetRespondingTypes(const idEventDef& ev)
	{
		idList<idTypeInfo*> tempList;

		int numTypes = idClass::GetNumTypes();

		for (int i = 0; i < numTypes; ++i)
		{
			idTypeInfo* info = idClass::GetType(i);

			if (info->RespondsTo(ev))
			{
				tempList.Append(info);
			}
		}

		idList<idTypeInfo*> finalList;

		// Remove subclasses from the list, only keep top-level nodes
		for (int i = 0; i < tempList.Num(); ++i)
		{
			bool isSubclass = false;

			for (int j = 0; j < tempList.Num(); ++j)
			{
				if (i == j) continue;

				if (tempList[i]->IsType(*tempList[j]))
				{
					isSubclass = true;
					break;
				}
			}

			if (!isSubclass)
			{
				finalList.Append(tempList[i]);
			}
		}

		return finalList;
	}

	int SortTypesByClassname(idTypeInfo* const* a, idTypeInfo* const* b)
	{
		return idStr::Cmp((*a)->classname, (*b)->classname);
	}
}

ScriptEventDocGenerator::ScriptEventDocGenerator()
{
	for (int i = 0; i < idEventDef::NumEventCommands(); ++i)
	{
		const idEventDef* def = idEventDef::GetEventCommand(i);

		_events[std::string(def->GetName())] = def;
	}

	time_t timer = time(NULL);
	struct tm* t = localtime(&timer);

	_dateStr = va("%04u-%02u-%02u %02u:%02u", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min);
}

// --------- D3 Script -----------

idStr ScriptEventDocGeneratorD3Script::GetEventDocumentation(const idEventDef& ev)
{
	idStr out = "/**\n";
	out += " * ";

	// Format line breaks in the description
	idStr desc(ev.GetDescription());
	desc.Replace("\n", "\n * ");

	out += desc;
		
	const EventArgs& args = ev.GetArgs();

	idStr argDesc;

	for ( int i = 0; i < args.size(); i++ )
	{
		if (idStr::Length(args[i].desc) == 0)
		{
			continue;
		}

		// Format line breaks in the description
		idStr desc(args[i].desc);
		desc.Replace("\n", "\n * ");

		argDesc += va("\n * @%s: %s", args[i].name, desc.c_str());
	}

	if (!argDesc.IsEmpty())
	{
		out += "\n * ";
		out += argDesc;
	}

	out += "\n */";

	return out;
}

void ScriptEventDocGeneratorD3Script::WriteDoc(idFile& out)
{
	Write(out, "#ifndef __TDM_EVENTS__\n");
	Write(out, "#define __TDM_EVENTS__\n\n");
	Write(out, "/**\n");
	Write(out, " * The Dark Mod Script Event Documentation\n");
	Write(out, " * \n");
	Write(out, " * This file has been generated automatically by the tdm_gen_script_event_doc console command.\n");
	Write(out, " * Last update: " + _dateStr + "\n");
	Write(out, " */\n");
	Write(out, "\n");
	Write(out, "// ===== THIS FILE ONLY SERVES FOR DOCUMENTATION PURPOSES, IT'S NOT ACTUALLY READ BY THE GAME =======\n");
	Write(out, "// ===== If you want to force this file to be loaded, change the line below to #if 1 ================\n");
	Write(out, "#if 0\n");
	Write(out, "\n");
	Write(out, "\n");

	for (EventMap::const_iterator i = _events.begin(); i != _events.end(); ++i)
	{
		const idEventDef& ev = *i->second;

		if (!EventIsPublic(ev)) continue;

		idTypeDef* returnType = idCompiler::GetTypeForEventArg(ev.GetReturnType());

		idStr signature = GetEventArgumentString(ev);
		idStr documentation = GetEventDocumentation(ev);
		idStr outStr = va("\n%s\nscriptEvent %s\t\t%s(%s);\n", 
			documentation.c_str(), returnType->Name(), ev.GetName(), signature.c_str());

		Write(out, outStr);
	}

	Write(out, "\n");
	Write(out, "#endif\n");
	Write(out, "\n");
	Write(out, "\n\n#endif\n");
}

// ------------- Mediawiki -------------

idStr ScriptEventDocGeneratorMediaWiki::GetEventDescription(const idEventDef& ev)
{
	idStr out = ":";

	// Format line breaks in the description
	idStr desc(ev.GetDescription());
	desc.Replace("\n", " "); // no artificial line breaks

	out += desc;
	out += "\n";
		
	const EventArgs& args = ev.GetArgs();

	idStr argDesc;

	for ( int i = 0; i < args.size(); i++ )
	{
		if (idStr::Length(args[i].desc) == 0)
		{
			continue;
		}

		// Format line breaks in the description
		idStr desc(args[i].desc);
		desc.Replace("\n", " "); // no artificial line breaks

		argDesc += va("::''%s'': %s\n", args[i].name, desc.c_str());
	}

	if (!argDesc.IsEmpty())
	{
		//out += "\n:";
		out += argDesc;
	}

	return out;
}

idStr ScriptEventDocGeneratorMediaWiki::GetEventDoc(const idEventDef* ev, bool includeSpawnclassInfo)
{
	idStr out;

	idTypeDef* returnType = idCompiler::GetTypeForEventArg(ev->GetReturnType());

	idStr signature = GetEventArgumentString(*ev);
	idStr description = GetEventDescription(*ev);

	idStr outStr = va("==== scriptEvent %s '''%s'''(%s); ====\n", 
		returnType->Name(), ev->GetName(), signature.c_str());

	out += outStr + "\n";
	out += description + "\n";

	// Get type response info
	idList<idTypeInfo*> list = GetRespondingTypes(*ev);
	list.Sort(SortTypesByClassname);

	if (includeSpawnclassInfo)
	{
		idStr typeInfoStr;

		for (int t = 0; t < list.Num(); ++t)
		{
			idTypeInfo* type = list[t];

			typeInfoStr += (typeInfoStr.IsEmpty()) ? "" : ", ";
			typeInfoStr += "''";
			typeInfoStr += type->classname;
			typeInfoStr += "''";
		}

		typeInfoStr = ":Spawnclasses responding to this event: " + typeInfoStr;
		out += typeInfoStr + "\n";
	}

	return out;
}

void ScriptEventDocGeneratorMediaWiki::WriteDoc(idFile& out)
{
	idStr version = va("%s %d.%02d, code revision %d", 
		GAME_VERSION, 
		TDM_VERSION_MAJOR, TDM_VERSION_MINOR, 
		RevisionTracker::Instance().GetHighestRevision() 
	);

	Writeln(out, "This page has been generated automatically by the tdm_gen_script_event_doc console command.");
	Writeln(out, "");
	Writeln(out, "Generated by " + version + ", last update: " + _dateStr);
	Writeln(out, "");
	Writeln(out, "{{tdm-scripting-reference-intro}}");

	// Table of contents, but don't show level 4 headlines
	Writeln(out, "<div class=\"toclimit-4\">"); // SteveL #3740
	Writeln(out, "__TOC__");
	Writeln(out, "</div>");

	Writeln(out, "= TDM Script Event Reference =");
	Writeln(out, "");
	Writeln(out, "== All Events ==");
	Writeln(out, "=== Alphabetic List ==="); // #3740 Two headers are required here for the toclimit to work. We can't skip a heading level.

	typedef std::vector<const idEventDef*> EventList;
	typedef std::map<idTypeInfo*, EventList> SpawnclassEventMap;
	SpawnclassEventMap spawnClassEventMap;

	for (EventMap::const_iterator i = _events.begin(); i != _events.end(); ++i)
	{
		const idEventDef* ev = i->second;

		if (!EventIsPublic(*ev)) continue;

		Write(out, GetEventDoc(ev, true));

		idList<idTypeInfo*> respTypeList = GetRespondingTypes(*ev);
		respTypeList.Sort(SortTypesByClassname);
			
		// Collect info for each spawnclass
		for (int t = 0; t < respTypeList.Num(); ++t)
		{
			idTypeInfo* type = respTypeList[t];
			SpawnclassEventMap::iterator typeIter = spawnClassEventMap.find(type);

			// Put the event in the class info map
			if (typeIter == spawnClassEventMap.end())
			{
				typeIter = spawnClassEventMap.insert(SpawnclassEventMap::value_type(type, EventList())).first;
			}

			typeIter->second.push_back(ev);
		}
	}

	// Write info grouped by class
	Writeln(out, "");
	Writeln(out, "== Events by Spawnclass / Entity Type ==");

	for (SpawnclassEventMap::const_iterator i = spawnClassEventMap.begin();
			i != spawnClassEventMap.end(); ++i)
	{
		Writeln(out, idStr("=== ") + i->first->classname + " ===");
		//Writeln(out, "Events:" + idStr(static_cast<int>(i->second.size())));

		for (EventList::const_iterator t = i->second.begin(); t != i->second.end(); ++t)
		{
			Write(out, GetEventDoc(*t, false));
		}
	}

	Writeln(out, "[[Category:Scripting]]");
}

// -------- XML -----------

void ScriptEventDocGeneratorXml::WriteDoc(idFile& out)
{
	pugi::xml_document doc;

	idStr version = va("%d.%02d", TDM_VERSION_MAJOR, TDM_VERSION_MINOR);

	time_t timer = time(NULL);
	struct tm* t = localtime(&timer);

	idStr isoDateStr = va("%04u-%02u-%02u", t->tm_year+1900, t->tm_mon+1, t->tm_mday);

	pugi::xml_node eventDocNode = doc.append_child("eventDocumentation");
	
	pugi::xml_node eventDocVersion = eventDocNode.append_child("info");
	eventDocVersion.append_attribute("game").set_value(GAME_VERSION);
	eventDocVersion.append_attribute("tdmversion").set_value(version.c_str());
	eventDocVersion.append_attribute("coderevision").set_value(RevisionTracker::Instance().GetHighestRevision());
	eventDocVersion.append_attribute("date").set_value(isoDateStr.c_str());

	for (EventMap::const_iterator i = _events.begin(); i != _events.end(); ++i)
	{
		const idEventDef* ev = i->second;

		if (!EventIsPublic(*ev)) continue;

		pugi::xml_node eventNode = eventDocNode.append_child("event");
		
		eventNode.append_attribute("name").set_value(ev->GetName());

		// Description
		pugi::xml_node evDescNode = eventNode.append_child("description");

		idStr desc(ev->GetDescription());
		desc.Replace("\n", " "); // no artificial line breaks

		evDescNode.append_attribute("value").set_value(desc.c_str());

		// Arguments
		static const char* gen = "abcdefghijklmnopqrstuvwxyz";
		int g = 0;

		const EventArgs& args = ev->GetArgs();

		for ( int i = 0; i < args.size(); i++ )
		{
			idTypeDef* type = idCompiler::GetTypeForEventArg(args[i].type);

			// Use a generic variable name "a", "b", "c", etc. if no name present
			pugi::xml_node argNode = eventNode.append_child("argument");
			argNode.append_attribute("name").set_value(strlen(args[i].name) > 0 ? args[i].name : idStr(gen[g++]).c_str());
			argNode.append_attribute("type").set_value(type->Name());

			idStr desc(args[i].desc);
			desc.Replace("\n", " "); // no artificial line breaks

			argNode.append_attribute("description").set_value(desc.c_str());
		}

		idList<idTypeInfo*> respTypeList = GetRespondingTypes(*ev);
		respTypeList.Sort(SortTypesByClassname);

		// Responding Events
		pugi::xml_node evRespTypesNode = eventNode.append_child("respondingTypes");

		for (int t = 0; t < respTypeList.Num(); ++t)
		{
			idTypeInfo* type = respTypeList[t];

			pugi::xml_node respTypeNode = evRespTypesNode.append_child("respondingType");
			respTypeNode.append_attribute("spawnclass").set_value(type->classname);
		}
	}

	std::stringstream stream;
	doc.save(stream);

    out.Write(stream.str().c_str(), static_cast<int>(stream.str().length()));
}
