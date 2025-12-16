cmake_minimum_required(VERSION 3.10)
project(cs16-client)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(LibraryNaming)
# include(ExternalProject)

if(DIII4A)
set(CS16_C_FLAGS "-Wno-ignored-attributes")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CS16_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CS16_C_FLAGS}")
endif()


set(BUILD_CLIENT ON)
set(BUILD_MAINUI ON)
set(BUILD_SERVER ON)
set(GAME_DIR "cstrike" CACHE STRING "Game directory name.")
set(CLIENT_INSTALL_DIR "cl_dlls" CACHE STRING "Client library location.")
set(SERVER_INSTALL_DIR "dlls" CACHE STRING "Server library location.")

# if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
# 	add_compile_options(/fsanitize=address)
# 	add_link_options(/fsanitize=address)
# endif()

if(VITA)
	message(STATUS "Building for PS Vita")
	add_compile_options(-fno-use-cxa-atexit)
endif()

if(BUILD_CLIENT)
	add_subdirectory(cl_dll)
endif()

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
# 	set(MESON_BUILD_TYPE debug)
# elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
# 	set(MESON_BUILD_TYPE debugoptimized)
# else()
# 	set(MESON_BUILD_TYPE release)
# endif()

if(BUILD_SERVER)
	# ExternalProject_Add(
	# 	yapb
	# 	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/yapb
	# 	CONFIGURE_COMMAND meson setup --buildtype ${MESON_BUILD_TYPE} build
	# 	BUILD_COMMAND meson compile -C build
	# 	BUILD_ALWAYS 1
	# 	BUILD_IN_SOURCE 1
	# 	INSTALL_COMMAND meson install -C build --destdir ${CMAKE_INSTALL_PREFIX}
	# )
	add_subdirectory(3rdparty/yapb)
	set_target_postfix(cs16_yapb)

	set(XASH_COMPAT ON CACHE BOOL "" FORCE)
	add_subdirectory(3rdparty/ReGameDLL_CS)
endif()

if(BUILD_MAINUI)
#	set(CS16CLIENT ON)
	# set(XASH_SDK ../../)
	add_subdirectory(3rdparty/mainui_cpp)
endif()

if(NOT DIII4A) #k: don't package extras
set(GRAPHS_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/graphs")

execute_process(
	COMMAND ${Python_EXECUTABLE}
	${CMAKE_CURRENT_SOURCE_DIR}/scripts/yapb_graph_dl.py
	${GRAPHS_OUTPUT}/addons/yapb/data/graph
)

set(EXTRAS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cs16client-extras")
set(YAPB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/yapb/cfg")

if(ANDROID)
	set(EXTRAS_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/assets")

	file(GLOB EXTRAS_FILES "${EXTRAS_DIR}/*")
	file(GLOB YAPB_FILES "${YAPB_DIR}/*" "${GRAPHS_OUTPUT}/*")

	file(REMOVE_RECURSE ${EXTRAS_OUTPUT})
	file(
		COPY ${EXTRAS_FILES} ${YAPB_FILES}
		DESTINATION ${EXTRAS_OUTPUT}
	)
else()
	set(EXTRAS_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/extras.pk3")

	execute_process(
		COMMAND ${Python_EXECUTABLE}
		${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack_extras.py
		${EXTRAS_OUTPUT}
		${EXTRAS_DIR}
		${YAPB_DIR}
		${GRAPHS_OUTPUT}
	)

	add_custom_target(generate_extras ALL
		DEPENDS ${GRAPHS_OUTPUT} ${EXTRAS_OUTPUT}
	)

	install(
		FILES "${EXTRAS_OUTPUT}"
		DESTINATION ${GAME_DIR}
	)
endif()

if(VITA)
	install(
		FILES
		"${CMAKE_CURRENT_BINARY_DIR}/config.cfg"
		"${CMAKE_CURRENT_BINARY_DIR}/video.cfg"
		"${CMAKE_CURRENT_BINARY_DIR}/opengl.cfg"
		DESTINATION "${GAME_DIR}/"
	)

	install(
		FILES
		"${CMAKE_CURRENT_BINARY_DIR}/kb_def.lst"
		DESTINATION "${GAME_DIR}/gfx/shell"
	)
endif()

if(WIN32)
	set(CPACK_GENERATOR "ZIP")
	set(CPACK_PACKAGE_FILE_NAME "CS16Client-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
elseif(VITA)
	set(CPACK_GENERATOR "ZIP")
	set(CPACK_PACKAGE_FILE_NAME "CS16Client-PSVita")
else()
	set(CPACK_GENERATOR "TGZ")
	set(CPACK_PACKAGE_FILE_NAME "CS16Client-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

include(CPack)
endif()