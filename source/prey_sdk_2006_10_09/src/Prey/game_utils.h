
#ifndef __GAME_UTILS_H__
#define __GAME_UTILS_H__

//Helper defines to allow line commenting with certain defines.
#define COMMENT SLASH(/)
#define SLASH(s) /##s

//////////////////////
// hhMatterPartner
//////////////////////
template< class PartnerType >
class hhMatterPartner {
public:
	const PartnerType&	GetPartner( const idEntity* ent, const idMaterial* material ) const;
	const PartnerType&	GetPartner( surfTypes_t type ) const;

protected:
	void				AddPartner( PartnerType partner, surfTypes_t type );

protected:
	// HUMANHEAD mdl:  This doesn't need to be saved because it's generated automatically by the constructor.  I think.
	idList< PartnerType > matterPartnerList;
};

template< class PartnerType >
const PartnerType& hhMatterPartner<PartnerType>::GetPartner( const idEntity* ent, const idMaterial* material ) const {
	return GetPartner( gameLocal.GetMatterType(ent, material) );
}

template< class PartnerType >
const PartnerType& hhMatterPartner<PartnerType>::GetPartner( surfTypes_t type ) const {
	return matterPartnerList[ (int)type ];
}

template< class PartnerType >
void hhMatterPartner<PartnerType>::AddPartner( PartnerType partner, surfTypes_t type ) {
	matterPartnerList.Insert( partner, (int)type );
}

//////////////////////
// hhMatterEventDefPartner
//////////////////////
class hhMatterEventDefPartner : public hhMatterPartner<const idEventDef*> {
public:
						hhMatterEventDefPartner( const char* eventNamePrefix );
};


//////////////////////
// hhUtils
//////////////////////
class hhUtils {
public:
	static int				ContentsOfBounds(const idBounds &localBounds, const idVec3 &location, const idMat3 &axis, idEntity *pass);
	static bool				EntityDefWillFit( const char *defName, const idVec3 &location, const idMat3 &axis, int contentMask, idEntity *pass );
	static void				SpawnDebrisMass( const char *debrisEntity, 
										 const idVec3 &origin,
										 const idVec3 *orientation = NULL,
										 const idVec3 *velocity = NULL,
										 const int power = -1,
   										 bool nonsolid = false,
										 float *duration = NULL,
										 idEntity *entForBounds = NULL ); //HUMANHEAD rww - added entForBounds
					
	static void				SpawnDebrisMass( const char *debrisEntity,
											 idEntity *sourceEntity,
											 const int power = -1 );
								  
	static hhProjectile *	LaunchProjectile(	idEntity *attacker,
												const char *projectile,
												const idMat3 &axis,
												const idVec3 &origin );

	static void				DebugAxis( const idVec3 &origin, const idMat3 &axis, int size, const int lifetime );
	static void				DebugCross( const idVec4 &color,
										const idVec3 &start,
										int size,
										const int lifetime = 0 );

	static idVec3			RandomVector();
	static float			RandomSign();
	static idVec3			RandomSpreadDir( const idMat3& baseAxis, const float spread );

	static idVec3			RandomPointInBounds(idBounds &bounds);
	static idVec3			RandomPointInShell( const float innerRadius, const float outerRadius );

	static void				SplitString( const idCmdArgs& input, idList<idStr>& pieces );
	static void				SplitString( const idStr& input, idList<idStr>& pieces, const char delimiter = ',' );

	static idVec3			GetLocalGravity( const idVec3& origin, const idBounds& bounds, const idVec3& defaultGravity = gameLocal.GetGravity() );

	static idVec3			ProjectOntoScreen(idVec3 &world, const renderView_t &renderView);

	static void				GetValues( idDict& source, const char *keyBase, idList<idStr> &values, bool numericOnly = false );

	static void				GetKeys( idDict& source, const char *keyBase, idList<idStr> &keys, bool numericOnly = false );

	static void				GetKeysAndValues( idDict& source, const char *keyBase, idList<idStr> &keys, idList<idStr> &values, bool numericOnly = false );

	static float			DetermineFinalFallVelocityMagnitude( const float totalFallDist, const float gravity );			

	static int 				ChannelName2Num( const char *name, const idDict *entityDef );

	static int				EntitiesTouchingClipmodel(idClipModel *clipModel, idEntity **entityList, int maxCount, int contents=MASK_SHOT_BOUNDINGBOX );

	static void				PassArgs( const idDict &source, idDict &dest, const char *passPrefix = "pass_" );

	static float			PointToAngle(float x, float y);

