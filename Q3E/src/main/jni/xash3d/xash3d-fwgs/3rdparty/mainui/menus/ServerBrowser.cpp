/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include "Bitmap.h"
#include "YesNoMessageBox.h"
#include "Table.h"
#include "keydefs.h"
#include "Switch.h"
#include "Field.h"
#include "utlvector.h"
#include "CheckBox.h"
#include "SpinControl.h"
#include "StringArrayModel.h"
#include "DropDown.h"

#define ART_BANNER_INET     "gfx/shell/head_inetgames"
#define ART_BANNER_LAN      "gfx/shell/head_lan"
#define ART_BANNER_LOCK     "gfx/shell/lock"
#define ART_BANNER_FAVORITE "gfx/shell/favorite"

#define MAX_PING 9.999f
#define FILTER_MAX_MAPS 16

class CMenuServerBrowser;

enum
{
	COLUMN_PASSWORD = 0,
	COLUMN_FAVORITE,
	COLUMN_NAME,
	COLUMN_MAP,
	COLUMN_PLAYERS,
	COLUMN_PING,
	COLUMN_IP,
	COLUMN_LAST
};

struct server_t
{
	netadr_t adr;
	char  info[512];
	float ping;
	int  numcl;
	int  maxcl;
	char name[64];
	char mapname[64];
	char clientsstr[64];
	char pingstr[64];
	char ipstr[64];
	bool favorite;
	bool havePassword;
	bool isLegacy;
	bool isGoldSrc;
	bool pending_info;

	server_t( netadr_t adr, const char *info, bool is_favorite, bool pending_info = false );
	void UpdateData();
	void SetPing( float ping );

	const char *ToProtocol( void )
	{
		if( isLegacy ) return "48";
		if( isGoldSrc ) return "gs";
		return "49";
	}

	int Rank( const server_t &other ) const
	{
		if( isLegacy > other.isLegacy ) return 100;
		else if( isLegacy < other.isLegacy ) return -100;
		return 0;
	}

	int NameCmp( const server_t &other ) const
	{
		return colorstricmp( name, other.name );
	}

	int AdrCmp( const server_t &other ) const
	{
		return EngFuncs::NET_CompareAdr( &adr, &other.adr );
	}

	int MapCmp( const server_t &other ) const
	{
		return stricmp( mapname, other.mapname );
	}

	int ClientCmp( const server_t &other ) const
	{
		if( numcl > other.numcl ) return 1;
		else if( numcl < other.numcl ) return -1;
		return 0;
	}

	int PingCmp( const server_t &other ) const
	{
		if( ping > other.ping ) return 1;
		else if( ping < other.ping ) return -1;
		return 0;
	}

	bool IsEmpty( ) const
	{
		return numcl == 0;
	}

	bool IsFull( ) const
	{
		return numcl >= maxcl;
	}

	// make generic
	// always rank new servers higher, even when sorting in reverse order
#define GENERATE_COMPAR_FN( method ) \
	static int method ## Ascend( const void *a, const void *b ) \
	{\
		return (( const server_t *)a)->Rank( *(( const server_t *)b) ) + (( const server_t *)a)->method( *(( const server_t *)b) );\
	}\
	static int method ## Descend( const void *a, const void *b ) \
	{\
		return (( const server_t *)a)->Rank( *(( const server_t *)b) ) + (( const server_t *)b)->method( *(( const server_t *)a) );\
	}\

	GENERATE_COMPAR_FN( NameCmp )
	GENERATE_COMPAR_FN( AdrCmp )
	GENERATE_COMPAR_FN( MapCmp )
	GENERATE_COMPAR_FN( ClientCmp )
	GENERATE_COMPAR_FN( PingCmp )
#undef GENERATE_COMPAR_FN
};

struct favlist_entry_t
{
	favlist_entry_t( const char *sadr, const char *prot, bool favorited = true )
	{
		Q_strncpy( this->sadr, sadr, sizeof( this->sadr ));
		Q_strncpy( this->prot, prot, sizeof( this->prot ));
		this->favorited = favorited;
	}

	void GenerateDummyInfoString( CUtlString &info ) const
	{
		info.AppendFormat( "\\host\\%s", sadr );
		info.AppendFormat( "\\gamedir\\%s", gMenu.m_gameinfo.gamefolder );
		info.Append( "\\map\\unknown\\numcl\\0\\maxcl\\0" );
		if( !stricmp( prot, "current" ) || !strcmp( prot, "49" ))
		{
			info.Append( "\\p\\49" );
		}
		else if( !stricmp( prot, "legacy" ) || !strcmp( prot, "48" ))
		{
			info.Append( "\\p\\48\\legacy\\1" );
		}
		else if( !stricmp( prot, "goldsrc" ) || !stricmp( prot, "gs" ))
		{
			info.Append( "\\p\\48\\gs\\1" );
		}
		else
		{
			UI_ShowMessageBox( "Invalid protocol while generating dummy info string" );
			info.Clear();
		}
	}

	void QueryServer( void ) const
	{
		EngFuncs::ClientCmdF( false, "queryserver \"%s\" \"%s\"", sadr, prot );
	}

	char sadr[128];
	char prot[16];
	bool favorited;
};

struct filterMap_t
{
	char name[64];

