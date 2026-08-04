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
#ifndef SR_STIMRESPONSECOLLECTION__H
#define SR_STIMRESPONSECOLLECTION__H

#include "Stim.h"
#include "Response.h"

/******************************************************************************
* The Stim/Response system consists of a collection class, which handles the
* actuall stimulations and responses. Each entity can have exactly one such 
* collection, and all stim/responses are added to this collection. All 
* primary interactions with stims and responses are done via this collection.
******************************************************************************/

/**
 * CStimResponseCollection is the collection to handle all interactions within
 * stimulations and their responses. For each stim of a given type only one
 * stim/response may exist in a given collection.
 * Stim/responses should always be identified by their type instead of their pointer.
 * Handling for the objects is done via the collection class.
 */
class CStimResponseCollection {
public:

	~CStimResponseCollection();

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

	/**
	 * AddStim/Response creates a new stim of the given type and returns the pointer to
	 * the new object. If the stim already existed, it is not created again but the 
	 * pointer still is returned to the existing object.
	 * The returned pointer may be used for configuring or activating the stim, but it
	 * may NEVER be used to delete the object, and it should not be passed around
	 * extensively, because it may become invalid.
	 */
// TODO: Add additional parameters to AddStim: Magnitude, Interleave duration, duration
	CStimPtr		AddStim(idEntity *Owner, int Type, float Radius = 0.0f, bool Removable = true, bool Default = false);
	CResponsePtr	AddResponse(idEntity *Owner, int Type, bool Removable = true, bool Default = false);

	/**
	 * RemoveStim will remove the stim of the given type and the object is destroyed.
	 * Any pointer that still exists will become invalid after that.
	 * The number of remaining stims is returned.
	 */
	int				RemoveStim(StimType type);
	int				RemoveResponse(StimType type);

	/**
	 * Returns true if the stim response collection has any stims or responses
	 **/
	bool			HasStim();
	bool			HasResponse();

	/**
	 * greebo: Tries to find the Stim/Response with the given ID.
	 * @returns: the pointer to the class, or NULL if the uniqueId couldn't be found.
	 */
	CStimResponsePtr	FindStimResponse(int uniqueId);

	// Returns the number of stims
	int				GetNumStims() { return m_Stims.Num(); }

	// Returns stim by index [0..GetNumStims())
	const CStimPtr&	GetStim(int index) { return m_Stims[index]; }

	// Returns the number of responses
	int				GetNumResponses() { return m_Responses.Num(); }

	// Returns stim by index [0..GetNumResponses())
	const CResponsePtr&	GetResponse(int index) { return m_Responses[index]; }	

	/** 
	 * greebo: Returns the stim with the given type, or NULL if nothing found.
	 */
	CStimPtr		GetStimByType(StimType type);

	/**
	 * greebo: Returns the response with the given type, or NULL if nothing found.
	 */
	CResponsePtr	GetResponseByType(StimType type);

	// Parses the given entity key values and constructs stims/responses using that information
	void			InitFromSpawnargs(const idDict& args, idEntity* owner);

private:

	bool			ParseSpawnArg(const idDict& args, idEntity* owner, const char sr_class, int index);

	/**
	 * If the stim contains information for a timed event, this function parses the string
	 * and creates the appropriate timer structure.
	 *
	 * The timer is configured by several strings on the entity:
	 *
 	 * Key: sr_timer_time
	 * Value: Time in the format: HOURS:MINUTES:SECONDS:MILLISECONDS
	 *
	 * Key: sr_timer_reload
	 * Value: N
	 *
	 * N = 0-N for the number of times it should be reloaded.
	 * A value of -1 means that it is infinitely reloaded (until disabled).
	 *
	 * Key: sr_timer_type
	 * Value: { RELOAD | SINGLESHOT }
	 *
	 * Key: sr_timer_waitforstart
	 * Value: { 0 | 1 } - Set true if timer should wait for StartTimer to start
	 * Otherwise starts on spawn.
	 */
	void			CreateTimer(const idDict& args, const CStimPtr& stim, int index);

	/**
	 * AddStim/Response with already configured objects. If the type already exists, the new object is not added 
	 * and the pointer to the existing object is returned, otherwise the added pointer is returned.
	 */
	CStimPtr		AddStim(const CStimPtr& stim);
	CResponsePtr	AddResponse(const CResponsePtr& response);

	/*
	* This static method is used to allocate, on the heap, a stim of a given type.
	* Some stim types create descended classes with virtual overrides of some stim methods.
	* It is important to always uses this instead of allocating a CStim object yourself so
	* that the correct descended class is created.
	*
	* @param p_owner A pointer to the entity which owns this stim
	*
	* @param type The enumerated stim type value
	*/
	static CStimPtr CreateStim(idEntity* p_Owner, StimType type);

	/*
	* This static method is used to allocate, on the heap, a response of a given type.
	* Some response types create descended classes with virtual overrides of some response methods.
	* It is important to always uses this instead of allocating a CResponse object yourself so
	* that the correct descended class is created.
	*
	* @param p_owner A pointer to the entity which owns this response
	*
	* @param type The enumerated stim type value for the response
	*/
	static CResponsePtr CreateResponse(idEntity* p_owner, StimType type);

private:
	idList<CStimPtr>		m_Stims;
	idList<CResponsePtr>	m_Responses;
};

#endif /* SR_STIMRESPONSECOLLECTION__H */
