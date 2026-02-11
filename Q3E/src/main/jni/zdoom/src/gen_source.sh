#!/bin/bash

TO_DIR=`pwd`/src
SRC_DIR=`pwd`/src

lemon -C${TO_DIR} ${SRC_DIR}/src/gamedata/xlat/xlat_parser.y

lemon -C${TO_DIR} ${SRC_DIR}/src/common/scripting/frontend/zcc-parse.lemon

re2c --no-generation-date -s -o ${SRC_DIR}/src/sc_man_scanner.h ${SRC_DIR}/src/common/engine/sc_man_scanner.re