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

#ifndef __WINVAR_H__
#define __WINVAR_H__

#include "Rectangle.h"

static const char *VAR_GUIPREFIX = "gui::";
static const int VAR_GUIPREFIX_LEN = static_cast<int>(strlen(VAR_GUIPREFIX));

class idWindow;
class idWinVar {
public:
	idWinVar();
	virtual ~idWinVar();

	virtual const char *GetTypeName() const = 0;

	void SetGuiInfo(idDict *gd, const char *_name);
	const char *GetName() const { 
		if (name) {
			if (guiDict && *name == '*') {
				return guiDict->GetString(&name[1]);
			}
			return name;
		}
		return ""; 
	}
	void SetName(const char *_name) { 
		delete []name; 
		name = NULL;
		if (_name) {
			name = new char[strlen(_name)+1]; 
			strcpy(name, _name); 
		}
	}

	idWinVar &operator=( const idWinVar &other ) {
		guiDict = other.guiDict;
		SetName(other.name);
		return *this;
	}

	idDict *GetDict() const { return guiDict; }
	bool NeedsUpdate() { return (guiDict != NULL); }

	virtual void Init(const char *_name, idWindow* win) = 0;
	virtual void Update() = 0;
	virtual const char *c_str() const = 0;
	virtual size_t Size() {	size_t sz = (name) ? strlen(name) : 0; return sz + sizeof(*this); }

	virtual void WriteToSaveGame( idFile *savefile ) = 0;
	virtual void ReadFromSaveGame( idFile *savefile ) = 0;

	virtual float x( void ) const = 0;

	bool Set(const char *val) { return _Set(val, false); }
	bool TestSet(const char *val) const { return const_cast<idWinVar*>(this)->_Set(val, true); }

	void SetEval(bool b) {
		eval = b;
	}
	bool GetEval() {
		return eval;
	}

protected:
	virtual bool _Set(const char *val, bool dryRun) = 0;

	idDict *guiDict;
	char *name;
	bool eval;
};

class idWinBool : public idWinVar {
public:
	idWinBool() : idWinVar() {}
	virtual ~idWinBool() override {}
	virtual const char *GetTypeName() const override { return "bool"; }
	virtual void Init(const char *_name, idWindow *win) override { idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetBool(GetName());
		}
	}
	int	operator==(	const bool &other ) { return (other == data); }
	bool &operator=(	const bool &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetBool(GetName(), data);
		}
		return data;
	}
	idWinBool &operator=( const idWinBool &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}

	operator bool() const { return data; }

	virtual bool _Set(const char *val, bool dryRun) override { 
		int parsedVal = 0;
		int numChars = 0;
		bool good = (idStr::IsNumeric(val) && sscanf(val, "%d%n", &parsedVal, &numChars) == 1 && !val[numChars]);
		// stgatilov #5869: other integers will work fine, but better warn
		good = good && (parsedVal == 0 || parsedVal == 1);
		if (!dryRun) {
			data = ( parsedVal != 0 );
			if (guiDict) {
				guiDict->SetBool(GetName(), data);
			}
		}
		return good;
	}

	virtual void Update() override {	
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetBool( s );
		}
	}

	virtual const char *c_str() const override { return va("%i", data); }

	// SaveGames
	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	virtual float x( void ) const override { return data ? 1.0f : 0.0f; };

protected:
	bool data = false;
};

