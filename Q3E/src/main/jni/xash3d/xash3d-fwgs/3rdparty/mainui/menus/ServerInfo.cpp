/*
Copyright (C) 2024 Alibek Omarov

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

#include <time.h>
#include "Framework.h"
#include "Table.h"
#include "Action.h"
#include "PicButton.h"
#include "utlvector.h"
#include "utlstring.h"

#define ART_BANNER "gfx/shell/head_multi"

enum
{
	COLUMN_POSITION = 0,
	COLUMN_NAME,
	COLUMN_FRAGS,
	COLUMN_TIME,

	COLUMN_RULE = 0,
	COLUMN_VALUE
};

struct player_entry_t
{
	player_entry_t( int index, const char *name, int frags, float time ) :
		index( index ), name( name ), frags( frags ), time( time )
	{
		index_str.Format( "%i", index );
		frags_str.Format( "%i", frags );

		int hours = time / 3600;
		int minutes = ( time - hours * 3600 ) / 60;
		int seconds = ( time - hours * 3600 - minutes * 60 );

		if( hours != 0 )
			time_str.Format( "%02i:%02i:%02i", hours, minutes, seconds );
		else
			time_str.Format( "%02i:%02i", minutes, seconds );
	}

	CUtlString index_str;
	int index;

	CUtlString name;

	CUtlString frags_str;
	int frags;

	CUtlString time_str;
	float time;

	int IndexCmp( const player_entry_t &other ) const
	{
		if( index > other.index ) return 1;
		else if( index < other.index ) return -1;
		return 0;
	}

	int NameCmp( const player_entry_t &other ) const
	{
		return colorstricmp( name.String(), other.name.String() );
	}

	int FragsCmp( const player_entry_t &other ) const
	{
		if( frags > other.frags ) return 1;
		else if( frags < other.frags ) return -1;
		return 0;
	}

	int TimeCmp( const player_entry_t &other ) const
	{
		if( time > other.time ) return 1;
		else if( time < other.time ) return -1;
		return 0;
	}

#define GENERATE_COMPAR_FN( method ) \
	static int method ## Ascend( const player_entry_t *a, const player_entry_t *b ) \
	{\
		return a->method( *b );\
	}\
	static int method ## Descend( const player_entry_t *a, const player_entry_t *b ) \
	{\
		return b->method( *a );\
	}\

	GENERATE_COMPAR_FN( IndexCmp )
	GENERATE_COMPAR_FN( NameCmp )
	GENERATE_COMPAR_FN( FragsCmp )
	GENERATE_COMPAR_FN( TimeCmp )

#undef GENERATE_COMPAR_FN
};

struct server_rule_t
{
	CUtlString rule;
	CUtlString value;
};

class CMenuPlayerListModel : public CMenuBaseModel
{
public:
	CMenuPlayerListModel() : CMenuBaseModel() {}

	void Update() override { }

	int GetColumns() const override { return 4;	}
	int GetRows() const override { return players.Count(); }

	bool Sort( int column, bool ascend ) override
	{
		switch( column )
		{
		case COLUMN_POSITION:
			players.Sort( ascend ? player_entry_t::IndexCmpAscend : player_entry_t::IndexCmpDescend );
			return true;
		case COLUMN_NAME:
			players.Sort( ascend ? player_entry_t::NameCmpAscend : player_entry_t::NameCmpDescend );
			return true;
		case COLUMN_FRAGS:
			players.Sort( ascend ? player_entry_t::FragsCmpAscend : player_entry_t::FragsCmpDescend );
			return true;
		case COLUMN_TIME:
			players.Sort( ascend ? player_entry_t::TimeCmpAscend : player_entry_t::TimeCmpDescend );
			return true;
		}

		return false;
	}

	const char *GetCellText( int line, int column ) override
	{
		switch( column )
		{
		case COLUMN_POSITION: return players[line].index_str.String();
		case COLUMN_NAME: return players[line].name.String();
		case COLUMN_FRAGS: return players[line].frags_str.String();
		case COLUMN_TIME: return players[line].time_str.String();
		}
		return nullptr;
	}

	CUtlVector<player_entry_t> players;
};

class CMenuServerRuleModel : public CMenuBaseModel
{
public:
	CMenuServerRuleModel() : CMenuBaseModel() {}

	void Update() override { }
	int GetColumns() const override { return 2; }
	int GetRows() const override { return rules.Count(); }
	const char *GetCellText( int line, int column ) override
	{
		switch( column )
		{
		case COLUMN_RULE: return rules[line].rule.String();
		case COLUMN_VALUE: return rules[line].value.String();
		}
		return nullptr;
	}

	CUtlVector<server_rule_t> rules;
};

class CMenuServerInfo : public CMenuFramework
{
public:
	CMenuServerInfo() : CMenuFramework( "CMenuServerInfo" ) {}

	void Set( netadr_t adr, const char *hostname, bool legacy )
	{
		m_adr = adr;
		m_legacy = legacy;

		Q_strncpy( server_hostname_str, hostname, sizeof( server_hostname_str ));
		server_hostname.szName = server_hostname_str;

		Q_strncpy( server_address_str, EngFuncs::NET_AdrToString( adr ), sizeof( server_address_str ));
		server_address.szName = server_address_str;
	}

	void Show() override;
	void Hide() override;

	void DoNetworkRequests();

	void PingResponse( net_response_t *resp );
	void PlayersResponse( net_response_t *resp );
	void RulesResponse( net_response_t *resp );

	static void PingResponseFunc( net_response_t *resp );
	static void PlayersResponseFunc( net_response_t *resp );
	static void RulesResponseFunc( net_response_t *resp );

private:
	void _Init() override;

	netadr_t m_adr;
	bool m_legacy;
	CMenuAction server_hostname_hint;
	CMenuAction server_hostname;
	CMenuAction server_address_hint;
	CMenuAction server_address;
	CMenuAction server_pingtime_hint;
	CMenuAction server_pingtime;

	CMenuPicButton done;

	CMenuTable players_list;
	CMenuTable rules_list;

	CMenuPlayerListModel players_model;
	CMenuServerRuleModel rules_model;

	char server_hostname_str[64];
	char server_address_str[64]; // enough for both IPv4 and IPv6
	char ping_str[16];

	int32_t ping_context;
	int32_t players_context;
	int32_t rules_context;
};

ADD_MENU( menu_serverinfo, CMenuServerInfo, UI_ServerInfo_Menu )

void UI_ServerInfo_Menu( netadr_t adr, const char *hostname, bool legacy )
{
	UI_ServerInfo_Menu();
	menu_serverinfo->Set( adr, hostname, legacy );
	menu_serverinfo->DoNetworkRequests();
}

void CMenuServerInfo::PingResponse( net_response_t *resp )
{
	if( resp->context != ping_context )
		return;

	if( resp->response == nullptr )
		return;

	snprintf( ping_str, sizeof( ping_str ), "%.f ms.", resp->ping * 1000.0f );
	server_pingtime.szName = ping_str;
}

void CMenuServerInfo::PlayersResponse( net_response_t *resp )
{
	if( resp->context != players_context )
		return;

	if( resp->response == nullptr )
		return;

	char *s = (char *)resp->response;

	if( s[0] != '\\' ) // legacy answer, which isn't an infostring
	{
		char *p;

		p = strtok( s, "\\" );

		do
		{
			char name[64];
			int frags, i;
			float time;

			if( p == nullptr )
				break;
			i = atoi( p );

			if(( p = strtok( nullptr, "\\" )) == nullptr )
				break;
			Q_strncpy( name, p, sizeof( name ));

			if(( p = strtok( nullptr, "\\" )) == nullptr )
				break;
			frags = atoi( p );

			if(( p = strtok( nullptr, "\\" )) == nullptr )
				break;
			time = atof( p );

			player_entry_t entry( i, name, frags, time );
			players_model.players.AddToTail( entry );

			p = strtok( nullptr, "\\" );
		} while( true );

	}
	else
	{
		int count = atoi( Info_ValueForKey( s, "players" ));
		players_model.players.EnsureCapacity( count );

		for( int i = 0; i < count; i++ )
		{
			char temp[64];
			const char *name;
			int frags;
			float time;

			snprintf( temp, sizeof( temp ), "p%ifrags", i );
			frags = atoi( Info_ValueForKey( s, temp ));

			snprintf( temp, sizeof( temp ), "p%itime", i );
			time = atof( Info_ValueForKey( s, temp ));

			// keep last so pointer returned by Info_ValueForKey won't be rewritten
			snprintf( temp, sizeof( temp ), "p%iname", i );
			name = Info_ValueForKey( s, temp );

			player_entry_t entry( i, name, frags, time );

			players_model.players.AddToTail( entry );
		}
	}
}

void CMenuServerInfo::RulesResponse( net_response_t *resp )
{
	if( resp->context != rules_context )
		return;

	if( resp->response == nullptr )
		return;

	if( m_legacy )
	{
		// this call is useless in old engine, it prints serverinfo
		// which doesn't contain needed information for us
	}
	else
	{
		char *s = (char *)resp->response;
		int count = atoi( Info_ValueForKey( s, "rules" ));
		char *p;

		rules_model.rules.EnsureCapacity( count );

		p = strtok( s, "\\" );

		do
		{
			server_rule_t rule;

			if( p == nullptr )
				break;
			rule.rule.SetValue( p );

			if(( p = strtok( nullptr, "\\" )) == nullptr )
				break;
			rule.value.SetValue( p );

			if( rule.rule != "rules" )
				rules_model.rules.AddToTail( rule );

			p = strtok( nullptr, "\\" );
		} while( true );
	}
}

void CMenuServerInfo::PingResponseFunc( net_response_t *resp )
{
	menu_serverinfo->PingResponse( resp );
}

void CMenuServerInfo::PlayersResponseFunc( net_response_t *resp )
{
	menu_serverinfo->PlayersResponse( resp );
}

void CMenuServerInfo::RulesResponseFunc( net_response_t *resp )
{
	menu_serverinfo->RulesResponse( resp );
}

void CMenuServerInfo::DoNetworkRequests()
{
	// always generate random context, the engine will verify it for us
	int flags = m_legacy ? FNETAPI_LEGACY_PROTOCOL : 0;
	const double timeout = 10.0;

	ping_context = EngFuncs::RandomLong( 0, 0x7fffffff );
	EngFuncs::textfuncs.pNetAPI->SendRequest( ping_context, NETAPI_REQUEST_PING, flags, timeout, &m_adr, CMenuServerInfo::PingResponseFunc );

	players_context = EngFuncs::RandomLong( 0, 0x7fffffff );
	EngFuncs::textfuncs.pNetAPI->SendRequest( players_context, NETAPI_REQUEST_PLAYERS, flags, timeout, &m_adr, CMenuServerInfo::PlayersResponseFunc );

	rules_context = EngFuncs::RandomLong( 0, 0x7fffffff );
	EngFuncs::textfuncs.pNetAPI->SendRequest( rules_context, NETAPI_REQUEST_RULES, flags, timeout, &m_adr, CMenuServerInfo::RulesResponseFunc );
}

void CMenuServerInfo::Show()
{
	CMenuFramework::Show();

	players_model.players.RemoveAll();
	rules_model.rules.RemoveAll();

	players_list.SetSortingColumn( COLUMN_POSITION, true );
}

void CMenuServerInfo::Hide()
{
	EngFuncs::textfuncs.pNetAPI->CancelRequest( rules_context );
	EngFuncs::textfuncs.pNetAPI->CancelRequest( players_context );
	EngFuncs::textfuncs.pNetAPI->CancelRequest( ping_context );

	ping_context = players_context = rules_context = -1;

	CMenuFramework::Hide();
}

void CMenuServerInfo::_Init()
{
	banner.SetPicture( ART_BANNER );

	server_hostname_hint.iFlags = QMF_INACTIVE;
	server_hostname_hint.colorBase = uiInputTextColor;
	server_hostname_hint.szName = L( "Server hostname:" );
	server_hostname_hint.SetRect( 72, 230, 250, 40 );

	server_hostname.iFlags = QMF_INACTIVE;
	server_hostname.colorBase = uiColorWhite;
	server_hostname.szName = "";
	server_hostname.SetCharSize( QM_SMALLFONT );
	server_hostname.SetRect( 72, 280, 250, 80 );

	server_address_hint.iFlags = QMF_INACTIVE;
	server_address_hint.colorBase = uiInputTextColor;
	server_address_hint.szName = L( "Server IP address:" );
	server_address_hint.SetRect( 72, 370, 250, 40 );

	server_address.iFlags = QMF_INACTIVE;
	server_address.colorBase = uiColorWhite;
	server_address.szName = "";
	server_address.SetCharSize( QM_SMALLFONT );
	server_address.SetRect( 72, 420, 250, 80 );

	server_pingtime_hint.iFlags = QMF_INACTIVE;
	server_pingtime_hint.colorBase = uiInputTextColor;
	server_pingtime_hint.szName = L( "Server 'ping' time:" );
	server_pingtime_hint.SetRect( 72, 510, 250, 40 );

	server_pingtime.iFlags = QMF_INACTIVE;
	server_pingtime.colorBase = uiColorWhite;
	server_pingtime.szName = "??? ms.";
	server_pingtime.SetCharSize( QM_SMALLFONT );
	server_pingtime.SetRect( 72, 560, 250, 80 );

	done.szName = L( "Done" );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CMenuServerInfo::Hide );
	done.SetCoord( 72, 650 );

	players_list.bAllowSorting = true;
	players_list.SetModel( &players_model );
	players_list.SetCharSize( QM_SMALLFONT );
	players_list.SetupColumn( COLUMN_POSITION, "#", 32.0f, true );
	players_list.SetupColumn( COLUMN_NAME, L( "Player Name" ), 0.60f );
	players_list.SetupColumn( COLUMN_FRAGS, L( "Kills" ), 0.10f );
	players_list.SetupColumn( COLUMN_TIME, L( "Time" ), 0.30f );
	players_list.SetRect( 360, 230,	-20, 250 );

	rules_list.SetModel( &rules_model );
	rules_list.SetupColumn( COLUMN_RULE, L( "Server Rule" ), 0.5f );
	rules_list.SetupColumn( COLUMN_VALUE, L( "Value" ), 0.5f );
	rules_list.SetRect( 360, 490, -20, 200 );

	AddItem( banner );
	AddItem( server_hostname_hint );
	AddItem( server_hostname );
	AddItem( server_address_hint );
	AddItem( server_address );
	AddItem( server_pingtime_hint );
	AddItem( server_pingtime );
	AddItem( done );
	AddItem( players_list );
	AddItem( rules_list );
}
