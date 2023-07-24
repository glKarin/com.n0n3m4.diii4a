// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DEVICECONTEXT_H__
#define __DEVICECONTEXT_H__

class idSoundEmitter;

enum drawTextFlags {
	DTF_LEFT				= BIT(0),	// align text to the left of the rectangle
	DTF_CENTER				= BIT(1),	// centers text horizontally in the rectangle
	DTF_RIGHT				= BIT(2),	// align text to the right of the rectangle
	DTF_BOTTOM				= BIT(3),	// align text to the bottom of the rectangle (single line only)
	DTF_VCENTER				= BIT(4),	// centers text vertically in the rectangle (single line only)
	DTF_TOP					= BIT(5),	// aligns text to the top of the rectangle
	DTF_NOCLIPPING			= BIT(6),	// draw text without clipping (faster)
	DTF_SINGLELINE			= BIT(7),	// display on a single lone only, carriage returns and line feeds do not break the line
	DTF_WORDWRAP			= BIT(8),	// lines are automatically wrapped between words if a word would extend past the edge of the rectangle
	DTF_NOCOLORS			= BIT(9),	// don't use color escapes
	DTF_INCLUDECOLORESCAPES	= BIT(10),	// treat color escapes as visible characters
	DTF_DROPSHADOW			= BIT(11),	// draw a black drop shadow (TODO in the font rendering, handled by the GUIs for now)
	DTF_TRUNCATE			= BIT(12),	// adds a ... to the end of text that's too long for the draw rectangle
	DTF_NO_SNAP_TO_PIXELS	= BIT(13),	// disables snapping coordinates to screen pixels (useful for scrolling text)
};

class sdDeviceContext {
public:
	virtual void			Reset() = 0;

	virtual void			BeginEmitToCurrentView( const float modelMatrix[16], const int allowInViewID, const bool weaponDepthHack ) = 0;
	virtual void			BeginEmitFullScreen() = 0;
	virtual void			End() = 0;

	virtual void			SetColor( const idVec4& color ) = 0;
	virtual void			SetColor( const float r, const float g, const float b, const float a ) = 0;
	virtual idVec4			SetColorMultiplier( const idVec4& c ) = 0;
	virtual void			SetRegister( const int index, const float value ) = 0;
	virtual void			SetRegisters( const float* values ) = 0;

	virtual void			EnableClipping( bool enable ) = 0;
	virtual void 			PushClipRect( const sdBounds2D& bounds ) = 0;
	virtual void 			PopClipRect() = 0;

	virtual void			DrawRect( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float angle = 0.0f ) = 0;
	virtual void			DrawClippedRect( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float angle = 0.0f ) = 0;
	virtual void			DrawMaskedClippedRect( float x, float y, float w, float h, float s01, float t01, float s02, float t02, float s11, float t11, float s12, float t12, const idMaterial* material, float angle = 0.0f ) = 0;

