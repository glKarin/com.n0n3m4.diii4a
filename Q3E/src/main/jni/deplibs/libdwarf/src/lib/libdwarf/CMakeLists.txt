set_source_group(SOURCES "Source Files"
dwarf_abbrev.c
dwarf_alloc.c
dwarf_crc.c
dwarf_crc32.c
dwarf_arange.c
dwarf_debug_sup.c
dwarf_debugaddr.c
dwarf_debuglink.c
dwarf_die_deliv.c
dwarf_debugnames.c
dwarf_dsc.c
dwarf_elf_load_headers.c
dwarf_elfread.c
dwarf_elf_rel_detector.c
dwarf_error.c
dwarf_fill_in_attr_form.c
dwarf_find_sigref.c
dwarf_fission_to_cu.c
dwarf_form.c
dwarf_form_class_names.c
dwarf_frame.c
dwarf_frame2.c
dwarf_frame_iter.c
dwarf_frame_cfa_read.c
dwarf_gdbindex.c
dwarf_global.c
dwarf_gnu_index.c
dwarf_groups.c
dwarf_harmless.c
dwarf_generic_init.c
dwarf_init_finish.c
dwarf_leb.c
dwarf_line.c
dwarf_loc.c
dwarf_loclists.c
dwarf_locationop_read.c
dwarf_local_malloc.c
dwarf_lname_version.c
dwarf_lvn_name.c
dwarf_machoread.c
dwarf_64machoread.c
dwarf_macro.c
dwarf_macro5.c
dwarf_memcpy_swap.c
dwarf_names.c
dwarf_object_read_common.c
dwarf_object_detector.c
dwarf_peread.c
dwarf_query.c
dwarf_ranges.c
dwarf_rnglists.c
dwarf_safe_arithmetic.c
dwarf_safe_strcpy.c
dwarf_secname_ck.c
dwarf_seekr.c
dwarf_setup_sections.c
dwarf_string.h
dwarf_string.c
dwarf_stringsection.c
dwarf_tied.c
dwarf_str_offsets.c
dwarf_tsearchhash.c
dwarf_util.c
dwarf_xu_index.c
dwarf_print_lines.c)

set_source_group(HEADERS "Header Files"
dwarf.h
dwarf_abbrev.h
dwarf_alloc.h
dwarf_arange.h
dwarf_base_types.h
dwarf_debugaddr.h
dwarf_debuglink.h
dwarf_die_deliv.h
dwarf_debugnames.h
dwarf_dsc.h
dwarf_elf_access.h
dwarf_elf_defines.h
dwarf_elfread.h
dwarf_elf_rel_detector.h
dwarf_elfstructs.h
dwarf_error.h
dwarf_frame.h
dwarf_gdbindex.h
dwarf_global.h
dwarf_harmless.h
dwarf_gnu_index.h
dwarf_line.h
dwarf_loc.h
dwarf_loclists.h
dwarf_local_malloc.h
dwarf_machoread.h
dwarf_macro.h
dwarf_macro5.h
dwarf_object_detector.h
dwarf_opaque.h
dwarf_pe_descr.h
dwarf_peread.h
dwarf_reading.h
dwarf_rnglists.h
dwarf_safe_arithmetic.h
dwarf_safe_strcpy.h
dwarf_tied_decls.h
dwarf_tsearch.h
dwarf_setup_sections.h
dwarf_str_offsets.h
dwarf_universal.h
dwarf_util.h
dwarf_xu_index.h
libdwarf_private.h
dwarf_macho_loader.h
dwarf_memcpy_swap.h)

set_source_group(CONFIGURATION_FILES "Configuration Files"
  "${PROJECT_SOURCE_DIR}/cmake/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h")

# The -DPIC is so we find the right DW_API value in libdwarf.h
# with cmake with Linux.
# Dwarfgen requires all symbols in .so to be visible, so
# do not say hidden on libdwarf in that case.
if (BUILD_SHARED)
    if(NOT BUILD_DWARFGEN)
      set(CMAKE_C_VISIBILITY_PRESET hidden)
    endif()
    set(DEFS LIBDWARF_BUILD PIC)
else()
    set(DEFS LIBDWARF_STATIC)
endif()

if (PIC_ALWAYS)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  list(APPEND DEFS PIC)
endif()

if(BUILD_SHARED)
    add_library(dwarf SHARED ${SOURCES} ${HEADERS} ${CONFIGURATION_FILES})
    add_library(libdwarf::dwarf-shared ALIAS dwarf)
else()
    add_library(dwarf STATIC ${SOURCES} ${HEADERS} ${CONFIGURATION_FILES})
    add_library(libdwarf::dwarf-static ALIAS dwarf)
endif()
set_folder(dwarf src/lib/libdwarf)
target_compile_options(dwarf PRIVATE ${COMPILER_FLAGS} ${DW_FWALL}
    -D_GNU_SOURCE)
msvc_posix(dwarf)
target_compile_definitions(dwarf PUBLIC ${DEFS})
target_include_directories(dwarf PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/lib/libdwarf>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
if(ZLIB_FOUND AND zstd_FOUND)
  target_link_libraries(dwarf PRIVATE  ZLIB::ZLIB ${ZSTD_LIB} )
endif()
set_target_properties(dwarf PROPERTIES PUBLIC_HEADER "libdwarf.h;dwarf.h")
set_target_properties(dwarf PROPERTIES VERSION "${PROJECT_VERSION}" SOVERSION "${PROJECT_VERSION_MAJOR}")
install(TARGETS dwarf
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )

configure_file(libdwarf.pc.in libdwarf.pc @ONLY)

# The install has to be here, not in
# another CMakeLists.txt  to make install work properly
# for cmake before cmake 3.13. This also works
# for newer cmake.
include(CMakePackageConfigHelpers)
configure_package_config_file(${PROJECT_SOURCE_DIR}/src/lib/libdwarf/cmake/libdwarfConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/libdwarfConfig.cmake" INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdwarf")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/libdwarfConfigVersion.cmake" VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libdwarfConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/libdwarfConfigVersion.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdwarf")
install(TARGETS dwarf EXPORT libdwarfTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(EXPORT libdwarfTargets
        FILE libdwarf-targets.cmake
        NAMESPACE libdwarf::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdwarf")
export(
  TARGETS dwarf
  NAMESPACE libdwarf::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/libdwarf-targets.cmake"
)
install(FILES "${PROJECT_BINARY_DIR}/src/lib/libdwarf/libdwarf.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(FILES "${PROJECT_SOURCE_DIR}/cmake/Findzstd.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdwarf")
