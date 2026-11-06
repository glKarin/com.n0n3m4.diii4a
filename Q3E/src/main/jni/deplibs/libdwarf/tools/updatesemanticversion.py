#!/usr/bin/env python3
# Update current package semantic version.
# 2021 David Anderson
# This python3 script is hereby placed in the Public Domain
# for anyone to use in any way.

import os
import sys
import datetime


def usage():
    print("Example of use:")
    print("  python3 tools/updatesemanticversion.py 2.0.0")
    print("Then git push as appropriate")
    sys.exit(0)


def deriveversions(sver):
    nums = sver.split(".")
    if len(nums) != 3:
        return (False, 0, 0, 0)
    ma = nums[0]
    min = nums[1]
    mic = nums[2]
    if not ma.isdecimal():
        return (False, 0, 0, 0)
    if not min.isdecimal():
        return (False, 0, 0, 0)
    if not mic.isdecimal():
        return (False, 0, 0, 0)
    return (True, ma, min, mic)


hmeson="  version: '1.99.0'"
hmmes ="  version: '"
hmmee = "'"
def updatemesonversion(lcount, l, sver, maj, min, mic):
    if l.startswith(hmmes):
       s = ''.join([hmmes,sver,hmmee,",\n"])
       return s, int(lcount)+1
    return l,lcount

ha = """#define DW_LIBDWARF_VERSION_MAJOR """
hb = """#define DW_LIBDWARF_VERSION_MINOR """
hc = """#define DW_LIBDWARF_VERSION_MICRO """
hv = """#define DW_LIBDWARF_VERSION """




def updatelhstring(lcount, l, sver, maj, min, mic):
    if l.startswith(ha):
        l2 = ha + maj + "\n"
        return l2, int(lcount) + 1
    if l.startswith(hb):
        l2 = hb + min + "\n"
        return l2, int(lcount) + 1
    if l.startswith(hc):
        l2 = hc + mic + "\n"
        return l2, int(lcount) + 1
    if l.startswith(hv):
        l2 = "".join([hv, '"', sver, '"\n'])
        return l2, int(lcount) + 1
    return l, lcount


# set(VERSION 0.2.0)
begincm = "project(libdwarf VERSION "


# As of 10.0.0 looks like (but all one line)
#project(libdwarf VERSION 0.9.3 DESCRIPTION ""
#  HOMEPAGE_URL "https://github.com/davea42/libdwarf-code.git"
#  LANGUAGES C CXX)

def updatecmstring(cmcount, l, sver, maj, min, mic):
    if l.startswith(begincm):
        pa = begincm
        wds = l.split(begincm)
        postpl = wds[1]
        p3 = postpl.split()
        over = p3[0]
        partc = ' '.join(p3[1:])
        l2 = "".join([begincm, sver," ", partc,"\n"])
        return l2, int(cmcount) + 1
    return l, cmcount


mm = ".ds vE Rev "

#libdwarf.dox
#    @date 2022-02-23 v0.3.4
doxdate = "    @date "
doxd2   = "    @date"
def updatedoxversion(doxcount, l, sver):
    if l.startswith(doxdate):
        wds = l.split()
        if len(wds) < 3:
            return l, doxcount
        if not wds[0] == "@date":
            return l, doxcount
        vnow = ''.join(["v",sver,"\n"])
        d=datetime.date.today()
        ds = d.isoformat()
        l2 = " ".join([doxd2,ds,vnow])
        print("New libdwarf.dox date:",l2)
        return l2, int(doxcount + 1)
    return l, doxcount

"d=0.10.2"
def updatemakerelease(mrcount, l, sver):
    if l.startswith("d="):
        wds= l.split("=");
        l2= ''.join(["d=",sver,"\n"])
        return l2, int(mrcount + 1)
    return l,mrcount;
"d=0.10.2"
def updateallsimplebuilds(asbcount, l, sver):
    if l.startswith("d="):
        wds= l.split("=");
        l2= ''.join(["d=",sver,"\n"])
        return l2, int(asbcount + 1)
    return l,asbcount;

def updatemmversion(mmcount, l, sver):
    if l.startswith(mm):
        wds = l.split()
        if len(wds) != 8:
            return l, mmcount
        wb = " ".join(wds[0:7])
        l2 = " ".join([wb, sver, "\n"])
        return l2, int(mmcount + 1)
    return l, mmcount


# m4_define([v_maj], [0])
# m4_define([v_min], [2])
# m4_define([v_mic], [0])
ma = "m4_define([v_maj], ["
mn = "m4_define([v_min], ["
mc = "m4_define([v_mic], ["


