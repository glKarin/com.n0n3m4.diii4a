// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __TEXT_UTILITIES_H__
#define __TEXT_UTILITIES_H__

/*
============
sdTextUtilities
============
*/
class sdTextUtilities {
public:
	int					Write	( idFile* file, const char* string, bool indent = true );
	void				Indent	( idFile* file );
	void				Unindent( idFile* file );

	void				CloseFile( idFile* file );

private:
	idList< idFile* > fileList;
	idList< int > indentList;
};


typedef sdSingleton< sdTextUtilities > sdTextUtil;

#endif // !__TEXT_UTILITIES_H__
