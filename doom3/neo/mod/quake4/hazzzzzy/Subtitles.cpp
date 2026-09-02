// karin: Quake 4 简体中文汉化项目新增 —— 语音字幕（Apex/HL2 风格整块面板）
// 面板底部锚定，新行从下方顶入，高度平滑伸缩，行淡入淡出
// GUI 资产: guis/subtitles.gui

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "Sound.h"
#include "Subtitles.h"

idCVar harm_g_subtitles( "harm_g_subtitles", "1", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "enable voice subtitles" );
idCVar harm_g_subtitleHoldTime( "harm_g_subtitleHoldTime", "700", CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "extra time(ms) subtitle stays after voice ends" );
idCVar harm_g_subtitleMinTime( "harm_g_subtitleMinTime", "1500", CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "minimum time(ms) a subtitle stays" );
idCVar harm_g_subtitleTest( "harm_g_subtitleTest", "", CVAR_GAME, "debug: push a test subtitle line" );
idCVar harm_g_subtitleDebug( "harm_g_subtitleDebug", "0", CVAR_GAME | CVAR_BOOL, "debug: print subtitle add/skip decisions to console" );
// karin: \xE5\xAD\x97\xE5\xB9\x95\xE6\xBC\x94\xE7\xA4\xBA\xE6\xA8\xA1\xE5\xBC\x8F \xE2\x80\x94\xE2\x80\x94 \xE5\xBE\xAA\xE7\x8E\xAF\xE6\xBC\x94\xE7\xA4\xBA\xE6\x89\x80\xE6\x9C\x89\xE5\xAD\x97\xE5\xB9\x95\xE7\xB1\xBB\xE5\x9E\x8B\xEF\xBC\x88\xE6\x95\x8C\xE5\x86\x9B/\xE8\xA7\x92\xE8\x89\xB2/\xE6\x97\xA0\xE7\xBA\xBF\xE7\x94\xB5/\xE5\xB9\xBF\xE6\x92\xAD/\xE9\xA9\xAC\xE5\x85\x8B\xE9\xBE\x99\xEF\xBC\x89
idCVar harm_g_subtitleDemo( "harm_g_subtitleDemo", "0", CVAR_GAME | CVAR_BOOL, "demo: cycle through all subtitle types for visual testing" );
// karin: \xE4\xBA\xBA\xE5\x90\x8D\xE6\xB1\x89\xE5\x8C\x96\xE5\xBC\x80\xE5\x85\xB3\xEF\xBC\x88\xE9\xBB\x98\xE8\xAE\xA4 0 = \xE4\xBF\x9D\xE7\x95\x99\xE8\x8B\xB1\xE6\x96\x87\xE5\x90\x8D\xEF\xBC\x89
idCVar harm_g_cnNames( "harm_g_cnNames", "0", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "localize character names to Chinese in subtitles" );

