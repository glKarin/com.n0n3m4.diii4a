
# man page

install_man('dwarfdump.1')

if get_option('dwarfgen') == true
  install_man('dwarfgen.1')
endif

have_doc = 'no'
if get_option('doc') == true
  doxygen = find_program('doxygen', required : false)
  if doxygen.found() == true
    have_doc = 'yes'
    doxygen_cmd = run_command(doxygen, check: true)
    dir_doc = join_paths(dir_data, 'doc/libdwarf')
    install_subdir('html', install_dir : dir_doc)
    install_subdir('latex', install_dir : dir_doc)
  else
    warning('doxygen not found, documentation generation is disabled')
  endif
endif