	filterMap_t( ): filterMap_t( "" ) {}

	filterMap_t( const filterMap_t &obj )
	{
		count = obj.count;
		Q_strncpy( name, obj.name, sizeof( name ) );
		Q_strncpy( display, obj.display, sizeof( display ) );
	}

	filterMap_t( const char *s, unsigned short c = 0 )
	{
		count = c;
		Q_strncpy( name, s, sizeof( name ) );
		UpdateDisplay( );
	}

	inline bool operator==( const char *s ) const
	{
		return colorstricmp( name, s ) == 0;
	}

	void AddCount( unsigned short c )
	{
		count += c;
		UpdateDisplay();
	}

	const char *GetDisplay( ) const
	{
		return display;
	}

	static int CmpName( const filterMap_t *a, const filterMap_t *b )
	{
		return colorstricmp( a->name, b->name );
	}

	static int CmpCount( const filterMap_t *a, const filterMap_t *b )
	{
		unsigned short ca = a->count;
		unsigned short cb = b->count;
		return ca < cb ? -1 : (ca > cb ? 1 : 0);
	}

	static int CmpCountInvert( const filterMap_t *a, const filterMap_t *b )
	{
		unsigned short ca = a->count;
		unsigned short cb = b->count;
		return ca < cb ? 1 : (ca > cb ? -1 : 0);
	}

private:
	char display[128];
	unsigned short count;

	void UpdateDisplay()
	{
		snprintf( display, sizeof( display ), "(%d) %s", count, name );
	}
};

class CMenuGameListModel : public CMenuBaseModel
{
public:
	CMenuGameListModel( CMenuServerBrowser *parent ) :
		CMenuBaseModel(), parent( parent ), m_iSortingColumn(-1)
	{
		filterPing = MAX_PING * 1000.0f;
		filterEmpty = 0;
		filterFull = 0;
	}

	void Update() override;

	int GetColumns() const override
	{
		return COLUMN_LAST; // havePassword, game, mapname, maxcl, ping, (hidden)ip
	}

	int GetRows() const override
	{
		return servers.Count();
	}

	ECellType GetCellType( int line, int column ) override
	{
		if( column == COLUMN_PASSWORD || column == COLUMN_FAVORITE )
			return CELL_IMAGE_ADDITIVE;
		return CELL_TEXT;
	}

	const char *GetCellText( int line, int column ) override
	{
		switch( column )
		{
		case COLUMN_PASSWORD: return servers[line].havePassword ? ART_BANNER_LOCK : NULL;
		case COLUMN_FAVORITE: return servers[line].favorite ? ART_BANNER_FAVORITE : NULL;
		case COLUMN_NAME: return servers[line].name;
		case COLUMN_MAP: return servers[line].mapname;
		case COLUMN_PLAYERS: return servers[line].clientsstr;
		case COLUMN_PING: return servers[line].pingstr;
		case COLUMN_IP: return servers[line].ipstr;
		default: return NULL;
		}
	}

	bool GetCellColors( int line, int column, unsigned int &textColor, bool &force) const override
	{
		if( servers[line].isLegacy )
		{
			CColor color = uiPromptTextColor;
			color.a = color.a * 0.5;
			textColor = color;

			// allow colorstrings only in server name
			force = column != COLUMN_NAME;

			return true;
		}
		return false;
	}

	void OnActivateEntry( int line ) override;

	void Flush()
	{
		filterMaps.RemoveAll();
		servers.RemoveAll();
		serversRefreshTime = gpGlobals->time;
	}

	bool IsHavePassword( int line )
	{
		return servers[line].havePassword;
	}

	void AddServerToList( netadr_t adr, const char *info, bool is_favorite );

	bool Sort( int column, bool ascend ) override;

	void SetFilterMap( const char *mapname )
	{
		filterMap = filterMap_t( mapname );
	}

	float serversRefreshTime;
	float filterPing;
	char filterEmpty;
	char filterFull;
	CUtlVector<server_t> servers;

	filterMap_t filterMap;
	CUtlVector<filterMap_t> filterMaps;
private:
	CMenuServerBrowser *parent;

	int m_iSortingColumn;
	bool m_bAscend;
};

class CMenuServerBrowser: public CMenuFramework
{
public:
	CMenuServerBrowser() : CMenuFramework( "CMenuServerBrowser" ), gameListModel( this ) { }
	void Draw() override;
	void Show() override;
	void Hide() override;
	bool KeyUp( int key ) override;

	void SetLANOnly( bool lanOnly )
	{
		m_bLanOnly = lanOnly;
	}
	void GetGamesList( void );
	void ClearList( void );
	void RefreshList( void );
	void JoinGame( void );
	void ResetPing( void )
	{
		gameListModel.serversRefreshTime = EngFuncs::DoubleTime();
	}
	void ViewGameInfo( void );
	void OnTabSwitch( void );

	void ParseServerListFromFile( const char *filename, CUtlVector<favlist_entry_t> &list );
	void SaveServerListToFile( const char *filename, const CUtlVector<favlist_entry_t> &list );
	void QueryServerList( const CUtlVector<favlist_entry_t> &list );
	void FavoriteServer( void );
	void MaybeEnableFavoriteButton( void );
	void ToggleFavoriteButton( bool en );
	void OnChangeSelectedServer( void );
	void SaveLists( void );

