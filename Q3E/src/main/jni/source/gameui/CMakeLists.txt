cmake_minimum_required(VERSION 3.9)
project (source-GameUI)

set(binary_name ${SOURCE_ENGINE_PREFIX}GameUI)

set(source_src 
		BackgroundMenuButton.cpp
		BasePanel.cpp
		GameConsole.cpp
		GameUI_Interface.cpp
		ModInfo.cpp
		MouseMessageForwardingPanel.cpp
		../common/GameUI/ObjectList.cpp
		PanelListPanel.cpp
		RunGameEngine.cpp
		../common/GameUI/scriptobject.cpp
		Sys_Utils.cpp
		TextEntryBox.cpp
		TGAImagePanel.cpp
		../public/vgui_controls/vgui_controls.cpp
		VGuiSystemModuleLoader.cpp
		BonusMapsDatabase.cpp
		../common/language.cpp
		../common/imageutils.cpp
		SaveGameBrowserDialog.cpp
		gameui_util.cpp
		BitmapImagePanel.cpp
		CommandCheckButton.cpp
		CvarNegateCheckButton.cpp
		../common/GameUI/cvarslider.cpp
		CvarTextEntry.cpp
		CvarToggleCheckButton.cpp
		HapticControlBox.cpp
		KeyToggleCheckButton.cpp
		LabeledCommandComboBox.cpp
		URLButton.cpp
		vcontrolslistpanel.cpp
		BenchmarkDialog.cpp
		BonusMapsDialog.cpp
		CommentaryDialog.cpp
		CommentaryExplanationDialog.cpp
		ContentControlDialog.cpp
		CustomTabExplanationDialog.cpp
		GameConsoleDialog.cpp
		LoadGameDialog_Xbox.cpp
		LoadGameDialog.cpp
		MultiplayerAdvancedDialog.cpp
		NewGameDialog.cpp
		PlayerListDialog.cpp
		SaveGameDialog_Xbox.cpp
		SaveGameDialog.cpp
		LoadCommentaryDialog.cpp
		LoadingDialog.cpp
		BaseSaveGameDialog.cpp
		CreateMultiplayerGameBotPage.cpp
		CreateMultiplayerGameDialog.cpp
		CreateMultiplayerGameGameplayPage.cpp
		CreateMultiplayerGameServerPage.cpp
		OptionsDialog_Xbox.cpp
		ControllerDialog.cpp
		matchmaking/achievementsdialog.cpp
		matchmaking/basedialog.cpp
		matchmaking/dialogmenu.cpp
		matchmaking/leaderboarddialog.cpp
		matchmaking/matchmakingbasepanel.cpp
		matchmaking/pausedialog.cpp
		matchmaking/sessionlobbydialog.cpp
		matchmaking/sessionoptionsdialog.cpp
		matchmaking/sessionbrowserdialog.cpp
		matchmaking/welcomedialog.cpp
		OptionsDialog.cpp
		OptionsSubAudio.cpp
		OptionsSubDifficulty.cpp
		OptionsSubGame.cpp
		OptionsSubKeyboard.cpp
		OptionsSubMouse.cpp
		OptionsSubTouch.cpp
		OptionsSubMultiplayer.cpp
		OptionsSubPortal.cpp
		OptionsSubVideo.cpp
		OptionsSubVoice.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
		../common/GameUI
		../thirdparty
)

set(source_def "GAMEUI_EXPORTS=1;NO_STEAM=1;VERSION_SAFE_STEAM_API_INTERFACES=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}vgui_controls 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}tier3 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	${SOURCE_ENGINE_PREFIX}vtf 
	${SOURCE_ENGINE_PREFIX}bitmap 
	${SOURCE_ENGINE_PREFIX}mathlib 
	SDL2 
	${SOURCE_ENGINE_PREFIX}steam_api 
	${SOURCE_ENGINE_PREFIX}matsys_controls 
	${SOURCE_ENGINE_PREFIX}jpeg 
	${SOURCE_ENGINE_PREFIX}png_static 
	${SOURCE_ENGINE_PREFIX}zlibstatic 
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

