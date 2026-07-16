# FFmpeg 5 个库的 IMPORTED INTERFACE 定义
# 使用方式：在你的 CMakeLists.txt 中 include 本文件，然后 target_link_libraries(你的目标 PRIVATE ffmpeg::avcodec ffmpeg::avformat ...)

# ========== 配置路径（按需修改） ==========
set(FFMPEG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_ROOT}/depincs/ffmpeg/ffmpeg60/include")
set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/libs")

# ========== 工具函数：创建 IMPORTED SHARED 库 ==========
function(create_ffmpeg_lib name)
    add_library(ffmpeg::${name} SHARED IMPORTED GLOBAL)
    set_target_properties(ffmpeg::${name} PROPERTIES
        IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/lib${name}.so"
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
    )
    # 传递依赖
    if(ARGN)
        set_target_properties(ffmpeg::${name} PROPERTIES
            INTERFACE_LINK_LIBRARIES "${ARGN}"
        )
    endif()
endfunction()

# ========== 5 个库定义（含依赖关系） ==========

# avutil：基础工具库，无依赖
create_ffmpeg_lib(avutil)

# swresample：重采样，依赖 avutil
create_ffmpeg_lib(swresample ffmpeg::avutil)

# swscale：图像缩放/格式转换，依赖 avutil
create_ffmpeg_lib(swscale ffmpeg::avutil)

# avcodec：编解码，依赖 avutil + swresample
create_ffmpeg_lib(avcodec ffmpeg::avutil ffmpeg::swresample)

# avformat：封装/解封装，依赖 avcodec + avutil
create_ffmpeg_lib(avformat ffmpeg::avcodec ffmpeg::avutil)
