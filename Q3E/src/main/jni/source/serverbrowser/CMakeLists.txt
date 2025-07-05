cmake_minimum_required(VERSION 3.9)
project (source-ServerBrowser)

set(binary_name ${SOURCE_ENGINE_PREFIX}ServerBrowser)

set(source_src 
		BaseGamesPage.cpp
#		BlacklistedServers.cpp
#		CustomGames.cpp
		DialogAddServer.cpp
		DialogGameInfo.cpp
		DialogServerPassword.cpp
		FavoriteGames.cpp
#		FriendsGames.cpp
		HistoryGames.cpp
		InternetGames.cpp
		LanGames.cpp
		ModList.cpp
		ServerBrowser.cpp
		ServerBrowserDialog.cpp
		ServerContextMenu.cpp
		ServerListCompare.cpp
#		SpectateGames.cpp
		VACBannedConnRefusedDialog.cpp
		QuickListPanel.cpp
		../public/vgui_controls/vgui_controls.cpp
		../common/ServerBrowser/blacklisted_server_manager.cpp
		../public/tier0/memoverride.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
		../common
)

set(source_def "SERVERBROWSER_EXPORTS=1;VERSION_SAFE_STEAM_API_INTERFACES=1;GAME_SRC=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}vgui_controls 
	${SOURCE_ENGINE_PREFIX}mathlib 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2
	${SOURCE_ENGINE_PREFIX}tier3 
	${SOURCE_ENGINE_PREFIX}vstdlib  
	${SOURCE_ENGINE_PREFIX}steam_api 
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

