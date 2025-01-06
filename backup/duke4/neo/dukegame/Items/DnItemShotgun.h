// DnItemShotgun.h
//

//
// DnItemShotgun
//
class DnItemShotgun : public DnItem
{
	CLASS_PROTOTYPE(DnItemShotgun);

protected:
	virtual void  TouchEvent(DukePlayer* player, trace_t* trace) override;
};