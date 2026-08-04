/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __DARKMOD_OVERLAYSYS_H__
#define __DARKMOD_OVERLAYSYS_H__

const int OVERLAYS_MIN_HANDLE = 1;
const int OVERLAYS_INVALID_HANDLE = 0;

/**
* These is the layer order of the overlay for the player GUI.
*/
enum
{
	LAYER_UNDERWATER = 0,	// Draw the underwater overlay first
	LAYER_MAIN_HUD = 1,
	LAYER_INVENTORY = 2,
 	LAYER_INVGRID = 3,		// #4286
	LAYER_OBJECTIVES = 12,
	LAYER_WAITUNTILREADY = 13,
	LAYER_SUBTITLES = 20,	// stgatilov #2454
};

struct SOverlay;

/// Container class to keep track of multiple GUIs.
/**	An overlay system is used to keep track of an arbitrary number of GUIs.
 *	The overlay system consists of zero or more 'overlays'. An overlay
 *	contains a GUI and some bookkeeping information. Each overlay has a
 *	'layer' which is used to sort the order the overlays are drawn in.
 *	Overlays in higher layers are drawn on top of overlays in lower layers.
 *	Multiple overlays may exist in the same layer, but their drawing order
 *	with respect to eachother is undefined.
 *	
 *	An overlay may be opaque. If/when the overlay system is rendered directly
 *	to screen, this overlay is assumed to have no transparent sections, and
 *	nothing underneath it will be rendered. This has no effect on GUIs that
 *	are in the game-world.
 *	
 *	An overlay may be interactive. If the overlay system is asked which GUI
 *	interactivity should be routed to, it will return the GUI in the highest
 *	interactive overlay. This has no effect on GUIs that are in the
 *	game-world.
 *	
 *	An overlay may be external, meaning that its GUI points to a GUI defined
 *	externally. Note that although external overlays are saved and restored,
 *	their GUI pointer isn't, and needs to be set again.
 *	
 *	I'm assuming the overlay system will only be used to contain a few
 *	overlays at a time, so I've opted for simpler data structures and easier
 *	code, rather than trying to make the most efficient system possible.
 *	However, certain easy optimizatons have been made; although accessing a
 *	different overlay from last time is O(N) with respect to the number of
 *	overlays contained, repeatedly accessing the same overlay is very fast.
 *	Similarly, results relating to which layers are opaque/interactive are
 *	cached.
 */
class COverlaySys
{
  public:

	COverlaySys();
	~COverlaySys();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/// Draws the contained GUIs to the screen, in order.
	/// if onlyOverlayHandles is specified, all UIs not in the list are skipped
	void					drawOverlays( idList<int> *onlyOverlayHandles = nullptr );
	/// Returns true if any of the GUIs are opaque.
	bool					isOpaque();
	/// Returns the interactive GUI.
	idUserInterface*		findInteractive();
	/// Used for iterating through the overlays in drawing-order.
	/**	Very efficient if used properly:
	 *	    int h = OVERLAYS_INVALID_HANDLE;
	 *	    while ( (h = o.getNextOverlay(h) ) != OVERLAYS_INVALID_HANDLE )
	 *	        o.doSomethingWith( h );
	 *	It loses efficiency if a handle is accessed other than the one returned.
	 */
	int						getNextOverlay( int handle );

	/// Create a new overlay, returning a handle for that overlay.
	int						createOverlay( int layer, int handle = OVERLAYS_INVALID_HANDLE );
	/// Destroy an overlay.
	void					destroyOverlay( int handle );
	/// Returns whether or not an overlay exists.
	bool					exists( int handle );
	/// Sets the overlay's GUI to an external GUI.
	void					setGui( int handle, idUserInterface* gui );
	/// Sets the overlay's GUI to an internal, unique one.
	bool					setGui( int handle, const char* file );
	/// Return an overlay's GUI.
    idUserInterface*		getGui( int handle );
	/// Change an overlay's layer.
	void					setLayer( int handle, int layer );
	/// Return an overlay's layer.
	int						getLayer( int handle );
	/// Return whether or not an overlay is external.
	bool					isExternal( int handle );
	/// Change whether or not an overlay is considered opaque.
	void					setOpaque( int handle, bool isOpaque );
	/// Return whether or not an overlay is considered opaque.
	bool					isOpaque( int handle );
	/// Change whether or not an overlay is considered interactive.
	void					setInteractive( int handle, bool isInteractive );
	/// Return whether or not an overlay is considered interactive.
	bool					isInteractive( int handle );

	/**
	 * greebo: This cycles through all overlays and calls HandleNamedEvent() 
	 *		   on each visible GUI.
	 */
	void					broadcastNamedEvent(const char* eventName);

	/**
	 * greebo: Use these methods to set the state variables of ALL active overlays.
	 *         This is similar to the broadcastNamedEvent, but these routines only set
	 *         a GUI state variable (e.g. "gui::HUD_Opacity").
	 */
	void					setGlobalStateString(const char* varName, const char *value);
	void					setGlobalStateBool(const char* varName, const bool value);
	void					setGlobalStateInt(const char* varName, const int value);
	void					setGlobalStateFloat(const char* varName, const float value);

  private:

	/// Returns the overlay associated with a handle.
	SOverlay*				findOverlay( int handle, bool updateCache = true );
	/// Returns the highest opaque overlay.
	idLinkList<SOverlay>*	findOpaque();

	/// The list of overlays.
	idLinkList<SOverlay>	m_overlays;

	/// The last handle accessed.
	int						m_lastUsedHandle;
	/// The overlay of the last handle accessed.
	SOverlay*				m_lastUsedOverlay;

	/// Whether or not the highest opaque overlay needs to be recalculated.
	bool					m_updateOpaque;
	/// The cached value of the highest opaque overlay.
	idLinkList<SOverlay>*	m_highestOpaque;

	/// Whether or not the interactive overlay needs to be recalculated.
	bool					m_updateInteractive;
	/// The cached value of the interactive overlay.
	idUserInterface*		m_highestInteractive;

	/// This is the next handle to try out when creating a new overlay.
	int						m_nextHandle;
};

struct SOverlay
{
	idLinkList<SOverlay>	m_node;
	idUserInterface*		m_gui;
	int						m_handle;
	int						m_layer;
	bool					m_external;
	bool					m_opaque;
	bool					m_interactive;
};

#endif /* __DARKMOD_OVERLAYSYS_H__ */
