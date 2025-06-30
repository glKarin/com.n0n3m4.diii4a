@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by MODELER.HPJ. >"hlp\Modeler.hm"
echo. >>"hlp\Modeler.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\Modeler.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\Modeler.hm"
echo. >>"hlp\Modeler.hm"
echo // Prompts (IDP_*) >>"hlp\Modeler.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\Modeler.hm"
echo. >>"hlp\Modeler.hm"
echo // Resources (IDR_*) >>"hlp\Modeler.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\Modeler.hm"
echo. >>"hlp\Modeler.hm"
echo // Dialogs (IDD_*) >>"hlp\Modeler.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\Modeler.hm"
echo. >>"hlp\Modeler.hm"
echo // Frame Controls (IDW_*) >>"hlp\Modeler.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\Modeler.hm"
REM -- Make help for Project MODELER


echo Building Win32 Help files
start /wait hcrtf -x "hlp\Modeler.hpj"
echo.
if exist Debug\nul copy "hlp\Modeler.hlp" C:\Work\Scape\Bin\Debug
if exist Debug\nul copy "hlp\Modeler.cnt" C:\Work\Scape\Bin\Debug
if exist Release\nul copy "hlp\Modeler.hlp" C:\Work\Scape\Bin
if exist Release\nul copy "hlp\Modeler.cnt" C:\Work\Scape\Bin
echo.


