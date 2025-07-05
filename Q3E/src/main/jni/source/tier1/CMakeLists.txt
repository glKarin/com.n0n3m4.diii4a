cmake_minimum_required(VERSION 3.9)
project (source-tier1)

set(binary_name ${SOURCE_ENGINE_PREFIX}tier1)

set(source_src 
		../utils/lzma/C/LzmaDec.c
		bitbuf.cpp
		byteswap.cpp
		characterset.cpp
		checksum_crc.cpp
		checksum_md5.cpp
		checksum_sha1.cpp
		commandbuffer.cpp
		convar.cpp
		datamanager.cpp
		diff.cpp
		generichash.cpp
		ilocalize.cpp
		interface.cpp
		KeyValues.cpp
		keyvaluesjson.cpp
		kvpacker.cpp
		lzmaDecoder.cpp
		lzss.cpp # [!$SOURCESDK]
		mempool.cpp
		memstack.cpp
		NetAdr.cpp
		newbitbuf.cpp
		rangecheckedvar.cpp
		reliabletimer.cpp
		snappy-sinksource.cpp
		snappy-stubs-internal.cpp
		snappy.cpp
		sparsematrix.cpp
		splitstring.cpp
		stringpool.cpp
		strtools.cpp
		strtools_unicode.cpp
		tier1.cpp
		tokenreader.cpp
		uniqueid.cpp
		utlbinaryblock.cpp
		utlbuffer.cpp
		utlbufferutil.cpp
		utlstring.cpp
		utlsymbol.cpp
)
set(source_src
	${source_src}
			#pathmatch.cpp
			processor_detect_linux.cpp
			qsort_s.cpp
)

set(source_inc 
		.
		../
		../public
		../public/tier1
		../public/tier0
		../common
)

set(source_def "TIER1_STATIC_LIB=1")

set(source_lib 
	iconv
)

add_library (${binary_name} STATIC ${source_src})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_include_directories(${binary_name} PRIVATE ${source_inc})
target_link_libraries(${binary_name} PRIVATE ${source_lib})

