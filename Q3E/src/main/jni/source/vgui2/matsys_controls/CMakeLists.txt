cmake_minimum_required(VERSION 3.9)
project (source-matsys_controls)

set(binary_name ${SOURCE_ENGINE_PREFIX}matsys_controls)

set(source_src 
		assetpicker.cpp
		baseassetpicker.cpp
		colorpickerpanel.cpp
		curveeditorpanel.cpp
		gamefiletreeview.cpp
		manipulator.cpp
		matsyscontrols.cpp
		mdlpanel.cpp
		mdlpicker.cpp
		mdlsequencepicker.cpp
		picker.cpp
		potterywheelpanel.cpp
		proceduraltexturepanel.cpp
		QCGenerator.cpp
		sequencepicker.cpp
		tgapreviewpanel.cpp
		vmtpicker.cpp
		vmtpreviewpanel.cpp
		vtfpicker.cpp
		vtfpreviewpanel.cpp
		vmtpanel.cpp
)

set(source_inc 
		.
		../../public
		../../public/tier0
		../../public/tier1
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

