cmake_minimum_required( VERSION 3.5.0 )

set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG" )

add_library( ${ECWOLF_PREFIX}gdtoa STATIC
	dmisc.c
	dtoa.c
	misc.c
)

# Disable warnings for << operator precedence (4554) and
# unreferenced labels (4102) from VC
if( MSVC )
	target_compile_options(${ECWOLF_PREFIX}gdtoa PRIVATE /wd4554 /wd4102 )
endif()

target_include_directories(${ECWOLF_PREFIX}gdtoa PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(${ECWOLF_PREFIX}gdtoa PRIVATE -DINFNAN_CHECK -DMULTIPLE_THREADS)

# These can't be run effectively in a cross compiling environment so we'll just
# not generate the header and let it error if we haven't collected the data
# statically.
if( NOT MSVC AND NOT APPLE AND NOT CMAKE_CROSSCOMPILING )
	add_executable( arithchk arithchk.c )
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arith.h 
		COMMAND arithchk >${CMAKE_CURRENT_BINARY_DIR}/arith.h
		DEPENDS arithchk )

	add_executable( qnan qnan.c arith.h )
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gd_qnan.h
		COMMAND qnan >${CMAKE_CURRENT_BINARY_DIR}/gd_qnan.h
		DEPENDS qnan )
	target_include_directories( qnan PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )

	target_sources(${ECWOLF_PREFIX}gdtoa PRIVATE arith.h gd_qnan.h)
endif()
