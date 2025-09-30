cmake_minimum_required(VERSION 3.12)

project(omohsdl)

set(SOURCES_SDL_CLIENT
"./android_input.c"
"./android_mouse.c"
		"./ifaddrs.c"
)

set(SOURCES_SDL_GL
"./android_gamma.c"
"./android_glimp.c"
)

if (NO_MODERN_DMA)
	list(APPEND SOURCES_SDL "./android_snd.c")
endif()

add_library(omohsdl_client STATIC ${SOURCES_SDL_CLIENT})
target_compile_features(omohsdl_client PUBLIC c_variadic_macros)
target_link_libraries(omohsdl_client PRIVATE ${OPENMOHAA_TARGET_PREFIX}qcommon ${OPENMOHAA_TARGET_PREFIX}qcommon_standalone)

if (NO_MODERN_DMA)
	target_compile_definitions(omohsdl_client PRIVATE NO_MODERN_DMA=1)
endif()

add_library(omohsdl_gl STATIC ${SOURCES_SDL_GL})
target_link_libraries(omohsdl_gl PRIVATE ${OPENMOHAA_TARGET_PREFIX}qcommon)
target_link_libraries(omohsdl_gl PUBLIC EGL GLESv1_CM android)

if(${CMAKE_VERSION} VERSION_GREATER "3.11")
	cmake_policy(SET CMP0074 NEW)
endif()

add_library(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE)

if(WIN32)
	find_package(SDL2)

	if (SDL2_FOUND)
		string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
		target_include_directories(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE ${SDL2_INCLUDE_DIRS})
		target_link_libraries(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE ${SDL2_LIBRARIES})
	else()
		message(WARNING "SDL2 not found, falling back to using SDL2 from the source tree")

		target_include_directories(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE "../thirdparty/SDL2-2.32.8/include")

		if (MSVC)
			if(CMAKE_SIZEOF_VOID_P EQUAL 8)
				add_library(sdl2 SHARED IMPORTED)
				set_target_properties(sdl2 PROPERTIES
				  IMPORTED_IMPLIB "${OPENMOHAA_SOURCE_DIR}/code/thirdparty/libs/win64/SDL2.lib"
				)

				add_library(sdl2main SHARED IMPORTED)
				set_target_properties(sdl2main PROPERTIES
				  IMPORTED_IMPLIB "${OPENMOHAA_SOURCE_DIR}/code/thirdparty/libs/win64/SDL2main.lib"
				)
			else()
				add_library(sdl2 SHARED IMPORTED)
				set_target_properties(sdl2 PROPERTIES
				  IMPORTED_IMPLIB "${OPENMOHAA_SOURCE_DIR}/code/thirdparty/libs/win32/SDL2.lib"
				)

				add_library(sdl2main SHARED IMPORTED)
				set_target_properties(sdl2main PROPERTIES
				  IMPORTED_IMPLIB "${OPENMOHAA_SOURCE_DIR}/code/thirdparty/libs/win32/SDL2main.lib"
				)
			endif()

			target_link_libraries(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE sdl2 sdl2main)
		endif()
	endif()

elseif(ANDROID) #k
	target_include_directories(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE "${OPENMOHAA_COMMON_DIR}")
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE oboeaudio)
elseif(UNIX)
	find_package(SDL2 REQUIRED)
	
	if (SDL2_FOUND)
		string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
		target_include_directories(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE ${SDL2_INCLUDE_DIRS})
		target_link_libraries(${OPENMOHAA_TARGET_PREFIX}sdllib INTERFACE ${SDL2_LIBRARIES})
	endif()
endif()

target_link_libraries(omohsdl_client PUBLIC ${OPENMOHAA_TARGET_PREFIX}sdllib)
target_link_libraries(omohsdl_gl PUBLIC ${OPENMOHAA_TARGET_PREFIX}sdllib)
