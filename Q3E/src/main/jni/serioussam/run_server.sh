#!/bin/bash

# Info
GAME_NAME="Serious Sam Classic"

# Directory
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Initialization
cd "${CURRENT_DIR}"

# Actions
run_option1() {
  echo "Running ${option1}"
  cd "./SamTFE/Bin"
  export LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}"
  "./DedicatedServer" "DefaultCoop"
}

run_option2() {
  echo "Running ${option3}"
  cd "./SamTSE/Bin"
  export LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}"
  "./DedicatedServer" "DefaultCoop"
}

run_option3() {
  echo "Running ${option5}"
  "nano" "${CURRENT_DIR}/SamTFE/Scripts/Dedicated/DefaultCoop/init.ini"
}
run_option4() {
  echo "Running ${option7}"
  "nano" "${CURRENT_DIR}/SamTSE/Scripts/Dedicated/DefaultCoop/init.ini"
}

Launcher
echo "Running ${GAME_NAME} launcher"
 
option1="Serious Sam TFE Classic (Start Dedicated Server)"
option2="Serious Sam TSE Classic (Start Dedicated Server)"
option3="Serious Sam TFE Classic (Edit  Server Settings)"
option4="Serious Sam TSE Classic (Edit  Server Settings)"


window_title="${GAME_NAME} Launcher"

LAUNCHER=$(zenity --width 450 --height 300 --list \
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
