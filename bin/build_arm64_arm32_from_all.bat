@echo off

set RELEASE=70
set VERSION=1.1.0harmattan%RELEASE%
set SRC_PATH=F:\qobj\droid\DIII4A
set ROOT_PATH=F:\qobj\droid\Build
set APK=%ROOT_PATH%\idTech4Amm-%VERSION%.apk
set ALL_LIBS_PATH=%ROOT_PATH%\lib
set BUILD_LIBS_PATH=%ROOT_PATH%\Q3E\libs
set ARM64=arm64-v8a
set ARM32=armeabi-v7a
set UNZIP_BIN=E:\devinst\GnuWin32\bin\unzip.exe
set "REMOVE_LIBS=q3eloader q3e_bt SDL2 xdl xunwind"
set "REMOVE_BUILDS=Q3E\.cxx Q3E\build idTech4Amm\build"
set SRC_APK=%SRC_PATH%\idTech4Amm\build\outputs\apk\release\idTech4Amm-release.apk
set BUILD_APK=%ROOT_PATH%\idTech4Amm\build\outputs\apk\release\idTech4Amm-release.apk
rem set "REMOVE_LIBS=q3eloader q3e_bt SDL2 xdl VkLayer_khronos_validation xunwind swresample swscale avcodec avdevice avfilter avformat avutil"

rem Remove old libraries
call :remove_old_libraries

rem copy source apk
if not exist "%SRC_APK%" (
    echo Source apk file not exists
    pause && exit
)
call :copy_src_apk

rem unzip apk
call :extract_libraries
if not exist "%ALL_LIBS_PATH%" (
    echo Libraries file not exists
    pause && exit
)

rem Remove q3e/bt/SDL2 library
set arch=%ARM64%
call :remove_arch_libraries
set arch=%ARM32%
call :remove_arch_libraries

rem Remove project library
set arch=%ARM64%
call :remove_project_libraries
set arch=%ARM32%
call :remove_project_libraries

rem Build arm64 apk
call :clean_build

rem Copy arm64 library
set arch=%ARM64%
call :copy_libraries

call :build_project
if not exist "%SRC_APK%" (
    echo Build %arch% apk error
    pause && exit
)

rem save arm64 apk
set apk_arch=arm64
call :copy_apk

rem Build arm64 apk
call :clean_build

rem Copy arm64 library
set arch=%ARM32%
call :copy_libraries

call :build_project
if not exist "%SRC_APK%" (
    echo Build %arch% apk error
    pause && exit
)

rem save arm32 apk
set apk_arch=armv7
call :copy_apk

echo Build done, then clean project
call :clean_build
call :remove_old_libraries

rem Remove project library
set arch=%ARM64%
call :remove_project_libraries
set arch=%ARM32%
call :remove_project_libraries

echo Done!!!

pause;
goto:eof

:copy_libraries
set libs=%ALL_LIBS_PATH%\%arch%\*.so
set topath=%BUILD_LIBS_PATH%\%arch%
if exist "%ALL_LIBS_PATH%\%arch%" (
    echo Copy %libs% to %topath%
    xcopy /Y/Q %libs% %topath%
)
goto:eof

:remove_arch_libraries
echo Remove %arch% libraries
for %%a in (%REMOVE_LIBS%) do (
    set libname=%%a
    call :remove_library
)
goto:eof

:remove_library
set libfullpath=%ALL_LIBS_PATH%\%arch%\lib%libname%.so
if exist "%libfullpath%" (
    echo Delete outer %libfullpath%
    del /Q %libfullpath%
)
goto:eof

:remove_old_libraries
if exist "%ALL_LIBS_PATH%" (
    echo Remove libraries directory: %ALL_LIBS_PATH%
    del /S/Q %ALL_LIBS_PATH%
    rmdir /S/Q %ALL_LIBS_PATH%
)
goto:eof

:remove_project_libraries
set libfullpath=%BUILD_LIBS_PATH%\%arch%\
if exist "%libfullpath%" (
    echo Delete inner %libfullpath%
    del /S/Q %libfullpath%
)
goto:eof

:copy_src_apk
if exist "%APK%" (
    echo Remove old apk: %APK%
    del /Q %APK%
)
echo Copy %SRC_APK% to %APK%
copy /y %SRC_APK% %APK%
goto:eof

:extract_libraries
if exist "%APK%" (
    echo Unzip %APK%
    %UNZIP_BIN% %APK% "lib*" -d %ROOT_PATH%
) else (
    echo Missing apk file in %APK%
)
goto:eof

:clean_directory
if exist "%clean_dir%" (
    echo Remove directory: %clean_dir%
    del /S/Q %clean_dir%
    rmdir /S/Q %clean_dir%
)
goto:eof

:clean_build
echo Clean %arch% builds
call gradlew clean -Pabifilters=%arch% -Dorg.gradle.java.home=E:\devinst\java11
for %%a in (%REMOVE_BUILDS%) do (
    set clean_dir=%ROOT_PATH%\%%a
    call :clean_directory
)
goto:eof

:build_project
echo Build %arch%
call gradlew assembleRelease -Pabifilters=%arch% -Dorg.gradle.java.home=E:\devinst\java11
goto:eof

:copy_apk
set save_apk=%ROOT_PATH%\idTech4Amm-%VERSION%_%apk_arch%.apk
if exist "%save_apk%" (
    echo Remove old apk: %save_apk%
    del /Q %save_apk%
)
echo Copy %BUILD_APK% to %save_apk%
copy /y %BUILD_APK% %save_apk%
goto:eof



