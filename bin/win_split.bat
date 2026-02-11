@echo off

cd idTech4A++_windows_x64.1.1.0harmattan59

dir

mkdir doom3
mkdir quake4
mkdir prey

call:create_mod_folder doom3 base game "DOOM 3" Doom3
call:create_mod_folder doom3 d3xp d3xp "Resurrection of Evil" Doom3
call:create_mod_folder doom3 d3le d3le "The Lost Mission" Doom3
call:create_mod_folder doom3 cdoom cdoom "Classic DOOM" Doom3
call:create_mod_folder doom3 rivensin rivensin "Rivensin" Doom3
call:create_mod_folder doom3 hardcorps hardcorps "Hardcorps" Doom3
call:create_mod_folder doom3 hexeneoc hexeneoc "Hexen: Edge of Chaos" Doom3
call:create_mod_folder doom3 librecoop librecoop "LibreCoop" Doom3
call:create_mod_folder doom3 librecoopxp librecoopxp "LibreCoop(D3XP)" Doom3
call:create_mod_folder doom3 perfected perfected "Perfected Doom 3" Doom3
call:create_mod_folder doom3 perfected_roe perfected_roe "Perfected Doom 3(D3XP)" Doom3
call:create_mod_folder doom3 tfphobos tfphobos "Phobos" Doom3
call:create_mod_folder doom3 overthinked overthinked "Overthinked DooM^3" Doom3
call:create_mod_folder doom3 fraggingfree fraggingfree "Fragging Free" Doom3
call:create_mod_folder doom3 sabot sabot "SABot(v7)" Doom3

call:create_mod_folder quake4 q4base q4game "Quake 4" Quake4
call:create_mod_folder quake4 hardqore hardqore "Hardqore" Quake4

call:create_mod_folder prey base preygame "Prey(2006)" Prey

call:copy_engine_dll doom3 Doom3
call:copy_engine_dll quake4 Quake4
call:copy_engine_dll prey Prey

move doom3\run_game.bat doom3\run_doom3.bat
move quake4\run_q4game.bat quake4\run_quake4.bat
move prey\run_preygame.bat prey\run_prey.bat

cd ..

:copy_engine_dll
echo copy %~2 to %~1
move %~2.exe %~1
copy libcurl.dll %~1
copy OpenAL32.dll %~1
copy SDL2.dll %~1
copy zlib1.dll %~1
dir %~1
goto:eof

:create_mod_folder
SETLOCAL
SET dir=%~1\%~2
echo Create mod folder: %dir%
if exist "%dir%" (
    del /S/Q %dir%
) else (
    mkdir %dir%
)
SET bat_file=run_%~3.bat
echo copy ~3.dll to %~1
move %~3.dll %~1
echo Run %bat_file% > "%dir%\Put %~4 game files to here.txt"
echo %~5.exe +set r_useShadowMapping 1 +set harm_r_softStencilShadow 1 +set fs_game %~3 > "%~1\%bat_file%"
ENDLOCAL
goto:eof

pause;