	void ShowAddServerBox( void );
	void AddServer( void );

	void AddServerToList( netadr_t adr, const char *info );

	static void Connect( server_t &server );

	CMenuPicButton *joinGame;
	CMenuPicButton *createGame;
	CMenuPicButton *refresh;
	CMenuPicButton *viewGameInfo;
	CMenuPicButton *favorite;
	CMenuPicButton *addServer;
	CMenuSwitch tabSwitch; // not actually tabs

	CMenuDropDownFloat filterPing;
	CMenuDropDownInt filterEmpty;
	CMenuDropDownInt filterFull;
	CMenuDropDownStr filterMap;

	CMenuYesNoMessageBox msgBox;
	CMenuTable	gameList;
	CMenuGameListModel gameListModel;
	CMenuCheckBox showip;

	CMenuYesNoMessageBox askPassword;
	CMenuField password;

	CMenuYesNoMessageBox addServerBox;
	CMenuField addressField;
	CMenuSpinControl serverProtocol;

	int	  refreshTime;
	int   refreshTime2;

	bool m_bLanOnly;

	CUtlVector<favlist_entry_t> favoritesList;
	CUtlVector<favlist_entry_t> historyList;
private:
	void _Init() override;
	void _VidInit() override;
};

static server_t staticServerSelect( netadr_t(), "", false );
static bool staticWaitingPassword = false;

ADD_MENU3( menu_internetgames, CMenuServerBrowser, UI_InternetGames_Menu );
ADD_MENU4( menu_langame, NULL, UI_LanGame_Menu, NULL );

/*
=================
CMenuServerBrowser::Menu
=================
*/
void UI_ServerBrowser_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	// stop demos to allow network sockets to open
	if ( gpGlobals->demoplayback && EngFuncs::GetCvarFloat( "cl_background" ))
	{
		uiStatic.m_iOldMenuDepth = uiStatic.menu.Count();
		EngFuncs::ClientCmd( FALSE, "stop\n" );
		uiStatic.m_fDemosPlayed = true;
	}

	menu_internetgames->Show();
}

void UI_InternetGames_Menu( void )
{
	menu_internetgames->SetLANOnly( false );

	UI_ServerBrowser_Menu();
}

void UI_LanGame_Menu( void )
{
	menu_internetgames->SetLANOnly( true );

	UI_ServerBrowser_Menu();
}

server_t::server_t( netadr_t adr, const char *info, bool is_favorite, bool pending_info ) :
	adr( adr ), favorite( is_favorite ), pending_info( pending_info )
{
	Q_strncpy( this->info, info, sizeof( this->info ));
}

void server_t::UpdateData( void )
{
	Q_strncpy( name, Info_ValueForKey( info, "host" ), sizeof( name ));
	Q_strncpy( mapname, Info_ValueForKey( info, "map" ), sizeof( mapname ));
	Q_strncpy( ipstr, EngFuncs::NET_AdrToString( adr ), sizeof( ipstr ));
	numcl = atoi( Info_ValueForKey( info, "numcl" ));
	maxcl = atoi( Info_ValueForKey( info, "maxcl" ));
	snprintf( clientsstr, sizeof( clientsstr ), "%d\\%d", numcl, maxcl );
	havePassword = !strcmp( Info_ValueForKey( info, "password" ), "1" );
	isGoldSrc = !strcmp( Info_ValueForKey( info, "gs" ), "1" );
	isLegacy = !strcmp( Info_ValueForKey( info, "legacy" ), "1" );
}

void server_t::SetPing( float ping )
{
	ping = bound( 0.0f, ping, MAX_PING );

	if( isLegacy )
		ping /= 2;

	this->ping = ping;
	snprintf( pingstr, sizeof( pingstr ), "%.f ms", ping * 1000 );
}

bool CMenuGameListModel::Sort(int column, bool ascend)
{
	m_iSortingColumn = column;
	if( column == -1 )
		return false; // disabled

	m_bAscend = ascend;
	switch( column )
	{
	case COLUMN_PASSWORD:
		return false;
	case COLUMN_NAME:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::NameCmpAscend : server_t::NameCmpDescend );
		return true;
	case COLUMN_MAP:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::MapCmpAscend : server_t::MapCmpDescend );
		return true;
	case COLUMN_PLAYERS:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::ClientCmpAscend : server_t::ClientCmpDescend );
		return true;
	case COLUMN_PING:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::PingCmpAscend : server_t::PingCmpDescend );
		return true;
	case COLUMN_IP:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::AdrCmpAscend : server_t::AdrCmpDescend );
		return true;
	}

	return false;
}

/*
=================
CMenuServerBrowser::GetGamesList
=================
*/
void CMenuGameListModel::Update( void )
{
	int		i;

	// regenerate table data
	for( i = 0; i < servers.Count(); i++ )
		servers[i].UpdateData();

	if( servers.Count() )
	{
		parent->joinGame->SetGrayed( false );
		parent->MaybeEnableFavoriteButton();
		parent->OnChangeSelectedServer();
		if( m_iSortingColumn != -1 )
			Sort( m_iSortingColumn, m_bAscend );
	}
}

void CMenuGameListModel::OnActivateEntry( int line )
{
	CMenuServerBrowser::Connect( servers[line] );
}

