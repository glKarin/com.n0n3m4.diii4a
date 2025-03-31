cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(realrtcw_5_0)
set(REALRTCW_VERSION "5.0")
set(REALRTCW_SUFFIX "_5_0")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 98)

option(USE_CODEC_OPUS "" ON)
option(USE_FREETYPE "" ON)
option(USE_INTERNAL_JPEG "" ON)
option(USE_INTERNAL_VORBIS "" ON)
option(USE_INTERNAL_OGG "" ON)
option(USE_INTERNAL_FREETYPE "" ON)
option(USE_INTERNAL_ZLIB "" OFF)
option(USE_BLOOM "" ON)
option(USE_VOIP "" OFF)
option(USE_MUMBLE "" OFF)

add_definitions(-DPRODUCT_VERSION=\"${REALRTCW_VERSION}\")
add_definitions(-D_DIII4A) # android
add_definitions(-DNO_VM_COMPILED -DUSE_LOCAL_HEADERS) # base
add_definitions(-DUSE_OPENAL -DUSE_OPENAL_DLOPEN) # OpenAL dlopen
add_definitions(-DUSE_OPENGLES) # OpenGLES 1.1
add_definitions(-DUSE_CODEC_VORBIS) # ogg
add_definitions(-DUSE_CURL -DUSE_CURL_DLOPEN) # curl dlopen
if(USE_CODEC_OPUS)
    add_definitions(-DUSE_CODEC_OPUS -DOPUS_BUILD -DHAVE_LRINTF -DFLOATING_POINT -DFLOAT_APPROX -DUSE_ALLOCA) # opus
endif()
if(USE_VOIP)
add_definitions(-DUSE_VOIP)
endif()
if(USE_BLOOM)
add_definitions(-DUSE_BLOOM)
endif()

set(realrtcwclient_libs android log GLESv1_CM EGL)
set(realrtcwclient_libs ${realrtcwclient_libs} oboeaudio)
if(NOT USE_INTERNAL_OGG)
set(realrtcwclient_libs ${realrtcwclient_libs} ogg)
endif()
if(NOT USE_INTERNAL_VORBIS)
set(realrtcwclient_libs ${realrtcwclient_libs} vorbisfile vorbis)
endif()
if(NOT USE_INTERNAL_JPEG)
    set(realrtcwclient_libs ${realrtcwclient_libs} jpeg)
else()
    add_definitions(-DUSE_INTERNAL_JPEG) # jpeg
endif()
if(USE_FREETYPE)
    if(NOT USE_INTERNAL_FREETYPE)
        set(realrtcwclient_libs ${realrtcwclient_libs} freetype)
    else()
        add_definitions(-DFT2_BUILD_LIBRARY) # freetype2
    endif()
endif()
if(NOT USE_INTERNAL_ZLIB)
    set(realrtcwclient_libs ${realrtcwclient_libs} z)
else()
    add_definitions(-DNO_GZIP) # zlib
endif()
#set(realrtcwclient_libs ${realrtcwclient_libs} curl)
#message(FATAL_ERROR "STOP")
# -fshort-enums

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O3 -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization") # -Qunused-arguments
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O3 -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization") # -Qunused-arguments

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
add_definitions(-D_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P})
if(CMAKE_CXX_BYTE_ORDER EQUAL BIG_ENDIAN)
    set(big_endian 1)
else()
    set(big_endian 0)
endif()
add_definitions(-D_IS_BIG_ENDIAN=${big_endian})

set(realrtcwcgame_src
        code/cgame/cg_main.c
        code/game/bg_animation.c
        code/game/bg_misc.c
        code/game/bg_pmove.c
        code/game/bg_slidemove.c
        code/game/bg_lib.c
        code/cgame/cg_atmospheric.c
        code/cgame/cg_consolecmds.c
        code/cgame/cg_draw.c
        code/cgame/cg_drawtools.c
        code/cgame/cg_effects.c
        code/cgame/cg_ents.c
        code/cgame/cg_event.c
        code/cgame/cg_flamethrower.c
        code/cgame/cg_info.c
        code/cgame/cg_localents.c
        code/cgame/cg_marks.c
        code/cgame/cg_newdraw.c
        code/cgame/cg_particles.c
        code/cgame/cg_players.c
        code/cgame/cg_playerstate.c
        code/cgame/cg_polybus.c
        code/cgame/cg_predict.c
        code/cgame/cg_scoreboard.c
        code/cgame/cg_servercmds.c
        code/cgame/cg_snapshot.c
        code/cgame/cg_sound.c
        code/cgame/cg_trails.c
        code/cgame/cg_view.c
        code/cgame/cg_weapons.c
        code/ui/ui_shared.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/cgame/cg_syscalls.c #
        )

