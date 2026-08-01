#!/usr/bin/env python3
# Copyright 2020 David Anderson.
# This code is hereby placed into the public domain.

# For msys2 mingw this trims off Windows C: stuff
# msys2 cannot always accept the true Windows path
# in open().
# For all else turns the path to the code directory into ...std...

#  test/canonicalpath.py in libdwarf test directory
#  scripts/canonicalpath.py in regressiontests directory

import os
import sys

def dowinb(s):
    winb="C:/msys64/davea/"
    w= s.replace(winb,"/")
    return w

def printfixedcontent(sname,codepath):
   for line in open(sname):
       w=dowinb(line.rstrip())
       if not codepath == 'xxxx':
           s = w.replace(codepath,"...std...")
           w = s
       print(w)
   return

if __name__ == '__main__':
    #print(len(sys.argv))
    if len(sys.argv) > 3:
       s = sys.argv[1]
       codepath = sys.argv[2]
       patchwhere = sys.argv[3]
    else:
       print("BAD arg count",len(sys.argv),"to canonicalpath.py")
       sys.exit(1)
    if patchwhere == "content":
        printfixedcontent(s,codepath)
        sys.exit(0)
    # We are fixing the content
    w = os.path.abspath(s)
    w= dowinb(w)
    # patchwhere is "start"
    if not codepath == 'xxxx':
        if w.startswith(codepath):
            s = w.replace(codepath,"...std...")
            w = s
    print(w)