void CMenuGameListModel::AddServerToList( netadr_t adr, const char *info, bool is_favorite )
{
	int i;
	int pos = -1;

	// ignore if duplicated
	for( i = 0; i < servers.Count(); i++ )
	{
		if( !EngFuncs::NET_CompareAdr( &servers[i].adr, &adr ))
		{
			if( servers[i].pending_info )
			{
				pos = i;
				break;
			}
			return;
		}

		if( !stricmp( servers[i].info, info ))
			return;
	}

	server_t server( adr, info, is_favorite );

	server.UpdateData();
	server.SetPing( EngFuncs::DoubleTime() - serversRefreshTime );

	if( server.ping > filterPing )
		return;

	if( filterEmpty == '1' && !server.IsEmpty( ))
		return;
	if( filterEmpty == '0' && server.IsEmpty( ))
		return;

	if( filterFull == '1' && !server.IsFull( ))
		return;
	if( filterFull == '0' && server.IsFull( ))
		return;

	if( filterMap.name[0] && colorstricmp( filterMap.name, server.mapname ) != 0 )
		return;

	if( server.mapname[0] != 0 )
	{
		bool foundMap = false;

		for( int i = 0; i < filterMaps.Count(); ++i )
		{
			foundMap = filterMaps[i] == server.mapname;

			if( foundMap )
			{
				filterMaps[i].AddCount( server.numcl );
				break;
			}
		}

		if( !foundMap )
		{
			filterMaps.AddToTail( filterMap_t( server.mapname, server.numcl ) );

			// sort by players count
			filterMaps.Sort( filterMap_t::CmpCountInvert );

			parent->filterMap.Clear();
			for( int i = Q_min( FILTER_MAX_MAPS, filterMaps.Count() ); i--; )
				parent->filterMap.AddItem( filterMaps[i].GetDisplay(), filterMaps[i].name );
			parent->filterMap.AddItem( L( "any map" ), "" );

			if( !filterMap.name[0] )
				parent->filterMap.SelectLast( false );
		}
	}

	if( pos >= 0 )
		servers[pos] = server;
	else
		servers.AddToTail( server );

	if( m_iSortingColumn != -1 )
		Sort( m_iSortingColumn, m_bAscend );
}

void CMenuServerBrowser::Connect( server_t &server )
{
	// prevent refresh during connect
	menu_internetgames->refreshTime = uiStatic.realTime + 999999;

	// ask user for password
	if( server.havePassword )
	{
		// if dialog window is still open, then user have entered the password
		if( !staticWaitingPassword )
		{
			// save current select
			staticServerSelect = server;
			staticWaitingPassword = true;

			// show password request window
			menu_internetgames->askPassword.Show();

			return;
		}
	}
	else
	{
		// remove password, as server don't require it
		EngFuncs::CvarSetString( "password", "" );
	}

	staticWaitingPassword = false;

	const char *sadr = EngFuncs::NET_AdrToString( server.adr );
	const char *prot = server.ToProtocol();

	if( menu_internetgames->m_bLanOnly == false )
	{
		if( menu_internetgames->historyList.Count() > 20 ) // FIXME: make configurable
			menu_internetgames->historyList.FastRemove( 0 );
		menu_internetgames->historyList.AddToTail( favlist_entry_t( sadr, prot, true ) );


		menu_internetgames->SaveLists();
	}

	EngFuncs::ClientCmdF( false, "connect \"%s\" \"%s\"\n", sadr, prot );

	UI_ConnectionProgress_Connect( "" );
}

/*
=================
CMenuServerBrowser::JoinGame
=================
*/
void CMenuServerBrowser::JoinGame()
{
	gameListModel.OnActivateEntry( gameList.GetCurrentIndex() );
}

void CMenuServerBrowser::FavoriteServer()
{
	int i = gameList.GetCurrentIndex();

	if( !gameListModel.servers.IsValidIndex( i ))
		return;

	server_t &serv = gameListModel.servers[i];
	const char *sadr = EngFuncs::NET_AdrToString( serv.adr );

	serv.favorite = !serv.favorite;

	ToggleFavoriteButton( !serv.favorite );

	if( serv.favorite )
	{
		favlist_entry_t entry( sadr, serv.ToProtocol() );
		favoritesList.AddToTail( entry );
	}
	else
	{
		FOR_EACH_VEC( favoritesList, i )
		{
			if( !strcmp( favoritesList[i].sadr, sadr ))
			{
				favoritesList.FastRemove( i );
				break;
			}
		}
	}
}

void CMenuServerBrowser::MaybeEnableFavoriteButton( void )
{
	bool is_nat = EngFuncs::GetCvarFloat( "cl_nat" ) > 0;

	favorite->SetGrayed( is_nat );
}

void CMenuServerBrowser::ToggleFavoriteButton( bool en )
{
	if( en )
	{
		favorite->szName = L( "Favorite" );
		favorite->SetPicture( PC_FAVORITE, 'o' );
	}
	else
	{
		favorite->szName = L( "Unfavorite" );
		favorite->SetPicture( PC_UNFAVORITE, 'o' );
	}
}

