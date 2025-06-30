/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

// set new thumbnail
void SetThumbnail(CTFileName fn);
// remove thumbnail
void ClearThumbnail(void);

void InitializeMenus( void);
void ReInitializeMenus(void);
void DestroyMenus( void);
void MenuOnKeyDown( int iVKey);
void MenuOnChar(MSG msg);
void MenuOnMouseMove(PIX pixI, PIX pixJ);
void MenuOnLMBDown(void);
BOOL DoMenu( CDrawPort *pdp); // returns TRUE if still active, FALSE if should quit
void StartMenus( const char *str="");
void StopMenus(BOOL bGoToRoot =TRUE);
BOOL IsMenusInRoot(void);
void ChangeToMenu( class CGameMenu *pgmNew);
extern void PlayMenuSound(CSoundData *psd);

#define KEYS_ON_SCREEN 14
#define LEVELS_ON_SCREEN 16
#define SERVERS_ON_SCREEN 15
#define VARS_ON_SCREEN 14

extern CListHead _lhServers;

extern class CPlayerProfileMenu gmPlayerProfile;
extern class CSelectPlayersMenu gmSelectPlayersMenu;
extern class CCustomizeAxisMenu gmCustomizeAxisMenu;
extern INDEX _iLocalPlayer;

enum GameMode {
  GM_NONE = 0,
  GM_SINGLE_PLAYER,
  GM_NETWORK,
  GM_SPLIT_SCREEN,
  GM_DEMO,
  GM_INTRO,
};
extern GameMode _gmMenuGameMode;
extern GameMode _gmRunningGameMode;

extern CGameMenu *pgmCurrentMenu;

class CGameMenu {
public:
  CListHead gm_lhGadgets;
  CGameMenu *gm_pgmParentMenu;
  BOOL gm_bPopup;
  const char *gm_strName;   // menu name (for mod interface only)
  class CMenuGadget *gm_pmgSelectedByDefault;
  class CMenuGadget *gm_pmgArrowUp;
  class CMenuGadget *gm_pmgArrowDn;
  class CMenuGadget *gm_pmgListTop;
  class CMenuGadget *gm_pmgListBottom;
  INDEX gm_iListOffset;
  INDEX gm_iListWantedItem;   // item you want to focus initially
  INDEX gm_ctListVisible;
  INDEX gm_ctListTotal;
  CGameMenu(void);
  void ScrollList(INDEX iDir);
  void KillAllFocuses(void);
  virtual void Initialize_t(void);
  virtual void Destroy(void);
  virtual void StartMenu(void);
  virtual void FillListItems(void);
  virtual void EndMenu(void);
  // return TRUE if handled
  virtual BOOL OnKeyDown( int iVKey);
  virtual BOOL OnChar(MSG msg);
  virtual void Think(void);
};

class CConfirmMenu : public CGameMenu {
public:
  void Initialize_t(void);
  // return TRUE if handled
  BOOL OnKeyDown( int iVKey);

  void BeLarge(void);
  void BeSmall(void);
};

class CMainMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
};

class CInGameMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
};

class CSinglePlayerMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
};

class CCreditsMenu : public CGameMenu {
public:
  void Initialize_t(void);
};

class CDisabledMenu : public CGameMenu {
public:
  void Initialize_t(void);
};

class CSinglePlayerNewMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
};

class CLevelsMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void FillListItems(void);
  void StartMenu(void);
};

class CVarMenu : public CGameMenu {
public:
  CTFileName gm_fnmMenuCFG;
  void Initialize_t(void);
  void FillListItems(void);
  void StartMenu(void);
  void EndMenu(void);
  void Think(void);
};

class CServersMenu : public CGameMenu {
public:
  BOOL m_bInternet;
  void Initialize_t(void);
  void StartMenu(void);
  void Think(void);
};

