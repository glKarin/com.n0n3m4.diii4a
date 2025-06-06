#!/bin/bash

# Info
GAME_NAME="Serious Sam Classic (MangoHUD)"

# Directory
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Initialization
cd "${CURRENT_DIR}"

# Actions
run_option1() {
  echo "Running ${option1}"
  cd "./SamTFE/Bin"
  export LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}"
  MANGOHUD_CONFIG="offset_y=75,offset_x=20,cpu_temp,gpu_temp,ram,vram,engine_version" mangohud --dlsym "./SeriousSam"
}

run_option2() {
  echo "Running ${option2}"
  cd "./SamTSE/Bin"
  export LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}"
  MANGOHUD_CONFIG="offset_y=75,offset_x=20,cpu_temp,gpu_temp,ram,vram,engine_version" mangohud --dlsym "./SeriousSam"
}

run_option3() {
  echo "Running ${option3}"
  cd "./SamTFE/Bin"
  export LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}"
  vblank_mode=0 MANGOHUD_CONFIG="offset_y=75,offset_x=20,cpu_temp,gpu_temp,ram,vram,engine_version" mangohud --dlsym "./SeriousSam"
}

run_option4() {
  echo "Running ${option4}"
  cd "./SamTSE/Bin"
  export LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}"
  vblank_mode=0 MANGOHUD_CONFIG="offset_y=75,offset_x=20,cpu_temp,gpu_temp,ram,vram,engine_version" mangohud --dlsym "./SeriousSam"
}

Launcher
echo "Running ${GAME_NAME} launcher"
 
option1="Run Serious Sam TFE Classic (Lock 60fps)"
option2="Run Serious Sam TSE Classic (Lock 60fps)"
option3="Run Serious Sam TFE Classic (UnLock fps)"
option4="Run Serious Sam TSE Classic (UnLock fps)"

window_title="${GAME_NAME} Launcher"

LAUNCHER=$(zenity --width 400 --height 300 --list \
  --title "${window_title}" \
  --text="Launch:" \
  --column=" " \
  "${option1}" \
  "${option2}" \
  "${option3}" \
  "${option4}" 
)

if [ "${LAUNCHER}" == "${option1}" ]
then
	run_option1
elif [ "${LAUNCHER}" == "${option2}" ]
then
	run_option2
elif [ "${LAUNCHER}" == "${option3}" ]
then
	run_option3
elif [ "${LAUNCHER}" == "${option4}" ]
then
	run_option4
else
	exit 0
fi
