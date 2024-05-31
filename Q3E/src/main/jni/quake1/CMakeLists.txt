cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(quake1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(q1libs android log EGL z)
set(q1libs ${q1libs} vorbisfile vorbisenc vorbis png jpeg ogg freetype oboeaudio)
#set(q1libs ${q1libs} curl)
#message(FATAL_ERROR "STOP")
# -fshort-enums
set(CFLAGS_WARNINGS "-Wall -Werror=vla -Werror=c++-compat -Wwrite-strings -Wshadow -Wold-style-definition -Wstrict-prototypes -Wsign-compare -Wdeclaration-after-statement -Wmissing-prototypes")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CFLAGS_WARNINGS} -O3 -fno-math-errno -fno-trapping-math -Winline")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# Switch off some annoying warnings
if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    if (CMAKE_C_COMPILER_VERSION GREATER 7.99)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation -Wno-format-overflow")
    endif()
endif()

set(cpu ${CMAKE_SYSTEM_PROCESSOR})
if(cpu STREQUAL "powerpc")
    set(cpu "ppc")
elseif(cpu STREQUAL "aarch64")
    # "arm64" is more obvious, and some operating systems (like macOS) use it instead of "aarch64"
    set(cpu "arm64")
elseif(cpu MATCHES "[aA][mM][dD]64" OR cpu MATCHES "[xX].*64")
    set(cpu "x86_64")
elseif(cpu MATCHES "i.86" OR cpu MATCHES "[xX]86")
    set(cpu "i386")
elseif(cpu MATCHES "[aA][rR][mM].*") # some kind of arm..
    # On 32bit Raspbian gcc -dumpmachine returns sth starting with "arm-",
    # while clang -dumpmachine says "arm6k-..." - try to unify that to "arm"
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # sizeof(void*) == 8 => must be arm64
        set(cpu "arm64")
    else() # should be 32bit arm then (probably "armv7l" "armv6k" or sth like that)
        set(cpu "arm")
    endif()
endif()
set(ARCH "${cpu}")
add_definitions(-DARCH_STRING="${ARCH}")

set(q1sound_src
	snd_main.c snd_mem.c snd_mix.c snd_ogg.c snd_wav.c
)

set(q1common_src
	bih.c
	crypto.c
	cd_shared.c
	cl_cmd.c
	cl_collision.c
	cl_demo.c
	cl_ents.c
	cl_ents4.c
	cl_ents5.c
	cl_ents_nq.c
	cl_ents_qw.c
	cl_input.c
	cl_main.c
	cl_parse.c
	cl_particles.c
	cl_screen.c
	cl_video.c
	cl_video_libavw.c
	clvm_cmds.c
	cmd.c
	collision.c
	com_crc16.c
	com_ents.c
	com_ents4.c
	com_game.c
	com_infostring.c
	com_msg.c
	common.c
	console.c
	csprogs.c
	curves.c
	cvar.c
	dpvsimpledecode.c
	filematch.c
	fractalnoise.c
	fs.c
	ft2.c
	utf8lib.c
	gl_backend.c
	gl_draw.c
	gl_rmain.c
	gl_rsurf.c
	gl_textures.c
	hmac.c
	host.c
	image.c
	image_png.c
	jpeg.c
	keys.c
	lhnet.c
	libcurl.c
	mathlib.c
	matrixlib.c
	mdfour.c
	meshqueue.c
	mod_skeletal_animatevertices_sse.c
	mod_skeletal_animatevertices_generic.c
	model_alias.c
	model_brush.c
	model_shared.c
	model_sprite.c
	netconn.c
	palette.c
	phys.c
	polygon.c
	portals.c
	protocol.c
	prvm_cmds.c
	prvm_edict.c
	prvm_exec.c
	r_explosion.c
	r_lightning.c
	r_modules.c
	r_shadow.c
	r_sky.c
	r_sprites.c
	r_stats.c
	sbar.c
	sv_ccmds.c
	sv_demo.c
	sv_ents.c
	sv_ents4.c
	sv_ents5.c
	sv_ents_csqc.c
	sv_ents_nq.c
	sv_main.c
	sv_move.c
	sv_phys.c
	sv_save.c
	sv_send.c
	sv_user.c
	svbsp.c
	svvm_cmds.c
	sys_shared.c
	taskqueue.c
	vid_shared.c
	view.c
	wad.c
	world.c
	zone.c
        )

set(q1menu_src
	menu.c
	mvm_cmds.c
)

set(q1sound_src
		snd_main.c snd_mem.c snd_mix.c snd_ogg.c snd_wav.c
)

include_directories("../deplibs/oboe/include")
set(q1sys_src
	builddate.c
		android/snd_android.c
		android/vid_android.c
		android/sys.c
		thread_pthread.c
)

add_definitions(-DCONFIG_MENU -D_FILE_OFFSET_BITS=64 -D__KERNEL_STRICT_NAMES)
add_definitions(-D_DIII4A -D_NOSDL -D_GLDL -D_NOKTX -DLINK_TO_LIBJPEG=1) # _GLDBG
set(Q1LIBRARY idtech_quake)
add_library(${Q1LIBRARY} SHARED
        ${q1common_src}
        ${q1menu_src}
		${q1sound_src}
		${q1sys_src}
        )
target_link_libraries(${Q1LIBRARY}
		${q1libs}
		)
target_include_directories(${Q1LIBRARY} PRIVATE "./android/include")
target_include_directories(${Q1LIBRARY} PRIVATE "../deplibs/libvorbis/include")
target_include_directories(${Q1LIBRARY} PRIVATE "../deplibs/libjpeg/libjpeg")
target_include_directories(${Q1LIBRARY} PRIVATE "../common")


