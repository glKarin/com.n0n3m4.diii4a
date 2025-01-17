make_release_only()

add_library(adl OBJECT)

target_sources(adl
PRIVATE
	adlmidi_midiplay.cpp
	adlmidi_opl3.cpp
	adlmidi_private.cpp
	adlmidi.cpp
	adlmidi_load.cpp
	inst_db.cpp
	chips/opal_opl3.cpp
	chips/dosbox/dbopl.cpp
	chips/nuked_opl3_v174.cpp
	chips/java_opl3.cpp
	chips/dosbox_opl3.cpp
	chips/nuked_opl3.cpp
	chips/nuked/nukedopl3_174.c
	chips/nuked/nukedopl3.c
	wopl/wopl_file.c
)

target_compile_definitions(adl PRIVATE ADLMIDI_DISABLE_MIDI_SEQUENCER)

target_include_directories(adl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

use_fast_math(adl)
