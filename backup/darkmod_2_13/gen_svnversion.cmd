@echo off
setlocal enableDelayedExpansion

rem This batch file was combined from two answers:
rem   https://stackoverflow.com/a/9260308/556899
rem   https://stackoverflow.com/a/2340018/556899

rem run svnversion and capture its output to SVNVERSION variable
for /f %%i in ('svnversion -c') do set SVNVERSION=%%i

rem read svnversion_template.h line by line and copy to svnversion.h, while doing expansion
(
  for /f "usebackq delims=" %%A in ("idlib\svnversion_template.h") do echo %%A
) >"idlib\svnversion.h"

rem post message to MSVC output console
echo Inserted SVN revision number into svnversion.h
