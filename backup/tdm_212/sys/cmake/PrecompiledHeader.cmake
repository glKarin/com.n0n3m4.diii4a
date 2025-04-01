#include(ucm.cmake)

macro(add_precompiled_header Target PrecompiledHeader PrecompiledSource IncludedAs)
	# gather all source files for the target
	cmake_parse_arguments(PCH "" "" "EXCLUDE" ${ARGN})
	get_target_property(Sources ${Target} SOURCES)
	# precompiled headers for C/C++ sources are different, so don't apply to .c files
	list(FILTER Sources EXCLUDE REGEX "\\.c$")
	list(FILTER Sources EXCLUDE REGEX "\\.rc$")
	ucm_remove_files(${PCH_EXCLUDE} ${PrecompiledSource} FROM Sources)

	# determine output paths
	set(PrecompiledOutputDir "${CMAKE_CURRENT_BINARY_DIR}/${Target}_pch")
	file(MAKE_DIRECTORY ${PrecompiledOutputDir})

	if(MSVC)
		get_filename_component(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
		set(PrecompiledBinary "${PrecompiledOutputDir}/${PrecompiledBasename}.pch")
		message("Setting up precompiled header for MSVC")
		set_source_files_properties(${PrecompiledSource}
				PROPERTIES COMPILE_FLAGS "/Yc\"${IncludedAs}\" /Fp\"${PrecompiledBinary}\""
				OBJECT_OUTPUTS "${PrecompiledBinary}")
		set_source_files_properties(${Sources}
				PROPERTIES COMPILE_FLAGS "/Yu\"${IncludedAs}\" /FI\"${IncludedAs}\" /Fp\"${PrecompiledBinary}\""
				OBJECT_DEPENDS "${PrecompiledBinary}")
	endif()

	if(CMAKE_COMPILER_IS_GNUCXX)
		message("Setting up precompiled header for GCC")
		# we need to use custom commands to compile the header
		# first step, copy it to the build dir
		set(CopiedPch "${PrecompiledOutputDir}/${IncludedAs}")
		set(PchDepFile "${CopiedPch}.dep")
		add_custom_command(OUTPUT "${CopiedPch}"
				COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${PrecompiledHeader}" "${CopiedPch}"
				MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${PrecompiledHeader}"
				COMMENT "Copying header")
		# second step, we need to gather the same compile flags as for the target so that the compiled header works
		set(PchFlagsFile "${PrecompiledOutputDir}/compile_flags.rsp")
		export_all_flags(${Target} "${PchFlagsFile}")
		set(CompilerFlags "@${PchFlagsFile}")
		# third step, compile the header
		get_filename_component(BaseName ${IncludedAs} NAME)
		set(CompiledPch "${PrecompiledOutputDir}/${BaseName}.gch")
		if(CMAKE_GENERATOR STREQUAL "Ninja")
			# tell G++ to produce a depfile for the Ninja generator to keep track of when to rebuild the PCH
			add_custom_command(OUTPUT "${CompiledPch}"
					COMMAND "${CMAKE_CXX_COMPILER}" ${CompilerFlags} -x c++-header -o "${CompiledPch}" "${CopiedPch}" -MD -MF "${PchDepFile}"
					DEPENDS "${CopiedPch}" "${PchFlagsFile}"
                    DEPFILE "${PchDepFile}"
					COMMENT "Precompiling header")
		else()
			# use CMake's implicit dependency scanner for Makefiles to determine when to rebuild PCH
			add_custom_command(OUTPUT "${CompiledPch}"
					COMMAND "${CMAKE_CXX_COMPILER}" ${CompilerFlags} -x c++-header -o "${CompiledPch}" "${CopiedPch}"
					DEPENDS "${CopiedPch}" "${PchFlagsFile}"
                    IMPLICIT_DEPENDS CXX "${CopiedPch}"
					COMMENT "Precompiling header")
		endif()

		SET_SOURCE_FILES_PROPERTIES(${Sources}
				PROPERTIES COMPILE_FLAGS "-include \"${CopiedPch}\" -Winvalid-pch"
				OBJECT_DEPENDS "${CompiledPch}")
	endif()
endmacro()

function(export_all_flags Target Filename)
	set(_include_directories "$<TARGET_PROPERTY:${Target},INCLUDE_DIRECTORIES>")
	set(_compile_definitions "$<TARGET_PROPERTY:${Target},COMPILE_DEFINITIONS>")
	string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _current_build_compile_flags)
	set(_compile_flags "$<TARGET_PROPERTY:${Target},COMPILE_FLAGS>" "${CMAKE_CXX_FLAGS}" "${${_current_build_compile_flags}}" "-std=c++14")
	set(_compile_options "$<TARGET_PROPERTY:${Target},COMPILE_OPTIONS>")
	set(_include_directories "$<$<BOOL:${_include_directories}>:-I$<JOIN:${_include_directories},\n-I>\n>")
	set(_compile_definitions "$<$<BOOL:${_compile_definitions}>:-D$<JOIN:${_compile_definitions},\n-D>\n>")
	set(_compile_flags "$<$<BOOL:${_compile_flags}>:$<JOIN:${_compile_flags},\n>\n>")
	set(_compile_options "$<$<BOOL:${_compile_options}>:$<JOIN:${_compile_options},\n>\n>")
	file(GENERATE OUTPUT "${Filename}" CONTENT "${_compile_definitions}${_include_directories}${_compile_flags}${_compile_options}\n")
endfunction()