void CMenuServerBrowser::ClearList()
{
	filterMap.Clear();
	if( gameListModel.filterMap.name[0] )
		filterMap.AddItem( gameListModel.filterMap.GetDisplay(), gameListModel.filterMap.name );
	filterMap.AddItem( L( "any map" ), "" );

	gameListModel.Flush();
	joinGame->SetGrayed( true );
	viewGameInfo->SetGrayed( true );
	favorite->SetGrayed( true );
}

void CMenuServerBrowser::ParseServerListFromFile( const char *filename, CUtlVector<favlist_entry_t> &list )
{
	byte *pfile = EngFuncs::COM_LoadFile( filename );
	char *afile = (char *)pfile;

	while( true )
	{
		favlist_entry_t entry( "", "", true );

		afile = EngFuncs::COM_ParseFile( afile, entry.sadr, sizeof( entry.sadr ));
		if( !afile )
			break;

		afile = EngFuncs::COM_ParseFile( afile, entry.prot, sizeof( entry.prot ));
		if( !afile )
			break;

		list.AddToTail( entry );
	}

	EngFuncs::COM_FreeFile( pfile );
}

void CMenuServerBrowser::SaveServerListToFile( const char *filename, const CUtlVector<favlist_entry_t> &list )
{
	CUtlString s;

	if( list.Count() == 0 )
	{
		EngFuncs::DeleteFile( filename );
		return;
	}

	FOR_EACH_VEC( list, i )
	{
		if( !list[i].favorited )
			continue;

		s.AppendFormat( "%s %s\n", list[i].sadr, list[i].prot );
	}

	EngFuncs::COM_SaveFile( filename, s.Get( ), s.Length( ));
}

void CMenuServerBrowser::QueryServerList( const CUtlVector<favlist_entry_t> &list )
{
	FOR_EACH_VEC( list, i )
	{
		netadr_t adr;

		if( !EngFuncs::textfuncs.pNetAPI->StringToAdr( (char *)list[i].sadr, &adr ))
			continue;

		bool found = false;

		FOR_EACH_VEC( gameListModel.servers, j )
		{
			if( !EngFuncs::NET_CompareAdr( &gameListModel.servers, &adr ))
			{
				found = true;
				break;
			}
		}

		if( !found )
		{
			CUtlString fakeInfoString;
			list[i].GenerateDummyInfoString( fakeInfoString );

			server_t serv( adr, fakeInfoString, list[i].favorited, true );
			serv.UpdateData();
			serv.SetPing( 9.999f );

			gameListModel.servers.AddToTail( serv );
		}

		list[i].QueryServer();
	}

	UI_MenuResetPing_f();
}

void CMenuServerBrowser::RefreshList()
{
	ClearList();

	if( m_bLanOnly )
	{
		EngFuncs::ClientCmd( FALSE, "localservers\n" );
	}
	else if( uiStatic.realTime > refreshTime2 )
	{
		if( tabSwitch.GetState() == 2 )
			QueryServerList( favoritesList );
		else if( tabSwitch.GetState() == 3 )
			QueryServerList( historyList );
		else
		{
			char filter[128] = "";
			char *buf = filter;
			int remaining = sizeof(filter);

			if( filterEmpty.GetItem( ) )
			{
				int n = snprintf(buf, remaining, "\\empty\\%c", (char) filterEmpty.GetItem( ) );
				remaining -= n;
				buf += n;
			}

			if( filterFull.GetItem( ) )
			{
				int n = snprintf(buf, remaining, "\\full\\%c", (char) filterFull.GetItem( ) );
				remaining -= n;
				buf += n;
			}

			if( filterMap.GetItem( )[0] )
			{
				int n = snprintf(buf, remaining, "\\map\\%s", filterMap.GetItem( ) );
				remaining -= n;
				buf += n;
			}

			EngFuncs::ClientCmdF( FALSE, "internetservers %s\n", filter );
		}

		refreshTime2 = uiStatic.realTime + (EngFuncs::GetCvarFloat("cl_nat") ? 4000:1000);
		refresh->SetGrayed( true );
		if( uiStatic.realTime + 20000 < refreshTime )
			refreshTime = uiStatic.realTime + 20000;
	}
}

void CMenuServerBrowser::ViewGameInfo()
{
	int idx = gameList.GetCurrentIndex();

	if( idx < 0 || idx >= gameListModel.GetRows( ))
		return;

	UI_ServerInfo_Menu( gameListModel.servers[idx].adr, gameListModel.servers[idx].name, gameListModel.servers[idx].isLegacy );
}

void CMenuServerBrowser::OnTabSwitch()
{
	int idx = tabSwitch.GetState();

	switch( idx )
	{
	case 0: // Direct
	case 1: // NAT
		EngFuncs::CvarSetValue( "cl_nat", idx );
		break;
	default: // Favorites and History
		EngFuncs::CvarSetValue( "cl_nat", 0.0f );
		break;
	}

	ClearList();
	RefreshList();
}

void CMenuServerBrowser::OnChangeSelectedServer( void )
{
	int i = gameList.GetCurrentIndex();

	if( !gameListModel.servers.IsValidIndex( i ))
		return;

	bool fav = gameListModel.servers[i].favorite;
	ToggleFavoriteButton( !fav );
}

void CMenuServerBrowser::ShowAddServerBox( void )
{
	addServerBox.Show();
}

