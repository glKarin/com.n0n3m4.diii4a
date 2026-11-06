#!/usr/bin/env python3

# Run as:
# transformpath.py inpath outpath
# Does the transformations of paths

import os
import sys


def writeout(lines, path):
    try:
        f = open(path, "w")
    except:
        print("Unable to open output ", path, " giving up")
        sys.exit(1)
    for l in lines:
        print(l, file=f)
    f.close()


def readin(srcfile):
    try:
        f = open(srcfile, "r")
    except:
        print("Unable to open input file ", srcfile, " giving up")
        sys.exit(1)
    y = f.readlines()
    out = []
    for l in y:
        x = l.rstrip()
        out += [x]
    f.close()
    return out


#  for Windows msys2, the default prefixes of msys2 /
globalroot = ["C:/msys64/davea"]
# ignore D: too messy.

modstr = { "C:": "/c",  "c:": "/c"}


def winmodsrcrep(insrc):
    start = insrc[int(0) : int(2)]
    # print("dadebug startmodsrc",start)
    newv = modstr.get(start, False)
    if newv:
        # print("dadebug startmodsrc found",newv)
        w = "".join([newv, insrc[2:]])
        # print("dadebug winmodsrc gets ",w)
        return True, w
    return False, insrc


#   This to hide unwanted c: from Windows
#   result of getcwd
#   so the regression test works.
#   This is not particularly general or safe
#   or fast.. But the number of records to check is small.
#   It assumes the disk of the source is   C:
#   c: are probably not useful.
def pathrep(l1, srcpath, repstr):
    # print("dadebug pathrep: on ",l1,"srcpath", srcpath," repstr",repstr)
    if not l1.find(srcpath) == -1:
        # for p in ["C:","c:","/c",""]:
        for p in [""]:
            modsource = "".join([p, srcpath])
            # print("dadebug pathrep ck modsrc",modsource)
            if l1.find(modsource) != -1:
                l2 = l1.replace(modsource, repstr)
                # print("dadebug replace modsource",modsource,l1,l2)
                return True, l2
    # window C: replace
    t, modsrc2 = winmodsrcrep(srcpath)
    if t:
        if l1.find(modsrc2) != -1:
            l2 = l1.replace(modsrc2, repstr)
            # print("dadebug replace modsrc2",l2)
            return True, l2
    # print("dadebug final after winmodsrc",modsrc2)
    return False, l1


def whichpathtype(line):
    if line.startswith(" global path"):
        return "gp"
    if line.startswith("===Exec-path"):
        return "src"
    if line.startswith(" Debuglink target"):
        return "src"
    if not line.find("===Referred-path") == -1:
        return "src"
    if not line.find(" Path [") == -1:
        return "src"
    return "other"


def transform(ilines, srcpath, binpath):
    out = []
    for n, l1 in enumerate(ilines):
        # print("")
        # print("dadebug srcpath",srcpath,"binpath",binpath)
        # print("dadebug iline[",n," is:",l1)
        linetype = whichpathtype(l1)
        if linetype == "src":
            # print("dadebug try src path")
            chgd, l2 = pathrep(l1, srcpath, "..src..")
            if not chgd:
                chgd, l2 = pathrep(l1, globalroot[0], "")
                #if not chgd:
                #    chgd, l2 = pathrep(l1, globalroot[1], "")
            # print("dadebug result:",l2)
            out += [l2]
            continue
        if linetype == "gp":
            # print("dadebug try global path")
            chgd, l5 = pathrep(l1, globalroot[0], "")
            #if not chgd:
            #    chgd, l5 = pathrep(l1, globalroot[1], "")
            # print("dadebug result:",l5)
            out += [l5]
            continue
        # 'other'
        # print("dadebug try binpath")
        chgd, l3 = pathrep(l1, binpath, "..bld..")
        # print("dadebug result:",l3)
        out += [l3]
    return out


if __name__ == "__main__":
    origfile = False
    newfile = False
    if len(sys.argv) > 4:
        localsrcpath = sys.argv[1]
        localbinpath = sys.argv[2]
        infile = sys.argv[3]
        outfile = sys.argv[4]
    else:
        # print("transformpath. args required:", \
        #    "localsrcpath localbinpath  inputpath outpath")
        exit(1)
    ilines = readin(infile)
    outlines = transform(ilines, localsrcpath, localbinpath)
    writeout(outlines, outfile)
    sys.exit(0)
