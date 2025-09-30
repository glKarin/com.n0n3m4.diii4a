cmake_minimum_required(VERSION 3.12)

include_directories("./common")
include_directories("./gcdkey")

file(GLOB SRCS_common
	"./common/*.c"
    "./darray.c"
    "./gserver.c"
    "./gserverlist.c"
	"./hashtable.c"
	"./md5c.c"
	"./gutil.c"
)

file(GLOB SRCS_gcdkey
"./gcdkey/*.c"
)

file(GLOB_RECURSE SRCS_ghttp
"./ghttp/*.c"
)

file(GLOB SRCS_GP
"./GP/*.c"
)

file(GLOB_RECURSE SRCS_gstats
"./gstats/*.c"
)

file(GLOB SRCS_natneg
"./natneg/*.c"
)

file(GLOB SRCS_pinger
"./pinger/*.c"
)

file(GLOB SRCS_pt
"./pt/*.c"
)

file(GLOB SRCS_qr2
"./qr2/*.c"
)

file(GLOB SRCS_sake
"./sake/*.c"
)

file(GLOB SRCS_sc
"./sc/*.c"
)

file(GLOB SRCS_serverbrowsing
"./serverbrowsing/*.c"
)

file(GLOB_RECURSE SRCS_webservices
"./webservices/*.c"
)

add_library(${OPENMOHAA_TARGET_PREFIX}gcd_common STATIC ${SRCS_common})
add_library(${OPENMOHAA_TARGET_PREFIX}gcd_key STATIC ${SRCS_gcdkey})
#add_library(${OPENMOHAA_TARGET_PREFIX}gcd_gp STATIC ${SRCS_GP})
#add_library(${OPENMOHAA_TARGET_PREFIX}gcd_gstats STATIC ${SRCS_gstats})
add_library(${OPENMOHAA_TARGET_PREFIX}gcd_natneg STATIC ${SRCS_natneg})
#add_library(${OPENMOHAA_TARGET_PREFIX}gcd_pinger STATIC ${SRCS_pinger})
#add_library(${OPENMOHAA_TARGET_PREFIX}gcd_pt STATIC ${SRCS_pt})
add_library(${OPENMOHAA_TARGET_PREFIX}gcd_qr2 STATIC ${SRCS_qr2})
#add_library(${OPENMOHAA_TARGET_PREFIX}gcd_sake STATIC ${SRCS_sake})
#add_library(${OPENMOHAA_TARGET_PREFIX}gcd_sc STATIC ${SRCS_sc})
#add_library(${OPENMOHAA_TARGET_PREFIX}gcd_serverbrowsing STATIC ${SRCS_serverbrowsing})
#add_library(gcd_webservices STATIC ${SRCS_webservices})
add_library(${OPENMOHAA_TARGET_PREFIX}gcd INTERFACE)

set_property(TARGET ${OPENMOHAA_TARGET_PREFIX}gcd_common ${OPENMOHAA_TARGET_PREFIX}gcd_key PROPERTY POSITION_INDEPENDENT_CODE ON)

if(UNIX)
	add_definitions(-D_LINUX=1)
endif(UNIX)

set(DEPENDENT_LIBS
	${OPENMOHAA_TARGET_PREFIX}gcd_key
#	gcd_gp
#	gcd_gstats
	${OPENMOHAA_TARGET_PREFIX}gcd_natneg
#	gcd_pinger
#	gcd_pt
	${OPENMOHAA_TARGET_PREFIX}gcd_qr2
#	gcd_sake
#	gcd_sc
#	gcd_serverbrowsing
#	gcd_webservices
)

target_link_libraries(${OPENMOHAA_TARGET_PREFIX}gcd INTERFACE ${DEPENDENT_LIBS})

foreach(LIB ${DEPENDENT_LIBS})
	target_link_libraries(${LIB} PUBLIC ${OPENMOHAA_TARGET_PREFIX}gcd_common)
endforeach()

target_link_libraries(${OPENMOHAA_TARGET_PREFIX}gcd_key PRIVATE ${OPENMOHAA_TARGET_PREFIX}gcd_qr2)
target_link_libraries(${OPENMOHAA_TARGET_PREFIX}gcd_qr2 PRIVATE ${OPENMOHAA_TARGET_PREFIX}gcd_natneg)
