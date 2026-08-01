
tests = [
  [
   'test_dwarfstring.c',
   '../src/lib/libdwarf/dwarf_string.c'
  ],
  [
   'test_extra_flag_strings.c',
   '../src/lib/libdwarf/dwarf_string.c',
   '../src/lib/libdwarfp/dwarf_pro_log_extra_flag_strings.c'
  ],
  [
   'test_linkedtopath.c',
   '../src/lib/libdwarf/dwarf_string.c',
   '../src/lib/libdwarf/dwarf_safe_strcpy.c',
   '../src/lib/libdwarf/dwarf_debuglink.c',
  ],
  [
   'test_getopt.c',
   '../src/bin/dwarfdump/dd_getopt.c'
  ],
  [
   'test_makename.c',
   '../src/bin/dwarfdump/dd_esb.c',
   '../src/bin/dwarfdump/dd_makename.c',
   '../src/bin/dwarfdump/dd_tsearchbal.c'
  ],
  [
   'test_helpertree.c',
   '../src/bin/dwarfdump/dd_helpertree.c',
   '../src/bin/dwarfdump/dd_tsearchbal.c'
  ],
  [
   'test_ignoresec.c',
   '../src/lib/libdwarf/dwarf_secname_ck.c',
  ],
  [
   'test_esb.c',
   '../src/bin/dwarfdump/dd_esb.c',
   '../src/bin/dwarfdump/dd_tsearchbal.c'
  ],
  [
   'test_sanitized.c',
   '../src/bin/dwarfdump/dd_esb.c',
   '../src/bin/dwarfdump/dd_sanitized.c',
   '../src/bin/dwarfdump/dd_utf8.c'
  ],

  [
   'test_macrocheck.c',
   '../src/bin/dwarfdump/dd_esb.c',
   '../src/bin/dwarfdump/dd_tsearchbal.c'
  ],
  [
   'test_dwarf_leb.c',
   '../src/lib/libdwarf/dwarf_leb.c'
  ],
  [
   'test_dwarf_tied.c',
   '../src/lib/libdwarf/dwarf_tied.c',
   '../src/lib/libdwarf/dwarf_tsearchhash.c'
  ],
  [
   'test_getname.c',
   '../src/lib/libdwarf/dwarf_names.c'
  ],
  [
   'test_canonical.c',
   '../src/bin/dwarfdump/dd_canonical_append.c',
   '../src/bin/dwarfdump/dd_safe_strcpy.c',
   '../src/bin/dwarfdump/dd_esb.c'
  ],
  [
   'test_int64_test.c'
  ],
  [
   'test_safe_strcpy.c',
   '../src/lib/libdwarf/dwarf_safe_strcpy.c',
  ],
  [
   'test_setupsections.c',
   '../src/lib/libdwarf/dwarf_setup_sections.c',
  ],
  [
   'test_regex.c',
   '../src/bin/dwarfdump/dd_regex.c',
  ]
]

incdir = include_directories(
  '../src/lib/libdwarf',
  '../src/lib/libdwarfp',
  '../src/bin/dwarfdump'
)

#  pathname = join_paths('test',test_name)
#    executable(pathname, test_src,
foreach test_src : tests
  test_name = test_src[0].split('.')[0]
  test(test_name,
    executable(test_name, test_src,
      c_args : [ dev_cflags, libdwarf_args, '-DTESTING', '-DLIBDWARF_BUILD' ],
      link_args :  dwarf_link_args,
      include_directories : [ config_dir, incdir ],
      install : false
    )
  )
endforeach

argstests = [
  [
   'test_errmsglist.c',
   '../src/lib/libdwarf/dwarf_safe_strcpy.c'
  ],
  [
   'test_lname.c'
  ]
]

projectbase = project_source_base_root
message(['mesondebug test projectbase',projectbase])
foreach atest_src : argstests
  atest_name = atest_src[0].split('.')[0]
  atexec = executable(atest_name, atest_src,
    c_args : [ dev_cflags, libdwarf_args, '-DTESTING' ],
    link_args :  dwarf_link_args,
    include_directories : [ config_dir, incdir ],
    install : false)
  test(atest_name,atexec, args: ['-f',projectbase])
endforeach

semantic_ver = meson.project_version()
pyscripttests = [
  ['Elf'],
  ['PE',],
  ['Macos'],
]

#git_exe = find_program('git', required: false)
#pkgconfig_mod = import('pkgconfig')


builddir = project_build_base_root
message(['mesondebug test builds ',builddir])

configure_file(
  input: '../src/bin/dwarfdump/dwarfdump.conf',
  output: 'dwarfdump.conf',
  copy: true
)

py3_exe = import('python').find_installation(required:false)
if py3_exe.found()
  foreach testscr : pyscripttests
    pytest_name = testscr[0]
    message(pytest_name)
    semanticver = meson.project_version()
    buildbase = builddir
    pyexec_name = join_paths(projectbase,'test','test_dwarfdump.py')
    test(pytest_name,py3_exe, args: [pyexec_name, pytest_name,'meson', projectbase, buildbase, semanticver])
  endforeach
endif

shscripttests =  []
if get_option('dwarfexample') == true
  if host_os == 'windows'
    shscripttests =  [
    ['test_jitreaderdiff.sh'],
    ]
  else
    shscripttests =  [
    ['test_debuglink-a.sh'],
    ['test_debuglink-b.sh'],
    ['test_jitreaderdiff.sh'],
  ]
  endif
endif

sh_exe = find_program('sh',required:false)
if sh_exe.found()
  foreach shscr : shscripttests
    test_name = shscr[0]
    message(test_name)
    shexec_name = join_paths(projectbase,'test',test_name)
    test(test_name,sh_exe,args: [shexec_name, projectbase ])
  endforeach
endif
