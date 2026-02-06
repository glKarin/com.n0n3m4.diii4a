/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __DECLPDA_H__
#define __DECLPDA_H__

#include "idlib/containers/StrList.h"
#include "framework/DeclManager.h"

/*
===============================================================================

	idDeclPDA

===============================================================================
*/


class idDeclEmail : public idDecl {
public:
							idDeclEmail() {}

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void ) const;
	virtual void			List( void ) const;

	const char *			GetFrom() const { return from; }
	const char *			GetBody() const { return text; }
	const char *			GetSubject() const { return subject; }
	const char *			GetDate() const { return date; }
	const char *			GetTo() const { return to; }
	const char *			GetImage() const { return image; }
	const char *			GetReply1() const { return reply1; }
	const char *			GetReply2() const { return reply2; }
	const char*				GetReplyFunc1() const { return replyfunc1; }
	const char*				GetReplyFunc2() const { return replyfunc2; }
	bool					IsCritical() const { return isCritical; }
	const char*				GetReadFunc() const { return readfunc; }
	bool					HasReplies() const { return !reply1.IsEmpty(); }

private:
	idStr					text;
	idStr					subject;
	idStr					date;
	idStr					to;
	idStr					from;
	idStr					image;
	idStr					reply1;
	idStr					reply2;
	idStr					replyfunc1;
	idStr					replyfunc2;
	bool					isCritical;
	idStr					readfunc; //BC what script function to call when email is read.
};


class idDeclVideo : public idDecl {
public:
							idDeclVideo() {};

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void ) const;
	virtual void			List( void ) const;

	const char *			GetRoq() const { return video; }
	const char *			GetWave() const { return audio; }
	const char *			GetVideoName() const { return videoName; }
	const char *			GetInfo() const { return info; }
	const char *			GetPreview() const { return preview; }

private:
	idStr					preview;
	idStr					video;
	idStr					videoName;
	idStr					info;
	idStr					audio;
};


class idDeclAudio : public idDecl {
public:
							idDeclAudio() {};

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void ) const;
	virtual void			List( void ) const;

	const char *			GetAudioName() const { return audioName; }
	const char *			GetWave() const { return audio; }
	const char *			GetInfo() const { return info; }
	const char *			GetPreview() const { return preview; }

private:
	idStr					audio;
	idStr					audioName;
	idStr					info;
	idStr					preview;
};


class idDeclPDA : public idDecl {
public:
							idDeclPDA() { originalEmails = originalVideos = 0; };

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void ) const;
	virtual void			List( void ) const;

	virtual void			AddVideo( const char *name, bool unique = true ) const;
	virtual void			AddAudio( const char *name, bool unique = true ) const;
	virtual void			AddEmail( const char *name, bool unique = true ) const;
	virtual void			AddEmailReplied(const char* name) const;
	virtual bool			IsEmailReplied(const char* name) const;
	virtual bool			AddEmailRead(const char* name) const;
	virtual bool			IsEmailRead(const char* name) const;
	virtual bool			IsEmailExist(const char* name) const;
	virtual bool			HasCriticalUnrepliedEmails() const;
	virtual bool			HasUnreadEmails() const;
	virtual void			RemoveAddedEmailsAndVideos() const;

	virtual const int		GetNumVideos() const;
	virtual const int		GetNumAudios() const;
	virtual const int		GetNumEmails() const;
	virtual const idDeclVideo *GetVideoByIndex( int index ) const;
	virtual const idDeclAudio *GetAudioByIndex( int index ) const;
	virtual const idDeclEmail *GetEmailByIndex( int index ) const;

	virtual void			SetSecurity( const char *sec ) const;

	const char *			GetPdaName() const { return pdaName; }
	const char *			GetSecurity() const {return security; }
	const char *			GetFullName() const { return fullName; }
	const char *			GetIcon() const { return icon; }
	const char *			GetPost() const { return post; }
	const char *			GetID() const { return id; }
	const char *			GetTitle() const { return title; }

	const char* GetDescription() const { return description; } //BC
	const char* GetSenderModel() const { return sendermodel; }

	void					ResetEmails() const;

private:
	mutable idStrList		videos;
	mutable idStrList		audios;
	mutable idStrList		emails;
	mutable idStrList		emailsReplied;
	mutable idStrList		emailsRead;
	idStr					pdaName;
	idStr					fullName;
	idStr					icon;
	idStr					id;
	idStr					post;
	idStr					title;
	mutable idStr			security;
	mutable	int				originalEmails;
	mutable int				originalVideos;

	idStr					description; //BC
	idStr					sendermodel; //BC
};

#endif /* !__DECLPDA_H__ */
