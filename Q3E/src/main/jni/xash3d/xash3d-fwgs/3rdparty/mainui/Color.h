/*
Color.cpp -- color class
Copyright (C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#pragma once
#ifndef COLOR_H
#define COLOR_H

class CColor
{
public:
	CColor( ) : rgba( 0 ), init( false ),
		a(3,rgba), r(2,rgba), g(1,rgba), b(0,rgba) { }
	CColor( unsigned int rgba_ ) : rgba( rgba_ ), init( false ),
		a(3,rgba), r(2,rgba), g(1,rgba), b(0,rgba) { }

	inline unsigned int operator =( unsigned int color )
	{
		Set( color );
		return color;
	}

	inline CColor& operator =( CColor& c )
	{
		Set( c.rgba );
		return c;
	}

	inline operator unsigned int() { return rgba; }

	inline void Set( unsigned int color )
	{
		rgba = color;
		init = true;
	}

	inline void SetDefault( unsigned int color )
	{
		if( !IsOk() ) Set( color );
	}

	// get rid of this someday
	inline bool IsOk() { return init; }

	unsigned int rgba;
private:
	class ColorWrap
	{
	private:
		const byte _byte;
		unsigned int & _rgba;

		ColorWrap(int bytenum, unsigned int &rgba): _byte(bytenum), _rgba(rgba) {}
	public:
		inline unsigned int operator =(unsigned int value) const
		{
			value &= 0xff;
			_rgba = (_rgba & ~(0xff << (_byte*8)) ) | ( value << (_byte*8) );
			return value;
		}
		inline operator unsigned int() const
		{
			return (_rgba >> _byte * 8) & 0xff;
		}
		friend class CColor;
	};
	bool init;
public:
		const ColorWrap r,g,b,a;
};

#endif // COLOR_H
