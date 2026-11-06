project('libdwarf', ['c','cpp'],
  version: '2.3.2',
  default_options : [
    'buildtype=debugoptimized',
    'warning_level=3',
  ],
  meson_version : '>=0.54'
)

# version 0.56 is the first version with project_source_root and
# project_build_root.   We use meson.current_source_dir() and
# meson.current_build_dir()  (and read these before
# meson knows of other directories) to avoid meson.build_root()
# meson.source_root() and avoid requring 0.56. Ubuntu 20.04,
# for example, uses meson 0.53.

# meson file system module
fs = import('fs')

v_arr = meson.project_version().split('.')
v_maj = v_arr[0]
v_min = v_arr[1]
v_mic = v_arr[2]

lib_type = get_option('default_library')

# install paths
dir_prefix = get_option('prefix')
dir_include = join_paths(dir_prefix, get_option('includedir'))
dir_pkginclude = join_paths(dir_include, meson.project_name())
dir_bin = join_paths(dir_prefix, get_option('bindir'))
dir_lib = join_paths(dir_prefix, get_option('libdir'))
dir_data = join_paths(dir_prefix, get_option('datadir'))
dir_pkgdata = join_paths(dir_data, meson.project_name())
dir_locale = join_paths(dir_prefix, get_option('localedir'))

# host
windows = import('windows')
host_os = host_machine.system()

windows = ['windows', 'cygwin']
#bsd for meson 0.46 and 0.47
bsd = ['bsd', 'freebsd', 'dragonfly', 'netbsd', 'openbsd']
linux = ['linux']
osx = ['darwin']
sun = ['sunos']

sys_linux = linux.contains(host_machine.system())
sys_bsd = bsd.contains(host_machine.system())
sys_windows = windows.contains(host_machine.system())
sys_osx = osx.contains(host_machine.system())
sys_sun = sun.contains(host_machine.system())

# compiler

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

dev_cflags = []
dev_cppflags = []
dev_cppflags_onlylibdwarf = []
dev_ldflags = []

# -D_GNU_SOURCE is needed as otherwise
# certain gcc options (such as -std=c99 )
# will prevent building because strdup()
# in string.h will be invisible.

dev_cflags_try = [
  '-Wpointer-arith',
  '-Wmissing-declarations',
  '-Wstrict-prototypes',
  '-Wcomment',
  '-Wformat',
  '-Wuninitialized',
  '-Wshadow',
  '-Wno-long-long',
  '-Wfloat-compare',
  '-Wsign-compare',
  '-Wno-missing-field-initializers',
  '-fno-omit-frame-pointer',
  '-D_GNU_SOURCE'
]
sanitize_flags_try = [
  '-fsanitize=address',
  '-fsanitize=leak',
  '-fsanitize=undefined'
]

drmemory_flags_try = [
  '-fno-inline',
  '-g',
  '-g0',
  '-gdwarf-5'
]

if sys_windows
  if cc.get_id() != 'msvc'
    dev_cflags_try += '-Wno-pedantic-ms-format'
  endif
else
  dev_cflags_try += '-fvisibility=hidden'
endif

foreach cf: dev_cflags_try
  if cc.has_argument(cf)
    dev_cflags += cf
  endif
  if get_option('dwarfgen') == true
    if cpp.has_argument(cf)
      dev_cppflags += cf
    endif
  endif
endforeach
dev_drmemory = get_option('drmemory')
if dev_drmemory
  foreach cf: drmemory_flags_try
    if cc.has_argument(cf)
      dev_cflags += cf
    endif
  endforeach
endif

dev_speciallibdwarfmalloc = get_option('libdwarfspecialmalloc') 
if dev_speciallibdwarfmalloc
  dev_cppflags_onlylibdwarf += '-DLIBDWARF_MALLOC'
endif

#if fs.isdir('/usr/local/include')
#  dev_cflags += '-I/usr/local/include'
#endif
#if fs.isdir('/opt/local/include')
#  dev_cflags += '-I/opt/local/include'
#endif

dwarf_link_args = []
dev_sanitize = get_option('sanitize')
if dev_sanitize
  foreach cf: sanitize_flags_try
    if cc.has_argument(cf)
      dev_cflags += cf
      dwarf_link_args += cf
    endif
  endforeach
endif

dev_decompression = get_option('decompression')

libdwarf_args = [ '-D__USE_MINGW_ANSI_STDIO=0' ]
if cc.get_id() == 'msvc'
  libdwarf_args += [ '-D_CRT_NONSTDC_NO_WARNINGS']
endif

config_dir = [include_directories('.')]

# configuration

# For msys2 regressiontesting, and only that.
dev_regressiontesting = get_option('regressiontesting')
if dev_regressiontesting 
  dev_cflags += ['-DDWREGRESSIONTEMP']
endif

buildmmap = get_option('buildmmap')
mmapresult = false

# sys/stat.h is for dwarfgen.
# sys/mman.h is for mmap() and sysconf()
header_checks = [
  'unistd.h',
  'sys/types.h',
  'malloc.h',
  'stdint.h',
  'inttypes.h',
  'stddef.h',
  'fcntl.h',
  'sys/stat.h',
  'stdafx.h',
]
if sys_windows == false
  header_checks += 'unistd.h'
endif
if buildmmap == true
  header_checks += 'sys/mman.h'
endif

config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('PROJECT_VERSION', meson.project_version())
config_h.set_quoted('PACKAGE_BIN_DIR', dir_bin)
config_h.set_quoted('PACKAGE_LIB_DIR', dir_lib)
config_h.set_quoted('PACKAGE_DATA_DIR', dir_data)
config_h.set_quoted('LOCALEDIR', dir_locale)

