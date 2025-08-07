cmake_minimum_required(VERSION 3.9)
project (source-tier0)

set(binary_name ${SOURCE_ENGINE_PREFIX}tier0)
set(binary_output_name ${SOURCE_GAME_PREFIX}tier0)

set(source_src 
		assert_dialog.cpp
		commandline.cpp
		cpu.cpp
		cpumonitoring.cpp
		cpu_usage.cpp
		dbg.cpp
		dynfunction.cpp
		fasttimer.cpp
		# InterlockedCompareExchange128.masm [$WIN64]
		mem.cpp
		mem_helpers.cpp
		memdbg.cpp
		memstd.cpp
		memvalidate.cpp
		minidump.cpp
		pch_tier0.cpp
		#pmc360.cpp		[$X360]
		PMELib.cpp		#[$WINDOWS||$POSIX]
		progressbar.cpp
		security.cpp
		systeminformation.cpp
		stacktools.cpp
		thread.cpp		#[$WINDOWS||$POSIX]
		threadtools.cpp
		tier0_strtools.cpp
		tslist.cpp
		vprof.cpp
		#../tier1/pathmatch.cpp # [$LINUXALL]
)
set(source_src 
	${source_src}
			cpu_posix.cpp
			platform_posix.cpp
			pme_posix.cpp
			vcrmode_posix.cpp
)

set(source_inc 
		.
		../public
		../public/tier0
)

set(source_def "WAF_CFLAGS=\"CFLAGS\";WAF_LDFLAGS=\"LDFLAGS\";TIER0_DLL_EXPORT=1")

set(source_lib 
	dl 
	m 
	log
)

add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

set_target_properties(${binary_name} PROPERTIES OUTPUT_NAME "${binary_output_name}")
