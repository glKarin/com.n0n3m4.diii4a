#include "Misc.h"
#include "Entity.h"


class idTutorialPrompt : public idEntity
{
public:
	CLASS_PROTOTYPE(idTutorialPrompt);

							idTutorialPrompt(void);
	virtual					~idTutorialPrompt(void);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	//virtual void			Think(void);

private:

	void					SetText(const char* text, const char* keys);
	void					DoPopup(const char* text, const char* keys);
	bool					promptActive;
	idStr					currentText;

	

};
//#pragma once