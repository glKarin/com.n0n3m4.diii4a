
tag_attr_src = [
  'tag_attr.c',
  '../dwarfdump/dd_common.c',
  '../dwarfdump/dd_esb.c',
  '../dwarfdump/dd_getopt.c',
  '../dwarfdump/dd_glflags.c',
  '../dwarfdump/dd_makename.c',
  '../dwarfdump/dd_naming.c',
  '../dwarfdump/dd_safe_strcpy.c',
  '../dwarfdump/dd_sanitized.c',
  '../dwarfdump/dd_tsearchbal.c',
  '../dwarfdump/dd_utf8.c',
  '../tag_tree/tag_common.c'
]

tag_attr_args = [
  '-DTRIVIAL_NAMING',
  '-DBUILD_NONLIB_SOURCE',
]

if (lib_type == 'static')
  tag_attr_args += ['-DLIBDWARF_STATIC']
endif

localinclude = include_directories('../dwarfdump')

executable('tag_attr', tag_attr_src,
  c_args : [ dev_cflags, libdwarf_args, tag_attr_args ],
  link_args :  dwarf_link_args,
  dependencies : libdwarf,
  include_directories : [config_dir,localinclude],
  install : false
)
