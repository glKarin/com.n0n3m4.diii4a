#!/bin/sh

set -e

openssl aes-256-cbc -K $encrypted_29b4419ace44_key -iv $encrypted_29b4419ace44_iv -in .travis-id_xonotic.enc -out /tmp/id_xonotic -d

set -x

chmod 0600 /tmp/id_xonotic

export USRLOCAL="$PWD"/usrlocal

rev=`git rev-parse HEAD`

sftp -oStrictHostKeyChecking=no -i /tmp/id_xonotic -P 2342 -b - autobuild-bin-uploader@srv04.xonotic.org <<EOF || true
mkdir ${rev}
EOF

for os in "$@"; do

  deps=".deps/${os}"
  case "${os}" in
    linux32)
      chroot=
      makeflags='STRIP=:
        CC="${CC} -m32 -march=i686 -g1 -I../../../${deps}/include -L../../../${deps}/lib -DSUPPORTIPV6"
        SDL_CONFIG=$USRLOCAL/bin/sdl2-config
        DP_LINK_CRYPTO=shared
          LIB_CRYPTO="../../../${deps}/lib/libd0_blind_id.a ../../../${deps}/lib/libgmp.a"
        DP_LINK_CRYPTO_RIJNDAEL=dlopen
        DP_LINK_JPEG=shared
          LIB_JPEG=../../../${deps}/lib/libjpeg.a
        DP_LINK_ODE=shared
          CFLAGS_ODE="-DUSEODE -DLINK_TO_LIBODE -DdDOUBLE"
          LIB_ODE="../../../${deps}/lib/libode.a -lstdc++"
        DP_LINK_ZLIB=shared'
      maketargets='release'
      outputs='darkplaces-sdl:darkplaces-linux32-sdl darkplaces-dedicated:darkplaces-linux32-dedicated'
      ;;
    linux64)
      chroot=
      makeflags='STRIP=:
        CC="${CC} -m64 -g1 -I../../../${deps}/include -L../../../${deps}/lib -DSUPPORTIPV6"
        SDL_CONFIG=$USRLOCAL/bin/sdl2-config
        DP_LINK_CRYPTO=shared
          LIB_CRYPTO="../../../${deps}/lib/libd0_blind_id.a ../../../${deps}/lib/libgmp.a"
        DP_LINK_CRYPTO_RIJNDAEL=dlopen
        DP_LINK_JPEG=shared
          LIB_JPEG="../../../${deps}/lib/libjpeg.a"
        DP_LINK_ODE=shared
          CFLAGS_ODE="-DUSEODE -DLINK_TO_LIBODE -DdDOUBLE"
          LIB_ODE="../../../${deps}/lib/libode.a -lstdc++"
        DP_LINK_ZLIB=shared'
      maketargets='release'
      outputs='darkplaces-sdl:darkplaces-linux64-sdl darkplaces-dedicated:darkplaces-linux64-dedicated'
      ;;
    win32)
      chroot=
      # Need to use -mstackrealign as nothing guarantees that callbacks from
      # other Win32 DLLs - including SDL2 - retain 16 bytes alignment.
      makeflags='STRIP=:
        D3D=1
        DP_MAKE_TARGET=mingw
        UNAME=MINGW32
        WIN32RELEASE=1
        CC="i686-w64-mingw32-gcc -static -g1 -mstackrealign -Wl,--dynamicbase -Wl,--nxcompat -I../../../${deps}/include -L../../../${deps}/lib -DSUPPORTIPV6"
        WINDRES="i686-w64-mingw32-windres"
        SDL_CONFIG="../../../${deps}/bin/sdl2-config"
        DP_LINK_CRYPTO=dlopen
        DP_LINK_CRYPTO_RIJNDAEL=dlopen
        DP_LINK_JPEG=dlopen
        DP_LINK_ODE=dlopen
        DP_LINK_ZLIB=dlopen'
      maketargets='release'
      outputs='darkplaces-sdl.exe:darkplaces-x86.exe darkplaces-dedicated.exe:darkplaces-x86-dedicated.exe'
      ;;
    win64)
      chroot=
      makeflags='STRIP=:
        D3D=1
        DP_MAKE_TARGET=mingw
        UNAME=MINGW32
        WIN64RELEASE=1
        CC="x86_64-w64-mingw32-gcc -static -g1 -Wl,--dynamicbase -Wl,--nxcompat -I../../../${deps}/include -L../../../${deps}/lib -DSUPPORTIPV6"
        WINDRES="x86_64-w64-mingw32-windres"
        SDL_CONFIG="../../../${deps}/bin/sdl2-config"
        DP_LINK_CRYPTO=dlopen
        DP_LINK_CRYPTO_RIJNDAEL=dlopen
        DP_LINK_JPEG=dlopen
        DP_LINK_ODE=dlopen
        DP_LINK_ZLIB=dlopen'
      maketargets='release'
      outputs='darkplaces-sdl.exe:darkplaces.exe darkplaces-dedicated.exe:darkplaces-dedicated.exe'
      ;;
    osx)
      chroot=
      makeflags='STRIP=:
        CC="gcc -g1 -arch x86_64 -mmacosx-version-min=10.6 -Wl,-rpath -Wl,@loader_path/../Frameworks -Wl,-rpath -Wl,@loader_path -I../../../${deps}/include -L../../../${deps}/lib -DSUPPORTIPV6"
        SDLCONFIG_MACOSXCFLAGS="-I${PWD}/SDL2.framework/Headers"
        SDLCONFIG_MACOSXLIBS="-F${PWD} -framework SDL2 -framework Cocoa -I${PWD}/SDL2.framework/Headers"
        SDLCONFIG_MACOSXSTATICLIBS="-F${PWD} -framework SDL2 -framework Cocoa -I${PWD}/SDL2.framework/Headers"
        DP_LINK_CRYPTO=dlopen
        DP_LINK_CRYPTO_RIJNDAEL=dlopen
        DP_LINK_JPEG=dlopen
        DP_LINK_ODE=dlopen
        DP_LINK_ZLIB=shared'
      maketargets='sv-release sdl-release'
      outputs='darkplaces-sdl:darkplaces-osx-sdl-bin darkplaces-dedicated:darkplaces-osx-dedicated'
      ;;
  esac

  # Condense whitespace in makeflags.
  makeflags=$(
    printf "%s\n" "$makeflags" | tr '\n' ' '
  )

  (
    trap "${chroot} make -C ${PWD} ${makeflags} clean" EXIT
    eval "${chroot} make -C ${PWD} ${makeflags} ${maketargets}"
    for o in $outputs; do
      src=${o%%:*}
      dst=${o#*:}
      sftp -oStrictHostKeyChecking=no -i /tmp/id_xonotic -P 2342 -b - autobuild-bin-uploader@srv04.xonotic.org <<EOF
put ${src} ${rev}/${dst}
EOF
    done
  )

done
