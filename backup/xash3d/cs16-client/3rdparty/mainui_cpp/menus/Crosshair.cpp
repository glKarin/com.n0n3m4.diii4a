/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2025 APAmk2, Vladislav4KZ, Velaron

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
#include "PicButton.h"
#include "Slider.h"
#include "CheckBox.h"
#include "SpinControl.h"
#include "StringArrayModel.h"

#define ART_BANNER "gfx/shell/head_xhair"

static const char *g_szCrosshairSizes[] = { "auto", "small", "medium", "large" };

static struct
{
	const char *name;
	int r, g, b;
} g_CrosshairColors[] = {
	{ "#Valve_Green", 50, 250, 50 },
	{ "#Valve_Red", 250, 50, 50 },
	{ "#Valve_Blue", 50, 50, 250 },
	{ "#Valve_Yellow", 250, 250, 50 },
	{ "#Valve_Ltblue", 50, 250, 250 }
};

class CMenuClassicCrosshair : public CMenuItemsHolder
{
public:
	void Save();

private:
	void _Init() override;
	void Reload() override;

	CMenuSpinControl size;
	CMenuSpinControl color;
	CMenuCheckBox translucent;

	class CMenuCrosshairPreview : public CMenuBaseItem
	{
	public:
		void Draw() override;

		HIMAGE hImage;
		HIMAGE hWhite;
	} preview;
};

class CMenuXhair : public CMenuItemsHolder
{
public:
	void Save();

private:
	void _Init() override;
	void Reload() override;

	CMenuCheckBox additive;

	CMenuSlider r;
	CMenuSlider g;
	CMenuSlider b;
	CMenuSlider a;

	CMenuCheckBox dot;
	CMenuCheckBox dynamicMove;
	CMenuSlider dynamicScale;
	CMenuCheckBox gapUseWeaponValue;
	CMenuSlider gap;
	CMenuSlider pad;
	CMenuSlider size;
	CMenuCheckBox xhairT;
	CMenuSlider thick;

	class CMenuCrosshairPreview : public CMenuBaseItem
	{
	public:
		void Draw() override;
		void DrawCrosshairPadding( int _pad, int _x0, int _y0, int _x1, int _y1 );
		void DrawCrosshairSection( int _x0, int _y0, int _x1, int _y1 );
		int ScaleForRes( float value, int height );

		HIMAGE hImage;
		HIMAGE hWhite;
	} preview;
};

class CMenuCrosshair : public CMenuFramework
{
public:
	CMenuCrosshair() :
	    CMenuFramework( "CMenuCrosshair" ) { }

	void SaveAndPopMenu() override;
	void ToggleMenu();

	CMenuCheckBox useXhair;
	CMenuClassicCrosshair crosshair;
	CMenuXhair xhair;
	CMenuPicButton done;

private:
	void _Init() override;
};

void CMenuCrosshair::SaveAndPopMenu()
{
	useXhair.WriteCvar();

	xhair.Save();
	crosshair.Save();

	CMenuFramework::SaveAndPopMenu();
}

void CMenuCrosshair::ToggleMenu()
{
	if ( useXhair.bChecked )
	{
		xhair.Show();
		crosshair.Hide();
	}
	else
	{
		xhair.Hide();
		crosshair.Show();
	}
}

void CMenuCrosshair::_Init()
{
	banner.SetPicture( ART_BANNER );

	useXhair.SetNameAndStatus( L( "Enhanced crosshair" ), L( "Enables enhanced crosshair" ) );
	useXhair.LinkCvar( "xhair_enable" );
	useXhair.SetCoord( 72, 230 );
	useXhair.onChanged = VoidCb( &CMenuCrosshair::ToggleMenu );

	crosshair.SetRect( 72, 350, 880, 466 );
	xhair.SetRect( 72, 350, 880, 466 );

	AddItem( banner );
	AddItem( useXhair );
	AddItem( crosshair );
	AddItem( xhair );
	AddItem( done );

	done.szName = L( "GameUI_OK" );
	done.SetCoord( 72, 280 );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CMenuCrosshair::SaveAndPopMenu );

	ToggleMenu();
}

