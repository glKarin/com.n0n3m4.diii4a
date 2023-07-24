// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GUISURFACERENDERABLE_H__
#define __GUISURFACERENDERABLE_H__

struct guiPoint_t {
	float	x, y;				// 0.0 to 1.0 range if trace hit a gui, otherwise -1
	idVec3	worldPos;
};

struct guiSurface_t;

class sdGuiSurfaceRenderable {
public:
							sdGuiSurfaceRenderable();
							~sdGuiSurfaceRenderable();

	void					Init( const guiSurface_t& surface, const guiHandle_t& handle, const int allowInViewID = 0, const bool weaponDepthHack = false );
	void					SetPosition( const idVec3& origin, const idMat3& axis );
	void					GetPosition( idVec3 &origin, idMat3 &axis ) const;

	void					GetOrigin( idVec3 &origin ) const;
	void					SetOrigin( const idVec3 &origin );

	void					DrawCulled( const idFrustum& viewFrustum ) const;
#if !defined( _EDITWORLD )
	void					DrawCulled( const idPVS& pvs, const pvsHandle_t pvsHandle, const idFrustum& viewFrustum ) const;
#endif
	guiPoint_t				Trace( const idVec3& start, const idVec3& end ) const;

	int						GetGuiNum() const;
	guiHandle_t				GetGuiHandle() const;

	void					Clear();

private:
	void					Draw() const;

private:
	idVec3					origin;
	idMat3					axis;

	guiSurface_t			surface;
	guiHandle_t				handle;

	int						allowInViewID;
	bool					weaponDepthHack;

	float					axisLenSquared[2];
};

ID_INLINE void sdGuiSurfaceRenderable::GetOrigin( idVec3 &_origin ) const {
	_origin = origin;
}

ID_INLINE void sdGuiSurfaceRenderable::SetOrigin( const idVec3 &_origin ) {
	origin = _origin;
}

ID_INLINE void sdGuiSurfaceRenderable::SetPosition( const idVec3& _origin, const idMat3& _axis ) {
	origin = _origin;
	axis = _axis;
}

ID_INLINE void sdGuiSurfaceRenderable::GetPosition( idVec3 &_origin, idMat3 &_axis ) const {
	_origin = origin;
	_axis = axis;
}

#endif /* __GUISURFACERENDERABLE_H__ */
