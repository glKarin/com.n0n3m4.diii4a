/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


typedef struct {
	const idMaterial	*material;
	float				color[4];
	int					firstVert;
	int					numVerts;
	int					firstIndex;
	int					numIndexes;
#ifdef _SPLASHDAMAGE //karin: register shaderParms from GUI
	float				registers[MAX_ENTITY_SHADER_PARMS];
	bool				registerShaderParms;
#endif
} guiModelSurface_t;

class idGuiModel
{
	public:
		idGuiModel();

		void	Clear();

		void	WriteToDemo(idDemoFile *demo);
		void	ReadFromDemo(idDemoFile *demo);

		void	EmitToCurrentView(float modelMatrix[16], bool depthHack);
		void	EmitFullScreen();

		// these calls are forwarded from the renderer
		void	SetColor(float r, float g, float b, float a);
		void	DrawStretchPic(const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *hShader,
		                       bool clip = true, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f);
		void	DrawStretchPic(float x, float y, float w, float h,
		                       float s1, float t1, float s2, float t2, const idMaterial *hShader);
		void	DrawStretchTri(idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material);

		//---------------------------
	private:
		void	AdvanceSurf();
		void	EmitSurface(guiModelSurface_t *surf, float modelMatrix[16], float modelViewMatrix[16], bool depthHack);

		guiModelSurface_t		*surf;

		idList<guiModelSurface_t>	surfaces;
		idList<glIndex_t>		indexes;
		idList<idDrawVert>	verts;
#ifdef _SPLASHDAMAGE //karin: using sdGuiModel on sdDeviceContext, so idGuiModel only used on renderer
		friend class sdGuiModel;
#endif
};

#ifdef _SPLASHDAMAGE //karin: using sdGuiModel on sdDeviceContext, so idGuiModel only used on renderer
class sdGuiModel : public idGuiModel
{
public:
	sdGuiModel(void);

	void	BeginEmitToCurrentView(const float modelMatrix[16], int allowInViewID, bool depthHack);
	void	BeginEmitFullScreen(void);
	void	End(void);
	idVec4	CurrentColor(void);
	void	SetRegister(int index, float value);
	void	SetRegisters(const float *values);
	void	Flush(void);
	// idDrawVert must setup color
	void	DrawStretchPic(const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *hShader,
			bool clip = true, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f);
	void	DrawStretchPicWithColor(const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *hShader,
			bool clip = true, const idVec4 *color = NULL, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f);
	void	ClearCurrentView(void);
	void	EmitCurrentView(void);

private:
	struct emitCurrentView_t 
	{
		srfTriangles_t	tri;
		float shaderParms[MAX_ENTITY_SHADER_PARMS];
		float modelMatrix[16];
		bool weaponDepthHack;
		const idMaterial *material;
	};
	void	EndCurrentView(void);
	void	AddCurrentViewSurface(guiModelSurface_t *surf);

	//---------------------------
private:
	enum {
		EMIT_TO_NONE,
		EMIT_TO_CURRENTVIEW,
		EMIT_TO_FULLSCREEN,
	};

	float					emitModelMatrix[16];
	bool					emitDepthHack;
	int						usingCurrentView;
	idList<emitCurrentView_t> currentViewSurfs;
};
#endif