// \xE4\xBA\xBA\xE5\x90\x8D\xE6\x98\xA0\xE5\xB0\x84\xE8\xA1\xA8\xEF\xBC\x88\xE8\x8B\xB1\xE6\x96\x87 \xE2\x86\x92 \xE4\xB8\xAD\xE6\x96\x87\xEF\xBC\x89
struct NameMap { const char *en; const char *cn; };
static const NameMap nameMap[] = {
	{ "Kane", "\xe5\x87\xaf\xe6\x81\xa9" },					// \xe5\x87\xaf\xe6\x81\xa9
	{ "Voss", "\xe6\xb2\x83\xe6\x96\xaf" },					// \xe6\xb2\x83\xe6\x96\xaf
	{ "Bidwell", "\xe6\xaf\x94\xe5\xbe\xb7\xe5\xa8\x81\xe5\xb0\x94" },		// \xe6\xaf\x94\xe5\xbe\xb7\xe5\xa8\x81\xe5\xb0\x94
	{ "Morris", "\xe8\x8e\xab\xe9\x87\x8c\xe6\x96\xaf" },				// \xe8\x8e\xab\xe9\x87\x8c\xe6\x96\xaf
	{ "Strauss", "\xe6\x96\xbd\xe7\x89\xb9\xe5\x8a\xb3\xe6\x96\xaf" },		// \xe6\x96\xbd\xe7\x89\xb9\xe5\x8a\xb3\xe6\x96\xaf
	{ "Rhodes", "\xe7\xbd\x97\xe5\x85\xb9" },					// \xe7\xbd\x97\xe5\x85\xb9
	{ "Anderson", "\xe5\xae\x89\xe5\xbe\xb7\xe6\xa3\xae" },			// \xe5\xae\x89\xe5\xbe\xb7\xe6\xa3\xae
	{ "Cortez", "\xe7\xa7\x91\xe5\xb0\x94\xe7\x89\xb9\xe6\x96\xaf" },		// \xe7\xa7\x91\xe5\xb0\x94\xe7\x89\xb9\xe6\x96\xaf
	{ "Miller", "\xe7\xb1\xb3\xe5\x8b\x92" },					// \xe7\xb1\xb3\xe5\x8b\x92
	{ "Richards", "\xe7\x90\x86\xe6\x9f\xa5\xe5\x85\xb9" },			// \xe7\x90\x86\xe6\x9f\xa5\xe5\x85\xb9
	{ "Hollenbeck", "\xe9\x9c\x8d\xe4\xbc\xa6\xe8\xb4\x9d\xe5\x85\x8b" },		// \xe9\x9c\x8d\xe4\xbc\xa6\xe8\xb4\x9d\xe5\x85\x8b
	{ "Looms", "\xe5\x8d\xa2\xe5\xa7\x86\xe6\x96\xaf" },				// \xe5\x8d\xa2\xe5\xa7\x86\xe6\x96\xaf
	{ "Sledge", "\xe6\x96\xaf\xe8\x8e\xb1\xe5\xa5\x87" },				// \xe6\x96\xaf\xe8\x8e\xb1\xe5\xa5\x87
	{ "Mahler", "\xe9\xa9\xac\xe5\x8b\x92" },					// \xe9\xa9\xac\xe5\x8b\x92
	{ "Silverman", "\xe8\xa5\xbf\xe5\xb0\x94\xe5\xbc\x97\xe6\x9b\xbc" },		// \xe8\xa5\xbf\xe5\xb0\x94\xe5\xbc\x97\xe6\x9b\xbc
	{ "Harper", "\xe5\x93\x88\xe7\x8f\x80" },					// \xe5\x93\x88\xe7\x8f\x80
	{ "Scott", "\xe6\x96\xaf\xe7\xa7\x91\xe7\x89\xb9" },				// \xe6\x96\xaf\xe7\xa7\x91\xe7\x89\xb9
	{ "Walker", "\xe6\xb2\x83\xe5\x85\x8b" },					// \xe6\xb2\x83\xe5\x85\x8b
	{ "Makron", "\xe9\xa9\xac\xe5\x85\x8b\xe9\xbe\x99" },				// \xe9\xa9\xac\xe5\x85\x8b\xe9\xbe\x99
	{ "Webb", "\xe9\x9f\xa6\xe4\xbc\xaf" },					// \xe9\x9f\xa6\xe4\xbc\xaf
	{ "Jones", "\xe7\x90\xbc\xe6\x96\xaf" },					// \xe7\x90\xbc\xe6\x96\xaf
	{ "Doyle", "\xe5\xa4\x9a\xe4\xbc\x8a\xe5\xb0\x94" },				// \xe5\xa4\x9a\xe4\xbc\x8a\xe5\xb0\x94
	{ "Newberry", "\xe7\xba\xbd\xe4\xbc\xaf\xe9\x87\x8c" },			// \xe7\xba\xbd\xe4\xbc\xaf\xe9\x87\x8c
	{ "Westmore", "\xe5\xa8\x81\xe6\x96\xaf\xe7\x89\xb9\xe6\x91\xa9" },		// \xe5\xa8\x81\xe6\x96\xaf\xe7\x89\xb9\xe6\x91\xa9
	{ "Holtz", "\xe9\x9c\x8d\xe5\xb0\x94\xe8\x8c\xa8" },				// \xe9\x9c\x8d\xe5\xb0\x94\xe8\x8c\xa8
	{ "Spencer", "\xe6\x96\xaf\xe6\xbd\x98\xe5\xa1\x9e" },			// \xe6\x96\xaf\xe6\xbd\x98\xe5\xa1\x9e
	{ "Slidjonovitch", "\xe6\x96\xaf\xe5\x88\xa9\xe5\xbe\xb7\xe7\xba\xa6\xe8\xaf\xba\xe7\xbb\xb4\xe5\xa5\x87" },	// \xe6\x96\xaf\xe5\x88\xa9\xe5\xbe\xb7\xe7\xba\xa6\xe8\xaf\xba\xe7\xbb\xb4\xe5\xa5\x87
	{ "Nikolai", "\xe5\xb0\xbc\xe5\x8f\xa4\xe6\x8b\x89" },			// \xe5\xb0\xbc\xe5\x8f\xa4\xe6\x8b\x89
	{ "Lorenzi", "\xe6\xb4\x9b\xe4\xbc\xa6\xe9\xbd\x90" },			// \xe6\xb4\x9b\xe4\xbc\xa6\xe9\xbd\x90
	{ "Hammond", "\xe5\x93\x88\xe8\x92\x99\xe5\xbe\xb7" },			// \xe5\x93\x88\xe8\x92\x99\xe5\xbe\xb7
	{ "Hanks", "\xe6\xb1\x89\xe5\x85\x8b\xe6\x96\xaf" },				// \xe6\xb1\x89\xe5\x85\x8b\xe6\x96\xaf
	{ "Herman", "\xe8\xb5\xab\xe5\xb0\x94\xe6\x9b\xbc" },			// \xe8\xb5\xab\xe5\xb0\x94\xe6\x9b\xbc
	{ "Leonard", "\xe4\xbc\xa6\xe7\xba\xb3\xe5\xbe\xb7" },			// \xe4\xbc\xa6\xe7\xba\xb3\xe5\xbe\xb7
	{ "Terrance", "\xe7\x89\xb9\xe4\xbc\xa6\xe6\x96\xaf" },			// \xe7\x89\xb9\xe4\xbc\xa6\xe6\x96\xaf
	{ "Garr", "\xe5\x8a\xa0\xe5\xb0\x94" },					// \xe5\x8a\xa0\xe5\xb0\x94
	{ "Rutger", "\xe6\x8b\x89\xe7\x89\xb9\xe6\xa0\xbc" },			// \xe6\x8b\x89\xe7\x89\xb9\xe6\xa0\xbc
	{ "Mills", "\xe7\xb1\xb3\xe5\xb0\x94\xe6\x96\xaf" },			// \xe7\xb1\xb3\xe5\xb0\x94\xe6\x96\xaf
	{ "Banks", "\xe7\x8f\xad\xe5\x85\x8b\xe6\x96\xaf" },			// \xe7\x8f\xad\xe5\x85\x8b\xe6\x96\xaf
	{ "Alex", "\xe4\xba\x9a\xe5\x8e\x86\xe5\x85\x8b\xe6\x96\xaf" },		// \xe4\xba\x9a\xe5\x8e\x86\xe5\x85\x8b\xe6\x96\xaf
	{ "Biessman", "\xe6\xaf\x94\xe6\x96\xaf\xe6\x9b\xbc" },			// \xe6\xaf\x94\xe6\x96\xaf\xe6\x9b\xbc
	{ "Friedman", "\xe5\xbc\x97\xe9\x87\x8c\xe5\xbe\xb7\xe6\x9b\xbc" },		// \xe5\xbc\x97\xe9\x87\x8c\xe5\xbe\xb7\xe6\x9b\xbc
	{ "Coppel", "\xe7\xa7\x91\xe4\xbd\xa9\xe5\xb0\x94" },			// \xe7\xa7\x91\xe4\xbd\xa9\xe5\xb0\x94
	{ "Matthew", "\xe9\xa9\xac\xe4\xbf\xae" },					// \xe9\xa9\xac\xe4\xbf\xae
	{ "Paul", "\xe4\xbf\x9d\xe7\xbd\x97" },					// \xe4\xbf\x9d\xe7\xbd\x97
	{ "Mark", "\xe9\xa9\xac\xe5\x85\x8b" },					// \xe9\xa9\xac\xe5\x85\x8b
	{ "Kovitch", "\xe7\xa7\x91\xe7\xbb\xb4\xe5\xa5\x87" },			// \xe7\xa7\x91\xe7\xbb\xb4\xe5\xa5\x87 (Slidjonovitch \xe7\x9a\x84\xe7\xae\x80\xe7\xa7\xb0)
	{ "Marin", "\xe9\xa9\xac\xe6\x9e\x97" },					// \xe9\xa9\xac\xe6\x9e\x97
	{ "Chase", "\xe8\x94\xa1\xe6\x96\xaf" },					// \xe8\x94\xa1\xe6\x96\xaf
	{ "Mist", "\xe7\xb1\xb3\xe6\x96\xaf\xe7\x89\xb9" },				// \xe7\xb1\xb3\xe6\x96\xaf\xe7\x89\xb9
	{ "Ways", "\xe9\x9f\xa6\xe6\x96\xaf" },					// \xe9\x9f\xa6\xe6\x96\xaf
	{ "Dallas", "\xe8\xbe\xbe\xe6\x8b\x89\xe6\x96\xaf" },			// \xe8\xbe\xbe\xe6\x8b\x89\xe6\x96\xaf
	{ "Aldrin", "\xe5\xa5\xa5\xe5\xb0\x94\xe5\xbe\xb7\xe6\x9e\x97" },		// \xe5\xa5\xa5\xe5\xb0\x94\xe5\xbe\xb7\xe6\x9e\x97
	{ "Portsmith", "\xe6\xb3\xa2\xe7\x89\xb9\xe5\x8f\xb2\xe5\xaf\x86\xe6\x96\xaf" },	// \xe6\xb3\xa2\xe7\x89\xb9\xe5\x8f\xb2\xe5\xaf\x86\xe6\x96\xaf
	{ "Berlin", "\xe6\x9f\x8f\xe6\x9e\x97" },					// \xe6\x9f\x8f\xe6\x9e\x97
	{ "Pierce", "\xe7\x9a\xae\xe5\xb0\x94\xe6\x96\xaf" },			// \xe7\x9a\xae\xe5\xb0\x94\xe6\x96\xaf
	{ "Alejandro", "\xe4\xba\x9a\xe5\x8e\x86\xe6\x9d\xad\xe5\xbe\xb7\xe7\xbd\x97" },	// \xe4\xba\x9a\xe5\x8e\x86\xe6\x9d\xad\xe5\xbe\xb7\xe7\xbd\x97
	{ "Madison", "\xe9\xba\xa6\xe8\xbf\xaa\xe9\x80\x8a" },			// \xe9\xba\xa6\xe8\xbf\xaa\xe9\x80\x8a
	{ "Patton", "\xe5\xb7\xb4\xe9\xa1\xbf" },				// \xe5\xb7\xb4\xe9\xa1\xbf
	{ "Ja", "\xe5\x97\xaf" },						// \xe5\x97\xaf (Strauss \xe5\xbe\xb7\xe8\xaf\xad\xe5\x8f\xa3\xe5\xa4\xb4\xe7\xa6\x85)
	{ "Hillstrom", "\xe5\xb8\x8c\xe5\xb0\x94\xe6\x96\xaf\xe7\x89\xb9\xe7\xbd\x97\xe5\xa7\x86" },
	{ "Marshall", "\xe9\xa9\xac\xe6\xad\x87\xe5\xb0\x94" },
	{ "Watson", "\xe6\xb2\x83\xe6\xa3\xae" },
	{ "Dees", "\xe8\xbf\xaa\xe6\x96\xaf" },
	{ "Ripkey", "\xe9\x87\x8c\xe6\x99\xae\xe5\x9f\xba" },
	{ "Dischler", "\xe8\xbf\xaa\xe6\x96\xbd\xe5\x8b\x92" },
	{ "Damato", "\xe8\xbe\xbe\xe9\xa9\xac\xe6\x89\x98" },
	{ "Jackson", "\xe6\x9d\xb0\xe5\x85\x8b\xe9\x80\x8a" },
	{ "Salmon", "\xe8\x90\xa8\xe8\x92\x99" },
	{ "Hummer", "\xe5\x93\x88\xe9\xbb\x98" },
	{ "Sanchez", "\xe6\xa1\x91\xe5\x88\x87\xe6\x96\xaf" },
	{ "Houchard", "\xe8\x83\xa1\xe6\xb2\x99\xe5\xb0\x94" },
	{ "Nicholson", "\xe5\xb0\xbc\xe7\xa7\x91\xe5\xb0\x94\xe6\xa3\xae" },
	{ "Vainio", "\xe7\x93\xa6\xe4\xbc\x8a\xe5\xb0\xbc\xe5\xa5\xa5" },
	{ "Singer", "\xe8\xbe\x9b\xe6\xa0\xbc" },
	{ "Stern", "\xe6\x96\xaf\xe7\x89\xb9\xe6\x81\xa9" },
	{ "Hilsabeck", "\xe5\xb8\x8c\xe5\xb0\x94\xe8\x90\xa8\xe8\xb4\x9d\xe5\x85\x8b" },
	{ "Dynerman", "\xe8\xbf\xaa\xe7\xba\xb3\xe6\x9b\xbc" },
	{ "Gummelt", "\xe5\x8f\xa4\xe6\xa2\x85\xe5\xb0\x94\xe7\x89\xb9" },
	{ "Law", "\xe5\x8a\xb3" },
	{ "Stetson", "\xe6\x96\xaf\xe7\x89\xb9\xe6\xa3\xae" },
	{ "Brandt", "\xe5\xb8\x83\xe5\x85\xb0\xe7\x89\xb9" },
	{ "Pleva", "\xe6\x99\xae\xe5\x88\x97\xe7\x93\xa6" },
	{ "Bennette", "\xe8\xb4\x9d\xe5\x86\x85\xe7\x89\xb9" },
	{ "Morois", "\xe8\x8e\xab\xe9\xb2\x81\xe7\x93\xa6" },
	{ "Hay", "\xe6\xb5\xb7" },
	{ "McKenzie", "\xe9\xba\xa6\xe8\x82\xaf\xe9\xbd\x90" },
	{ "Crowns", "\xe5\x85\x8b\xe5\x8a\xb3\xe6\x81\xa9\xe6\x96\xaf" },
	{ "Cordes", "\xe7\xa7\x91\xe5\xbe\xb7\xe6\x96\xaf" },
	{ "McNutt", "\xe9\xba\xa6\xe5\x85\x8b\xe7\xba\xb3\xe7\x89\xb9" },
	{ "Babcock", "\xe5\xb7\xb4\xe5\xb8\x83\xe7\xa7\x91\xe5\x85\x8b" },
	{ "Suzuki", "\xe9\x93\x83\xe6\x9c\xa8" },
	{ "Steinberg", "\xe6\x96\xaf\xe5\x9d\xa6\xe4\xbc\xaf\xe6\xa0\xbc" },
	{ "Meier", "\xe8\xbf\x88\xe5\xb0\x94" },
	{ "Stills", "\xe6\x96\xaf\xe8\x92\x82\xe5\xb0\x94\xe6\x96\xaf" },
	{ "Short", "\xe8\x82\x96\xe7\x89\xb9" },
	{ "Bettenberg", "\xe8\xb4\x9d\xe6\xbb\x95\xe8\xb4\x9d\xe6\xa0\xbc" },
	{ "Gulisano", "\xe5\x8f\xa4\xe5\x88\xa9\xe8\x90\xa8\xe8\xaf\xba" },
	{ "Williams", "\xe5\xa8\x81\xe5\xbb\x89\xe6\x96\xaf" },
	{ "Graves", "\xe6\xa0\xbc\xe9\x9b\xb7\xe5\xa4\xab\xe6\x96\xaf" },
	{ "Quarles", "\xe5\xa4\xb8\xe5\xb0\x94\xe6\x96\xaf" },
	{ "Egnew", "\xe5\x9f\x83\xe6\xa0\xbc\xe7\xba\xbd" },
	{ "Swekel", "\xe6\x96\xaf\xe9\x9f\xa6\xe5\x85\x8b\xe5\xb0\x94" },
	{ "Hooper", "\xe8\x83\xa1\xe7\x8f\x80" },
	{ "Spence", "\xe6\x96\xaf\xe5\xbd\xad\xe6\x96\xaf" },
	{ "Farnsworth", "\xe6\xb3\x95\xe6\x81\xa9\xe6\x96\xaf\xe6\xb2\x83\xe6\x80\x9d" },
	{ "Wright", "\xe8\xb5\x96\xe7\x89\xb9" },
	{ "Stevens", "\xe5\x8f\xb2\xe8\x92\x82\xe6\x96\x87\xe6\x96\xaf" },
	{ "Nguyen", "\xe9\x98\xae" },
	{ "Pitman", "\xe7\x9a\xae\xe7\x89\xb9\xe6\x9b\xbc" },
	{ "Rodeman", "\xe7\xbd\x97\xe5\xbe\xb7\xe6\x9b\xbc" },
	{ "Kovak", "\xe7\xa7\x91\xe7\x93\xa6\xe5\x85\x8b" },
	{ "Hawkins", "\xe9\x9c\x8d\xe9\x87\x91\xe6\x96\xaf" },
	{ "Ness", "\xe5\x86\x85\xe6\x96\xaf" },
	{ "Yongue", "\xe6\x89\xac" },
	{ "Burns", "\xe4\xbc\xaf\xe6\x81\xa9\xe6\x96\xaf" },
	{ "Shaw", "\xe8\x82\x96" },
	{ "Strang", "\xe6\x96\xaf\xe7\x89\xb9\xe6\x9c\x97" },
	{ "Thomas", "\xe6\x89\x98\xe9\xa9\xac\xe6\x96\xaf" },
	{ "Pupino", "\xe6\x99\xae\xe7\x9a\xae\xe8\xaf\xba" },
	{ "Kasanoff", "\xe5\x8d\xa1\xe8\x90\xa8\xe8\xaf\xba\xe5\xa4\xab" },
	{ "Polman", "\xe6\xb3\xa2\xe5\xb0\x94\xe6\x9b\xbc" },
	{ "Wendt", "\xe6\x96\x87\xe7\x89\xb9" },
	{ "Asher", "\xe9\x98\xbf\xe8\x88\x8d" },
	{ "Dundee", "\xe9\x82\x93\xe8\xbf\xaa" },
	{ "Zick", "\xe9\xbd\x90\xe5\x85\x8b" },
	{ "Schumacher", "\xe8\x88\x92\xe9\xa9\xac\xe8\xb5\xab" },
	{ "Marlin", "\xe9\xa9\xac\xe6\x9e\x97" },
	{ "Lathrop", "\xe6\x8b\x89\xe6\x96\xaf\xe7\xbd\x97\xe6\x99\xae" },
	{ "Rodriguez", "\xe7\xbd\x97\xe5\xbe\xb7\xe9\x87\x8c\xe6\xa0\xbc\xe6\x96\xaf" },
	{ "Schilder", "\xe5\xb8\xad\xe5\xb0\x94\xe5\xbe\xb7" },
	{ "Leffler", "\xe8\x8e\xb1\xe5\xbc\x97\xe5\x8b\x92" },
	{ "Blascoe", "\xe5\xb8\x83\xe6\x8b\x89\xe6\x96\xaf\xe7\xa7\x91" },
	{ "Berry", "\xe8\xb4\x9d\xe9\x87\x8c" },
	{ "Egan", "\xe4\xbc\x8a\xe6\xa0\xb9" },
	{ "Cleveland", "\xe5\x85\x8b\xe5\x88\xa9\xe5\xa4\xab\xe5\x85\xb0" },
	{ "Schwarzer", "\xe6\x96\xbd\xe7\x93\xa6\xe6\xb3\xbd" },
	{ "Eckley", "\xe5\x9f\x83\xe5\x85\x8b\xe5\x88\xa9" },
	{ "Chapman", "\xe6\x9f\xa5\xe6\x99\xae\xe6\x9b\xbc" },
	{ "Monse", "\xe8\x92\x99\xe6\x96\xaf" },
	{ "Jarlsberg", "\xe4\xba\x9a\xe5\xb0\x94\xe6\x96\xaf\xe8\xb4\x9d\xe6\xa0\xbc" },
	{ "Lanier", "\xe6\x8b\x89\xe5\xb0\xbc\xe5\xb0\x94" },
	{ "Livingston", "\xe5\x88\xa9\xe6\x96\x87\xe6\x96\xaf\xe9\xa1\xbf" },
	{ "Vasker", "\xe7\x93\xa6\xe6\x96\xaf\xe5\x85\x8b" },
	{ "McKinnon", "\xe9\xba\xa6\xe9\x87\x91\xe5\x86\x9c" },
	{ "Harkins", "\xe5\x93\x88\xe9\x87\x91\xe6\x96\xaf" },
	{ "Showers", "\xe8\x82\x96\xe5\xb0\x94\xe6\x96\xaf" },
	{ "Albain", "\xe5\xa5\xa5\xe5\xb0\x94\xe7\x8f\xad" },
	{ "Vernon", "\xe5\xbc\x97\xe5\x86\x9c" },
	{ "Cavel", "\xe5\x8d\xa1\xe7\xbb\xb4\xe5\xb0\x94" },
	{ "Cimino", "\xe5\xa5\x87\xe7\xb1\xb3\xe8\xaf\xba" },
	{ "Kofsky", "\xe7\xa7\x91\xe5\xa4\xab\xe6\x96\xaf\xe5\x9f\xba" },
	{ "Shepard", "\xe8\xb0\xa2\xe6\xb3\xbc\xe5\xbe\xb7" },
	{ "Ekberg", "\xe5\x9f\x83\xe5\x85\x8b\xe4\xbc\xaf\xe6\xa0\xbc" },
	{ "Whitaker", "\xe6\x83\xa0\xe7\x89\xb9\xe5\x85\x8b" },
	{ "Bordwell", "\xe5\x8d\x9a\xe5\xbe\xb7\xe9\x9f\xa6\xe5\xb0\x94" },
	{ "Schwartz", "\xe6\x96\xbd\xe7\x93\xa6\xe8\x8c\xa8" },
	{ "Glovels", "\xe6\xa0\xbc\xe6\xb4\x9b\xe5\xbc\x97\xe5\xb0\x94\xe6\x96\xaf" },
	{ "Ramirez", "\xe6\x8b\x89\xe7\xb1\xb3\xe9\x9b\xb7\xe6\x96\xaf" },
	{ "Moire", "\xe8\x8e\xab\xe7\x93\xa6\xe5\xb0\x94" },
	{ "Keifer", "\xe5\x9f\xba\xe5\xbc\x97" },
	{ "Holmes", "\xe9\x9c\x8d\xe5\xa7\x86\xe6\x96\xaf" },
	{ "Border", "\xe5\x8d\x9a\xe5\xbe\xb7" },
	{ "Potts", "\xe6\xb3\xa2\xe8\x8c\xa8" },
	{ "Pearson", "\xe7\x9a\xae\xe5\xb0\x94\xe9\x80\x8a" },
	{ "Fuchs", "\xe5\xaf\x8c\xe5\x85\x8b\xe6\x96\xaf" },
	{ "Ashworth", "\xe9\x98\xbf\xe4\xbb\x80\xe6\xb2\x83\xe6\x80\x9d" },
	{ "Milage", "\xe7\xb1\xb3\xe5\x88\xa9\xe5\xa5\x87" },
	{ "Kruszka", "\xe5\x85\x8b\xe9\xb2\x81\xe4\xbb\x80\xe5\x8d\xa1" },
	{ "Fassett", "\xe6\xb3\x95\xe5\xa1\x9e\xe7\x89\xb9" },
	{ "Gray", "\xe6\xa0\xbc\xe9\x9b\xb7" },
	// === Squad compound replacements ===
	{ "Rhino Squad", "\xe7\x8a\x80\xe7\x89\x9b\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Scorpion Squad", "\xe5\xa4\xa9\xe8\x9d\x8e\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Badger Squad", "\xe7\x8c\x9b\xe7\x8d\xbe\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Condor Squad", "\xe7\xa5\x9e\xe9\xb9\xb0\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Kodiak Squad", "\xe6\xa3\x95\xe7\x86\x8a\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Raven Squad", "\xe6\xb8\xa1\xe9\xb8\xa6\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Viper Squad", "\xe6\xaf\x92\xe8\x9b\x87\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Wolf Squad", "\xe9\x87\x8e\xe7\x8b\xbc\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Eagle Squad", "\xe9\x9b\x84\xe9\xb9\xb0\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Cobra Squad", "\xe7\x9c\xbc\xe9\x95\x9c\xe8\x9b\x87\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Warthog Squad", "\xe7\x96\xa3\xe7\x8c\xaa\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Bison Squad", "\xe9\x87\x8e\xe7\x89\x9b\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Hyena Squad", "\xe9\xac\xa3\xe7\x8b\x97\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Cougar Squad", "\xe7\xbe\x8e\xe6\xb4\xb2\xe7\x8b\xae\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Mantis Squad", "\xe8\x9e\xb3\xe8\x9e\x82\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Grizzly Squad", "\xe7\x81\xb0\xe7\x86\x8a\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Armadillo Squad", "\xe7\x8a\xb0\xe7\x8b\xb3\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Fox Squad", "\xe7\x8b\x90\xe7\x8b\xb8\xe5\xb0\x8f\xe9\x98\x9f" },
	{ "Falcon Squad", "\xe7\x8c\x8e\xe9\xb9\xb0\xe5\xb0\x8f\xe9\x98\x9f" },
	// === Squad animal names (\xE5\x8D\x95\xE7\x8B\xAC\xE6\x9B\xBF\xE6\x8D\xA2\xEF\xBC\x8C\xE7\x94\xA8\xE4\xBA\x8E "Rhino \xE5\xB0\x8F\xE9\x98\x9F" \xE8\xBF\x99\xE7\xA7\x8D\xE6\xB7\xB7\xE5\x90\x88\xE6\x96\x87\xE6\x9C\xAC) ===
	{ "Rhino", "\xe7\x8a\x80\xe7\x89\x9b" },
	{ "Scorpion", "\xe5\xa4\xa9\xe8\x9d\x8e" },
	{ "Badger", "\xe7\x8c\x9b\xe7\x8d\xbe" },
	{ "Condor", "\xe7\xa5\x9e\xe9\xb9\xb0" },
	{ "Kodiak", "\xe6\xa3\x95\xe7\x86\x8a" },
	{ "Raven", "\xe6\xb8\xa1\xe9\xb8\xa6" },
	{ "Viper", "\xe6\xaf\x92\xe8\x9b\x87" },
	{ "Wolf", "\xe9\x87\x8e\xe7\x8b\xbc" },
	{ "Eagle", "\xe9\x9b\x84\xe9\xb9\xb0" },
	{ "Cobra", "\xe7\x9c\xbc\xe9\x95\x9c\xe8\x9b\x87" },
	{ "Warthog", "\xe7\x96\xa3\xe7\x8c\xaa" },
	{ "Bison", "\xe9\x87\x8e\xe7\x89\x9b" },
	{ "Hyena", "\xe9\xac\xa3\xe7\x8b\x97" },
	{ "Cougar", "\xe7\xbe\x8e\xe6\xb4\xb2\xe7\x8b\xae" },
	{ "Mantis", "\xe8\x9e\xb3\xe8\x9e\x82" },
	{ "Grizzly", "\xe7\x81\xb0\xe7\x86\x8a" },
	{ "Armadillo", "\xe7\x8a\xb0\xe7\x8b\xb3" },
	{ "Fox", "\xe7\x8b\x90\xe7\x8b\xb8" },
	{ "Falcon", "\xe7\x8c\x8e\xe9\xb9\xb0" },
	{ "McClane", "\xe9\xba\xa6\xe5\x85\x8b\xe8\x8e\xb1\xe6\x81\xa9" },
	{ "Collins", "\xe6\x9f\xaf\xe6\x9e\x97\xe6\x96\xaf" },
	{ "Haloway", "\xe5\x93\x88\xe6\xb4\x9b\xe5\xa8\x81" },
	{ "Huxley", "\xe5\x93\x88\xe5\x85\x8b\xe6\x96\xaf\xe5\x88\xa9" },
	{ "Shane", "\xe8\xb0\xa2\xe6\x81\xa9" },
	{ "Masters", "\xe9\xa9\xac\xe6\x96\xaf\xe7\x89\xb9\xe6\x96\xaf" },
	{ "Meuller", "\xe7\xa9\x86\xe5\x8b\x92" },
	{ "Morte", "\xe8\x8e\xab\xe7\x89\xb9" },
	{ "Ranka", "\xe5\x85\xb0\xe5\x8d\xa1" },
	{ "Summers", "\xe8\x90\xa8\xe9\xbb\x98\xe6\x96\xaf" },
	{ "Iowa", "\xe8\xa1\xa3\xe9\x98\xbf\xe5\x8d\x8e" },
	{ NULL, NULL }
};

