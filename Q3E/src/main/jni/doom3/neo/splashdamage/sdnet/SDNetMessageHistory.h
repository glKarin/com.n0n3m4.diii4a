// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETMESSAGEHISTORY_H__ )
#define __SDNETMESSAGEHISTORY_H__

//===============================================================
//
//	sdNetMessageHistory
//
//===============================================================

struct messageHistoryEntry_t {
	idWStr			message;
	time_t			timeStamp;
};

class sdNetMessageHistory {
public:
	static const int						MAX_ENTRIES = 30;
	static const wchar_t* const				MESSAGE_STORE_VERSION;
	static const wchar_t* const				MESSAGE_STORE_HEADER;

											sdNetMessageHistory();

	virtual void							AddEntry( const wchar_t* msg );

	virtual bool							IsLoaded() const { return !filename.IsEmpty(); }
	virtual bool							Load( const char* fileName );
	virtual bool							Store();
	virtual void							Unload();

	virtual int								GetNumEntries() const { return entries.Num(); }
	virtual const messageHistoryEntry_t&	GetEntry( int index ) const { return entries[ index ]; }

private:
	idList< messageHistoryEntry_t >			entries;
	bool									loaded;
	idStr									filename;
};

#endif /* !__SDNETMESSAGEHISTORY_H__ */