set(realrtcwgame_src
        code/game/g_main.c
        code/game/ai_cast.c
        code/game/ai_cast_characters.c
        code/game/ai_cast_debug.c
        code/game/ai_cast_events.c
        code/game/ai_cast_fight.c
        code/game/ai_cast_func_attack.c
        code/game/ai_cast_func_boss1.c
        code/game/ai_cast_funcs.c
        code/game/ai_cast_script_actions.c
        code/game/ai_cast_script.c
        code/game/ai_cast_script_ents.c
        code/game/ai_cast_sight.c
        code/game/ai_cast_think.c
        code/game/ai_chat.c
        code/game/ai_cmd.c
        code/game/ai_dmnet.c
        code/game/ai_dmq3.c
        code/game/ai_main.c
        code/game/ai_team.c
        code/game/bg_animation.c
        code/game/bg_misc.c
        code/game/bg_pmove.c
        code/game/bg_slidemove.c
        code/game/bg_lib.c
        code/game/g_active.c
        code/game/g_alarm.c
        code/game/g_bot.c
        code/game/g_client.c
        code/game/g_cmds.c
        code/game/g_combat.c
        code/game/g_items.c
        code/game/g_mem.c
        code/game/g_misc.c
        code/game/g_missile.c
        code/game/g_mover.c
        code/game/g_props.c
        code/game/g_save.c
        code/game/g_script_actions.c
        code/game/g_script.c
        code/game/g_session.c
        code/game/g_spawn.c
        code/game/g_svcmds.c
        code/game/g_target.c
        code/game/g_team.c
        code/game/g_tramcar.c
        code/game/g_trigger.c
        code/game/g_utils.c
        code/game/g_weapon.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/game/g_syscalls.c
        )

set(realrtcwui_src
        code/ui/ui_main.c
        code/ui/ui_atoms.c
        code/ui/ui_gameinfo.c
        code/ui/ui_players.c
        code/ui/ui_shared.c
        code/game/bg_misc.c
        code/game/bg_lib.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/ui/ui_syscalls.c
        )

if(USE_INTERNAL_JPEG)
set(realrtcwjpeglib_src
        code/jpeg-8c/jaricom.c
        code/jpeg-8c/jcapimin.c
        code/jpeg-8c/jcapistd.c
        code/jpeg-8c/jcarith.c
        code/jpeg-8c/jccoefct.c
        code/jpeg-8c/jccolor.c
        code/jpeg-8c/jcdctmgr.c
        code/jpeg-8c/jchuff.c
        code/jpeg-8c/jcinit.c
        code/jpeg-8c/jcmainct.c
        code/jpeg-8c/jcmarker.c
        code/jpeg-8c/jcmaster.c
        code/jpeg-8c/jcomapi.c
        code/jpeg-8c/jcparam.c
        code/jpeg-8c/jcprepct.c
        code/jpeg-8c/jcsample.c
        code/jpeg-8c/jctrans.c
        code/jpeg-8c/jdapimin.c
        code/jpeg-8c/jdapistd.c
        code/jpeg-8c/jdarith.c
        code/jpeg-8c/jdatadst.c
        code/jpeg-8c/jdatasrc.c
        code/jpeg-8c/jdcoefct.c
        code/jpeg-8c/jdcolor.c
        code/jpeg-8c/jddctmgr.c
        code/jpeg-8c/jdhuff.c
        code/jpeg-8c/jdinput.c
        code/jpeg-8c/jdmainct.c
        code/jpeg-8c/jdmarker.c
        code/jpeg-8c/jdmaster.c
        code/jpeg-8c/jdmerge.c
        code/jpeg-8c/jdpostct.c
        code/jpeg-8c/jdsample.c
        code/jpeg-8c/jdtrans.c
        code/jpeg-8c/jerror.c
        code/jpeg-8c/jfdctflt.c
        code/jpeg-8c/jfdctfst.c
        code/jpeg-8c/jfdctint.c
        code/jpeg-8c/jidctflt.c
        code/jpeg-8c/jidctfst.c
        code/jpeg-8c/jidctint.c
        code/jpeg-8c/jmemmgr.c
        code/jpeg-8c/jmemnobs.c
        code/jpeg-8c/jquant1.c
        code/jpeg-8c/jquant2.c
        code/jpeg-8c/jutils.c
        )
endif()

