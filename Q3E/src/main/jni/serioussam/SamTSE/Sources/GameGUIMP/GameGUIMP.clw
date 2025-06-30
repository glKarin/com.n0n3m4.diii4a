; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
ClassCount=28
Class1=CGameApp
LastClass=CDlgSelectPlayer
NewFileInclude2=#include "Game.h"
ResourceCount=16
NewFileInclude1=#include "stdafx.h"
Resource1=IDD_MULTIPLAYER_JOIN
Class2=CEditConsole
LastTemplate=CDialog
Class3=CDlgSinglePlayerNew
Class4=CDlgSinglePlayerNewGame
Resource2=IDD_SELECT_PROVIDER_ON_JOIN
Resource3=IDD_VIDEO_QUALITY
Resource4=IDD_CREATE_PLAYER
Resource5=IDD_MULTIPLAYER_OPEN_LOCAL_PLAYERS
Resource6=IDD_SETTINGS_PLAYERS
Resource7=IDD_MULTIPLAYER_NEW
Resource8=IDD_CONSOLE
Resource9=IDD_RENAME_CONTROLS
Class5=CDlgAudioQuality
Class6=CDlgCreatePlayer
Class7=CDlgMultiplayerJoinGame
Class8=CDlgNewMultiplayerGame
Class9=CLocalPlayersList
Class10=CDlgPlayerAppearance
Class11=CDlgPlayerControls
Class12=CDlgPlayerSettings
Class13=CDlgVideoQuality
Class14=CPressKeyEditControl
Class15=CActionsListControl
Class16=CAxisListCtrl
Class17=CPressKeyWnd
Class18=CPressKeyFullScreenWnd
Class19=CDlgPressNewButtonMessage
Class20=CPlayerSettingsPlayerList
Resource10=IDD_SINGLE_PLAYER_NEW
Class21=CDlgMultiplayerOpenLocalPlayers
Resource11=IDD_SELECT_PLAYER
Class22=CDlgSelectProviderOnJoin
Class23=CDlgConsole
Resource12=IDD_AUDIO_QUALITY
Resource13=IDD_PLAYER_CONTROLS
Class24=CDlgEditButtonAction
Class25=CConsoleSymbolsCombo
Class26=CPlayerSettingsControlsList
Resource14=IDD_PLAYER_APPEARANCE
Class27=CDlgRenameControls
Resource15=IDD_EDIT_BUTTON_ACTION
Class28=CDlgSelectPlayer
Resource16=IDR_BUTTON_ACTION_POPUP

[CLS:CGameApp]
Type=0
HeaderFile=Game.h
ImplementationFile=Game.cpp
Filter=N

[DLG:IDD_SINGLE_PLAYER_NEW]
Type=1
Class=CDlgSinglePlayerNewGame
ControlCount=14
Control1=IDC_STATIC,static,1342308352
Control2=ID_AVAILABLE_PLAYERS,combobox,1344340227
Control3=IDC_STATIC,static,1342308352
Control4=IDC_DIFICULTY_LEVEL,button,1342308361
Control5=IDC_SINGLE_NORMAL,button,1342177289
Control6=IDC_SINGLE_HARD,button,1342177289
Control7=IDC_SINGLE_EXTREME,button,1342177289
Control8=IDOK,button,1342242817
Control9=IDCANCEL,button,1342242816
Control10=IDC_CHOOSE_LEVEL,button,1342242816
Control11=IDC_STATIC,static,1342308352
Control12=IDC_LEVEL_NAME,static,1342308352
Control13=IDC_STATIC,static,1342308352
Control14=ID_AVAILABLE_CONTROLS,combobox,1344340227

[CLS:CDlgSinglePlayerNew]
Type=0
HeaderFile=DlgSinglePlayerNew.h
ImplementationFile=DlgSinglePlayerNew.cpp
BaseClass=CDialog
Filter=D
LastObject=CDlgSinglePlayerNew

