// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "DeviceContext_local.h"
#include "FontManager_local.h"
#include "renderer/tr_local.h"

//static idCVar qqq("qqq","0",0,"");
#if 0
#define DC_PLACEHOLDER(...) //Sys_Printf(__VA_ARGS__)
#define DC_DRAW(...) R_DC_DebugType(__VA_ARGS__) //Sys_Printf(__VA_ARGS__)
#define DC_DEBUG_MATERIAL(...) //R_DC_DebugMaterial(__VA_ARGS__)
#define DC_DEBUG_POS(x, y) //R_DC_DebugPos(x, y)
#else
#define DC_PLACEHOLDER(...)
#define DC_DRAW(...)
#define DC_DEBUG_MATERIAL(...)
#define DC_DEBUG_POS(...)
#endif

#define DC_UNUSED_ON_GAME

#define AsASCIICharLang(text_, len_) ( !_hasWideCharFont || idStr::IsPureASCII(text_, len_) )

#define COLOR_FTOUB(x) ((byte)((x) * 255.0f))

#define NORMALIZE_DEG(x) DEG2RAD(-x) //karin: is degree in ETQW, but need radian in DOOM3

const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 480;

static void R_DC_DebugMaterial(const idMaterial *shader, float x, float y, float w, float h)
{
	if(!shader)
		return;

	//if (idStr::FindText(shader->GetName(), "commandmaps/island_territory") == -1) return;

	idVec4 c = tr.gameGuiModel->CurrentColor();
	tr.gameGuiModel->SetColor(1.0f, 0.0f, 0.0f, 0.5f);
	idWStr str = StrToWStr(shader->GetName());
	sdBounds2D bb = sdBounds2D(x, y, 640/* - x - w*/, 480/* - y - h*/);
	deviceContext->DrawText(str.c_str(), bb, DTF_WORDWRAP);
	tr.gameGuiModel->SetColor(c[0], c[1], c[2], c[3]);
}

static float dc_debug_x;
static float dc_debug_y;
static void R_DC_DebugPos(float x, float y)
{
	dc_debug_x = x;
	dc_debug_y = y;
}

static void R_DC_DebugType(const char *fmt, ...)
{
	char text[1024] = {0};
	va_list argptr;
	va_start(argptr, fmt);
	idStr::vsnPrintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);
	
	idVec4 c = tr.gameGuiModel->CurrentColor();
	tr.gameGuiModel->SetColor(1.0f, 0.0f, 0.0f, 0.5f);
	idWStr str = StrToWStr(text);
	sdBounds2D bb = sdBounds2D(dc_debug_x, dc_debug_y, 640/* - x - w*/, 480/* - y - h*/);
	deviceContext->DrawText(str.c_str(), bb, DTF_WORDWRAP);
	tr.gameGuiModel->SetColor(c[0], c[1], c[2], c[3]);
}

sdDeviceContextLocal::sdDeviceContextLocal()
: whiteImage(NULL)
{
	xScale = 0.0;
	SetSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	enableClipping = true;
	clipRects.Clear();
	tempColor = vec4_one;
	usingTempColor = false;
}

void sdDeviceContextLocal::Reset() {
	xScale = 0.0;
	SetSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	enableClipping = true;
	clipRects.Clear();
	tempColor = vec4_one;
	usingTempColor = false;
	whiteImage = declManager->FindMaterial("guis/assets/white");
	whiteImage->SetSort(SS_GUI);
}

void sdDeviceContextLocal::BeginEmitToCurrentView( const float modelMatrix[16], const int allowInViewID, const bool weaponDepthHack ) {
	tr.gameGuiModel->BeginEmitToCurrentView(modelMatrix, allowInViewID, weaponDepthHack);
}

void sdDeviceContextLocal::BeginEmitFullScreen() {
	tr.gameGuiModel->BeginEmitFullScreen();
}

void sdDeviceContextLocal::End() {
	tr.gameGuiModel->End();
	tr.gameGuiModel->SetRegisters(NULL);
	SetColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void sdDeviceContextLocal::SetColor( const idVec4& color ) {
    tr.gameGuiModel->SetColor(color[0], color[1], color[2], color[3]);
}

void sdDeviceContextLocal::SetColor( const float r, const float g, const float b, const float a ) {
    tr.gameGuiModel->SetColor(r, g, b, a);
}

idVec4 sdDeviceContextLocal::SetColorMultiplier( const idVec4& c ) {
    return tr.gameGuiModel->CurrentColor();
}

void sdDeviceContextLocal::SetRegister( const int index, const float value ) {
	tr.gameGuiModel->SetRegister(index, value);
}

void sdDeviceContextLocal::SetRegisters( const float* values ) {
	tr.gameGuiModel->SetRegisters(values);
}

void sdDeviceContextLocal::EnableClipping( bool enable ) {
    enableClipping = enable;
}

void sdDeviceContextLocal::PushClipRect( const sdBounds2D& bounds ) {
	clipRects.Append(bounds);
}

void sdDeviceContextLocal::PopClipRect() {
    if (clipRects.Num()) {
        clipRects.RemoveIndex(clipRects.Num()-1);
    }
}

void sdDeviceContextLocal::DrawRect( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float angle ) {
	DC_DEBUG_POS(x, y);
	DC_DRAW("DCDraw:DrawRect|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPicRotated(x, y, w, h, s1, t1, s2, t2, material, angle);
}

void sdDeviceContextLocal::DrawClippedRect( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float angle ) {
	DC_DRAW("DCDraw:DrawClippedRect|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	if (ClippedCoords(&x, &y, &w, &h, &s1, &t1, &s2, &t2)) {
		return;
	}

	DrawRect(x, y, w, h, s1, t1, s2, t2, material, angle);
}

void sdDeviceContextLocal::DrawMaskedClippedRect( float x, float y, float w, float h, float s01, float t01, float s02, float t02, float s11, float t11, float s12, float t12, const idMaterial* material, float angle ) {
	DC_UNUSED_ON_GAME
	DC_DRAW("DCDraw:DrawMaskedClippedRect|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	idDrawVert verts[4];
	glIndex_t indexes[6];
	indexes[0] = 3;
	indexes[1] = 0;
	indexes[2] = 2;
	indexes[3] = 2;
	indexes[4] = 0;
	indexes[5] = 1;
	verts[0].xyz[0] = x;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].st[0] = s01;
	verts[0].st[1] = t01;
	verts[0].normal[0] = 0;
	verts[0].normal[1] = 0;
	verts[0].normal[2] = 1;
	verts[0].tangents[0][0] = 1;
	verts[0].tangents[0][1] = 0;
	verts[0].tangents[0][2] = 0;
	verts[0].tangents[1][0] = 0;
	verts[0].tangents[1][1] = 1;
	verts[0].tangents[1][2] = 0;
	verts[1].xyz[0] = x + w;
	verts[1].xyz[1] = y;
	verts[1].xyz[2] = 0;
	verts[1].st[0] = s02;
	verts[1].st[1] = t02;
	verts[1].normal[0] = 0;
	verts[1].normal[1] = 0;
	verts[1].normal[2] = 1;
	verts[1].tangents[0][0] = 1;
	verts[1].tangents[0][1] = 0;
	verts[1].tangents[0][2] = 0;
	verts[1].tangents[1][0] = 0;
	verts[1].tangents[1][1] = 1;
	verts[1].tangents[1][2] = 0;
	verts[2].xyz[0] = x + w;
	verts[2].xyz[1] = y + h;
	verts[2].xyz[2] = 0;
	verts[2].st[0] = s12;
	verts[2].st[1] = t12;
	verts[2].normal[0] = 0;
	verts[2].normal[1] = 0;
	verts[2].normal[2] = 1;
	verts[2].tangents[0][0] = 1;
	verts[2].tangents[0][1] = 0;
	verts[2].tangents[0][2] = 0;
	verts[2].tangents[1][0] = 0;
	verts[2].tangents[1][1] = 1;
	verts[2].tangents[1][2] = 0;
	verts[3].xyz[0] = x;
	verts[3].xyz[1] = y + h;
	verts[3].xyz[2] = 0;
	verts[3].st[0] = s11;
	verts[3].st[1] = t11;
	verts[3].normal[0] = 0;
	verts[3].normal[1] = 0;
	verts[3].normal[2] = 1;
	verts[3].tangents[0][0] = 1;
	verts[3].tangents[0][1] = 0;
	verts[3].tangents[0][2] = 0;
	verts[3].tangents[1][0] = 0;
	verts[3].tangents[1][1] = 1;
	verts[3].tangents[1][2] = 0;

	//Generate a translation so we can translate to the center of the image rotate and draw
	idVec3 origTrans;
	origTrans.x = x+(w/2);
	origTrans.y = y+(h/2);
	origTrans.z = 0;


	//Rotate the verts about the z axis before drawing them
	idMat4 rotz;
	rotz.Identity();
	angle = NORMALIZE_DEG(angle); //karin: is degree in ETQW, but need radian in DOOM3
	float sinAng = idMath::Sin(angle);
	float cosAng = idMath::Cos(angle);
	rotz[0][0] = cosAng;
	rotz[0][1] = sinAng;
	rotz[1][0] = -sinAng;
	rotz[1][1] = cosAng;

	for (int i = 0; i < 4; i++) {
		//Translate to origin
		verts[i].xyz -= origTrans;

		//Rotate
		verts[i].xyz = rotz * verts[i].xyz;

		//Translate back
		verts[i].xyz += origTrans;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], 4, 6, material, (angle == 0.0) ? false : true);

	DC_DEBUG_MATERIAL(material, x, y, w, h);
}

void sdDeviceContextLocal::DrawCinematic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, idSoundEmitter* referenceSound, float angle ) {
	DC_PLACEHOLDER("DC:DrawCinematic|%s\n", material?material->GetName():NULL);

	if(!material)
		return;
}

