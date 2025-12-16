#include "BaseMenu.h"
#include "Scissor.h"

#define MAX_SCISSORS 16

static class CScissorState
{
public:
	CScissorState() : iDepth( 0 ) { }

	int iDepth;
	Point coordStack[MAX_SCISSORS];
	Size sizeStack[MAX_SCISSORS];
} scissor;

void CropByPreviousScissors( Point pt, Size sz, int &x, int &y, int &w, int &h )
{
	int inRight = pt.x + sz.w;
	int inBottom = pt.y + sz.h;

	int outLeft = x, outRight = x + w;
	int outTop = y, outBottom = y + h;

	if( outLeft < pt.x )
		outLeft = pt.x;

	if( outTop < pt.y )
		outTop = pt.y;

	if( outRight > inRight )
		outRight = inRight;

	if( outBottom > inBottom )
		outBottom = inBottom;

	x = outLeft;
	y = outTop;

	w = outRight - outLeft;
	h = outBottom - outTop;
}

void UI::Scissor::PushScissor(int x, int y, int w, int h)
{
	if( scissor.iDepth + 1 > MAX_SCISSORS )
	{
		Con_DPrintf( "UI::PushScissor: Scissor stack limit exceeded" );
		return;
	}

	// have active scissors. Disable current
	if( scissor.iDepth > 0 )
	{
		EngFuncs::PIC_DisableScissor();
		CropByPreviousScissors( scissor.coordStack[scissor.iDepth - 1 ], scissor.sizeStack[scissor.iDepth - 1],
			x, y, w, h );
	}

	scissor.coordStack[scissor.iDepth].x = x;
	scissor.coordStack[scissor.iDepth].y = y;
	scissor.sizeStack[scissor.iDepth].w = w;
	scissor.sizeStack[scissor.iDepth].h = h;

	EngFuncs::PIC_EnableScissor( x, y, w, h );
	scissor.iDepth++;
}

void UI::Scissor::PopScissor()
{
	if( scissor.iDepth <= 0 )
	{
		Con_DPrintf( "UI::PopScissor: no stack" );
		return;
	}

	EngFuncs::PIC_DisableScissor();
	scissor.iDepth--;

	if( scissor.iDepth > 0 )
	{
		EngFuncs::PIC_EnableScissor(
			scissor.coordStack[scissor.iDepth - 1].x,
			scissor.coordStack[scissor.iDepth - 1].y,
			scissor.sizeStack[scissor.iDepth - 1].w,
			scissor.sizeStack[scissor.iDepth - 1].h );
	}
}