[CLS:CDlgSinglePlayerNewGame]
Type=0
HeaderFile=dlgsingleplayernewgame.h
ImplementationFile=dlgsingleplayernewgame.cpp
BaseClass=CDialog
LastObject=ID_AVAILABLE_PLAYERS
Filter=D
VirtualFilter=dWC

[DLG:IDD_MULTIPLAYER_NEW]
Type=1
Class=CDlgNewMultiplayerGame
ControlCount=29
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_LEVEL_NAME,static,1342308352
Control5=IDC_CHOOSE_LEVEL,button,1342242816
Control6=IDC_STATIC,static,1342308352
Control7=IDC_PROVIDERS,combobox,1344339971
Control8=IDC_STATIC,static,1342308352
Control9=IDC_SESSION_NAME,edit,1350631552
Control10=IDC_STATIC,static,1342308352
Control11=IDC_GAME_TYPE,combobox,1344339971
Control12=IDC_TEAM_PLAY,button,1342242851
Control13=IDC_STATIC,static,1342308352
Control14=IDC_MULTIPLAYER_EASY,button,1342373897
Control15=IDC_MULTIPLAYER_NORMAL,button,1342177289
Control16=IDC_MULTIPLAYER_HARD,button,1342177289
Control17=IDC_MULTIPLAYER_EXTREME,button,1342177289
Control18=IDC_STATIC,static,1342308352
Control19=IDC_FRAG_LIMIT,edit,1350639744
Control20=IDC_SPIN_FRAG_LIMIT,msctls_updown32,1342177463
Control21=IDC_STATIC,static,1342308352
Control22=IDC_TIME_LIMIT,edit,1350639744
Control23=IDC_SPIN_TIME_LIMIT,msctls_updown32,1342177463
Control24=IDC_STATIC,static,1342308353
Control25=IDC_LOCAL_PLAYERS,listbox,1352745299
Control26=IDC_STATIC,static,1342308352
Control27=IDC_MAX_PLAYERS,edit,1350639744
Control28=IDC_SPIN_MAX_PLAYERS,msctls_updown32,1342177463
Control29=IDC_DUMMY_LIST,listbox,1218511107

[DLG:IDD_VIDEO_QUALITY]
Type=1
Class=CDlgVideoQuality
ControlCount=14
Control1=IDC_STATIC,static,1342308352
Control2=IDC_TEXTURE_QUALITY,button,1342308361
Control3=IDC_TEXTURE_QUALITY_NORMAL,button,1342177289
Control4=IDC_TEXTURE_QUALITY_HIGH,button,1342177289
Control5=IDC_STATIC,static,1342308352
Control6=IDC_OBJECT_SHADOW_QUALITY,button,1342308361
Control7=IDC_OBJECT_SHADOW_QUALITY_LOW,button,1342177289
Control8=IDC_OBJECT_SHADOW_QUALITY_HIGH,button,1342177289
Control9=IDC_STATIC,static,1342308352
Control10=IDC_WORLD_SHADOW_QUALITY,button,1342308361
Control11=IDC_WORLD_SHADOW_QUALITY_NORMAL,button,1342177289
Control12=IDC_WORLD_SHADOW_QUALITY_HIGH,button,1342177289
Control13=IDOK,button,1342242817
Control14=IDCANCEL,button,1342242816

[DLG:IDD_AUDIO_QUALITY]
Type=1
Class=CDlgAudioQuality
ControlCount=7
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=65535,static,1342308352
Control4=IDC_AUDIO_QUALITY_LOW,button,1342308361
Control5=IDC_AUDIO_QUALITY_NORMAL,button,1342177289
Control6=IDC_AUDIO_QUALITY_HIGH,button,1342177289
Control7=IDC_USE_DSOUND3D,button,1342242851

