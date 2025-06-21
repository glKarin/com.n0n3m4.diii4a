if(WIN32)
	# Windows XP compatible platform toolset. Must be set before project(),
	# otherwise change of CMAKE_*_TOOLSET will take no effect.
	# We get VS version from the generator name because neither MSVC* nor other
	# variables that describe the compiler aren't available before project().
	if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio ([0-9]+)")
		if(${CMAKE_MATCH_1} LESS 11)
			# Nothing. Older VS does support XP by default.
		elseif(${CMAKE_MATCH_1} EQUAL 11)
			# Visual Studio 11 2012
			set(CMAKE_GENERATOR_TOOLSET "v110_xp" CACHE STRING "CMAKE_GENERATOR_TOOLSET" FORCE)
			set(CMAK_VS_PLATFORM_TOOLSET "v110_xp" CACHE STRING "CMAKE_VS_PLATFORM_TOOLSET" FORCE)
		elseif (${CMAKE_MATCH_1} EQUAL 12)
			# Visual Studio 12 2013
			set(CMAKE_GENERATOR_TOOLSET "v120_xp" CACHE STRING "CMAKE_GENERATOR_TOOLSET" FORCE)
			set(CMAKE_VS_PLATFORM_TOOLSET "v120_xp" CACHE STRING "CMAKE_VS_PLATFORM_TOOLSET" FORCE)
		elseif (${CMAKE_MATCH_1} EQUAL 14)
			# Visual Studio 14 2015
			set(CMAKE_GENERATOR_TOOLSET "v140_xp" CACHE STRING "CMAKE_GENERATOR_TOOLSET" FORCE)
			set(CMAKE_VS_PLATFORM_TOOLSET "v140_xp" CACHE STRING "CMAKE_VS_PLATFORM_TOOLSET" FORCE)
		elseif (${CMAKE_MATCH_1} EQUAL 15)
			# Visual Studio 15 2017
			set(CMAKE_GENERATOR_TOOLSET "v141_xp" CACHE STRING "CMAKE_GENERATOR_TOOLSET" FORCE)
			set(CMAKE_VS_PLATFORM_TOOLSET "v141_xp" CACHE STRING "CMAKE_VS_PLATFORM_TOOLSET" FORCE)
		else()
			message(WARNING "WARNING: You maybe building without Windows XP compability. See which toolchain version Visual Studio provides, and say cmake to use it: cmake -G \"Visual Studio XX\" -T \"vXXX_xp\"")
		endif()
	endif()
endif()
