
dwarfdump_src = [
  'dd_addrmap.c',
  'dd_all_srcfiles.c',
  'dd_attr_form.c',
  'dd_canonical_append.c',
  'dd_check_attr_encoding.c',
  'dd_checkutil.c',
  'dd_command_options.c',
  'dd_common.c',
  'dd_compiler_info.c',
  'dd_regex.c',
  'dd_safe_strcpy.c',
  'dwarfdump.c',
  'dd_tsearchbal.c',
  'dd_dwconf.c',
  'dd_getopt.c',
  'dd_esb.c',
  'dd_glflags.c',
  'dd_helpertree.c',
  'dd_lvn_table.c',
  'dd_macrocheck.c',
  'dd_makename.c',
  'dd_naming.c',
  'dd_opscounttab.c',
  'print_abbrevs.c',
  'print_aranges.c',
  'print_debugfission.c',
  'print_die.c',
  'dd_trace_abstract_origin_etc.c',
  'print_debug_addr.c',
  'print_debug_gnu.c',
  'print_debug_names.c',
  'print_debug_sup.c',
  'print_frames.c',
  'print_gdbindex.c',
  'print_hipc_lopc_attr.c',
  'print_lines.c',
  'print_llex_codes.c',
  'print_origloclist_codes.c',
  'print_loclists.c',
  'print_loclists_codes.c',
  'print_macinfo.c',
  'print_macro.c',
  'print_pubnames.c',
  'print_ranges.c',
  'print_rnglists.c',
  'print_section_groups.c',
  'print_sections.c',
  'print_str_offsets.c',
  'print_strings.c',
  'print_tag_attributes_usage.c',
  'dd_sanitized.c',
  'dd_strstrnocase.c',
  'dd_true_section_name.c',
  'dd_uri.c',
  'dd_utf8.c'
]

pkgdwarfdump = join_paths(dir_data, 'dwarfdump')
dwarfdump_args = [ '-DCONFPREFIX=' + pkgdwarfdump ]

if (lib_type == 'static')
  dwarfdump_args += ['-DLIBDWARF_STATIC']
endif

dwarfdump_exe = executable('dwarfdump', dwarfdump_src,
  c_args : [ dev_cflags, libdwarf_args, dwarfdump_args ],
  link_args :  dwarf_link_args ,
  dependencies : libdwarf,
  include_directories : config_dir,
  install : true
)

meson.override_find_program('dwarfdump', dwarfdump_exe)

install_data('dwarfdump.conf',
  install_dir : pkgdwarfdump
)
