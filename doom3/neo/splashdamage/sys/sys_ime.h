// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYS_IME__
#define __SYS_IME__

#include "sys_input.h"

/*
======================================================================

IME

======================================================================
*/

class sdIMEGeneric : public sdIME {
public:
	sdIMEGeneric();

	virtual bool			Init();
	virtual void			Shutdown();

	virtual bool			LangSupportsIME() const;

	virtual void			Enable( bool enable );
	virtual bool			IsEnabled() const;
	virtual void			FinalizeString( bool send = false );
	virtual int				GetCursorChars() const;

	virtual bool			IsReadingWindowActive() const;
	virtual bool			IsHorizontalReading() const;
	virtual bool			VerticalCandidateLine() const;

	virtual state_e			GetState() const;
	virtual const wchar_t*	GetIndicator() const;

	virtual bool			IsCandidateListActive() const;
	virtual const wchar_t*	GetCandidate( const unsigned int index ) const;
	virtual int				GetCandidateCount() const;
	virtual int				GetCandidateSelection() const;

	virtual const wchar_t*	GetCompositionString() const;
	virtual const byte*		GetCompositionStringAttributes() const;

	virtual const lang_e	GetLanguage() const;
	virtual const lang_e	GetPrimaryLanguage() const;

private:
	bool enable;
};

extern sdIME *globalIME;

#endif /* !__SYS_IME__ */
