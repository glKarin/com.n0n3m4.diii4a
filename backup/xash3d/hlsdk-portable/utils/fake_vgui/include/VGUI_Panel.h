//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_PANEL_H
#define VGUI_PANEL_H


/*

TODO:

Maybe have the border know who they are added to.
A border can only be added to 1 thing, and will be
removed from the other. That way they can actually
be memory managed. Also do Layout's this way too.

TODO:
	outlinedRect should have a thickness arg

*/

					 
#include<VGUI.h>
#include<VGUI_Dar.h>
#include<VGUI_Scheme.h>
#include<VGUI_Color.h>
#include<VGUI_Cursor.h>
#include <VGUI_MouseCode.h>
#include <VGUI_KeyCode.h>
//#include <VGUI_RepaintSignal.h>

namespace vgui
{

enum  KeyCode;
enum  MouseCode;
class SurfaceBase;
class FocusChangeSignal;
class InputSignal;
class Cursor;
class Layout;
class FocusNavGroup;
class Border;
class Font;
class BuildGroup;
class App;
class LayoutInfo;
class RepaintSignal;

class VGUIAPI Panel
{
public:
	Panel() {}
	Panel(int x,int y,int wide,int tall) {setPos(x, y); setSize(wide, tall);}
private:
	void init(int x,int y,int wide,int tall) {}
public:
	virtual void   setPos(int x,int y) {_pos[0] = x; _pos[1] = y;}
	virtual void   getPos(int& x,int& y) {x = _pos[0]; y = _pos[1];}
	virtual void   setSize(int wide,int tall) {_size[0] = wide, _size[1] = tall;}
	virtual void   getSize(int& wide,int& tall) {wide = _size[0], tall = _size[1];}
	virtual void   setBounds(int x,int y,int wide,int tall) {}
	virtual void   getBounds(int& x,int& y,int& wide,int& tall) {}
	virtual int    getWide() {return _size[0];}
	virtual int    getTall() {return _size[1];}
	virtual Panel* getParent() {return _parent;}
	virtual void   setVisible(bool state) {_visible = state;}
	virtual bool   isVisible() {return _visible;}
	virtual bool   isVisibleUp() {return false;}
	virtual void   repaint() {}
	virtual void   repaintAll() {}
	virtual void   getAbsExtents(int& x0,int& y0,int& x1,int& y1) {}
	virtual void   getClipRect(int& x0,int& y0,int& x1,int& y1) {}
	virtual void   setParent(Panel* newParent) {_parent = newParent; newParent->addChild(this);}
	virtual void   addChild(Panel* child) {}
	virtual void   insertChildAt(Panel* child,int index) {}
	virtual void   removeChild(Panel* child) {}
	virtual bool   wasMousePressed(MouseCode code) {return false;}
	virtual bool   wasMouseDoublePressed(MouseCode code) {return false;}
	virtual bool   isMouseDown(MouseCode code) {return false;}
	virtual bool   wasMouseReleased(MouseCode code) {return false;}
	virtual bool   wasKeyPressed(KeyCode code) {return false;}
	virtual bool   isKeyDown(KeyCode code) {return false;}
	virtual bool   wasKeyTyped(KeyCode code) {return false;}
	virtual bool   wasKeyReleased(KeyCode code) {return false;}
	virtual void   addInputSignal(InputSignal* s) {}
	virtual void   removeInputSignal(InputSignal* s) {}
	virtual void   addRepaintSignal(RepaintSignal* s) {}
	virtual void   removeRepaintSignal(RepaintSignal* s) {}
	virtual bool   isWithin(int x,int y) {return false;} //in screen space
	virtual Panel* isWithinTraverse(int x,int y) {return 0;}
	virtual void   localToScreen(int& x,int& y) {}
	virtual void   screenToLocal(int& x,int& y) {}
	virtual void   setCursor(Cursor* cursor) {}
	virtual void   setCursor(Scheme::SchemeCursor scu) {}
	virtual Cursor* getCursor() {return 0;}
	virtual void   setMinimumSize(int wide,int tall) {}
	virtual void   getMinimumSize(int& wide,int& tall) {}
	virtual void   requestFocus() {}
	virtual bool   hasFocus() {return false;}
	virtual int    getChildCount() {return 0;}
	virtual Panel* getChild(int index) {return 0;}
	virtual void   setLayout(Layout* layout) {}
	virtual void   invalidateLayout(bool layoutNow) {}
	virtual void   setFocusNavGroup(FocusNavGroup* focusNavGroup) {}
	virtual void   requestFocusPrev() {}
	virtual void   requestFocusNext() {}
	virtual void   addFocusChangeSignal(FocusChangeSignal* s) {}
	virtual bool   isAutoFocusNavEnabled() {return false;}
	virtual void   setAutoFocusNavEnabled(bool state) {}
	virtual void   setBorder(Border* border) {}
	virtual void   setPaintBorderEnabled(bool state) {}
	virtual void   setPaintBackgroundEnabled(bool state) {}
	virtual void   setPaintEnabled(bool state) {}
	virtual void   getInset(int& left,int& top,int& right,int& bottom) {}
	virtual void   getPaintSize(int& wide,int& tall) {}
	virtual void   setPreferredSize(int wide,int tall) {}
	virtual void   getPreferredSize(int& wide,int& tall) {}
	virtual SurfaceBase* getSurfaceBase() {return 0;}
	virtual bool   isEnabled() {return _enabled = false;}
	virtual void   setEnabled(bool state) {_enabled = true;}
	virtual void   setBuildGroup(BuildGroup* buildGroup,const char* panelPersistanceName) {}
	virtual bool   isBuildGroupEnabled() {return false;}
	virtual void   removeAllChildren() {}
	virtual void   repaintParent() {}
	virtual Panel* createPropertyPanel() {return 0;}
	virtual void   getPersistanceText(char* buf,int bufLen) {}
	virtual void   applyPersistanceText(const char* buf) {}
	virtual void   setFgColor(Scheme::SchemeColor sc) {}
	virtual void   setBgColor(Scheme::SchemeColor sc) {}
	virtual void   setFgColor(int r,int g,int b,int a) {}
	virtual void   setBgColor(int r,int g,int b,int a) {}
	virtual void   getFgColor(int& r,int& g,int& b,int& a) {}
	virtual void   getBgColor(int& r,int& g,int& b,int& a) {}
	virtual void   setBgColor(Color color) {}
	virtual void   setFgColor(Color color) {}
	virtual void   getBgColor(Color& color) {}
	virtual void   getFgColor(Color& color) {}
	virtual void   setAsMouseCapture(bool state) {}
	virtual void   setAsMouseArena(bool state) {}
	virtual App*   getApp() {return 0;}
	virtual void   getVirtualSize(int& wide,int& tall) {}
	virtual void   setLayoutInfo(LayoutInfo* layoutInfo) {}
	virtual LayoutInfo* getLayoutInfo() {return 0;}
	virtual bool   isCursorNone() {return false;}
public: //bullshit public
	virtual void solveTraverse() {}
	virtual void paintTraverse() {}
	virtual void setSurfaceBaseTraverse(SurfaceBase* surfaceBase) {}
protected:
	virtual void performLayout() {}
	virtual void internalPerformLayout() {}
	virtual void drawSetColor(Scheme::SchemeColor sc) {}
	virtual void drawSetColor(int r,int g,int b,int a) {}
	virtual void drawFilledRect(int x0,int y0,int x1,int y1) {}
	virtual void drawOutlinedRect(int x0,int y0,int x1,int y1) {}
	virtual void drawSetTextFont(Scheme::SchemeFont sf) {}
	virtual void drawSetTextFont(Font* font) {}
	virtual void drawSetTextColor(Scheme::SchemeColor sc) {}
	virtual void drawSetTextColor(int r,int g,int b,int a) {}
	virtual void drawSetTextPos(int x,int y) {}
	virtual void drawPrintText(const char* str,int strlen) {}
	virtual void drawPrintText(int x,int y,const char* str,int strlen) {}
	virtual void drawPrintChar(char ch) {}
	virtual void drawPrintChar(int x,int y,char ch) {}
	virtual void drawSetTextureRGBA(int id,const char* rgba,int wide,int tall) {}
	virtual void drawSetTexture(int id) {}
	virtual void drawTexturedRect(int x0,int y0,int x1,int y1) {}
	virtual void solve() {}
	virtual void paintTraverse(bool repaint) {if(repaint) paintBackground();}
	virtual void paintBackground() {}
	virtual void paint() {}
	virtual void paintBuildOverlay() {}
	virtual void internalCursorMoved(int x,int y) {}
	virtual void internalCursorEntered() {}
	virtual void internalCursorExited() {}
	virtual void internalMousePressed(MouseCode code) {}
	virtual void internalMouseDoublePressed(MouseCode code) {}
	virtual void internalMouseReleased(MouseCode code) {}
	virtual void internalMouseWheeled(int delta) {}
	virtual void internalKeyPressed(KeyCode code) {}
	virtual void internalKeyTyped(KeyCode code) {}
	virtual void internalKeyReleased(KeyCode code) {}
	virtual void internalKeyFocusTicked() {}
	virtual void internalFocusChanged(bool lost) {}
	virtual void internalSetCursor() {}
protected:
	int               _pos[2];
	int               _size[2];
	int               _loc[2];
	int               _minimumSize[2];
	int               _preferredSize[2];
	Dar<Panel*>       _childDar;
	Panel*            _parent;
	SurfaceBase*      _surfaceBase;
	Dar<InputSignal*> _inputSignalDar;
	Dar<RepaintSignal*> _repaintSignalDar;
	int               _clipRect[4];
	Cursor*           _cursor;
	Scheme::SchemeCursor _schemeCursor;
	bool              _visible;
	Layout*           _layout;
	bool              _needsLayout;
	FocusNavGroup*    _focusNavGroup;
	Dar<FocusChangeSignal*> _focusChangeSignalDar;
	bool              _autoFocusNavEnabled;
	Border*           _border;
private:
	bool                _needsRepaint;
	bool                _enabled;
	BuildGroup*         _buildGroup;
	Color               _fgColor;
	Color               _bgColor;
	LayoutInfo*         _layoutInfo;
	bool                _paintBorderEnabled;
	bool                _paintBackgroundEnabled;
	bool                _paintEnabled;
friend class App;
friend class SurfaceBase;
friend class Image;
};
}

#endif
