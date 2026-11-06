

uritable_args = []

if (lib_type == 'static')
  uritable_args += ['-DLIBDWARF_STATIC']
endif

executable('builduritable', 'uritablebuild.c',
  c_args : [ dev_cflags, libdwarf_args,uritable_args ],
  link_args :  dwarf_link_args,
  dependencies : libdwarf,
  include_directories : config_dir,
  install : false
)
