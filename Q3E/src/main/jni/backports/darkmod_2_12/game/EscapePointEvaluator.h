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
#ifndef ESCAPE_POINT_EVALUATOR__H
#define ESCAPE_POINT_EVALUATOR__H

#include "precompiled.h"

// Forward Declaration
class EscapePoint;
struct EscapeConditions;

/**
 * greebo: An EscapePointEvaluator gets fed with one EscapePoint after the
 *         other (by any algorithm) and inspects each of them. The most
 *         suitable one is stored internally and can be retrieved at any time
 *         by using GetBestEscapePoint().
 */
class EscapePointEvaluator
{
protected:
	const EscapeConditions* _conditions;

	// This holds the ID of the best escape point so far
	int _bestId;

	// The area number the AI starts to flee in
	int _startAreaNum;

		// The best travel time so far
	int _bestTime;

	// This is either -1 (find farthest) or 1 (find nearest)
	int _distanceMultiplier;

	idVec3 _threatPosition;

private:
	// Silence compiler warning about assignment operators
	EscapePointEvaluator& operator=(const EscapePointEvaluator& other);

public:
	// Default Constructor
	EscapePointEvaluator(const EscapeConditions& conditions);

	/**
	 * greebo: Evaluate the given escape point.
	 *
	 * @returns: FALSE means that the evaluation can be stopped (prematurely),
	 *           no more EscapePoints need to be passed.
	 */
	virtual bool Evaluate(EscapePoint& escapePoint) = 0;

	/**
	 * greebo: Returns the ID of the best found escape point.
	 *
	 * @returns: an ID of -1 is returned if no point was found to be suitable.
	 */
	virtual int GetBestEscapePoint()
	{
		return _bestId;
	}

protected:
	/**
	 * Performs the distance check according to the escape conditions.
	 * If the given escapePoint is better, the _bestId is updated.
	 *
	 * @returns FALSE if the search is finished (DIST_DONT_CARE) or 
	 *          TRUE if the search can continue.
	 */
	bool	PerformDistanceCheck(EscapePoint& escapePoint);

	/** grayman #3548
	 * Performs a neighborhood check at the flee point, looking at
	 * the relationship of the fleeing AI with the AI in the neighborhood.
	 *
	 * @returns FALSE if the search is finished (no hostiles) or 
	 *          TRUE if the search can continue.
	 */
	bool	PerformRelationshipCheck(EscapePoint& escapePoint, int team);

	/** grayman #3847
	 * Checks the distance from the flee point to the threat location.
	 *
	 * @returns FALSE if the search is finished (far from threat) or 
	 *          TRUE if the search can continue.
	 */
	bool	PerformProximityToThreatCheck(EscapePoint& escapePoint, idVec3 _threatLocation);
};
typedef std::shared_ptr<EscapePointEvaluator> EscapePointEvaluatorPtr;

/**
 * ==== EVALUATOR IMPLEMENTATIONS === 
 */

/**
 * greebo: This visitor returns the escape point which is nearest or farthest away
 *         from the threatening entity. This is determined by the algorithm in the
 *         EscapeConditions structure.
 */
class AnyEscapePointFinder :
	public EscapePointEvaluator
{
private:
	// The team of the fleeing AI, which is evaluated against the
	// teams of the AI near the flee point.
	int _team;
	idVec3 _threatLocation; // grayman #3847
public:
	AnyEscapePointFinder(const EscapeConditions& conditions);

	virtual bool Evaluate(EscapePoint& escapePoint) override;
};

/**
 * greebo: This visitor tries to locate a guarded escape point.
 */
class GuardedEscapePointFinder :
	public EscapePointEvaluator
{
private:
	// The team of the fleeing AI, which is evaluated against the
	// teams of the AI near the flee point.
	int _team;
	idVec3 _threatLocation; // grayman #3847
public:
	GuardedEscapePointFinder(const EscapeConditions& conditions);

	virtual bool Evaluate(EscapePoint& escapePoint) override;
};

/**
 * greebo: This visitor tries to locate a friendly escape point.
 *         Whether an escape point is friendly or not is determined
 *         by the "team" spawnarg on the PathFlee entity and is checked
 *         using the RelationsManager.
 */
class FriendlyEscapePointFinder :
	public EscapePointEvaluator
{
private:
	// The team of the fleeing AI, which is evaluated against the
	// team of the escape point and the teams of the AI near the flee point.
	int _team;
	idVec3 _threatLocation; // grayman #3847
public:
	FriendlyEscapePointFinder(const EscapeConditions& conditions);

	virtual bool Evaluate(EscapePoint& escapePoint) override;
};

/**
 * greebo: This visitor tries to locate a friendly AND guarded escape point.
 *         Whether an escape point is friendly or not is determined
 *         by the "team" spawnarg on the PathFlee entity and is checked
 *         using the RelationsManager.
 */
class FriendlyGuardedEscapePointFinder :
	public EscapePointEvaluator
{
private:
	// The team of the fleeing AI, which is evaluated against the
	// team of the escape point and the teams of the AI near the flee point.
	int _team;
	idVec3 _threatLocation; // grayman #3847
public:
	FriendlyGuardedEscapePointFinder(const EscapeConditions& conditions);

	virtual bool Evaluate(EscapePoint& escapePoint) override;
};

#endif /* ESCAPE_POINT_EVALUATOR__H */
