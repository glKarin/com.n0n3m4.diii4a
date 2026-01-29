dnl
dnl This file will be processed to provide relevant package lists for the various packages built from FTE's source.
dnl The output will need to be combined with any other packages, and then signed for these packages to be considered valid.
dnl Users can add extra sources with the `pkg addsource URL' console command (will show a prompt, to try to avoid exploits).
dnl
define(`DATE',FTE_DATE)dnl
define(`REVISION',FTE_REVISION)dnl
define(`DLSIZE',`esyscmd(`stat --printf="%s" $1')')dnl
define(`SHA512',`esyscmd(`fteqw -sha512 $1')')dnl
define(`URL',`url		"$1"
		dlsize		"DLSIZE($1)"
 		sha512		"SHA512($1)" ')dnl
define(`CAT',`$1$2$3$4$5$6$7$8$9')dnl
define(`ZIP',`unzipfile	"$2"
		URL($1$3)')dnl
define(`FILE',`file		"$1$2$3$4$5$6$7$8$9"')dnl
define(`WINENGINE',`category	"Engine"
	ver			"REVISION"
	gamedir		""
	license		"GPLv2"
	{
		arch		"win_x86-FTE$1"
		FILE(fteqw_,REVISION,_32.exe)
		ZIP(win32/,CAT($2,.exe),CAT($2,_win32.zip))
	}
	{
		arch		"win_x64-FTE$1"
		FILE(fteqw_,REVISION,_64.exe)
		ZIP(win64/,CAT($2,64.exe),CAT($2,_win64.zip))
	}')dnl
define(`LINENGINE2',`{
		arch		"linux_amd64-FTE$1"
		FILE(fteqw_,REVISION,_64.bin)
		ZIP(linux_amd64/,CAT($2,64),$3)
	}')dnl
define(`LINENGINE',`LINENGINE2($1,$2,CAT($2_lin64.zip))')dnl
define(`HIDE',)dnl
define(`GAME',`ifelse(FTE_GAME,`$1',`$2'
,)')dnl
define(`TEST',`ifelse(FTE_TEST,`1',`	test		"1"
',`	test		"0"
')')dnl
{
	package	"fte_cl"
	WINENGINE(-m,fteqw)
	LINENGINE(-m,fteqw)
	title		"CAT(`FTE Engine ',DATE)"
	desc		"The awesome FTE engine (multi-renderer build)"
TEST()dnl
}
HIDE(`
{
	package	"fte_cl_gl"
dnl	WINENGINE(-gl,fteglqw)
dnl	//don't bother advertising it on linux
dnl	title		"CAT(`FTE Engine ',DATE,` - OpenGL'")
	desc		"The awesome FTE engine (OpenGL-only build)"
TEST()dnl
}
{
	package	"fte_cl_vk"
dnl	WINENGINE(-vk,ftevkqw)
dnl	//don't bother advertising it on linux
dnl	title		"CAT(`FTE Engine ',DATE,` - Vulkan')"
	desc		"The awesome FTE engine (Vulkan-only build)"
TEST()dnl
}
{
	package	"fte_cl_d3d"
dnl	WINENGINE(-d3d,fted3dqw)
	//no d3d on linux
dnl	title		"CAT(`FTE Engine ',DATE,` - Direct3D')"
	desc		"The awesome FTE engine (Direct3D-only build)"
TEST()dnl
}')dnl
{
	package	"fte_sv"
	WINENGINE(-sv,fteqwsv)
	LINENGINE2(-sv,fteqw-sv,fteqwsv_lin64.zip)
	title		"CAT(`FTE Engine ',DATE,` - Server')"
	desc		"The awesome FTE engine (server-only build)"
TEST()dnl
}
define(`WINPLUG',`category	"Plugins"
	ver			"REVISION"
	gamedir		""
	license		"GPLv2"
	{
		arch		"win_x86"
		FILE(fteplug_$1_x86.REVISION.dll)
		URL(win32/fteplug_$1_x86.dll)
	}
	{
		arch		"win_x64"
		FILE(fteplug_$1_x64.REVISION.dll)
		URL(win64/fteplug_$1_x64.dll)
	}')dnl
define(`WIN64PLUG',`category	"Plugins"
	ver			"REVISION"
	gamedir		""
	license		"GPLv2"
	{
		arch		"win_x64"
		FILE(fteplug_$1_x64.REVISION.dll)
		URL(win64/fteplug_$1_x64.dll)
	}')dnl
define(`LINPLUG',`{
		arch		"linux_amd64"
		FILE(fteplug_$1_amd64.REVISION,.so)
		URL(linux_amd64/fteplug_$1_amd64.so)
	}')dnl
GAME(quake,
`{
	package "fteplug_ezhud"
	WINPLUG(ezhud)
	LINPLUG(ezhud)
	title			"EzHud Plugin"
	replace			"ezhud"
	desc			"Some lame alternative configurable hud."
TEST()dnl
}')dnl
GAME(quake,
`{
	package 		"fteplug_qi"
	WINPLUG(qi)
	LINPLUG(qi)
	category		"Plugins"
	title			"Quake Injector Plugin"
	replace			"Quake Injector Plugin"
	author			"Spike"
	website			"https://www.quaddicted.com/reviews/"
	desc			"Provides a way to quickly list+install+load numerous different maps and mods. Some better than others."
	desc			"If youre a single-player fan then these will keep you going for quite some time."
	desc			"The database used is from quaddicted.com."
TEST()dnl
}')dnl
{
	package "fteplug_irc"
	WINPLUG(irc)
	LINPLUG(irc)
	title			"IRC Plugin"
	replace			"IRC Plugin"
	desc			"Allows you to converse on IRC servers in-game."
	desc			"Requires manual configuration."
TEST()dnl
}
{
	package "fteplug_xmpp"
	WINPLUG(xmpp)
	LINPLUG(xmpp)
	title			"XMPP Plugin"
	desc			"Allows you to converse on XMPP servers. This also includes a method for NAT holepunching between contacts."
	desc			"Requires manual configuration."
TEST()dnl
}
{
	package "fteplug_openssl"
	WINPLUG(openssl)
	license			"GPLv3"	//Apache2+GPLv2=GPLv3
	title			"OpenSSL Plugin"
	author			"Spike"
	desc			"Provides TLS and DTLS support, instead of using Microsoft's probably-outdated libraries."
	desc			"Required for fully functional DTLS support on windows."
	desc			"Connecting to QEx servers requires additional setup."
TEST()dnl
}
{
	package "fteplug_hl2"
	WINPLUG(hl2)
	LINPLUG(hl2)
	title			"HalfLife2 Formats Plugin"
	desc			"Provides support for HalfLife2 bsp and texture formats."
	desc			"Some related games may work, but this is not guarenteed."
	desc			"Requires mod support for full functionality."
TEST()dnl
}
HIDE(`
{
	package 		"fteplug_models"
	WINPLUG(models)
	LINPLUG(models)
	title			"Model exporter plugin"
	desc			""
TEST()dnl
}
{	
	package fteplug_ode
TEST()dnl
}
{
	package fteplug_cef
TEST()dnl
}
{
	package		"fteplug_ffmpeg"
	{
		arch	"win_x86"
		file	"fteplug_ffmpeg_x86."#REVISION#".dll"
		url		"win32/fteplug_ffmpeg_x86.dll"
	}
	{
		arch	"win_x64"
		file	"fteplug_ffmpeg_x64."#REVISION#".dll"
		url		"win64/fteplug_ffmpeg_x64.dll"
	}
//	{
//		arch 	"linux_x64"
//		file	"fteplug_ffmpeg_amd64."#REVISION#".so"
//		url		"linux_amd64/fteplug_ffmpeg_amd64.so"
//	}
	ver			REVISION
	category	"Plugins"
	title		"FFmpeg Plugin"
	file		"fteplug_ffmpeg"
	gamedir		""
TEST()dnl
}
{
	package		"libffmpeg"
	{
		arch	"win_x86"
		url		"win32/ffmpeg-4.0-x86.zip"
	}
	{
		arch	"win_x64"
		url		"win64/ffmpeg-4.0-x64.zip"
	}
//	{
//		arch	"linux_x64"
//		url		"linux_amd64/ffmpeg-4.0-fteplug_ezhud_amd64.so"
//	}
	ver			"4.0"
	category	"Plugins"
	title		"FFmpeg Library"
	extract		"zip"
	gamedir		""
}
')dnl
GAME(quake2,
`{
	package			"q2game_baseq2"
	{
		arch		"win_x86"
		FILE(q2gamex86_baseq2.dll)
		URL(win32/q2gamex86_baseq2.dll)
	}
	{
		arch		"win_x64"
		FILE(q2gamex64_baseq2.dll)
		URL(win64/q2gamex64_baseq2.dll)
	}
	{
		arch		"linux_amd64"
		FILE(q2gameamd64_baseq2.so)
		URL(linux_amd64/q2gameamd64_baseq2.so)
	}
	ver				"20190606"
	category		"Mods"
	title			"Gamecode: Base Game"
	license			"GPLv2"
	website			"https://github.com/yquake2/yquake2"
	desc			"Quake2 Gamecode (from yamagiq2). Required for single player or servers."
TEST()dnl
}')dnl
GAME(quake,
`{
	package			"fte_csaddon"
	category		"Plugins"
	title			"Ingame Map Editor"
	ver				REVISION
	gamedir			"fte"
	FILE(csaddon.dat)
	URL(csaddon/csaddon.dat)
	desc			"This is Spikes map editing user interface. It is only active while running singleplayer (or sv_cheats is enabled)."
	desc			"To activate, set the ca_show cvar to 1 (suggestion: ^abind c toggle ca_show^a)."
	license			"GPLv2, source on the fte svn"
	author			"Spike"
TEST()dnl
}')dnl
GAME(quake,
`{
	package			"fte_menusys"
	category		"AfterQuake"
	title			"Replacement Menus"
	ver				REVISION
	gamedir			"fte"
	FILE(menu.dat)
	URL(csaddon/menu.dat)
	desc			"This provides a more modern mouse-first menu system."
	license			"GPLv2, source on the fte svn"
	author			"Spike"
TEST()dnl
}')dnl