set(realrtcwbotlib_src
        code/src/botlib/be_aas_bspq3.c
        code/src/botlib/be_aas_cluster.c
        code/src/botlib/be_aas_debug.c
        code/src/botlib/be_aas_entity.c
        code/src/botlib/be_aas_file.c
        code/src/botlib/be_aas_main.c
        code/src/botlib/be_aas_move.c
        code/src/botlib/be_aas_optimize.c
        code/src/botlib/be_aas_reach.c
        code/src/botlib/be_aas_route.c
        code/src/botlib/be_aas_routealt.c
        code/src/botlib/be_aas_routetable.c
        code/src/botlib/be_aas_sample.c
        code/src/botlib/be_ai_char.c
        code/src/botlib/be_ai_chat.c
        code/src/botlib/be_ai_gen.c
        code/src/botlib/be_ai_goal.c
        code/src/botlib/be_ai_move.c
        code/src/botlib/be_ai_weap.c
        code/src/botlib/be_ai_weight.c
        code/src/botlib/be_ea.c
        code/src/botlib/be_interface.c
        code/src/botlib/l_crc.c
        code/src/botlib/l_libvar.c
        code/src/botlib/l_log.c
        code/src/botlib/l_memory.c
        code/src/botlib/l_precomp.c
        code/src/botlib/l_script.c
        code/src/botlib/l_struct.c
)

set(realrtcwrenderer_src
        code/renderer/tr_altivec.c
        code/renderer/tr_animation.c
        code/renderer/tr_backend.c
        code/renderer/tr_bsp.c
        code/renderer/tr_cmds.c
        code/renderer/tr_cmesh.c
        code/renderer/tr_curve.c
        code/renderer/tr_flares.c
        code/renderer/tr_font.c
        code/renderer/tr_image.c
        code/renderer/tr_image_bmp.c
        code/renderer/tr_image_jpg.c
        code/renderer/tr_image_pcx.c
        code/renderer/tr_image_png.c
        code/renderer/tr_image_tga.c
        code/renderer/tr_init.c
        code/renderer/tr_light.c
        code/renderer/tr_main.c
        code/renderer/tr_marks.c
        code/renderer/tr_mesh.c
        code/renderer/tr_model.c
        code/renderer/tr_model_iqm.c
        code/renderer/tr_noise.c
        code/renderer/tr_scene.c
        code/renderer/tr_shade.c
        code/renderer/tr_shade_calc.c
        code/renderer/tr_shader.c
        code/renderer/tr_shadows.c
        code/renderer/tr_sky.c
        code/renderer/tr_surface.c
        code/renderer/tr_world.c
        )
if(USE_BLOOM)
set(realrtcwrenderer_src ${realrtcwrenderer_src} code/renderer/tr_bloom.c)
endif()

set(realrtcwclient_src
        code/client/cl_cgame.c
        code/client/cl_cin.c
        code/client/cl_console.c
        code/client/cl_input.c
        code/client/cl_keys.c
        code/client/cl_main.c
        code/client/cl_net_chan.c
        code/client/cl_parse.c
        code/client/cl_scrn.c
        code/client/cl_ui.c
        code/client/cl_avi.c
        code/qcommon/cm_load.c
        code/qcommon/cm_patch.c
        code/qcommon/cm_polylib.c
        code/qcommon/cm_test.c
        code/qcommon/cm_trace.c
        code/qcommon/cmd.c
        code/qcommon/common.c
        code/qcommon/cvar.c
        code/qcommon/files.c
        code/qcommon/md4.c
        code/qcommon/md5.c
        code/qcommon/msg.c
        code/qcommon/net_chan.c
        code/qcommon/net_ip.c
        code/qcommon/huffman.c
        code/client/snd_altivec.c
        code/client/snd_adpcm.c
        code/client/snd_dma.c
        code/client/snd_mem.c
        code/client/snd_mix.c
        code/client/snd_wavelet.c
        code/client/snd_main.c
        code/client/snd_codec.c
        code/client/snd_codec_wav.c
        code/client/snd_codec_ogg.c
        code/client/snd_codec_opus.c
        code/client/qal.c
        code/client/snd_openal.c
        code/client/cl_curl.c
        code/server/sv_bot.c
        code/server/sv_ccmds.c
        code/server/sv_client.c
        code/server/sv_game.c
        code/server/sv_init.c
        code/server/sv_main.c
        code/server/sv_net_chan.c
        code/server/sv_snapshot.c
        code/server/sv_world.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/zlib-1.2.11/unzip.c
        code/zlib-1.2.11/ioapi.c
        code/qcommon/puff.c
        code/qcommon/vm.c
        code/qcommon/vm_interpreted.c
        code/botlib/be_aas_bspq3.c
        code/botlib/be_aas_cluster.c
        code/botlib/be_aas_debug.c
        code/botlib/be_aas_entity.c
        code/botlib/be_aas_file.c
        code/botlib/be_aas_main.c
        code/botlib/be_aas_move.c
        code/botlib/be_aas_optimize.c
        code/botlib/be_aas_reach.c
        code/botlib/be_aas_route.c
        code/botlib/be_aas_routealt.c
        code/botlib/be_aas_routetable.c
        code/botlib/be_aas_sample.c
        code/botlib/be_ai_char.c
        code/botlib/be_ai_chat.c
        code/botlib/be_ai_gen.c
        code/botlib/be_ai_goal.c
        code/botlib/be_ai_move.c
        code/botlib/be_ai_weap.c
        code/botlib/be_ai_weight.c
        code/botlib/be_ea.c
        code/botlib/be_interface.c
        code/botlib/l_crc.c
        code/botlib/l_libvar.c
        code/botlib/l_log.c
        code/botlib/l_memory.c
        code/botlib/l_precomp.c
        code/botlib/l_script.c
        code/botlib/l_struct.c
        code/splines/math_angles.cpp
        code/splines/math_matrix.cpp
        code/splines/math_quaternion.cpp
        code/splines/math_vector.cpp
        code/splines/q_parse.cpp
        code/splines/splines.cpp
        code/splines/util_str.cpp
        code/android/android_input.c
        code/android/android_glimp.c
        code/android/android_snd.c
        code/android/android_main.c
        code/android/ifaddrs.c
        code/sys/sys_unix.c
        code/sys/con_log.c
        code/sys/con_passive.c
        )

