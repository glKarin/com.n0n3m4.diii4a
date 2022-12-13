// Game_note.cpp
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


CLASS_DECLARATION(hhConsole, hhNote)
END_CLASS


void hhNote::Spawn(void) {

	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateString("guitext", spawnArgs.GetString("text", "no text"));

		// For overriding the text that is defined in the Map file.  Used when we put a note
		// in as a placeholder for obsolete items.  The text key may already be used (consoles),
		// so use this to override the text.
		idStr override;
		if (spawnArgs.GetString("textoverride", "", override)) {
			renderEntity.gui[0]->SetStateString("guitext", override.c_str());
		}
	}
}


