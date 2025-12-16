/*
ScriptMenu.cpp -- dynamic menu built on *.scr files
Copyright (C) 2017 a1batross

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "Framework.h"
#include "CFGScript.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Editable.h"
#include "CheckBox.h"
#include "Slider.h"
#include "SpinControl.h"
#include "Field.h"
#include "ItemsHolder.h"
#include "Action.h"

#define ART_BANNER_SERVER "gfx/shell/head_advoptions"
#define ART_BANNER_USER "gfx/shell/head_gameopts"
#define IGNORE_ALREADY_USED_CVARS 1

class CMenuScriptConfigPage : public CMenuItemsHolder
{
public:
	CMenuScriptConfigPage();
	~CMenuScriptConfigPage() override;

	bool IsItemFits( CMenuEditable &item );
	void PrepareItem( CMenuEditable &item );
	void Save();

private:
	int m_iCurrentHeight;
	int m_iPadding;
};

class CMenuScriptConfig : public CMenuFramework
{
public:
	CMenuScriptConfig();
	~CMenuScriptConfig() override;

	void SetScriptConfig( const char *path, bool earlyInit = false );

	void SaveAndPopMenu() override
	{
		for( int i = m_iPagesIndex, j = 0; j < m_iPagesCount; i++, j++ )
			((CMenuScriptConfigPage*)m_pItems[i])->Save();

		CMenuFramework::SaveAndPopMenu();
	}

	void FlipMenu( void );
	static void ListItemCvarWriteCb( CMenuBaseItem *pSelf, void *pExtra );
	static void ListItemCvarGetCb( CMenuBaseItem *pSelf, void *pExtra );

	scrvardef_t *m_pVars;

private:
	CMenuSpinControl pageSelector;
	// CMenuAction unavailable;

	void FreeItems( void );

	void _Init() override;

	const char *m_szConfig;
	int m_iVarsCount;
	int m_iPagesIndex;
	int m_iPagesCount;
	int m_iCurrentPage;
};

CMenuScriptConfigPage::CMenuScriptConfigPage() : CMenuItemsHolder()
{
	m_bWrapCursor = false; // Don't cycle in page
	m_iCurrentHeight = 0;
	m_iPadding = 16;
	SetRect( 360, 230, 660, 440 );
}

CMenuScriptConfigPage::~CMenuScriptConfigPage()
{
	FOR_EACH_VEC( m_pItems, i )
	{
		delete m_pItems[i];
	}
}

bool CMenuScriptConfigPage::IsItemFits(CMenuEditable &item)
{
	if( m_iCurrentHeight + item.size.h + m_iPadding >= size.h )
		return false;
	return true;
}

void CMenuScriptConfigPage::PrepareItem(CMenuEditable &item)
{
	item.SetCoord( 0, m_iCurrentHeight );
	m_iCurrentHeight += item.size.h + m_iPadding;
}

void CMenuScriptConfigPage::Save()
{
	FOR_EACH_VEC( m_pItems, i )
	{
		((CMenuEditable*)m_pItems[i])->WriteCvar();
	}
}

CMenuScriptConfig::CMenuScriptConfig() : CMenuFramework( "ScriptConfig" ),
	m_pVars(), m_szConfig(), m_iVarsCount(), m_iPagesIndex(), m_iPagesCount(), m_iCurrentPage()
{

}

CMenuScriptConfig::~CMenuScriptConfig()
{
	CSCR_FreeList( m_pVars );
	for( int i = m_iPagesIndex; i < m_iPagesIndex + m_iPagesCount; i++ )
	{
		delete m_pItems[i];
	}
}

void CMenuScriptConfig::ListItemCvarWriteCb(CMenuBaseItem *pSelf, void *pExtra)
{
	CMenuSpinControl *self = (CMenuSpinControl*)pSelf;
	scrvarlist_t *list = (scrvarlist_t*)pExtra;
	scrvarlistentry_t *entry = list->pEntries;

	int entryNum = (int)self->GetCurrentValue();
	for( int i = 0; i < entryNum; i++, entry = entry->next );

	EngFuncs::CvarSetValue( self->CvarName(), entry->flValue );
}

void CMenuScriptConfig::ListItemCvarGetCb(CMenuBaseItem *pSelf, void *pExtra)
{
	CMenuSpinControl *self = (CMenuSpinControl*)pSelf;
	scrvarlist_t *list = (scrvarlist_t*)pExtra;
	scrvarlistentry_t *entry = list->pEntries;

	float value = EngFuncs::GetCvarFloat( self->CvarName() );
	int i;
	for( i = 0; entry; entry = entry->next, i++ )
	{
		if( entry->flValue == value )
		{
			self->SetCurrentValue( i );
			break;
		}
	}

	if( entry )
	{
		self->SetCvarValue( i );
	}
}

void CMenuScriptConfig::_Init( void )
{
	AddItem( banner );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuScriptConfig::SaveAndPopMenu ) );
	AddButton( L( "GameUI_Cancel" ), nullptr, PC_CANCEL, VoidCb( &CMenuScriptConfig::Hide ) );

	if( !m_pVars )
		return;

	// RemoveItem( unavailable );
	pageSelector.SetRect( 780, 180, 160, 32 );
	AddItem( pageSelector );


	CMenuScriptConfigPage *page = new CMenuScriptConfigPage;
	page->SetRect( 340, 255, 660, 500 );
	page->iFlags &= ~(QMF_GRAYED|QMF_INACTIVE|QMF_MOUSEONLY);
	page->Show();
	m_iCurrentPage = 0;
	m_iPagesCount = 1;
	m_iPagesIndex = m_pItems.Count();
	AddItem( page );

	for( scrvardef_t *var = m_pVars; var; var = var->next )
	{
		CMenuEditable *editable;
		CMenuEditable::cvarType_e cvarType;

#if IGNORE_ALREADY_USED_CVARS
		if( !stricmp( var->name, "hostname") ||
			!stricmp( var->name, "sv_password" ) ||
			!stricmp( var->name, "maxplayers") )
			continue;
#endif

		switch( var->type )
		{
		case T_BOOL:
		{
			CMenuCheckBox *checkbox = new CMenuCheckBox;

			editable = checkbox;
			cvarType = CMenuEditable::CVAR_VALUE;
			break;
		}
		case T_NUMBER:
		{
			CMenuSpinControl *spinControl = new CMenuSpinControl;
			float fMin, fMax;

			if( var->number.fMin == -1 ) fMin = -9999;
			else fMin = var->number.fMin;

			if( var->number.fMax == -1 ) fMax = 9999;
			else fMax = var->number.fMax;

			spinControl->Setup( fMin, fMax, 1 );
			editable = spinControl;

			cvarType = CMenuEditable::CVAR_VALUE;
			break;
		}
		case T_STRING:
		{
			CMenuField *field = new CMenuField;
			field->iMaxLength = CS_SIZE;
			editable = field;
			cvarType = CMenuEditable::CVAR_STRING;
			break;
		}
		case T_LIST:
		{
			CMenuSpinControl *spinControl = new CMenuSpinControl;

			spinControl->Setup( var->list.pModel );
			spinControl->onCvarGet = ListItemCvarGetCb;
			spinControl->onCvarGet.pExtra = (void*)&var->list;
			spinControl->onCvarWrite = ListItemCvarWriteCb;
			spinControl->onCvarWrite.pExtra = (void*)&var->list;
			cvarType = CMenuEditable::CVAR_VALUE;
			editable = spinControl;
			break;
		}
		default: continue;
		}

		if( var->type != T_BOOL )
			editable->SetSize( 300, 32 );

		editable->iFlags |= QMF_NOTIFY;
		// editable->szName = var->name;
		editable->szStatusText = L( var->desc );
		editable->SetCharSize( QM_SMALLFONT );
		editable->LinkCvar( var->name, cvarType );
		editable->iFlags &= ~(QMF_GRAYED|QMF_INACTIVE|QMF_MOUSEONLY);
		editable->Show();

		// create new page
		if( !page->IsItemFits( *editable ) )
		{
			page = new CMenuScriptConfigPage;
			page->Hide();
			page->iFlags &= ~(QMF_GRAYED|QMF_INACTIVE);
			page->SetRect( 340, 255, 660, 440 );

			AddItem( page );
			m_iPagesCount++;
		}

		page->PrepareItem( *editable );
		page->AddItem( editable );
	}


	pageSelector.SetInactive(false);
	pageSelector.Setup( 1, m_iPagesCount, 1 );
	pageSelector.SetCurrentValue( 1 );
	pageSelector.onChanged = VoidCb( &CMenuScriptConfig::FlipMenu );
}

void CMenuScriptConfig::SetScriptConfig(const char *path, bool earlyInit)
{
	if( m_szConfig && m_pVars && !stricmp( m_szConfig, path ) )
		return; // do nothing

	m_szConfig = path;

	if( m_pVars )
		CSCR_FreeList( m_pVars );

	m_pVars = CSCR_LoadDefaultCVars( m_szConfig, &m_iVarsCount );
}

void CMenuScriptConfig::FlipMenu( void )
{
	int newIndex = (int)pageSelector.GetCurrentValue() - 1;

	CMenuScriptConfigPage *oldPage = (CMenuScriptConfigPage *)m_pItems[m_iPagesIndex + m_iCurrentPage];
	CMenuScriptConfigPage *newPage = (CMenuScriptConfigPage *)m_pItems[m_iPagesIndex + newIndex];

	oldPage->Hide();
	newPage->Show();

	m_iCurrentPage = newIndex;
}

ADD_MENU3( menu_serveroptions, CMenuScriptConfig, UI_AdvServerOptions_Menu );
ADD_MENU3( menu_useroptions, CMenuScriptConfig, UI_AdvUserOptions_Menu );

void UI_AdvServerOptions_Menu()
{
	menu_serveroptions->banner.SetPicture( ART_BANNER_SERVER );
	menu_serveroptions->szName = L( "Server Options" );
	menu_serveroptions->Show();
}

void UI_AdvUserOptions_Menu()
{
	menu_useroptions->banner.SetPicture( ART_BANNER_USER );
	menu_useroptions->szName = L( "GameUI_MultiplayerAdvanced" );
	menu_useroptions->Show();
}

void UI_LoadScriptConfig()
{
	// yes, create cvars if needed
	menu_serveroptions->SetScriptConfig( "settings.scr", true );
	menu_useroptions->SetScriptConfig( "user.scr", true );
}

bool UI_AdvUserOptions_IsAvailable()
{
	return menu_useroptions->m_pVars != NULL;
}

bool UI_AdvServerOptions_IsAvailable()
{
	return menu_serveroptions->m_pVars != NULL;
}