if host_machine.endian() == 'big'
  config_h.set10('WORDS_BIGENDIAN', true)
endif

if cc.has_function_attribute('unused') == true
  config_h.set10('HAVE_UNUSED_ATTRIBUTE', true)
endif

if cc.has_function('setlocale') == true
  if cc.has_function('nl_langinfo') == true
    config_h.set10('HAVE_UTF8', true)
  endif
endif

foreach header : header_checks
  if cc.has_header(header)
    config_h.set10('HAVE_'+header.underscorify().to_upper(), true)
  endif
endforeach

foreach t : [ 'uint64_t', 'uintptr_t', 'intptr_t' ]
  if not cc.has_type(t, prefix: '#include <stdint.h>')
    error('Sanity check failed: type @0@ not provided via stdint.h'.format(t))
  endif
endforeach

code = '''
  #include <unistd.h>
  #include <string.h>
  #include <stdlib.h>
  #include <sys/mman.h>
  int main()
  {
      long pagesize = sysconf(_SC_PAGESIZE);
      int fd = 4;
      mmap(0, 100,PROT_READ|PROT_WRITE,MAP_PRIVATE,
         fd,100);
      munmap((void *)100,100);
      return 0;
  }'''
### or use compiler.links?
if get_option('buildmmap') == true
  mmapresult = cc.compiles(code,name: 'mmap check')
  if mmapresult
    config_h.set10('HAVE_FULL_MMAP',true)
  endif
endif


# Do this early so we have the base source and build directory names
# for this and test/meson.build
project_source_base_root = meson.current_source_dir()
project_build_base_root = meson.current_build_dir()

subdir('src/lib/libdwarf')
subdir('src/bin/dwarfdump')
have_libdwarfp = false
if get_option('dwarfgen') == true
  subdir('src/lib/libdwarfp')
  subdir('src/bin/dwarfgen')
  have_libdwarfp = true
endif
subdir('src/bin/attr_form')
subdir('src/bin/buildopstab')
subdir('src/bin/builduritable')
subdir('src/bin/gennames')
subdir('src/bin/tag_attr')
subdir('src/bin/tag_tree')
subdir('src/bin/checkDW_LANG')
if get_option('dwarfexample') == true
  subdir('src/bin/dwarfexample')
endif
subdir('doc')

configure_file(
  input: 'src/bin/dwarfdump/dwarfdump.conf',
  output: 'dwarfdump.conf',
  copy: true
)

subdir('test')

# Use config_h after all subdirs have set values

#configure_file(output : 'config.h', configuration : config_h)

# pkg-config files

pkgconf = configuration_data()

pkgconf.set('prefix', get_option('prefix'))
pkgconf.set('exec_prefix', '${prefix}')
pkgconf.set('libdir', join_paths('${prefix}','@0@'.format(get_option('libdir'))))
pkgconf.set('includedir', join_paths('${prefix}','@0@'.format(get_option('includedir'))))
pkgconf.set('pkgincludedir', join_paths('${prefix}','@0@'.format(get_option('includedir')),'/libdwarf'))
pkgconf.set('VMAJ', v_maj)
pkgconf.set('PACKAGE_VERSION', meson.project_version())
pkgconf.set('PROJECT_VERSION', meson.project_version())
if dev_decompression
  pkgconf.set('requirements_libdwarf_pc', 'zlib')
  pkgconf.set('requirements_libdwarf_pc', 'libzstd')
endif

pkgconf.set('requirements_libdwarf_libs', '')

pkg_install_dir = '@0@/pkgconfig'.format(get_option('libdir'))

configure_file(
  input : join_paths(project_source_base_root, 'libdwarf.pc.in'),
  output : 'libdwarf.pc',
  configuration : pkgconf,
  install_dir : pkg_install_dir
)

# output
# Use config_h after all subdirs have set values

configure_file(output : 'config.h', configuration : config_h)

libdwarf_output = ('always')
if dev_decompression
  libdwarf_output = 'always (zlib: ' + (zlib_deps.found() ? 'yes' : 'no') + ')'
  libdwarf_output += ' always (libzstd: ' + (libzstd_deps.found() ? 'yes' : 'no') + ')'
endif

libdwarfp_output = (have_libdwarfp ? 'yes' : 'no')

summary({'OS': host_os,
         'BuildOS-BigEndian': host_machine.endian() == 'big' ? 'yes' : 'no',
         'libdwarf': libdwarf_output,
         'dwarfdump': 'always',
         'libdwarfp': libdwarfp_output,
         'dwarfgen': have_libdwarfp ? 'yes' : 'no',
         'dwarfexample': get_option('dwarfexample') ? 'yes' : 'no',
         'documentation': have_doc,
        }, section: 'Configuration Options Summary:')

summary({'zlib': (zlib_deps.found() ? 'yes' : 'no'),
         'libzstd': (libzstd_deps.found() ? 'yes' : 'no'),
        }, section: 'Referenced Libraries:')

summary({'mmap available': (mmapresult ? 'yes' : 'no'),
        }, section: 'Optional Features:')

summary({'prefix': dir_prefix,
         'bindir': dir_bin,
         'libdir': dir_lib,
         'incdir': dir_include,
         'pkgincdir': dir_pkginclude,
         'datadir': dir_data,
         'pkgdatadir': dir_pkgdata,
        }, section: 'Directories:')

summary({'compilation': 'ninja',
         'installation': 'ninja install',
         'Cflags': dev_cflags,
        }, section: 'Compilation')

