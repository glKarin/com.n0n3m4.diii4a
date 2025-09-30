cmake_minimum_required( VERSION 3.16 )

if( COMMAND cmake_policy )
	if( POLICY CMP0011 )
		cmake_policy( SET CMP0011 NEW )
	endif()
	if( POLICY CMP0054 )
		cmake_policy( SET CMP0054 NEW )
	endif()
	if ( POLICY CMP0067 )
		cmake_policy( SET CMP0067 NEW )
	endif()
	if ( POLICY CMP0091 )
		cmake_policy( SET CMP0091 NEW )
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()
endif()


set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_command( OUTPUT ${SRC_DIR}/xlat_parser.c ${SRC_DIR}/xlat_parser.h
	COMMAND lemon -C${SRC_DIR} ${SRC_DIR}/gamedata/xlat/xlat_parser.y
	DEPENDS lemon ${SRC_DIR}/gamedata/xlat/xlat_parser.y )

add_custom_command( OUTPUT ${SRC_DIR}/zcc-parse.c ${SRC_DIR}/zcc-parse.h
	COMMAND lemon -C${SRC_DIR} ${SRC_DIR}/common/scripting/frontend/zcc-parse.lemon
	DEPENDS lemon ${SRC_DIR}/common/scripting/frontend/zcc-parse.lemon )

add_custom_command( OUTPUT ${SRC_DIR}/sc_man_scanner.h
	COMMAND re2c --no-generation-date -s -o ${SRC_DIR}/sc_man_scanner.h ${SRC_DIR}/common/engine/sc_man_scanner.re
	DEPENDS re2c ${SRC_DIR}/common/engine/sc_man_scanner.re )

add_subdirectory( tools )

add_custom_target(final ALL
    COMMAND echo ${SRC_DIR}/xlat_parser.c ${SRC_DIR}/xlat_parser.h ${SRC_DIR}/zcc-parse.c ${SRC_DIR}/zcc-parse.h ${SRC_DIR}/sc_man_scanner.h;
    COMMAND start "" ${SRC_DIR}
    DEPENDS ${SRC_DIR}/xlat_parser.c ${SRC_DIR}/xlat_parser.h ${SRC_DIR}/zcc-parse.c ${SRC_DIR}/zcc-parse.h ${SRC_DIR}/sc_man_scanner.h
)