if(USE_CODEC_OPUS)
set(realrtcw_opus_dir "opus-1.2.1")
set(realrtcw_opusfile_dir "opusfile-0.9")
set(realrtcw_opus_src
        code/${realrtcw_opus_dir}/src/opus.c
        code/${realrtcw_opus_dir}/src/opus_decoder.c
        code/${realrtcw_opus_dir}/src/opus_encoder.c
        code/${realrtcw_opus_dir}/src/opus_multistream.c
        code/${realrtcw_opus_dir}/src/opus_multistream_encoder.c
        code/${realrtcw_opus_dir}/src/opus_multistream_decoder.c
        code/${realrtcw_opus_dir}/src/repacketizer.c
        code/${realrtcw_opus_dir}/src/analysis.c
        code/${realrtcw_opus_dir}/src/mlp.c
        code/${realrtcw_opus_dir}/src/mlp_data.c
        code/${realrtcw_opus_dir}/celt/bands.c
        code/${realrtcw_opus_dir}/celt/celt.c
        code/${realrtcw_opus_dir}/celt/celt_encoder.c
        code/${realrtcw_opus_dir}/celt/celt_decoder.c
        code/${realrtcw_opus_dir}/celt/cwrs.c
        code/${realrtcw_opus_dir}/celt/entcode.c
        code/${realrtcw_opus_dir}/celt/entdec.c
        code/${realrtcw_opus_dir}/celt/entenc.c
        code/${realrtcw_opus_dir}/celt/kiss_fft.c
        code/${realrtcw_opus_dir}/celt/laplace.c
        code/${realrtcw_opus_dir}/celt/mathops.c
        code/${realrtcw_opus_dir}/celt/mdct.c
        code/${realrtcw_opus_dir}/celt/modes.c
        code/${realrtcw_opus_dir}/celt/pitch.c
        code/${realrtcw_opus_dir}/celt/celt_lpc.c
        code/${realrtcw_opus_dir}/celt/quant_bands.c
        code/${realrtcw_opus_dir}/celt/rate.c
        code/${realrtcw_opus_dir}/celt/vq.c
        code/${realrtcw_opus_dir}/silk/CNG.c
        code/${realrtcw_opus_dir}/silk/code_signs.c
        code/${realrtcw_opus_dir}/silk/init_decoder.c
        code/${realrtcw_opus_dir}/silk/decode_core.c
        code/${realrtcw_opus_dir}/silk/decode_frame.c
        code/${realrtcw_opus_dir}/silk/decode_parameters.c
        code/${realrtcw_opus_dir}/silk/decode_indices.c
        code/${realrtcw_opus_dir}/silk/decode_pulses.c
        code/${realrtcw_opus_dir}/silk/decoder_set_fs.c
        code/${realrtcw_opus_dir}/silk/dec_API.c
        code/${realrtcw_opus_dir}/silk/enc_API.c
        code/${realrtcw_opus_dir}/silk/encode_indices.c
        code/${realrtcw_opus_dir}/silk/encode_pulses.c
        code/${realrtcw_opus_dir}/silk/gain_quant.c
        code/${realrtcw_opus_dir}/silk/interpolate.c
        code/${realrtcw_opus_dir}/silk/LP_variable_cutoff.c
        code/${realrtcw_opus_dir}/silk/NLSF_decode.c
        code/${realrtcw_opus_dir}/silk/NSQ.c
        code/${realrtcw_opus_dir}/silk/NSQ_del_dec.c
        code/${realrtcw_opus_dir}/silk/PLC.c
        code/${realrtcw_opus_dir}/silk/shell_coder.c
        code/${realrtcw_opus_dir}/silk/tables_gain.c
        code/${realrtcw_opus_dir}/silk/tables_LTP.c
        code/${realrtcw_opus_dir}/silk/tables_NLSF_CB_NB_MB.c
        code/${realrtcw_opus_dir}/silk/tables_NLSF_CB_WB.c
        code/${realrtcw_opus_dir}/silk/tables_other.c
        code/${realrtcw_opus_dir}/silk/tables_pitch_lag.c
        code/${realrtcw_opus_dir}/silk/tables_pulses_per_block.c
        code/${realrtcw_opus_dir}/silk/VAD.c
        code/${realrtcw_opus_dir}/silk/control_audio_bandwidth.c
        code/${realrtcw_opus_dir}/silk/quant_LTP_gains.c
        code/${realrtcw_opus_dir}/silk/VQ_WMat_EC.c
        code/${realrtcw_opus_dir}/silk/HP_variable_cutoff.c
        code/${realrtcw_opus_dir}/silk/NLSF_encode.c
        code/${realrtcw_opus_dir}/silk/NLSF_VQ.c
        code/${realrtcw_opus_dir}/silk/NLSF_unpack.c
        code/${realrtcw_opus_dir}/silk/NLSF_del_dec_quant.c
        code/${realrtcw_opus_dir}/silk/process_NLSFs.c
        code/${realrtcw_opus_dir}/silk/stereo_LR_to_MS.c
        code/${realrtcw_opus_dir}/silk/stereo_MS_to_LR.c
        code/${realrtcw_opus_dir}/silk/check_control_input.c
        code/${realrtcw_opus_dir}/silk/control_SNR.c
        code/${realrtcw_opus_dir}/silk/init_encoder.c
        code/${realrtcw_opus_dir}/silk/control_codec.c
        code/${realrtcw_opus_dir}/silk/A2NLSF.c
        code/${realrtcw_opus_dir}/silk/ana_filt_bank_1.c
        code/${realrtcw_opus_dir}/silk/biquad_alt.c
        code/${realrtcw_opus_dir}/silk/bwexpander_32.c
        code/${realrtcw_opus_dir}/silk/bwexpander.c
        code/${realrtcw_opus_dir}/silk/debug.c
        code/${realrtcw_opus_dir}/silk/decode_pitch.c
        code/${realrtcw_opus_dir}/silk/inner_prod_aligned.c
        code/${realrtcw_opus_dir}/silk/lin2log.c
        code/${realrtcw_opus_dir}/silk/log2lin.c
        code/${realrtcw_opus_dir}/silk/LPC_analysis_filter.c
        code/${realrtcw_opus_dir}/silk/LPC_inv_pred_gain.c
        code/${realrtcw_opus_dir}/silk/table_LSF_cos.c
        code/${realrtcw_opus_dir}/silk/NLSF2A.c
        code/${realrtcw_opus_dir}/silk/NLSF_stabilize.c
        code/${realrtcw_opus_dir}/silk/NLSF_VQ_weights_laroia.c
        code/${realrtcw_opus_dir}/silk/pitch_est_tables.c
        code/${realrtcw_opus_dir}/silk/resampler.c
        code/${realrtcw_opus_dir}/silk/resampler_down2_3.c
        code/${realrtcw_opus_dir}/silk/resampler_down2.c
        code/${realrtcw_opus_dir}/silk/resampler_private_AR2.c
        code/${realrtcw_opus_dir}/silk/resampler_private_down_FIR.c
        code/${realrtcw_opus_dir}/silk/resampler_private_IIR_FIR.c
        code/${realrtcw_opus_dir}/silk/resampler_private_up2_HQ.c
        code/${realrtcw_opus_dir}/silk/resampler_rom.c
        code/${realrtcw_opus_dir}/silk/sigm_Q15.c
        code/${realrtcw_opus_dir}/silk/sort.c
        code/${realrtcw_opus_dir}/silk/sum_sqr_shift.c
        code/${realrtcw_opus_dir}/silk/stereo_decode_pred.c
        code/${realrtcw_opus_dir}/silk/stereo_encode_pred.c
        code/${realrtcw_opus_dir}/silk/stereo_find_predictor.c
        code/${realrtcw_opus_dir}/silk/stereo_quant_pred.c
        code/${realrtcw_opus_dir}/silk/LPC_fit.c
        code/${realrtcw_opus_dir}/silk/float/apply_sine_window_FLP.c
        code/${realrtcw_opus_dir}/silk/float/corrMatrix_FLP.c
        code/${realrtcw_opus_dir}/silk/float/encode_frame_FLP.c
        code/${realrtcw_opus_dir}/silk/float/find_LPC_FLP.c
        code/${realrtcw_opus_dir}/silk/float/find_LTP_FLP.c
        code/${realrtcw_opus_dir}/silk/float/find_pitch_lags_FLP.c
        code/${realrtcw_opus_dir}/silk/float/find_pred_coefs_FLP.c
        code/${realrtcw_opus_dir}/silk/float/LPC_analysis_filter_FLP.c
        code/${realrtcw_opus_dir}/silk/float/LTP_analysis_filter_FLP.c
        code/${realrtcw_opus_dir}/silk/float/LTP_scale_ctrl_FLP.c
        code/${realrtcw_opus_dir}/silk/float/noise_shape_analysis_FLP.c
        code/${realrtcw_opus_dir}/silk/float/process_gains_FLP.c
        code/${realrtcw_opus_dir}/silk/float/regularize_correlations_FLP.c
        code/${realrtcw_opus_dir}/silk/float/residual_energy_FLP.c
        code/${realrtcw_opus_dir}/silk/float/warped_autocorrelation_FLP.c
        code/${realrtcw_opus_dir}/silk/float/wrappers_FLP.c
        code/${realrtcw_opus_dir}/silk/float/autocorrelation_FLP.c
        code/${realrtcw_opus_dir}/silk/float/burg_modified_FLP.c
        code/${realrtcw_opus_dir}/silk/float/bwexpander_FLP.c
        code/${realrtcw_opus_dir}/silk/float/energy_FLP.c
        code/${realrtcw_opus_dir}/silk/float/inner_product_FLP.c
        code/${realrtcw_opus_dir}/silk/float/k2a_FLP.c
        code/${realrtcw_opus_dir}/silk/float/LPC_inv_pred_gain_FLP.c
        code/${realrtcw_opus_dir}/silk/float/pitch_analysis_core_FLP.c
        code/${realrtcw_opus_dir}/silk/float/scale_copy_vector_FLP.c
        code/${realrtcw_opus_dir}/silk/float/scale_vector_FLP.c
        code/${realrtcw_opus_dir}/silk/float/schur_FLP.c
        code/${realrtcw_opus_dir}/silk/float/sort_FLP.c
        code/${realrtcw_opusfile_dir}/src/info.c
        code/${realrtcw_opusfile_dir}/src/internal.c
        code/${realrtcw_opusfile_dir}/src/opusfile.c
        code/${realrtcw_opusfile_dir}/src/stream.c
        code/${realrtcw_opusfile_dir}/src/http.c
        )
