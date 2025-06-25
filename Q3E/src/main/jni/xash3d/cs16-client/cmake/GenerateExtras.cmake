# ugly and not performant but cmake is what it is
if(NOT ANDROID)
	add_custom_command(OUTPUT ${EXTRAS_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${EXTRAS_DIR}")

	add_custom_target(pack_extras ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cs16client-extras" "${EXTRAS_DIR}"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/yapb/cfg" "${EXTRAS_DIR}"
		COMMAND ${CMAKE_COMMAND} -E tar cf "${EXTRAS_DIR}.pk3" --format=zip .
		WORKING_DIRECTORY "${EXTRAS_DIR}"
		DEPENDS "${EXTRAS_DIR}")

	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/extras.pk3" DESTINATION "${GAME_DIR}/")

	# copy pre-generated config files for psvita
	if(VITA)
		install(FILES 
			"${CMAKE_CURRENT_BINARY_DIR}/config.cfg" 
			"${CMAKE_CURRENT_BINARY_DIR}/video.cfg" 
			"${CMAKE_CURRENT_BINARY_DIR}/opengl.cfg" 
			DESTINATION "${GAME_DIR}/")

		install(FILES 
			"${CMAKE_CURRENT_BINARY_DIR}/kb_def.lst" 
			DESTINATION "${GAME_DIR}/gfx/shell")
	endif()
endif()