class idWinStr : public idWinVar {
public:
	idWinStr() : idWinVar() {}
	virtual ~idWinStr() override {}
	virtual const char *GetTypeName() const override { return "str"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetString(GetName());
		} 
	}
	int	operator==(	const idStr &other ) const {
		return (other == data);
	}
	int	operator==(	const char *other ) const {
		return (data == other);
	}
	idStr &operator=(	const idStr &other ) {
		data = other;
		if (guiDict) {
			guiDict->Set(GetName(), data);
		}
		return data;
	}
	idWinStr &operator=( const idWinStr &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	operator const char *() const {
		return data.c_str();
	}
	operator const idStr &() const {
		return data;
	}
	int LengthWithoutColors() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		return data.LengthWithoutColors();
	}
	int Length() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		return data.Length();
	}
	void RemoveColors() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		data.RemoveColors();
	}
	virtual const char *c_str() const override {
		return data.c_str();
	}

	virtual bool _Set(const char *val, bool dryRun) override {
		if (!dryRun) {
			data = val;
			if ( guiDict ) {
				guiDict->Set(GetName(), data);
			}
		}
		return true;
	}

	virtual void Update() override {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetString( s );
		}
	}

	virtual size_t Size() override {
		size_t sz = idWinVar::Size();
		return sz +data.Allocated();
	}

	// SaveGames
	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );

		int len = data.Length();
		savefile->Write( &len, sizeof( len ) );
		if ( len > 0 ) {
			savefile->Write( data.c_str(), len );
		}
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );

		int len;
		savefile->Read( &len, sizeof( len ) );
		if ( len > 0 ) {
			data.Fill( ' ', len );
			savefile->Read( &data[0], len );
		}
	}

	// return wether string is emtpy
	virtual float x( void ) const override { return data[0] ? 1.0f : 0.0f; };

protected:
	idStr data;
};

class idWinInt : public idWinVar {
public:
	idWinInt() : idWinVar() {}
	virtual ~idWinInt() override {}
	virtual const char *GetTypeName() const override { return "int"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinVar::Init(_name,  win);
		if (guiDict) {
			data = guiDict->GetInt(GetName());
		} 
	}
	int &operator=(	const int &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetInt(GetName(), data);
		}
		return data;
	}
	idWinInt &operator=( const idWinInt &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	operator int () const {
		return data;
	}
	virtual bool _Set(const char *val, bool dryRun) override {
		int parsedVal = 0;
		int numChars = 0;
		bool good = (idStr::IsNumeric(val) && sscanf(val, "%d%n", &parsedVal, &numChars) == 1 && !val[numChars]);
		if (!dryRun) {
			data = parsedVal;
			if (guiDict) {
				guiDict->SetInt(GetName(), data);
			}
		}
		return good;
	}

	virtual void Update() override {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetInt( s );
		}
	}
	virtual const char *c_str() const override {
		return va("%i", data);
	}

	// SaveGames
	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	// no suitable conversion
	virtual float x( void ) const override { assert( false ); return 0.0f; };

protected:
	int data = 0;
};

class idWinFloat : public idWinVar {
public:
	idWinFloat() : idWinVar() {}
	virtual ~idWinFloat() override {}
	virtual const char *GetTypeName() const override { return "float"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetFloat(GetName());
		} 
	}
	idWinFloat &operator=( const idWinFloat &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	float &operator=(	const float &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetFloat(GetName(), data);
		}
		return data;
	}
	operator float() const {
		return data;
	}
	virtual bool _Set(const char *val, bool dryRun) override {
		float parsedVal = 0;
		int numChars = 0;
		bool good = (idStr::IsNumeric(val) && sscanf(val, "%f%n", &parsedVal, &numChars) == 1 && !val[numChars]);
		if (!dryRun) {
			data = parsedVal;
			if (guiDict) {
				guiDict->SetFloat(GetName(), data);
			}
		}
		return good;
	}
	virtual void Update() override {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetFloat( s );
		}
	}
	virtual const char *c_str() const override {
		return va("%f", data);
	}

	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	virtual float x( void ) const override { return data; };
protected:
	float data = 0.0f;
};

class idWinRectangle : public idWinVar {
public:
	idWinRectangle() : idWinVar() {}
	virtual ~idWinRectangle() override {}
	virtual const char *GetTypeName() const override { return "rect"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinVar::Init(_name, win);
		if (guiDict) {
			idVec4 v = guiDict->GetVec4(GetName());
			data.x = v.x;
			data.y = v.y;
			data.w = v.z;
			data.h = v.w;
		} 
	}
	