void CMenuServerBrowser::AddServer( void )
{
	netadr_t adr;

	if( !EngFuncs::textfuncs.pNetAPI->StringToAdr( (char *)addressField.GetBuffer(), &adr ))
	{
		UI_ShowMessageBox( L( "Invalid address" ));
		return;
	}

	const char *proto;

	switch( (int)serverProtocol.GetCurrentValue( ))
	{
	case 0:
		proto = "49";
		break;
	case 1:
		proto = "48";
		break;
	case 2:
		proto = "gs";
		break;
	default:
		UI_ShowMessageBox( L( "Invalid protocol" ));
		return;
	}

	// FIXME: for now we can only show custom servers at favorites tab

	favlist_entry_t entry( addressField.GetBuffer(), proto, false );
	favoritesList.AddToTail( entry );

	if( tabSwitch.GetState() != 2 )
		tabSwitch.SetState( 2 );

	CUtlString fakeInfoString;
	entry.GenerateDummyInfoString( fakeInfoString );

	server_t serv( adr, fakeInfoString, false, true );
	serv.UpdateData();
	serv.SetPing( 9.999f );
	gameListModel.servers.AddToTail( serv );

	entry.QueryServer();
	UI_MenuResetPing_f();
}

/*
=================
UI_Background_Ownerdraw
=================
*/
void CMenuServerBrowser::Draw( void )
{
	CMenuFramework::Draw();

	if( uiStatic.realTime > refreshTime )
	{
		RefreshList();
		refreshTime = uiStatic.realTime + 20000; // refresh every 10 secs
	}

	if( uiStatic.realTime > refreshTime2 )
	{
		refresh->SetGrayed( false );
	}
}

bool CMenuServerBrowser::KeyUp( int key )
{
	if( key == 'i' )
	{
		gameList.SetColumnWidth( COLUMN_IP, 300, true );

		gameList.VidInit();
		gameListModel.Update();
	}

	return CMenuFramework::KeyUp( key );
}

