cmake_minimum_required(VERSION 3.5)
project(cs16_regamedll CXX)

if (CMAKE_HOST_WIN32)
	add_custom_target(appversion DEPENDS
		COMMAND "${PROJECT_SOURCE_DIR}/regamedll/msvc/PreBuild.bat"
		"${PROJECT_SOURCE_DIR}/regamedll/version" "${PROJECT_SOURCE_DIR}/regamedll"
	)
else()
	add_custom_target(appversion DEPENDS
		COMMAND "${PROJECT_SOURCE_DIR}/regamedll/version/appversion.sh" "${PROJECT_SOURCE_DIR}"
	)
endif()

add_subdirectory(regamedll)
