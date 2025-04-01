/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __FILE_H__
#define __FILE_H__

#ifndef MAX_PRINT_MSG_SIZE
	#define	MAX_PRINT_MSG_SIZE	16 * 1024
#endif

/*
==============================================================

  File Streams.

==============================================================
*/

// stgatilov #5766: remember where search directory belongs to
// this is useful for determining whether file comes from TDM core or from FM
typedef enum {
	FDOM_UNKNOWN = 0,
	FDOM_CORE,
	FDOM_FM
} domainStatus_t;

// mode parm for Seek
typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

class idFileSystemLocal;


class idFile {
public:
	virtual					~idFile( void ) {};
							// Get the name of the file.
	virtual const char *	GetName( void );
							// Get the full file path.
	virtual const char *	GetFullPath( void );
							// Checks if the file is compressed (i.e. compressed file inside PK4)
	virtual bool			IsCompressed( void );
							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len );
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len );
							// Returns the length of the file.
	virtual int				Length( void );
							// Return a time value for reload operations.
	virtual ID_TIME_T		Timestamp( void );
							//stgatilov #5766: is it core TDM file or part of FM?
	virtual domainStatus_t	GetDomain() const;
							// Returns offset in file.
	virtual int				Tell( void );
							// Forces flush on files being writting to.
	virtual void			ForceFlush( void );
							// Causes any buffered data to be written to the file.
	virtual void			Flush( void );
							// Seek on a file.
	virtual int				Seek( long offset, fsOrigin_t origin );
							// Go back to the beginning of the file.
	virtual void			Rewind( void );
							// Like fprintf.
	virtual int				Printf( const char *fmt, ... ) id_attribute((format(printf,2,3)));
							// Like fprintf but with argument pointer
	virtual int				VPrintf( const char *fmt, va_list arg );
							// Write a string with high precision floating point numbers to the file.
	virtual int				WriteFloatString( const char *fmt, ... ) id_attribute((format(printf,2,3)));
	
	// Endian portable alternatives to Read(...)
	virtual int				ReadInt( int &value );
	virtual int				ReadUnsignedInt( unsigned int &value );
	virtual int				ReadShort( short &value );
	virtual int				ReadUnsignedShort( unsigned short &value );
	virtual int				ReadChar( char &value );
	virtual int				ReadUnsignedChar( unsigned char &value );
	virtual int				ReadFloat( float &value );
	virtual int				ReadBool( bool &value );
	virtual int				ReadString( idStr &string );
	virtual int				ReadVec2( idVec2 &vec );
	virtual int				ReadVec3( idVec3 &vec );
	virtual int				ReadVec4( idVec4 &vec );
	virtual int				ReadVec6( idVec6 &vec );
	virtual int				ReadMat3( idMat3 &mat );
	
	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value );
	virtual int				WriteUnsignedInt( const unsigned int value );
	virtual int				WriteShort( const short value );
	virtual int				WriteUnsignedShort( unsigned short value );
	virtual int				WriteChar( const char value );
	virtual int				WriteUnsignedChar( const unsigned char value );
	virtual int				WriteFloat( const float value );
	virtual int				WriteBool( const bool value );
	virtual int				WriteString( const char *string );
	virtual int				WriteVec2( const idVec2 &vec );
	virtual int				WriteVec3( const idVec3 &vec );
	virtual int				WriteVec4( const idVec4 &vec );
	virtual int				WriteVec6( const idVec6 &vec );
	virtual int				WriteMat3( const idMat3 &mat );
};


class idFile_Memory : public idFile {
	friend class			idFileSystemLocal;

public:
							idFile_Memory( void );	// file for writing without name
							idFile_Memory( const char *name );	// file for writing
							idFile_Memory( const char *name, const char *data, int length, bool owned = false );	// file for reading
	virtual					~idFile_Memory( void ) override;

	virtual const char *	GetName( void ) override { return name.c_str(); }
	virtual const char *	GetFullPath( void ) override { return name.c_str(); }
	virtual int				Read( void *buffer, int len ) override;
	virtual int				Write( const void *buffer, int len ) override;
	virtual int				Length( void ) override;
	virtual ID_TIME_T		Timestamp( void ) override;
	virtual int				Tell( void ) override;
	virtual void			ForceFlush( void ) override;
	virtual void			Flush( void ) override;
	virtual int				Seek( long offset, fsOrigin_t origin ) override;

