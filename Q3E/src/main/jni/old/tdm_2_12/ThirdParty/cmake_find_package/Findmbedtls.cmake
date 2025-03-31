include(${CMAKE_CURRENT_LIST_DIR}/tdm_find_package.cmake)

set(mbedtls_FOUND 1)
set(mbedtls_INCLUDE_DIRS "${ARTEFACTS_DIR}/mbedtls/include")
set(mbedtls_LIBRARY_DIR "${ARTEFACTS_DIR}/mbedtls/lib/${PACKAGE_PLATFORM}")
if(MSVC)
	set(mbedtls_LIBRARIES
		"${mbedtls_LIBRARY_DIR}/mbedtls.lib"
		"${mbedtls_LIBRARY_DIR}/mbedx509.lib"
		"${mbedtls_LIBRARY_DIR}/mbedcrypto.lib"
	)
else()
	set(mbedtls_LIBRARIES
		"${mbedtls_LIBRARY_DIR}/libmbedtls.a"
		"${mbedtls_LIBRARY_DIR}/libmbedx509.a"
		"${mbedtls_LIBRARY_DIR}/libmbedcrypto.a"
	)
endif()
