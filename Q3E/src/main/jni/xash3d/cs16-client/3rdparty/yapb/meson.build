#
# YaPB - Counter-Strike Bot based on PODBot by Markus Klinge.
# Copyright © YaPB Project Developers <yapb@jeefo.net>.
#
# SPDX-License-Identifier: MIT
#

# version is now passed into the bot dll
project(
   'yapb',
   'cpp',
   version: '4.5',
   license: 'MIT',
   default_options: [
      'buildtype=release',
      'b_ndebug=if-release',
      'cpp_std=c++17',
      'warning_level=3',
      'werror=true',
      'backend=ninja',
      'optimization=3',
      'default_library=static',
      'cpp_eh=none',
      'cpp_rtti=false',
      'b_vscrt=static_from_buildtype',
      'b_lto=true',
      'b_lto_mode=default',
      'b_lundef=true',
   ],
   meson_version: '>=0.58.0')

find_program('ninja', required: true)

compiler= meson.get_compiler('cpp')
cross = meson.is_cross_build ()
os = host_machine.system()
cpu = host_machine.cpu_family()
cxx = compiler.get_id()
build_type = get_option ('buildtype')

opt_64bit = get_option('64bit')
opt_native = get_option('native')
opt_winxp = get_option('winxp')
opt_nosimd = get_option('nosimd')
opt_static_linkent = get_option('static_linkent')

# cpp and ldflags from scratch
cxxflags = []
ldflags = []

# custom flags
flags_versioned = ['-DVERSION_GENERATED']

# git is optional, but it's setups our version info
git = find_program('git', required: false)

if git.found()
   run_command('git', 'config', '--global', '--add', 'safe.directory', '*', check: false)

   # get the commit data
   count = run_command('git', 'rev-list', '--count', 'HEAD', check: false).stdout().strip()
   hash = run_command ('git', 'rev-parse', '--short', 'HEAD', check: false).stdout().strip()
   author = run_command ('git', 'log', '--pretty="%ae"', '-1', check: false).stdout().strip()

   hostname = find_program('hostname', required: false)

   if hostname.found()
      machine = run_command('hostname',  check: false).stdout().strip()
   else
      machine = 'unknown'
   endif

   bot_version = meson.project_version()
   cxx_version = compiler.version()

   if os == 'windows'
      winver = ','.join(bot_version.split('.'))
   else
      winver = bot_version
   endif

   cxxflags += flags_versioned
   version_data = configuration_data()

   version_data.set('BUILD_COUNT',  count)
   version_data.set('BUILD_HASH', hash)
   version_data.set('BUILD_AUTHOR', author)
   version_data.set('BUILD_MACHINE', machine)
   version_data.set('BUILD_VERSION', bot_version)
   version_data.set('BUILD_WINVER', winver)
   version_data.set('BUILD_COMPILER', '@0@ @1@'.format(cxx, cxx_version))

   configure_file(input: 'inc/version.h.in', output: 'version.build.h', configuration: version_data)
endif

# define crlib native build
if opt_native
   cxxflags += ['-DCR_NATIVE_BUILD']
endif

# globally disables all simd optimizations
if opt_nosimd
   cxxflags += ['-DCR_DISABLE_SIMD']
endif