void sdDeviceContextLocal::DrawClippedWinding( const idWinding2D& winding, const idMaterial* material ) {
	DC_PLACEHOLDER("DC:DrawClippedWinding|%s\n", material?material->GetName():NULL);

	int i, start;

	if(!material)
		return;

	if (winding.GetNumPoints() < 3)
		return;

	idList<idDrawVert> verts;
	verts.SetNum(winding.GetNumPoints());
	idDrawVert *vert = verts.Ptr();
	idList<glIndex_t> indexes;
	indexes.SetNum((winding.GetNumPoints() - 2) * 3);
	glIndex_t *idx = indexes.Ptr();

	for(i = 0; i < winding.GetNumPoints(); i++, vert++)
	{
		const idVec2 &uv = winding.GetST(i);

		vert->xyz[0] = winding[i][0];
		vert->xyz[1] = winding[i][1];
		vert->xyz[2] = 0;
		vert->st[0] = uv[0];
		vert->st[1] = uv[1];
		AdjustCoords(&vert->xyz[0], &vert->xyz[1], NULL, NULL);
		vert->normal[0] = 0;
		vert->normal[1] = 0;
		vert->normal[2] = 1;
		vert->tangents[0][0] = 1;
		vert->tangents[0][1] = 0;
		vert->tangents[0][2] = 0;
		vert->tangents[1][0] = 0;
		vert->tangents[1][1] = 1;
		vert->tangents[1][2] = 0;

		if (i < 2)
			continue;

		start = (i - 1);
		idx[0] = 0;
		idx[1] = start;
		idx[2] = i;
		idx += 3;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], verts.Num(), indexes.Num(), material, false, NULL);
}

void sdDeviceContextLocal::DrawClippedWindingMasked( const idWinding2D& winding, const idMaterial* material, float minx, float miny, float width, float height ) {
	DC_UNUSED_ON_GAME
	DC_PLACEHOLDER("DC:DrawClippedWindingMasked|%s\n", material?material->GetName():NULL);

	int i, start;

	if(!material)
		return;

	if (winding.GetNumPoints() < 3)
		return;

	idList<idDrawVert> verts;
	verts.SetNum(winding.GetNumPoints());
	idDrawVert *vert = verts.Ptr();
	idList<glIndex_t> indexes;
	indexes.SetNum((winding.GetNumPoints() - 2) * 3);
	glIndex_t *idx = indexes.Ptr();

	for(i = 0; i < winding.GetNumPoints(); i++, vert++)
	{
		const idVec2 &uv = winding.GetST(i);

		vert->xyz[0] = winding[i][0];
		vert->xyz[1] = winding[i][1];
		vert->xyz[2] = 0;
		vert->st[0] = uv[0];
		vert->st[1] = uv[1];
		AdjustCoords(&vert->xyz[0], &vert->xyz[1], NULL, NULL);
		vert->normal[0] = 0;
		vert->normal[1] = 0;
		vert->normal[2] = 1;
		vert->tangents[0][0] = 1;
		vert->tangents[0][1] = 0;
		vert->tangents[0][2] = 0;
		vert->tangents[1][0] = 0;
		vert->tangents[1][1] = 1;
		vert->tangents[1][2] = 0;

		if (i < 2)
			continue;

		start = (i - 1);
		idx[0] = 0;
		idx[1] = start;
		idx[2] = i;
		idx += 3;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], verts.Num(), indexes.Num(), material, false, &colorWhite);
}

