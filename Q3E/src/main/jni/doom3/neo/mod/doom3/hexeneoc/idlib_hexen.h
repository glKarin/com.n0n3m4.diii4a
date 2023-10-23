#ifndef _KARIN_IDLIBHEXEN_H
#define _KARIN_IDLIBHEXEN_H
#ifdef _HEXENEOC

#define BUTTON_ATTACK2 BUTTON_5

// Zeroth
const int IMPULSE_41			= 41;			// hec Inventory Scroll Right
const int IMPULSE_42			= 42;			// hec Inventory Scroll Left
const int IMPULSE_43			= 43;			// hec use selected item
const int IMPULSE_44			= 44;			// hec drop selected item
const int IMPULSE_45			= 45;			// toggle automap

namespace hxBounds
{
/*
============
Zeroth
idBounds::GetMaxs
============
*/
ID_INLINE idVec3 GetMaxs( const idBounds &Self )
{
    return idVec3( Self.b[1] );
}

/*
============
Zeroth
idBounds::GetMins
============
*/
ID_INLINE idVec3 GetMins( const idBounds &Self )
{
    return idVec3( Self.b[0] );
}
}

namespace hxList
{
/*
================
HEXEN
idList<type>::Shuffle
================
*/
template< class type >
ID_INLINE void Shuffle( idList<type> &Self )
{
    if ( !Self.list )
    {
        return;
    }

    int i, rnd;
    type tmp;

    for ( i = 0; i < Self.num; i++ )
    {
        rnd = rand() % Self.num;
        tmp = Self.list[rnd];
        Self.list[rnd] = Self.list[i];
        Self.list[i] = tmp;
    }
}
}

namespace hxVec3
{
/*
=============
Zeroth
toAngle
=============
*/
ID_INLINE float toAngle( const idVec3 &Self, const idVec3 &B)
{
// return the angle in degrees between two idVec3s
    idVec3	Bn = B;
    Bn.Normalize();
    idVec3	An = Self;
    An.Normalize();
    return RAD2DEG( idMath::ACos( An * Bn ) );
}
}

#endif
#endif //_KARIN_IDLIBHEXEN_H
