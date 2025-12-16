#include "BaseWindow.h"
#include "CheckBox.h"
#include "PicButton.h"
#include "Action.h"

enum EPresets { EPRESET_NORMAL = 0, EPRESET_DSL, EPRESET_SLOW, EPRESET_LAST };

class CMenuConnectionWarning : public CMenuBaseWindow
{
public:
	CMenuConnectionWarning() : CMenuBaseWindow( "ConnectionWarning" )
	{

	}
	void _Init() override;
	void _VidInit() override;
	bool KeyDown( int key ) override;

	void WriteSettings(const EPresets preset );

	CMenuPicButton done;
private:
	CMenuBackgroundBitmap background;
	CMenuPicButton options;
	CMenuCheckBox normal, dsl, slowest;
	CMenuAction title, message;
};

bool CMenuConnectionWarning::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		return true; // handled
	}

	return CMenuBaseWindow::KeyDown( key );
}

void CMenuConnectionWarning::_Init()
{
	iFlags |= QMF_DIALOG;

	background.bForceColor = true;
	background.colorBase = uiPromptBgColor;

	normal.szName = L( "Normal internet connection" );
	normal.SetCoord( 20, 140 );
	SET_EVENT( normal.onChanged,
		((CMenuConnectionWarning*)pSelf->Parent())->WriteSettings( EPRESET_NORMAL ) );

	dsl.szName = L( "DSL or PPTP with limited packet size" );
	dsl.SetCoord( 20, 200 );
	SET_EVENT( dsl.onChanged,
		((CMenuConnectionWarning*)pSelf->Parent())->WriteSettings( EPRESET_DSL ) );

	slowest.szName = L( "Slow connection mode (64kbps)" );
	slowest.SetCoord( 20, 260 );
	SET_EVENT( slowest.onChanged,
		((CMenuConnectionWarning*)pSelf->Parent())->WriteSettings( EPRESET_SLOW ) );

	done.SetPicture( PC_DONE );
	done.szName = L( "Done" );
	done.SetGrayed( true );
	done.SetRect( 410, 320, UI_BUTTONS_WIDTH / 2, UI_BUTTONS_HEIGHT );
	done.onReleased = VoidCb( &CMenuConnectionWarning::Hide );
	done.bEnableTransitions = false;

	options.SetPicture( PC_ADV_OPT );
	options.szName = L( "Adv. Options" );
	SET_EVENT_MULTI( options.onReleased,
	{
		CMenuConnectionWarning *p = pSelf->GetParent(CMenuConnectionWarning);
		UI_GameOptions_Menu();
		p->done.SetGrayed( false );
	});
	options.SetRect( 154, 320, UI_BUTTONS_WIDTH, UI_BUTTONS_HEIGHT );
	options.bEnableTransitions = false;

	title.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	title.eTextAlignment = QM_CENTER;
	title.szName = L( "Connection problem" );
	title.SetRect( 0, 16, 640, 20 );

	message.iFlags = QMF_INACTIVE;
	message.szName = L( "Too many lost packets while connecting!\nPlease select network settings" );
	message.SetRect( 20, 60, 600, 32 );

	AddItem( background );
	AddItem( done );
	AddItem( options );
	AddItem( normal );
	AddItem( dsl );
	AddItem( slowest );
	AddItem( title );
	AddItem( message );
}

void CMenuConnectionWarning::_VidInit()
{
	SetRect( DLG_X + 192, 192, 640, 384 );
	pos.x += uiStatic.xOffset;
	pos.y += uiStatic.yOffset;
}

void CMenuConnectionWarning::WriteSettings( const EPresets preset)
{
	const struct
	{
		float cl_cmdrate;
		float cl_updaterate;
		float rate;
	} presets[EPRESET_LAST] =
	{
	{ 30, 60, 25000 },
	{ 30, 60, 25000 },
	{ 25, 30, 7500 }
	};

	EngFuncs::CvarSetValue( "cl_cmdrate",    presets[preset].cl_cmdrate );
	EngFuncs::CvarSetValue( "cl_updaterate", presets[preset].cl_updaterate );
	EngFuncs::CvarSetValue( "rate",          presets[preset].rate );

	normal.bChecked  = preset == EPRESET_NORMAL;
	dsl.bChecked     = preset == EPRESET_DSL;
	slowest.bChecked = preset == EPRESET_SLOW;

	done.SetGrayed( false );
}

ADD_MENU3( menu_connectionwarning, CMenuConnectionWarning, UI_ConnectionWarning_f );
void UI_ConnectionWarning_f()
{
	if( !UI_IsVisible() )
		UI_Main_Menu();
	menu_connectionwarning->Show();
}
