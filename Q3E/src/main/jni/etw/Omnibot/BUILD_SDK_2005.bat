@echo off
echo Compiling Omni-Bot Common Library
echo ---------------------------------
call "%VS80COMNTOOLS%\vsvars32.bat"
@echo on

@echo Building Release Version
@Devenv.exe projects\msvc80\Omni-bot80.sln /build "Release" /project "Common" /out "common_release.txt"
@echo Building Debug Version
@Devenv.exe projects\msvc80\Omni-bot80.sln /build "Debug" /project "Common" /out "common_debug.txt"
@echo off

echo Copying Common Library Headers
echo ------------------------------
xcopy /y ".\Common\*.h" "..\..\SDK\head\Omnibot\common\"
xcopy /y ".\Common\BotExports.cpp" "..\..\SDK\head\Omnibot\common\"

echo Copying Common Library libs
echo ---------------------------
xcopy /y ".\projects\msvc80\build\Common\Debug\*.lib" "..\..\SDK\head\Omnibot\projects\msvc80\Common\Debug\"
xcopy /y ".\projects\msvc80\build\Common\Release\*.lib" "..\..\SDK\head\Omnibot\projects\msvc80\Common\Release\"
xcopy /y ".\build\Common\gcc\release\link-static\libCommon.a" "..\..\SDK\head\Omnibot\build\Common\gcc\release\link-static\Common.a"

echo Copying Dependencies
echo ------------------------
xcopy /y /E ".\dependencies\*.h" "..\..\SDK\head\Omnibot\dependencies\"
xcopy /y /E ".\dependencies\*.inl" "..\..\SDK\head\Omnibot\dependencies\"
xcopy /y /E ".\dependencies\*.mcr" "..\..\SDK\head\Omnibot\dependencies\"
xcopy /y /E ".\dependencies\*.lib" "..\..\SDK\head\Omnibot\dependencies\"

echo Copying Skeleton Project
echo ------------------------
xcopy /y ".\SKELETON\*.*" "..\..\SDK\head\Omnibot\SKELETON\"
xcopy /y ".\ET\*.*" "..\..\SDK\head\Omnibot\ET\"
xcopy /y ".\Q4\*.*" "..\..\SDK\head\Omnibot\Q4\"
xcopy /y ".\TeamFortressLib\*.*" "..\..\SDK\head\Omnibot\TeamFortressLib\"

echo Creating Patch Files
echo ------------------------
diff -urN --exclude=.svn --exclude=debug --exclude=release --exclude=Debug --exclude=Release --exclude=*.ncb --exclude=*.suo ..\GameInterfaces\ET\ET_SDK ..\GameInterfaces\ET\2.60 > ..\GameInterfaces\ET\patches\omni-bot.patch

echo Copying Game Interfaces
echo ------------------------
xcopy /y /E "..\GameInterfaces\ET\*.*" "..\..\SDK\head\GameInterfaces\ET\" /EXCLUDE:SDK_EXCLUDE_FILES.txt
xcopy /y /E "..\GameInterfaces\etf\*.*" "..\..\SDK\head\GameInterfaces\etf\" /EXCLUDE:SDK_EXCLUDE_FILES.txt
xcopy /y /E "..\GameInterfaces\HL1\*.*" "..\..\SDK\head\GameInterfaces\HL1\" /EXCLUDE:SDK_EXCLUDE_FILES.txt
xcopy /y /E "..\GameInterfaces\Q41.4.1\*.*" "..\..\SDK\head\GameInterfaces\Q41.4.1\" /EXCLUDE:SDK_EXCLUDE_FILES.txt
xcopy /y /E "..\GameInterfaces\JK_JA\*.*" "..\..\SDK\head\GameInterfaces\JK_JA\" /EXCLUDE:SDK_EXCLUDE_FILES.txt

echo Copying SDK Documentation
xcopy /y /E ".\Docs\*.*" "..\..\SDK\head\Omnibot\Docs\" /EXCLUDE:SDK_EXCLUDE_FILES.txt

echo done.
pause
