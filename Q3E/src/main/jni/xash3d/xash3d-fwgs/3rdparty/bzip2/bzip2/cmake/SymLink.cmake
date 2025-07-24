# Install a symlink of script to the "bin" directory.
# Not intended for use on Windows.
function(install_script_symlink original symlink)
    add_custom_command(OUTPUT ${symlink}
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${original} ${symlink}
        DEPENDS ${original}
        COMMENT "Generating symbolic link ${symlink} of ${original}")
    add_custom_target(${symlink}_tgt ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${symlink})
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${symlink} DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

# Install a symlink of binary target to the "bin" directory.
# On Windows, it will be a copy instead of a symlink.
function(install_target_symlink original symlink)
    if(WIN32)
        set(op copy)
        set(symlink "${symlink}.exe")
    else()
        set(op create_symlink)
    endif()
    add_custom_command(TARGET ${original} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E ${op} $<TARGET_FILE_NAME:${original}> ${symlink}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${original}>
        COMMENT "Generating symbolic link (or copy) ${symlink} of ${original}")
    install(PROGRAMS $<TARGET_FILE_DIR:${original}>/${symlink} DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()