	static idMat3			SwapXZ( const idMat3& axis );

	static void				CreateFxDefinition( idStr &definition, const char* smokeName, const float duration );

	static float			CalculateSoundVolume( const float value, const float min, const float max );

	static float			CalculateScale( const float value, const float min, const float max );

	static idBounds			ScaleBounds( const idBounds& bounds, float scale );

	static idVec3			DetermineOppositePointOnBounds( const idVec3& start, const idVec3& dir, const idBounds& bounds );

	template< class Type >
	static void				Swap( Type& Val1, Type& Val2 );

	template< class listType > static ID_INLINE void RemoveContents( idList<listType>& list, bool clear ) {
		for( int index = list.Num() - 1; index >= 0; --index ) {
			SAFE_REMOVE( list[index] );
		}
		if( clear ) {
			list.Clear();
		} else {
			memset( list.Ptr(), 0, list.MemoryUsed() );
		}
	}
};

/*
================
hhUtils::SwapXZ

HUMANHEAD: aob
================
*/
ID_INLINE idMat3 hhUtils::SwapXZ( const idMat3& axis ) {
	return idMat3( -axis[2], axis[1], axis[0] );
}


/*
===============
hhMath::Swap
===============
*/
template< class Type >
ID_INLINE void hhUtils::Swap( Type& Val1, Type& Val2 ) {
	Type Temp;

	Temp = Val1;
	Val1 = Val2;
	Val2 = Temp;
}

template< class Type >
class hhCycleList {
	public:
						hhCycleList();
		virtual			~hhCycleList();

		void			Clear();
		void			Append( Type& obj );
		void			AddUnique( Type& obj );
		const Type&		Next();
		const Type&		Previous();
		const Type&		Random();
		const Type&		Get() const;
		int				Num() const;
		//rww - needed for networking sync
		int				GetCurrentIndex(void) { return currentIndex; }
		void			SetCurrentIndex(int newIndex) { currentIndex = newIndex; }

		const Type &	operator[]( int index ) const;
		Type &			operator[]( int index );
	protected:
		int				currentIndex;
		idList<Type>	list;
};

/*
===============
hhCycleList::hhCycleList
===============
*/
template< class Type >
ID_INLINE hhCycleList<Type>::hhCycleList() { 
	Clear(); 
}

/*
===============
hhCycleList::~hhCycleList
===============
*/
template< class Type >
ID_INLINE hhCycleList<Type>::~hhCycleList() {
	Clear();
}

/*
===============
hhCycleList::Clear
===============
*/
template< class Type >
ID_INLINE void hhCycleList<Type>::Clear() {
	list.Clear(); 
	currentIndex = 0;
}

/*
===============
hhCycleList::Append
===============
*/
template< class Type >
ID_INLINE void hhCycleList<Type>::Append( Type& obj ) {
	list.Append( obj );
}

/*
===============
hhCycleList::AddUnique
===============
*/
template< class Type >
ID_INLINE void hhCycleList<Type>::AddUnique( Type& obj ) {
	list.AddUnique( obj );
}

/*
===============
hhCycleList::Next
===============
*/
template< class Type >
ID_INLINE const Type& hhCycleList<Type>::Next() {
	assert( list.Num() );

	currentIndex = (currentIndex + 1) % list.Num();
	return Get();
}

/*
===============
hhCycleList::Previous
===============
*/
template< class Type >
ID_INLINE const Type& hhCycleList<Type>::Previous() {
	--currentIndex; 
	if( currentIndex < 0 ) {
		currentIndex = (list.Num() - 1);
	} 
	return Get();
}

/*
===============
hhCycleList::Random
===============
*/
template< class Type >
ID_INLINE const Type& hhCycleList<Type>::Random() {
	currentIndex = gameLocal.random.RandomInt( list.Num() );
	return Get();
}

/*
===============
hhCycleList::Get
===============
*/
template< class Type >
ID_INLINE const Type& hhCycleList<Type>::Get() const {
	return list[currentIndex];
}

/*
===============
hhCycleList::Num
===============
*/
template< class Type >
ID_INLINE int hhCycleList<Type>::Num() const {
	return list.Num();
}

/*
===============
hhCycleList::operator[]
===============
*/
template< class Type >
ID_INLINE const Type & hhCycleList<Type>::operator[]( int index ) const {
	return list[index];
}

/*
===============
hhCycleList::operator[]
===============
*/
template< class Type >
ID_INLINE Type & hhCycleList<Type>::operator[]( int index ) {
	return list[index];
}

#endif /* __GAME_UTILS_H__ */
