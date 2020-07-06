#/bin/bash

ARCH=armeabi-v7a
FPU=neon
DIR=${PWD}
BUILD=release

if [ "x$2" = 'xdebug' ]; then
	BUILD='debug';
fi
BUILD_DIR=${DIR}/build/${BUILD}

if [ "x$1" = 'xvfp' ]; then
	FPU='vfp';
fi

if [ "x$3" = 'xarmeabi' ]; then
	ARCH='armeabi';
fi
DST_DIR=${DIR}/build/package/${ARCH}/${FPU}

if [ ${FPU} = 'neon' ]; then
	SUFFIX='_neon';
fi

if [ -e ${DST_DIR} ]; then
	rm -rf ${DST_DIR};
fi
mkdir -p ${DST_DIR};

BUILD_DIRS=`ls ${BUILD_DIR}`;
for i in ${BUILD_DIRS}; do
	so=`ls ${BUILD_DIR}/$i/sys/scons/lib*.so`;
	filename=`basename $so .so`;
	if [ $i = 'base' -o $i = 'core' ]; then
		target_filename=${filename}${SUFFIX}.so;
	else
		target_filename=lib${i}${SUFFIX}.so;
	fi
	cmd="cp -f $so ${DST_DIR}/$target_filename";
	echo $cmd;
	$cmd;
done
