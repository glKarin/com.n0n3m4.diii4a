/*
menu_btnsbmp_table.h - btns_main layout
Copyright (C) 2011 CrazyRussian

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef MENU_BTNSBMP_TABLE_H
#define MENU_BTNSBMP_TABLE_H
	
enum EDefaultBtns
{
	PC_NEW_GAME = 0,
	PC_RESUME_GAME,
	PC_HAZARD_COURSE,
	PC_CONFIG,
	PC_LOAD_GAME,
	PC_SAVE_LOAD_GAME,
	PC_VIEW_README,
	PC_QUIT,
	PC_MULTIPLAYER,
	PC_EASY,
	PC_MEDIUM,
	PC_DIFFICULT,
	PC_SAVE_GAME,
	PC_LOAD_GAME2,
	PC_CANCEL,
	PC_GAME_OPTIONS,
	PC_VIDEO,
	PC_AUDIO,
	PC_CONTROLS,
	PC_DONE,
	PC_QUICKSTART,
	PC_USE_DEFAULTS,
	PC_OK,
	PC_VID_OPT,
	PC_VID_MODES,
	PC_ADV_CONTROLS,
	PC_ORDER_HL,
	PC_DELETE,
	PC_INET_GAME,
	PC_CHAT_ROOMS,
	PC_LAN_GAME,
	PC_CUSTOMIZE,
	PC_SKIP,
	PC_EXIT,
	PC_CONNECT,
	PC_REFRESH,
	PC_FILTER,
	PC_FILTER2,
	PC_CREATE,
	PC_CREATE_GAME,
	PC_CHAT_ROOMS2,
	PC_LIST_ROOMS,
	PC_SEARCH,
	PC_SERVERS,
	PC_JOIN,
	PC_FIND,
	PC_CREATE_ROOM,
	PC_JOIN_GAME,
	PC_SEARCH_GAMES,
	PC_FIND_GAME,
	PC_START_GAME,
	PC_VIEW_GAME_INFO,
	PC_UPDATE,
	PC_ADD_SERVER,
	PC_DISCONNECT,
	PC_CONSOLE,
	PC_CONTENT_CONTROL,
	PC_UPDATE2,
	PC_VISIT_WON,
	PC_PREVIEWS,
	PC_ADV_OPT,
	PC_3DINFO_SITE,
	PC_CUSTOM_GAME,
	PC_ACTIVATE,
	PC_INSTALL,
	PC_VISIT_WEB_SITE,
	PC_REFRESH_LIST,
	PC_DEACTIVATE,
	PC_ADV_OPT2,
	PC_SPECTATE_GAME,
	PC_SPECTATE_GAMES,
	PC_BUTTONCOUNT		// must be last
};

// Non-standard button images start here
#define PC_GAMEPAD       "gfx/shell/btn_gamepad"
#define PC_TOUCH         "gfx/shell/btn_touch"
#define PC_TOUCH_OPTIONS "gfx/shell/btn_touch_options"
#define PC_TOUCH_BUTTONS "gfx/shell/btn_touch_buttons"
#define PC_FAVORITE      "gfx/shell/btn_favorite"
#define PC_UNFAVORITE    "gfx/shell/btn_unfavorite"
#define PC_CROSSHAIR	 "gfx/shell/btn_xhair"

#define BUTTON_NOFOCUS	0
#define BUTTON_FOCUS	1
#define BUTTON_PRESSED	2

#endif//MENU_BTNSBMP_TABLE_H
