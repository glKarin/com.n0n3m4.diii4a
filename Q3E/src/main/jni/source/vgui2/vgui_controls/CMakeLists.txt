cmake_minimum_required(VERSION 3.9)
project (source-vgui_controls)

set(binary_name ${SOURCE_ENGINE_PREFIX}vgui_controls)

set(source_src 
		AnalogBar.cpp
		AnimatingImagePanel.cpp
		AnimationController.cpp
		BitmapImagePanel.cpp
		BuildFactoryHelper.cpp
		BuildGroup.cpp
		BuildModeDialog.cpp
		Button.cpp
		CheckButton.cpp
		CheckButtonList.cpp
		CircularProgressBar.cpp
		ComboBox.cpp
		consoledialog.cpp
		ControllerMap.cpp
		controls.cpp
		cvartogglecheckbutton.cpp
		DirectorySelectDialog.cpp
		Divider.cpp
		EditablePanel.cpp
		ExpandButton.cpp
		FileOpenDialog.cpp
		FileOpenStateMachine.cpp
		../../public/filesystem_helpers.cpp
		FocusNavGroup.cpp
		Frame.cpp
		GraphPanel.cpp
		HTML.cpp
		Image.cpp
		ImageList.cpp
		ImagePanel.cpp
		InputDialog.cpp
		KeyBindingHelpDialog.cpp
		KeyBoardEditorDialog.cpp
		KeyRepeat.cpp
		Label.cpp
		ListPanel.cpp
		ListViewPanel.cpp
		Menu.cpp
		MenuBar.cpp
		MenuButton.cpp
		MenuItem.cpp
		MessageBox.cpp
		MessageDialog.cpp
		Panel.cpp
		PanelListPanel.cpp
		PerforceFileExplorer.cpp
		PerforceFileList.cpp
		perforcefilelistframe.cpp
		ProgressBar.cpp
		ProgressBox.cpp
		PropertyDialog.cpp
		PropertyPage.cpp
		PropertySheet.cpp
		QueryBox.cpp
		RadioButton.cpp
		RichText.cpp
		RotatingProgressBar.cpp
		savedocumentquery.cpp
		ScalableImagePanel.cpp
		ScrollableEditablePanel.cpp
		ScrollBar.cpp
		ScrollBarSlider.cpp
		SectionedListPanel.cpp
		Slider.cpp
		Splitter.cpp
		subrectimage.cpp
		TextEntry.cpp
		TextImage.cpp
		ToggleButton.cpp
		Tooltip.cpp
		ToolWindow.cpp
		TreeView.cpp
		TreeViewListControl.cpp
		URLLabel.cpp
		WizardPanel.cpp
		WizardSubPanel.cpp
		../src/vgui_key_translation.cpp
)

set(source_inc 
		.
		../../public
		../../public/tier0
		../../public/tier1
)

add_library (${binary_name} STATIC ${source_src})
target_include_directories(${binary_name} PRIVATE ${source_inc})

