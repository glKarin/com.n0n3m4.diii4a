#!/usr/bin/env python3

# Run aas:
# dwdiff.py config baseline newfile

import os
import sys
import difflib


def readin(srcfile):
    hasdos = False
    try:
        f = open(srcfile, "r")
    except:
        print("Unable to open input conf ", srcfile, " giving up")
        sys.exit(1)
    y = f.readlines()
    out = []
    for l in y:
        if l.endswith("\r\n"):
            hasdos = True
        out += [l.rstrip()]
    f.close()
    return hasdos, out


if __name__ == "__main__":
    origfile = False
    newfile = False
    if len(sys.argv) > 2:
        origfile = sys.argv[1]
        newfile = sys.argv[2]
    else:
        print("dwdiff.py args required: bldtype baseline newfile")
        exit(1)
    hasdos, olines = readin(origfile)
    hasdos, nlines = readin(newfile)
    # diffs = difflib.unified_diff(olines,nlines,lineterm='')
    diffs = difflib.context_diff(
        olines, nlines, lineterm="", fromfile=origfile, tofile=newfile
    )
    used = False
    for s in diffs:
        print("There are differences.")
        used = True
        break
    if used:
        print(
            "Line Count Base=",
            len(olines),
            " Line Count Test=",
            len(nlines),
        )
        for s in diffs:
            print(s)
        sys.exit(1)
    sys.exit(0)
