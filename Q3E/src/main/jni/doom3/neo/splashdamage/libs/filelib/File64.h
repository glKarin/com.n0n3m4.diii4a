// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __FILE64_H__
#define __FILE64_H__

#include "File.h"

#ifdef _WIN32
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <io.h>	
#else
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

/*
http://www.gnu.org/software/libc/manual/html_node/Low_002dLevel-I_002fO.html#Low_002dLevel-I_002fO
*/

class sdFile64 {
public:
#ifndef _WIN32
#ifdef MACOS_X
	typedef int fSize_t;
#else
	typedef off64_t fSize_t;
#endif
#else
	typedef __int64 fSize_t;
#endif

					sdFile64( void ) {
						fh = -1;
					}
	virtual			~sdFile64( void ) {
						Close();
					}


	bool			IsOpen( void ) {
						return ( fh != -1 );
					}

	const char *	GetName( void ) const {
						return fileName.c_str();
					}
	fSize_t			GetFileSize( void ) const {
						return fileSize;
					}

	bool			Open( idStr fileName, fsMode_t mode );

	void			Close( void ) {
						if ( fh != -1 ) {
#ifndef _WIN32
							close( fh );
#else
							_close( fh );
#endif
							fh = -1;
						}
					}

	void			Seek( fSize_t offset, fsOrigin_t origin ) {
						if ( fh != -1 ) {
#ifndef _WIN32
#ifdef MACOS_X
							lseek( fh, offset, origin );
#else
							lseek64( fh, offset, origin );
#endif
#else
							_lseeki64( fh, offset, origin );							
#endif
						}
					}

	void			Flush( void ) {
						if ( fh != -1 ) {
#ifndef _WIN32
							fsync( fh );
#else
							_commit( fh );
#endif
						}
					}

	int				Write( const void *buffer, int len ) {
						if ( fileMode != FS_WRITE ) {
							return 0;
						}
						if ( fh != -1 ) {
#ifndef _WIN32
							return write( fh, buffer, len );
#else
							return _write( fh, buffer, len );
#endif
						}
						return 0;
					}

	int				Read( void *buffer, int len ) {
						if ( fileMode != FS_READ ) {
							return 0;
						}
						if ( fh != -1 ) {
#ifndef _WIN32
							return read( fh, buffer, len );
#else
							return _read( fh, buffer, len );
#endif
						}
						return 0;
					}

	byte			ReadByte( void ) {
						byte	b;

						if ( !IsOpen() ) {
							return 0;
						}

#ifndef _WIN32
						read( fh, &b, 1 );
#else
						_read( fh, &b, 1 );
#endif
						return b;
					}

	short			ReadShort( void ) {
						byte	b[2];

						if ( !IsOpen() ) {
							return 0;
						}

#ifndef _WIN32
						read( fh, &b, 2 );
#else
						_read( fh, &b, 2 );
#endif
						return b[0] + ( b[1] << 8 );
					}

	unsigned		TimeStamp( void ) {
						if ( !IsOpen() ) {
							return 0;
						}
#ifndef _WIN32
						struct stat st;
						fstat( fh, &st);
						return st.st_mtime;
#else
						struct _stat st;
						_fstat( fh, &st );
						return static_cast< unsigned >( st.st_mtime );
#endif
					}

private:
	idStr			fileName;
	fSize_t			fileSize;
	int				fh;

	fsMode_t		fileMode;
};

#endif /* !__FILE64_H__ */