void CMenuClassicCrosshair::_Init()
{
	int x = 0, y = 0;

	static const char *itemlist[V_ARRAYSIZE( g_CrosshairColors )];
	static CStringArrayModel colors( itemlist, V_ARRAYSIZE( g_CrosshairColors ) );
	for ( size_t i = 0; i < V_ARRAYSIZE( g_CrosshairColors ); i++ )
		itemlist[i] = L( g_CrosshairColors[i].name );

	static const char *sizelist[] = { L( "GameUI_Auto" ), L( "GameUI_Small" ), L( "GameUI_Medium" ), L( "GameUI_Large" ) };
	static CStringArrayModel sizes( sizelist, V_ARRAYSIZE( sizelist ) );

	size.Setup( &sizes );
	size.LinkCvar( "cl_crosshair_size", CMenuEditable::CVAR_VALUE );
	size.SetRect( x, y, 220, 32 );
	y += 60;

	color.Setup( &colors );
	color.LinkCvar( "cl_crosshair_color", CMenuEditable::CVAR_STRING );
	color.SetRect( x, y, 220, 32 );
	y += 60;

	translucent.SetNameAndStatus( L( "GameUI_Translucent" ), NULL );
	translucent.LinkCvar( "cl_crosshair_translucent" );
	translucent.SetCoord( x, y );
	y = 0;
	x = 584;

	preview.szName = L( "GameUI_CrosshairDescription" );
	preview.SetRect( x, y, 200, 200 );
	preview.hImage = EngFuncs::PIC_Load( "gfx/vgui/crosshair.tga", 0 );
	preview.hWhite = EngFuncs::PIC_Load( "*white" );

	AddItem( preview );
	AddItem( size );
	AddItem( color );
	AddItem( translucent );
}

void CMenuXhair::_Init()
{
	int x = 0, y = 0;

	r.SetNameAndStatus( L( "Red" ), NULL );
	r.Setup( 0, 255, 1 );
	r.SetCoord( x, y );
	y += 60;

	g.SetNameAndStatus( L( "Green" ), NULL );
	g.Setup( 0, 255, 1 );
	g.SetCoord( x, y );
	y += 60;

	b.SetNameAndStatus( L( "Blue" ), NULL );
	b.Setup( 0, 255, 1 );
	b.SetCoord( x, y );
	y += 60;

	a.SetNameAndStatus( L( "Alpha" ), NULL );
	a.Setup( 0, 255, 1 );
	a.SetCoord( x, y );
	y += 60;

	dynamicMove.SetNameAndStatus( L( "Dynamic move" ), NULL );
	dynamicMove.LinkCvar( "xhair_dynamic_move" );
	dynamicMove.SetCoord( x, y );
	y += 60;

	gapUseWeaponValue.SetNameAndStatus( L( "Gap, use weapon value" ), NULL );
	gapUseWeaponValue.LinkCvar( "xhair_gap_useweaponvalue" );
	gapUseWeaponValue.SetCoord( x, y );
	y += 60;

	additive.SetNameAndStatus( L( "Additive" ), NULL );
	additive.LinkCvar( "xhair_additive" );
	additive.SetCoord( x, y );
	y = 0;
	x += 292;

	gap.SetNameAndStatus( L( "Gap" ), NULL );
	gap.Setup( -6, 15, 1 );
	gap.LinkCvar( "xhair_gap" );
	gap.SetCoord( x, y );
	y += 60;

	pad.SetNameAndStatus( L( "Padding" ), NULL );
	pad.Setup( 0, 16, 1 );
	pad.LinkCvar( "xhair_pad" );
	pad.SetCoord( x, y );
	y += 60;

	size.SetNameAndStatus( L( "Size" ), NULL );
	size.Setup( 0, 32, 1 );
	size.LinkCvar( "xhair_size" );
	size.SetCoord( x, y );
	y += 60;

	thick.SetNameAndStatus( L( "Thickness" ), NULL );
	thick.Setup( 0, 32, 1 );
	thick.LinkCvar( "xhair_thick" );
	thick.SetCoord( x, y );
	y += 60;

	dynamicScale.SetNameAndStatus( L( "Dynamic scale" ), NULL );
	dynamicScale.Setup( 0, 3.0f, 0.1f );
	dynamicScale.LinkCvar( "xhair_dynamic_scale" );
	dynamicScale.SetCoord( x, y );
	y += 60;

	dot.SetNameAndStatus( L( "Dot" ), NULL );
	dot.LinkCvar( "xhair_dot" );
	dot.SetCoord( x, y );
	y += 60;

	xhairT.SetNameAndStatus( L( "T-Shape" ), NULL );
	xhairT.LinkCvar( "xhair_t" );
	xhairT.SetCoord( x, y );
	y = 0;
	x += 292;

	preview.SetNameAndStatus( L( "GameUI_CrosshairDescription" ), NULL );
	preview.SetRect( x, y, 200, 200 );
	preview.hImage = EngFuncs::PIC_Load( "gfx/vgui/crosshair.tga", 0 );
	preview.hWhite = EngFuncs::PIC_Load( "*white" );

	AddItem( r );
	AddItem( g );
	AddItem( b );
	AddItem( a );

	AddItem( dynamicMove );
	AddItem( gap );
	AddItem( pad );
	AddItem( size );
	AddItem( thick );
	AddItem( dynamicScale );
	AddItem( dot );
	AddItem( xhairT );
	AddItem( gapUseWeaponValue );
	AddItem( additive );

	AddItem( preview );
}