/*
=================
CMenuServerBrowser::Init
=================
*/
void CMenuServerBrowser::_Init( void )
{
	AddItem( banner );

	joinGame = AddButton( L( "Join game" ), nullptr, PC_JOIN_GAME, VoidCb( &CMenuServerBrowser::JoinGame ), QMF_GRAYED );
	joinGame->onReleasedClActive = msgBox.MakeOpenEvent();

	createGame = AddButton( L( "GameUI_GameMenu_CreateServer" ), NULL, PC_CREATE_GAME );
	SET_EVENT_MULTI( createGame->onReleased,
	{
		if( ((CMenuServerBrowser*)pSelf->Parent())->m_bLanOnly )
			EngFuncs::CvarSetValue( "public", 0.0f );
		else EngFuncs::CvarSetValue( "public", 1.0f );

		UI_CreateGame_Menu();
	});

	viewGameInfo = AddButton( L( "View game info" ), nullptr, PC_VIEW_GAME_INFO, VoidCb( &CMenuServerBrowser::ViewGameInfo ), QMF_GRAYED );
	favorite = AddButton( L( "Favorite" ), nullptr, PC_FAVORITE, VoidCb( &CMenuServerBrowser::FavoriteServer ), 0, 'o' );
	refresh = AddButton( L( "Refresh" ), nullptr, PC_REFRESH, VoidCb( &CMenuServerBrowser::RefreshList ) );
	addServer = AddButton( L( "Add server" ), nullptr, PC_ADD_SERVER, VoidCb( &CMenuServerBrowser::ShowAddServerBox ));

	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuServerBrowser::Hide ) );

	msgBox.SetMessage( L( "Join a network game will exit any current game, OK to exit?" ) );
	msgBox.SetPositiveButton( L( "GameUI_OK" ), PC_OK );
	msgBox.HighlightChoice( CMenuYesNoMessageBox::HIGHLIGHT_YES );
	msgBox.onPositive = VoidCb( &CMenuServerBrowser::JoinGame );
	msgBox.Link( this );

	gameList.SetCharSize( QM_SMALLFONT );
	gameList.SetupColumn( COLUMN_PASSWORD, NULL, 32.0f, true );
	gameList.SetupColumn( COLUMN_FAVORITE, NULL, 32.0f, true );
	gameList.SetupColumn( COLUMN_NAME, L( "Name" ), 0.40f );
	gameList.SetupColumn( COLUMN_MAP, L( "GameUI_Map" ), 0.25f );
	gameList.SetupColumn( COLUMN_PLAYERS, L( "Players" ), 100.0f, true );
	gameList.SetupColumn( COLUMN_PING, L( "Ping" ), 120.0f, true );
	gameList.SetupColumn( COLUMN_IP, L( "IP" ), 0, true );
	gameList.SetModel( &gameListModel );
	gameList.bFramedHintText = true;
	gameList.bAllowSorting = true;
	gameList.onChanged = VoidCb( &CMenuServerBrowser::OnChangeSelectedServer );
	gameList.SetSize( -20, 465 );

	tabSwitch.SetRect( 360, 230, -20, 32 );
	tabSwitch.AddSwitch( L( "Direct" ));
	tabSwitch.AddSwitch( "NAT" ); // intentionally not localized
	tabSwitch.AddSwitch( L( "Favorites" ));
	tabSwitch.AddSwitch( L( "History" ));
	tabSwitch.eTextAlignment = QM_CENTER;
	tabSwitch.bMouseToggle = false;
	tabSwitch.bKeepToggleWidth = true;
	tabSwitch.iSelectColor = uiInputFgColor;
	tabSwitch.iFgTextColor = uiInputFgColor - 0x00151515; // bit darker
	tabSwitch.onChanged = VoidCb( &CMenuServerBrowser::OnTabSwitch );

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
		createGame->SetGrayed( true );	// server.dll is missed - remote servers only

	password.bHideInput = true;
	password.bAllowColorstrings = false;
	password.bNumbersOnly = false;
	password.szName = L( "GameUI_Password" );
	password.iMaxLength = 16;
	password.SetRect( 188, 140, 270, 32 );

	SET_EVENT_MULTI( askPassword.onPositive,
	{
		CMenuServerBrowser *parent = (CMenuServerBrowser*)pSelf->Parent();

		EngFuncs::CvarSetString( "password", parent->password.GetBuffer() );
		parent->password.Clear(); // we don't need entered password anymore
		CMenuServerBrowser::Connect( staticServerSelect );
	});

	SET_EVENT_MULTI( askPassword.onNegative,
	{
		CMenuServerBrowser *parent = (CMenuServerBrowser*)pSelf->Parent();

		EngFuncs::CvarSetString( "password", "" );
		parent->password.Clear(); // we don't need entered password anymore
		staticWaitingPassword = false;
	});

	askPassword.SetMessage( L( "GameUI_PasswordPrompt" ) );
	askPassword.Link( this );
	askPassword.Init();
	askPassword.AddItem( password );

	addressField.bAllowColorstrings = false;
	addressField.szName = NULL;
	addressField.eTextAlignment = QM_LEFT;
	addressField.SetRect( 64, 150, 512, 32 );

	static const char *protlist[] =
	{
		"Xash3D 49 (New)",
		"Xash3D 48 (Old)",
		"GoldSource 48",
	};
	static CStringArrayModel protlistModel( protlist, V_ARRAYSIZE( protlist ));

	serverProtocol.Setup( &protlistModel );
	serverProtocol.SetCurrentValue( 0.0f );
	serverProtocol.eTextAlignment = QM_LEFT;
	serverProtocol.SetRect( 64, 100, 512, 32 );\

	addServerBox.onPositive = VoidCb( &CMenuServerBrowser::AddServer );
	addServerBox.SetMessage( L( "Enter server Internet address\n(e.g., 209.255.10.255:27015)" ));
	addServerBox.dlgMessage1.SetCoord( 8, 8 ); // a bit offset
	addServerBox.dlgMessage1.eTextAlignment = QM_TOPLEFT;
	addServerBox.Link( this );
	addServerBox.Init();
	addServerBox.AddItem( serverProtocol );
	addServerBox.AddItem( addressField );

	filterPing.AddItem( "500ms", 500.0f );
	filterPing.AddItem( "200ms", 200.0f );
	filterPing.AddItem( "100ms", 100.0f );
	filterPing.AddItem( "50ms", 50.0f );
	filterPing.AddItem( "20ms", 20.0f );
	filterPing.AddItem( L( "ping" ), MAX_PING * 1000.0f );
	filterPing.SelectLast( );
	filterPing.bDropUp = true;
	filterPing.eTextAlignment = QM_LEFT;
	filterPing.iFgTextColor = uiInputFgColor - 0x00151515;
	filterPing.SetCharSize( QM_SMALLFONT );
	filterPing.SetSize( 70, 30 );
	SET_EVENT_MULTI( filterPing.onChanged,
	{
		CMenuDropDownFloat *self = (CMenuDropDownFloat*)pSelf;
		CMenuServerBrowser *parent = (CMenuServerBrowser*)self->Parent();

		parent->gameListModel.filterPing = self->GetItem( ) / 1000.0f;
		parent->RefreshList();
	});

	filterEmpty.AddItem( L( "empty" ), '1' );
	filterEmpty.AddItem( L( "not empty" ), '0' );
	filterEmpty.AddItem( L( "any" ), 0 );
	filterEmpty.SelectLast( false );
	filterEmpty.bDropUp = true;
	filterEmpty.eTextAlignment = QM_LEFT;
	filterEmpty.iFgTextColor = uiInputFgColor - 0x00151515;
	filterEmpty.SetCharSize( QM_SMALLFONT );
	filterEmpty.SetSize( 120, 30 );
	SET_EVENT_MULTI( filterEmpty.onChanged,
	{
		CMenuDropDownInt *self = (CMenuDropDownInt*)pSelf;
		CMenuServerBrowser *parent = (CMenuServerBrowser*)self->Parent();

		parent->gameListModel.filterEmpty = self->GetItem( );
		parent->RefreshList();
	});

	filterFull.AddItem( L( "full" ), '1' );
	filterFull.AddItem( L( "not full" ), '0' );
	filterFull.AddItem( L( "any" ), 0 );
	filterFull.SelectLast( false );
	filterFull.bDropUp = true;
	filterFull.eTextAlignment = QM_LEFT;
	filterFull.iFgTextColor = uiInputFgColor - 0x00151515;
	filterFull.SetCharSize( QM_SMALLFONT );
	filterFull.SetSize( 120, 30 );
	SET_EVENT_MULTI( filterFull.onChanged,
	{
		CMenuDropDownInt *self = (CMenuDropDownInt*)pSelf;
		CMenuServerBrowser *parent = (CMenuServerBrowser*)self->Parent();

		parent->gameListModel.filterFull = self->GetItem( );
		parent->RefreshList();
	});

	filterMap.AddItem( L( "any map" ), "" );
	filterMap.bDropUp = true;
	filterMap.eTextAlignment = QM_LEFT;
	filterMap.iFgTextColor = uiInputFgColor - 0x00151515;
	filterMap.SetCharSize( QM_SMALLFONT );
	filterMap.SetSize( 200, 30 );
	SET_EVENT_MULTI( filterMap.onChanged,
	{
		CMenuDropDownStr *self = (CMenuDropDownStr*)pSelf;
		CMenuServerBrowser *parent = (CMenuServerBrowser*)self->Parent();

		parent->gameListModel.SetFilterMap( self->GetItem( ) );
		parent->RefreshList();
	});

	AddItem( gameList );
	AddItem( tabSwitch );

	AddItem( filterPing );
	AddItem( filterEmpty );
	AddItem( filterFull );
	AddItem( filterMap );
}

