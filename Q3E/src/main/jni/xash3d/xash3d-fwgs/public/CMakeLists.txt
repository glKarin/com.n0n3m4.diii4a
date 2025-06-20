cmake_minimum_required(VERSION 3.9)
project (xash3d-fwgs-public)

file(GLOB FWGS_PUBLIC_INCLUDES *.h)
file(GLOB FWGS_PUBLIC_SOURCES *.c)
add_library (fwgs_public STATIC ${FWGS_PUBLIC_SOURCES})
target_include_directories(fwgs_public PRIVATE . ../common ../pm_shared ../engine)