endif()

if(USE_FREETYPE)
set(realrtcw_freetype_dir "freetype-2.9")
if(USE_INTERNAL_FREETYPE)
set(realrtcw_freetype_src
        code/${realrtcw_freetype_dir}/src/base/ftsystem.c
        code/${realrtcw_freetype_dir}/src/base/ftdebug.c
        code/${realrtcw_freetype_dir}/src/base/ftinit.c
        code/${realrtcw_freetype_dir}/src/base/ftbase.c
        code/${realrtcw_freetype_dir}/src/base/ftbbox.c
        code/${realrtcw_freetype_dir}/src/base/ftbdf.c
        code/${realrtcw_freetype_dir}/src/base/ftbitmap.c
        code/${realrtcw_freetype_dir}/src/base/ftcid.c
        code/${realrtcw_freetype_dir}/src/base/ftfntfmt.c
        code/${realrtcw_freetype_dir}/src/base/ftfstype.c
        code/${realrtcw_freetype_dir}/src/base/ftgasp.c
        code/${realrtcw_freetype_dir}/src/base/ftglyph.c
        code/${realrtcw_freetype_dir}/src/base/ftgxval.c
        code/${realrtcw_freetype_dir}/src/base/ftlcdfil.c
        code/${realrtcw_freetype_dir}/src/base/ftmm.c
        code/${realrtcw_freetype_dir}/src/base/ftotval.c
        code/${realrtcw_freetype_dir}/src/base/ftpatent.c
        code/${realrtcw_freetype_dir}/src/base/ftpfr.c
        code/${realrtcw_freetype_dir}/src/base/ftstroke.c
        code/${realrtcw_freetype_dir}/src/base/ftsynth.c
        code/${realrtcw_freetype_dir}/src/base/fttype1.c
        code/${realrtcw_freetype_dir}/src/base/ftwinfnt.c
        code/${realrtcw_freetype_dir}/src/truetype/truetype.c
        code/${realrtcw_freetype_dir}/src/type1/type1.c
        code/${realrtcw_freetype_dir}/src/cff/cff.c
        code/${realrtcw_freetype_dir}/src/cid/type1cid.c
        code/${realrtcw_freetype_dir}/src/pfr/pfr.c
        code/${realrtcw_freetype_dir}/src/type42/type42.c
        code/${realrtcw_freetype_dir}/src/winfonts/winfnt.c
        code/${realrtcw_freetype_dir}/src/pcf/pcf.c
        code/${realrtcw_freetype_dir}/src/bdf/bdf.c
        code/${realrtcw_freetype_dir}/src/sfnt/sfnt.c
        code/${realrtcw_freetype_dir}/src/autofit/autofit.c
        code/${realrtcw_freetype_dir}/src/pshinter/pshinter.c
        code/${realrtcw_freetype_dir}/src/raster/raster.c
        code/${realrtcw_freetype_dir}/src/smooth/smooth.c
        code/${realrtcw_freetype_dir}/src/cache/ftcache.c
        code/${realrtcw_freetype_dir}/src/gzip/ftgzip.c
        code/${realrtcw_freetype_dir}/src/lzw/ftlzw.c
        code/${realrtcw_freetype_dir}/src/bzip2/ftbzip2.c
        code/${realrtcw_freetype_dir}/src/psaux/psaux.c
        code/${realrtcw_freetype_dir}/src/psnames/psnames.c
        )