void CMenuXhair::Reload()
{
	int cvarColor[4] = { 0, 255, 0, 255 };

	CMenuItemsHolder::Reload();

	if ( sscanf( EngFuncs::GetCvarString( "xhair_color" ), "%d %d %d %d", &cvarColor[0], &cvarColor[1], &cvarColor[2], &cvarColor[3] ) == 4 )
	{
		r.SetCurrentValue( bound( 0, cvarColor[0], 255 ) );
		g.SetCurrentValue( bound( 0, cvarColor[1], 255 ) );
		b.SetCurrentValue( bound( 0, cvarColor[2], 255 ) );
		a.SetCurrentValue( bound( 0, cvarColor[3], 255 ) );
	}
}

void CMenuClassicCrosshair::Save()
{
	int i;
	char colorstr[32];

	i = size.GetCurrentValue();
	EngFuncs::CvarSetString( "cl_crosshair_size", g_szCrosshairSizes[i] );

	i = color.GetCurrentValue();
	if ( i >= 0 && i < V_ARRAYSIZE( g_CrosshairColors ) )
	{
		snprintf( colorstr, sizeof( colorstr ), "%i %i %i", g_CrosshairColors[i].r, g_CrosshairColors[i].g, g_CrosshairColors[i].b );
		EngFuncs::CvarSetString( "cl_crosshair_color", colorstr );
	}

	translucent.WriteCvar();
}

int CMenuXhair::CMenuCrosshairPreview::ScaleForRes( float value, int height )
{
	/* "default" resolution is 640x480 */
	return rint( value * ( (float)height / 480.0f ) );
}

void CMenuXhair::CMenuCrosshairPreview::DrawCrosshairSection( int _x0, int _y0, int _x1, int _y1 )
{
	CMenuXhair *parent = (CMenuXhair *)Parent();

	float x0 = (float)_x0;
	float y0 = (float)_y0;
	float x1 = (float)_x1 - _x0;
	float y1 = (float)_y1 - _y0;

	EngFuncs::PIC_Set( hWhite, parent->r.GetCurrentValue(), parent->g.GetCurrentValue(), parent->b.GetCurrentValue(), parent->a.GetCurrentValue() );

	if ( parent->additive.bChecked )
		EngFuncs::PIC_DrawAdditive( x0, y0, x1, y1 );
	else
		EngFuncs::PIC_DrawTrans( x0, y0, x1, y1 );
}

void CMenuXhair::CMenuCrosshairPreview::DrawCrosshairPadding( int _pad, int _x0, int _y0, int _x1, int _y1 )
{
	CMenuXhair *parent = (CMenuXhair *)Parent();

	float pad = (float)_pad;
	float x0 = (float)_x0;
	float y0 = (float)_y0;
	float x1 = (float)_x1 - _x0;
	float y1 = (float)_y1 - _y0;

	EngFuncs::PIC_Set( hWhite, 0, 0, 0, parent->a.GetCurrentValue() );
	EngFuncs::PIC_DrawTrans( x0 - pad, y0 - pad, x1 + 2 * pad, pad ); // top part

	EngFuncs::PIC_Set( hWhite, 0, 0, 0, parent->a.GetCurrentValue() );
	EngFuncs::PIC_DrawTrans( x0 - pad, y0 + y1, x1 + 2 * pad, pad ); // bottom part

	EngFuncs::PIC_Set( hWhite, 0, 0, 0, parent->a.GetCurrentValue() );
	EngFuncs::PIC_DrawTrans( x0 - pad, y0, pad, y1 ); // left part

	EngFuncs::PIC_Set( hWhite, 0, 0, 0, parent->a.GetCurrentValue() );
	EngFuncs::PIC_DrawTrans( x0 + x1, y0, pad, y1 ); // right part
}

