@echo off

SET PROJECT_DIR=F:/qobj/droid/DIII4A
SET DST_DIR=%PROJECT_DIR%/Q3E/src/main/jni/gzdoom/src

lemon -C%DST_DIR% %DST_DIR%/src/gamedata/xlat/xlat_parser.y

lemon -C%DST_DIR% %DST_DIR%/common/scripting/frontend/zcc-parse.lemon

re2c --no-generation-date -s -o %DST_DIR%/sc_man_scanner.h %DST_DIR%/common/engine/sc_man_scanner.re