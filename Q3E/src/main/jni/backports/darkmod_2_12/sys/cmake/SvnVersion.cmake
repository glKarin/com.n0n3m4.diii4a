macro(get_svn_revision OutVar)
	message("Determining SVN revision")
	find_program(SVNVERSION_PROGRAM NAMES svnversion)
	if (SVNVERSION_PROGRAM)
		execute_process(COMMAND "${SVNVERSION_PROGRAM}" "-c"
				WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
				RESULT_VARIABLE SVNVERSION_EXITCODE
				OUTPUT_VARIABLE ${OutVar}
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		if (NOT SVNVERSION_EXITCODE EQUAL 0)
			message("svnversion failed")
			set(${OutVar} "NOTFOUND")
		endif()
	else()
		message("svnversion not found")
		set(${OutVar} "NOTFOUND")
	endif()
endmacro()
