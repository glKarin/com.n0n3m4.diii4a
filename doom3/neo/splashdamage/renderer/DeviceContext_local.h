// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DEVICECONTEXT_LOCAL_H__
#define __DEVICECONTEXT_LOCAL_H__

#include "DeviceContext.h"

class sdDeviceContextLocal : public sdDeviceContext
{
public:
    sdDeviceContextLocal();

    virtual void			Reset();

    virtual void			BeginEmitToCurrentView( const float modelMatrix[16], const int allowInViewID, const bool weaponDepthHack );
    virtual void			BeginEmitFullScreen();
    virtual void			End();

    virtual void			SetColor( const idVec4& color );
    virtual void			SetColor( const float r, const float g, const float b, const float a );
    virtual idVec4			SetColorMultiplier( const idVec4& c );
    virtual void			SetRegister( const int index, const float value );
    virtual void			SetRegisters( const float* values );

    virtual void			EnableClipping( bool enable );
    virtual void 			PushClipRect( const sdBounds2D& bounds );
    virtual void 			PopClipRect();

    virtual void			DrawRect( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float angle = 0.0f );
    virtual void			DrawClippedRect( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float angle = 0.0f );
    virtual void			DrawMaskedClippedRect( float x, float y, float w, float h, float s01, float t01, float s02, float t02, float s11, float t11, float s12, float t12, const idMaterial* material, float angle = 0.0f );

    virtual void			DrawCinematic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, idSoundEmitter* referenceSound, float angle = 0.0f );

    virtual void			DrawClippedWinding( const idWinding2D& winding, const idMaterial* material );
    virtual void			DrawClippedWindingMasked( const idWinding2D& winding, const idMaterial* material, float minx, float miny, float width, float height );

    virtual void			DrawMaskedMaterial( float x, float y, float w, float h, float u0, float v0, float u1, float v1, const idMaterial* material, const idVec4 &color, float scaleX = 1.0f, float scaleY = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f, float angle = 0.0f );

    virtual void			DrawMaterial( float x, float y, float w, float h, const idMaterial* material, const idVec4 &color, float scaleX = 1.0f, float scaleY = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f, float angle = 0.0f );
    virtual void			DrawMaterial( const idVec4& rect, const idMaterial *material, const idVec4 &color, const idVec2& scale = idVec2( 1.0f, 1.0f ), const idVec2& offset = vec2_origin, float angle = 0.0f );
    virtual void			DrawMaterial( const sdBounds2D& rect, const idMaterial *material, const idVec4 &color, const idVec2& scale = idVec2( 1.0f, 1.0f ), const idVec2& offset = vec2_origin, float angle = 0.0f );
    virtual void			DrawMaterial( float x, float y, float w, float h, const idMaterial* material, const idVec4 &color, const idVec2& st0, const idVec2& st1 );
    virtual void			DrawRotatedMaterial( float angle, idVec2 topLeft, idVec2 extents, const idMaterial* material, const idVec4& color );

    virtual void			DrawWindingMaterial( const idWinding2D& winding, const idMaterial* material, const idVec4& color );

    virtual void			DrawRect( float x, float y, float w, float h, const idVec4 &color );
    virtual void			DrawClippedRect( float x, float y, float w, float h, const idVec4 &color );
    virtual void			DrawBox( float x, float y, float w, float h, float size, const idVec4 &color );
    virtual void			DrawClippedBox( float x, float y, float w, float h, float size, const idVec4 &color );

    virtual void			DrawCircleMaterial( const float x, const float y, const idVec2& radius, const int numSides, const idVec4& tcInfo, const idMaterial* material, const idVec4& color, float rotation );
    virtual void			DrawCircleMaterialMasked( const float x, const float y, const idVec2& radius, const int numSides, const idVec4& tcInfo, const idMaterial* material, const idVec4& color, float rotation, float s11, float t11, float s12, float t12 );
    virtual void			DrawCircle( const float x, const float y, const idVec2& radius, const float width, const int numSides, const idVec4& color );

    virtual void			DrawLineMaterial( const idVec2& start, const idVec2& end, const float width, const idMaterial* material, const idVec4& color );
    virtual void			DrawLine( const idVec2& start, const idVec2& end, const float width, const idVec4 &color );

    virtual void			DrawFilledArc( const float x, const float y, const float radius, int numSides, float percent, const idVec4 &color, float startAngle = 0.0f, const idMaterial *material = NULL );
    virtual void			DrawFilledArcMasked( const float x, const float y, const float radius, int numSides, float percent, const idVec4 &color, float s11, float t11, float s12, float t12, float startAngle = 0.0f, const idMaterial *material = NULL );
    virtual void			DrawArc( const float x, const float y, const float radius, const float width, const int numSides, const float percent, const idVec4 &color, const float startAngle = 0.0f );
    virtual void			DrawTimer( const float x, const float y, const float w, const float h, float percent, const idVec4 &color, const idMaterial* material, bool invert, const idVec2& st0 = vec2_zero, const idVec2& st1 = vec2_one );

    virtual qhandle_t		FindFont( const char* fontName );
    virtual void			FreeFont( const qhandle_t font );

    virtual const int		GetFontHeight( const qhandle_t font, const int pointSize );

    virtual void			SetFont( const qhandle_t font );
    virtual void			SetFontSize( const int pointSize );
    virtual void			DrawText( const wchar_t* text, const sdBounds2D& rect, unsigned int flags );
    virtual void			GetTextDimensions( const wchar_t* text, const sdBounds2D& rect, unsigned int flags, const qhandle_t font, const int pointSize, int& width, int& height, float* scale = NULL, int** charAdvances = NULL, idList< int >* lineBreaks = NULL );

    virtual void			OverrideAspectRationCorrection( bool setOverride );
    virtual float			GetAspectRatioCorrection() const;

private:
	bool					ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2);
	void					DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader, const idVec4 *color = NULL);
	void					SetSize(float width, float height);
	void 					AdjustCoords(float *x, float *y, float *w, float *h);
	void 					DrawStretchPicRotated(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader, float angle, const idVec4 *color = NULL);
	void					SetTempColor(const idVec4 &c);
	void					UnsetTempColor();
	void					LineDrawVerts(const idVec2 &start, const idVec2 &end, float width, idDrawVert verts[4], glIndex_t indexes[6]);
	void					CirclePoints(float x, float y, float xRadius, float yRadius, int numSides, idList<idVec2> &verts);
	void					CirclePoints(float x, float y, float xRadius, float yRadius, int numSides, float start, float percent, idList<idVec2> &verts);

private:
	float					xScale;
	float					yScale;

	float					vidHeight;
	float					vidWidth;
	bool				    enableClipping;
	idList<sdBounds2D>		clipRects;
	const idMaterial		*whiteImage;
	idVec4					tempColor;
	bool					usingTempColor;

	friend class sdFontManagerLocal;
};


extern sdDeviceContextLocal deviceContextLocal;

#endif /* !__DEVICECONTEXT_LOCAL_H__ */
