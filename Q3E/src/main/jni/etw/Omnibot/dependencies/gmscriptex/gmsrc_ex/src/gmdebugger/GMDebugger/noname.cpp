///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb  1 2007)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP

#include "noname.h"

#include "WxToolButton1_XPM.xpm"
#include "WxToolButton2_XPM.xpm"
#include "WxToolButton3_XPM.xpm"
#include "WxToolButton4_XPM.xpm"
#include "WxToolButton5_XPM.xpm"
#include "WxToolButton6_XPM.xpm"
#include "WxToolButton7_XPM.xpm"
#include "WxToolButton8_XPM.xpm"
#include "WxToolButton9_XPM.xpm"

///////////////////////////////////////////////////////////////////////////
BEGIN_EVENT_TABLE( GMDebuggerFrame, wxFrame )
	EVT_TEXT_ENTER( ID_DEBUGTEXTIN, GMDebuggerFrame::_wxFB_OnDebugTextInput )
END_EVENT_TABLE()

GMDebuggerFrame::GMDebuggerFrame( wxWindow* parent, int id, wxString title, wxPoint pos, wxSize size, int style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	DebuggerMenuBar = new wxMenuBar( 0 );
	wxMenu* m_FileMenu;
	m_FileMenu = new wxMenu();
	wxMenuItem* FileOpen = new wxMenuItem( m_FileMenu, ID_FILEOPEN, wxString( wxT("Open") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( FileOpen );
	wxMenuItem* FileSave = new wxMenuItem( m_FileMenu, ID_FILESAVE, wxString( wxT("Save") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( FileSave );
	wxMenuItem* FileSaveAs = new wxMenuItem( m_FileMenu, ID_FILESAVEAS, wxString( wxT("Save As") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( FileSaveAs );
	wxMenuItem* Connect = new wxMenuItem( m_FileMenu, ID_CONNECT, wxString( wxT("Connect") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( Connect );
	wxMenuItem* Close = new wxMenuItem( m_FileMenu, ID_CLOSE, wxString( wxT("Close") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( Close );
	DebuggerMenuBar->Append( m_FileMenu, wxT("File") );
	wxMenu* m_EditMenu;
	m_EditMenu = new wxMenu();
	DebuggerMenuBar->Append( m_EditMenu, wxT("Edit") );
	wxMenu* m_DebugMenu;
	m_DebugMenu = new wxMenu();
	DebuggerMenuBar->Append( m_DebugMenu, wxT("Debug") );
	wxMenu* m_HelpMenu;
	m_HelpMenu = new wxMenu();
	DebuggerMenuBar->Append( m_HelpMenu, wxT("Help") );
	this->SetMenuBar( DebuggerMenuBar );
	
	m_DebuggerStatusBar = this->CreateStatusBar( 3, wxST_SIZEGRIP, ID_DEBUGGERSTATUSBAR );
	m_DebuggerToolbar = this->CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL, ID_DEBUGGERTOOLBAR ); 
	m_DebuggerToolbar->AddTool( ID_STEPINTO, wxT("tool"), wxBitmap( WxToolButton1_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->AddTool( ID_STEPOVER, wxT("tool"), wxBitmap( WxToolButton2_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->AddTool( ID_STEPOUT, wxT("tool"), wxBitmap( WxToolButton3_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->AddTool( ID_RESUMECURRENT, wxT("tool"), wxBitmap( WxToolButton4_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->AddTool( ID_STOPDEBUGGING, wxT("tool"), wxBitmap( WxToolButton5_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->AddTool( ID_TOGGLEBREAKPOINT, wxT("tool"), wxBitmap( WxToolButton6_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->AddTool( ID_BREAKALL, wxT("tool"), wxBitmap( WxToolButton7_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->AddTool( ID_RESUMEALL, wxT("tool"), wxBitmap( WxToolButton8_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->AddTool( ID_BREAKCURRENT, wxT("tool"), wxBitmap( WxToolButton9_XPM_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_DebuggerToolbar->Realize();
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	bSizer4->SetMinSize(wxSize( 425,500 )); 
	m_CodeView = new wxScintilla( this, ID_CODEVIEW, wxDefaultPosition, wxSize( -1,-1 ), 0, wxEmptyString );
	m_CodeView->SetUseTabs( true );
	m_CodeView->SetTabWidth( 4 );
	m_CodeView->SetIndent( 4 );
	m_CodeView->SetTabIndents( false );
	m_CodeView->SetBackSpaceUnIndents( true );
	m_CodeView->SetViewEOL( false );
	m_CodeView->SetViewWhiteSpace( false );
	m_CodeView->SetMarginWidth( 2, 0 );
	m_CodeView->SetIndentationGuides( true );
	m_CodeView->SetMarginType( 1, wxSCI_MARGIN_SYMBOL );
	m_CodeView->SetMarginMask( 1, wxSCI_MASK_FOLDERS );
	m_CodeView->SetMarginWidth( 1, 16);
	m_CodeView->SetMarginSensitive( 1, true );
	m_CodeView->SetProperty( wxT("fold"), wxT("1") );
	m_CodeView->SetFoldFlags( wxSCI_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSCI_FOLDFLAG_LINEAFTER_CONTRACTED );
	m_CodeView->SetMarginType( 0, wxSCI_MARGIN_NUMBER );
	m_CodeView->SetMarginWidth( 0, m_CodeView->TextWidth( wxSCI_STYLE_LINENUMBER, wxT("_99999") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDER, wxSCI_MARK_BOXPLUS );
	m_CodeView->MarkerSetBackground( wxSCI_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( wxSCI_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDEROPEN, wxSCI_MARK_BOXMINUS );
	m_CodeView->MarkerSetBackground( wxSCI_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( wxSCI_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDERSUB, wxSCI_MARK_EMPTY );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDEREND, wxSCI_MARK_BOXPLUS );
	m_CodeView->MarkerSetBackground( wxSCI_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( wxSCI_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDEROPENMID, wxSCI_MARK_BOXMINUS );
	m_CodeView->MarkerSetBackground( wxSCI_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( wxSCI_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDERMIDTAIL, wxSCI_MARK_EMPTY );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDERTAIL, wxSCI_MARK_EMPTY );
	m_CodeView->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_CodeView->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_CodeView->SetMinSize( wxSize( 450,450 ) );
	
	bSizer4->Add( m_CodeView, 0, wxALIGN_LEFT|wxALIGN_TOP|wxALL|wxEXPAND, 0 );
	
	m_DebugTextOut = new wxTextCtrl( this, ID_DEBUGTEXTOUT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH|wxTE_RICH2 );
	bSizer4->Add( m_DebugTextOut, 2, wxALIGN_LEFT|wxBOTTOM|wxEXPAND, 0 );
	
	m_DebugTextInput = new wxTextCtrl( this, ID_DEBUGTEXTIN, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_PROCESS_ENTER|wxTE_RICH|wxTE_RICH2 );
	bSizer4->Add( m_DebugTextInput, 1, wxALL|wxEXPAND, 0 );
	
	bSizer1->Add( bSizer4, 0, wxEXPAND, 0 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	m_VarNotebook = new wxNotebook( this, ID_DEFAULT, wxDefaultPosition, wxDefaultSize, wxNB_FLAT );
	m_VarNotebook->SetMinSize( wxSize( -1,270 ) );
	
	m_GlobalsPanelWindow = new wxScrolledWindow( m_VarNotebook, ID_GLOBALS, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_GlobalsPanelWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );
	
	m_GlobalsPropGrid = new wxPropertyGrid(m_GlobalsPanelWindow, ID_GLOBALSSVARIABLELIST, wxDefaultPosition, wxDefaultSize, wxPG_AUTO_SORT|wxPG_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER|wxPG_STATIC_SPLITTER|wxPG_TOOLTIPS);
	bSizer31->Add( m_GlobalsPropGrid, 1, wxEXPAND, 0 );
	
	m_GlobalsPanelWindow->SetSizer( bSizer31 );
	m_GlobalsPanelWindow->Layout();
	bSizer31->Fit( m_GlobalsPanelWindow );
	m_VarNotebook->AddPage( m_GlobalsPanelWindow, wxT("Globals"), false );
	m_AutoPanelWindow = new wxScrolledWindow( m_VarNotebook, ID_AUTOPANEL, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_AutoPanelWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	m_AutosPropGrid = new wxPropertyGrid(m_AutoPanelWindow, ID_AUTOSVARIABLELIST, wxDefaultPosition, wxDefaultSize, wxPG_AUTO_SORT|wxPG_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER|wxPG_STATIC_SPLITTER|wxPG_TOOLTIPS);
	bSizer3->Add( m_AutosPropGrid, 1, wxEXPAND, 0 );
	
	m_AutoPanelWindow->SetSizer( bSizer3 );
	m_AutoPanelWindow->Layout();
	bSizer3->Fit( m_AutoPanelWindow );
	m_VarNotebook->AddPage( m_AutoPanelWindow, wxT("Auto"), false );
	
	bSizer2->Add( m_VarNotebook, 0, wxEXPAND|wxTOP, 0 );
	
	m_ThreadList = new wxListCtrl( this, ID_THREADLIST, wxDefaultPosition, wxSize( 225,260 ), wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VIRTUAL );
	bSizer2->Add( m_ThreadList, 0, wxEXPAND, 0 );
	
	m_Callstack = new wxListCtrl( this, ID_CALLSTACK, wxDefaultPosition, wxSize( 225,100 ), wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer2->Add( m_Callstack, 0, wxEXPAND, 0 );
	
	m_panel3 = new wxPanel( this, ID_DEFAULT, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizer2->Add( m_panel3, 1, wxEXPAND, 0 );
	
	bSizer1->Add( bSizer2, 1, wxEXPAND, 0 );
	
	this->SetSizer( bSizer1 );
	this->Layout();
}
BEGIN_EVENT_TABLE( AboutDialog, wxDialog )
	EVT_BUTTON( wxID_ANY, AboutDialog::_wxFB_OnAboutOk )
END_EVENT_TABLE()

AboutDialog::AboutDialog( wxWindow* parent, int id, wxString title, wxPoint pos, wxSize size, int style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	m_textCtrl3 = new wxTextCtrl( this, wxID_ANY, wxT("Game Monkey Script Remote Debugger\nby Jeremy Swigart\n\nQuestions/Comments/Bugs, report to drevil@omni-bot.com\n\nhttp://www.omni-bot.com"), wxDefaultPosition, wxDefaultSize, wxTE_AUTO_URL|wxTE_CENTRE|wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH|wxTE_RICH2 );
	bSizer6->Add( m_textCtrl3, 2, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );
	
	m_AboutBtnOk = new wxButton( this, wxID_ANY, wxT("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_AboutBtnOk, 0, wxALIGN_CENTER|wxALL, 5 );
	
	this->SetSizer( bSizer6 );
	this->Layout();
}