// \xE5\xB0\x86\xE8\xAF\xB4\xE8\xAF\x9D\xE4\xBA\xBA\xE5\x90\x8D\xEF\xBC\x88\xE5\x89\x8D\xE7\xBC\x80\xEF\xBC\x89\xE7\xBF\xBB\xE8\xAF\x91\xE4\xB8\xBA\xE4\xB8\xAD\xE6\x96\x87\xEF\xBC\x88\xE5\xA4\xA7\xE5\xB0\x8F\xE5\x86\x99\xE4\xB8\x8D\xE6\x95\x8F\xE6\x84\x9F\xEF\xBC\x89
static const char *TranslateSpeakerName( const char *speaker ) {
	if ( !harm_g_cnNames.GetBool() || !speaker || !speaker[0] ) {
		return speaker;
	}
	for ( int i = 0; nameMap[i].en; i++ ) {
		if ( idStr::Icmp( speaker, nameMap[i].en ) == 0 ) {
			return nameMap[i].cn;
		}
	}
	return speaker;
}

// \xE5\xB0\x86\xE5\xAD\x97\xE5\xB9\x95\xE6\x96\x87\xE6\x9C\xAC\xE4\xB8\xAD\xE7\x9A\x84\xE8\x8B\xB1\xE6\x96\x87\xE4\xBA\xBA\xE5\x90\x8D\xE6\x9B\xBF\xE6\x8D\xA2\xE4\xB8\xBA\xE4\xB8\xAD\xE6\x96\x87\xEF\xBC\x8C\xE5\xB9\xB6\xE5\x8E\xBB\xE9\x99\xA4\xE5\xA4\x9A\xE4\xBD\x99\xE7\xA9\xBA\xE6\xA0\xBC
// \xE5\x90\x8C\xE6\x97\xB6\xE7\x94\xA8\xE4\xBA\x8E\xE5\x89\x8D\xE7\xBC\x80\xEF\xBC\x88\xE5\xA4\x84\xE7\x90\x86\xE2\x80\x9C\xE4\xB8\xAD\xE5\xA3\xAB Morris\xE2\x80\x9D\xE2\x86\x92\xE2\x80\x9C\xE4\xB8\xAD\xE5\xA3\xAB\xE8\x8E\xAB\xE9\x87\x8C\xE6\x96\xAF\xE2\x80\x9D\xEF\xBC\x89
static void TranslateNamesInText( idStr &text ) {
	if ( !harm_g_cnNames.GetBool() ) {
		return;
	}
	// \xE5\xA4\x8D\xE5\x90\x88\xE5\x9C\xB0\xE5\x90\x8D\xE5\x85\x88\xE5\xA4\x84\xE7\x90\x86\xEF\xBC\x88\xE9\x81\xBF\xE5\x85\x8D "New" \xE5\x92\x8C "Berlin" \xE5\x88\x86\xE5\xBC\x80\xE6\x9B\xBF\xE6\x8D\xA2\xEF\xBC\x89
	text.Replace( "New Berlin", "\xe6\x96\xb0\xe6\x9f\x8f\xe6\x9e\x97" );		// \xe6\x96\xb0\xe6\x9f\x8f\xe6\x9e\x97
	// \xE5\x85\xA8\xE5\x90\x8D\xE5\xA4\x8D\xE5\x90\x88\xE6\x9B\xBF\xE6\x8D\xA2\xEF\xBC\x88\xE5\x90\x8D+\xE5\xA7\x93\xE7\x94\xA8\xE9\x97\xB4\xE9\x9A\x94\xE5\x8F\xB7 \xC2\xB7 \xE5\x88\x86\xE9\x9A\x94\xEF\xBC\x89
	text.Replace( "Alejandro Cortez", "\xe4\xba\x9a\xe5\x8e\x86\xe6\x9d\xad\xe5\xbe\xb7\xe7\xbd\x97\xc2\xb7\xe7\xa7\x91\xe5\xb0\x94\xe7\x89\xb9\xe6\x96\xaf" );
	text.Replace( "Matthew Kane", "\xe9\xa9\xac\xe4\xbf\xae\xc2\xb7\xe5\x87\xaf\xe6\x81\xa9" );
	text.Replace( "Nikolai Slidjonovitch", "\xe5\xb0\xbc\xe5\x8f\xa4\xe6\x8b\x89\xc2\xb7\xe6\x96\xaf\xe5\x88\xa9\xe5\xbe\xb7\xe7\xba\xa6\xe8\xaf\xba\xe7\xbb\xb4\xe5\xa5\x87" );
	text.Replace( "Herman Ways", "\xe8\xb5\xab\xe5\xb0\x94\xe6\x9b\xbc\xc2\xb7\xe9\x9f\xa6\xe6\x96\xaf" );
	text.Replace( "Paul Herman", "\xe4\xbf\x9d\xe7\xbd\x97\xc2\xb7\xe8\xb5\xab\xe5\xb0\x94\xe6\x9b\xbc" );
	text.Replace( "Terrance Garr", "\xe7\x89\xb9\xe4\xbc\xa6\xe6\x96\xaf\xc2\xb7\xe5\x8a\xa0\xe5\xb0\x94" );
	text.Replace( "Mark Lorenzi", "\xe9\xa9\xac\xe5\x85\x8b\xc2\xb7\xe6\xb4\x9b\xe4\xbc\xa6\xe9\xbd\x90" );
	for ( int i = 0; nameMap[i].en; i++ ) {
		// \xE6\x9B\xBF\xE6\x8D\xA2\xE9\xA1\xBA\xE5\xBA\x8F\xEF\xBC\x9A\xE5\x85\x88\xE5\xA4\x84\xE7\x90\x86\xE5\xB8\xA6\xE6\x9B\xB4\xE5\xA4\x9A\xE7\xA9\xBA\xE6\xA0\xBC\xE7\x9A\x84\xEF\xBC\x8C\xE9\x81\xBF\xE5\x85\x8D\xE6\x8F\x90\xE5\x89\x8D\xE6\x9B\xBF\xE6\x8D\xA2\xE5\x90\x8E\xE6\x89\xBE\xE4\xB8\x8D\xE5\x88\xB0
		idStr both    = va( " %s ", nameMap[i].en );	// " Miller "
		idStr trailing = va( "%s ", nameMap[i].en );	// "Miller "
		idStr leading  = va( " %s", nameMap[i].en );	// " Miller"
		text.Replace( both.c_str(), nameMap[i].cn );		// " Miller " -> "中文名"
		text.Replace( trailing.c_str(), nameMap[i].cn );	// "Miller " -> "中文名"
		text.Replace( leading.c_str(), nameMap[i].cn );		// " Miller" -> "中文名"
		text.Replace( nameMap[i].en, nameMap[i].cn );		// "Miller" -> "中文名"
	}
}

