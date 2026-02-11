cmake_minimum_required(VERSION 3.5.0)
project(ECWolf)

set(DIII4A True)
if(DIII4A)
	set(ECWOLF_PREFIX "ecwolf_")
	add_definitions(-D_DIII4A)
	include_directories(${DIII4A_DEPEND_COMMON_PATH})
endif()
# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

include(src/versiondefs.cmake)

# We need a newer CPack version to get desirable results.
# Note: CMake 3.11+ recommended for best results
if(NOT CMAKE_VERSION VERSION_LESS "3.6.0")
	set(USE_CPACK ON)
else()
	set(USE_CPACK OFF)
endif()

if(USE_CPACK)
	set(CPACK_PACKAGE_CONTACT "First Last <example@example.com>" CACHE STRING "Contact info for archive maintainer.")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Advanced source port of the Wolfenstein 3D engine.\n Provides enhanced game engine for playing Wolfenstein 3D, Spear of Destiny, and\n Super 3-D Noah's Ark. Requires the original game data files.")
	string(TOLOWER "${PRODUCT_NAME}" CPACK_PACKAGE_NAME)
	# Use same prefix for packaging for consistency
	set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
	set(CPACK_STRIP_FILES ON)
	set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
	set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/CPackProjectConfig.cmake")

	if(WIN32)
		set(CPACK_GENERATOR "ZIP")
	else()
		set(CPACK_GENERATOR "DEB")
		set(CPACK_DEB_COMPONENT_INSTALL ON)
		set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
		set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://maniacsvault.net/ecwolf/")
		set(CPACK_DEBIAN_PACKAGE_SECTION "games")
		set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
		set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
		set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
	endif()

	include(CPack)
endif()

# Multiarch support
if(UNIX AND NOT APPLE)
	# Default to "games" as per the Debian packaging guidelines
	# However, it can be overridden for other distributions or as needed
	set(CMAKE_INSTALL_BINDIR "games" CACHE STRING "Installation directory for binaries")
	# Include GNUInstallDirs to get standard install directories
	include(GNUInstallDirs)
	if(CPACK_PACKAGE_NAME)
		# Match this to package name instead of project name
		set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_PACKAGE_NAME}")
	endif()
	# GNUInstallDirs doesn't provide this, but we could use share/product_name
	set(CMAKE_INSTALL_DATAPRODUCTDIR "${CMAKE_INSTALL_DATADIR}/${PRODUCT_DIRECTORY}")
else()
	# CMAKE_ prefix used here to match the variable set by GNUInstallDirs.
	# Value currently doesn't matter outside of Linux these are more or less
	# to keep the script working where values are expected.
	if(NOT WIN32)
		set(CMAKE_INSTALL_BINDIR "bin")
		set(CMAKE_INSTALL_DATAROOTDIR "share")
		set(CMAKE_INSTALL_LIBDIR "lib")
		set(CMAKE_INSTALL_MANDIR "share/man")
		set(CMAKE_INSTALL_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}")
		set(CMAKE_INSTALL_DATAPRODUCTDIR "${CMAKE_INSTALL_DATADIR}/${PRODUCT_DIRECTORY}")
		if(CPACK_PACKAGE_NAME)
			set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_PACKAGE_NAME}")
		else()
			set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/${PRODUCT_DIRECTORY}")
		endif()
	else()
		set(CMAKE_INSTALL_BINDIR ".")
		set(CMAKE_INSTALL_DATAROOTDIR ".")
		set(CMAKE_INSTALL_LIBDIR ".")
		set(CMAKE_INSTALL_MANDIR ".")
		set(CMAKE_INSTALL_DATADIR ".")
		set(CMAKE_INSTALL_DATAPRODUCTDIR ".")
		set(CMAKE_INSTALL_DOCDIR ".")
	endif()

	# Vendored libraries might include GNUInstallDirs so do so now
	include(GNUInstallDirs)
endif()

# Support cross compiling
option(TOOLS_ONLY "Only build the tools, not ecwolf itself (useful for cross-compiling)" OFF)

option( FORCE_CROSSCOMPILE "Turn on cross compiling." NO )
if( FORCE_CROSSCOMPILE )
	set( CMAKE_CROSSCOMPILING TRUE )
endif()

if(CMAKE_CROSSCOMPILING AND NOT DIII4A)
	set(IMPORT_EXECUTABLES "IMPORTFILE-NOTFOUND" CACHE FILEPATH "Export file from native build.")
	include(${IMPORT_EXECUTABLES})
endif()

