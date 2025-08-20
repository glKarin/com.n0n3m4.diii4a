#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import print_function

import os
import sys

try:
    from urllib import urlretrieve

    URLError = IOError
except ImportError:
    from urllib.request import urlretrieve
    from urllib.error import URLError
    
# banned for me :(
# DATABASE_URL = "https://yapb.jeefo.net/graph/"
DATABASE_URL = "https://raw.githubusercontent.com/yapb/graph/master/graph/"
DEST_DIR = sys.argv[1]
OFFICIAL_MAPS = [
    "as_oilrig",
    "cs_747",
    "cs_assault",
    "cs_backalley",
    "cs_estate",
    "cs_havana",
    "cs_italy",
    "cs_militia",
    "cs_office",
    "cs_siege",
    "de_airstrip",
    "de_aztec",
    "de_cbble",
    "de_chateau",
    "de_dust",
    "de_dust2",
    "de_inferno",
    "de_nuke",
    "de_piranesi",
    "de_prodigy",
    "de_storm",
    "de_survivor",
    "de_torn",
    "de_train",
    "de_vertigo"
]

if not os.path.exists(DEST_DIR):
    os.makedirs(DEST_DIR)

for graph_name in OFFICIAL_MAPS:
    file_url = "{}{}.graph".format(DATABASE_URL, graph_name)
    file_path = os.path.join(DEST_DIR, "{}.graph".format(graph_name))

    try:
        # don't download if it already exists
        if not os.path.exists(file_path):
            urlretrieve(file_url, file_path)
            print("Downloaded: {}".format(file_path))
    except URLError:
        print("Failed to download: {}".format(file_url))
        continue
    except Exception as e:
        print("Unknown error: {}".format(e))
        continue

print("YaPB Graphs have been downloaded for the maps specified in the list.")