void sdDeviceContextLocal::DrawMaskedMaterial( float x, float y, float w, float h, float u0, float v0, float u1, float v1, const idMaterial* material, const idVec4 &color, float scaleX, float scaleY, float offsetX, float offsetY, float angle ) {
	DC_DEBUG_POS(x, y);
	DC_DRAW("DCDraw:DrawMaskedMaterial|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	if (color.w == 0.0f) {
		return;
	}

	float s0 = 0.0f;
	float t0 = 0.0f;
	float s1 = 1.0f;
	float t1 = 1.0f;

	//
	//  handle negative scales as well
	if (scaleX < 0) {
		w *= -1;
		scaleX *= -1;
	}

	if (scaleY < 0) {
		h *= -1;
		scaleY *= -1;
	}

	//
	if (w < 0) {	// flip about vertical
		w  = -w;
		idSwap(s0, s1);
		s0 = s0 * scaleX;
		s1 = s1 * scaleX;
	} else {
		s0 = s0 * scaleX;
		s1 = s1 * scaleX;
	}

	if (h < 0) {	// flip about horizontal
		h  = -h;
		idSwap(t0, t1);
		t0 = t0 * scaleY;
		t1 = t1 * scaleY;
	} else {
		t0 = t0 * scaleY;
		t1 = t1 * scaleY;
	}

	s0 += offsetX;
	s1 += offsetX;
	t0 += offsetY;
	t1 += offsetY;

	if (angle == 0.0f && ClippedCoords(&x, &y, &w, &h, &s0, &t0, &s1, &t1)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPicRotated(x, y, w, h, s0, t0, s1, t1, material, angle, &color);
}

void sdDeviceContextLocal::DrawMaterial( float x, float y, float w, float h, const idMaterial* material, const idVec4 &color, float scaleX, float scaleY, float offsetX, float offsetY, float angle ) {
	DC_DEBUG_POS(x, y);
	DC_DRAW("DCDraw:DrawMaterial|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	if (color.w == 0.0f) {
		return;
	}

	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = 1.0f;
	float v1 = 1.0f;

	//
	//  handle negative scales as well
	if (scaleX < 0) {
		w *= -1;
		scaleX *= -1;
	}

	if (scaleY < 0) {
		h *= -1;
		scaleY *= -1;
	}

	//
	if (w < 0) {	// flip about vertical
		w  = -w;
		idSwap(u0, u1);
		u0 = u0 * scaleX;
		u1 = u1 * scaleX;
	} else {
		u0 = u0 * scaleX;
		u1 = u1 * scaleX;
	}

	if (h < 0) {	// flip about horizontal
		h  = -h;
		idSwap(v0, v1);
		v0 = v0 * scaleY;
		v1 = v1 * scaleY;
	} else {
		v0 = v0 * scaleY;
		v1 = v1 * scaleY;
	}

	u0 += offsetX;
	u1 += offsetX;
	v0 += offsetY;
	v1 += offsetY;

	if (angle == 0.0f && ClippedCoords(&x, &y, &w, &h, &u0, &v0, &u1, &v1)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPicRotated(x, y, w, h, u0, v0, u1, v1, material, angle, &color);
}

void sdDeviceContextLocal::DrawMaterial( const idVec4& rect, const idMaterial *material, const idVec4 &color, const idVec2& scale, const idVec2& offset, float angle ) {
	DC_DRAW("DCDraw:DrawMaterial2|%s\n", material?material->GetName():NULL);
	DrawMaterial(rect.x, rect.y, rect.z, rect.w, material, color, scale.x, scale.y, offset.x, offset.y, angle);
}

void sdDeviceContextLocal::DrawMaterial( const sdBounds2D& rect, const idMaterial *material, const idVec4 &color, const idVec2& scale, const idVec2& offset, float angle ) {
	DC_DRAW("DCDraw:DrawMaterial3|%s\n", material?material->GetName():NULL);
	DrawMaterial(rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight(), material, color, scale.x, scale.y, offset.x, offset.y, angle);
}

void sdDeviceContextLocal::DrawMaterial( float x, float y, float w, float h, const idMaterial* material, const idVec4 &color, const idVec2& st0, const idVec2& st1 ) {
	DC_DEBUG_POS(x, y);
	DC_DRAW("DCDraw:DrawMaterial5|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	//printf("dddmmm|%f %f %f %f\n",st0.x, st0.y, st1.x, st1.y);
	float s0 = st0[0];
	float t0 = st0[1];
	float s1 = st1[0];
	float t1 = st1[1];

	if (ClippedCoords(&x, &y, &w, &h, &s0, &t0, &s1, &t1)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPic(x, y, w, h, s0, t0, s1, t1, material, &color);

	DC_DEBUG_MATERIAL(material, x, y, w, h);
}

void sdDeviceContextLocal::DrawRotatedMaterial( float angle, idVec2 topLeft, idVec2 extents, const idMaterial* material, const idVec4& color ) {
	DC_UNUSED_ON_GAME
	DC_DRAW("DCDraw:DrawRotatedMaterial|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	if (color.w == 0.0f) {
		return;
	}

	float	s0, s1, t0, t1;
	float scalex = 1.0f;
	float scaley = 1.0f;
	float x = topLeft.x;
	float y = topLeft.y;
	float w = extents.x;
	float h = extents.y;

	//
	//  handle negative scales as well
	if (scalex < 0) {
		w *= -1;
		scalex *= -1;
	}

	if (scaley < 0) {
		h *= -1;
		scaley *= -1;
	}

	//
	if (w < 0) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if (h < 0) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	if (angle == 0.0f && ClippedCoords(&x, &y, &w, &h, &s0, &t0, &s1, &t1)) {
		return;
	}

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPicRotated(x, y, w, h, s0, t0, s1, t1, material, angle, &color);
}

void sdDeviceContextLocal::DrawWindingMaterial( const idWinding2D& winding, const idMaterial* material, const idVec4& color ) {
	DC_PLACEHOLDER("DC:DrawWindingMaterial|%s\n", material?material->GetName():NULL);

	if(!material)
		return;
}

void sdDeviceContextLocal::DrawRect( float x, float y, float w, float h, const idVec4 &color ) {
	DC_DRAW("DCDraw:DrawRect\n");

    if (color.w == 0.0f) {
        return;
    }

	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPic(x, y, w, h, 0, 0, 0, 0, whiteImage, &color);
}

void sdDeviceContextLocal::DrawClippedRect( float x, float y, float w, float h, const idVec4 &color ) {
	DC_DRAW("DCDraw:DrawClippedRect\n");

    if (color.w == 0.0f) {
        return;
    }

	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	DrawRect(x, y, w, h, color);
}

void sdDeviceContextLocal::DrawBox( float x, float y, float w, float h, float size, const idVec4 &color ) {
	DC_DRAW("DCDraw:DrawBox\n");

    if (color.w == 0.0f) {
        return;
    }

    AdjustCoords(&x, &y, &w, &h);
    DrawStretchPic(x, y, size, h, 0, 0, 0, 0, whiteImage, &color);
    DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, whiteImage, &color);
    DrawStretchPic(x, y, w, size, 0, 0, 0, 0, whiteImage, &color);
    DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, whiteImage, &color);
}

void sdDeviceContextLocal::DrawClippedBox( float x, float y, float w, float h, float size, const idVec4 &color ) {
	DC_DRAW("DCDraw:DrawClippedBox\n");

    if (color.w == 0.0f) {
        return;
    }

    if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
        return;
    }

	DrawBox(x, y, w, h, size, color);
}

/**
 *	tcInfo: texCoord Info
 * tcInfo.x: center.x
 * tcInfo.y: center.y
 * tcInfo.z: radius.x
 * tcInfo.w: radius.y
 */
void sdDeviceContextLocal::DrawCircleMaterial( const float x, const float y, const idVec2& radius, const int numSides, const idVec4& tcInfo, const idMaterial* material, const idVec4& color, float angle ) {
	if(!material)
		return;

	if (color.w == 0.0f) {
		return;
	}

	//DrawFilledArc( x, y, radius.x, numSides, qqq.GetFloat(), color, angle, material ); return;
	float offsetX = tcInfo[0];
	float offsetY = tcInfo[1];
	float scaleX = tcInfo[2];
	float scaleY = tcInfo[3];

	idList<idVec2> outerPoints;
	idList<idVec2> uvs;
	int start, i;

	CirclePoints(x, y, radius[0], radius[1], numSides, outerPoints);
	CirclePoints(offsetX, offsetY, scaleX, scaleY, numSides, uvs);

	idList<idDrawVert> verts;
	verts.SetNum(numSides + 1);
	idDrawVert *vert = verts.Ptr();
	idList<glIndex_t> indexes;
	indexes.SetNum(numSides * 3);
	glIndex_t *idx = indexes.Ptr();

	vert->xyz[0] = x;
	vert->xyz[1] = y;
	vert->xyz[2] = 0;
	vert->st[0] = offsetX;
	vert->st[1] = offsetY;
	vert->normal[0] = 0;
	vert->normal[1] = 0;
	vert->normal[2] = 1;
	vert->tangents[0][0] = 1;
	vert->tangents[0][1] = 0;
	vert->tangents[0][2] = 0;
	vert->tangents[1][0] = 0;
	vert->tangents[1][1] = 1;
	vert->tangents[1][2] = 0;

	vert++;

	for(i = 0; i < numSides; i++, vert++)
	{
		idVec2 &outer = outerPoints[i];
		idVec2 &uv = uvs[i];
		AdjustCoords(&outer[0], &outer[1], NULL, NULL);

		vert->xyz[0] = outer[0];
		vert->xyz[1] = outer[1];
		vert->xyz[2] = 0;
		vert->st[0] = uv[0];
		vert->st[1] = uv[1];
		vert->normal[0] = 0;
		vert->normal[1] = 0;
		vert->normal[2] = 1;
		vert->tangents[0][0] = 1;
		vert->tangents[0][1] = 0;
		vert->tangents[0][2] = 0;
		vert->tangents[1][0] = 0;
		vert->tangents[1][1] = 1;
		vert->tangents[1][2] = 0;

		if (i == 0)
			continue;

		start = (i - 1);
		idx[0] = 0;
		idx[1] = start;
		idx[2] = i;
		idx += 3;
	}

	// tail -> head
	start = (i - 1);
	idx[0] = 0;
	idx[1] = start;
	idx[2] = 1;


	//Generate a translation so we can translate to the center of the image rotate and draw
	idVec3 origTrans;
	origTrans.x = x;
	origTrans.y = y;
	origTrans.z = 0;

	//Rotate the verts about the z axis before drawing them
	idMat4 rotz;
	rotz.Identity();
	angle = NORMALIZE_DEG(angle); //karin: is degree in ETQW, but need radian in DOOM3
	float sinAng = idMath::Sin(angle);
	float cosAng = idMath::Cos(angle);
	rotz[0][0] = cosAng;
	rotz[0][1] = sinAng;
	rotz[1][0] = -sinAng;
	rotz[1][1] = cosAng;

	vert = &verts[1];
	for (i = 1; i < verts.Num(); i++, vert++) {
		//Translate to origin
		vert->xyz -= origTrans;

		//Rotate
		vert->xyz = rotz * vert->xyz;

		//Translate back
		vert->xyz += origTrans;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], verts.Num(), indexes.Num(), material, false, &color);
}

