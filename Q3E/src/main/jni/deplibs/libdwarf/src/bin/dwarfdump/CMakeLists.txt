
set_source_group(SOURCES "Source Files" dd_addrmap.c
    dd_all_srcfiles.c
    dd_checkutil.c dd_common.c dd_regex.c dd_safe_strcpy.c
    dwarfdump.c dd_dwconf.c dd_helpertree.c
    dd_glflags.c dd_command_options.c dd_compiler_info.c
    dd_macrocheck.c
    dd_opscounttab.c
    print_abbrevs.c print_aranges.c
    dd_attr_form.c
    dd_canonical_append.c
    dd_check_attr_encoding.c
    dd_lvn_table.c
    print_debugfission.c print_die.c
    dd_trace_abstract_origin_etc.c
    print_debug_addr.c
    print_debug_gnu.c
    print_debug_names.c  print_debug_sup.c
    print_frames.c  print_gdbindex.c
    print_hipc_lopc_attr.c
    print_lines.c
    print_llex_codes.c print_origloclist_codes.c
    print_loclists_codes.c
    print_loclists.c
    print_macro.c print_macinfo.c
    print_pubnames.c print_ranges.c
    print_rnglists.c
    print_str_offsets.c
    print_sections.c  print_section_groups.c
    print_strings.c
    print_tag_attributes_usage.c
    dd_sanitized.c dd_strstrnocase.c
    dd_true_section_name.c dd_uri.c dd_utf8.c
    dd_getopt.c dd_makename.c
    dd_naming.c dd_esb.c dd_tsearchbal.c)
	
set_source_group(HEADERS "Header Files"
  dd_addrmap.h dd_all_srcfiles.h
  dd_attr_form.h dd_checkutil.h dd_common.h dd_regex.h
  dd_safe_strcpy.h dd_dwconf.h
  dd_minimal.h
  dd_command_options.h dd_compiler_info.h
  dd_opscounttab.h
  print_debug_gnu.h
  dd_dwconf_using_functions.h dd_esb_using_functions.h
  dd_elf_cputype.h
  dd_pe_cputype.h
  dd_helpertree.h
  dd_canonical_append.h
  dwarfdump-af-table-ext.h dwarfdump-af-table-std.h
  dwarfdump-ta-ext-table.h dwarfdump-ta-table.h
  dwarfdump-tt-ext-table.h dwarfdump-tt-table.h
  dd_getopt.h dd_esb.h dd_glflags.h dd_globals.h
  dd_mac_cputype.h
  dd_macrocheck.h dd_defined_types.h
  dd_sanitized.h
  dd_naming.h dd_makename.h dd_tsearchbal.h print_frames.h
  dd_uri.h dd_utf8.h
  ../../lib/libdwarf/libdwarf_private.h)

set_source_group(CONFIGURATION_FILES "Configuration Files"
  ${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h)
	
add_executable(dwarfdump ${SOURCES} ${HEADERS} ${CONFIGURATION_FILES})

set_folder(dwarfdump src/bin/dwarfdump)

target_compile_definitions(dwarfdump PRIVATE
    "CONFPREFIX=${CMAKE_INSTALL_PREFIX}/lib" ${DW_LIBDWARF_STATIC}
    _GNU_SOURCE)
target_compile_options(dwarfdump PRIVATE ${DW_FWALL})

target_link_libraries(dwarfdump PRIVATE dwarf)

install(TARGETS dwarfdump DESTINATION
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES dwarfdump.conf DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dwarfdump)
