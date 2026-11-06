#!/bin/sh
# This encapsulates the way we release and how we set
# up tags marking a release.
# It uses configure and computes values we put
# in www.prevanders.net/dwarf.html 

d=2.3.2
chkres() {
r=$1
m=$2
if [ $r -ne 0 ]
then
  echo "FAIL $m.  Exit status $r"
  exit 1
fi
}

bldloc=/tmp/bldrel
rm -rf $bldloc
mkdir $bldloc
chkres $? "mkdir failed"
cd $bldloc
chkres $? "cd failed"
~/dwarf/code/configure
chkres $? "configure failed"
make dist
chkres $? "make dist "
f=libdwarf-$d.tar.xz
ls libdwarf-$d*
#mv  libdwarf-$d.tar.gz libdwarf-$d.tar.gz
#chkres $? "rename failed"
echo "Release name: $f"

cp $f /home/davea/web4/gweb/pagedata/
chkres $? "cp to gweb failed"
cp $f /home/davea/Desktop/
chkres $? "cp to Desktop failed"
echo "Now size of release, bytes:"
ls -l $f
echo "Now md5sum:"
md5sum $f
chkres $? "md5sum failed"
echo "Now sha512sum:"
sha512sum $f| fold -w 32
chkres $? "sha512sum pipe failed"

  # To get unforgeable checksums for the tar.gz file
  # md5sum is weak, but the pair should be
  # a strong confirmation.
  # The fold(1) is just to make the web
  # release page easier to work with.

echo "The release is $bldloc/$f"
date
echo "Do dwarf.html and doc/libdwarf.dox have latest?"
echo "Now do by hand:"
echo "git tag -a v$d  -m Release=$d"
echo "git tag -a libdwarf-$d  -m Release=$d"
echo "now push the tags:"
echo "git push origin libdwarf-$d"  
echo "git push origin v$d"    
echo "Then to verify tags match main:"
echo "git diff main v$d --name-status"
echo "git diff main libdwarf-$d --name-status"
echo "done makerelease.sh"
