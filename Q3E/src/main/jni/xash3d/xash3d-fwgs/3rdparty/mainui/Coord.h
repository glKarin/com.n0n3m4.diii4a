/*
Coord.h -- simple coordinate and size management
Copyright (C) 2017 a1batross


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#pragma once
#ifndef COORD_H
#define COORD_H

struct Point
{
	Point() { x = y = 0; }
	Point( int x, int y ) : x(x), y(y) {}

	int x, y;
	Point Scale();
	friend Point operator +( Point &a, Point &b ) { return Point( a.x + b.x, a.y + b.y ); }
	friend Point operator -( Point &a, Point &b ) { return Point( a.x - b.x, a.y - b.y ); }

	Point& operator+=( Point &a )
	{
		x += a.x;
		y += a.y;
		return *this;
	}

	Point& operator-=( Point &a )
	{
		x -= a.x;
		y -= a.y;
		return *this;
	}

	Point operator *( float scale ) { return Point( x * scale, y * scale );	}
	Point operator /( float scale ) { return Point( x / scale, y / scale );	}
};

struct Size
{
	Size() { w = h = 0; }
	Size( int w, int h ) : w(w), h(h) {}

	int w, h;
	Size Scale();
};

#endif // COORD_H