/*
=================
CMenuServerBrowser::VidInit
=================
*/
void CMenuServerBrowser::_VidInit()
{
	refreshTime = uiStatic.realTime + 500; // delay before update 0.5 sec
	refreshTime2 = uiStatic.realTime + 500;

	if( m_bLanOnly )
	{
		gameList.SetCoord( 360, 230 );
	}
	else
	{
		gameList.SetCoord( 360, 230 + tabSwitch.size.h + uiStatic.outlineWidth );
	}

	int x = gameList.pos.x;
	int y = gameList.pos.y + gameList.size.h + UI_OUTLINE_WIDTH;
	filterPing.SetCoord( x, y );
	x += filterPing.size.w + 10;
	filterEmpty.SetCoord( x, y );
	x += filterEmpty.size.w + 10;
	filterFull.SetCoord( x, y );
	x += filterFull.size.w + 10;
	filterMap.SetCoord( x, y );

	// force close all menus
	filterPing.MenuClose( );
	filterEmpty.MenuClose( );
	filterFull.MenuClose( );
	filterMap.MenuClose( );
}

void CMenuServerBrowser::Show()
{
	CMenuFramework::Show();

	if( m_bLanOnly )
	{
		banner.SetPicture( ART_BANNER_LAN );
		favorite->Hide();
		addServer->Hide();
		tabSwitch.Hide();
	}
	else
	{
		banner.SetPicture( ART_BANNER_INET );
		favorite->Show();
		addServer->Show();
		tabSwitch.Show();

		favoritesList.RemoveAll();
		historyList.RemoveAll();
		ParseServerListFromFile( "favorite_servers.lst", favoritesList );
		ParseServerListFromFile( "history_servers.lst", historyList );
	}

	RealignButtons();

	// clear out server table
	staticWaitingPassword = false;
	gameListModel.Flush();
	gameList.SetSortingColumn( COLUMN_PING );
	joinGame->SetGrayed( true );
	viewGameInfo->SetGrayed( true );
	favorite->SetGrayed( true );
}

void CMenuServerBrowser::SaveLists()
{
	// TODO: we can actually cache master server response, so every time
	// player opens up this menu, they instantly get server list at first
	// and then re-sync with master server is just delayed
	// ... but I'm tired with this, so let's do it next time :)

	if( m_bLanOnly )
		return;

	SaveServerListToFile( "favorite_servers.lst", favoritesList );
	SaveServerListToFile( "history_servers.lst", historyList );
}

void CMenuServerBrowser::Hide()
{
	SaveLists();
	CMenuFramework::Hide();
}

void CMenuServerBrowser::AddServerToList( netadr_t adr, const char *info )
{
#ifndef XASH_ALL_SERVERS
	if( stricmp( gMenu.m_gameinfo.gamefolder, Info_ValueForKey( info, "gamedir" )) != 0 )
		return;
#endif

	if( !WasInit() )
		return;

	if( !IsVisible() )
		return;

	const char *s = EngFuncs::NET_AdrToString( adr );
	bool is_favorite = false;

	FOR_EACH_VEC( favoritesList, i )
	{
		if( !strcmp( favoritesList[i].sadr, s ))
		{
			is_favorite = true;
			break;
		}
	}

	gameListModel.AddServerToList( adr, info, is_favorite );

	joinGame->SetGrayed( false );
	viewGameInfo->SetGrayed( false );
	MaybeEnableFavoriteButton();
	OnChangeSelectedServer();
}

/*
=================
UI_AddServerToList
=================
*/
void UI_AddServerToList( netadr_t adr, const char *info )
{
	if( !menu_internetgames )
		return;

	menu_internetgames->AddServerToList( adr, info );
}

/*
=================
UI_MenuResetPing_f
=================
*/
void UI_MenuResetPing_f( void )
{
	if( menu_internetgames )
		menu_internetgames->ResetPing();
}
ADD_COMMAND( menu_resetping, UI_MenuResetPing_f );
