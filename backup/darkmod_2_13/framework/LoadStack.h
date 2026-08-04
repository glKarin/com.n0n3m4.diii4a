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
#pragma once

class idDecl;
class idImageAsset;
class idSoundSample;
class idRenderModel;
class idEntity;
class idMapEntity;
class idWindow;

extern idCVar decl_stack;

//stgatilov: represents the sequence of nested object loads
//used for better reports of missing assets
class LoadStack {
public:
	enum Type {
		tNone = 0,
		tDecl,
		tImage,
		tSoundSample,
		tModel,
		tEntity,
		tMapEntity,
		tWindow,
	};
	struct Level {
		Type type;
		union {
			void *ptr;
			idDecl *decl;
			idImageAsset *image;
			idSoundSample *soundSample;
			idRenderModel *model;
			idEntity *entity;
			idMapEntity *mapEntity;
			idWindow *window;
		};
		const idPoolStr *values[2];

		ID_FORCE_INLINE Level() { type = tNone; ptr = nullptr; values[0] = values[1] = nullptr; }
		ID_FORCE_INLINE bool operator== (const Level &other) const { return ptr == other.ptr; }
		void Print(int indent) const;
		void SaveStrings();
		void FreeStrings();
		void Clear();
	};
	//max chain: entity -> skin -> material -> image ?
	static const int MAX_LEVELS = 8;

	~LoadStack();
	LoadStack() = default;
	LoadStack(const LoadStack &src);
	LoadStack& operator= (const LoadStack &) = delete;	//TODO

	void Clear();
	template<class T> static Level LevelOf(T *ptr);
	void Rollback(void *ptr);
	void Append(const Level &lvl);
	void PrintStack(int indent, const Level &addLastLevel = Level()) const;

	static void ShowMemoryUsage_f( const idCmdArgs &args );
	static void ListStrings_f( const idCmdArgs &args );

private:
	Level levels[MAX_LEVELS];

	//all strings are stored in this pool
	static idStrPool pool;
};
