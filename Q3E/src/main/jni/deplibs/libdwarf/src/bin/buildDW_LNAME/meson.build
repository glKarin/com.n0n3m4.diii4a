
localinclude = include_directories('../dwarfdump')

buildopstab_args = [ '-I.' ]

executable('checklangname', 'checklangname.c',
  c_args : [ dev_cflags, libdwarf_args, buildopstab_args ],
  link_args :  dwarf_link_args,
  include_directories : [config_dir,localinclude],
  install : false
)