void CMenuXhair::CMenuCrosshairPreview::Draw()
{
	CMenuXhair *parent = (CMenuXhair *)Parent();

	int center_x, center_y;
	int gap, length, thickness;
	int y0, y1, x0, x1;
	wrect_t inner;
	wrect_t outer;

	/* calculate the coordinates */
	center_x = m_scPos.x + ( m_scSize.w / 2 );
	center_y = m_scPos.y + ( m_scSize.h / 2 );

	gap = ScaleForRes( 4 + parent->gap.GetCurrentValue(), ScreenHeight );
	length = ScaleForRes( parent->size.GetCurrentValue(), ScreenHeight );
	thickness = ScaleForRes( parent->thick.GetCurrentValue(), ScreenHeight );
	thickness = Q_max( 1, thickness );

	inner.left = ( center_x - gap - thickness / 2 );
	inner.right = ( inner.left + 2 * gap + thickness );
	inner.top = ( center_y - gap - thickness / 2 );
	inner.bottom = ( inner.top + 2 * gap + thickness );

	outer.left = ( inner.left - length );
	outer.right = ( inner.right + length );
	outer.top = ( inner.top - length );
	outer.bottom = ( inner.bottom + length );

	y0 = ( center_y - thickness / 2 );
	x0 = ( center_x - thickness / 2 );
	y1 = ( y0 + thickness );
	x1 = ( x0 + thickness );

	// APAMk2 - Draw Crosshair's BG before Crosshair itself
	if ( !hImage )
	{
		UI_FillRect( m_scPos, m_scSize, uiPromptBgColor );
	}
	else
	{
		EngFuncs::PIC_Set( hImage, 255, 255, 255 );
		EngFuncs::PIC_DrawTrans( m_scPos, m_scSize );
	}

	int textHeight = m_scPos.y - ( m_scChSize * 1.5f );
	UI_DrawString( font, m_scPos.x, textHeight, m_scSize.w, m_scChSize, szName, uiColorHelp, m_scChSize, QM_LEFT, ETF_SHADOW | ETF_NOSIZELIMIT | ETF_FORCECOL );

	/* draw padding if wanted */
	if ( parent->pad.GetCurrentValue() )
	{
		/* don't scale this */
		int pad = (int)parent->pad.GetCurrentValue();

		if ( parent->dot.bChecked )
			DrawCrosshairPadding( pad, x0, y0, x1, y1 );

		if ( !parent->xhairT.bChecked )
			DrawCrosshairPadding( pad, x0, outer.top, x1, inner.top );

		DrawCrosshairPadding( pad, x0, inner.bottom, x1, outer.bottom );
		DrawCrosshairPadding( pad, outer.left, y0, inner.left, y1 );
		DrawCrosshairPadding( pad, inner.right, y0, outer.right, y1 );
	}

	if ( parent->dot.bChecked )
		DrawCrosshairSection( x0, y0, x1, y1 );

	if ( !parent->xhairT.bChecked )
		DrawCrosshairSection( x0, outer.top, x1, inner.top );

	DrawCrosshairSection( x0, inner.bottom, x1, outer.bottom );
	DrawCrosshairSection( outer.left, y0, inner.left, y1 );
	DrawCrosshairSection( inner.right, y0, outer.right, y1 );

	// draw the rectangle
	if ( eFocusAnimation == QM_HIGHLIGHTIFFOCUS && IsCurrentSelected() )
		UI_DrawRectangle( m_scPos, m_scSize, uiInputTextColor );
	else
		UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );
}

