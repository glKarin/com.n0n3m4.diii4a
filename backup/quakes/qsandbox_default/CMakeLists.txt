CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(quake_sandbox_game)

# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()
set(CMAKE_BUILD_TYPE "RelWithDebInfo")

add_definitions(-D_DIII4A) # android
set(cpu ${CMAKE_SYSTEM_PROCESSOR})
if(cpu STREQUAL "powerpc")
	set(cpu "ppc")
elseif(cpu STREQUAL "aarch64")
	# "arm64" is more obvious, and some operating systems (like macOS) use it instead of "aarch64"
	set(cpu "arm64")
elseif(cpu MATCHES "[aA][mM][dD]64" OR cpu MATCHES "[xX].*64")
	set(cpu "x86_64")
elseif(cpu MATCHES "i.86" OR cpu MATCHES "[xX]86")
	set(cpu "i386")
elseif(cpu MATCHES "[aA][rR][mM].*") # some kind of arm..
	# On 32bit Raspbian gcc -dumpmachine returns sth starting with "arm-",
	# while clang -dumpmachine says "arm6k-..." - try to unify that to "arm"
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) # sizeof(void*) == 8 => must be arm64
		set(cpu "arm64")
	else() # should be 32bit arm then (probably "armv7l" "armv6k" or sth like that)
		set(cpu "arm")
	endif()
endif()
set(ARCH "${cpu}")
add_definitions(-DARCH_STRING="${ARCH}")
add_definitions(-D_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P})
if(CMAKE_CXX_BYTE_ORDER EQUAL BIG_ENDIAN)
	set(big_endian 1)
else()
	set(big_endian 0)
endif()
add_definitions(-D_IS_BIG_ENDIAN=${big_endian})

# windows with CMAKE
#add_definitions(-DVSTUDIO)

if(ANDROID)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O0 -g -ggdb -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization -Wno-implicit-function-declaration") # -Werror-implicit-function-declaration -Qunused-arguments
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O0 -g -ggdb -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization -Wno-implicit-function-declaration") # -Werror-implicit-function-declaration -Qunused-arguments
endif()


#cc="./lcc -w -DQAGAME -DQ3_VM -S -Wf-target=bytecode -Wf-g -I../../../code/game -I../../../code/qcommon $1"
SET(qs_game_source_list
	code/game/ai_chat.c
	code/game/ai_cmd.c
	code/game/ai_dmnet.c
	code/game/ai_dmq3.c
	code/game/ai_main.c
	code/game/ai_team.c
	code/game/bg_lib.c
	code/game/bg_misc.c
	code/game/bg_pmove.c
	code/game/bg_slidemove.c
	code/game/g_active.c
	code/game/g_arenas.c
	code/game/g_bot.c
	code/game/g_client.c
	code/game/g_cmds.c
	code/game/g_combat.c
	code/game/g_items.c
	code/game/g_main.c
	code/game/bg_alloc.c
	code/game/g_fileops.c
	code/game/g_misc.c
	code/game/g_mapfiles.c
	code/game/g_minigames.c
	code/game/g_mapcycle.c
	code/game/g_missile.c
	code/game/g_mover.c
	code/game/g_playerstore.c
	code/game/g_session.c
	code/game/g_spawn.c
	code/game/g_svcmds.c
	code/game/g_svcmds_ext.c
	code/game/g_target.c
	code/game/g_team.c
	code/game/g_trigger.c
	code/game/g_unlagged.c
	code/game/g_utils.c
	code/game/g_vote.c
	code/game/g_weapon.c
	code/qcommon/q_math.c
	code/qcommon/q_shared.c
	code/qcommon/ns_main.c
	code/qcommon/ns_func_game.c
	code/game/g_syscalls.c
)