							// returns const pointer to the memory buffer
	const char *			GetDataPtr( void ) const { return filePtr; }
							// set the file granularity
	void					SetGranularity( int g ) { assert( g > 0 ); granularity = g; }
	void					SetTimestamp( ID_TIME_T t ) { timestamp = t; }

private:
	idStr					name;			// name of the file
	int						mode;			// open mode
	int						fileSize;		// size of the file
	int						allocated;		// allocated size
	int						granularity;	// file granularity
	char *					filePtr;		// buffer holding the file data
	bool					owned;			// if the filePtr is owned and should be deleted
	int						timestamp;		// custom timestamp (if set)
	char *					curPtr;			// current read/write pointer
};


class idFile_BitMsg : public idFile {
	friend class			idFileSystemLocal;

public:
							idFile_BitMsg( idBitMsg &msg );
							idFile_BitMsg( const idBitMsg &msg );
	virtual					~idFile_BitMsg( void ) override;

	virtual const char *	GetName( void ) override { return name.c_str(); }
	virtual const char *	GetFullPath( void ) override { return name.c_str(); }
	virtual int				Read( void *buffer, int len ) override;
	virtual int				Write( const void *buffer, int len ) override;
	virtual int				Length( void ) override;
	virtual ID_TIME_T		Timestamp( void ) override;
	virtual int				Tell( void ) override;
	virtual void			ForceFlush( void ) override;
	virtual void			Flush( void ) override;
	virtual int				Seek( long offset, fsOrigin_t origin ) override;

private:
	idStr					name;			// name of the file
	int						mode;			// open mode
	idBitMsg *				msg;
};


class idFile_Permanent : public idFile {
	friend class			idFileSystemLocal;

public:
							idFile_Permanent( void );
	virtual					~idFile_Permanent( void ) override;

	virtual const char *	GetName( void ) override { return name.c_str(); }
	virtual const char *	GetFullPath( void ) override { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len ) override;
	virtual int				Write( const void *buffer, int len ) override;
	virtual int				Length( void ) override;
	virtual ID_TIME_T		Timestamp( void ) override;
	virtual domainStatus_t	GetDomain() const override;
	virtual int				Tell( void ) override;
	virtual void			ForceFlush( void ) override;
	virtual void			Flush( void ) override;
	virtual int				Seek( long offset, fsOrigin_t origin ) override;

	// returns file pointer
	FILE *					GetFilePtr( void ) { return o; }

private:
	idStr					name;			// relative path of the file - relative path
	idStr					fullPath;		// full file path - OS path
	int						mode;			// open mode
	int						fileSize;		// size of the file
	FILE *					o;				// file handle
	bool					handleSync;		// true if written data is immediately flushed
	domainStatus_t			domain;			// stgatilov #5766
};


class idFile_InZip : public idFile {
	friend class			idFileSystemLocal;

public:
							idFile_InZip( void );
	virtual					~idFile_InZip( void ) override;

	virtual const char *	GetName( void ) override { return name.c_str(); }
	virtual const char *	GetFullPath( void ) override { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len ) override;
	virtual int				Write( const void *buffer, int len ) override;
	virtual int				Length( void ) override;
	virtual ID_TIME_T		Timestamp( void ) override;
	virtual domainStatus_t	GetDomain() const override;
	virtual int				Tell( void ) override;
	virtual void			ForceFlush( void ) override;
	virtual void			Flush( void ) override;
	virtual int				Seek( long offset, fsOrigin_t origin ) override;
	virtual bool			IsCompressed( void ) override;

private:
	idStr					name;			// name of the file in the pak
	idStr					fullPath;		// full file path including pak file name
	bool compressed;		// whether the file is actually compressed
	uint64_t				zipFilePos;		// zip file info position in pak
	int						fileSize;		// size of the file
	void *					z;				// unzip info
	domainStatus_t			domain;			// stgatilov #5766
};

#endif /* !__FILE_H__ */
