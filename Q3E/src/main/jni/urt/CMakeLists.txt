cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(urbanterror)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 98)

set(IOQ3_VERSION "4.3.4")

set(URBANTERROR_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")

option(USE_CODEC_OPUS "" ON)
option(USE_FREETYPE "" ON)
option(USE_INTERNAL_JPEG "" ON)
option(USE_INTERNAL_VORBIS "" ON)
option(USE_INTERNAL_OGG "" ON)
option(USE_INTERNAL_ZLIB "" OFF)
option(USE_VOIP "" OFF)
option(USE_MUMBLE "" OFF)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_definitions(-DPRODUCT_VERSION=\"${IOQ3_VERSION}\")
add_definitions(-D_DIII4A) # android
add_definitions(-DNO_VM_COMPILED -DUSE_LOCAL_HEADERS) # base
add_definitions(-DUSE_OPENAL -DUSE_OPENAL_DLOPEN) # OpenAL dlopen
add_definitions(-DUSE_OPENGLES) # OpenGLES 1.1
add_definitions(-DUSE_CODEC_VORBIS) # ogg
add_definitions(-DUSE_CURL -DUSE_CURL_DLOPEN) # curl dlopen
add_definitions(-DUSE_AUTH -DUSE_ALTGAMMA=1 -DUSE_DEMO_FORMAT_42=1 -DUSE_ICON)

if(USE_CODEC_OPUS)
    add_definitions(-DUSE_CODEC_OPUS -DOPUS_BUILD -DHAVE_LRINTF -DFLOATING_POINT -DFLOAT_APPROX -DUSE_ALLOCA) # opus
endif()
if(USE_VOIP)
    add_definitions(-DUSE_VOIP)
endif()
add_definitions(-DSTANDALONE) # Standalone game

set(urbanterrorclient_libs android log GLESv1_CM EGL)
set(urbanterrorclient_libs ${urbanterrorclient_libs} oboeaudio)
if(USE_FREETYPE)
    set(urbanterrorclient_libs ${urbanterrorclient_libs} freetype)
endif()
if(NOT USE_INTERNAL_OGG)
    set(urbanterrorclient_libs ${urbanterrorclient_libs} ogg)
endif()
if(NOT USE_INTERNAL_VORBIS)
    set(urbanterrorclient_libs ${urbanterrorclient_libs} vorbisfile vorbis)
endif()
if(NOT USE_INTERNAL_JPEG)
    set(urbanterrorclient_libs ${urbanterrorclient_libs} jpeg)
else()
    add_definitions(-DUSE_INTERNAL_JPEG) # jpeg
endif()
if(NOT USE_INTERNAL_ZLIB)
    set(urbanterrorclient_libs ${urbanterrorclient_libs} z)
else()
    add_definitions(-DNO_GZIP) # zlib
endif()
#set(urbanterrorclient_libs ${urbanterrorclient_libs} curl)
# -fshort-enums

set(UT_CFLAGS "-pipe -fsigned-char -fPIC -DNDEBUG -O3 -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization") # -Werror-implicit-function-declaration -Qunused-arguments

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${UT_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${UT_CFLAGS}")

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

if(USE_INTERNAL_JPEG)
    set(urbanterrorjpeglib_src
            external/jpeg-8c/jaricom.c
            external/jpeg-8c/jcapimin.c
            external/jpeg-8c/jcapistd.c
            external/jpeg-8c/jcarith.c
            external/jpeg-8c/jccoefct.c
            external/jpeg-8c/jccolor.c
            external/jpeg-8c/jcdctmgr.c
            external/jpeg-8c/jchuff.c
            external/jpeg-8c/jcinit.c
            external/jpeg-8c/jcmainct.c
            external/jpeg-8c/jcmarker.c
            external/jpeg-8c/jcmaster.c
            external/jpeg-8c/jcomapi.c
            external/jpeg-8c/jcparam.c
            external/jpeg-8c/jcprepct.c
            external/jpeg-8c/jcsample.c
            external/jpeg-8c/jctrans.c
            external/jpeg-8c/jdapimin.c
            external/jpeg-8c/jdapistd.c
            external/jpeg-8c/jdarith.c
            external/jpeg-8c/jdatadst.c
            external/jpeg-8c/jdatasrc.c
            external/jpeg-8c/jdcoefct.c
            external/jpeg-8c/jdcolor.c
            external/jpeg-8c/jddctmgr.c
            external/jpeg-8c/jdhuff.c
            external/jpeg-8c/jdinput.c
            external/jpeg-8c/jdmainct.c
            external/jpeg-8c/jdmarker.c
            external/jpeg-8c/jdmaster.c
            external/jpeg-8c/jdmerge.c
            external/jpeg-8c/jdpostct.c
            external/jpeg-8c/jdsample.c
            external/jpeg-8c/jdtrans.c
            external/jpeg-8c/jerror.c
            external/jpeg-8c/jfdctflt.c
            external/jpeg-8c/jfdctfst.c
            external/jpeg-8c/jfdctint.c
            external/jpeg-8c/jidctflt.c
            external/jpeg-8c/jidctfst.c
            external/jpeg-8c/jidctint.c
            external/jpeg-8c/jmemmgr.c
            external/jpeg-8c/jmemnobs.c
            external/jpeg-8c/jquant1.c
            external/jpeg-8c/jquant2.c
            external/jpeg-8c/jutils.c
            )
endif()

set(urbanterrorrenderer_src
        code/renderergl1/tr_animation.c
        code/renderergl1/tr_backend.c
        code/renderergl1/tr_bsp.c
        code/renderergl1/tr_cmds.c
        code/renderergl1/tr_curve.c
        code/renderergl1/tr_flares.c
        code/renderercommon/tr_font.c
        code/renderergl1/tr_image.c
        code/renderercommon/tr_image_bmp.c
        code/renderercommon/tr_image_jpg.c
        code/renderercommon/tr_image_pcx.c
        code/renderercommon/tr_image_png.c
        code/renderercommon/tr_image_tga.c
        code/renderergl1/tr_init.c
        code/renderergl1/tr_light.c
        code/renderergl1/tr_main.c
        code/renderergl1/tr_marks.c
        code/renderergl1/tr_mesh.c
        code/renderergl1/tr_model.c
        code/renderergl1/tr_model_iqm.c
        code/renderercommon/tr_noise.c
        code/renderergl1/tr_scene.c
        code/renderergl1/tr_shade.c
        code/renderergl1/tr_shade_calc.c
        code/renderergl1/tr_shader.c
        code/renderergl1/tr_shadows.c
        code/renderergl1/tr_sky.c
        code/renderergl1/tr_surface.c
        code/renderergl1/tr_world.c
        code/android/android_glimp.c
        code/qcommon/q_shared.c
        code/qcommon/puff.c
        code/qcommon/q_math.c
        code/renderergl1/tr_subs.c
        )

set(urbanterrorclient_src
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
        code/client/snd_adpcm.c
        code/client/snd_dma.c
        code/client/snd_dmahd.c
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
        code/qcommon/unzip.c
        code/qcommon/ioapi.c
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
        code/android/android_input.c
        code/sys/con_log.c
        code/sys/sys_autoupdater.c
        code/android/android_main.c
        code/android/android_snd.c
        code/android/ifaddrs.c
        code/sys/con_passive.c
        code/sys/sys_unix.c
        )

if(USE_CODEC_OPUS)
    set(urbanterror_opus_dir "opus-1.1.4")
    set(urbanterror_opusfile_dir "opusfile-0.8")
    set(urbanterror_opus_src
            external/${urbanterror_opus_dir}/src/opus.c
            external/${urbanterror_opus_dir}/src/opus_decoder.c
            external/${urbanterror_opus_dir}/src/opus_encoder.c
            external/${urbanterror_opus_dir}/src/opus_multistream.c
            external/${urbanterror_opus_dir}/src/opus_multistream_encoder.c
            external/${urbanterror_opus_dir}/src/opus_multistream_decoder.c
            external/${urbanterror_opus_dir}/src/repacketizer.c
            external/${urbanterror_opus_dir}/src/analysis.c
            external/${urbanterror_opus_dir}/src/mlp.c
            external/${urbanterror_opus_dir}/src/mlp_data.c
            external/${urbanterror_opus_dir}/celt/bands.c
            external/${urbanterror_opus_dir}/celt/celt.c
            external/${urbanterror_opus_dir}/celt/celt_encoder.c
            external/${urbanterror_opus_dir}/celt/celt_decoder.c
            external/${urbanterror_opus_dir}/celt/cwrs.c
            external/${urbanterror_opus_dir}/celt/entcode.c
            external/${urbanterror_opus_dir}/celt/entdec.c
            external/${urbanterror_opus_dir}/celt/entenc.c
            external/${urbanterror_opus_dir}/celt/kiss_fft.c
            external/${urbanterror_opus_dir}/celt/laplace.c
            external/${urbanterror_opus_dir}/celt/mathops.c
            external/${urbanterror_opus_dir}/celt/mdct.c
            external/${urbanterror_opus_dir}/celt/modes.c
            external/${urbanterror_opus_dir}/celt/pitch.c
            external/${urbanterror_opus_dir}/celt/celt_lpc.c
            external/${urbanterror_opus_dir}/celt/quant_bands.c
            external/${urbanterror_opus_dir}/celt/rate.c
            external/${urbanterror_opus_dir}/celt/vq.c
            external/${urbanterror_opus_dir}/silk/CNG.c
            external/${urbanterror_opus_dir}/silk/code_signs.c
            external/${urbanterror_opus_dir}/silk/init_decoder.c
            external/${urbanterror_opus_dir}/silk/decode_core.c
            external/${urbanterror_opus_dir}/silk/decode_frame.c
            external/${urbanterror_opus_dir}/silk/decode_parameters.c
            external/${urbanterror_opus_dir}/silk/decode_indices.c
            external/${urbanterror_opus_dir}/silk/decode_pulses.c
            external/${urbanterror_opus_dir}/silk/decoder_set_fs.c
            external/${urbanterror_opus_dir}/silk/dec_API.c
            external/${urbanterror_opus_dir}/silk/enc_API.c
            external/${urbanterror_opus_dir}/silk/encode_indices.c
            external/${urbanterror_opus_dir}/silk/encode_pulses.c
            external/${urbanterror_opus_dir}/silk/gain_quant.c
            external/${urbanterror_opus_dir}/silk/interpolate.c
            external/${urbanterror_opus_dir}/silk/LP_variable_cutoff.c
            external/${urbanterror_opus_dir}/silk/NLSF_decode.c
            external/${urbanterror_opus_dir}/silk/NSQ.c
            external/${urbanterror_opus_dir}/silk/NSQ_del_dec.c
            external/${urbanterror_opus_dir}/silk/PLC.c
            external/${urbanterror_opus_dir}/silk/shell_coder.c
            external/${urbanterror_opus_dir}/silk/tables_gain.c
            external/${urbanterror_opus_dir}/silk/tables_LTP.c
            external/${urbanterror_opus_dir}/silk/tables_NLSF_CB_NB_MB.c
            external/${urbanterror_opus_dir}/silk/tables_NLSF_CB_WB.c
            external/${urbanterror_opus_dir}/silk/tables_other.c
            external/${urbanterror_opus_dir}/silk/tables_pitch_lag.c
            external/${urbanterror_opus_dir}/silk/tables_pulses_per_block.c
            external/${urbanterror_opus_dir}/silk/VAD.c
            external/${urbanterror_opus_dir}/silk/control_audio_bandwidth.c
            external/${urbanterror_opus_dir}/silk/quant_LTP_gains.c
            external/${urbanterror_opus_dir}/silk/VQ_WMat_EC.c
            external/${urbanterror_opus_dir}/silk/HP_variable_cutoff.c
            external/${urbanterror_opus_dir}/silk/NLSF_encode.c
            external/${urbanterror_opus_dir}/silk/NLSF_VQ.c
            external/${urbanterror_opus_dir}/silk/NLSF_unpack.c
            external/${urbanterror_opus_dir}/silk/NLSF_del_dec_quant.c
            external/${urbanterror_opus_dir}/silk/process_NLSFs.c
            external/${urbanterror_opus_dir}/silk/stereo_LR_to_MS.c
            external/${urbanterror_opus_dir}/silk/stereo_MS_to_LR.c
            external/${urbanterror_opus_dir}/silk/check_control_input.c
            external/${urbanterror_opus_dir}/silk/control_SNR.c
            external/${urbanterror_opus_dir}/silk/init_encoder.c
            external/${urbanterror_opus_dir}/silk/control_codec.c
            external/${urbanterror_opus_dir}/silk/A2NLSF.c
            external/${urbanterror_opus_dir}/silk/ana_filt_bank_1.c
            external/${urbanterror_opus_dir}/silk/biquad_alt.c
            external/${urbanterror_opus_dir}/silk/bwexpander_32.c
            external/${urbanterror_opus_dir}/silk/bwexpander.c
            external/${urbanterror_opus_dir}/silk/debug.c
            external/${urbanterror_opus_dir}/silk/decode_pitch.c
            external/${urbanterror_opus_dir}/silk/inner_prod_aligned.c
            external/${urbanterror_opus_dir}/silk/lin2log.c
            external/${urbanterror_opus_dir}/silk/log2lin.c
            external/${urbanterror_opus_dir}/silk/LPC_analysis_filter.c
            external/${urbanterror_opus_dir}/silk/LPC_inv_pred_gain.c
            external/${urbanterror_opus_dir}/silk/table_LSF_cos.c
            external/${urbanterror_opus_dir}/silk/NLSF2A.c
            external/${urbanterror_opus_dir}/silk/NLSF_stabilize.c
            external/${urbanterror_opus_dir}/silk/NLSF_VQ_weights_laroia.c
            external/${urbanterror_opus_dir}/silk/pitch_est_tables.c
            external/${urbanterror_opus_dir}/silk/resampler.c
            external/${urbanterror_opus_dir}/silk/resampler_down2_3.c
            external/${urbanterror_opus_dir}/silk/resampler_down2.c
            external/${urbanterror_opus_dir}/silk/resampler_private_AR2.c
            external/${urbanterror_opus_dir}/silk/resampler_private_down_FIR.c
            external/${urbanterror_opus_dir}/silk/resampler_private_IIR_FIR.c
            external/${urbanterror_opus_dir}/silk/resampler_private_up2_HQ.c
            external/${urbanterror_opus_dir}/silk/resampler_rom.c
            external/${urbanterror_opus_dir}/silk/sigm_Q15.c
            external/${urbanterror_opus_dir}/silk/sort.c
            external/${urbanterror_opus_dir}/silk/sum_sqr_shift.c
            external/${urbanterror_opus_dir}/silk/stereo_decode_pred.c
            external/${urbanterror_opus_dir}/silk/stereo_encode_pred.c
            external/${urbanterror_opus_dir}/silk/stereo_find_predictor.c
            external/${urbanterror_opus_dir}/silk/stereo_quant_pred.c
            external/${urbanterror_opus_dir}/silk/float/apply_sine_window_FLP.c
            external/${urbanterror_opus_dir}/silk/float/corrMatrix_FLP.c
            external/${urbanterror_opus_dir}/silk/float/encode_frame_FLP.c
            external/${urbanterror_opus_dir}/silk/float/find_LPC_FLP.c
            external/${urbanterror_opus_dir}/silk/float/find_LTP_FLP.c
            external/${urbanterror_opus_dir}/silk/float/find_pitch_lags_FLP.c
            external/${urbanterror_opus_dir}/silk/float/find_pred_coefs_FLP.c
            external/${urbanterror_opus_dir}/silk/float/LPC_analysis_filter_FLP.c
            external/${urbanterror_opus_dir}/silk/float/LTP_analysis_filter_FLP.c
            external/${urbanterror_opus_dir}/silk/float/LTP_scale_ctrl_FLP.c
            external/${urbanterror_opus_dir}/silk/float/noise_shape_analysis_FLP.c
            external/${urbanterror_opus_dir}/silk/float/process_gains_FLP.c
            external/${urbanterror_opus_dir}/silk/float/regularize_correlations_FLP.c
            external/${urbanterror_opus_dir}/silk/float/residual_energy_FLP.c
            external/${urbanterror_opus_dir}/silk/float/warped_autocorrelation_FLP.c
            external/${urbanterror_opus_dir}/silk/float/wrappers_FLP.c
            external/${urbanterror_opus_dir}/silk/float/autocorrelation_FLP.c
            external/${urbanterror_opus_dir}/silk/float/burg_modified_FLP.c
            external/${urbanterror_opus_dir}/silk/float/bwexpander_FLP.c
            external/${urbanterror_opus_dir}/silk/float/energy_FLP.c
            external/${urbanterror_opus_dir}/silk/float/inner_product_FLP.c
            external/${urbanterror_opus_dir}/silk/float/k2a_FLP.c
            external/${urbanterror_opus_dir}/silk/float/LPC_inv_pred_gain_FLP.c
            external/${urbanterror_opus_dir}/silk/float/levinsondurbin_FLP.c
            external/${urbanterror_opus_dir}/silk/float/solve_LS_FLP.c
            external/${urbanterror_opus_dir}/silk/float/pitch_analysis_core_FLP.c
            external/${urbanterror_opus_dir}/silk/float/scale_copy_vector_FLP.c
            external/${urbanterror_opus_dir}/silk/float/scale_vector_FLP.c
            external/${urbanterror_opus_dir}/silk/float/schur_FLP.c
            external/${urbanterror_opus_dir}/silk/float/sort_FLP.c
            external/${urbanterror_opus_dir}/silk/float/prefilter_FLP.c
            external/${urbanterror_opusfile_dir}/src/info.c
            external/${urbanterror_opusfile_dir}/src/internal.c
            external/${urbanterror_opusfile_dir}/src/opusfile.c
            external/${urbanterror_opusfile_dir}/src/stream.c
            external/${urbanterror_opusfile_dir}/src/http.c
            )
endif()

set(urbanterror_ogg_dir "libogg-1.3.2")
if(USE_INTERNAL_OGG)
    set(urbanterror_ogg_src
            external/${urbanterror_ogg_dir}/src/bitwise.c
            external/${urbanterror_ogg_dir}/src/framing.c
            )
endif()

set(urbanterror_vorbis_dir "libvorbis-1.3.5")
if(USE_INTERNAL_VORBIS)
    set(urbanterror_vorbis_src
            external/${urbanterror_vorbis_dir}/lib/analysis.c
            external/${urbanterror_vorbis_dir}/lib/bitrate.c
            external/${urbanterror_vorbis_dir}/lib/block.c
            external/${urbanterror_vorbis_dir}/lib/codebook.c
            external/${urbanterror_vorbis_dir}/lib/envelope.c
            external/${urbanterror_vorbis_dir}/lib/floor0.c
            external/${urbanterror_vorbis_dir}/lib/floor1.c
            external/${urbanterror_vorbis_dir}/lib/info.c
            external/${urbanterror_vorbis_dir}/lib/lookup.c
            external/${urbanterror_vorbis_dir}/lib/lpc.c
            external/${urbanterror_vorbis_dir}/lib/lsp.c
            external/${urbanterror_vorbis_dir}/lib/mapping0.c
            external/${urbanterror_vorbis_dir}/lib/mdct.c
            external/${urbanterror_vorbis_dir}/lib/psy.c
            external/${urbanterror_vorbis_dir}/lib/registry.c
            external/${urbanterror_vorbis_dir}/lib/res0.c
            external/${urbanterror_vorbis_dir}/lib/sharedbook.c
            external/${urbanterror_vorbis_dir}/lib/smallft.c
            external/${urbanterror_vorbis_dir}/lib/synthesis.c
            external/${urbanterror_vorbis_dir}/lib/vorbisenc.c
            external/${urbanterror_vorbis_dir}/lib/vorbisfile.c
            external/${urbanterror_vorbis_dir}/lib/window.c
            )
endif()

set(urbanterror_zlib_dir "zlib")
if(USE_INTERNAL_ZLIB)
    set(urbanterror_zlib_src
            external/${urbanterror_zlib_dir}/adler32.c
            external/${urbanterror_zlib_dir}/crc32.c
            external/${urbanterror_zlib_dir}/inffast.c
            external/${urbanterror_zlib_dir}/inflate.c
            external/${urbanterror_zlib_dir}/inftrees.c
            external/${urbanterror_zlib_dir}/zutil.c
            )
endif()

# struct define conflict with opus
add_library(urbanterror_vorbis STATIC
        ${urbanterror_vorbis_src}
        )
set_target_properties(urbanterror_vorbis PROPERTIES COMPILE_FLAGS "-fPIC")
target_include_directories(urbanterror_vorbis PRIVATE "external/${urbanterror_vorbis_dir}/include" "external/${urbanterror_vorbis_dir}/lib")
target_include_directories(urbanterror_vorbis PRIVATE "external/${urbanterror_ogg_dir}/include")

add_library(urbanterrorrenderer1 STATIC
        ${urbanterrorrenderer_src}
        )
set_target_properties(urbanterrorrenderer1 PROPERTIES COMPILE_FLAGS "-fPIC")
target_include_directories(urbanterrorrenderer1 PRIVATE "external/jpeg-8c")
target_include_directories(urbanterrorrenderer1 PRIVATE "external/curl-7.54.0/include")
target_include_directories(urbanterrorrenderer1 PRIVATE "${URBANTERROR_COMMON_DIR}")

set(URBANTERRORLIBRARY urbanterror)
add_library(${URBANTERRORLIBRARY} SHARED
        ${urbanterrorclient_src}
        ${urbanterrorjpeglib_src}
        ${urbanterror_opus_src}
        ${urbanterror_ogg_src}
        ${urbanterror_zlib_src}
        )
set_target_properties(${URBANTERRORLIBRARY} PROPERTIES COMPILE_DEFINITIONS "BOTLIB")
target_link_libraries(${URBANTERRORLIBRARY}
        ${urbanterrorclient_libs}
        urbanterror_vorbis
        urbanterrorrenderer1
        )
target_include_directories(${URBANTERRORLIBRARY} PRIVATE "external/curl-7.54.0/include")
target_include_directories(${URBANTERRORLIBRARY} PRIVATE "external/${urbanterror_ogg_dir}/include")
target_include_directories(${URBANTERRORLIBRARY} PRIVATE "external/${urbanterror_vorbis_dir}/include")
if(USE_CODEC_OPUS)
    target_include_directories(${URBANTERRORLIBRARY} PRIVATE "external/${urbanterror_opus_dir}/include" "external/${urbanterror_opus_dir}/celt" "external/${urbanterror_opus_dir}/silk" "external/${urbanterror_opus_dir}/silk/float" "external/${urbanterror_opusfile_dir}/include")
endif()
target_include_directories(${URBANTERRORLIBRARY} PRIVATE "external/jpeg-8c")
target_include_directories(${URBANTERRORLIBRARY} PRIVATE "code")
target_include_directories(${URBANTERRORLIBRARY} PRIVATE "${URBANTERROR_COMMON_DIR}")
if(USE_FREETYPE)
    target_include_directories(${URBANTERRORLIBRARY} PRIVATE "../deplibs/freetype/include")
endif()
if(USE_INTERNAL_ZLIB)
    target_include_directories(${URBANTERRORLIBRARY} PRIVATE "code/${urbanterror_zlib_dir}")
endif()
