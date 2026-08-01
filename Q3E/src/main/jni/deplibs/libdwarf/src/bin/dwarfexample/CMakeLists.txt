
add_library(dwarf_example INTERFACE)
target_compile_definitions(dwarf_example INTERFACE CONFPREFIX={CMAKE_INSTALL_PREFIX}/lib ${DW_LIBDWARF_STATIC} _GNU_SOURCE)
target_compile_options(dwarf_example INTERFACE ${DW_FWALL})
target_link_libraries(dwarf_example INTERFACE dwarf)

add_executable(simplereader simplereader.c)
set_folder(simplereader src/bin/dwarfexample)
target_link_libraries(simplereader PRIVATE dwarf_example)

add_executable(frame1 frame1.c)
set_folder(frame1 src/bin/dwarfexample)
target_link_libraries(frame1 PRIVATE dwarf_example)

add_executable(frame2 frame2.c)
set_folder(frame2 src/bin/dwarfexample)
target_link_libraries(frame2 PRIVATE dwarf_example)

add_executable(findfuncbypc findfuncbypc.c)
set_folder(findfuncbypc src/bin/dwarfexample)
target_link_libraries(findfuncbypc PRIVATE dwarf_example)

add_executable(dwdebuglink dwdebuglink.c)
set_folder(dwdebuglink src/bin/dwarfexample)
target_link_libraries(dwdebuglink PRIVATE dwarf_example)

add_executable(jitreader jitreader.c)
set_folder(jitreader src/bin/dwarfexample)
target_link_libraries(jitreader PRIVATE dwarf_example)

add_executable(showsectiongroups showsectiongroups.c)
set_folder(jitreader src/bin/dwarfexample)
target_link_libraries(showsectiongroups PRIVATE dwarf_example)
