// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __FILE_INZIP_H__
#define __FILE_INZIP_H__

class idFile_InZip : public idFile {
	friend class			idFileSystemLocal;

public:
							idFile_InZip( void );
	virtual					~idFile_InZip( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void ) const;
	virtual unsigned int	Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	idStr					name;			// name of the file in the pak
	idStr					fullPath;		// full file path including pak file name
	int						zipFilePos;		// zip file info position in pak
	int						fileSize;		// size of the file
	void *					z;				// unzip info
};

#endif /* !__FILE_INZIP_H__ */
