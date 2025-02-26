#!/bin/sh
#Script to set up a debian chroot suitable for compiling fte's public builds.
#deterministic builds are attempted but also requires:
#	gcc/etc versions must match exactly (we use debian oldstable, which should reduce issues...)
#	sourcecode must be unmodified, particuarly if 'svnversion' reports modified even in an irrelevant file then embedded revision numbers will be wrong.
#	third party dependancies need to work and not get messed up (either me failing to re-run makelibs, random wget failures, or outdated revisions being removed from public access)
#	obtained sourcecode revision must match the binary you're trying to duplicate (pre-5601 will insist on updating to latest svn (which may not even have a public build), so expect problems trying to duplicate older builds when the scripts instead try to grab the most recent build).
#for regular use you should probably set up schroot so you don't need to remember so many args
#requires about 2.3gb for the chroot+win64 build.
#android and emscripten targets require proper mounting of /proc and /dev and are NOT supported by this script. don't try enabling them

FTEBUILD=/tmp/ftebuild #change freely
CHUID=1000 #should generally be your non-root user id, giving you the same access in or out of the chroot...
CHUSER=spike #sadly this matters. youll just need to pretend to be me inside your chroot for now.
DEBIANMIRROR=http://ftp.uk.debian.org/debian/
DEBIANVERSION=stretch	#oldstable now... should update to stable, but paranoid to update due to portability of glibc symbols.
LANG= #no language packs installed, so would be spammy if the following rules inherit the host lang
#FTEREVISON="-r 5601"	#earlier than 5601 will fail (scripts will try to update to latest)
#THREADS="-j 8" #override number of threads to compile with, if you have a decent cpu.

#package lists
#general packages required to get the build system working (python+unzip+etc is for third-party dependancies)
GENERALPACKAGES= subversion build-essential automake ca-certificates unzip p7zip-full zip libtool python pkg-config
#package list needed to crosscompile for windows
WINTARGETPACKAGES= mingw-w64
#dev packages required to compile the linux build properly. Comment out for less downloads/diskusage
LINUXTARGETPACKAGES= gcc-multilib g++-multilib mesa-common-dev libasound2-dev libxcursor-dev libgnutls28-dev

#NOTE: chroot does NOT wipe all environment settings. some get carried over. This is a problem if your distro has a default PATH that excludes the system programs on debian, so this is included to be sure.
export PATH=/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games

#Set up our chroot (you can skip this part entirely if you're preconfigured a VM)
#make sure debootstrap is installed, without erroring out if you're not on debian-derivative (NOTE: such users will needs to manually install it first from somewhere!)
(which apt-get>/dev/null) && apt-get install --no-install-recommends debootstrap
#create the new debian chroot. it should receive the most recent versions of packages.
debootstrap $DEBIANVERSION $FTEBUILD $DEBIANMIRROR
echo "FTEBuild">$FTEBUILD/etc/debian_chroot														#optional, so it shows if you decide to run a bash prompt inside the chroot.
chroot $FTEBUILD adduser --uid $CHUID $CHUSER 													#create a user (with a homedir), so we dont depend upon root inside the guest, where possible

#Install the extra packages needed to build
chroot $FTEBUILD apt-get install --no-install-recommends $GENERALPACKAGES $WINTARGETPACKAGES $LINUXTARGETPACKAGES
#NOW we finally start with non-debian downloads by grabbing the FTE sourcecode
chroot $FTEBUILD su $CHUSER -c "svn checkout https://svn.code.sf.net/p/fteqw/code/trunk ~/quake/fteqw-code $FTEREVISON"	#grab all the source code.
#FTE has some setup bollocks, which does some toolchain checks and such. You can choose which targets to build here.
#NOTE: the following line will download third-party packages.
chroot $FTEBUILD su $CHUSER -c "cd ~/quake/fteqw-code && ./build_setup.sh --noupdate"
#And finally the main rebuild thing. drop the --noupdate part if you want to build the latest-available revision.
chroot $FTEBUILD su $CHUSER -c "cd ~/quake/fteqw-code && ./build_wip.sh --noupdate $THREADS"


#to remove your chroot afterwards:
#rm --one-file-system -rf $FTEBUILD
