/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#define DOING_NOTHING 0
#define PRESS_KEY_WAITING 1
#define RELEASE_RETURN_WAITING 2

#define EMPTYSLOTSTRING TRANS("<save a new one>")

class CMenuGadget {
public:
  CListNode mg_lnNode;
  FLOATaabbox2D mg_boxOnScreen;
  BOOL mg_bVisible;
  BOOL mg_bEnabled;
  BOOL mg_bLabel;
  BOOL mg_bFocused;
  INDEX mg_iInList; // for scrollable gadget lists
  
  CTString mg_strTip;
  CMenuGadget *mg_pmgLeft;
  CMenuGadget *mg_pmgRight;
  CMenuGadget *mg_pmgUp;
  CMenuGadget *mg_pmgDown;

  CMenuGadget( void);
  // return TRUE if handled
  virtual BOOL OnKeyDown( int iVKey);
  virtual BOOL OnChar(MSG msg);
  virtual void OnActivate( void);
  virtual void OnSetFocus( void);
  virtual void OnKillFocus( void);
  virtual void Appear( void);
  virtual void Disappear( void);
  virtual void Think( void);
  virtual void OnMouseOver(PIX pixI, PIX pixJ);

  virtual COLOR GetCurrentColor(void);
  virtual void  Render( CDrawPort *pdp);
  virtual BOOL  IsSeparator(void) { return FALSE; };
};

enum ButtonFontSize {
  BFS_SMALL = 0,
  BFS_MEDIUM = 1,
  BFS_LARGE  = 2,
};

class CMGTitle : public CMenuGadget {
public:
  CTString mg_strText;
  void Render( CDrawPort *pdp);
};

class CMGHighScore : public CMenuGadget {
public:
  void Render( CDrawPort *pdp);
};

class CMGButton : public CMenuGadget {
public:
  CTString mg_strLabel;   // for those that have labels separately from main text
  CTString mg_strText;
  INDEX mg_iCenterI;
  enum  ButtonFontSize mg_bfsFontSize;
  BOOL  mg_bEditing;
  BOOL  mg_bHighlighted;
  BOOL  mg_bRectangle;
  BOOL  mg_bMental;
  INDEX mg_iTextMode;
  INDEX mg_iCursorPos;

  INDEX mg_iIndex;
  void (*mg_pActivatedFunction)(void);
  CMGButton(void);
  void SetText( CTString strNew);
  void OnActivate(void);
  void Render( CDrawPort *pdp);
  PIX  GetCharOffset( CDrawPort *pdp, INDEX iCharNo);
};

enum ArrowDir {
  AD_NONE,
  AD_UP,
  AD_DOWN,
  AD_LEFT,
  AD_RIGHT,
};

class CMGArrow : public CMGButton {
public:
  enum ArrowDir mg_adDirection;
  void Render( CDrawPort *pdp);
  void OnActivate( void);
};

class CMGModel : public CMGButton {
public:
  CModelObject mg_moModel;
  CModelObject mg_moFloor;
  CPlacement3D mg_plModel;
  BOOL mg_fFloorY;

  CMGModel(void);
  void Render( CDrawPort *pdp);
};

class CMGEdit : public CMGButton {
public:
  INDEX mg_ctMaxStringLen;
  CTString *mg_pstrToChange;
  CMGEdit(void);
  // return TRUE if handled
  BOOL OnKeyDown(int iVKey);
  BOOL OnChar(MSG msg);
  void Clear(void);
  void OnActivate(void);
  void OnKillFocus(void);
  void Render( CDrawPort *pdp);
  virtual void OnStringChanged(void);
  virtual void OnStringCanceled(void);
};

class CMGKeyDefinition : public CMenuGadget {
public:
  INDEX mg_iState;
  INDEX mg_iControlNumber;

  CTString mg_strLabel;
  CTString mg_strBinding;

  CMGKeyDefinition(void);
  void Appear(void);
  void Disappear(void);
  void OnActivate(void);
  // return TRUE if handled
  BOOL OnKeyDown( int iVKey);
  void Think( void);
  // set names for both key bindings
  void SetBindingNames(BOOL bDefining);
  void DefineKey(INDEX iDik);
  void Render( CDrawPort *pdp);
};