endif()
endif()

set(realrtcw_ogg_dir "libogg-1.3.3")
if(USE_INTERNAL_OGG)
    set(realrtcw_ogg_src
            code/${realrtcw_ogg_dir}/src/bitwise.c
            code/${realrtcw_ogg_dir}/src/framing.c
            )
endif()

set(realrtcw_vorbis_dir "libvorbis-1.3.6")
if(USE_INTERNAL_VORBIS)
    set(realrtcw_vorbis_src
            code/${realrtcw_vorbis_dir}/lib/analysis.c
            code/${realrtcw_vorbis_dir}/lib/bitrate.c
            code/${realrtcw_vorbis_dir}/lib/block.c
            code/${realrtcw_vorbis_dir}/lib/codebook.c
            code/${realrtcw_vorbis_dir}/lib/envelope.c
            code/${realrtcw_vorbis_dir}/lib/floor0.c
            code/${realrtcw_vorbis_dir}/lib/floor1.c
            code/${realrtcw_vorbis_dir}/lib/info.c
            code/${realrtcw_vorbis_dir}/lib/lookup.c
            code/${realrtcw_vorbis_dir}/lib/lpc.c
            code/${realrtcw_vorbis_dir}/lib/lsp.c
            code/${realrtcw_vorbis_dir}/lib/mapping0.c
            code/${realrtcw_vorbis_dir}/lib/mdct.c
            code/${realrtcw_vorbis_dir}/lib/psy.c
            code/${realrtcw_vorbis_dir}/lib/registry.c
            code/${realrtcw_vorbis_dir}/lib/res0.c
            code/${realrtcw_vorbis_dir}/lib/sharedbook.c
            code/${realrtcw_vorbis_dir}/lib/smallft.c
            code/${realrtcw_vorbis_dir}/lib/synthesis.c
            code/${realrtcw_vorbis_dir}/lib/vorbisenc.c
            code/${realrtcw_vorbis_dir}/lib/vorbisfile.c
            code/${realrtcw_vorbis_dir}/lib/window.c
            )
