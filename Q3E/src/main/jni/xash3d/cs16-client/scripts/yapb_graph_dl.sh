#!/bin/bash

# Specify the YaPB Graph Database URL
DATABASE_URL="https://yapb.jeefo.net/graph/"

# Specify the path to the file with the maps list
LIST_FILE="scripts/official_maps.lst"

# Checking the presence of the list file
if [ ! -f $LIST_FILE ]; then
    echo "File $LIST_FILE not found."
    exit 1
fi

# Read the map list line by line and download graphs for them
while IFS= read -r graph_name; do
    file_url="${DATABASE_URL}${graph_name}.graph"
    wget "$file_url" -P "3rdparty/cs16client-extras/addons/yapb/data/graph"
done < "$LIST_FILE"

echo "YaPB Graphs have been downloaded for the maps specified in the list."
