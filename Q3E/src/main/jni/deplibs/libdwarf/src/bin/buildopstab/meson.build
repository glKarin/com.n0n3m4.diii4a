
localinclude = include_directories('../dwarfdump')

buildopstab_args = [ '-I.' ]

if (lib_type == 'static')
  buildopstab_args += ['-DLIBDWARF_STATIC']
endif

executable('buildopstab', 'buildopscounttab.c',
  c_args : [ dev_cflags, libdwarf_args, buildopstab_args ],
  link_args :  dwarf_link_args,
  dependencies : libdwarf,
  include_directories : [config_dir,localinclude],
  install : false
)
