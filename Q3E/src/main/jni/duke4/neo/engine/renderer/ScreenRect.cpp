// ScreenRect.cpp
//

#include "RenderSystem_local.h"

/*
======================
idScreenRect::Clear
======================
*/
void idScreenRect::Clear() {
	x1 = y1 = 32000;
	x2 = y2 = -32000;
	zmin = 0.0f; zmax = 1.0f;
}

/*
======================
idScreenRect::AddPoint
======================
*/
void idScreenRect::AddPoint(float x, float y) {
	int	ix = idMath::FtoiFast(x);
	int iy = idMath::FtoiFast(y);

	if (ix < x1) {
		x1 = ix;
	}
	if (ix > x2) {
		x2 = ix;
	}
	if (iy < y1) {
		y1 = iy;
	}
	if (iy > y2) {
		y2 = iy;
	}
}

/*
======================
idScreenRect::Expand
======================
*/
void idScreenRect::Expand() {
	x1--;
	y1--;
	x2++;
	y2++;
}

/*
======================
idScreenRect::Intersect
======================
*/
void idScreenRect::Intersect(const idScreenRect& rect) {
	if (rect.x1 > x1) {
		x1 = rect.x1;
	}
	if (rect.x2 < x2) {
		x2 = rect.x2;
	}
	if (rect.y1 > y1) {
		y1 = rect.y1;
	}
	if (rect.y2 < y2) {
		y2 = rect.y2;
	}
}

/*
======================
idScreenRect::Union
======================
*/
void idScreenRect::Union(const idScreenRect& rect) {
	if (rect.x1 < x1) {
		x1 = rect.x1;
	}
	if (rect.x2 > x2) {
		x2 = rect.x2;
	}
	if (rect.y1 < y1) {
		y1 = rect.y1;
	}
	if (rect.y2 > y2) {
		y2 = rect.y2;
	}
}

/*
======================
idScreenRect::Equals
======================
*/
bool idScreenRect::Equals(const idScreenRect& rect) const {
	return (x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2);
}

/*
======================
idScreenRect::IsEmpty
======================
*/
bool idScreenRect::IsEmpty() const {
	return (x1 > x2 || y1 > y2);
}