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
#ifndef __LOD_COMPONENT_H__
#define __LOD_COMPONENT_H__


// stgatilov: information about LOD properties of an entity
class LodComponent {
public:
	LodComponent();

	void					Save( idSaveGame &savefile ) const;
	void					Restore( idRestoreGame &savefile );

	/**
	* Parse the LOD spawnargs and returns 0 if the entity has no LOD (or hide_distance),
	* otherwise returns a handle registered with ModelGenerator::RegisterLODData();
	*/
	lod_handle				ParseLODSpawnargs( idEntity* entity, const idDict* dict, const float fRandom );

	/**
	* Tels: Stop LOD changes permanently. If doTeam is true, also disables it on teammembers.
	*/
	static void				StopLOD( idEntity *ent, bool doTeam);

	/**
	* Tels: Stop LOD changes temporarily. If doTeam is true, also disables it on teammembers.
	*/
	static void				DisableLOD( idEntity *ent, bool doTeam );
	static void				EnableLOD( idEntity *ent, bool doTeam );

	// Tels: If LOD is enabled on this entity, compute new LOD level and new alpha value.
	// We pass in a pointer to the data (so the LODE can use shared data) as well as the distance,
	// so the lode can pre-compute the distance.
	float					ThinkAboutLOD( const lod_data_t* lod_data, const float deltaSq );

	// Tels: If LOD is enabled on this entity, call ThinkAboutLOD, computing new LOD level and new
	// alpha value, then do the right things like Hide/Show, SetAlpha, switch models/skin etc.
	// We pass in a pointer to the data (so LOD can use shared data) as well as the distance,
	// so the distance can be pre-computed.
	// SteveL #3770: Params removed. They are now determined in SwitchLOD itself to avoid code repetition
	// as multiple classes now use LOD.
	bool					SwitchLOD();

	// Tels: Returns the distance that should be considered for LOD and hiding, depending on:
	//	* the distance of the origin to the given player origin
	//	* the lod-bias set in the menu
	//	* some minimum and maximum distances based on entity size/importance
	// The returned value is the actual distance squared, and rounded down to an integer.
	float					GetLODDistance( const lod_data_t *m_LOD, const idVec3 &playerOrigin, const idVec3 &entOrigin, const idVec3 &entSize, const float lod_bias ) const;

	/**
	* Tels: Hide the entity if tdm_lod_bias is outside MinLODBias .. MaxLODBias.
	* stgatilov: I guess "updateAfterLodBiasChange" would be a better name...
	*/
	void					Event_HideByLODBias( void );


	//stgatilov: hacky accessors for SEED + CStaticMulti
	//don't use them anywhere else, especially the setter!
	int						GetLodLevel() const { return m_LODLevel; }
	void					SetLodLevel( int level ) { m_LODLevel = level; }

private:
	static const int NOLOD = INT_MAX;		// used to disable LOD temp.
											// SteveL #3770: Moved from a #define in Entity.cpp as many classes now need to use it

	// stgatilov: if true, then this is empty space within LodSystem
	// it must be skipped for all purposes
	bool					m_dead;

	// stgatilov: owner entity which is controlled by this component
	idEntity *				m_entity;

	/**
	* Tels: Contains handle to (sharable, constant) LOD data if != 0.
	*/
	lod_handle				m_LODHandle;

	/**
	* Tels: Info for LOD, per entity. used if m_LODHandle != 0:
	* Timestamp for !next! LOD think. If equals NOLOD,
	* then LOD thinking is temporarily disabled.
	**/
	int						m_DistCheckTimeStamp;

	/**
	* Current LOD (0 - normal, 1,2,3,4,5 LOD, 6 hidden). For entities
	* hidden by MinLODBias/MaxLODBias, is -1 to mark it as hidden.
	**/
	int						m_LODLevel;

	/* Store the current model and skin to avoid flicker by not
	*  switching from one model/skin to the same model/skin when
	*  changing the LOD.
	*/
	int						m_ModelLODCur;
	int						m_SkinLODCur;

	// stgatilov: store currently applied offset_lod so that we can reverse it
	// even if m_LODHandle is removed due to hot-reload map editing.
	idVec3					m_OffsetLODCur;

	/* Each entity is hidden (and stops thinking) when tdm_lod_bias
	*  is between m_MinLODBias and m_MaxLODBias. Thus entities can
	*  be removed for slower machines without having the full LOD
	*  system active for this entity.
	*/
	float					m_MinLODBias;
	float					m_MaxLODBias;

	/* If "lod_hidden_skin" is set, use this to switch the skin instead
	*  of hiding the entity. mVisibleSkin stores the skin when the entity
	*  was visible, so we can restore it when the menu setting changes.
	*/
	idStr					m_HiddenSkin;
	idStr					m_VisibleSkin;

	friend class LodSystem;
};


// stgatilov: container for all LOD compoments
class LodSystem {
public:

	void Clear();

	void Save( idSaveGame &savefile ) const;
	void Restore( idRestoreGame &savefile );

	void AddToEnd(const LodComponent &component);
	void Remove(int index);

	inline LodComponent& Get(int index) { return m_components[index]; }
	inline const LodComponent& Get(int index) const { return m_components[index]; }

	int FindEntity(idEntity *entity) const;

	void ThinkAllLod();

	void UpdateAfterLodBiasChanged();

private:
	// note: elements with m_dead = true must be skipped
	// note: idEntity::lodIdx contains index within this list
	idList<LodComponent> m_components;
};

#endif