void sdDeviceContextLocal::DrawCircleMaterialMasked( const float x, const float y, const idVec2& radius, const int numSides, const idVec4& tcInfo, const idMaterial* material, const idVec4& color, float angle, float u0, float v0, float u1, float v1 ) {
	DC_PLACEHOLDER("DC:DrawCircleMaterialMasked|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	if (color.w == 0.0f) {
		return;
	}

	//DrawFilledArcMasked( x, y, radius.x, numSides, qqq.GetFloat(), color,0,0,1,1, angle, material ); return;
	float offsetX = tcInfo[0];
	float offsetY = tcInfo[1];
	float scaleX = tcInfo[2];
	float scaleY = tcInfo[3];

	idList<idVec2> outerPoints;
	idList<idVec2> uvs;
	int start, i;

	CirclePoints(x, y, radius[0], radius[1], numSides, outerPoints);
	CirclePoints(offsetX, offsetY, scaleX, scaleY, numSides, uvs);

	idList<idDrawVert> verts;
	verts.SetNum(numSides + 1);
	idDrawVert *vert = verts.Ptr();
	idList<glIndex_t> indexes;
	indexes.SetNum(numSides * 3);
	glIndex_t *idx = indexes.Ptr();

	vert->xyz[0] = x;
	vert->xyz[1] = y;
	vert->xyz[2] = 0;
	vert->st[0] = offsetX;
	vert->st[1] = offsetY;
	vert->normal[0] = 0;
	vert->normal[1] = 0;
	vert->normal[2] = 1;
	vert->tangents[0][0] = 1;
	vert->tangents[0][1] = 0;
	vert->tangents[0][2] = 0;
	vert->tangents[1][0] = 0;
	vert->tangents[1][1] = 1;
	vert->tangents[1][2] = 0;

	vert++;

	for(i = 0; i < numSides; i++, vert++)
	{
		idVec2 &outer = outerPoints[i];
		idVec2 &uv = uvs[i];
		AdjustCoords(&outer[0], &outer[1], NULL, NULL);

		vert->xyz[0] = outer[0];
		vert->xyz[1] = outer[1];
		vert->xyz[2] = 0;
		vert->st[0] = uv[0];
		vert->st[1] = uv[1];
		vert->normal[0] = 0;
		vert->normal[1] = 0;
		vert->normal[2] = 1;
		vert->tangents[0][0] = 1;
		vert->tangents[0][1] = 0;
		vert->tangents[0][2] = 0;
		vert->tangents[1][0] = 0;
		vert->tangents[1][1] = 1;
		vert->tangents[1][2] = 0;

		if (i == 0)
			continue;

		start = (i - 1);
		idx[0] = 0;
		idx[1] = start;
		idx[2] = i;
		idx += 3;
	}

	// tail -> head
	start = (i - 1);
	idx[0] = 0;
	idx[1] = start;
	idx[2] = 1;


	//Generate a translation so we can translate to the center of the image rotate and draw
	idVec3 origTrans;
	origTrans.x = x;
	origTrans.y = y;
	origTrans.z = 0;

	//Rotate the verts about the z axis before drawing them
	idMat4 rotz;
	rotz.Identity();
	angle = NORMALIZE_DEG(angle); //karin: is degree in ETQW, but need radian in DOOM3
	float sinAng = idMath::Sin(angle);
	float cosAng = idMath::Cos(angle);
	rotz[0][0] = cosAng;
	rotz[0][1] = sinAng;
	rotz[1][0] = -sinAng;
	rotz[1][1] = cosAng;

	vert = &verts[1];
	for (i = 1; i < verts.Num(); i++, vert++) {
		//Translate to origin
		vert->xyz -= origTrans;

		//Rotate
		vert->xyz = rotz * vert->xyz;

		//Translate back
		vert->xyz += origTrans;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], verts.Num(), indexes.Num(), material, false, &color);

	DC_DEBUG_MATERIAL(material, x, y, radius[0]*2, radius[1]*2);
}

