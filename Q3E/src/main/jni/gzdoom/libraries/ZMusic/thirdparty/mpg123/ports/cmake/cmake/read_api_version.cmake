function(read_api_version project_version api_version outapi_version synapi_version )

    file( READ "${CMAKE_SOURCE_DIR}/../../configure.ac" configure_ac )

    string( REGEX MATCH "AC_INIT\\(\\[mpg123\\], \\[([0-9\\.]+)" result ${configure_ac} )
    set( ${project_version} ${CMAKE_MATCH_1} PARENT_SCOPE )

    string( REGEX MATCH "API_VERSION=([0-9]+)" result ${configure_ac} )
    set( ${api_version} ${CMAKE_MATCH_1} PARENT_SCOPE )

    string( REGEX MATCH "OUTAPI_VERSION=([0-9]+)" result ${configure_ac} )
    set( ${outapi_version} ${CMAKE_MATCH_1} PARENT_SCOPE )

    string( REGEX MATCH "SYNAPI_VERSION=([0-9]+)" result ${configure_ac} )
    set( ${synapi_version} ${CMAKE_MATCH_1} PARENT_SCOPE )

endfunction()
