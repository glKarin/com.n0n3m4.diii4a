/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include <tier1/UtlVector.h>

class CHintMessage
{
public:
	CHintMessage(const char *hintString, bool isHint, CUtlVector<const char *> *args, float duration);
	~CHintMessage(void);

public:
	float GetDuration(void) const { return m_duration; }
	void Send(CBaseEntity *client);
	bool IsEquivalent(const char *hintString, CUtlVector<const char *> *args) const;

private:
	const char *m_hintString;
	CUtlVector<char *> m_args;
	float m_duration;
	bool m_isHint;
};

class CHintMessageQueue
{
public:
	void Reset(void);
	void Update(CBaseEntity *player);
	bool AddMessage(const char *message, float duration, bool isHint, CUtlVector<const char *> *args);
	bool IsEmpty(void) { return m_messages.Count() == 0; }

private:
	float m_tmMessageEnd;
	CUtlVector<CHintMessage *> m_messages;
};
