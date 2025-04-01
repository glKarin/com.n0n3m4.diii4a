#!/bin/bash

export STEAMWORKSDIR=/path/to/steamworks_sdk_151/
export RELEASE=0

g++ -o launcher -Wall -O0 -ggdb3 -DRELEASE=$RELEASE steamshim_parent.cpp -I ${STEAMWORKSDIR}/sdk/public ${STEAMWORKSDIR}/sdk/redistributable_bin/linux64/libsteam_api.so