void CMenuClassicCrosshair::CMenuCrosshairPreview::Draw()
{
	CMenuClassicCrosshair *parent = (CMenuClassicCrosshair *)Parent();

	int length;
	int x = m_scPos.x, y = m_scPos.y;
	int w = m_scSize.w, h = m_scSize.h;
	int delta;
	int i = parent->color.GetCurrentValue();
	int r = 0, g = 255, b = 0, a = 180;

	if ( i >= 0 && i < V_ARRAYSIZE( g_CrosshairColors ) )
	{
		r = g_CrosshairColors[i].r;
		g = g_CrosshairColors[i].g;
		b = g_CrosshairColors[i].b;
	}
	else
	{
		sscanf( EngFuncs::GetCvarString( "cl_crosshair_color" ), "%d %d %d", &r, &g, &b );
	}

	if ( !hImage )
	{
		UI_FillRect( m_scPos, m_scSize, uiPromptBgColor );
	}
	else
	{
		EngFuncs::PIC_Set( hImage, 255, 255, 255 );
		EngFuncs::PIC_DrawTrans( m_scPos, m_scSize );
	}

	switch ( (int)parent->size.GetCurrentValue() )
	{
	case 1:
		length = 10;
		break;
	case 2:
		length = 20;
		break;
	case 3:
		length = 30;
		break;
	case 0:
		if ( ScreenWidth < 640 )
			length = 30;
		else if ( ScreenWidth < 1024 )
			length = 20;
		else
			length = 10;
	default:
		break;
	}

	length *= ScreenHeight / 768.0f;
	delta = ( w / 2 - length ) * 0.5f;

	if ( !parent->translucent.bChecked )
	{
		EngFuncs::PIC_Set( hWhite, r, g, b, a );
		EngFuncs::PIC_DrawTrans( x + w / 2, y + delta, 1, length );

		EngFuncs::PIC_Set( hWhite, r, g, b, a );
		EngFuncs::PIC_DrawTrans( x + w / 2, y + h / 2 + delta, 1, length );

		EngFuncs::PIC_Set( hWhite, r, g, b, a );
		EngFuncs::PIC_DrawTrans( x + delta, y + h / 2, length, 1 );

		EngFuncs::PIC_Set( hWhite, r, g, b, a );
		EngFuncs::PIC_DrawTrans( x + w / 2 + delta, y + h / 2, length, 1 );
	}
	else
	{
		EngFuncs::PIC_Set( hWhite, r, g, b, a );
		EngFuncs::PIC_DrawAdditive( x + w / 2, y + delta, 1, length );

		EngFuncs::PIC_Set( hWhite, r, g, b, a );
		EngFuncs::PIC_DrawAdditive( x + w / 2, y + h / 2 + delta, 1, length );

		EngFuncs::PIC_Set( hWhite, r, g, b, a );
		EngFuncs::PIC_DrawAdditive( x + delta, y + h / 2, length, 1 );

		EngFuncs::PIC_Set( hWhite, r, g, b, a );
		EngFuncs::PIC_DrawAdditive( x + w / 2 + delta, y + h / 2, length, 1 );
	}

	int textHeight = m_scPos.y - ( m_scChSize * 1.5f );
	UI_DrawString( font, m_scPos.x, textHeight, m_scSize.w, m_scChSize, szName, uiColorHelp, m_scChSize, QM_LEFT, ETF_SHADOW | ETF_NOSIZELIMIT | ETF_FORCECOL );

	// draw the rectangle
	if ( eFocusAnimation == QM_HIGHLIGHTIFFOCUS && IsCurrentSelected() )
		UI_DrawRectangle( m_scPos, m_scSize, uiInputTextColor );
	else
		UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );
}

void CMenuClassicCrosshair::Reload()
{
	char colorstr[32];
	int rgb[3];
	char sizestr[32];
	int i, j;

	CMenuItemsHolder::Reload();

	strncpy( colorstr, EngFuncs::GetCvarString( "cl_crosshair_color" ), sizeof( colorstr ) );
	if ( sscanf( colorstr, "%d %d %d", &rgb[0], &rgb[1], &rgb[2] ) == 3 )
	{
		j = V_ARRAYSIZE( g_CrosshairColors );
		for ( i = 0; i <= j; i++ )
		{
			if ( i == j )
			{
				color.SetCurrentValue( colorstr );
				break;
			}

			if ( g_CrosshairColors[i].r == rgb[0] && g_CrosshairColors[i].g == rgb[1] && g_CrosshairColors[i].b == rgb[2] )
			{
				color.SetCurrentValue( i );
				break;
			}
		}
	}

	strncpy( sizestr, EngFuncs::GetCvarString( "cl_crosshair_size" ), sizeof( sizestr ) );
	j = V_ARRAYSIZE( g_szCrosshairSizes );
	for ( i = 0; i <= j; i++ )
	{
		if ( i == j )
		{
			size.SetCurrentValue( EngFuncs::GetCvarFloat( "cl_crosshair_size" ) );
			break;
		}

		if ( !stricmp( sizestr, g_szCrosshairSizes[i] ) )
		{
			size.SetCurrentValue( i );
			break;
		}
	}
}

void CMenuXhair::Save()
{
	char colorstr[32];

	snprintf( colorstr, sizeof( colorstr ), "%d %d %d %d", (int)r.GetCurrentValue(), (int)g.GetCurrentValue(), (int)b.GetCurrentValue(), (int)a.GetCurrentValue() );
	EngFuncs::CvarSetString( "xhair_color", colorstr );

	additive.WriteCvar();
	dot.WriteCvar();
	dynamicMove.WriteCvar();
	dynamicScale.WriteCvar();
	gapUseWeaponValue.WriteCvar();
	gap.WriteCvar();
	pad.WriteCvar();
	size.WriteCvar();
	xhairT.WriteCvar();
	thick.WriteCvar();
}

ADD_MENU( menu_crosshair, CMenuCrosshair, UI_Crosshair_Menu );