void sdDeviceContextLocal::DrawCircle( const float x, const float y, const idVec2& radius, const float width, const int numSides, const idVec4& color ) {
	idList<idVec2> outerPoints;
	idList<idVec2> innerPoints;
	int start, i;

	CirclePoints(x, y, radius[0], radius[1], numSides, outerPoints);
	CirclePoints(x, y, radius[0] - width, radius[1] - width, numSides, innerPoints);

	idList<idDrawVert> verts;
	verts.SetNum(numSides * 2);
	idDrawVert *vert = verts.Ptr();
	idList<glIndex_t> indexes;
	indexes.SetNum(numSides * 6);
	glIndex_t *idx = indexes.Ptr();

	for(i = 0; i < numSides; i++, vert += 2)
	{
		idVec2 &outer = outerPoints[i];
		idVec2 &inner = innerPoints[i];
		AdjustCoords(&outer[0], &outer[1], NULL, NULL);
		AdjustCoords(&inner[0], &inner[1], NULL, NULL);

		vert[0].xyz[0] = inner[0];
		vert[0].xyz[1] = inner[1];
		vert[0].xyz[2] = 0;
		vert[0].st[0] = 0;
		vert[0].st[1] = 0;
		vert[0].normal[0] = 0;
		vert[0].normal[1] = 0;
		vert[0].normal[2] = 1;
		vert[0].tangents[0][0] = 1;
		vert[0].tangents[0][1] = 0;
		vert[0].tangents[0][2] = 0;
		vert[0].tangents[1][0] = 0;
		vert[0].tangents[1][1] = 1;
		vert[0].tangents[1][2] = 0;
		vert[1].xyz[0] = outer[0];
		vert[1].xyz[1] = outer[1];
		vert[1].xyz[2] = 0;
		vert[1].st[0] = 0;
		vert[1].st[1] = 0;
		vert[1].normal[0] = 0;
		vert[1].normal[1] = 0;
		vert[1].normal[2] = 1;
		vert[1].tangents[0][0] = 1;
		vert[1].tangents[0][1] = 0;
		vert[1].tangents[0][2] = 0;
		vert[1].tangents[1][0] = 0;
		vert[1].tangents[1][1] = 1;
		vert[1].tangents[1][2] = 0;

		if (i == 0)
			continue;

		start = (i - 1) * 2;
		idx[0] = start + 0;
		idx[1] = start + 3;
		idx[2] = start + 2;
		idx[3] = start + 0;
		idx[4] = start + 1;
		idx[5] = start + 3;
		idx += 6;
	}

	// tail -> head
	start = (i - 1) * 2;
	idx[0] = start + 0;
	idx[1] = 1;
	idx[2] = 0;
	idx[3] = start + 0;
	idx[4] = start + 1;
	idx[5] = 1;

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], verts.Num(), indexes.Num(), whiteImage, false, &color);
}

void sdDeviceContextLocal::DrawLineMaterial( const idVec2& start, const idVec2& end, const float width, const idMaterial* material, const idVec4& color ) {
	DC_UNUSED_ON_GAME

	if(!material)
		return;

	idDrawVert verts[4];
	glIndex_t indexes[6];
	LineDrawVerts(start, end, width, verts, indexes);
	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], 4, 6, material, start[0] != end[0] && start[1] != end[1], &color);
}

void sdDeviceContextLocal::DrawLine( const idVec2& start, const idVec2& end, const float width, const idVec4 &color ) {
	idDrawVert verts[4];
	glIndex_t indexes[6];
	LineDrawVerts(start, end, width, verts, indexes);
	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], 4, 6, whiteImage, start[0] != end[0] && start[1] != end[1], &color);
}

void sdDeviceContextLocal::DrawFilledArc( const float x, const float y, const float radius, int numSides, float percent, const idVec4 &color, float startAngle, const idMaterial *material ) {
	DC_PLACEHOLDER("DC:DrawFilledArc|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	if (color.w == 0.0f) {
		return;
	}

	if (percent == 0.0f) {
		return;
	}

	idList<idVec2> outerPoints;
	idList<idVec2> uvs;
	int start, i;

	CirclePoints(x, y, radius, radius, numSides, startAngle, percent, outerPoints);
	if(outerPoints.Num() < 2)
		return;
	CirclePoints(0.5f, 0.5f, 0.5f, 0.5f, numSides, startAngle, percent, uvs);

	idList<idDrawVert> verts;
	verts.SetNum(outerPoints.Num() + 1);
	idDrawVert *vert = verts.Ptr();
	idList<glIndex_t> indexes;
	indexes.SetNum((outerPoints.Num() - 1) * 3);
	glIndex_t *idx = indexes.Ptr();

	vert->xyz[0] = x;
	vert->xyz[1] = y;
	vert->xyz[2] = 0;
	vert->st[0] = 0.5f;
	vert->st[1] = 0.5f;
	vert->normal[0] = 0;
	vert->normal[1] = 0;
	vert->normal[2] = 1;
	vert->tangents[0][0] = 1;
	vert->tangents[0][1] = 0;
	vert->tangents[0][2] = 0;
	vert->tangents[1][0] = 0;
	vert->tangents[1][1] = 1;
	vert->tangents[1][2] = 0;

	vert++;

	for(i = 0; i < outerPoints.Num(); i++, vert++)
	{
		idVec2 &outer = outerPoints[i];
		idVec2 &uv = uvs[i];
	//Sys_Printf("xxx %f %f %f %f %d %d\n",i, outer.x, outer.y, uv.x, uv[1], outerPoints.Num(), uvs.Num());
		AdjustCoords(&outer[0], &outer[1], NULL, NULL);

		vert->xyz[0] = outer[0];
		vert->xyz[1] = outer[1];
		vert->xyz[2] = 0;
		vert->st[0] = uv[0];
		vert->st[1] = uv[1];
		vert->normal[0] = 0;
		vert->normal[1] = 0;
		vert->normal[2] = 1;
		vert->tangents[0][0] = 1;
		vert->tangents[0][1] = 0;
		vert->tangents[0][2] = 0;
		vert->tangents[1][0] = 0;
		vert->tangents[1][1] = 1;
		vert->tangents[1][2] = 0;

		if (i == 0)
			continue;

		start = (i - 1);
		idx[0] = 0;
		idx[1] = start;
		idx[2] = i;
		idx += 3;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], verts.Num(), indexes.Num(), material, false, &color);
}

void sdDeviceContextLocal::DrawFilledArcMasked( const float x, const float y, const float radius, int numSides, float percent, const idVec4 &color, float s11, float t11, float s12, float t12, float startAngle, const idMaterial *material ) {
	DC_PLACEHOLDER("DC:DrawFilledArcMasked|%s\n", material?material->GetName():NULL);

	if(!material)
		return;

	if (color.w == 0.0f) {
		return;
	}

	if (percent == 0.0f) {
		return;
	}

	idList<idVec2> outerPoints;
	idList<idVec2> uvs;
	int start, i;

	CirclePoints(x, y, radius, radius, numSides, startAngle, percent, outerPoints);
	if(outerPoints.Num() < 2)
		return;
	CirclePoints(0.5f, 0.5f, 0.5f, 0.5f, numSides, startAngle, percent, uvs);

	idList<idDrawVert> verts;
	verts.SetNum(outerPoints.Num() + 1);
	idDrawVert *vert = verts.Ptr();
	idList<glIndex_t> indexes;
	indexes.SetNum((outerPoints.Num() - 1) * 3);
	glIndex_t *idx = indexes.Ptr();

	vert->xyz[0] = x;
	vert->xyz[1] = y;
	vert->xyz[2] = 0;
	vert->st[0] = 0.5f;
	vert->st[1] = 0.5f;
	vert->normal[0] = 0;
	vert->normal[1] = 0;
	vert->normal[2] = 1;
	vert->tangents[0][0] = 1;
	vert->tangents[0][1] = 0;
	vert->tangents[0][2] = 0;
	vert->tangents[1][0] = 0;
	vert->tangents[1][1] = 1;
	vert->tangents[1][2] = 0;

	vert++;

	for(i = 0; i < outerPoints.Num(); i++, vert++)
	{
		idVec2 &outer = outerPoints[i];
		idVec2 &uv = uvs[i];
		AdjustCoords(&outer[0], &outer[1], NULL, NULL);

		vert->xyz[0] = outer[0];
		vert->xyz[1] = outer[1];
		vert->xyz[2] = 0;
		vert->st[0] = uv[0];
		vert->st[1] = uv[1];
		vert->normal[0] = 0;
		vert->normal[1] = 0;
		vert->normal[2] = 1;
		vert->tangents[0][0] = 1;
		vert->tangents[0][1] = 0;
		vert->tangents[0][2] = 0;
		vert->tangents[1][0] = 0;
		vert->tangents[1][1] = 1;
		vert->tangents[1][2] = 0;

		if (i == 0)
			continue;

		start = (i - 1);
		idx[0] = 0;
		idx[1] = start;
		idx[2] = i;
		idx += 3;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], verts.Num(), indexes.Num(), material, false, &color);
}

