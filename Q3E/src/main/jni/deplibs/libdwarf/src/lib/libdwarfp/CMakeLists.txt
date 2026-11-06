set_source_group(SOURCES "Source Files"
"${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_memcpy_swap.c"
"${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_string.c"
"${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_tsearchhash.c"
dwarf_pro_alloc.c
dwarf_pro_arange.c
dwarf_pro_debug_sup.c
dwarf_pro_die.c
dwarf_pro_dnames.c
dwarf_pro_error.c
dwarf_pro_expr.c
dwarf_pro_finish.c
dwarf_pro_forms.c
dwarf_pro_funcs.c
dwarf_pro_frame.c
dwarf_pro_init.c
dwarf_pro_line.c
dwarf_pro_reloc.c
dwarf_pro_reloc_stream.c
dwarf_pro_reloc_symbolic.c
dwarf_pro_pubnames.c
dwarf_pro_section.c
dwarf_pro_types.c
dwarf_pro_vars.c
dwarf_pro_macinfo.c
dwarf_pro_weaks.c)

set_source_group(HEADERS "Header Files"
libdwarfp.h
dwarf_pro_alloc.h
dwarf_pro_arange.h
dwarf_pro_die.h
dwarf_pro_error.h
dwarf_pro_expr.h
dwarf_pro_frame.h
dwarf_pro_incl.h
dwarf_pro_line.h
dwarf_pro_log_extra_flag_strings.c
dwarf_pro_macinfo.h
dwarf_pro_opaque.h
dwarf_pro_reloc.h
dwarf_pro_reloc_stream.h
dwarf_pro_reloc_symbolic.h
dwarf_pro_section.h
dwarf_pro_types.h
dwarf_pro_util.h)

set_source_group(CONFIGURATION_FILES "Configuration Files"
    "${PROJECT_SOURCE_DIR}/cmake/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h")

if (BUILD_SHARED)
    if(NOT BUILD_DWARFGEN)
      set(CMAKE_C_VISIBILITY_PRESET hidden)
    endif()
    set(DEFS LIBDWARF_BUILD PIC)
else()
    set(DEFS LIBDWARF_STATIC)
endif()

if (BUILD_SHARED)
    add_library(dwarfp SHARED ${SOURCES} ${HEADERS} ${CONFIGURATION_FILES})
    add_library(libdwarf::dwarfp-shared ALIAS dwarfp)
else()
    add_library(dwarfp STATIC ${SOURCES} ${HEADERS} ${CONFIGURATION_FILES})
    add_library(libdwarf::dwarfp-static ALIAS dwarfp)
endif()
# Macos needs the PRIVATE too using APPLE
# needed with MINGW OR MSYS OR WIN32 OR APPLE
# allowed everywhere.
target_link_libraries(dwarfp PRIVATE dwarf)
set_folder(dwarfp src/lib/libdwarfp)
target_compile_definitions(dwarfp PUBLIC ${DEFS})
target_compile_definitions(dwarfp PRIVATE _GNU_SOURCE)
set_target_properties(dwarfp PROPERTIES VERSION "${PROJECT_VERSION}" SOVERSION "${PROJECT_VERSION_MAJOR}")
target_compile_options(dwarfp PRIVATE ${DW_COMPILER_FLAGS}
    ${DW_FWALL})
msvc_posix(dwarfp)
set_target_properties(dwarfp PROPERTIES PUBLIC_HEADER "libdwarfp.h")

install(TARGETS dwarfp
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )

# The install has to be here, not in
# ../CMakeLists.txt  to make install work properly
# for cmake before cmake 3.13. This also works
# for newer cmake.
configure_file(libdwarfp.pc.in libdwarfp.pc @ONLY)

install(TARGETS dwarfp  EXPORT libdwarfpTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )
install(EXPORT libdwarfpTargets FILE libdwarfp-targets.cmake
        NAMESPACE libdwarfp::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdwarfp")
install(FILES cmake/libdwarfp-config.cmake DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdwarfp")
install(FILES "${PROJECT_BINARY_DIR}/src/lib/libdwarfp/libdwarfp.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
