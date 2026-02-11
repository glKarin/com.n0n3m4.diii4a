#ifndef __SYS_DEBUGGER_H__
#define __SYS_DEBUGGER_H__

#if INGAME_DEBUGGER_ENABLED

#define DEBUG_MIN_ENT_DIST			75		// Minimum distance to entity for FILTER_DISTANCE
#define DEBUG_NUM_COLS				10		// Number of cols in debug_columns.gui (max supported)
#define DEBUG_NUM_ROWS				200		// Number of rows in debug_columns.gui (max supported)
#define NUM_DISPLAYLIST_COLUMNS		10

class idTypeInfo;

enum EDebugMode {
	DEBUGMODE_NONE=0,
	DEBUGMODE_STATS,
	DEBUGMODE_SPAWNARGS,
	DEBUGMODE_GAMEINFO1,
	DEBUGMODE_GAMEINFO2,
	DEBUGMODE_GAMEINFO3,
	NUM_DEBUGMODES
};

enum EColumnDataType {
	COLUMNTYPE_ALPHA,
	COLUMNTYPE_NUMERIC
};

// Filter bitmasks
#define FILTER_NONE				0
#define FILTER_CLASS			1
#define FILTER_ACTIVE			2
#define FILTER_DISTANT			3
#define FILTER_MONSTERS			4
#define FILTER_ACTIVESOUNDS		5
#define FILTER_HASMODEL			6
#define FILTER_ANIMATING		7
#define FILTER_EXPENSIVE		8
#define FILTER_VISIBLE			9
#define FILTER_CLOSE			10
#define FILTER_DORMANT			11
#define NUM_FILTERS				12

#define FILTERSTATE_NONE		0
#define	FILTERSTATE_INCLUDED	1
#define FILTERSTATE_EXCLUDED	2

class hhDisplayCell {
public:
							hhDisplayCell();
							~hhDisplayCell();

	inline void				operator=( const idStr &text );
	inline void				operator=( const char *text );
	inline void				operator=( const int i );
	inline void				operator=( const float f );
	inline void				operator=( const hhDisplayCell &cell );

	inline hhDisplayCell &	operator+=( const int i );
	inline hhDisplayCell &	operator+=( const float f );

	inline bool				IsInt() const;
	inline int				StringLength();
	inline EColumnDataType	Type() const;
	inline float			Float() const;
	inline int				Int() const;
	const char *			String() const;

private:
	inline void				AllocateString();

	idStr					*string;
	EColumnDataType			type;
	float					value;
};



class hhDisplayItem {
public:
	hhDisplayCell column[NUM_DISPLAYLIST_COLUMNS];
	friend bool operator==( const hhDisplayItem &a, const hhDisplayItem &b ) {
		return !idStr::Icmp(a.column[0].String(), b.column[0].String());
	};
	// idList<>::Append() uses operator= on it's objects, so we need to override the
	// default memcpy() functionality, with something that constructs a new idStr in hhDisplayCell
	// Force idList<> to copy each element individually, so hhDisplayCell::operator=() is used
	inline void operator=( const hhDisplayItem &item ) {
		for (int ix=0; ix<NUM_DISPLAYLIST_COLUMNS; ix++) {
			column[ix] = item.column[ix];
		}
	}
};


class hhDebugger {
public:
							hhDebugger();
							~hhDebugger();
	void					Shutdown();
	void					UpdateDebugger();
	bool					IsInteractive()	{ return bInteractive; }
	void					Reset();

	// These need public access for sort routines
	static int				sortColumn;					// Column of data to sort upon
	static bool				sortAscending;				// Sort direction

	// Data accessors
	void					SetMode(EDebugMode mode);
	EDebugMode				GetMode()						{	return debugMode;		}
	void					SetClassCollapse(bool on);
	bool					GetClassCollapse()				{	return bClassCollapse;	}
	void					SetDrawEntities(bool on);
	bool					GetDrawEntities()				{	return bDrawEntities;	}
	void					SetPointer(bool on);
	bool					GetPointer()					{	return bPointer;		}
	void					SetSelectedEntity(idEntity *ent);

private:
	void					HandleFrameEvents();
	bool					HandleGuiCommands(const char *commands);
	bool					HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);
	void					TranslateRowCommand(int row);
	void					CaptureInput(bool bCapture);
	void					DeterminePotentialSet();
	void					DetermineSelectionSet();
	void					UpdateGUI_Entity();
	void					DrawPotentialSet();
	void					DrawSelectionSet();
	void					FillGUI();
	void					DrawGUI();
	void					Initialize();

	void					FillDisplayList_Stats();
	void					FillDisplayList_SpawnArgs();
	void					FillDisplayList_GameInfo(int page);

	void					SetSortToColumn(int col);
	idTypeInfo				*ClassForRow(int row);
	idEntity				*EntityForRow(int row);
	idVec4					ColorForIndex(int index, float alpha=1.0f);
	idVec4					ColorForEntity(idEntity *ent, float alpha=1.0f);

	int						IndexForFilter(int filter);
	int						FilterForIndex(int index);
	bool					FilterIncluded(int filterIndex);
	bool					FilterExcluded(int filterIndex);
	bool					FilterIsActive(int filterIndex)		{	return (FilterIncluded(filterIndex) || FilterExcluded(filterIndex));	}
	void					IncludeFilter(int filterIndex);
	void					ExcludeFilter(int filterIndex);
	void					ClearFilter(int filterIndex);
	void					ClearAllFilters();
	void					Toggle3wayFilter(int filterIndex);

private:
	// Data
	bool					bInitialized;		// Whether the system is inited
	bool					bClassCollapse;		// Whether to collapse entities into their classes
	bool					bDrawEntities;		// Whether to draw potential set
	bool					bPointer;			// Whether the selection pointer is currently active
	bool					bInteractive;		// Whether input is currently captured
	idEntityPtr<idEntity>	selectedEntity;		// Entity currently selected by GUI choice or pointer

	idVec3					xaxis;
	idVec3					yaxis;
	idVec3					zaxis;

	idTypeInfo				*filterClass;		// Class to be used when FILTER_CLASS is active
	int						includeFilters;		// Bitmask of current inclusion filters used to cull potentialSet
	int						excludeFilters;		// Bitmask of current exclusion filters used to cull potentialSet

	idList<idEntity *>		potentialSet;		// All entities that pass the filters
	idList<hhDisplayItem>	displayList;
	int						displayColumnsUsed;	// Number of columns of display list actually used

	EDebugMode				debugMode;			// Mode for debugger display
	idUserInterface			*gui;
	idDict					dict;				// Scratch pad dictionary, needs to be persistent over
												// the time of UpdateDebugger()
};


extern hhDebugger debugger;

#endif	// INGAME_DEBUGGER_ENABLED

#endif	// __SYS_DEBUGGER_H__