def updateacversion(account, l, sver, maj, min, mic):
    if l.startswith(ma):
        l2 = "".join([ma, maj, "])\n"])
        return l2, int(account) + 1
    if l.startswith(mn):
        l2 = "".join([mn, min, "])\n"])
        return l2, int(account) + 1
    if l.startswith(mc):
        l2 = "".join([mc, mic, "])\n"])
        return l2, int(account) + 1
    return l, int(account)


#  type is "ac" or "cm" (etc)
def updatefile(fname, type, sver, maj, min, mic):
    foundcm = 0
    foundac = 0
    foundlh = 0
    foundmm = 0
    foundasb = 0
    founddox = 0
    foundmmeson = 0
    foundmr = 0
    print("Processing", fname, " type", type, " newver", sver)
    try:
        fin = open(fname, "r")
    except IOError as message:
        print("File could not be opened read: ", message, file=sys.stderr)
        sys.exit(1)
    content = fin.readlines()
    fin.close()
    outdata = []
    for i, l in enumerate(content):
        if type == "cm":
            lx, foundcm = updatecmstring(foundcm, l, sver, maj, min, mic)
            outdata += [lx]
            continue
        elif type == "lh":
            lx, foundlh = updatelhstring(foundlh, l, sver, maj, min, mic)
            outdata += [lx]
            continue
        elif type == "ac":
            lx, foundac = updateacversion(foundac, l, sver, maj, min, mic)
            outdata += [lx]
            continue
        elif type == "mmeson":
            lx, foundac = updatemesonversion(foundmmeson, l, sver,\
                 maj, min, mic)
            outdata += [lx]
            continue
        elif type == "mr":
            lx, foundmr = updatemakerelease(foundmr, l, sver)
            outdata += [lx]
            continue
        elif type == "asb":
            lx, foundasb = updateallsimplebuilds(foundasb, l, sver)
            outdata += [lx]
            continue
        elif type == "dox":
            lx, founddox = updatedoxversion(founddox, l, sver)
            outdata += [lx]
            continue
        elif type == "mm":
            lx, foundmm = updatemmversion(foundmm, l, sver)
            outdata += [lx]
            continue
        print("Unknown type of file! Give up!", type)
        sys.exit(1)
    if type == "mm" and not foundmm == 1:
        print("Something wrong, did not find", fname, "  version line in mm")
        sys.exit(1)
    if type == "cm" and not foundcm == 1:
        print(
            "Something wrong, did not find", fname, "  version line in CMakeLists.txt"
        )
        sys.exit(1)
    if type == "ac" and not foundac == 3:
        print("Something wrong, did not find configure.ac", "version lines")
        sys.exit(1)
    if type == "mr" and not foundmr == 1:
        print("Something wrong, did not find makerelease.sh", "version lines")
        sys.exit(1)
    if type == "dox" and not founddox == 1:
        print("Something wrong, did not find libdwarf.dox", "version lines")
        sys.exit(1)
    if type == "lh" and not foundlh == 4:
        print("Something wrong, did not find libdwarf.h", "version lines")
        sys.exit(1)
    try:
        fo = open(fname, "w")
    except IOError as message:
        print("File could not be opened write: ", message, file=sys.stderr)
        sys.exit(1)
    for i, l in enumerate(outdata):
        print(l, end="", file=fo)
    fo.close()


expecteddirs = ["bugxml", "cmake", "doc", "m4", "scripts", "src", "test","tools"]


if __name__ == "__main__":
    if len(sys.argv) < 2:
        usage()
    if len(sys.argv) > 2:
        usage()
    sver = sys.argv[1]

    ok, maj, min, mic = deriveversions(sver)
    if not ok:
        print("the new semantic version", sver, "is not in the properformat")
        sys.exit(1)
    dirnames = os.listdir(".")
    for d in expecteddirs:
        if d not in dirnames:
            print(
                "Run in base code directory.",
                d,
                "is missing so we are in the wrong directory",
            )
            sys.exit(1)
    updatefile("configure.ac", "ac", sver, maj, min, mic)
    updatefile("CMakeLists.txt", "cm", sver, maj, min, mic)
    updatefile("src/lib/libdwarf/libdwarf.h", "lh", sver, maj, min, mic)
    updatefile("doc/libdwarf.dox", "dox", sver, maj, min, mic)
    updatefile("doc/libdwarfp.mm", "mm", sver, maj, min, mic)
    updatefile("meson.build", "mmeson", sver, maj, min, mic)
    updatefile("tools/makerelease.sh", "mr", sver, maj, min, mic)
    updatefile("scripts/allsimplebuilds.sh", "asb", sver, maj, min, mic)