class CMGTrigger : public CMenuGadget {
public:
  CTString mg_strLabel;
  CTString mg_strValue;
  CTString *mg_astrTexts;
  INDEX mg_ctTexts;
  INDEX mg_iSelected;
  INDEX mg_iCenterI;
  BOOL mg_bVisual;

  CMGTrigger(void);
  
  void ApplyCurrentSelection(void);
  void OnSetNextInList(int iVKey);
  void (*mg_pPreTriggerChange)(INDEX iCurrentlySelected);
  void (*mg_pOnTriggerChange)(INDEX iCurrentlySelected);
  
  // return TRUE if handled
  BOOL OnKeyDown( int iVKey);
  void Render( CDrawPort *pdp);
};

class CMGSlider : public CMGButton {
public:
  FLOAT mg_fFactor;
  INDEX mg_iMinPos;
  INDEX mg_iMaxPos;
  INDEX mg_iCurPos;

  CMGSlider();
  void ApplyCurrentPosition(void);
  void ApplyGivenPosition(INDEX iMin, INDEX iMax, INDEX iCur);
  // return TRUE if handled
  virtual BOOL OnKeyDown( int iVKey);
  void (*mg_pOnSliderChange)(INDEX iCurPos);
  PIXaabbox2D GetSliderBox(void);
  void Render( CDrawPort *pdp);
};                    

class CMGLevelButton : public CMGButton {
public:
  CTFileName mg_fnmLevel;

  void OnActivate(void);
  void OnSetFocus( void);
};

class CMGVarButton : public CMGButton {
public:
  class CVarSetting *mg_pvsVar;
  PIXaabbox2D GetSliderBox(void);
  BOOL OnKeyDown(int iVKey);
  void Render( CDrawPort *pdp);
  BOOL IsSeparator(void);
  BOOL IsEnabled(void);
};

// file button states
#define FBS_NORMAL    0 // normal active state
#define FBS_SAVENAME  1 // typing in the save name
#define FBS_RENAME    2 // renaming existing file
class CMGFileButton : public CMGEdit {
public:
  CMGFileButton(void);
  CTFileName mg_fnm;
  CTString mg_strDes;   // entire description goes here
  CTString mg_strInfo;  // info part of text to print above the gadget tip
  INDEX mg_iState;
  // refresh current text from description
  void RefreshText(void);
  // save description to disk
  void SaveDescription(void);
  void SaveYes(void);
  void DoSave(void);
  void DoLoad(void);
  void StartEdit(void);

  // return TRUE if handled
  BOOL OnKeyDown(int iVKey);
  void OnActivate(void);
  void OnSetFocus(void);
  void OnKillFocus(void);

  // overrides from edit gadget
  void OnStringChanged(void);
  void OnStringCanceled(void);
  void Render( CDrawPort *pdp);
};

class CMGServerList : public CMGButton {
public:
  INDEX mg_iSelected;
  INDEX mg_iFirstOnScreen;
  INDEX mg_ctOnScreen;
  // server list dimensions
  PIX mg_pixMinI;     
  PIX mg_pixMaxI;
  PIX mg_pixListMinJ;
  PIX mg_pixListStepJ;
  // header dimensions
  PIX mg_pixHeaderMinJ;
  PIX mg_pixHeaderMidJ;
  PIX mg_pixHeaderMaxJ;
  PIX mg_pixHeaderI[8];
  // scrollbar dimensions
  PIX mg_pixSBMinI;
  PIX mg_pixSBMaxI;
  PIX mg_pixSBMinJ;
  PIX mg_pixSBMaxJ;
  // scrollbar dragging params
  PIX mg_pixDragJ;
  PIX mg_iDragLine;
  PIX mg_pixMouseDrag;
  // current mouse pos
  PIX mg_pixMouseI;
  PIX mg_pixMouseJ;

  INDEX mg_iSort;     // column to sort by
  BOOL mg_bSortDown;  // sort in reverse order

  CMGServerList();
  BOOL OnKeyDown(int iVKey);
  PIXaabbox2D GetScrollBarFullBox(void);
  PIXaabbox2D GetScrollBarHandleBox(void);
  void OnSetFocus(void);
  void OnKillFocus(void);
  void Render( CDrawPort *pdp);
  void AdjustFirstOnScreen(void);
  void OnMouseOver(PIX pixI, PIX pixJ);
};

class CMGChangePlayer : public CMGButton {
public:
  INDEX mg_iLocalPlayer;

  void SetPlayerText(void);
  void OnActivate( void);
};
