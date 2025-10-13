#!/bin/bash

#This is a fairly lame shell script
#It could be written so much better...
#Anyway, what it does is ask the user lots of questions and then pipes some text to a file which can be used by the engine.
#the Makefile explicitally tests for config.h, and will pass the right precompiler to gcc so that this file is actually used.
#And so we don't break in the absence of this file.

if [ "$1" = "y" ]; then
	defaulttoyes=true
	echo "Checking installed libraries"
else
	echo "Answer the questions to generate a config.h file"
	echo "If you wish to remove the config, delete it and recompile, make will sort stuff out"
	echo "Many of these questions are irrelevent if you want to build only a dedicated server, for instance"
	echo "Some of them depend on others"
	echo "Usage of this script is not fully supported by the FTE team, and not every combination will likly work"
	echo "If using this script does produce compile errors, you can try reporting the issue preferably via irc"
fi

#clear out the config
echo "//warning: generated file." > config.h
echo "//Use 'make config' to alter this file" >> config.h
echo "//it is safe to delete this file if you want to use the default settings" >> config.h
echo "" >> config.h

query()
{
	if [ "$defaulttoyes" = "true" ]; then
		ans=y
	else
		read -n 1 -p "$1 " ans
		echo ""
	fi
	if [ "$ans" = "y" -o "$ans" = "Y" ]; then
		echo "#define $2" >> config.h
	else
		echo "//#define $2" >> config.h
	fi
}
querylibrary()
{
	if [ -f /usr/include/$3 ] ; then
		query "$1" "$2"
		return
	fi
	if [ -f /usr/local/include/$4 ] ; then
		query "$1" "$2"
		return
	fi
#they don't have it, force no.
	echo "$1 n"
	echo "//#define $2" >> config.h
}

querylibrary "Is libz (zlib) available on this system (zip support)?" AVAIL_ZLIB "zlib.h"
querylibrary "Is libvorbis (a free media library) available on this system (ogg support) ?" AVAIL_OGGVORBIS "vorbis/vorbisfile.h"
# querylibrary "Is libmad (an mp3 library) available on this system (mp3 support) ?" AVAIL_MP3
querylibrary "Is libpng available on this system (png support)?" AVAIL_PNGLIB "png.h"
querylibrary "Is libjpeg available on this system (jpeg support)?" AVAIL_JPEGLIB "jpeglib.h"
query "Do you want to enable the dds support ?" DDS
query "Do you want to enable on-server rankings?" SVRANKING
query "Do you want to enable stainmaps in software rendering?" SWSTAINS
query "Do you want to enable secondary/reverse views?" "SIDEVIEWS 4"
query "Do you want to enable quake2 sprites (sp2) ?" SP2MODELS
query "Do you want to enable quake2 models (md2) ?" MD2MODELS
query "Do you want to enable quake3arena models (md3) ?" MD3MODELS
query "Do you want to enable doom3 models (md5) ?" MD5MODELS
query "Do you want to enable 'zymotic' models (zym, used by nexuiz) ?" ZYMOTICMODELS
query "Do you want to enable basic halflife model support (mdl) ?" HALFLIFEMODELS
query "Do you want to enable network compression (huffman) ?" HUFFNETWORK
#query "Do you want to enable doom wad, map and sprite support (best to say no here) ?" DOOMWADS
query "Do you want to enable quake2 map support ?" Q2BSPS
query "Do you want to enable quake3 map support ?" Q3BSPS
query "Do you want to enable fte's heightmap support ?" TERRAIN
query "Do you want to enable the built in master server ?" SV_MASTER
query "Do you want to enable the FTE_NPCCHAT qc extention ?" SVCHAT
query "Do you want to enable the quake2 server ?" Q2SERVER
query "Do you want to enable the quake2 client ?" Q2CLIENT
query "Do you want to enable the quake3 server ?" Q3SERVER
query "Do you want to enable the quake3 client ?" Q3CLIENT
query "Do you want to enable netquake compatability ?" NQPROT
query "Do you want to allow connections via tcp (for suppose3rd party firewalls) ?" TCPCONNECT
query "Do you want to enable fish-eye views (only in software) ?" FISH
query "Do you want to enable the built in http/ftp server ?" WEBSERVER
query "Do you want to enable the built in http/ftp clients ?" WEBCLIENT
query "Do you want to enable the deluxemap generation routine ?" RUNTIMELIGHTING
#query "Do you want to enable the 'qterm' (this is a major security risk) ?" QTERM
query "Do you want to enable the server browser ?" CL_MASTER
query "Do you want to enable the serial-mouse support (used in splitscreen) ?" SERIALMOUSE
query "Do you want to enable the per-pixel lighting routines ?" PPL
query "Do you want to enable the text editor ?" TEXTEDITOR
query "Do you want to enable the plugin support ?" PLUGINS
query "Do you want to enable csqc support ?" CSQC_DAT
query "Do you want to enable menu.dat support (used by nexuiz) ?" MENU_DAT
query "Do you want to enable the built in irc client (note that there is also a plugin irc client, which cooler) ?" IRCCLIENT









echo "#define R_XFLIP" >> config.h
echo "#define IN_XFLIP" >> config.h

