
if ( BUILD_DWARFDUMP )
  install(FILES dwarfdump.1 
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endif()

if ( BUILD_DWARFGEN )
  install(FILES dwarfgen.1  
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endif()