[DLG:IDD_MULTIPLAYER_JOIN]
Type=1
Class=CDlgMultiplayerJoinGame
ControlCount=7
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_SESSION_LIST,listbox,1352728835
Control4=IDC_LOCAL_PLAYERS,listbox,1352745299
Control5=IDC_STATIC,static,1342308353
Control6=IDC_STATIC,static,1342308353
Control7=IDC_DUMMY_LIST,listbox,1218511107

[DLG:IDD_SETTINGS_PLAYERS]
Type=1
Class=CDlgPlayerSettings
ControlCount=9
Control1=IDC_RENAME_PLAYER,button,1342242816
Control2=IDC_PLAYER_APPEARANCE,button,1342242816
Control3=IDC_AVAILABLE_PLAYERS,listbox,1352728833
Control4=IDC_RENAME_CONTROLS,button,1342242816
Control5=IDC_EDIT_CONTROLS,button,1342242816
Control6=IDC_AVAILABLE_CONTROLS,listbox,1352728833
Control7=IDC_STATIC,static,1342308353
Control8=IDOK,button,1342242817
Control9=IDC_STATIC,static,1342308353

[DLG:IDD_CREATE_PLAYER]
Type=1
Class=CDlgCreatePlayer
ControlCount=4
Control1=IDC_EDIT_PLAYER_NAME,edit,1350631552
Control2=IDOK,button,1342242817
Control3=IDCANCEL,button,1342242816
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_PLAYER_CONTROLS]
Type=1
Class=CDlgPlayerControls
ControlCount=27
Control1=IDC_BUTTON_ACTIONS_LIST,SysListView32,1350664197
Control2=IDOK,button,1342242817
Control3=IDC_PRESS_MESSAGE,static,1342308353
Control4=IDC_STATIC,static,1342308353
Control5=IDC_EDIT_FIRST_CONTROL,edit,1484783744
Control6=ID_FIRST_CONTROL_NONE,button,1476395008
Control7=IDC_STATIC,static,1342308353
Control8=IDC_EDIT_SECOND_CONTROL,edit,1484783744
Control9=ID_SECOND_CONTROL_NONE,button,1476395008
Control10=ID_MOVE_CONTROL_DOWN,button,1342243584
Control11=ID_MOVE_CONTROL_UP,button,1342242816
Control12=ID_BUTTON_ACTION_EDIT,button,1342242816
Control13=ID_BUTTON_ACTION_ADD,button,1342242816
Control14=ID_BUTTON_ACTION_REMOVE,button,1342242816
Control15=IDC_AXIS_ACTIONS_LIST,SysListView32,1350664197
Control16=IDC_STATIC,static,1342308353
Control17=IDC_CONTROLER_AXIS,combobox,1478557699
Control18=IDC_INVERT_CONTROLER,button,1476460579
Control19=IDC_CONTROLER_TYPE_T,static,1476526080
Control20=IDC_RELATIVE_ABSOLUTE_TYPE,button,1476526089
Control21=IDC_ABSOLUTE_RADIO,button,1476395017
Control22=IDC_CONTROLER_SENSITIVITY_T,static,1476526081
Control23=IDC_CONTROLER_SENSITIVITY,msctls_trackbar32,1476460545
Control24=ID_DEFAULT,button,1342242816
Control25=IDCANCEL,button,1342242816
Control26=IDC_STATIC,button,1342178055
Control27=IDC_STATIC,button,1342178055

[DLG:IDD_PLAYER_APPEARANCE]
Type=1
Class=CDlgPlayerAppearance
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_AVAILABLE_APPEARANCES,combobox,1344340227
Control4=IDC_STATIC,static,1342308352

[CLS:CDlgAudioQuality]
Type=0
HeaderFile=DlgAudioQuality.h
ImplementationFile=DlgAudioQuality.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC

[CLS:CDlgCreatePlayer]
Type=0
HeaderFile=DlgCreatePlayer.h
ImplementationFile=DlgCreatePlayer.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC

[CLS:CDlgMultiplayerJoinGame]
Type=0
HeaderFile=DlgMultiplayerJoinGame.h
ImplementationFile=DlgMultiplayerJoinGame.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CDlgMultiplayerJoinGame

[CLS:CDlgNewMultiplayerGame]
Type=0
HeaderFile=DlgNewMultiplayerGame.h
ImplementationFile=DlgNewMultiplayerGame.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CDlgNewMultiplayerGame

[CLS:CLocalPlayersList]
Type=0
HeaderFile=LocalPlayersList.h
ImplementationFile=LocalPlayersList.cpp
BaseClass=CListBox
Filter=W

[CLS:CDlgPlayerAppearance]
Type=0
HeaderFile=DlgPlayerAppearance.h
ImplementationFile=DlgPlayerAppearance.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC

[CLS:CDlgPlayerControls]
Type=0
HeaderFile=DlgPlayerControls.h
ImplementationFile=DlgPlayerControls.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_CONTROLER_SENSITIVITY_T

[CLS:CDlgPlayerSettings]
Type=0
HeaderFile=DlgPlayerSettings.h
ImplementationFile=DlgPlayerSettings.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_RENAME_PLAYER

[CLS:CDlgVideoQuality]
Type=0
HeaderFile=DlgVideoQuality.h
ImplementationFile=DlgVideoQuality.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC

[CLS:CPressKeyEditControl]
Type=0
HeaderFile=PressKeyEditControl.h
ImplementationFile=PressKeyEditControl.cpp
BaseClass=CEdit
Filter=W
LastObject=CPressKeyEditControl
VirtualFilter=WC

[CLS:CActionsListControl]
Type=0
HeaderFile=ActionsListControl.h
ImplementationFile=ActionsListControl.cpp
BaseClass=CListCtrl
Filter=W
VirtualFilter=FWC
LastObject=CActionsListControl

[CLS:CAxisListCtrl]
Type=0
HeaderFile=AxisListCtrl.h
ImplementationFile=AxisListCtrl.cpp
BaseClass=CListCtrl
Filter=W
VirtualFilter=FWC

[CLS:CPressKeyWnd]
Type=0
HeaderFile=PressKeyWnd.h
ImplementationFile=PressKeyWnd.cpp
BaseClass=generic CWnd
Filter=W
LastObject=CPressKeyWnd

[CLS:CPressKeyFullScreenWnd]
Type=0
HeaderFile=PressKeyFullScreenWnd.h
ImplementationFile=PressKeyFullScreenWnd.cpp
BaseClass=generic CWnd
Filter=W
LastObject=CPressKeyFullScreenWnd

[CLS:CDlgPressNewButtonMessage]
Type=0
HeaderFile=DlgPressNewButtonMessage.h
ImplementationFile=DlgPressNewButtonMessage.cpp
BaseClass=CDialog
Filter=D
LastObject=CDlgPressNewButtonMessage
VirtualFilter=dWC

[CLS:CPlayerSettingsPlayerList]
Type=0
HeaderFile=PlayerSettingsPlayerList.h
ImplementationFile=PlayerSettingsPlayerList.cpp
BaseClass=CListBox
Filter=W
VirtualFilter=bWC

[DLG:IDD_MULTIPLAYER_OPEN_LOCAL_PLAYERS]
Type=1
Class=CDlgMultiplayerOpenLocalPlayers
ControlCount=5
Control1=IDC_LOCAL_PLAYERS,listbox,1352745299
Control2=IDOK,button,1342242817
Control3=IDCANCEL,button,1342242816
Control4=IDC_STATIC,static,1342308353
Control5=IDC_DUMMY_LIST,listbox,1218511107

[CLS:CDlgMultiplayerOpenLocalPlayers]
Type=0
HeaderFile=DlgMultiplayerOpenLocalPlayers.h
ImplementationFile=DlgMultiplayerOpenLocalPlayers.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_AVAILABLE_PROVIDERS

