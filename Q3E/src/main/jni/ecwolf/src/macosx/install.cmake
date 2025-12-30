# icns creation
find_program(ICONUTIL_EXECUTABLE iconutil DOC "Xcode iconutil binary")
add_custom_command(OUTPUT icon.icns
	COMMAND ${ICONUTIL_EXECUTABLE} -c icns -o icon.icns ${CMAKE_CURRENT_SOURCE_DIR}/macosx/icon.iconset
	COMMENT "Building icns"
	DEPENDS
		macosx/icon.iconset/icon_16x16.png
		macosx/icon.iconset/icon_16x16@2x.png
		macosx/icon.iconset/icon_32x32.png
		macosx/icon.iconset/icon_32x32@2x.png
		macosx/icon.iconset/icon_256x256.png
		macosx/icon.iconset/icon_256x256@2x.png
		macosx/icon.iconset/icon_512x512.png
		macosx/icon.iconset/icon_512x512@2x.png
)

target_sources(engine PRIVATE icon.icns)
set_target_properties(engine PROPERTIES MACOSX_BUNDLE_ICON_FILE icon.icns)
set_source_files_properties(icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

if(SDL_LIBRARY)
	string(REPLACE "-framework Cocoa" "" DIRS ${SDL_LIBRARY})
endif()

install(CODE "include(BundleUtilities)\nfixup_bundle(\"${OUTPUT_DIR}/ecwolf.app\" \"\" \"${DIRS}\")" COMPONENT Runtime)
