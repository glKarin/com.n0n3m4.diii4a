/*
=======================================================================

DYANMIC MENU HEADER

=======================================================================
*/


#define PT_FRIENDLY 			1
#define PT_ENEMY 				2
#define PT_BOTONLY				4
#define PT_PLAYERONLY			8
#define PT_EXCLUDEPARENT		16
#define PT_EXCLUDEGRANDPARENT	32
#define PT_ALL					64




typedef void (*createHandler)(void);
typedef void (*eventHandler)(int index);


// manipulate menu
qboolean DynamicMenu_AddItem( const char* string, int id, createHandler crh, eventHandler evh);
qboolean DynamicMenu_AddIconItem( const char* string, int id, const char* icon, createHandler crh, eventHandler evh);
void DynamicMenu_AddBackground( const char* background);

qboolean DynamicMenu_SubMenuInit( void);
void DynamicMenu_FinishSubMenuInit( void );
void DynamicMenu_MenuInit( qboolean fullscreen, qboolean wraparound );

void DynamicMenu_ClearFocus( int pos );
void DynamicMenu_SetFocus( int pos );

void DynamicMenu_SetFlags(int depth, int id, int flags);

int DynamicMenu_ServerGametype(void);

//
// information about the menu structure
//

// general
int DynamicMenu_Depth(void);

// takes an index and returns data
qboolean DynamicMenu_OnActiveList( int index );
const char* DynamicMenu_StringAtIndex(int index);
int DynamicMenu_DepthOfIndex( int index );
int DynamicMenu_IdAtIndex(int index);

// takes a depth and returns data
int DynamicMenu_ActiveIdAtDepth(int depth);
int DynamicMenu_ActiveIndexAtDepth(int depth);


// returns data about items
const char* DynamicMenu_ItemShortname(int index);



// core services
void DynamicMenu_AddListOfPlayers( int type, createHandler crh, eventHandler evh );
void DynamicMenu_AddListOfItems( int exclude, createHandler crh, eventHandler evh );

