cmake_minimum_required(VERSION 3.9)
project (source-togles)

set(binary_name ${SOURCE_ENGINE_PREFIX}togl)

set(source_src 
		linuxwin/dx9asmtogl2.cpp
		linuxwin/dxabstract.cpp
		linuxwin/glentrypoints.cpp	
		linuxwin/glmgr.cpp			
		linuxwin/glmgrbasics.cpp	
		#linuxwin/intelglmallocworkaround.cpp		[$OSXALL]
		#linuxwin/mach_override.c					[$OSXALL]
		linuxwin/cglmtex.cpp		
		linuxwin/cglmfbo.cpp		
		linuxwin/cglmprogram.cpp	
		linuxwin/cglmbuffer.cpp	
		linuxwin/cglmquery.cpp
		linuxwin/asanstubs.cpp
		linuxwin/decompress.c
)

set(source_inc 
		.
		../public
		../public/tier0
		../public/tier1
)

set(source_def "TOGL_DLL_EXPORT=1;strncpy=use_Q_strncpy_instead;_snprintf=use_Q_snprintf_instead")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}tier2 
	${SOURCE_ENGINE_PREFIX}vstdlib 
	${SOURCE_ENGINE_PREFIX}mathlib  
	)

	add_library (${binary_name} SHARED ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