// \xE5\x86\x9B\xE8\xA1\x94\xE5\x88\x97\xE8\xA1\xA8\xEF\xBC\x88\xE7\x94\xA8\xE4\xBA\x8E\xE5\x89\x8D\xE7\xBC\x80\xE8\xA7\x84\xE8\x8C\x83\xE5\x8C\x96\xEF\xBC\x89
static const char *cnRanks[] = {
	"\xe5\x88\x97\xe5\x85\xb5",						// \xe5\x88\x97\xe5\x85\xb5
	"\xe4\xb8\x8b\xe5\xa3\xab",						// \xe4\xb8\x8b\xe5\xa3\xab
	"\xe4\xb8\xad\xe5\xa3\xab",						// \xe4\xb8\xad\xe5\xa3\xab
	"\xe4\xb8\x8a\xe5\xa3\xab",						// \xe4\xb8\x8a\xe5\xa3\xab
	"\xe4\xb8\x80\xe7\xad\x89\xe5\x85\xb5",				// \xe4\xb8\x80\xe7\xad\x89\xe5\x85\xb5
	"\xe4\xba\x8c\xe7\xad\x89\xe5\x85\xb5",				// \xe4\xba\x8c\xe7\xad\x89\xe5\x85\xb5
	"\xe4\xb8\xad\xe5\xb0\x89",						// \xe4\xb8\xad\xe5\xb0\x89
	"\xe4\xb8\x8a\xe5\xb0\x89",						// \xe4\xb8\x8a\xe5\xb0\x89
	"\xe5\xb0\x91\xe6\xa0\xa1",						// \xe5\xb0\x91\xe6\xa0\xa1
	"\xe4\xb8\xad\xe6\xa0\xa1",						// \xe4\xb8\xad\xe6\xa0\xa1
	"\xe4\xb8\x8a\xe6\xa0\xa1",						// \xe4\xb8\x8a\xe6\xa0\xa1
	"\xe5\xb0\x91\xe5\xb0\x86",						// \xe5\xb0\x91\xe5\xb0\x86
	"\xe4\xb8\xad\xe5\xb0\x86",						// \xe4\xb8\xad\xe5\xb0\x86
	"\xe4\xb8\x8a\xe5\xb0\x86",						// \xe4\xb8\x8a\xe5\xb0\x86
	"\xe5\xa4\xa7\xe5\xb0\x86",						// \xe5\xa4\xa7\xe5\xb0\x86
	"\xe5\xb0\x86\xe5\x86\x9b",						// \xe5\xb0\x86\xe5\x86\x9b
	NULL
};

// \xE5\x89\x8D\xE7\xBC\x80\xE5\x86\x9B\xE8\xA1\x94\xE8\xA7\x84\xE8\x8C\x83\xE5\x8C\x96\xEF\xBC\x9A\xE5\x86\x9B\xE8\xA1\x94\xE5\x9C\xA8\xE5\x89\x8D\xE7\x9A\x84\xE7\xA7\xBB\xE5\x88\xB0\xE5\x90\x8E\xE9\x9D\xA2
static void NormalizeRank( idStr &speaker ) {
	if ( !harm_g_cnNames.GetBool() ) {
		return;
	}
	for ( int i = 0; cnRanks[i]; i++ ) {
		int len = (int)strlen( cnRanks[i] );
		if ( speaker.Length() > len && idStr::Cmpn( speaker.c_str(), cnRanks[i], len ) == 0 ) {
			idStr name = speaker.Mid( len, speaker.Length() - len );
			speaker = va( "%s%s", name.c_str(), cnRanks[i] );
			return;
		}
	}
}

// \xE5\x85\xAC\xE5\xBC\x80\xE6\x8E\xA5\xE5\x8F\xA3\xEF\xBC\x9A\xE7\xBB\x99 Player.cpp \xE5\x87\x86\xE5\xBF\x83\xE7\xAD\x89\xE5\x9C\xBA\xE6\x99\xAF\xE8\xB0\x83\xE7\x94\xA8
static void BracketEnemyNames( idStr &text );	// forward declaration
void rvSubtitles::LocalizeText( idStr &text ) {
	TranslateNamesInText( text );
	NormalizeRank( text );
	BracketEnemyNames( text );
}

// Strogg \xE6\x95\x8C\xE6\x96\xB9\xE5\x8D\x95\xE4\xBD\x8D\xE5\x90\x8D\xE5\x88\x97\xE8\xA1\xA8\xEF\xBC\x88\xE5\x86\x85\xE5\xAE\xB9\xE4\xB8\xAD\xE5\x87\xBA\xE7\x8E\xB0\xE6\x97\xB6\xE7\x94\xA8\xE3\x80\x8C\xE3\x80\x8D\xE5\x8C\x85\xE8\xA3\xB9\xEF\xBC\x89
static const char *enemyUnitNames[] = {
	"\xe6\xad\xa5\xe5\x85\xb5",						// \xe6\xad\xa5\xe5\x85\xb5
	"\xe6\x9c\xba\xe7\x82\xae\xe5\x85\xb5",					// \xe6\x9c\xba\xe7\x82\xae\xe5\x85\xb5
	"\xe7\x8b\x82\xe6\x88\x98\xe5\xa3\xab",					// \xe7\x8b\x82\xe6\x88\x98\xe5\xa3\xab
	"\xe8\xa7\x92\xe6\x96\x97\xe5\xa3\xab",					// \xe8\xa7\x92\xe6\x96\x97\xe5\xa3\xab
	"\xe9\x93\x81\xe5\xa8\x98\xe5\xad\x90",					// \xe9\x93\x81\xe5\xa8\x98\xe5\xad\x90
	"\xe5\xa4\xb1\xe8\xb4\xa5\xe6\x94\xb9\xe9\x80\xa0\xe4\xbd\x93",		// \xe5\xa4\xb1\xe8\xb4\xa5\xe6\x94\xb9\xe9\x80\xa0\xe4\xbd\x93
	"\xe7\xa7\x91\xe5\xad\xa6\xe5\xae\xb6",					// \xe7\xa7\x91\xe5\xad\xa6\xe5\xae\xb6
	"\xe5\x93\xa8\xe5\x8d\xab",							// \xe5\x93\xa8\xe5\x8d\xab
	"\xe6\x94\xb6\xe5\x89\xb2\xe8\x80\x85",					// \xe6\x94\xb6\xe5\x89\xb2\xe8\x80\x85
	"\xe8\xbd\xbb\xe5\x9e\x8b\xe5\x9d\xa6\xe5\x85\x8b",				// \xe8\xbd\xbb\xe5\x9e\x8b\xe5\x9d\xa6\xe5\x85\x8b
	"\xe6\x82\xac\xe6\xb5\xae\xe5\x9d\xa6\xe5\x85\x8b",				// \xe6\x82\xac\xe6\xb5\xae\xe5\x9d\xa6\xe5\x85\x8b
	"\xe6\x95\xb0\xe6\x8d\xae\xe6\xb5\x81\xe5\xae\x88\xe5\x8d\xab",		// \xe6\x95\xb0\xe6\x8d\xae\xe6\xb5\x81\xe5\xae\x88\xe5\x8d\xab
	"\xe7\xbd\x91\xe7\xbb\x9c\xe5\xae\x88\xe5\x8d\xab",				// \xe7\xbd\x91\xe7\xbb\x9c\xe5\xae\x88\xe5\x8d\xab
	"\xe4\xbc\xa0\xe9\x80\x81\xe6\x8a\x95\xe6\x94\xbe\xe5\x99\xa8",		// \xe4\xbc\xa0\xe9\x80\x81\xe6\x8a\x95\xe6\x94\xbe\xe5\x99\xa8
	"\xe7\xbb\xb4\xe4\xbf\xae\xe6\x9c\xba\xe5\x99\xa8\xe4\xba\xba",		// \xe7\xbb\xb4\xe4\xbf\xae\xe6\x9c\xba\xe5\x99\xa8\xe4\xba\xba
	"\xe6\x88\x98\xe6\x9c\xaf\xe5\x85\xb5",					// \xe6\x88\x98\xe6\x9c\xaf\xe5\x85\xb5
	"\xe7\x9c\xbc\xe6\x9f\x84",							// \xe7\x9c\xbc\xe6\x9f\x84
	"\xe9\xa9\xac\xe5\x85\x8b\xe9\xbe\x99",					// \xe9\xa9\xac\xe5\x85\x8b\xe9\xbe\x99
	NULL
};