	virtual void			DrawCinematic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, idSoundEmitter* referenceSound, float angle = 0.0f ) = 0;

	virtual void			DrawClippedWinding( const idWinding2D& winding, const idMaterial* material ) = 0;
	virtual void			DrawClippedWindingMasked( const idWinding2D& winding, const idMaterial* material, float minx, float miny, float width, float height ) = 0;

	virtual void			DrawMaskedMaterial( float x, float y, float w, float h, float u0, float v0, float u1, float v1, const idMaterial* material, const idVec4 &color, float scaleX = 1.0f, float scaleY = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f, float angle = 0.0f ) = 0;

	virtual void			DrawMaterial( float x, float y, float w, float h, const idMaterial* material, const idVec4 &color, float scaleX = 1.0f, float scaleY = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f, float angle = 0.0f ) = 0;
	virtual void			DrawMaterial( const idVec4& rect, const idMaterial *material, const idVec4 &color, const idVec2& scale = idVec2( 1.0f, 1.0f ), const idVec2& offset = vec2_origin, float angle = 0.0f ) = 0;
	virtual void			DrawMaterial( const sdBounds2D& rect, const idMaterial *material, const idVec4 &color, const idVec2& scale = idVec2( 1.0f, 1.0f ), const idVec2& offset = vec2_origin, float angle = 0.0f ) = 0;
	virtual void			DrawMaterial( float x, float y, float w, float h, const idMaterial* material, const idVec4 &color, const idVec2& st0, const idVec2& st1 ) = 0;
	virtual void			DrawRotatedMaterial( float angle, idVec2 topLeft, idVec2 extents, const idMaterial* material, const idVec4& color ) = 0;

	virtual void			DrawWindingMaterial( const idWinding2D& winding, const idMaterial* material, const idVec4& color ) = 0;

	virtual void			DrawRect( float x, float y, float w, float h, const idVec4 &color ) = 0;
	virtual void			DrawClippedRect( float x, float y, float w, float h, const idVec4 &color ) = 0;
	virtual void			DrawBox( float x, float y, float w, float h, float size, const idVec4 &color ) = 0;
	virtual void			DrawClippedBox( float x, float y, float w, float h, float size, const idVec4 &color ) = 0;

	virtual void			DrawCircleMaterial( const float x, const float y, const idVec2& radius, const int numSides, const idVec4& tcInfo, const idMaterial* material, const idVec4& color, float rotation ) = 0;
	virtual void			DrawCircleMaterialMasked( const float x, const float y, const idVec2& radius, const int numSides, const idVec4& tcInfo, const idMaterial* material, const idVec4& color, float rotation, float s11, float t11, float s12, float t12 ) = 0;
	virtual void			DrawCircle( const float x, const float y, const idVec2& radius, const float width, const int numSides, const idVec4& color ) = 0;

	virtual void			DrawLineMaterial( const idVec2& start, const idVec2& end, const float width, const idMaterial* material, const idVec4& color ) = 0;
	virtual void			DrawLine( const idVec2& start, const idVec2& end, const float width, const idVec4 &color ) = 0;

	virtual void			DrawFilledArc( const float x, const float y, const float radius, int numSides, float percent, const idVec4 &color, float startAngle = 0.0f, const idMaterial *material = NULL ) = 0;
	virtual void			DrawFilledArcMasked( const float x, const float y, const float radius, int numSides, float percent, const idVec4 &color, float s11, float t11, float s12, float t12, float startAngle = 0.0f, const idMaterial *material = NULL ) = 0;
	virtual void			DrawArc( const float x, const float y, const float radius, const float width, const int numSides, const float percent, const idVec4 &color, const float startAngle = 0.0f ) = 0;
	virtual void			DrawTimer( const float x, const float y, const float w, const float h, float percent, const idVec4 &color, const idMaterial* material, bool invert, const idVec2& st0 = vec2_zero, const idVec2& st1 = vec2_one ) = 0;

	virtual qhandle_t		FindFont( const char* fontName ) = 0;
	virtual void			FreeFont( const qhandle_t font ) = 0;

	virtual const int		GetFontHeight( const qhandle_t font, const int pointSize ) = 0;

	virtual void			SetFont( const qhandle_t font ) = 0;
	virtual void			SetFontSize( const int pointSize ) = 0;
    virtual void			DrawText( const wchar_t* text, const sdBounds2D& rect, unsigned int flags ) = 0;
	virtual void			GetTextDimensions( const wchar_t* text, const sdBounds2D& rect, unsigned int flags, const qhandle_t font, const int pointSize, int& width, int& height, float* scale = NULL, int** charAdvances = NULL, idList< int >* lineBreaks = NULL ) = 0;

	virtual void			OverrideAspectRationCorrection( bool setOverride ) = 0;
	virtual float			GetAspectRatioCorrection() const = 0;
};

extern sdDeviceContext* deviceContext;

#endif /* !__DEVICECONTEXT_H__ */