void sdDeviceContextLocal::DrawArc( const float x, const float y, const float radius, const float width, const int numSides, const float percent, const idVec4 &color, const float startAngle ) {
	DC_UNUSED_ON_GAME
	DC_PLACEHOLDER("DC:DrawArc\n");

	if (color.w == 0.0f) {
		return;
	}

	if (percent == 0.0f) {
		return;
	}

	idList<idVec2> outerPoints;
	int start, i;

	CirclePoints(x, y, radius, radius, numSides, startAngle, percent, outerPoints);
	if(outerPoints.Num() < 2)
		return;

	idList<idDrawVert> verts;
	verts.SetNum(outerPoints.Num() + 1);
	idDrawVert *vert = verts.Ptr();
	idList<glIndex_t> indexes;
	indexes.SetNum(outerPoints.Num() * 3);
	glIndex_t *idx = indexes.Ptr();

	vert->xyz[0] = x;
	vert->xyz[1] = y;
	vert->xyz[2] = 0;
	vert->st[0] = 0;
	vert->st[1] = 0;
	vert->normal[0] = 0;
	vert->normal[1] = 0;
	vert->normal[2] = 1;
	vert->tangents[0][0] = 1;
	vert->tangents[0][1] = 0;
	vert->tangents[0][2] = 0;
	vert->tangents[1][0] = 0;
	vert->tangents[1][1] = 1;
	vert->tangents[1][2] = 0;

	vert++;

	for(i = 0; i < outerPoints.Num(); i++, vert++)
	{
		idVec2 &outer = outerPoints[i];
		AdjustCoords(&outer[0], &outer[1], NULL, NULL);

		vert->xyz[0] = outer[0];
		vert->xyz[1] = outer[1];
		vert->xyz[2] = 0;
		vert->st[0] = 0;
		vert->st[1] = 0;
		vert->normal[0] = 0;
		vert->normal[1] = 0;
		vert->normal[2] = 1;
		vert->tangents[0][0] = 1;
		vert->tangents[0][1] = 0;
		vert->tangents[0][2] = 0;
		vert->tangents[1][0] = 0;
		vert->tangents[1][1] = 1;
		vert->tangents[1][2] = 0;

		if (i == 0)
			continue;

		start = (i - 1);
		idx[0] = 0;
		idx[1] = start;
		idx[2] = i;
		idx += 3;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], verts.Num(), indexes.Num(), whiteImage, false, &color);
}

void sdDeviceContextLocal::DrawTimer( const float x, const float y, const float w, const float h, float percent, const idVec4 &color, const idMaterial* material, bool invert, const idVec2& st0, const idVec2& st1 ) {
	DC_PLACEHOLDER("DC:DrawTimer|%s\n", material?material->GetName():NULL);

	if(!material)
		return;
}

qhandle_t sdDeviceContextLocal::FindFont( const char* name ) {
	return fontManagerLocal.FindFont(name);
}

void sdDeviceContextLocal::FreeFont( const qhandle_t font ) {
	fontManagerLocal.FreeFont(font);
}

const int sdDeviceContextLocal::GetFontHeight( const qhandle_t font, const int pointSize ) {
    return fontManagerLocal.GetFontHeight(font, pointSize);
}

void sdDeviceContextLocal::SetFont( const qhandle_t num ) {
	fontManagerLocal.SetFont(num);
}

void sdDeviceContextLocal::SetFontSize( const int pointSize ) {
	fontManagerLocal.SetFontSize(pointSize);
}

void sdDeviceContextLocal::DrawText( const wchar_t* text, const sdBounds2D& rect, unsigned int flags ) {
	bool wrap = (flags & DTF_WORDWRAP) || (flags & DTF_SINGLELINE) == 0;
	int textAlign = flags & (DTF_CENTER | DTF_RIGHT | DTF_LEFT | DTF_VCENTER | DTF_BOTTOM | DTF_TOP);
	
	//printf("xxx %ls |%f %f |%f %f\n", text, rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight());
	fontManagerLocal.DrawText(text, rect, textAlign, wrap, !wrap, tr.gameGuiModel->CurrentColor());
	//DrawBox(rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight(), 1, colorGreen);

	//const idMaterial* material = declManager->FindMaterial("commandmaps/area22");
	//DrawCircleMaterial( rect.GetCenter().x, rect.GetCenter().y, idVec2(rect.GetWidth()/2,rect.GetWidth()/2), 90, idVec4(0,0,1,1), material, idVec4(1,1,1,1), 135 );
}

void sdDeviceContextLocal::GetTextDimensions( const wchar_t* text, const sdBounds2D& rect, unsigned int flags, const qhandle_t font, const int pointSize, int& width, int& height, float* scale, int** charAdvances, idList< int >* lineBreaks ) {
	bool wrap = (flags & DTF_WORDWRAP) || (flags & DTF_SINGLELINE) == 0;
	int textAlign = flags & (DTF_CENTER | DTF_RIGHT | DTF_LEFT | DTF_VCENTER | DTF_BOTTOM | DTF_TOP);

	fontManagerLocal.GetTextDimensions(text, rect, textAlign, wrap, !wrap, font, pointSize, width, height, scale, charAdvances, lineBreaks);
	//printf("zzz %ls |%f %f |%f %f |%d %d\n", text, rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight(), width, height);
	//DrawBox(rect.GetLeft(), rect.GetTop(), width, height, 1, colorRed);
}

void sdDeviceContextLocal::OverrideAspectRationCorrection( bool setOverride ) {
}

float sdDeviceContextLocal::GetAspectRatioCorrection() const {
    return 1.0f;
}

bool sdDeviceContextLocal::ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2)
{

    if (enableClipping == false || clipRects.Num() == 0) {
        return false;
    }

    int c = clipRects.Num();

    while (--c > 0) {
        sdBounds2D *clipRect = &clipRects[c];

#if 0 // for debug
		{
			idVec4 color = colorRed;
			float x = clipRect->GetLeft();
			float y = clipRect->GetTop();
			float w = clipRect->GetWidth();
			float h = clipRect->GetHeight();
			float size = 2;
			DrawStretchPic(x, y, size, h, 0, 0, 0, 0, whiteImage, &color);
			DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, whiteImage, &color);
			DrawStretchPic(x, y, w, size, 0, 0, 0, 0, whiteImage, &color);
			DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, whiteImage, &color);
		}
