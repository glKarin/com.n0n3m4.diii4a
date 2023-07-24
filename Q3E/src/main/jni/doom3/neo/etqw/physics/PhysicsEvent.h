// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __PHYSICS_EVENT_H__
#define __PHYSICS_EVENT_H__

class sdPhysicsEvent {
public:
	typedef idLinkList< sdPhysicsEvent > nodeType_t;

								sdPhysicsEvent( nodeType_t& list );
	virtual						~sdPhysicsEvent( void ) { ; }

	int							GetCreationTime( void ) const { return _creationTime; }

	const nodeType_t&			GetNode( void ) const { return _node; }

	virtual void				Apply( void ) const = 0;

private:
	int							_creationTime;
	nodeType_t					_node;
};

class sdPhysicsEvent_RadiusPush : public sdPhysicsEvent {
public:
								sdPhysicsEvent_RadiusPush( nodeType_t& list, const idVec3 &origin, float radius, const sdDeclDamage* damageDecl, float push, const idEntity *inflictor, const idEntity *ignore, int flags );

	void						Apply( void ) const;

private:
	idVec3						_origin;
	float						_radius;
	float						_push;
	idEntityPtr< idEntity >		_inflictor;
	idEntityPtr< idEntity >		_ignore;
	int							_flags;
	const sdDeclDamage*			_damageDecl;
};

#endif // __PHYSICS_EVENT_H__