# configure flags gcc and clang
if cxx == 'clang' or cxx == 'gcc'
   cxxflags += [
      '-pipe',
      '-fno-threadsafe-statics',
      '-pthread'
   ]

   if not opt_native and cpu != 'arm' and not cpu.startswith('ppc')
      cxxflags += '-mtune=generic'
   endif

   if cpu == 'aarch64'
      cxxflags += [
         '-march=armv8-a+fp+simd', 
      ]
   elif cpu != 'arm' and not cpu.startswith('ppc')
      if not opt_nosimd
         cxxflags += [
            '-msse', '-msse2', '-msse3', '-msse3', '-mfpmath=sse'
         ]
      endif
      
      if opt_native
         cxxflags += '-march=native'
      elif opt_64bit
         cxxflags += '-march=x86-64'
      else
         cxxflags += '-march=i686'
      endif
   endif
   
   cxx_version = compiler.version()

   # setup optimization flags
   if build_type == 'release'
      cxxflags += [
         '-funroll-loops', '-fomit-frame-pointer', '-fno-stack-protector', '-fvisibility=hidden', '-fvisibility-inlines-hidden', '-fno-math-errno'
      ]

      if os != 'darwin' and os != 'windows' and cpu != 'aarch64' and cpu != 'arm' and not cpu.startswith('ppc')
         if not opt_static_linkent
            cxxflags += [
               '-fdata-sections',
               '-ffunction-sections',
               '-fcf-protection=none',
               '-fno-plt'
            ]

            ldflags += [
               '-Wl,--version-script=' + meson.project_source_root() + '/ext/ldscripts/version.lds',
               '-Wl,--gc-sections'
            ]
         else
            cxxflags += ['-DLINKENT_STATIC']
         endif
      
         if cxx == 'gcc'
            cxxflags += [
               '-fgraphite-identity', '-floop-nest-optimize'
            ]
            ldflags += [
               '-fgraphite-identity', '-floop-nest-optimize'
            ]
         endif
      endif

      # disable lto partitioning on gcc to get symver working 
      if os != 'darwin' and os != 'windows' and cxx == 'gcc' and cxx_version.version_compare('<11.0')
         ldflags += [
            '-flto-partition=none'
         ]
      endif
   else
      cxxflags += ['-g3', '-ggdb', '-DDEBUG', '-D_FORTIFY_SOURCE=2']
   endif

   # special script for mingw-64 builds
   if os == 'windows' and cxx == 'gcc' and cxx_version.version_compare('<12.0')
      ldflags += [
         '-Xlinker', '--script', '-Xlinker', meson.project_source_root() + '/ext/ldscripts/i386pe.lds'
      ]
   endif

   # always statically link libgcc on non darwin platforms
   if os != 'darwin'
      if cross or (cxx != 'clang' and os == 'windows')
         ldflags += '-static-libgcc'
      endif
   else
      cxxflags += '-mmacosx-version-min=10.9'

      ldflags += [
         '-lstdc++', '-mmacosx-version-min=10.9'
      ]
   endif

   # by default we buid 32bit binaries
   if not opt_64bit and cpu != 'aarch64' and cpu != 'arm' and not cpu.startswith('ppc')
      cxxflags += '-m32'
      ldflags += '-m32'

      if cross and cxx == 'clang' and os == 'windows'
         ldflags += '-Wl,/MACHINE:X86'
         cxxflags += '-Wl,/MACHINE:X86'
      endif
   else
      cxxflags += '-fPIC'
      ldflags += '-fPIC'
   endif

   # link needed libraries
   if os == 'linux'
      ldflags += [
         '-lm', '-ldl', '-lpthread'
      ]
   elif os == 'windows'
      if cxx == 'gcc' or (cross and cxx == 'clang')
         ldflags += '-Wl,--kill-at'
      endif

      ldflags += [
         '-luser32', '-lws2_32'
      ]
   endif
elif os == 'windows' and (cxx =='msvc' or cxx == 'clang-cl')

   # define for building on winxp on msvc
   if opt_winxp
      cxxflags += [
         '/TP', '/DCR_HAS_WINXP_SUPPORT', '/D_WIN32_WINNT=0x0501', '/D_USING_V110_SDK71_'
      ]
      ldflags += ['/SUBSYSTEM:WINDOWS,5.01']
   endif

   if not opt_64bit and cxx == 'clang'
      cxxflags += '/MACHINE:X86'
      ldflags += '/MACHINE:X86'
   endif

   # minor optimizations for release build
   if build_type == 'release'
      cxxflags += [
         '/Zc:threadSafeInit-', '/GS-', '/Ob2', '/Oy', '/Oi', '/Ot', '/fp:precise', '/GF', '/Gw', '/arch:SSE2', '/Zi', '/guard:ehcont-', '/guard:cf-'
      ]

      # add wpo if msvc
      if cxx == 'msvc'
         cxxflags += [
            '/GL', '/DEBUG'
         ]
      endif

      # add linker flags
      ldflags += [
         '/OPT:REF,ICF', '/GUARD:NO ', '/LTCG', 'delayimp.lib', '/DELAYLOAD:user32.dll', '/DELAYLOAD:ws2_32.dll',
      ]
   endif
   
   ldflags += [
      'user32.lib', 'ws2_32.lib'
   ]
endif

# pass our hell to meson
add_global_arguments(cxxflags, language: 'cpp')
add_global_link_arguments(ldflags, language: 'cpp')

# add the sources
sources = files(
   'src/analyze.cpp',
   'src/botlib.cpp',
   'src/chatlib.cpp',
   'src/combat.cpp',
   'src/config.cpp',
   'src/control.cpp',
   'src/engine.cpp',
   'src/fakeping.cpp',
   'src/graph.cpp',
   'src/hooks.cpp',
   'src/linkage.cpp',
   'src/manager.cpp',
   'src/module.cpp',
   'src/message.cpp',
   'src/navigate.cpp',
   'src/planner.cpp',
   'src/practice.cpp',
   'src/sounds.cpp',
   'src/storage.cpp',
   'src/support.cpp',
   'src/tasks.cpp',
   'src/vision.cpp',
   'src/vistable.cpp'
)

# add the include directories
includes = include_directories([
   '.', 'inc', 'ext', 'ext/crlib', 'ext/linkage'
], is_system: true)

# if have git and on windows add windows-specific version info
if os == 'windows' and git.found()
   sources += import('windows').compile_resources(
      'vc/yapb.rc', 
      include_directories: includes, 
      args: flags_versioned
   )
endif

target_name = meson.project_name()

# xash specific postfix for binaries
if cpu == 'aarch64'
    target_name += '_arm64'
elif opt_64bit
    target_name += '_amd64'
endif

# instruct meson we're want our little shared lib bot
shared_library(
   target_name, 
   sources,
   include_directories: includes,
   gnu_symbol_visibility: 'hidden',
   name_prefix: ''
)

run_target('package',
   command: ['python3', meson.project_source_root() + '/package.py', '@0@.@1@'.format(bot_version, count)])
