cmake_minimum_required(VERSION 3.9)
project (source-vaudio_opus)

set(binary_name ${SOURCE_ENGINE_PREFIX}vaudio_opus)

set(source_src 
		'voiceencoder_opus.cpp',
		'../frame_encoder/voice_codec_frame.cpp',
		'../../../tier1/interface.cpp',
		'../../../public/tier0/memoverride.cpp'
)

set(source_inc 
		'.',
		'../frame_encoder',
		'../../../public',
		'../../../public/tier1',
		'../../../public/tier0',
		'../../',
		'../../../common',
		'../../../public/engine/audio',
		'celt'
)

set(source_def "OPUS_EXPORTS=1")

set(source_lib 
	${SOURCE_ENGINE_PREFIX}tier0 
	${SOURCE_ENGINE_PREFIX}tier1 
	${SOURCE_ENGINE_PREFIX}vstdlib
	OPUS
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})
set_target_properties(${binary_name} PROPERTIES COMPILE_DEFINITIONS "${source_def}")
target_link_libraries(${binary_name} PRIVATE ${source_lib})

