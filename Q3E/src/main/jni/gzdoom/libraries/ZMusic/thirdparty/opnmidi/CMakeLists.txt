make_release_only()

add_library(opn OBJECT)

target_sources(opn
PRIVATE
	opnmidi_load.cpp
	opnmidi_private.cpp
	opnmidi.cpp
	opnmidi_midiplay.cpp
	opnmidi_opn2.cpp
	chips/np2/fmgen_fmgen.cpp
	chips/np2/fmgen_opna.cpp
	chips/np2/fmgen_fmtimer.cpp
	chips/np2/fmgen_file.cpp
	chips/np2/fmgen_psg.cpp
	chips/mame_opn2.cpp
	chips/gens_opn2.cpp
	chips/mame_opna.cpp
	chips/np2_opna.cpp
	chips/mamefm/ymdeltat.cpp
	chips/mamefm/resampler.cpp
	chips/mamefm/fm.cpp
	chips/nuked_opn2.cpp
	chips/gens/Ym2612.cpp
	chips/gx_opn2.cpp
	chips/pmdwin_opna.cpp
	chips/nuked/ym3438.c
	chips/gx/gx_ym2612.c
	chips/pmdwin/opna.c
	chips/pmdwin/psg.c
	chips/pmdwin/rhythmdata.c
	chips/mamefm/emu2149.c
	chips/mame/mame_ym2612fm.c
	wopn/wopn_file.c
)

target_compile_definitions(opn
PRIVATE
	# we play with out own sequencer
	OPNMIDI_DISABLE_MIDI_SEQUENCER
	# Disable OPNMIDI's experimental yet emulator (using of it has some issues and missing notes in playback)
	OPNMIDI_DISABLE_GX_EMULATOR
)

target_include_directories(opn PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

use_fast_math(opn)
