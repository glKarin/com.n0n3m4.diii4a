#include "Trigger.h"

class idTrigger_enginewash : public idTrigger_Multi
{
public:
	CLASS_PROTOTYPE(idTrigger_enginewash);

						idTrigger_enginewash();
	virtual				~idTrigger_enginewash(void);
	void				Spawn();
	void				Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void				Restore(idRestoreGame *savefile);
	virtual void		Think(void);

	float				baseAngle;

private:
	void				Event_Touch(idEntity* other, trace_t* trace);
	
	int					timer;
	int					state;
	enum				{ EW_NONE, EW_PUSHING };
	
};