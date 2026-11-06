gennames_src = [
  '../../lib/libdwarf/dwarf_safe_strcpy.c',
  '../dwarfdump/dd_getopt.c',
  'gennames.c'
]

gennames_args = ['-I.']
if (lib_type == 'static')
  gennames_args += ['-DLIBDWARF_STATIC']
endif

localinclude = include_directories('../dwarfdump')
executable('gennames', gennames_src,
  c_args : [ dev_cflags, libdwarf_args, gennames_args ],
  link_args :  dwarf_link_args,
  dependencies : libdwarf,
  include_directories : [ config_dir, localinclude],
  install : false
)
