project(imgui)

set(CMAKE_CXX_STANDARD "11")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

set(src_imgui
		backends/imgui_impl_opengl3.cpp

		imgui.h
		imgui.cpp
		imgui_draw.cpp
		imgui_tables.cpp
		imgui_widgets.cpp

		imgui_demo.cpp
)

if(ANDROID)
	set(src_imgui ${src_imgui} backends/imgui_impl_android.cpp)
else()
	set(src_imgui ${src_imgui} backends/imgui_impl_sdl2.cpp)
endif()

ADD_LIBRARY(idtech4_imgui STATIC ${src_imgui})
target_include_directories(idtech4_imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)
