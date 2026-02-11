@echo off

set RELEASE=70
set WORK_DIR=F:\qobj\droid
set RELEASE_DIR=idTech4A++_1.1.0harmattan%RELEASE%-win_x64
set TARGET_DIR=%WORK_DIR%\%RELEASE_DIR%
set SOURCE_DIR=D:\project\c\neo_win_x64_release\Release
set ZIP_FILE=%WORK_DIR%\%RELEASE_DIR%.zip

set game=doom3
set "libs=game d3xp d3le cdoom rivensin hardcorps hexeneoc librecoop librecoopxp perfected perfected_roe tfphobos overthinked fraggingfree sabot"
echo Remove %game% binaries......
call :remove_binaries
echo Copy %game% binaries......
call :copy_binaries

set game=quake4
set "libs=q4game hardqore"
echo Remove %game% binaries......
call :remove_binaries
echo Copy %game% binaries......
call :copy_binaries

set game=prey
set "libs=preygame"
echo Remove %game% binaries......
call :remove_binaries
echo Copy %game% binaries......
call :copy_binaries

echo Zip......
call :zip_binaries

echo Done

pause;
goto:eof

:remove_file
if exist "%file%" (
    echo Remove %file%
    del /Q %file%
) else (
    echo %file% has removed
)
goto:eof

:remove_binaries
for %%a in (%libs%) do (
    set file=%TARGET_DIR%\%game%\%%a.dll
    call :remove_file
)
set file=%TARGET_DIR%\%game%\%game%.exe
call :remove_file
goto:eof

:copy_file
if exist "%src_file%" (
    echo Copy %src_file% to %dst_file%
    xcopy /Y/Q %src_file% %dst_file%
) else (
    echo %src_file% not exist
)
goto:eof

:copy_binaries
set dst_file=%TARGET_DIR%\%game%\
for %%a in (%libs%) do (
    set src_file=%SOURCE_DIR%\%%a.dll
    call :copy_file
)
set src_file=%SOURCE_DIR%\%game%.exe
call :copy_file
goto:eof

:zip_binaries
cd %WORK_DIR%
set file=%ZIP_FILE%
call :remove_file
call zip -rq %ZIP_FILE% %RELEASE_DIR%
goto:eof