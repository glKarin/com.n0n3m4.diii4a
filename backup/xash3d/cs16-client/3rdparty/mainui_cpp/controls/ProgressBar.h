/*
ProgressBar.h -- progress bar
Copyright (C) 2017 mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef CMENUPROGRESSBAR_H
#define CMENUPROGRESSBAR_H

#include "BaseItem.h"

class CMenuProgressBar : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuProgressBar();
	void Draw( void ) override;
	void LinkCvar( const char *cvName, float flMin, float flMax );
	void SetValue( float flValue );

private:
	float m_flMin, m_flMax, m_flValue;
	const char *m_szCvarName;
};

#endif // CMENUPROGRESSBAR_H