# Replacement variables for a possible long list of C/C++ compilers compatible with GCC
if( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
	set( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE TRUE )
else()
	set( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE FALSE )
endif()

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
	set( ZD_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE TRUE )
else()
	set( ZD_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE FALSE )
endif()

# Simplify pk3 building, add_pk3(filename srcdirectory)
function( add_pk3 PK3_NAME PK3_DIR )
	# Generate target name. Just use "pk3" for main pk3 target.
	string( REPLACE "." "_" PK3_TARGET ${PK3_NAME} )
	if( ${PK3_TARGET} STREQUAL "ecwolf_pk3" )
		set( PK3_TARGET "pk3" )
	endif()

	# Touch the zipdir executable here so that the pk3s are forced to rebuild
	# each time since their dependecy has "changed."
	if( NOT ANDROID )
		set(TARGET_DIR "$<TARGET_FILE_DIR:engine>")
		add_custom_target( ${PK3_TARGET} ALL
			COMMAND zipdir -udf $<TARGET_FILE_DIR:engine>/${PK3_NAME} ${PK3_DIR}
			DEPENDS zipdir )
		install(FILES ${TARGET_DIR}/${PK3_NAME} DESTINATION "${CMAKE_INSTALL_DATAPRODUCTDIR}")
	else()
		# On Android need to put pk3 in an assets directory
		add_custom_target( ${PK3_TARGET} ALL
			COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/assets
			COMMAND zipdir -udf ${OUTPUT_DIR}/assets/${PK3_NAME} ${PK3_DIR}
			DEPENDS zipdir )
	endif()
endfunction()

set(OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory in which to build ECWolf.")

option(GPL "Build GPL edition" ON)
option(USE_LIBTEXTSCREEN "Use libtextscreen instead of console iwad picker." OFF)

option(INTERNAL_ZLIB "Force build with internal zlib" OFF)
option(INTERNAL_BZIP2 "Force build with internal bzip2" OFF)
option(INTERNAL_JPEG "Force build with internal libjpeg" OFF)

if(DIII4A)
	set(INTERNAL_JPEG ON)
	set(INTERNAL_BZIP2 ON)
	set(INTERNAL_ZLIB ON)
endif()

if(NOT INTERNAL_ZLIB)
	find_package(ZLIB QUIET)
endif()
if(NOT INTERNAL_BZIP2)
	find_package(BZip2 QUIET)
endif()
if(NOT INTERNAL_JPEG)
	find_package(JPEG QUIET)
endif()

if(NOT ZLIB_FOUND OR INTERNAL_ZLIB)
	message(STATUS "Using internal zlib")
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib)
	add_subdirectory(deps/zlib)
	set(ZLIB_LIBRARIES ${ECWOLF_PREFIX}ZLIB::ZLIB)
	set(ZLIB_LIBRARY ${ECWOLF_PREFIX}ZLIB::ZLIB)
else()
	message(STATUS "Using system zlib, includes found at ${ZLIB_INCLUDE_DIR}")
endif()

if(NOT BZIP2_FOUND OR INTERNAL_BZIP2)
	message(STATUS "Using internal libbz2")
	set(BZIP2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/bzip2)
	add_subdirectory(deps/bzip2)
	set(BZIP2_LIBRARIES ${ECWOLF_PREFIX}bz2)
else()
	message(STATUS "Using system libbz2, includes found at ${BZIP2_INCLUDE_DIR}")
endif()

if(NOT TOOLS_ONLY)
if(NOT JPEG_FOUND OR INTERNAL_JPEG)
	message(STATUS "Using internal libjpeg")
	set(JPEG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/jpeg-6b)
	add_subdirectory(deps/jpeg-6b)
	set(JPEG_LIBRARIES ${ECWOLF_PREFIX}jpeg)
	set(JPEG_LIBRARY ${ECWOLF_PREFIX}jpeg)
else()
	message(STATUS "Using system libjpeg, includes found at ${JPEG_INCLUDE_DIR}")
endif()

	include(LocateSDL2.cmake)
endif()

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if(NOT DIII4A)
add_subdirectory(tools/zipdir)
endif()

add_subdirectory(deps/gdtoa)
add_subdirectory(deps/lzma)

if(GPL AND USE_LIBTEXTSCREEN)
	add_subdirectory(deps/textscreen)
endif()

if(ANDROID AND NOT DIII4A)
	set(LIBPNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android-libs)
	set(LIBPNG_LIBRARY png16_static)

	add_subdirectory(android-libs/libpng)
	add_subdirectory(android-libs/sigc++)
	add_subdirectory(android-libs/TinyXML)
	add_subdirectory(android-libs/TouchControls)
	add_subdirectory(android-libs/launcher)
endif()

add_subdirectory(src)
if (NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(wadsrc)
endif()

if(USE_CPACK AND NOT APPLE)
	if(WIN32)
		install(FILES docs/copyright DESTINATION "${CMAKE_INSTALL_DOCDIR}" RENAME licenses.txt)
	else()
		install(FILES docs/copyright DESTINATION "${CMAKE_INSTALL_DOCDIR}")
		install(FILES docs/changelog DESTINATION "${CMAKE_INSTALL_DOCDIR}")
		install(FILES docs/ecwolf.6 DESTINATION "${CMAKE_INSTALL_MANDIR}/man6")
		# this installs the Appstream Metainfo file (used by Flatpak)
		install(FILES docs/org.ecwolf.ECWolf.metainfo.xml DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")

		# Lintian requires that man pages in deb packages are compressed.
		find_program(GZIP_BINARY NAMES gzip DOC "gzip program for man page compression.")
		if(GZIP_BINARY)
			install(CODE "execute_process(COMMAND \"${GZIP_BINARY}\" -fn9 \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}/changelog\")")
			install(CODE "execute_process(COMMAND \"${GZIP_BINARY}\" -fn9 \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_MANDIR}/man6/ecwolf.6\")")
		endif()
	endif()
endif()

if( NOT CMAKE_CROSSCOMPILING )
	export(TARGETS ${CROSS_EXPORTS} FILE "${CMAKE_BINARY_DIR}/ImportExecutables.cmake" )
endif()
