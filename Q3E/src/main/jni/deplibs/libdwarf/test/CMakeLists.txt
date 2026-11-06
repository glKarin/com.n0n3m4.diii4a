add_library(dwarf_tests INTERFACE)
target_compile_definitions(dwarf_tests INTERFACE ${DW_LIBDWARF_STATIC} _GNU_SOURCE TESTING LIBDWARF_BUILD)
target_compile_options(dwarf_tests INTERFACE ${DW_FWALL})
target_include_directories(dwarf_tests INTERFACE "${PROJECT_SOURCE_DIR}/src/lib/libdwarf" "${PROJECT_SOURCE_DIR}/src/lib/libdwarfp" "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump")

  add_executable(selfteststring
    "${PROJECT_SOURCE_DIR}/test/test_dwarfstring.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_string.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_string.h")
  target_link_libraries(selfteststring PRIVATE dwarf_tests)
  add_test(NAME selfteststring COMMAND selfteststring)

  add_executable(selftestextrastring
    "${PROJECT_SOURCE_DIR}/test/test_extra_flag_strings.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarfp/dwarf_pro_log_extra_flag_strings.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_string.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_string.h")
  target_link_libraries(selftestextrastring PRIVATE dwarf_tests)
  add_test(NAME selftestextrastring COMMAND selftestextrastring)

  add_executable(selftest_linkedtopath
    "${PROJECT_SOURCE_DIR}/test/test_linkedtopath.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_safe_strcpy.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_safe_strcpy.h"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_string.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_string.h"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_debuglink.h"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_debuglink.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_error.h")
  target_link_libraries(selftest_linkedtopath PRIVATE dwarf_tests)
  if(MINGW OR MSYS OR WIN32)
    target_link_libraries(selftest_linkedtopath PRIVATE dwarf)
  endif()
  add_test(NAME selftest_linkedtopath COMMAND selftest_linkedtopath)

  add_executable(selftestlname "${PROJECT_SOURCE_DIR}/test/test_lname.c")
  add_test(NAME selftestlname COMMAND selftestlname -f "${PROJECT_SOURCE_DIR}")

  add_executable(selfgetopttest
    "${PROJECT_SOURCE_DIR}/test/test_getopt.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_getopt.h"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_getopt.c")
  target_link_libraries(selfgetopttest PRIVATE dwarf_tests)
  add_test(NAME selfgetopttest COMMAND selfgetopttest)

  add_executable(selfmakename
    "${PROJECT_SOURCE_DIR}/test/test_makename.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_esb.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_makename.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_tsearchbal.c")
  target_link_libraries(selfmakename PRIVATE dwarf_tests)
  add_test(NAME selfmakename COMMAND selfmakename)

  add_executable(selfhelpertree
    "${PROJECT_SOURCE_DIR}/test/test_helpertree.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_helpertree.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_tsearchbal.c")
  target_link_libraries(selfhelpertree PRIVATE dwarf_tests)
  add_test(NAME selfhelpertree COMMAND selfhelpertree)

  add_executable(selfignoresec
    "${PROJECT_SOURCE_DIR}/test/test_ignoresec.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_secname_ck.c")
  target_link_libraries(selfignoresec PRIVATE dwarf_tests)
  add_test(NAME selfignoresec COMMAND selfignoresec)

  add_executable(selfmacrocheck
    "${PROJECT_SOURCE_DIR}/test/test_macrocheck.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_esb.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_tsearchbal.c")
  target_link_libraries(selfmacrocheck PRIVATE dwarf_tests)
  add_test(NAME selfmacrocheck COMMAND selfmacrocheck)

  add_executable(selftestesb
    "${PROJECT_SOURCE_DIR}/test/test_esb.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_esb.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_tsearchbal.c")
  target_link_libraries(selftestesb PRIVATE dwarf_tests)
  add_test(NAME selftestesb COMMAND selftestesb)

  add_executable(selftestsanitized
    "${PROJECT_SOURCE_DIR}/test/test_sanitized.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_esb.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_sanitized.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_utf8.c")
  target_link_libraries(selftestsanitized PRIVATE dwarf_tests)
  add_test(NAME selftestsanitized COMMAND selftestesb)

  add_executable(selfleb
    "${PROJECT_SOURCE_DIR}/test/test_dwarf_leb.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_leb.c")
  target_link_libraries(selfleb PRIVATE dwarf_tests)
  add_test(NAME selfleb COMMAND selfleb)

  add_executable(selftied
    "${PROJECT_SOURCE_DIR}/test/test_dwarf_tied.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_tied.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_tsearchhash.c")
  target_link_libraries(selftied PRIVATE dwarf_tests)
  add_test(NAME selftied COMMAND selftied)

  add_executable(selftestnames
    "${PROJECT_SOURCE_DIR}/test/test_getname.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_names.c")
  target_link_libraries(selftestnames PRIVATE dwarf_tests)
  add_test(NAME selftestnames COMMAND selftestnames)

  add_executable(selfintobj64test "${PROJECT_SOURCE_DIR}/test/test_int64_test.c")
  target_link_libraries(selfintobj64test PRIVATE dwarf_tests)
  add_test(NAME selfintobj64test COMMAND selfintobj64test -f "${PROJECT_SOURCE_DIR}")

  add_executable(selferrmsglist
    "${PROJECT_SOURCE_DIR}/test/test_errmsglist.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_safe_strcpy.c")
  target_link_libraries(selferrmsglist PRIVATE dwarf_tests)
  add_test(NAME selferrmsglist COMMAND selferrmsglist -f "${PROJECT_SOURCE_DIR}")

  add_executable(selfcanonical
    "${PROJECT_SOURCE_DIR}/test/test_canonical.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_safe_strcpy.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_canonical_append.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_esb.c")
  target_link_libraries(selfcanonical PRIVATE dwarf_tests)
  add_test(NAME selfcanonical COMMAND selfcanonical)

  add_executable(selfsetupsections
    "${PROJECT_SOURCE_DIR}/test/test_setupsections.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_setup_sections.c")
  target_link_libraries(selfsetupsections PRIVATE dwarf_tests)
  add_test(NAME selfsetupsections COMMAND selfsetupsections)

  add_executable(selfteststrcpy
    "${PROJECT_SOURCE_DIR}/test/test_safe_strcpy.c"
    "${PROJECT_SOURCE_DIR}/src/lib/libdwarf/dwarf_safe_strcpy.c")
  target_link_libraries(selfteststrcpy PRIVATE dwarf_tests)
  add_test(NAME selfteststrcpy COMMAND selfteststrcpy)

  add_executable(selfregex
    "${PROJECT_SOURCE_DIR}/test/test_regex.c"
    "${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dd_regex.c")
  target_link_libraries(selfregex PRIVATE dwarf_tests)
  add_test(NAME selfregex COMMAND selfregex)

if(NOT WIN32)
  include(FindPython3)
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    configure_file("${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dwarfdump.conf" "${PROJECT_BINARY_DIR}/test/dwarfdump.conf" @ONLY)
    configure_file("${PROJECT_SOURCE_DIR}/src/bin/dwarfdump/dwarfdump.conf" "${PROJECT_BINARY_DIR}/dwarfdump.conf" @ONLY)
    add_test(NAME selfdwarfdumppe COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/test/test_dwarfdump.py" PE cmake "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}" ${PROJECT_VERSION})
    add_test(NAME selfdwarfdumpelf COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/test/test_dwarfdump.py" Elf cmake "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}" ${PROJECT_VERSION})
    add_test(NAME selfdwarfdumpmacho COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/test/test_dwarfdump.py" Macos cmake "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}" ${PROJECT_VERSION})
  endif()
endif()

if(BUILD_DWARFEXAMPLE AND NOT WIN32)
  add_test(NAME selfjitreader COMMAND sh -c "${PROJECT_SOURCE_DIR}/test/test_jitreaderdiff.sh ${PROJECT_SOURCE_DIR}")
  add_test(NAME selfdebuglinka COMMAND sh -c "${PROJECT_SOURCE_DIR}/test/test_debuglink-a.sh ${PROJECT_SOURCE_DIR}")
  add_test(NAME selfdebuglinkb COMMAND sh -c "${PROJECT_SOURCE_DIR}/test/test_debuglink-b.sh ${PROJECT_SOURCE_DIR}")
endif()
