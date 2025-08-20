#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys
import zipfile

out = sys.argv[1]
src_dirs = sys.argv[2:]

zip_file = zipfile.ZipFile(out, "w", compression=zipfile.ZIP_STORED)

for src in src_dirs:
    for dirpath, dirnames, filenames in os.walk(src):
        for filename in filenames:
            file_path = os.path.join(dirpath, filename)
            name = os.path.relpath(file_path, src)
            zip_file.write(file_path, name)

zip_file.close()