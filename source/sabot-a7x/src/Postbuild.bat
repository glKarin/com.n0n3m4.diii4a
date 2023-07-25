@echo off
rem add ./PostBuild "$(TargetPath)" "..\..\$(TargetFileName)"
rem attrib -r -s -h "%2"
echo Copying from %1 to %2
attrib -r "%2"
copy /Y "%1" "%2"
attrib +r "%2"