	int	operator==(	const idRectangle &other ) const {
		return (other == data);
	}

	idWinRectangle &operator=( const idWinRectangle &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	idRectangle &operator=(	const idVec4 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec4(GetName(), other);
		}
		return data;
	}

	idRectangle &operator=(	const idRectangle &other ) {
		data = other;
		if (guiDict) {
			idVec4 v = data.ToVec4();
			guiDict->SetVec4(GetName(), v);
		}
		return data;
	}
	
	operator const idRectangle&() const {
		return data;
	}

	virtual float x() const override {
		return data.x;
	}
	float y() const {
		return data.y;
	}
	float w() const {
		return data.w;
	}
	float h() const {
		return data.h;
	}
	float Right() const {
		return data.Right();
	}
	float Bottom() const {
		return data.Bottom();
	}
	idVec4 &ToVec4() {
		static idVec4 ret;
		ret = data.ToVec4();
		return ret;
	}
	virtual bool _Set(const char *val,bool dryRun) override {
		int ret;
		idRectangle v;
		if ( strchr ( val, ',' ) ) {
			ret = sscanf( val, "%f,%f,%f,%f", &v.x, &v.y, &v.w, &v.h );
		} else {
			ret = sscanf( val, "%f %f %f %f", &v.x, &v.y, &v.w, &v.h );
		}
		if (!dryRun) {
			data = v;
			if (guiDict) {
				idVec4 v = data.ToVec4();
				guiDict->SetVec4(GetName(), v);
			}
		}
		return (ret == 4);
	}
	virtual void Update() override {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			idVec4 v = guiDict->GetVec4( s );
			data.x = v.x;
			data.y = v.y;
			data.w = v.z;
			data.h = v.w;
		}
	}

	virtual const char *c_str() const override {
		return data.ToVec4().ToString();
	}

	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	idRectangle data;
};

class idWinVec2 : public idWinVar {
public:
	idWinVec2() : idWinVar() {}
	virtual ~idWinVec2() override {}
	virtual const char *GetTypeName() const override { return "vec2"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVec2(GetName());
		} 
	}
	int	operator==(	const idVec2 &other ) const {
		return (other == data);
	}
	idWinVec2 &operator=( const idWinVec2 &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	
	idVec2 &operator=(	const idVec2 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec2(GetName(), data);
		}
		return data;
	}
	virtual float x() const override {
		return data.x;
	}
	float y() const {
		return data.y;
	}
	virtual bool _Set(const char *val, bool dryRun) override {
		int ret;
		idVec2 v(0.0f, 0.0f);
		if ( strchr ( val, ',' ) ) {
			ret = sscanf( val, "%f,%f", &v.x, &v.y );
		} else {
			ret = sscanf( val, "%f %f", &v.x, &v.y);
		}
		if (!dryRun) {
			data = v;
			if (guiDict) {
				guiDict->SetVec2(GetName(), data);
			}
		}
		return (ret == 2);
	}
	operator const idVec2&() const {
		return data;
	}
	virtual void Update() override {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVec2( s );
		}
	}
	virtual const char *c_str() const override {
		return data.ToString();
	}
	void Zero() {
		data.Zero();
	}

	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	idVec2 data = idVec2(0.0f, 0.0f);
};