endif()

set(realrtcw_zlib_dir "zlib-1.2.11")
if(USE_INTERNAL_ZLIB)
    set(realrtcw_zlib_src
            code/${realrtcw_zlib_dir}/adler32.c
            code/${realrtcw_zlib_dir}/crc32.c
            code/${realrtcw_zlib_dir}/inffast.c
            code/${realrtcw_zlib_dir}/inflate.c
            code/${realrtcw_zlib_dir}/inftrees.c
            code/${realrtcw_zlib_dir}/zutil.c
            )
endif()

set(REALRTCW_DLL_PREFIX realrtcw${REALRTCW_SUFFIX})
add_definitions(-D_HARM_REALRTCW_DLL_VER=\"${REALRTCW_SUFFIX}\")

set(REALRTCWLIBRARY RealRTCW${REALRTCW_SUFFIX})
set(REALRTCWLIBRARY_UI ${REALRTCW_DLL_PREFIX}ui)
set(REALRTCWLIBRARY_CGAME ${REALRTCW_DLL_PREFIX}cgame)
set(REALRTCWLIBRARY_QAGAME ${REALRTCW_DLL_PREFIX}qagame)
set(REALRTCWLIBRARY_VORBIS ${REALRTCW_DLL_PREFIX}vorbis)

# struct define conflict with opus
add_library(${REALRTCWLIBRARY_VORBIS} STATIC
        ${realrtcw_vorbis_src}
)
set_target_properties(${REALRTCWLIBRARY_VORBIS} PROPERTIES COMPILE_FLAGS "-fPIC")
target_include_directories(${REALRTCWLIBRARY_VORBIS} PRIVATE "code/${realrtcw_vorbis_dir}/include" "code/${realrtcw_vorbis_dir}/lib")
target_include_directories(${REALRTCWLIBRARY_VORBIS} PRIVATE "code/${realrtcw_ogg_dir}/include")

add_library(${REALRTCWLIBRARY} SHARED
        ${realrtcwrenderer_src}
        ${realrtcwclient_src}
        ${realrtcwjpeglib_src}
        ${realrtcw_opus_src}
        ${realrtcw_freetype_src}
        ${realrtcw_ogg_src}
        ${realrtcw_zlib_src}
        )
set_target_properties(${REALRTCWLIBRARY} PROPERTIES COMPILE_DEFINITIONS "BOTLIB")
target_link_libraries(${REALRTCWLIBRARY}
        ${realrtcwclient_libs}
        ${REALRTCWLIBRARY_VORBIS}
        )
target_include_directories(${REALRTCWLIBRARY} PRIVATE "code/curl-7.60.0/include")
target_include_directories(${REALRTCWLIBRARY} PRIVATE "code/${realrtcw_ogg_dir}/include")
target_include_directories(${REALRTCWLIBRARY} PRIVATE "code/${realrtcw_vorbis_dir}/include")
target_include_directories(${REALRTCWLIBRARY} PRIVATE "code/jpeg-8c")
if(USE_CODEC_OPUS)
    target_include_directories(${REALRTCWLIBRARY} PRIVATE "code/${realrtcw_opus_dir}/include" "code/${realrtcw_opus_dir}/celt" "code/${realrtcw_opus_dir}/silk" "code/${realrtcw_opus_dir}/silk/float" "code/${realrtcw_opusfile_dir}/include")
endif()
if(USE_FREETYPE)
    target_include_directories(${REALRTCWLIBRARY} PRIVATE "code/${realrtcw_freetype_dir}/include")
endif()
target_include_directories(${REALRTCWLIBRARY} PRIVATE "code/jpeg-8c")
target_include_directories(${REALRTCWLIBRARY} PRIVATE "code")
target_include_directories(${REALRTCWLIBRARY} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../common")
if(USE_INTERNAL_ZLIB)
    target_include_directories(${REALRTCWLIBRARY} PRIVATE "code/${realrtcw_zlib_dir}")
endif()

add_library(${REALRTCWLIBRARY_UI} SHARED
        ${realrtcwui_src}
        )
set_target_properties(${REALRTCWLIBRARY_UI} PROPERTIES COMPILE_DEFINITIONS "UI")
set_target_properties(${REALRTCWLIBRARY_UI} PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(${REALRTCWLIBRARY_CGAME} SHARED
        ${realrtcwcgame_src}
        )
set_target_properties(${REALRTCWLIBRARY_CGAME} PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL;CGAME")
set_target_properties(${REALRTCWLIBRARY_CGAME} PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(${REALRTCWLIBRARY_QAGAME} SHARED
        ${realrtcwgame_src}
        )
set_target_properties(${REALRTCWLIBRARY_QAGAME} PROPERTIES COMPILE_DEFINITIONS "GAMEDLL;QAGAME")
set_target_properties(${REALRTCWLIBRARY_QAGAME} PROPERTIES COMPILE_FLAGS "-fPIC")