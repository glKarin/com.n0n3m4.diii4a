
examples = [
  'dwdebuglink.c',
  'findfuncbypc.c',
  'frame1.c',
  'frame2.c',
  'jitreader.c',
  'showsectiongroups.c',
  'simplereader.c'
]

example_args = []

if (lib_type == 'static')
  example_args += ['-DLIBDWARF_STATIC']
endif

libdwarf_dir = include_directories('../../lib/libdwarf')

foreach example_src : examples
  example_name = example_src.split('.')[0]
  executable(example_name, example_src,
    c_args : [ dev_cflags, libdwarf_args, example_args ],
    link_args :  dwarf_link_args,
    dependencies : libdwarf,
    include_directories : [ config_dir, libdwarf_dir ],
    install : false
  )
endforeach
