@echo off
echo Setting environment for minimal Visual C++ 6
set INCLUDE=%MSVCDir%\VC98\Include
set LIB=%MSVCDir%\VC98\Lib
set PATH=%MSVCDir%\VC98\Bin;%MSVCDir%\Common\MSDev98\Bin\;%PATH%

echo -- Compiler is MSVC6

set XASH3DSRC=..\..\Xash3D_original
set INCLUDES=-I../common -I../engine -I../pm_shared -I../game_shared -I../public -I../external -I../dlls -I../utils/fake_vgui/include
set SOURCES=../dlls/crossbow.cpp ^
	../dlls/crowbar.cpp ^
	../dlls/egon.cpp ^
	../dlls/gauss.cpp ^
	../dlls/handgrenade.cpp ^
	../dlls/hornetgun.cpp ^
	../dlls/mp5.cpp ^
	../dlls/python.cpp ^
	../dlls/rpg.cpp ^
	../dlls/satchel.cpp ^
	../dlls/shotgun.cpp ^
	../dlls/squeakgrenade.cpp ^
	../dlls/tripmine.cpp ^
	../dlls/glock.cpp ^
	ev_hldm.cpp ^
	hl/hl_baseentity.cpp ^
	hl/hl_events.cpp ^
	hl/hl_objects.cpp ^
	hl/hl_weapons.cpp ^
	ammo.cpp ^
	ammo_secondary.cpp ^
	ammohistory.cpp ^
	battery.cpp ^
	cdll_int.cpp ^
	com_weapons.cpp ^
	death.cpp ^
	demo.cpp ^
	entity.cpp ^
	ev_common.cpp ^
	events.cpp ^
	flashlight.cpp ^
	GameStudioModelRenderer.cpp ^
	geiger.cpp ^
	health.cpp ^
	hud.cpp ^
	hud_msg.cpp ^
	hud_redraw.cpp ^
	hud_spectator.cpp ^
	hud_update.cpp ^
	in_camera.cpp ^
	input.cpp ^
	input_goldsource.cpp ^
	input_mouse.cpp ^
	input_xash3d.cpp ^
	interpolation.cpp ^
	menu.cpp ^
	message.cpp ^
	parsemsg.cpp ^
	../pm_shared/pm_debug.c ^
	../pm_shared/pm_math.c ^
	../pm_shared/pm_shared.c ^
	saytext.cpp ^
	status_icons.cpp ^
	statusbar.cpp ^
	studio_util.cpp ^
	StudioModelRenderer.cpp ^
	text_message.cpp ^
	train.cpp ^
	tri.cpp ^
	util.cpp ^
	view.cpp ^
	scoreboard.cpp ^
	MOTD.cpp ^
	../game_shared/vcs_info.cpp ^
	../public/safe_snprintf.c ^
        ../external/openbsd/strlcpy.c ^
        ../external/openbsd/strlcat.c
set DEFINES=/DCLIENT_DLL /DCLIENT_WEAPONS /Dsnprintf=_snprintf /DNO_VOICEGAMEMGR /DGOLDSOURCE_SUPPORT /DNDEBUG
set LIBS=user32.lib Winmm.lib
set OUTNAME=client.dll

cl %DEFINES% %LIBS% %SOURCES% %INCLUDES% -o %OUTNAME% /link /dll /out:%OUTNAME% /release

echo -- Compile done. Cleaning...

del *.obj *.exp *.lib *.ilk
echo -- Done.
