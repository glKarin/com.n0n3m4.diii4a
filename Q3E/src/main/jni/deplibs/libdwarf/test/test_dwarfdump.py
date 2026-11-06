#!/usr/bin/env python3
# This test script is hereby placed into the public domain
# for anyone to used for any purpose.
#
# Run in test dir as:
# test_dwarfdump.py filetype buildsys sourcedirbase builddirbase \
#  semanticver
# where filetype is Elf, PE, or Macos
# where buildsys is conf, cmake, or meson

import os
import sys
import shutil
from subprocess import Popen, PIPE, STDOUT

# For reporting differences.
import difflib

gmaxlines = 700

testbase = [
    [
        "Elf",
        "testuriLE64ELf.base",
        "testuriLE64ELf.testme",
        "junk.LE64ELf.new",
    ],
    [
        "Macos",
        "test-mach-o-32.base",
        "test-mach-o-32.dSYM",
        "junk.mach-o.new",
    ],
    ["PE", "testobjLE32PE.base", "testobjLE32PE.exe", "junk.PE.new"],
]


class ddfiles:
    def __init__(self):
        self.testbase = False
        self.testobj = False
        self.newtest = False
        self.ddopts = ""

    def ddprint(self):
        print("Test baseline           :", self.testbase)
        print("Test object             :", self.testobj)
        print("Test output             :", self.newtest)
        print("Test dd options         :", self.ddopts)


class tdata:
    def __init__(self):
        self.objtype = False
        self.buildsystem = False
        self.srcbase = False
        self.bldbase = False
        self.semanticver = False
        self.cwd = False

    def tprint(self):
        print("Testdata objtype        :", self.objtype)
        print("Testdata build system   :", self.buildsystem)
        print("Testdata source base dir:", self.srcbase)
        print("Testdata build base dir :", self.bldbase)
        print("Testdata working dir    :", self.cwd)
        print("Testdata Semantic Ver   :", self.semanticver)


def setupfilesinvolved(td, dd):
    ftype = td.objtype
    for n, t in enumerate(testbase):
        if not t[0] == ftype:
            continue
        dd.testbase = t[1]
        dd.testobj = t[2]
        dd.newtest = t[3]
        dd.ddopts = ["-a", "-vvv"]
        return
    print(" FAIL test_dwarfdump.py to setup files for type ", ftype)
    sys.exit(1)


def fixtimestamp(txtout):
    # "Fix Last Time to 0 so compares can work ok
    # sed 's/last time 0x.*/last time 0x0/' <$i >$t
    out = []
    for n, s in enumerate(txtout):
        f = s.find("last time 0x")
        if f == -1:
            out += [s]
            continue
        l = "".join([s[0:f], "last time 0x0"])
        # print("dadebug timestamp fixed",l);
        out += [l]
    return out


def writetextout(text, filepath):
    try:
        f = open(filepath, "w")
    except:
        print(
            "Unable to write dwarfdump putput ",
            filepath,
            " giving up",
        )
        sys.exit(1)
    for s in txtout:
        # Even on windows writes in Linux text format
        print(s.rstrip("\n"), file=f, end="\n")
    f.close()


def copytobuild(srcfile, targetfile):
    try:
        f = open(srcfile, "r")
    except:
        print("Unable to open input conf ", srcfile, " giving up")
        sys.exit(1)
    y = f.readlines()
    f.close()
    try:
        w = open(targetfile, "w")
    except:
        print("Unable to output conf ", targetfile, " giving up")
        sys.exit(1)
    # print("dadebug write conf file, linecount",len(y))
    w.writelines(y)
    w.close()


# Read in lines to a list, strip off line-ends.
def getbaseline(path):
    try:
        f = open(path, "r")
    except:
        print("Unable to read in conf ", path, " giving up")
        sys.exit(1)
    y = f.readlines()
    # print("dadebug getbaseline return len",len(y))
    # y hase line endings (newline)
    out = []
    for x in y:
        out += [x.rstrip()]
    return out


