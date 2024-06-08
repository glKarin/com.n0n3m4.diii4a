// Duke_menu.cpp
//

#include "Gamelib/Game_local.h"

/*
=================
dnGameLocal::InitGuis
=================
*/
void dnGameLocal::InitGuis(void) {
	// we have a single instance of the main menu
#ifndef ID_DEMO_BUILD
	guiMainMenu = uiManager->FindGui("guis/mainmenu.gui", true, false, true);
#else
	guiMainMenu = uiManager->FindGui("guis/demo_mainmenu.gui", true, false, true);
#endif
}

/*
=================
dnGameLocal::StartMainMenu
=================
*/
void dnGameLocal::StartMainMenu(bool playIntro) {
	session->SetGUI(guiMainMenu, NULL);	

	// key bind names
	guiMainMenu->SetKeyBindingNames();

	// flag for in-game menu
	if (session->IsMapSpawned()) {
		guiMainMenu->SetStateString("inGame", session->IsMultiplayer() ? "2" : "1");
		guiMainMenu->HandleNamedEvent("noIntro");
	}
	else {
		guiMainMenu->SetStateString("inGame", "0");
		guiMainMenu->HandleNamedEvent(playIntro ? "playIntro" : "noIntro");
	}
}

/*
=================
dnGameLocal::StartMainMenu
=================
*/
idUserInterface* dnGameLocal::GetMainMenuUI(void) {
	return guiMainMenu;
}

/*
=================
dnGameLocal::HandleInGameCommands
=================
*/
void dnGameLocal::HandleInGameCommands(const char* menuCommand) {
	// execute the command from the menu
	idCmdArgs args;

	args.TokenizeString(menuCommand, false);

	const char* cmd = args.Argv(0);
	if (!idStr::Icmp(cmd, "close")) {
		session->CloseActiveMenu();
		session->ExitMenu();
	}
}

/*
=================
dnGameLocal::HandleMainMenuCommands
=================
*/
void dnGameLocal::HandleMainMenuCommands(const char* menuCommand) {
	// execute the command from the menu
	int icmd;
	idCmdArgs args;

	args.TokenizeString(menuCommand, false);

	for (icmd = 0; icmd < args.Argc(); ) {
		const char* cmd = args.Argv(icmd++);

		if (!idStr::Icmp(cmd, "quit")) {
			session->ExitMenu();
			common->Quit();
			return;
		}


		if (!idStr::Icmp(cmd, "play")) {
			if (args.Argc() - icmd >= 1) {
				idStr snd = args.Argv(icmd++);
				int channel = 1;
				if (snd.Length() == 1) {
					channel = atoi(snd);
					snd = args.Argv(icmd++);
				}
				session->GetMenuSoundWorld()->PlayShaderDirectly(snd, channel);

			}
			continue;
		}

		if (!idStr::Icmp(cmd, "music")) {
			if (args.Argc() - icmd >= 1) {
				idStr snd = args.Argv(icmd++);
				session->GetMenuSoundWorld()->PlayShaderDirectly(snd, 2);
			}
			continue;
		}
	}
}


/*
================
dnGameLocal::HandleGuiCommands
================
*/
const char* dnGameLocal::HandleGuiCommands(const char* menuCommand) {
	if (!isMultiplayer) {
		return NULL;
	}
	return mpGame.HandleGuiCommands(menuCommand);
}
