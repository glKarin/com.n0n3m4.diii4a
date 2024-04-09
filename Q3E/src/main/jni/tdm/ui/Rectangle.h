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
#ifndef IDRECTANGLE_H_
#define IDRECTANGLE_H_
//
// simple rectangle
//
extern void RotateVector(idVec3 &v, idVec3 origin, float a, float c, float s);
class idRectangle {
public:
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;
	idRectangle() { x = y = w= h = 0.0; }
	idRectangle(float ix, float iy, float iw, float ih) { x = ix; y = iy; w = iw; h = ih; }
	float Bottom() const { return y + h; }
	float Right() const { return x + w; }
	void Offset (float x, float y) { 
		this->x += x;
		this->y += y;
	}
	bool Contains(float xt, float yt) {
		if (w == 0.0 && h == 0.0) {
			return false;
		}
		if (xt >= x && xt <= Right() && yt >= y && yt <= Bottom()) {
			return true;
		}
		return false;
	}
	void Empty() { x = y = w = h = 0.0; };

	void ClipAgainst(idRectangle r, bool sizeOnly) {
		if (!sizeOnly) {
			if (x < r.x) {
				x = r.x;
			}
			if (y < r.y) {
				y = r.y;
			}
		}
		if (x + w > r.x + r.w) {
			w = (r.x + r.w) - x;
		}
		if (y + h > r.y + r.h) {
			h = (r.y + r.h) - y;
		}
	}



	void Rotate(float a, idRectangle &out) {
		idVec3 p1, p2, p4;
		float c, s;
		idVec3 center;
		center.Set((x + w) / 2.0, (y + h) / 2.0, 0);
		p1.Set(x, y, 0);
		p2.Set(Right(), y, 0);
		p4.Set(x, Bottom(), 0);
		if (a) {
			s = sin( DEG2RAD( a ) );
			c = cos( DEG2RAD( a ) );
		}
		else {
			s = c = 0;
		}
		RotateVector(p1, center, a, c, s);
		RotateVector(p2, center, a, c, s);
		RotateVector(p4, center, a, c, s);
		out.x = p1.x;
		out.y = p1.y;
		out.w = (p2 - p1).Length();
		out.h = (p4 - p1).Length();
	}

	idRectangle & operator+=( const idRectangle &a );
	idRectangle & operator-=( const idRectangle &a );
	idRectangle & operator/=( const idRectangle &a );
	idRectangle & operator/=( const float a );
	idRectangle & operator*=( const float a );
	idRectangle & operator=( const idVec4 v );
	int operator==(const idRectangle &a) const;
	float &	operator[]( const int index );
	char * String( void ) const;
	const idVec4& ToVec4() const;

};

ID_INLINE const idVec4 &idRectangle::ToVec4() const {
	return *reinterpret_cast<const idVec4 *>(&x);
}


ID_INLINE idRectangle &idRectangle::operator+=( const idRectangle &a ) {
	x += a.x;
	y += a.y;
	w += a.w;
	h += a.h;

	return *this;
}

ID_INLINE idRectangle &idRectangle::operator/=( const idRectangle &a ) {
	x /= a.x;
	y /= a.y;
	w /= a.w;
	h /= a.h;

	return *this;
}

ID_INLINE idRectangle &idRectangle::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	w *= inva;
	h *= inva;

	return *this;
}

ID_INLINE idRectangle &idRectangle::operator-=( const idRectangle &a ) {
	x -= a.x;
	y -= a.y;
	w -= a.w;
	h -= a.h;

	return *this;
}

ID_INLINE idRectangle &idRectangle::operator*=( const float a ) {
	x *= a;
	y *= a;
	w *= a;
	h *= a;

	return *this;
}


ID_INLINE idRectangle &idRectangle::operator=( const idVec4 v ) {
	x = v.x;
	y = v.y;
	w = v.z;
	h = v.w;
	return *this;
}

ID_INLINE int idRectangle::operator==( const idRectangle &a ) const {
	return (x == a.x && y == a.y && w == a.w && a.h);
}

ID_INLINE float& idRectangle::operator[]( int index ) {
	return ( &x )[ index ];
}

class idRegion {
public:
	idRegion() { };

	void Clear() {
		rects.Clear();
	}

	bool Contains(float xt, float yt) {
		int c = rects.Num();
		for (int i = 0; i < c; i++) {
			if (rects[i].Contains(xt, yt)) {
				return true;
			}
		}
		return false;
	}

	void AddRect(float x, float y, float w, float h) {
		rects.Append(idRectangle(x, y, w, h));
	}

	int GetRectCount() {
		return rects.Num();
	}

	idRectangle *GetRect(int index) {
		if (index >= 0 && index < rects.Num()) {
			return &rects[index];
		}
		return NULL;
	}

protected:

	idList<idRectangle> rects;
};


#endif
