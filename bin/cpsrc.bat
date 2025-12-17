@echo off

set SRC_PATH=F:\qobj\droid\DIII4A
set DST_PATH=F:\data\DIII4A

if exist "%DST_PATH%" (
	echo Remove %DST_PATH%
   del /S/Q %DST_PATH%
   rmdir /S/Q %DST_PATH%
) else (
	echo Not found %DST_PATH%
)

echo Create %DST_PATH%
mkdir %DST_PATH%
mkdir %DST_PATH%\idTech4amm
mkdir %DST_PATH%\Q3E

echo Copying %SRC_PATH% to %DST_PATH%

xcopy /E/I/Y/Q %SRC_PATH%\idTech4amm\src %DST_PATH%\idTech4amm\src
xcopy /Y/Q %SRC_PATH%\idTech4amm\build.gradle %DST_PATH%\idTech4amm\
xcopy /Y/Q %SRC_PATH%\idTech4amm\lint.xml %DST_PATH%\idTech4amm\
del /S/Q %DST_PATH%\idTech4amm\src\main\assets

xcopy /E/I/Y/Q %SRC_PATH%\Q3E\src %DST_PATH%\Q3E\src
xcopy /Y/Q %SRC_PATH%\Q3E\build.gradle %DST_PATH%\Q3E\
del /S/Q %DST_PATH%\Q3E\src\main\assets
del /S/Q %DST_PATH%\Q3E\src\main\jni\doom3bfg\base\def
del /S/Q %DST_PATH%\Q3E\src\main\jni\doom3bfg\base\materials
del /S/Q %DST_PATH%\Q3E\src\main\jni\doom3bfg\base\textures
del /S/Q %DST_PATH%\Q3E\src\main\jni\doom3bfg\base\maps
del /Q %DST_PATH%\Q3E\src\main\jni\doom3bfg\base\*.cfg
del /Q %DST_PATH%\Q3E\src\main\jni\doom3bfg\neo\astyle.*
del /Q %DST_PATH%\Q3E\src\main\jni\doom3bfg\neo\premake4.exe
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\etmain
rmdir /S/Q %DST_PATH%\Q3E\src\main\etw\etmain\static
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc\static
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc\static
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc_bm\static
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc_bm\static
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc_extra\static
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc_extra\static
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc_lights\static
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc_lights\static
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc_widepix\static
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\wadsrc_widepix\static
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\soundfont
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\fm_banks
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\lemon\Release
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\lemon\Release
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\re2c\Release
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\re2c\Release
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\re2c\examples
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\re2c\examples
del /Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\zipdir\CMakeCache.txt
del /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\zipdir\CMakeFiles
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\gzdoom\tools\zipdir\CMakeFiles
del /S/Q %DST_PATH%\Q3E\src\main\jni\tdm\glslprogs_mediump
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\tdm\glslprogs_mediump
del /Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\bjam*
del /Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\BUILD_SDK_2005.bat
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\BM
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\D3
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\DOD
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\ETQW
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\FF
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\HL2DM
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\JA
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\linux
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\MC
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\OF
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\projects
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\Q4
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\RTCW
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\SKELETON
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\TeamFortressLib
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\TF2
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\WOLF
del /S/Q "%DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\Detours Express 2.1\Detours.chm"
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\Recast\RecastDemo\Bin
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\sfml\extlibs\bin
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\sfml\lib
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\sfml\extlibs\bin
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\sfml\extlibs\libs-vc2005
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\VisualLeakDetector\bin
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\VisualLeakDetector\lib
del /Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\VisualLeakDetector\uninstall.exe
del /Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\VisualLeakDetector\visualleakdetector.reg
del /Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\tools
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\tools
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc_ex\bin
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc_ex\doc
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc_ex\EditorHighlighters
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc_ex\scripts
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc_ex\tools
rmdir /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc_ex\tools
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc\bin
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc\doc
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc\EditorHighlighters
del /S/Q %DST_PATH%\Q3E\src\main\jni\etw\Omnibot\dependencies\gmscriptex\gmsrc\scripts

rem xcopy /Y/Q %SRC_PATH%\CHANGES.zh.md %DST_PATH%\
rem xcopy /Y/Q %SRC_PATH%\README.zh.md %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\.gitignore %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\build.gradle %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\CHANGES.md %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\CHECK_FOR_UPDATE.json %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\CMakeLists.txt %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\gradle.properties %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\gradlew %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\gradlew.bat %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\idtech4amm.keystore %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\LICENSE %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\local.properties %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\README.md %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\settings.gradle %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\cmake_linux_build_doom3_quak4_prey.sh %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\cmake_msvc_build_doom3_quak4_prey.bat %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\cmake_linux_build_doom3bfg.sh %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\cmake_msvc_build_doom3_quak4_prey.bat %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\build_win_x64_doom3_quak4_prey.bat %DST_PATH%\
xcopy /Y/Q %SRC_PATH%\build_win_x86_doom3_quak4_prey.bat %DST_PATH%\

echo Done!
start "" %DST_PATH%

rem F:\qobj\droid\DIII4A\idTech4Amm\src\main\assets\source\DIII4A.source.tgz

pause