[DLG:IDD_SELECT_PROVIDER_ON_JOIN]
Type=1
Class=CDlgSelectProviderOnJoin
ControlCount=3
Control1=IDC_AVAILABLE_PROVIDERS,listbox,1352728835
Control2=IDOK,button,1342242817
Control3=IDCANCEL,button,1342242816

[CLS:CDlgSelectProviderOnJoin]
Type=0
HeaderFile=DlgSelectProviderOnJoin.h
ImplementationFile=DlgSelectProviderOnJoin.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC

[CLS:CEditConsole]
Type=0
HeaderFile=EditConsole.h
ImplementationFile=EditConsole.cpp
BaseClass=CEdit
Filter=W
LastObject=CEditConsole
VirtualFilter=WC

[DLG:IDD_CONSOLE]
Type=1
Class=CDlgConsole
ControlCount=4
Control1=IDC_CONSOLE_INPUT,edit,1353781444
Control2=IDC_CONSOLE_OUTPUT,edit,1353779396
Control3=IDC_STATIC,static,1342308352
Control4=IDC_CONSOLE_SYMBOLS,combobox,1344340227

[CLS:CDlgConsole]
Type=0
HeaderFile=DlgConsole.h
ImplementationFile=DlgConsole.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CDlgConsole

[MNU:IDR_BUTTON_ACTION_POPUP]
Type=1
Class=?
Command1=ID_BUTTON_ACTION_EDIT
Command2=ID_BUTTON_ACTION_ADD
Command3=ID_BUTTON_ACTION_REMOVE
CommandCount=3

[DLG:IDD_EDIT_BUTTON_ACTION]
Type=1
Class=CDlgEditButtonAction
ControlCount=8
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_BUTTON_ACTION_NAME,edit,1350631552
Control5=IDC_STATIC,static,1342308352
Control6=IDC_BUTTON_DOWN_COMMAND,edit,1350631556
Control7=IDC_STATIC,static,1342308352
Control8=IDC_BUTTON_UP_COMMAND,edit,1350631556

[CLS:CDlgEditButtonAction]
Type=0
HeaderFile=DlgEditButtonAction.h
ImplementationFile=DlgEditButtonAction.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC

[CLS:CConsoleSymbolsCombo]
Type=0
HeaderFile=ConsoleSymbolsCombo.h
ImplementationFile=ConsoleSymbolsCombo.cpp
BaseClass=CComboBox
Filter=W
LastObject=CConsoleSymbolsCombo
VirtualFilter=cWC

[CLS:CPlayerSettingsControlsList]
Type=0
HeaderFile=PlayerSettingsControlsList.h
ImplementationFile=PlayerSettingsControlsList.cpp
BaseClass=CListBox
Filter=W
VirtualFilter=bWC
LastObject=CPlayerSettingsControlsList

[DLG:IDD_RENAME_CONTROLS]
Type=1
Class=CDlgRenameControls
ControlCount=4
Control1=IDC_EDIT_CONTROLS_NAME,edit,1350631552
Control2=IDOK,button,1342242817
Control3=IDCANCEL,button,1342242816
Control4=IDC_STATIC,static,1342308352

[CLS:CDlgRenameControls]
Type=0
HeaderFile=DlgRenameControls.h
ImplementationFile=DlgRenameControls.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CDlgRenameControls

[DLG:IDD_SELECT_PLAYER]
Type=1
Class=CDlgSelectPlayer
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_COMBO_AVAILABLE_PLAYERS,combobox,1344339971
Control5=IDC_STATIC,static,1342308352
Control6=IDC_COMBO_AVAILABLE_CONTROLS,combobox,1344339971

[CLS:CDlgSelectPlayer]
Type=0
HeaderFile=DlgSelectPlayer.h
ImplementationFile=DlgSelectPlayer.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_COMBO_AVAILABLE_CONTROLS