#endif

        float ox = *x;
        float oy = *y;
        float ow = *w;
        float oh = *h;

        if (ow <= 0.0f || oh <= 0.0f) {
            break;
        }

        if (*x < clipRect->GetLeft()) {
            *w -= clipRect->GetLeft() - *x;
            *x = clipRect->GetLeft();
        } else if (*x > clipRect->GetRight()) {
            *x = *w = *y = *h = 0;
        }

        if (*y < clipRect->GetTop()) {
            *h -= clipRect->GetTop() - *y;
            *y = clipRect->GetTop();
        } else if (*y > clipRect->GetBottom()) {
            *x = *w = *y = *h = 0;
        }

        if (*w > clipRect->GetWidth()) {
            *w = clipRect->GetRight() - *x;
        } else if (*x + *w > clipRect->GetRight()) {
            *w = clipRect->GetRight() - *x;
        }

        if (*h > clipRect->GetHeight()) {
            *h = clipRect->GetBottom() - *y;
        } else if (*y + *h > clipRect->GetBottom()) {
            *h = clipRect->GetBottom() - *y;
        }

        if (s1 && s2 && t1 && t2 && ow > 0.0f) {
            float ns1, ns2, nt1, nt2;
            // upper left
            float u = (*x - ox) / ow;
            ns1 = *s1 * (1.0f - u) + *s2 * (u);

            // upper right
            u = (*x + *w - ox) / ow;
            ns2 = *s1 * (1.0f - u) + *s2 * (u);

            // lower left
            u = (*y - oy) / oh;
            nt1 = *t1 * (1.0f - u) + *t2 * (u);

            // lower right
            u = (*y + *h - oy) / oh;
            nt2 = *t1 * (1.0f - u) + *t2 * (u);

            // set values
            *s1 = ns1;
            *s2 = ns2;
            *t1 = nt1;
            *t2 = nt2;
        }
    }

    return (*w == 0 || *h == 0) ? true : false;
}

void sdDeviceContextLocal::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader, const idVec4 *color)
{
	//printf("ttt|%f %f %f %f\n",s1,t1,s2,t2);
	idDrawVert verts[4];
	glIndex_t indexes[6];
	indexes[0] = 3;
	indexes[1] = 0;
	indexes[2] = 2;
	indexes[3] = 2;
	indexes[4] = 0;
	indexes[5] = 1;
	verts[0].xyz[0] = x;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].st[0] = s1;
	verts[0].st[1] = t1;
	verts[0].normal[0] = 0;
	verts[0].normal[1] = 0;
	verts[0].normal[2] = 1;
	verts[0].tangents[0][0] = 1;
	verts[0].tangents[0][1] = 0;
	verts[0].tangents[0][2] = 0;
	verts[0].tangents[1][0] = 0;
	verts[0].tangents[1][1] = 1;
	verts[0].tangents[1][2] = 0;
	verts[1].xyz[0] = x + w;
	verts[1].xyz[1] = y;
	verts[1].xyz[2] = 0;
	verts[1].st[0] = s2;
	verts[1].st[1] = t1;
	verts[1].normal[0] = 0;
	verts[1].normal[1] = 0;
	verts[1].normal[2] = 1;
	verts[1].tangents[0][0] = 1;
	verts[1].tangents[0][1] = 0;
	verts[1].tangents[0][2] = 0;
	verts[1].tangents[1][0] = 0;
	verts[1].tangents[1][1] = 1;
	verts[1].tangents[1][2] = 0;
	verts[2].xyz[0] = x + w;
	verts[2].xyz[1] = y + h;
	verts[2].xyz[2] = 0;
	verts[2].st[0] = s2;
	verts[2].st[1] = t2;
	verts[2].normal[0] = 0;
	verts[2].normal[1] = 0;
	verts[2].normal[2] = 1;
	verts[2].tangents[0][0] = 1;
	verts[2].tangents[0][1] = 0;
	verts[2].tangents[0][2] = 0;
	verts[2].tangents[1][0] = 0;
	verts[2].tangents[1][1] = 1;
	verts[2].tangents[1][2] = 0;
	verts[3].xyz[0] = x;
	verts[3].xyz[1] = y + h;
	verts[3].xyz[2] = 0;
	verts[3].st[0] = s1;
	verts[3].st[1] = t2;
	verts[3].normal[0] = 0;
	verts[3].normal[1] = 0;
	verts[3].normal[2] = 1;
	verts[3].tangents[0][0] = 1;
	verts[3].tangents[0][1] = 0;
	verts[3].tangents[0][2] = 0;
	verts[3].tangents[1][0] = 0;
	verts[3].tangents[1][1] = 1;
	verts[3].tangents[1][2] = 0;

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], 4, 6, shader, false, color);

	//DC_DEBUG_MATERIAL(shader, x, y, w, h);
}

void sdDeviceContextLocal::SetSize(float width, float height)
{
	vidWidth = VIRTUAL_WIDTH;
	vidHeight = VIRTUAL_HEIGHT;
	xScale = yScale = 0.0f;

	if (width != 0.0f && height != 0.0f) {
		xScale = vidWidth * (1.0f / width);
		yScale = vidHeight * (1.0f / height);
	}
}

void sdDeviceContextLocal::AdjustCoords(float *x, float *y, float *w, float *h)
{
	if (x) {
		*x *= xScale;
	}
	
	if (y) {
		*y *= yScale;
	}
	
	if (w) {
		*w *= xScale;
	}
	
	if (h) {
		*h *= yScale;
	}
}

void sdDeviceContextLocal::DrawStretchPicRotated(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader, float angle, const idVec4 *color)
{

	//printf("rrr|%f %f %f %f\n",s1,t1,s2,t2);
	idDrawVert verts[4];
	glIndex_t indexes[6];
	indexes[0] = 3;
	indexes[1] = 0;
	indexes[2] = 2;
	indexes[3] = 2;
	indexes[4] = 0;
	indexes[5] = 1;
	verts[0].xyz[0] = x;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].st[0] = s1;
	verts[0].st[1] = t1;
	verts[0].normal[0] = 0;
	verts[0].normal[1] = 0;
	verts[0].normal[2] = 1;
	verts[0].tangents[0][0] = 1;
	verts[0].tangents[0][1] = 0;
	verts[0].tangents[0][2] = 0;
	verts[0].tangents[1][0] = 0;
	verts[0].tangents[1][1] = 1;
	verts[0].tangents[1][2] = 0;
	verts[1].xyz[0] = x + w;
	verts[1].xyz[1] = y;
	verts[1].xyz[2] = 0;
	verts[1].st[0] = s2;
	verts[1].st[1] = t1;
	verts[1].normal[0] = 0;
	verts[1].normal[1] = 0;
	verts[1].normal[2] = 1;
	verts[1].tangents[0][0] = 1;
	verts[1].tangents[0][1] = 0;
	verts[1].tangents[0][2] = 0;
	verts[1].tangents[1][0] = 0;
	verts[1].tangents[1][1] = 1;
	verts[1].tangents[1][2] = 0;
	verts[2].xyz[0] = x + w;
	verts[2].xyz[1] = y + h;
	verts[2].xyz[2] = 0;
	verts[2].st[0] = s2;
	verts[2].st[1] = t2;
	verts[2].normal[0] = 0;
	verts[2].normal[1] = 0;
	verts[2].normal[2] = 1;
	verts[2].tangents[0][0] = 1;
	verts[2].tangents[0][1] = 0;
	verts[2].tangents[0][2] = 0;
	verts[2].tangents[1][0] = 0;
	verts[2].tangents[1][1] = 1;
	verts[2].tangents[1][2] = 0;
	verts[3].xyz[0] = x;
	verts[3].xyz[1] = y + h;
	verts[3].xyz[2] = 0;
	verts[3].st[0] = s1;
	verts[3].st[1] = t2;
	verts[3].normal[0] = 0;
	verts[3].normal[1] = 0;
	verts[3].normal[2] = 1;
	verts[3].tangents[0][0] = 1;
	verts[3].tangents[0][1] = 0;
	verts[3].tangents[0][2] = 0;
	verts[3].tangents[1][0] = 0;
	verts[3].tangents[1][1] = 1;
	verts[3].tangents[1][2] = 0;

	//Generate a translation so we can translate to the center of the image rotate and draw
	idVec3 origTrans;
	origTrans.x = x+(w/2);
	origTrans.y = y+(h/2);
	origTrans.z = 0;


	//Rotate the verts about the z axis before drawing them
	idMat4 rotz;
	rotz.Identity();
	angle = NORMALIZE_DEG(angle); //karin: is degree in ETQW, but need radian in DOOM3
	float sinAng = idMath::Sin(angle);
	float cosAng = idMath::Cos(angle);
	rotz[0][0] = cosAng;
	rotz[0][1] = sinAng;
	rotz[1][0] = -sinAng;
	rotz[1][1] = cosAng;

	for (int i = 0; i < 4; i++) {
		//Translate to origin
		verts[i].xyz -= origTrans;

		//Rotate
		verts[i].xyz = rotz * verts[i].xyz;

		//Translate back
		verts[i].xyz += origTrans;
	}

	tr.gameGuiModel->DrawStretchPicWithColor(&verts[0], &indexes[0], 4, 6, shader, (angle == 0.0) ? false : true, color);

	DC_DEBUG_MATERIAL(shader, x, y, w, h);
}