class CPlayerProfileMenu : public CGameMenu {
public:
  INDEX *gm_piCurrentPlayer;
  void Initialize_t(void);
  INDEX ComboFromPlayer(INDEX iPlayer);
  INDEX PlayerFromCombo(INDEX iCombo);
  void SelectPlayer(INDEX iPlayer);
  void ApplyComboPlayer(INDEX iPlayer);
  void StartMenu(void);
  void EndMenu(void);
};

class CControlsMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
  void EndMenu(void);
  void ObtainActionSettings(void);
  void ApplyActionSettings(void);
};

class CFileInfo {
public:
  CListNode fi_lnNode;
  CTFileName fi_fnFile;
  CTString fi_strName;
};

class CLoadSaveMenu : public CGameMenu {
public:
  // settings adjusted before starting the menu
  CGameMenu *gm_pgmNextMenu;  // menu to go to after selecting a file (if null, use parent menu)
  CTFileName gm_fnmSelected;  // file that is selected initially
  CTFileName gm_fnmDirectory; // directory that should be read
  CTFileName gm_fnmBaseName;  // base file name for saving (numbers are auto-added)
  CTFileName gm_fnmExt;       // accepted file extension
  BOOL gm_bSave;              // set when chosing file for saving
  BOOL gm_bManage;            // set if managing (rename/delet is enabled)
  CTString gm_strSaveDes;     // default description (if saving)
  BOOL gm_bAllowThumbnails;   // set when chosing file for saving
  BOOL gm_bNoEscape;          // forbid exiting with escape/rmb
#define LSSORT_NONE     0
#define LSSORT_NAMEUP   1
#define LSSORT_NAMEDN   2
#define LSSORT_FILEUP   3
#define LSSORT_FILEDN   4
  INDEX gm_iSortType;    // sort type

  // function to activate when file is chosen
  // return true if saving succeeded - description is saved automatically
  // always return true for loading
  BOOL (*gm_pAfterFileChosen)(const CTFileName &fnm);

  // internal properties
  CListHead gm_lhFileInfos;   // all file infos to list
  INDEX gm_iLastFile;         // index of last saved file in numbered format

  // called to get info of a file from directory, or to skip it
  BOOL ParseFile(const CTFileName &fnm, CTString &strName);

  void Initialize_t(void);
  void StartMenu(void);
  void EndMenu(void);
  void FillListItems(void);
};

extern CLoadSaveMenu gmLoadSaveMenu;

class CHighScoreMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
};

class CNetworkMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
};

class CNetworkStartMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
  void EndMenu(void);
};

class CNetworkOpenMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
  void EndMenu(void);
};

class CNetworkJoinMenu : public CGameMenu {
public:
  void Initialize_t(void);
};

class CSplitScreenMenu: public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
};

class CSplitStartMenu: public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
  void EndMenu(void);
};

class CSelectPlayersMenu: public CGameMenu {
public:
  BOOL gm_bAllowDedicated;
  BOOL gm_bAllowObserving;
  void Initialize_t(void);
  void StartMenu(void);
  void EndMenu(void);
};

class COptionsMenu : public CGameMenu {
public:
  void Initialize_t(void);
};

class CVideoOptionsMenu : public CGameMenu {
public:
  void StartMenu(void);
  void Initialize_t(void);
};

class CRenderingOptionsMenu : public CGameMenu {
public:
  void StartMenu(void);
  void EndMenu(void);
  void Initialize_t(void);
};

class CAudioOptionsMenu : public CGameMenu {
public:
  void StartMenu(void);
  void Initialize_t(void);
};

class CCustomizeKeyboardMenu : public CGameMenu {
public:
  void Initialize_t(void);
  void StartMenu(void);
  void EndMenu(void);
  void FillListItems(void);
};

class CCustomizeAxisMenu : public CGameMenu {
public:
  ~CCustomizeAxisMenu(void);
  void Initialize_t(void);
  void StartMenu(void);
  void EndMenu(void);
  void ObtainActionSettings(void);
  void ApplyActionSettings(void);
};