// \xE5\xB0\x86\xE5\x86\x85\xE5\xAE\xB9\xE4\xB8\xAD\xE7\x9A\x84\xE6\x95\x8C\xE6\x96\xB9\xE5\x8D\x95\xE4\xBD\x8D\xE5\x90\x8D\xE7\x94\xA8\xE3\x80\x8C\xE3\x80\x8D\xE5\x8C\x85\xE8\xA3\xB9\xEF\xBC\x8C\xE9\x81\xBF\xE5\x85\x8D\xE9\x87\x8D\xE5\xA4\x8D\xE5\x8C\x85\xE8\xA3\xB9
static void BracketEnemyNames( idStr &text ) {
	for ( int i = 0; enemyUnitNames[i]; i++ ) {
		idStr bracketed = va( "\xe3\x80\x8c%s\xe3\x80\x8d", enemyUnitNames[i] );
		text.Replace( bracketed.c_str(), enemyUnitNames[i] );
		text.Replace( enemyUnitNames[i], bracketed.c_str() );
	}
	// \xE5\xB0\x8F\xE9\x98\x9F\xE5\x90\x8D\xEF\xBC\x9A\xE7\x8A\x80\xE7\x89\x9B\xE5\xB0\x8F\xE9\x98\x9F \xE2\x86\x92 \xE3\x80\x8C\xE7\x8A\x80\xE7\x89\x9B\xE3\x80\x8D\xE5\xB0\x8F\xE9\x98\x9F
	static const struct { const char *plain; const char *bracketed; } squadNames[] = {
		{ "\xe7\x8a\x80\xe7\x89\x9b\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\x8a\x80\xe7\x89\x9b\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe5\xa4\xa9\xe8\x9d\x8e\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe5\xa4\xa9\xe8\x9d\x8e\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe7\x8c\x9b\xe7\x8d\xbe\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\x8c\x9b\xe7\x8d\xbe\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe7\xa5\x9e\xe9\xb9\xb0\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\xa5\x9e\xe9\xb9\xb0\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe6\xa3\x95\xe7\x86\x8a\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe6\xa3\x95\xe7\x86\x8a\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe6\xb8\xa1\xe9\xb8\xa6\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe6\xb8\xa1\xe9\xb8\xa6\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe6\xaf\x92\xe8\x9b\x87\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe6\xaf\x92\xe8\x9b\x87\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe9\x87\x8e\xe7\x8b\xbc\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe9\x87\x8e\xe7\x8b\xbc\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe9\x9b\x84\xe9\xb9\xb0\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe9\x9b\x84\xe9\xb9\xb0\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe7\x9c\xbc\xe9\x95\x9c\xe8\x9b\x87\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\x9c\xbc\xe9\x95\x9c\xe8\x9b\x87\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe7\x96\xa3\xe7\x8c\xaa\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\x96\xa3\xe7\x8c\xaa\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe9\x87\x8e\xe7\x89\x9b\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe9\x87\x8e\xe7\x89\x9b\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe9\xac\xa3\xe7\x8b\x97\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe9\xac\xa3\xe7\x8b\x97\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe7\xbe\x8e\xe6\xb4\xb2\xe7\x8b\xae\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\xbe\x8e\xe6\xb4\xb2\xe7\x8b\xae\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe8\x9e\xb3\xe8\x9e\x82\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe8\x9e\xb3\xe8\x9e\x82\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe7\x81\xb0\xe7\x86\x8a\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\x81\xb0\xe7\x86\x8a\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe7\x8a\xb0\xe7\x8b\xb3\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\x8a\xb0\xe7\x8b\xb3\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe9\xbb\x84\xe8\xb2\x82\xe9\xb1\xbc\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe9\xbb\x84\xe8\xb2\x82\xe9\xb1\xbc\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ "\xe7\x8b\x90\xe7\x8b\xb8\xe5\xb0\x8f\xe9\x98\x9f", "\xe3\x80\x8c\xe7\x8b\x90\xe7\x8b\xb8\xe3\x80\x8d\xe5\xb0\x8f\xe9\x98\x9f" },
		{ NULL, NULL }
	};
	for ( int i = 0; squadNames[i].plain; i++ ) {
		text.Replace( squadNames[i].bracketed, squadNames[i].plain );
		text.Replace( squadNames[i].plain, squadNames[i].bracketed );
	}
}
idCVar harm_g_subtitlePVSCheck( "harm_g_subtitlePVSCheck", "1", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "hide subtitles of speakers outside player PVS (occluded)" );
idCVar harm_g_resIndex( "harm_g_resIndex", "-1", CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "resolution selector index (100-series=4:3, 200-series=16:9, 300-series=16:10)" );

// 近距离豁免：此距离（游戏单位）内即使隔墙（不在 PVS）也显示字幕
static const float SUB_PVS_NEAR_DIST = 240.0f;

// 面板布局（640x480 虚拟坐标）；2026-07-17 用户要求：面板上移且缩窄
// （与 guis/subtitles.gui 的 rect 及 SUB_TEXT_W 联动，改一处须同步三处）
static const float SUB_BOTTOM	= 410.0f;	// 面板底边（原 428）
static const float SUB_ROW_H	= 13.0f;	// 行高
static const float SUB_PAD		= 4.0f;		// 上下留白
// 字幕在面板里的额外 y 偏移（视觉居中微调）：v1.0.7 曾用 1.0 补偿 CJK drop 偏顶，
// 但 v1.0.9 用户反馈"字幕相较背景偏下" → 撤回为 0（chain 独立 canonical 后 lowpixel
// 与 marine 同源 chain=marine（marine 现在是 canonical），drop 度量与之前不同）。
static const float SUB_ROW_ADJ	= 0.0f;
static const int   FADE_IN_MS	= 150;
static const int   FADE_OUT_MS	= 300;

rvSubtitles gameSubtitles;

// —— 行宽度量（与引擎 DrawText 同款推进公式：xSkip * useScale）——
// 首次使用时从字幕 GUI 同款 fontdat 读取真实字形步进，文件缺失时退回估值
static const float SUB_TEXT_W		= 362.0f;					// 行像素预算（gui 文本区宽 368，留行首禁则并入余量）
// 2026-07-17 字幕改用 24 号字库同尺寸渲染：gui_smallFontLimit 置 0（启动脚本/
// 配置下发）后 textscale 0.19 落入 24 号档，useScale=0.19×48/24=0.38，屏显
// 尺寸不变、度量分辨率翻倍——fontdat 步进是整数，12 号下拉丁字母 4~6px 的
// ±0.5 舍入误差屏显达 ±1.1px（"gg"/"ss" 忽缝忽挤的根因），24 号相对误差减半
static const float SUB_USE_SCALE	= 0.19f * 48.0f / 24.0f;	// textscale 0.19 → 24 号字库（smallFontLimit=0）
static const char *SUB_FONT_DAT		= "fonts/chinese/lowpixel_24.fontdat";
static const float SUB_ASCII_FALLBACK	= 15.0f;				// 估值：ASCII 平均步进（字库像素）
static const float SUB_WIDE_FALLBACK	= 26.0f;				// 估值：全角步进（字库像素）

static bool		subMetricsTried = false;
static float	subAsciiAdv[128];
static byte *	subWideAdv = NULL;		// charcode → xSkip（字库像素，0=缺字形）
static int		subWideNum = 0;

// 声音 shader 名 → 说话人 映射（speaker_map.txt，build_lang.py 生成）
// speaker 实体/无线电播放的语音：有映射=角色台词(带角色名)，无映射=环境广播("广播"/"无线电")
static idDict	subSpeakerMap;
static bool		subSpeakerMapLoaded = false;
static void		SUB_LoadSpeakerMap( void );

/*
================
SUB_LoadFontMetrics

fontdat 布局（tr_font_tools.cpp）：
Q4 基础段 256 × 9 float（imageWidth,imageHeight,xSkip,pitch,top,s,t,s2,t2）+ 5 float；
宽字库扩展段 magic/version/numFiles/width/height + numIndexes + indexes[] + numGlyphs
+ 每字形 9 float + 32 字节贴图名 = 68 字节。
================
*/
static void SUB_LoadFontMetrics( void ) {
	subMetricsTried = true;
	int i;
	for ( i = 0; i < 128; i++ ) {
		subAsciiAdv[i] = SUB_ASCII_FALLBACK;
	}

	void *buf = NULL;
	int len = fileSystem->ReadFile( SUB_FONT_DAT, &buf );
	if ( len <= 0 || !buf ) {
		gameLocal.Warning( "rvSubtitles: %s not found, using fallback metrics", SUB_FONT_DAT );
		return;
	}
	const byte *p = ( const byte * )buf;

	const int baseSize = 256 * 36 + 20;
	if ( len >= baseSize ) {
		for ( i = 32; i < 127; i++ ) {
			float v;
			memcpy( &v, p + i * 36 + 2 * 4, sizeof( v ) );
			if ( v > 0.0f && v < 64.0f ) {
				subAsciiAdv[i] = v;
			}
		}
	}

	int off = baseSize + 20;
	int numIdx = 0;
	if ( len >= off + 4 ) {
		memcpy( &numIdx, p + off, 4 );
		off += 4;
	}
	if ( numIdx > 0 && numIdx <= 0x110000 && len >= off + numIdx * 4 + 4 ) {
		const byte *idxTable = p + off;
		off += numIdx * 4;
		int numGlyphs = 0;
		memcpy( &numGlyphs, p + off, 4 );
		off += 4;
		if ( numGlyphs > 0 && len >= off + numGlyphs * 68 ) {
			subWideAdv = new byte[numIdx];
			memset( subWideAdv, 0, numIdx );
			for ( i = 0; i < numIdx; i++ ) {
				int gi;
				memcpy( &gi, idxTable + i * 4, 4 );
				if ( gi >= 0 && gi < numGlyphs ) {
					float v;
					memcpy( &v, p + off + gi * 68 + 2 * 4, sizeof( v ) );
					int skip = ( int )v;
					subWideAdv[i] = ( byte )( ( skip < 0 ) ? 0 : ( ( skip > 255 ) ? 255 : skip ) );
				}
			}
			subWideNum = numIdx;
		}
	}
	fileSystem->FreeFile( buf );
}

/*
================
SUB_CharAdvance

单字符屏幕推进（640x480 虚拟像素）
================
*/
static float SUB_CharAdvance( unsigned int cp ) {
	float adv;
	if ( cp < 128 ) {
		adv = subAsciiAdv[cp];
	} else if ( subWideAdv && cp < ( unsigned int )subWideNum && subWideAdv[cp] ) {
		adv = subWideAdv[cp];
	} else {
		adv = SUB_WIDE_FALLBACK;
	}
	return adv * SUB_USE_SCALE;
}

/*
================
SUB_LoadSpeakerMap

加载声音 shader 名 → 说话人 映射（speaker_map.txt，build_lang.py 生成）。
格式：每行 "shader\tspeaker"，# 开头为注释。
用于区分 speaker 实体/无线电播放的是角色台词还是环境广播。
================
*/
static void SUB_LoadSpeakerMap( void ) {
	subSpeakerMapLoaded = true;
	void *buf = NULL;
	int len = fileSystem->ReadFile( "speaker_map.txt", &buf );
	if ( len <= 0 || !buf ) {
		return;
	}
	idStr data( ( const char * )buf );
	fileSystem->FreeFile( buf );
	int start = 0;
	while ( start < data.Length() ) {
		int end = data.Find( '\n', start );
		if ( end < 0 ) end = data.Length();
		idStr line = data.Mid( start, end - start );
		start = end + 1;
		line.StripTrailingWhitespace();
		if ( !line.Length() || line[0] == '#' ) continue;
		int tab = line.Find( '\t' );
		if ( tab < 0 ) continue;
		idStr shader = line.Left( tab );
		idStr name = line.Right( line.Length() - tab - 1 );
		shader.StripTrailingWhitespace();
		name.StripLeading( ' ' );
		if ( shader.Length() && name.Length() ) {
			subSpeakerMap.Set( shader, name );
		}
	}
}

/*
================
rvSubtitles::rvSubtitles
================
*/
rvSubtitles::rvSubtitles( void ) {
	numLines = 0;
	smoothH = 0.0f;
	lastDrawTime = 0;
}

/*
================
rvSubtitles::Clear
================
*/
void rvSubtitles::Clear( void ) {
	int i;
	for ( i = 0; i < MAX_SUBTITLE_LINES; i++ ) {
		lines[i].text.Clear();
		lines[i].startTime = 0;
		lines[i].endTime = 0;
	}
	numLines = 0;
	smoothH = 0.0f;
}

/*
================
rvSubtitles::AddLine
================
*/
void rvSubtitles::AddLine( const char *text, int endTime, subColor_t color ) {
	// 满员时挤掉最旧一条
	if ( numLines == MAX_SUBTITLE_LINES ) {
		int i;
		for ( i = 1; i < MAX_SUBTITLE_LINES; i++ ) {
			lines[i - 1] = lines[i];
		}
		numLines--;
	}

	subLine_t &sl = lines[numLines++];
	sl.text = text;
	sl.startTime = gameLocal.time;
	sl.endTime = endTime;
	sl.color = color;
}

/*
================
rvSubtitles::Sanitize

台词源串（transcribe text）里混有录音标注遗留物：
{furrow}/{idle} 等情绪标记、标记剥离后可能出现的连续空格。
这些只服务于口型/表情管线，展示前一律清理。
================
*/
void rvSubtitles::Sanitize( idStr &text ) {
	// 剥离 {…} 标记（不跨行，无嵌套）
	int lb;
	while ( ( lb = text.Find( '{' ) ) >= 0 ) {
		int rb = text.Find( '}', lb + 1 );
		if ( rb < 0 ) {
			break;
		}
		text = text.Left( lb ) + text.Right( text.Length() - rb - 1 );
	}

	// 折叠连续空格为单个
	idStr out;
	int i;
	bool lastSpace = false;
	for ( i = 0; i < text.Length(); i++ ) {
		if ( text[i] == ' ' ) {
			if ( lastSpace ) {
				continue;
			}
			lastSpace = true;
		} else {
			lastSpace = false;
		}
		out.Append( text[i] );
	}
	out.StripLeading( ' ' );
	out.StripTrailingWhitespace();
	text = out;
}

/*
================
rvSubtitles::IsAudible

说话点对玩家是否可听：
1. 过场演出 / 无关联声音 / 玩家自己说话 → 一律可听
2. 面向玩家的全局 VO（无线电类）→ 可听
3. 距离超出声音 shader 的 maxDistance（Q4 声音距离为游戏单位）→ 不可听
4. 近距离豁免外、说话实体不在玩家 PVS（隔墙/隔门）→ 不可听
================
*/
bool rvSubtitles::IsAudible( idEntity *bodyEnt, const idSoundShader *shader ) const {
	if ( !bodyEnt || !shader ) {
		return true;
	}
	// 过场期间过滤环境广播(speaker 实体)的字幕乱入,
	// 但保留剧情实体(idAI/idAFAttachment 等)的对白字幕。
	// 旧逻辑(2026-07-31)一刀切拦所有非 cinematic 实体,导致 Voss 被抓等
	// 过场对白(func_animate/head attachment 无 cinematic 标志)被误杀。
	// (2026-08-01 修复)
	if ( gameLocal.inCinematic ) {
		// Global sounds (e.g. intro ship broadcast vo_1_1_0_01_2) play worldwide and
		// the speaker often sits far from the camera — keep them regardless of distance.
		if ( shader->GetParms()->soundShaderFlags & SSF_GLOBAL ) {
			return true;
		}
		// 过场高潮的剧情广播/无线电但 Raven 未设 global 且 maxDist 仅 600, 镜头离
		// speaker 远会被下面的距离门控误杀, 按前缀强制放行:
		// - vo_2_2_10_251_* mcc_2 末尾"飞船被攻击"广播(radioWarnings)
		// - vo_1_2_12_30_* convoy2b"收割者登场"过场坦克无线电(Zebra 2/Bison 8,
		//   speaker 挂 s_global 全场可听但 shader decl 无 global 标志, 镜头距
		//   speaker 1100+ > 600 被误杀, 2026-08-14)
		// 后续发现同类过场广播可按此前缀数组扩展。
		static const char *cineWhitelist[] = {
			"vo_2_2_10_251_",
			"vo_1_2_12_30_",
			NULL
		};
		const char *cineSn = shader->GetName();
		if ( cineSn ) {
			for ( int ci = 0; cineWhitelist[ci]; ci++ ) {
				if ( idStr::Cmpn( cineSn, cineWhitelist[ci], (int)strlen( cineWhitelist[ci] ) ) == 0 ) {
					return true;
				}
			}
		}
		idPlayer *cinPlayer = gameLocal.GetLocalPlayer();
		// Cutscene actors (flagged "cinematic" "1"; head attachments inherit via
		// Actor::Attach) and the player always pass.
		if ( bodyEnt == cinPlayer || bodyEnt->cinematic ) {
			return true;
		}
		// Other entities (background speakers, outdoor NPCs) are gated by distance to
		// the cutscene camera. Stock cutscene dialogue plays through standalone speakers
		// (spkr_voss_intro etc.) that carry no cinematic flag but sit beside the camera;
		// bleed-through sources (talkLooms etc. in far hallways) exceed the sound's own
		// maxDistance and are dropped. The flag can't tell them apart (both are unflagged
		// vo_ speakers) — only space can.
		idCamera *cam = gameLocal.GetCamera();
		if ( cam ) {
			float cdist = ( cam->GetPhysics()->GetOrigin() - bodyEnt->GetPhysics()->GetOrigin() ).LengthFast();
			float cmax = shader->GetParms()->maxDistance;
			if ( cmax > 0.0f && cdist > cmax ) {
				if ( harm_g_subtitleDebug.GetBool() ) {
					gameLocal.Printf( "[SUB] skip (far from camera %.0f > %.0f): %s\n", cdist, cmax, bodyEnt->GetName() );
				}
				return false;
			}
			return true;
		}
		return false;
	}
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( !player || bodyEnt == player ) {
		return true;
	}
	if ( shader->IsVO_ForPlayer() ) {
		return true;
	}
	const soundShaderParms_t *parms = shader->GetParms();
	if ( parms->soundShaderFlags & SSF_GLOBAL ) {
		return true;
	}

	// 友军剧情语音按无线电对待（2026-07-17 用户反馈 Kovitch/Morris 台词缺字幕）：
	// 剧情脚本让队友在远处/隔墙处发言（实际经无线电传给玩家），距离与 PVS 门控
	// 会误拦——同队 actor 不做 PVS 门控，距离容差放宽 1.5 倍；
	// 敌军保留门控（防隔墙敌人喊话乱入字幕），距离留 1.15 容差纠正
	// 实体原点与声源发声点的偏差（实测 Morris 912 vs maxDistance 900 被误拦）
	bool friendly = false;
	if ( bodyEnt->IsType( idActor::GetClassType() ) ) {
		friendly = ( static_cast<idActor *>( bodyEnt )->team == player->team );
	}

	// speaker 实体（PA 广播）：广播系统设计为全设施覆盖，不应按距离/PVS 门控。
	// 环境音无 lipsync decl 不会进入此路径，只影响有 transcribe text 的 PA 广播。
	// (2026-08-01 用户反馈 Strogg 化后 Strogg 广播字幕时有时无：vo_pa_* 全部
	// 无 global 标志、maxDistance 仅 900，玩家走远后字幕被距离门控误拦)
	bool isSpeakerEnt = bodyEnt->IsType( idSound::GetClassType() );
	if ( isSpeakerEnt ) {
		return true;
	}

	float dist = ( player->GetPhysics()->GetOrigin() - bodyEnt->GetPhysics()->GetOrigin() ).LengthFast();
	float distLimit = parms->maxDistance * ( friendly ? 1.5f : 1.15f );
	if ( parms->maxDistance > 0.0f && dist > distLimit ) {
		if ( harm_g_subtitleDebug.GetBool() ) {
			gameLocal.Printf( "[SUB] skip (dist %.0f > limit %.0f): %s\n", dist, distLimit, bodyEnt->GetName() );
		}
		return false;
	}
	// speaker 实体已在上方提前 return，此处仅非 speaker 实体执行 PVS 门控
	if ( !friendly && harm_g_subtitlePVSCheck.GetBool() && dist > SUB_PVS_NEAR_DIST
	     && !gameLocal.InPlayerPVS( bodyEnt ) ) {
		if ( harm_g_subtitleDebug.GetBool() ) {
			gameLocal.Printf( "[SUB] skip (out of PVS, dist %.0f): %s\n", dist, bodyEnt->GetName() );
		}
		return false;
	}
	return true;
}

/*
================
rvSubtitles::Add

长台词按真实屏幕宽度拆行（fontdat 字形步进 × useScale，逐字符累计），
优先在空格处断行；拆出的各行共享同一个到期时间。
================
*/
void rvSubtitles::Add( const char *speaker, const char *text, int durationMs, subColor_t colorOverride ) {
	if ( !harm_g_subtitles.GetBool() || !text || !text[0] ) {
		return;
	}
	// #str_ 引用直接通过，GUI 文本控件自动解析为 lang 翻译文本
	// （v1.2.0 行为：不检查 # 前缀，lipsync decl 的 text 字段用 #str_xxx 引用 lang 翻译）

	idStr clean = text;
	Sanitize( clean );
	if ( !clean.Length() ) {
		return;
	}

	// karin: \xE4\xBA\xBA\xE5\x90\x8D\xE6\xB1\x89\xE5\x8C\x96 \xE2\x80\x94\xE2\x80\x94 \xE7\xBF\xBB\xE8\xAF\x91\xE5\x89\x8D\xE7\xBC\x80\xE5\x92\x8C\xE5\x86\x85\xE5\xAE\xB9\xE4\xB8\xAD\xE7\x9A\x84\xE4\xBA\xBA\xE5\x90\x8D
	idStr speakerStr = speaker ? speaker : "";
	if ( harm_g_cnNames.GetBool() && speakerStr.Length() ) {
		// \xE5\x85\x88\xE5\xB0\x9D\xE8\xAF\x95\xE7\xB2\xBE\xE7\xA1\xAE\xE5\x8C\xB9\xE9\x85\x8D\xEF\xBC\x88\xE5\xA4\x84\xE7\x90\x86 knownSpeakers \xE8\xBF\x94\xE5\x9B\x9E\xE7\x9A\x84\xE5\xB0\x8F\xE5\x86\x99\xE5\xAE\x9E\xE4\xBD\x93\xE5\x90\x8D\xEF\xBC\x89
		const char *exact = TranslateSpeakerName( speakerStr.c_str() );
		if ( exact != speakerStr.c_str() ) {
			speakerStr = exact;
		} else {
			// \xE5\x90\xAB\xE5\x86\x9B\xE8\xA1\x94\xE7\x9A\x84\xE5\xA4\x8D\xE5\x90\x88\xE5\x89\x8D\xE7\xBC\x80\xEF\xBC\x88\xE2\x80\x9C\xE4\xB8\xAD\xE5\xA3\xAB Morris\xE2\x80\x9D\xE2\x86\x92\xE2\x80\x9C\xE4\xB8\xAD\xE5\xA3\xAB\xE8\x8E\xAB\xE9\x87\x8C\xE6\x96\xAF\xE2\x80\x9D\xEF\xBC\x89
			TranslateNamesInText( speakerStr );
		}
		NormalizeRank( speakerStr );
	}
	TranslateNamesInText( clean );
	// \xE6\x95\x8C\xE6\x96\xB9\xE5\x8D\x95\xE4\xBD\x8D\xE5\x90\x8D\xE7\x94\xA8\xE3\x80\x8C\xE3\x80\x8D\xE5\x8C\x85\xE8\xA3\xB9
	BracketEnemyNames( clean );

	const char *translatedSpeaker = speakerStr.Length() ? speakerStr.c_str() : NULL;

	idStr full;
	if ( translatedSpeaker && translatedSpeaker[0] && translatedSpeaker[0] != '#' ) {
		full = va( "%s\xEF\xBC\x9A%s", translatedSpeaker, clean.c_str() );
	} else {
		full = clean;
	}

	// karin: 去重——同一内容 300ms 内不重复添加。
	// 比较纯内容（不含前缀），避免不同挂钩以不同前缀添加同一声音时绕过去重
	// （如 AI alert hook 加"敌军：xxx"，Entity StartSound hook 加"monster：xxx"）
	if ( numLines > 0 ) {
		subLine_t &last = lines[numLines - 1];
		if ( gameLocal.time - last.startTime < 300
			&& last.text.Length() >= clean.Length()
			&& last.text.Right( clean.Length() ) == clean ) {
			if ( harm_g_subtitleDebug.GetBool() ) {
				gameLocal.Printf( "[SUB] dedup: skipped duplicate content '%s'\n", clean.c_str() );
			}
			return;
		}
	}

	// 按说话人来源分配字幕配色(舰载广播蓝/敌方广播黄/无线电绿/其余白,统一正常亮度)
	subColor_t color;
	if ( colorOverride != SUB_COLOR_NORMAL ) {
		color = colorOverride;
	} else if ( !speaker || !speaker[0] || speaker[0] == '#' ) {
		color = SUB_COLOR_NORMAL;	// 无说话人(无名环境音) — 白
	} else if ( strstr( speaker, "\xE8\x88\xB0\xE8\xBD\xBD\xE5\xB9\xBF\xE6\x92\xAD" ) != NULL ) { // 舰载广播
		color = SUB_COLOR_HUMANCAST;	// 人类/舰船广播 — 蓝
	} else if ( strstr( speaker, "\xE5\xB9\xBF\xE6\x92\xAD" ) != NULL			// 含"广播"（敌军广播/广播）
	        || idStr::Icmp( speaker, "PA" ) == 0
	        || strstr( speaker, "\xE6\x92\xAD\xE6\x8A\xA5" ) != NULL ) {		// 含"播报"
		color = SUB_COLOR_BROADCAST;	// Strogg广播 — 黄
	} else if ( idStr::Cmp( speaker, "\xE6\x97\xA0\xE7\xBA\xBF\xE7\x94\xB5" ) == 0	// 无线电
	        || idStr::Icmp( speaker, "Radio" ) == 0 ) {
		color = SUB_COLOR_RADIO;		// 无线电 — 青
	} else if ( idStr::Icmp( speaker, "Makron" ) == 0 ) {
		color = SUB_COLOR_MAKRON;		// Makron — 紫
	} else if ( idStr::Cmp( speaker, "\xE6\x95\x8C\xE5\x86\x9B" ) == 0 ) {	// 敌军
		color = SUB_COLOR_ENEMY;		// 敌军语音 — 红
	} else {
		color = SUB_COLOR_NORMAL;	// 角色对白 — 白
		// 敌人单位名（步兵/机炮兵/角斗士等）→ 红
		for ( int i = 0; enemyUnitNames[i]; i++ ) {
			if ( idStr::Cmp( speaker, enemyUnitNames[i] ) == 0 ) {
				color = SUB_COLOR_ENEMY;
				break;
			}
		}
	}

	if ( harm_g_subtitleDebug.GetBool() ) {
		gameLocal.Printf( "[SUB] show (%dms) color=%d: %s\n", durationMs, color, full.c_str() );
	}

	int dur = durationMs;
	if ( dur < harm_g_subtitleMinTime.GetInteger() ) {
		dur = harm_g_subtitleMinTime.GetInteger();
	}
	int endTime = gameLocal.time + dur + harm_g_subtitleHoldTime.GetInteger();

	if ( !subMetricsTried ) {
		SUB_LoadFontMetrics();
	}

	const char *s = full.c_str();
	int len = full.Length();
	int pos = 0;
	while ( pos < len ) {
		float px = 0.0f;		// 本行累计屏幕宽
		int cut = pos;			// 硬切位置（字节）
		int lastSpace = -1;		// 行尾附近的空格断点（字节）
		int lastSpaceAny = -1;	// 任意位置最近空格（仅断词兜底用）
		int i = pos;
		while ( i < len ) {
			unsigned char c = s[i];
			int step = 1;
			unsigned int cp = c;
			if ( c >= 0xF0 ) {
				step = 4;
				cp = c & 0x07;
			} else if ( c >= 0xE0 ) {
				step = 3;
				cp = c & 0x0F;
			} else if ( c >= 0xC0 ) {
				step = 2;
				cp = c & 0x1F;
			}
			int k;
			for ( k = 1; k < step && i + k < len; k++ ) {
				cp = ( cp << 6 ) | ( s[i + k] & 0x3F );
			}
			float adv = SUB_CharAdvance( cp );
			if ( px + adv > SUB_TEXT_W ) {
				break;
			}
			px += adv;
			if ( c == ' ' ) {
				if ( px > SUB_TEXT_W * 0.92f ) {
					lastSpace = i;
				}
				lastSpaceAny = i;
			}
			i += step;
		}
		cut = i;
		if ( i < len ) {
			// 空格断点只在行尾附近（>70% 预算）才可取：中西文混排中前部的空格
			// （如 "瘫痪了 Strogg 的"）不能把整行断得过短。
			// 例外：溢出点落在英文单词中间时不硬切断词，退回任意位置的空格。
			bool midWord = i > pos &&
				isalnum( ( unsigned char )s[i] ) && isalnum( ( unsigned char )s[i - 1] );
			if ( lastSpace > pos ) {
				cut = lastSpace;
			} else if ( midWord && lastSpaceAny > pos ) {
				cut = lastSpaceAny;
			} else {
				// 行首禁则：中文标点不落行首，并入当前行（允许微超宽）
				static const char *noHead[] = {
					"\xEF\xBC\x8C", "\xE3\x80\x82", "\xEF\xBC\x81", "\xEF\xBC\x9F",
					"\xE3\x80\x81", "\xEF\xBC\x9B", "\xEF\xBC\x9A", "\xE2\x80\xA6",
					"\xE2\x80\x9D", "\xE3\x80\x8D", "\xEF\xBC\x89", NULL
				};	// ，。！？、；：…"」）
				bool absorbed = true;
				while ( absorbed ) {
					absorbed = false;
					int t;
					for ( t = 0; noHead[t]; t++ ) {
						int nlen = ( int )strlen( noHead[t] );
						if ( cut + nlen <= len && idStr::Cmpn( s + cut, noHead[t], nlen ) == 0 ) {
							cut += nlen;
							absorbed = true;
							break;
						}
					}
				}
			}
		}
		idStr seg = full.Mid( pos, cut - pos );
		seg.StripTrailingWhitespace();
		if ( seg.Length() ) {
			AddLine( seg.c_str(), endTime, color );
		}
		pos = cut;
		while ( pos < len && s[pos] == ' ' ) {
			pos++;
		}
	}
}

/*
================
rvSubtitles::LookupSpeaker

查声音 shader 名 → 说话人映射（speaker_map.txt）。供 Sound.cpp/Misc.cpp 挂钩区分
角色台词 vs 环境广播：有映射返回角色名，无映射返回 NULL（调用方 fallback 到默认前缀）。
================
*/
const char *rvSubtitles::LookupSpeaker( const char *shaderName ) {
	if ( !shaderName || !shaderName[0] ) return NULL;
	if ( !subSpeakerMapLoaded ) SUB_LoadSpeakerMap();
	const char *mapped = subSpeakerMap.GetString( shaderName );
	return ( mapped && mapped[0] ) ? mapped : NULL;
}

/*
================
rvSubtitles::AddFromEntity

从说话实体推断说话人名：
1. npc_name 有效且非占位名（Unnamed/未命名）→ 用之（本地化后）
2. speaker 实体(广播/画面外角色)优先按 shader 名查 speaker_map → 角色名/广播名
3. 否则在实体名里识别已知角色（过场演出实体常以角色命名，如 cin_voss）
4. 都失败 → 用 fallbackSpeaker（如"广播"）或留空
================
*/
void rvSubtitles::AddFromEntity( idEntity *bodyEnt, const char *text, int durationMs, const idSoundShader *shader, const char *fallbackSpeaker ) {
	static const char *knownSpeakers[] = {
		"cortez", "bidwell", "voss", "morris", "anderson", "rhodes",
		"sledge", "strauss", "kane", "walker", "hollenbeck", "scott",
		"mahler", "silverman", "harper", "makron", NULL
	};

	// 玩家听不到的说话不出字幕
	if ( !IsAudible( bodyEnt, shader ) ) {
		return;
	}

	idStr speaker;
	if ( bodyEnt ) {
		const char *rawName = bodyEnt->spawnArgs.GetString( "npc_name" );
		if ( rawName && rawName[0] ) {
			const char *loc = common->GetLocalizedString( rawName );
			// "未命名"必须用显式 UTF-8 转义：MSVC 把中文字面量按系统码页(GBK)编码，
			// 与 GetLocalizedString 返回的 UTF-8 字节永远不等，过滤会失效
			if ( loc && loc[0] && loc[0] != '#' &&
				 idStr::Icmp( loc, "Unnamed" ) != 0 &&
				 idStr::Cmp( loc, "\xE6\x9C\xAA\xE5\x91\xBD\xE5\x90\x8D" ) != 0 ) {
				speaker = loc;
			}
		}
		// speaker 实体优先按声音名查 speaker_map（build_lang 生成）：声音语义比实体名
		// 可靠。walkerPA_1 实体名含地图名"walker"，被下面的 knownSpeakers 子串匹配误判
		// 成角色 Walker → 字幕显示"沃克："+白色，实为敌军 PA 广播（2026-08-14 用户反馈）。
		// 提前查映射让"敌军广播"等声音映射优先生效；无映射的声音仍走实体名/兜底。
		if ( !speaker.Length() && shader && bodyEnt->IsType( idSound::GetClassType() ) ) {
			const char *sndMapped = LookupSpeaker( shader->GetName() );
			if ( sndMapped && sndMapped[0] ) {
				speaker = sndMapped;
			}
		}
		if ( !speaker.Length() ) {
			// 从实体名识别角色（大小写不敏感子串）
			idStr entName = bodyEnt->GetName();
			entName.ToLower();
			int i;
			for ( i = 0; knownSpeakers[i]; i++ ) {
				if ( entName.Find( knownSpeakers[i] ) >= 0 ) {
					speaker = knownSpeakers[i];
					speaker[0] = ( char )idStr::ToUpper( speaker[0] );
					break;
				}
			}
		}
		// 2026-07-18 用户要求字幕都有来源：无名友军 AI 兜底为"士兵"
		// （中文必须写 UTF-8 转义，MSVC GBK 码页坑）
		if ( !speaker.Length() && !fallbackSpeaker && bodyEnt->IsType( idActor::GetClassType() ) ) {
			idPlayer *pl = gameLocal.GetLocalPlayer();
			if ( pl && static_cast<idActor *>( bodyEnt )->team == pl->team ) {
				speaker = "\xE5\xA3\xAB\xE5\x85\xB5";	// 士兵
			}
		}
		// speaker 实体(广播 PA / 画面外角色如机甲检修 Morois)按 shader 名查角色映射:
		// 有映射=角色台词(如"Morois 下士",正常亮色), 无映射=环境广播(fallback"广播"+淡色)
		if ( !speaker.Length() && shader && bodyEnt->IsType( idSound::GetClassType() ) ) {
			const char *sn = shader->GetName();
			// alert 类声音(关卡 speaker 播放的怪物发现喊话, 如 berserker_alert):
			// bodyEnt 是 speaker 而非怪物, spawnclass 推断不适用; 从 shader 名推断怪物名,
			// 否则落 fallback "舰载广播" 显示成蓝色广播(应为红色怪物名)
			static const struct { const char *shader; const char *cn; } alertMap[] = {
				{ "berserker_alert", "\xE7\x8B\x82\xE6\x88\x98\xE5\xA3\xAB" },
				{ "gunner_alert", "\xE6\x9C\xBA\xE7\x82\xAE\xE5\x85\xB5" },
				{ "iron_maiden_alert", "\xE9\x93\x81\xE5\xA8\x98\xE5\xAD\x90" },
				{ "gladiator_alert", "\xE8\xA7\x92\xE6\x96\x97\xE5\xA3\xAB" },
				{ "failedtransfer_alert", "\xE5\xA4\xB1\xE8\xB4\xA5\xE6\x94\xB9\xE9\x80\xA0\xE4\xBD\x93" },
				{ "slimytransfer_alert", "\xE5\xA4\xB1\xE8\xB4\xA5\xE6\x94\xB9\xE9\x80\xA0\xE4\xBD\x93" },
				{ "scientist_alert", "\xE7\xA7\x91\xE5\xAD\xA6\xE5\xAE\xB6" },
				{ "grunt_alert", "\xE6\xAD\xA5\xE5\x85\xB5" },
				{ "sentry_alert", "\xE5\x93\xA8\xE5\x8D\xAB" },
				{ "smarine_alert", "\xE9\x99\x86\xE6\x88\x98\xE5\x85\xB5" },
				{ NULL, NULL }
			};
			for ( int i = 0; alertMap[i].shader; i++ ) {
				if ( idStr::Cmp( sn, alertMap[i].shader ) == 0 ) {
					speaker = alertMap[i].cn;
					break;
				}
			}
			if ( !speaker.Length() ) {
				const char *mapped = LookupSpeaker( sn );
				if ( mapped ) speaker = mapped;
			}
		}
	}
	// 武器改造声音 → 粉红色标志（speaker 保持说话人推断，不覆盖）
	bool isWeaponMod = false;
	if ( shader ) {
		const char *sn = shader->GetName();
		static const char *weaponModSounds[] = {
			"vo_1_1_11_565_1", "vo_1_2_12_10_1", "vo_2_2_10_335_1",
			"vo_2_2_4_45_1", "vo_2_2_4_45_2", "vo_2_2_4_75_1",
			"vo_2_2_4_75_2", "vo_2_2_7_171_1", "vo_3_1_5_3_1",
			"vo_3_1_3_25_1", "vo_1_1_13_20_1", "vo_1_1_5_40_1", NULL
		};
		for ( int i = 0; weaponModSounds[i]; i++ ) {
			if ( idStr::Cmp( sn, weaponModSounds[i] ) == 0 ) {
				isWeaponMod = true;
				break;
			}
		}
	}
	// 敌人怪物：从 spawnclass 推断中文名（步兵/机炮兵等），显示具体怪物名而非"舰载广播"
	if ( !speaker.Length() && bodyEnt && bodyEnt->IsType( idActor::GetClassType() ) ) {
		idPlayer *pl = gameLocal.GetLocalPlayer();
		if ( pl && static_cast<idActor *>( bodyEnt )->team != pl->team ) {
			const char *cls = bodyEnt->spawnArgs.GetString( "spawnclass", "" );
			static const struct { const char *cls; const char *cn; } monMap[] = {
				{ "rvMonsterGrunt", "\xE6\xAD\xA5\xE5\x85\xB5" },
				{ "rvMonsterGunner", "\xE6\x9C\xBA\xE7\x82\xAE\xE5\x85\xB5" },
				{ "rvMonsterBerserker", "\xE7\x8B\x82\xE6\x88\x98\xE5\xA3\xAB" },
				{ "rvMonsterGladiator", "\xE8\xA7\x92\xE6\x96\x97\xE5\xA3\xAB" },
				{ "rvMonsterIronMaiden", "\xE9\x93\x81\xE5\xA8\x98\xE5\xAD\x90" },
				{ "rvMonsterFailedTransfer", "\xE5\xA4\xB1\xE8\xB4\xA5\xE6\x94\xB9\xE9\x80\xA0\xE4\xBD\x93" },
				{ "rvMonsterScientist", "\xE7\xA7\x91\xE5\xAD\xA6\xE5\xAE\xB6" },
				{ "rvMonsterSentry", "\xE5\x93\xA8\xE5\x8D\xAB" },
				{ "rvMonsterStroggMarine", "\xE9\x99\x86\xE6\x88\x98\xE5\x85\xB5" },
				{ "rvMonsterSlimyTransfer", "\xE5\xA4\xB1\xE8\xB4\xA5\xE6\x94\xB9\xE9\x80\xA0\xE4\xBD\x93" },
				{ "rvMonsterBossBuddy", "\xE6\xB2\x83\xE6\x96\xAF" },	// 改造后 Voss boss 残存意识对白
				{ NULL, NULL }
			};
			for ( int i = 0; monMap[i].cls; i++ ) {
				if ( idStr::Icmp( cls, monMap[i].cls ) == 0 ) {
					speaker = monMap[i].cn;
					break;
				}
			}
		}
	}
	if ( !speaker.Length() && fallbackSpeaker && fallbackSpeaker[0] ) {
		// 区分 Strogg 广播(vo_pa_*)与人类/舰船广播：前者黄色，后者绿色
		if ( shader && idStr::Cmp( fallbackSpeaker, "\xE5\xB9\xBF\xE6\x92\xAD" ) == 0 ) {
			const char *sn = shader->GetName();
			if ( sn && idStr::Cmpn( sn, "vo_pa_", 6 ) != 0 ) {
				speaker = "\xE8\x88\xB0\xE8\xBD\xBD\xE5\xB9\xBF\xE6\x92\xAD"; // 舰载广播
			} else {
				speaker = "\xE6\x95\x8C\xE5\x86\x9B\xE5\xB9\xBF\xE6\x92\xAD"; // 敌军广播（vo_pa_）
			}
		} else {
			speaker = fallbackSpeaker;
		}
	}

	Add( speaker.Length() ? speaker.c_str() : NULL, text, durationMs, isWeaponMod ? SUB_COLOR_WEAPONMOD : SUB_COLOR_NORMAL );
}

/*
================
Harm_ApplyResolution

主菜单设置页选分辨率/比例后由 GUI onActionRelease 触发。原版靠引擎命令
fixup_mode 枚举显示模式填充选项，idTech4A++ 未实现该命令 → "No Choices Defined"。
改为 GUI 硬编码 choices/values 绑 harm_g_resIndex，由此命令解析索引 →
r_mode -1 + r_customWidth/Height + vid_restart。

挂在控制台命令而非 rvSubtitles::Draw()：idGameLocal::Draw 主菜单时
player==NULL 提前返回，字幕 Draw 不执行；命令由 GUI 主动触发，主菜单可用。
================
*/
void Harm_ApplyResolution( void ) {
	int resIdx = harm_g_resIndex.GetInteger();
	common->Printf( "[HARM] harm_applyVideo called, resIdx=%d\n", resIdx );
	int rw = 0, rh = 0;
	switch ( resIdx ) {
		// 4:3
		case 100: rw = 1024; rh = 768;  break;
		case 101: rw = 1280; rh = 960;  break;
		case 102: rw = 1600; rh = 1200; break;
		// 16:9
		case 200: rw = 1280; rh = 720;  break;
		case 201: rw = 1366; rh = 768;  break;
		case 202: rw = 1600; rh = 900;  break;
		case 203: rw = 1920; rh = 1080; break;
		case 204: rw = 2560; rh = 1440; break;
		case 205: rw = 3840; rh = 2160; break;
		// 16:10
		case 300: rw = 1280; rh = 800;  break;
		case 301: rw = 1440; rh = 900;  break;
		case 302: rw = 1680; rh = 1050; break;
		case 303: rw = 1920; rh = 1200; break;
		case 304: rw = 2560; rh = 1600; break;
		default:  rw = 0; break;
	}
	if ( rw > 0 ) {
		cvarSystem->SetCVarInteger( "r_mode", -1 );
		cvarSystem->SetCVarInteger( "r_customWidth", rw );
		cvarSystem->SetCVarInteger( "r_customHeight", rh );
		common->Printf( "[HARM] set r_mode=-1 custom=%dx%d, vid_restart queued\n", rw, rh );
	}
	// 无论是否设了分辨率都 vid_restart：改比例（r_aspectRatio）也需重启视频生效
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
}

/*
================
rvSubtitles::Draw

整块半透明面板：底部固定，高度=行数，平滑伸缩；
行順序旧→新自上而下，新行从底部顶入。
================
*/
void rvSubtitles::Draw( void ) {
	int i;
	int now = gameLocal.time;

	// 调试注入
	if ( harm_g_subtitleTest.GetString()[0] ) {
		Add( NULL, harm_g_subtitleTest.GetString(), 4000 );
		harm_g_subtitleTest.SetString( "" );
	}

	// karin: \xE5\xAD\x97\xE5\xB9\x95\xE6\xBC\x94\xE7\xA4\xBA\xE6\xA8\xA1\xE5\xBC\x8F \xE2\x80\x94\xE2\x80\x94 \xE5\xBE\xAA\xE7\x8E\xAF\xE6\xBC\x94\xE7\xA4\xBA\xE6\x89\x80\xE6\x9C\x89\xE7\xB1\xBB\xE5\x9E\x8B
	if ( harm_g_subtitleDemo.GetBool() ) {
		// \xE6\xBC\x94\xE7\xA4\xBA\xE7\x94\xA8\xE4\xBE\x8B\xEF\xBC\x9A\xE8\xA6\x86\xE7\x9B\x96\xxE5\x90\x84\xE7\xA7\x8D\xE5\xAD\x97\xE5\xB9\x95\xE7\xB1\xBB\xE5\x9E\x8B\xE5\x92\x8C\xE9\x85\x8D\xE8\x89\xB2
		struct DemoCase { const char *speaker; const char *text; subColor_t color; };
		static const DemoCase cases[] = {
			// \xE6\x95\x8C\xE5\x86\x9B\xE8\xAF\xAD\xE9\x9F\xB3\xEF\xBC\x88\xE7\xBA\xA2\xE8\x89\xB2\xEF\xBC\x89
			{ "\xE6\x95\x8C\xE5\x86\x9B", "\xE6\x9C\xBA\xE7\x82\xAE\xE5\x85\xB5\xE5\x8F\x91\xE7\x8E\xB0\xE7\x9B\xAE\xE6\xA0\x87\xEF\xBC\x81", SUB_COLOR_ENEMY },				// 敌军：机炮兵发现目标！
			{ "\xE6\x95\x8C\xE5\x86\x9B", "\xE7\x8B\x82\xE6\x88\x98\xE5\xA3\xAB\xE6\xAD\xA3\xE5\x9C\xA8\xE9\x9D\xA0\xE8\xBF\x91\xEF\xBC\x81", SUB_COLOR_ENEMY },				// 敌军：狂战士正在靠近！
			{ "\xE6\x95\x8C\xE5\x86\x9B", "\xE8\xA7\x92\xE6\x96\x97\xE5\xA3\xAB\xE5\xB7\xB2\xE9\x94\x81\xE5\xAE\x9A\xE4\xBD\xA0\xEF\xBC\x81", SUB_COLOR_ENEMY },				// 敌军：角斗士已锁定你！
			// \xE8\xA7\x92\xE8\x89\xB2\xE5\xAF\xB9\xE7\x99\xBD\xEF\xBC\x88\xE7\x99\xBD\xE8\x89\xB2\xEF\xBC\x89
			{ "Bidwell", "Kane\xEF\xBC\x81\xE6\x88\x91\xE4\xBB\xAC\xE9\x9C\x80\xE8\xA6\x81\xE4\xBD\xA0\xEF\xBC\x81", SUB_COLOR_NORMAL },				// Bidwell：Kane！我们需要你！
			{ "Voss", "\xE6\x89\x80\xE6\x9C\x89\xE5\xB0\x8F\xE9\x98\x9F\xEF\xBC\x8C\xE5\x89\x8D\xE8\xBF\x9B\xEF\xBC\x81", SUB_COLOR_NORMAL },					// Voss：所有小队，前进！
			{ "Strauss", "Kane\xE4\xB8\x8B\xE5\xA3\xAB\xEF\xBC\x8C\xE8\xBF\x87\xE6\x9D\xA5\xE3\x80\x82", SUB_COLOR_NORMAL },					// Strauss：Kane下士，过来。
			// \xE6\x97\xA0\xE7\xBA\xBF\xE7\x94\xB5\xEF\xBC\x88\xE9\x9D\x92\xE8\x89\xB2\xEF\xBC\x89
			{ "\xE6\x97\xA0\xE7\xBA\xBF\xE7\x94\xB5", "\xE6\xB1\x82\xE6\x95\x91\xEF\xBC\x81\xE8\xBF\x99\xE9\x87\x8C\xE6\x98\xAF\xE5\xB7\xB4\xE9\xA1\xBF\xE5\x8F\xB7\xE2\x80\x94\xE2\x80\x94", SUB_COLOR_RADIO },	// 无线电：求救！这里是巴顿号——
			// \xE5\xB9\xBF\xE6\x92\xAD\xEF\xBC\x88\xE9\xBB\x84\xE8\x89\xB2\xEF\xBC\x89
			{ "\xE5\xB9\xBF\xE6\x92\xAD", "\xE5\x85\xA5\xE4\xBE\xB5\xE6\xA3\x80\xE6\xB5\x8B\xE3\x80\x82\xE5\xB0\x81\xE9\x94\x81\xE6\x89\x80\xE6\x9C\x89\xE9\x80\x9A\xE9\x81\x93\xE3\x80\x82", SUB_COLOR_BROADCAST },	// 广播：入侵检测。封锁所有通道。
			// Makron\xEF\xBC\x88\xE7\xB4\xAB\xE8\x89\xB2\xEF\xBC\x89
			{ "makron", "\xE4\xBD\xA0\xE4\xBC\x9A\xE5\xA4\xB1\xE8\xB4\xA5\xE7\x9A\x84\xEF\xBC\x81", SUB_COLOR_MAKRON },					// makron：你会失败的！
			// \xE9\x97\xB4\xE9\x9A\x94\xE5\x8F\xB7\xE6\xB5\x8B\xE8\xAF\x95
			{ NULL, "\xE6\xB5\x8B\xE8\xAF\x95\xC2\xB7\xE9\x97\xB4\xE9\x9A\x94\xE5\x8F\xB7\xC2\xB7\xE6\xB5\x8B\xE8\xAF\x95", SUB_COLOR_NORMAL },				// 测试·间隔号·测试
		};
		static const int numCases = sizeof(cases) / sizeof(cases[0]);
		static int demoIdx = 0;
		static int demoNextTime = 0;
		if ( now >= demoNextTime ) {
			const DemoCase &dc = cases[demoIdx];
			Add( dc.speaker, dc.text, 3000, dc.color );
			demoIdx = (demoIdx + 1) % numCases;
			demoNextTime = now + 3500;	// \xE6\xAF\x8F 3.5 \xE7\xA7\x92\xE5\x88\x87\xE6\x8D\xA2
			if ( demoIdx == 0 ) {
				gameLocal.Printf( "[SUB DEMO] \xE5\xBE\xAA\xE7\x8E\xAF\xE9\x87\x8D\xE6\x96\xB0\xE5\xBC\x80\xE5\xA7\x8B\n" );
			}
		}
	}

	// 过期或跨地图残留（startTime 超前于当前时间）的行删除
	for ( i = 0; i < numLines; ) {
		if ( lines[i].endTime <= now || lines[i].startTime > now ) {
			int j;
			for ( j = i + 1; j < numLines; j++ ) {
				lines[j - 1] = lines[j];
			}
			numLines--;
		} else {
			i++;
		}
	}

	if ( !harm_g_subtitles.GetBool() ) {
		return;
	}

	// 平滑高度（一阶趋近）
	int dt = now - lastDrawTime;
	lastDrawTime = now;
	if ( dt < 0 || dt > 200 ) {
		dt = 16;
	}
	float targetH = numLines ? ( numLines * SUB_ROW_H + SUB_PAD * 2.0f ) : 0.0f;
	float k = dt / 130.0f;
	if ( k > 1.0f ) {
		k = 1.0f;
	}
	smoothH += ( targetH - smoothH ) * k;
	if ( smoothH < 0.5f && !numLines ) {
		smoothH = 0.0f;
		return;		// 完全收起时不绘制
	}

	// 每帧按名查找（uiManager 内部有实例缓存）。绝不可把返回指针存成员跨图用：
	// 换图时 GUI 实例被释放重建，悬空指针 SetState/Redraw 写坏堆 → 概率性
	// c0000409 崩溃（2026-07-17 换图崩溃转储定位于此）
	idUserInterface *gui = uiManager->FindGui( "guis/subtitles.gui", true, false, true );
	if ( !gui ) {
		static bool warned = false;
		if ( !warned ) {
			warned = true;
			gameLocal.Warning( "rvSubtitles: guis/subtitles.gui not found, subtitles disabled" );
		}
		return;
	}

	float panelTop = SUB_BOTTOM - smoothH;
	float bgAlpha = 0.55f * ( smoothH / ( SUB_ROW_H + SUB_PAD * 2.0f ) );
	if ( bgAlpha > 0.55f ) {
		bgAlpha = 0.55f;
	}

	gui->SetStateFloat( "subBgY", panelTop );
	gui->SetStateFloat( "subBgH", smoothH );
	gui->SetStateFloat( "subBgA", bgAlpha );

	for ( i = 0; i < MAX_SUBTITLE_LINES; i++ ) {
		if ( i < numLines ) {
			// 行淡入淡出
			float aIn = ( now - lines[i].startTime ) / ( float )FADE_IN_MS;
			float aOut = ( lines[i].endTime - now ) / ( float )FADE_OUT_MS;
			float a = ( aIn < aOut ) ? aIn : aOut;
			if ( a > 1.0f ) {
				a = 1.0f;
			}
			if ( a < 0.0f ) {
				a = 0.0f;
			}
			// 行锚定面板底部：旧行过期删除时剩余行保持原位（只有背景板平滑收缩），
			// 面板生长期间取 max(静止位, 顶部滑入位) 保留新行从下方顶入的平滑效果
			float restY = SUB_BOTTOM - SUB_PAD - ( numLines - i ) * SUB_ROW_H + SUB_ROW_ADJ;
			float slideY = panelTop + SUB_PAD + i * SUB_ROW_H + SUB_ROW_ADJ;
			float rowY = ( restY > slideY ) ? restY : slideY;
			gui->SetStateString( va( "subText%d", i ), lines[i].text.c_str() );
			// 按配色类型设前景色 R/G/B(subtitles.gui forecolor 读这些变量)
			// 广播黄 / 无线电青 / 其余白;alpha 不再因类型打折(2026-08-01 用户要求统一亮度)
			float subR, subG, subB;
			switch ( lines[i].color ) {
				case SUB_COLOR_BROADCAST: subR = 1.0f;  subG = 0.80f; subB = 0.15f; break;
				case SUB_COLOR_RADIO:     subR = 0.40f; subG = 0.90f; subB = 0.40f; break;
				case SUB_COLOR_MAKRON:    subR = 0.75f; subG = 0.35f; subB = 1.0f;  break;
				case SUB_COLOR_HUMANCAST: subR = 0.35f; subG = 0.60f; subB = 1.0f;  break;
				case SUB_COLOR_ENEMY:     subR = 1.0f;  subG = 0.35f; subB = 0.35f; break;
			case SUB_COLOR_WEAPONMOD: subR = 1.0f;  subG = 0.4f;  subB = 0.7f;  break; // 武器升级 — 粉红
				default:                   subR = 0.95f; subG = 0.95f; subB = 0.95f; break;
			}
			gui->SetStateFloat( va( "subTxtR%d", i ), subR );
			gui->SetStateFloat( va( "subTxtG%d", i ), subG );
			gui->SetStateFloat( va( "subTxtB%d", i ), subB );
			gui->SetStateFloat( va( "subTxtA%d", i ), a * 0.97f );
			gui->SetStateFloat( va( "subRowY%d", i ), rowY );
		} else {
			gui->SetStateString( va( "subText%d", i ), "" );
			gui->SetStateFloat( va( "subTxtA%d", i ), 0.0f );
			gui->SetStateFloat( va( "subRowY%d", i ), SUB_BOTTOM );
		}
	}
	gui->Redraw( now );
}