# Run dwarfdump, limiting output to gmaxlines lines
def rundwarfdump(td, dd, dwarfdumppath, objpath, lmaxlines):
    out = []
    print("Run:", dwarfdumppath, dd.ddopts[0], dd.ddopts[1], objpath)
    p1 = Popen(
        [dwarfdumppath, dd.ddopts[0], dd.ddopts[1], objpath],
        stdout=PIPE,
        stderr=PIPE,
    )
    bx = p1.stdout.read()
    # bx is a byte array
    # For windows this replaces cr lf with lf.
    # for Linux does nothing
    y = bx.decode("utf-8")
    # y is a single long string, multiline.
    z = y.splitlines()
    for n, s in enumerate(z):
        if n >= lmaxlines:
            break
        out += [s.rstrip()]
    return out


# we are in windows copy dll from lib build to
# the dwarfdump build directory.
def copydll(td):
    semantic = td.semanticver
    svlist = semantic.split(".")
    dllv = svlist[0]
    print("Copy dll semantic:",semantic," Prefixnum",dllv)
    dllname=''.join(["libdwarf-",dllv,".dll"])
    dllpath = os.path.join(td.bldbase,
        "src/lib/libdwarf",dllname)
    targetdllpath= os.path.join(td.bldbase,
        "src/bin/dwarfdump",dllname)
    print("dllpath",dllpath)
    print("targetdllpath",targetdllpath)
    if os.path.exists(dllpath):
        if not os.path.exists(targetdllpath):
            shutil.copy(dllpath,targetdllpath)

if __name__ == "__main__":
    if len(sys.argv) != 6:
        print("FAIL test_dwarfdump.py arg count wrong")
        sys.exit(1)
    td = tdata()
    td.objtype = sys.argv[1]
    td.buildsystem = sys.argv[2]
    td.srcbase = sys.argv[3]
    td.bldbase = sys.argv[4]
    td.semanticver = sys.argv[5]
    td.cwd = os.getcwd()
    print("Running dwarfdump test")
    td.tprint()
    dd = ddfiles()
    setupfilesinvolved(td, dd)
    dd.ddprint()
    confsrcpath = os.path.join(
        td.srcbase, "src/bin/dwarfdump/dwarfdump.conf"
    )
    conftargpath = os.path.join(td.bldbase, "test", "dwarfdump.conf")
    testconf = os.path.exists(conftargpath)
    if not testconf:
        print(
            "Copy to local directory", confsrcpath, "to", conftargpath
        )
        copytobuild(confsrcpath, conftargpath)
    if td.buildsystem == "meson":
        mesonloc = os.path.join(td.bldbase, "dwarfdump.conf")
        if not os.path.exists(mesonloc):
            print(
                "Copy to local directory", confsrcpath, "to", mesonloc
            )
            copytobuild(confsrcpath, mesonloc)
    dwarfdumppath = os.path.join(
        td.bldbase, "src/bin/dwarfdump/dwarfdump")
    dwarfdumppathexe = os.path.join(
        td.bldbase, "src/bin/dwarfdump/dwarfdump.exe")
    if os.path.exists(dwarfdumppathexe):
        dwarfdumppath = dwarfdumppathexe
        copydll(td)

    objpath = os.path.join(td.srcbase, "test", dd.testobj)
    baseline_path = os.path.join(td.srcbase, "test", dd.testbase)
    testout_path = os.path.join(td.bldbase, "test", dd.newtest)
    # extract N lines from dwarfdump putput
    print("Extract ", gmaxlines, "from dwarfdump output")
    txtout = rundwarfdump(td, dd, dwarfdumppath, objpath, gmaxlines)
    t = fixtimestamp(txtout)
    txtout = t
    writetextout(txtout, testout_path)
    basetext = getbaseline(baseline_path)
    # print("now do the diff")
    diffs = difflib.unified_diff(basetext, txtout, lineterm="")
    used = False
    for s in diffs:
        print("There are differences.")
        used = True
        break
    if used:
        tempfilepath = os.path.join(td.bldbase, "test", dd.newtest)
        # For meson will need "test" too
        if td.buildsystem == "meson":
            tempfilepath = os.path.join(td.cwd, "test", dd.newtest)
        else:
            tempfilepath = os.path.join(td.cwd, dd.newtest)
        print(
            "Line Count Base=",
            len(basetext),
            " Line Count Test=",
            len(txtout),
        )
        for s in diffs:
            print(s)
        print("FAIL test_dwarfdump.py on", td.objtype, " test object")
        print("If update to baseline desired then:")
        print("mv", tempfilepath, baseline_path)
        sys.exit(1)
    print("PASS", td.objtype)
    sys.exit(0)
