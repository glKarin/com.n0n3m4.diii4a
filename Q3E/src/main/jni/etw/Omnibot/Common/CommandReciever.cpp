#include "PrecompCommon.h"
#include "CommandReciever.h"
#include "ScriptManager.h"

//////////////////////////////////////////////////////////////////////////

ScriptCommandExecutor::ScriptCommandExecutor(gmMachine * a_machine, gmTableObject * a_commandTable)
	: m_Machine(a_machine)
	, m_CommandTable(a_commandTable)
{
}

bool ScriptCommandExecutor::Exec(const StringVector &_args, const gmVariable &_this)
{
	const bool bPrintUsage = _args.size() > 1 && _args[1] == "?";
	if(m_CommandTable)
	{
		DisableGCInScope gcEn(m_Machine);

		gmVariable vEntry = m_CommandTable->Get(m_Machine, _args[0].c_str());

		gmFunctionObject *pFn = vEntry.GetFunctionObjectSafe();
		if(!pFn)
		{
			// See if it's new style, and in a table.
			gmTableObject *pTbl = vEntry.GetTableObjectSafe();
			if(pTbl)
			{
				pFn = pTbl->Get(m_Machine,"Func").GetFunctionObjectSafe();

				bool bRunImmediate = false;
				if(!pTbl->Get(m_Machine,"RunImmediate").GetBoolSafe(bRunImmediate,false))
					bRunImmediate = false;

				if(bPrintUsage)
				{
					gmVariable vHelp = pTbl->Get(m_Machine,"Help");
					const char *pHelpString = vHelp.GetCStringSafe(0);
					if(pHelpString)
					{
						EngineFuncs::ConsoleMessage(pHelpString);
						return true;
					}
					else
					{
						gmTableObject *pUsageTbl = vHelp.GetTableObjectSafe();
						if(pUsageTbl)
						{
							gmTableIterator tIt;
							gmTableNode *pNode = pUsageTbl->GetFirst(tIt);
							while(pNode)
							{
								const char *pHelp = pNode->m_value.GetCStringSafe(0);
								if(pHelp)
									EngineFuncs::ConsoleMessage(pHelp);
								pNode = pUsageTbl->GetNext(tIt);
							}
							return true;
						}
					}
				}
			}
		}

		if(pFn)
		{
			if(bPrintUsage)
			{
				EngineFuncs::ConsoleError(va("No Usage Info For Command: %s", _args[0].c_str()));
				return true;
			}
			gmCall call;
			if(call.BeginFunction(m_Machine, pFn, _this, true))
			{
				// Add all the params
				gmTableObject *pParamTable = m_Machine->AllocTableObject();
				for(obuint32 i = 1; i < _args.size(); ++i) 
				{
					int iNum;
					float dNum;
					if (Utils::ConvertString(_args[i], iNum))
					{
						pParamTable->Set(m_Machine, i-1, gmVariable(iNum));
					}
					else if(Utils::ConvertString(_args[i], dNum))
					{
						pParamTable->Set(m_Machine, i-1, gmVariable(dNum));
					}
					else
					{
						pParamTable->Set(m_Machine, i-1, gmVariable(m_Machine->AllocStringObject(_args[i].c_str())));
					}
				}
				call.AddParamTable(pParamTable);
				call.End();

				CommandReciever::m_ConsoleCommand = _args[0];
				CommandReciever::m_ConsoleCommandThreadId = call.GetThreadId();
				return true;
			}
		}
		return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
String CommandReciever::m_ConsoleCommand;
int CommandReciever::m_ConsoleCommandThreadId, CommandReciever::m_MapDebugPrintThreadId;

CommandReciever::CommandMap CommandReciever::m_CommandMap;
CommandReciever::RecieverList CommandReciever::m_RecieverList;

CommandReciever::CommandReciever()
{
	m_RecieverList.push_back(this);
	static bool bDoOnce = false;
	if(!bDoOnce)
	{
		SetEx("help","Displays a list of commands",
			this, &CommandReciever::cmdHelp);
		bDoOnce = true;
	}
}

CommandReciever::~CommandReciever()
{
	m_RecieverList.remove(this);
}

bool CommandReciever::DispatchCommand(const StringVector &_args)
{
	const bool bPrintUsage = _args.size() > 1 && _args[1] == "?";

	// Look for default commands first.
	CommandMap::iterator cit = m_CommandMap.find(_args[0]);
	if(cit != m_CommandMap.end())
	{
		if(bPrintUsage)
		{
			EngineFuncs::ConsoleError("Usage info is available only for scripted commands.");
			return false;
		}
		(*cit->second.second)(_args);
		return true;
	}

	bool handled = false;

	// Look for the command in the script command table.
	gmMachine *m_Machine = ScriptManager::GetInstance()->GetMachine();
	gmTableObject *pCommandsTable = ScriptManager::GetInstance()->GetGlobalCommandsTable();
	if(pCommandsTable)
	{
		ScriptCommandExecutor cmdExec(m_Machine,pCommandsTable);
		if(cmdExec.Exec(_args))
		{
			if(bPrintUsage) return true;
			handled = true;
		}
	}

	// iterate all com
	for(RecieverList::iterator it = m_RecieverList.begin();
		it != m_RecieverList.end();
		++it)
	{
		if((*it)->UnhandledCommand(_args))
		{
			if(bPrintUsage) return true;
			handled = true;
		}
	}
	if(!handled) EngineFuncs::ConsoleError("Unrecognized command. Use /bot help for a list of commands.");
	return handled;
}

void CommandReciever::Set(const String _name, const String _info, CommandFunctorPtr _func)
{
	CommandMap::iterator it = m_CommandMap.find(_name);
	if(it != m_CommandMap.end())
	{
		it->second.second = _func;
	}
	else
	{
		m_CommandMap[_name] = CommandInfo(_info, _func);
	}
}

void CommandReciever::Alias(const String _name,const String _existingname)
{
	CommandMap::iterator it = m_CommandMap.find(_existingname);
	if(it != m_CommandMap.end())
	{
		m_CommandMap[_name] = it->second;
	}
}

void CommandReciever::Remove(const String _name)
{
	CommandMap::iterator it = m_CommandMap.find(_name);
	if(it != m_CommandMap.end())
	{
		m_CommandMap.erase(it);
	}
}

struct HelpItem
{
	const char *CommandName,*HelpString;
	HelpItem(const char *commandName, const char *helpString)
		: CommandName(commandName), HelpString(helpString){}
};

bool _HelpItemAlphabetical(const HelpItem &_pt1, const HelpItem &_pt2)
{
	return strcmp(_pt1.CommandName, _pt2.CommandName) < 0;
}

void CommandReciever::cmdHelp(const StringVector &_args)
{
	if(_args.size()==2)
	{
		//command usage info
		StringVector _args2;
		_args2.push_back(_args[1]);
		_args2.push_back("?");
		DispatchCommand(_args2);
		return;
	}

	EngineFuncs::ConsoleMessage("---- Omni-bot Command Help ----");
	CommandMap::const_iterator it = m_CommandMap.begin();
	for( ; it != m_CommandMap.end(); ++it)
	{
		if(it->first == "help")
			continue;
		EngineFuncs::ConsoleMessage(va("%s : %s", it->first.c_str(), it->second.first.c_str()));
	}

	gmMachine *m_Machine = ScriptManager::GetInstance()->GetMachine();
	gmTableObject *pCommandsTable = ScriptManager::GetInstance()->GetGlobalCommandsTable();
	if(pCommandsTable)
	{
		EngineFuncs::ConsoleMessage("---- Script Commands ----");
		std::vector<HelpItem> items;
		items.reserve(pCommandsTable->Count());
		gmTableIterator tIt;
		gmTableNode *pNode = pCommandsTable->GetFirst(tIt);
		while(pNode)
		{
			const char *pCommandName = pNode->m_key.GetCStringSafe();
			if(pCommandName)
			{
				gmTableObject *pTbl = pNode->m_value.GetTableObjectSafe();
				if(pTbl)
				{
					gmVariable vHelp = pTbl->Get(m_Machine,"Help");
					const char *pHelpString = vHelp.GetCStringSafe(0);
					if(pHelpString)
					{
						items.push_back(HelpItem(pCommandName,pHelpString));
					}
					else
					{
						gmTableObject *pUsageTbl = vHelp.GetTableObjectSafe();
						if(pUsageTbl)
						{
							pHelpString = pUsageTbl->Get(0).GetCStringSafe(0);
							if(pHelpString)
							{
								items.push_back(HelpItem(pCommandName,pHelpString));
							}
						}
					}
				}
			}
			pNode = pCommandsTable->GetNext(tIt);
		}
		std::sort(items.begin(), items.end(), _HelpItemAlphabetical);
		for(std::vector<HelpItem>::const_iterator cIt = items.begin(); cIt != items.end(); ++cIt)
		{
			EngineFuncs::ConsoleMessage(va("%s : %s", cIt->CommandName, cIt->HelpString));
		}
		EngineFuncs::ConsoleMessage("---- End Script Commands ----");
	}

	EngineFuncs::ConsoleMessage("---- End Omni-bot Command Help ----");
}