#cc="./lcc -w -DCGAME -DQ3_VM -S -Wf-target=bytecode -Wf-g -I..\..\..\code\cgame -I..\..\..\code\ui -I..\..\..\code\qcommon $1"
SET(qs_cgame_source_list
	code/cgame/cg_atmospheric.c
	code/cgame/cg_consolecmds.c
	code/cgame/cg_draw.c
	code/cgame/cg_drawtools.c
	code/cgame/cg_effects.c
	code/cgame/cg_ents.c
	code/cgame/cg_event.c
	code/cgame/cg_info.c
	code/cgame/cg_localents.c
	code/cgame/cg_main.c
	code/cgame/cg_marks.c
	code/cgame/cg_particles.c
	code/cgame/cg_players.c
	code/cgame/cg_playerstate.c
	code/cgame/cg_predict.c
	code/cgame/cg_scoreboard.c
	code/cgame/cg_servercmds.c
	code/cgame/cg_snapshot.c
	code/cgame/cg_unlagged.c
	code/cgame/cg_view.c
	code/cgame/cg_weapons.c
	code/game/bg_lib.c
	code/game/bg_misc.c
	code/game/bg_pmove.c
	code/game/bg_slidemove.c
	code/qcommon/q_math.c
	code/qcommon/q_shared.c
	code/qcommon/ns_main.c
	code/qcommon/ns_func_cgame.c
	code/cgame/cg_syscalls.c
)


#cc="./lcc -w -DQ3_UI -DQ3_VM -S -Wf-target=bytecode -Wf-g -I..\..\..\code\ui -I..\..\..\code\qcommon $1"
SET(qs_ui_source_list
	code/ui/ui_addbots.c
	code/ui/ui_advanced.c
	code/ui/ui_atoms.c
	code/ui/ui_cinematics.c
	code/ui/ui_confirm.c
	code/ui/ui_connect.c
	code/ui/ui_controls2.c
	code/ui/ui_credits.c
	code/ui/ui_demo2.c
	code/ui/ui_display.c
	code/ui/ui_dynamicmenu.c
	code/ui/ui_gameinfo.c
	code/ui/ui_ingame.c
	code/ui/ui_ingame_mapvote.c
	code/ui/ui_loadconfig.c
	code/ui/ui_loadconfiged.c
	code/ui/ui_main.c
	code/ui/ui_menu.c
	code/ui/ui_mods.c
	code/ui/ui_network.c
	code/ui/ui_options.c
	code/ui/ui_playermodel.c
	code/ui/ui_players.c
	code/ui/ui_playersettings.c
	code/ui/ui_preferences.c
	code/ui/ui_qmenu.c
	code/ui/ui_removebots.c
	code/ui/ui_sandbox.c
	code/ui/ui_saveconfiged.c
	code/ui/ui_serverinfo.c
	code/ui/ui_servers2.c
	code/ui/ui_setup.c
	code/ui/ui_sound.c
	code/ui/ui_specifyserver.c
	code/ui/ui_startserver_bot.c
	code/ui/ui_startserver_botsel.c
	code/ui/ui_startserver_common.c
	code/ui/ui_startserver_custommaps.c
	code/ui/ui_startserver_data.c
	code/ui/ui_startserver_items.c
	code/ui/ui_startserver_map.c
	code/ui/ui_startserver_mapsel.c
	code/ui/ui_startserver_script.c
	code/ui/ui_startserver_server.c
	code/ui/ui_startserver_weapon.c
	code/ui/ui_team.c
	code/ui/ui_teamorders.c
	code/ui/ui_video.c
	code/ui/ui_workshop.c
	code/game/bg_lib.c
	code/game/bg_misc.c
	code/qcommon/q_math.c
	code/qcommon/q_shared.c
	code/qcommon/ns_main.c
	code/qcommon/ns_func_ui.c
	code/qcommon/ns_interface_ui.c
	code/ui/ui_syscalls.c
)


# game
ADD_LIBRARY(qsqagame SHARED ${qs_game_source_list})
set_target_properties(qsqagame PROPERTIES COMPILE_DEFINITIONS "GAMEDLL;QAGAME")
set_target_properties(qsqagame PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(qsqagame PROPERTIES OUTPUT_NAME "qsqagame")
target_include_directories(qsqagame PRIVATE "code/game")



# cgame
ADD_LIBRARY(qscgame SHARED ${qs_cgame_source_list})
set_target_properties(qscgame PROPERTIES COMPILE_DEFINITIONS "GAMEDLL;CGAME")
set_target_properties(qscgame PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(qscgame PROPERTIES OUTPUT_NAME "qscgame")
target_include_directories(qscgame PRIVATE "code/game")



# cgame
ADD_LIBRARY(qsui SHARED ${qs_ui_source_list})
set_target_properties(qsui PROPERTIES COMPILE_DEFINITIONS "UI;Q3_UI")
set_target_properties(qsui PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(qsui PROPERTIES OUTPUT_NAME "qsui")

target_include_directories(qsui PRIVATE "code/game")
