
attr_form_src = [
  'attr_form_build.c',
  '../tag_tree/tag_common.c',
  '../dwarfdump/dd_attr_form.c',
  '../dwarfdump/dd_common.c',
  '../dwarfdump/dd_esb.c',
  '../dwarfdump/dd_getopt.c',
  '../dwarfdump/dd_glflags.c',
  '../dwarfdump/dd_makename.c',
  '../dwarfdump/dd_naming.c',
  '../dwarfdump/dd_safe_strcpy.c',
  '../dwarfdump/dd_sanitized.c',
  '../dwarfdump/dd_tsearchbal.c',
  '../dwarfdump/dd_utf8.c'
]

attr_form_args = [
  '-DTRIVIAL_NAMING',
  '-DBUILD_NONLIB_SOURCE',
  '-DSKIP_AF_CHECK',
]

if (lib_type == 'static')
  attr_form_args += ['-DLIBDWARF_STATIC']
endif

localinclude = include_directories('../dwarfdump')

executable('attr_form', attr_form_src,
  c_args : [ dev_cflags, libdwarf_args, attr_form_args ],
  link_args :  dwarf_link_args,
  dependencies : libdwarf,
  include_directories : [config_dir, localinclude],
  install : false
)