class idWinVec4 : public idWinVar {
public:
	idWinVec4() : idWinVar() {}
	virtual ~idWinVec4() override {}
	virtual const char *GetTypeName() const override { return "vec4"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVec4(GetName());
		} 
	}
	int	operator==(	const idVec4 &other ) const {
		return (other == data);
	}
	idWinVec4 &operator=( const idWinVec4 &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	idVec4 &operator=(	const idVec4 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec4(GetName(), data);
		}
		return data;
	}
	operator const idVec4&() const {
		return data;
	}

	virtual float x() const override {
		return data.x;
	}

	float y() const {
		return data.y;
	}

	float z() const {
		return data.z;
	}

	float w() const {
		return data.w;
	}
	virtual bool _Set(const char *val, bool dryRun) override {
		int ret;
		idVec4 v(0.0f, 0.0f, 0.0f, 0.0f);
		if ( strchr ( val, ',' ) ) {
			ret = sscanf( val, "%f,%f,%f,%f", &v.x, &v.y, &v.z, &v.w );
		} else {
			ret = sscanf( val, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
		}
		//stgatilov: "transition" expects vec4, but it often receives scalar for e.g. "rotation" property
		bool good = (ret == 4 || ret == 1);
		if (!dryRun) {
			data = v;
			if ( guiDict ) {
				guiDict->SetVec4( GetName(), data );
			}
		}
		return good;
	}
	virtual void Update() override {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVec4( s );
		}
	}
	virtual const char *c_str() const override {
		return data.ToString();
	}

	void Zero() {
		data.Zero();
		if ( guiDict ) {
			guiDict->SetVec4(GetName(), data);
		}
	}

	const idVec3 &ToVec3() const {
		return data.ToVec3();
	}

	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	idVec4 data = idVec4(0.0f, 0.0f, 0.0f, 0.0f);
};

class idWinVec3 : public idWinVar {
public:
	idWinVec3() : idWinVar() {}
	virtual ~idWinVec3() override {}
	virtual const char *GetTypeName() const override { return "vec3"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVector(GetName());
		} 
	}
	int	operator==(	const idVec3 &other ) const {
		return (other == data);
	}
	idWinVec3 &operator=( const idWinVec3 &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	idVec3 &operator=(	const idVec3 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVector(GetName(), data);
		}
		return data;
	}
	operator const idVec3&() const {
		return data;
	}

	virtual float x() const override {
		return data.x;
	}

	float y() const {
		return data.y;
	}

	float z() const {
		return data.z;
	}

	virtual bool _Set(const char *val, bool dryRun) override {
		int ret;
		idVec3 v(0.0f, 0.0f, 0.0f);
		if ( strchr ( val, ',' ) ) {
			ret = sscanf( val, "%f,%f,%f", &v.x, &v.y, &v.z );
		} else {
			ret = sscanf( val, "%f %f %f", &v.x, &v.y, &v.z );
		}
		if (!dryRun) {
			data = v;
			if (guiDict) {
				guiDict->SetVector(GetName(), data);
			}
		}
		return (ret == 3);
	}
	virtual void Update() override {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVector( s );
		}
	}
	virtual const char *c_str() const override {
		return data.ToString();
	}

	void Zero() {
		data.Zero();
		if (guiDict) {
			guiDict->SetVector(GetName(), data);
		}
	}

	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	idVec3 data = idVec3(0.0f, 0.0f, 0.0f);
};

