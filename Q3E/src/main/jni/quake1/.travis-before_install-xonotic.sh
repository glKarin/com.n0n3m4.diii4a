#!/bin/sh

set -ex

export USRLOCAL="$PWD"/usrlocal
mkdir "$USRLOCAL"

for os in "$@"; do
  git archive --format=tar --remote=git://de.git.xonotic.org/xonotic/xonotic.git \
    --prefix=".deps/${os}/" master:"misc/builddeps/${os}" | tar xvf -

  case "$os" in
    linux32)
      wget https://www.libsdl.org/release/SDL2-2.0.10.tar.gz
      tar xf SDL2-2.0.10.tar.gz
      (
      cd SDL2-2.0.10
      export CC="gcc -m32"
      i386 ./configure --enable-static --disable-shared --prefix="$USRLOCAL" \
        --enable-video-x11-xinput --enable-video-x11-xrandr ||\ cat config.log
      i386 make
      i386 make install
      )
      ;;
    linux64)
      wget https://www.libsdl.org/release/SDL2-2.0.10.tar.gz
      tar xf SDL2-2.0.10.tar.gz
      (
      cd SDL2-2.0.10
      ./configure --enable-static --disable-shared --prefix="$USRLOCAL" \
        --enable-video-x11-xinput --enable-video-x11-xrandr ||\ cat config.log
      make
      make install
      )
      ;;
    win32)
      git archive --format=tar --remote=git://de.git.xonotic.org/xonotic/xonotic.git \
        --prefix=".icons/" master:"misc/logos/icons_ico" | tar xvf -
      mv .icons/xonotic.ico darkplaces.ico
      ;;
    win64)
      git archive --format=tar --remote=git://de.git.xonotic.org/xonotic/xonotic.git \
        --prefix=".icons/" master:"misc/logos/icons_ico" | tar xvf -
      mv .icons/xonotic.ico darkplaces.ico
      ;;
    osx)
      git archive --format=tar --remote=git://de.git.xonotic.org/xonotic/xonotic.git \
        --prefix=SDL2.framework/ master:misc/buildfiles/osx/Xonotic.app/Contents/Frameworks/SDL2.framework | tar xvf -
      ;;
  esac
done

for X in .deps/*; do
  rsync --remove-source-files -aL "$X"/*/ "$X"/ || true
done