void sdDeviceContextLocal::SetTempColor(const idVec4 &c)
{
	tempColor = tr.gameGuiModel->CurrentColor();
	tr.gameGuiModel->SetColor(c[0], c[1], c[2], c[3]);
	usingTempColor = true;
}

void sdDeviceContextLocal::UnsetTempColor()
{
	if(usingTempColor)
	{
		usingTempColor = false;
		tr.gameGuiModel->SetColor(tempColor[0], tempColor[1], tempColor[2], tempColor[3]);
	}
}

void sdDeviceContextLocal::LineDrawVerts(const idVec2 &start, const idVec2 &end, float width, idDrawVert verts[4], glIndex_t indexes[6])
{
	idVec3 a(start[0], start[1], 0.0f);
	idVec3 b(end[0], end[1], 0.0f);
	idVec3 up(0.0f, 0.0f, 1.0f);
	idVec3 atob = b - a;
	atob.Normalize();

	float half = width * 0.5f;
	idVec3 right3 = atob.Cross(up);
	right3.Normalize();
	const idVec2 &right = right3.ToVec2();

	idVec2 startLeft = start - right * half;
	idVec2 startRight = start + right * half;
	idVec2 endLeft = end - right * half;
	idVec2 endRight = end + right * half;

	AdjustCoords(&startLeft[0], &startLeft[1], NULL, NULL);
	AdjustCoords(&startRight[0], &startRight[1], NULL, NULL);
	AdjustCoords(&endLeft[0], &endLeft[1], NULL, NULL);
	AdjustCoords(&endRight[0], &endRight[1], NULL, NULL);

	float s1 = 0.0f;
	float t1 = 0.0f;
	float s2 = 1.0f;
	float t2 = 1.0f;

	indexes[0] = 3;
	indexes[1] = 0;
	indexes[2] = 2;
	indexes[3] = 2;
	indexes[4] = 0;
	indexes[5] = 1;
	verts[0].xyz[0] = startRight[0];
	verts[0].xyz[1] = startRight[1];
	verts[0].xyz[2] = 0;
	verts[0].st[0] = s1;
	verts[0].st[1] = t1;
	verts[0].normal[0] = 0;
	verts[0].normal[1] = 0;
	verts[0].normal[2] = 1;
	verts[0].tangents[0][0] = 1;
	verts[0].tangents[0][1] = 0;
	verts[0].tangents[0][2] = 0;
	verts[0].tangents[1][0] = 0;
	verts[0].tangents[1][1] = 1;
	verts[0].tangents[1][2] = 0;
	verts[1].xyz[0] = endRight[0];
	verts[1].xyz[1] = endRight[1];
	verts[1].xyz[2] = 0;
	verts[1].st[0] = s2;
	verts[1].st[1] = t1;
	verts[1].normal[0] = 0;
	verts[1].normal[1] = 0;
	verts[1].normal[2] = 1;
	verts[1].tangents[0][0] = 1;
	verts[1].tangents[0][1] = 0;
	verts[1].tangents[0][2] = 0;
	verts[1].tangents[1][0] = 0;
	verts[1].tangents[1][1] = 1;
	verts[1].tangents[1][2] = 0;
	verts[2].xyz[0] = endLeft[0];
	verts[2].xyz[1] = endLeft[1];
	verts[2].xyz[2] = 0;
	verts[2].st[0] = s2;
	verts[2].st[1] = t2;
	verts[2].normal[0] = 0;
	verts[2].normal[1] = 0;
	verts[2].normal[2] = 1;
	verts[2].tangents[0][0] = 1;
	verts[2].tangents[0][1] = 0;
	verts[2].tangents[0][2] = 0;
	verts[2].tangents[1][0] = 0;
	verts[2].tangents[1][1] = 1;
	verts[2].tangents[1][2] = 0;
	verts[3].xyz[0] = startLeft[0];
	verts[3].xyz[1] = startLeft[1];
	verts[3].xyz[2] = 0;
	verts[3].st[0] = s1;
	verts[3].st[1] = t2;
	verts[3].normal[0] = 0;
	verts[3].normal[1] = 0;
	verts[3].normal[2] = 1;
	verts[3].tangents[0][0] = 1;
	verts[3].tangents[0][1] = 0;
	verts[3].tangents[0][2] = 0;
	verts[3].tangents[1][0] = 0;
	verts[3].tangents[1][1] = 1;
	verts[3].tangents[1][2] = 0;
}

void sdDeviceContextLocal::CirclePoints(float x, float y, float xRadius, float yRadius, int numSides, idList<idVec2> &verts)
{
	float r;
	int i;

	verts.SetNum(numSides + 1);
	
	float step = 360.0f / (float)numSides;

	for(i = 0; i < numSides; i++)
	{
		r = DEG2RAD(step * (float)i);
		verts[i][0] = x + xRadius * idMath::Cos(r);
		verts[i][1] = y + yRadius * idMath::Sin(r);
	}

	verts[i] = verts[0];
}

void sdDeviceContextLocal::CirclePoints(float x, float y, float xRadius, float yRadius, int numSides, float start, float percent, idList<idVec2> &verts)
{
	float r;
	int i;
	float last, cur;

	verts.Resize(numSides + 3);
	last = -1.0f;
	if(start < 0.0f)
		start += 360.0f;
	
	float step = 360.0f / (float)numSides;
	float end = 360.0f * percent + start;
	if(end > 360.0f)
		numSides *= 2;
	//common->Printf("xxx %f %f %f\n", start, end, percent);

	for(i = 0; i < numSides; i++)
	{
		cur = step * (float)i;
		if(cur < start)
		{
			last = cur;
			continue;
		}
		if(last >= 0.0f)
		{
			if(cur > start && last < start)
			{
				r = DEG2RAD(start);
				idVec2 &vert = verts.Alloc();
				vert[0] = x + xRadius * idMath::Cos(r);
				vert[1] = y + yRadius * idMath::Sin(r);
			}
			else if(cur > end && last < end)
			{
				r = DEG2RAD(end);
				idVec2 &vert = verts.Alloc();
				vert[0] = x + xRadius * idMath::Cos(r);
				vert[1] = y + yRadius * idMath::Sin(r);
				break;
			}
		}
		r = DEG2RAD(cur);
		idVec2 &vert = verts.Alloc();
		vert[0] = x + xRadius * idMath::Cos(r);
		vert[1] = y + yRadius * idMath::Sin(r);
		if(cur > end)
			break;
		last = cur;
	}
}

sdDeviceContextLocal deviceContextLocal;

sdDeviceContext* deviceContext = &deviceContextLocal;