class idWinBackground : public idWinStr {
public:
	idWinBackground() : idWinStr() {
		mat = NULL;
	}
	virtual ~idWinBackground() override {}
	virtual const char *GetTypeName() const override { return "background"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinStr::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetString(GetName());
		} 
	}
	int	operator==(	const idStr &other ) const {
		return (other == data);
	}
	int	operator==(	const char *other ) const {
		return (data == other);
	}
	idStr &operator=(	const idStr &other ) {
		data = other;
		if (guiDict) {
			guiDict->Set(GetName(), data);
		}
		if (mat) {
			if ( data == "" ) {
				(*mat) = NULL;
			} else {
				(*mat) = declManager->FindMaterial(data);
			}
		}
		return data;
	}
	idWinBackground &operator=( const idWinBackground &other ) {
		idWinVar::operator=(other);
		data = other.data;
		mat = other.mat;
		if (mat) {
			if ( data == "" ) {
				(*mat) = NULL;
			} else {
				(*mat) = declManager->FindMaterial(data);
			}
		}
		return *this;
	}
	operator const char *() const {
		return data.c_str();
	}
	operator const idStr &() const {
		return data;
	}
	int Length() {
		if (guiDict) {
			data = guiDict->GetString(GetName());
		}
		return data.Length();
	}
	virtual const char *c_str() const override {
		return data.c_str();
	}

	virtual bool _Set(const char *val, bool dryRun) override {
		const idMaterial *matVal = nullptr;
		if ( !val[0] ) {
			matVal = nullptr;
		} else {
			matVal = declManager->FindMaterial(val);
		}
		if (!dryRun) {
			data = val;
			if (guiDict) {
				guiDict->Set(GetName(), data);
			}
			if (mat) {
				(*mat) = matVal;
			}
		}
		return true;
	}

	virtual void Update() override {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetString( s );
			if (mat) {
				if ( data == "" ) {
					(*mat) = NULL;
				} else {
					(*mat) = declManager->FindMaterial(data);
				}
			}
		}
	}

	virtual size_t Size() override {
		size_t sz = idWinVar::Size();
		return sz + data.Allocated();
	}

	void SetMaterialPtr( const idMaterial **m ) {
		mat = m;
	}

	virtual void WriteToSaveGame( idFile *savefile ) override {
		savefile->Write( &eval, sizeof( eval ) );

		int len = data.Length();
		savefile->Write( &len, sizeof( len ) );
		if ( len > 0 ) {
			savefile->Write( data.c_str(), len );
		}
	}
	virtual void ReadFromSaveGame( idFile *savefile ) override {
		savefile->Read( &eval, sizeof( eval ) );

		int len;
		savefile->Read( &len, sizeof( len ) );
		if ( len > 0 ) {
			data.Fill( ' ', len );
			savefile->Read( &data[0], len );
		}
		if ( mat ) {
			if ( len > 0 ) {
				(*mat) = declManager->FindMaterial( data );
			} else {
				(*mat) = NULL;
			}
		}
	}

protected:
	idStr data;
	const idMaterial **mat;
};

/*
================
idMultiWinVar
multiplexes access to a list if idWinVar*
================
*/
class idMultiWinVar : public idList< idWinVar * > {
public:
	void Set( const char *val );
	void Update( void );
	void SetGuiInfo( idDict *dict );
};


//stgatilov: sometimes pointers are stored in idWinVar
//so I added this type specifically for such hacky cases
class idWinUIntPtr : public idWinVar {
public:
	idWinUIntPtr() : idWinVar() {}
	virtual ~idWinUIntPtr() override {}
	virtual const char *GetTypeName() const override { return "uintptr"; }
	virtual void Init(const char *_name, idWindow *win) override {
		idWinVar::Init(_name, win);
		assert(!guiDict);
	}
	size_t &operator=(const size_t &other) {
		data = other;
		assert(!guiDict);
		return data;
	}
	idWinUIntPtr &operator=(const idWinUIntPtr &other) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	operator size_t() const {
		return data;
	}
	virtual bool _Set(const char *val, bool dryRun) override {
		size_t parsedVal = 0;
		int numChars = 0;
		bool good = (idStr::IsNumeric(val) && sscanf(val, "%zu%n", &parsedVal, &numChars) == 1 && !val[numChars]);
		if (!dryRun) {
			data = parsedVal;
			assert(!guiDict);
		}
		return good;
	}

	virtual void Update() override {
		const char *s = GetName();
		assert(!guiDict);
	}
	virtual const char *c_str() const override {
		return va("%zu", data);
	}

	// SaveGames
	virtual void WriteToSaveGame(idFile *savefile) override {
		assert(false);
	}
	virtual void ReadFromSaveGame(idFile *savefile) override {
		assert(false);
	}

	// no suitable conversion
	virtual float x(void) const override { assert(false); return 0.0f; };

protected:
	size_t data = 0;
};



#endif /* !__WINVAR_H__ */

