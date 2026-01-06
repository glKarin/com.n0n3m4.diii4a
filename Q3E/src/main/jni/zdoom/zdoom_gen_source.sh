#!/bin/bash

TO_DIR=`pwd`/src
SRC_DIR=`pwd`/src

LEMON_PATH="./lemon/arm64/"
RE2C_PATH="./re2c/arm64/"

${LEMON_PATH}lemon -C${TO_DIR} ${SRC_DIR}/gamedata/xlat/xlat_parser.y

${LEMON_PATH}lemon -C${TO_DIR} ${SRC_DIR}/common/scripting/frontend/zcc-parse.lemon

${RE2C_PATH}re2c --no-generation-date -s -o ${SRC_DIR}/sc_man_scanner.h ${SRC_DIR}/common/engine/sc_man_scanner.re