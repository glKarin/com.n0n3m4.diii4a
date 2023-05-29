#ifndef __HH_ITEM_CABINET_H
#define __HH_ITEM_CABINET_H

/***********************************************************************

hhItemCabinet

***********************************************************************/
#define CABINET_MAX_ITEMS	3

class hhItemCabinet: public hhAnimatedEntity {
	CLASS_PROTOTYPE( hhItemCabinet );
	
public:
							hhItemCabinet();
	virtual					~hhItemCabinet();

	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	
protected:
	void					InitBoneInfo();

	void					PlayAnim( const char* pAnimName, int iBlendTime ); 
	void					PlayAnim( int pAnim, int iBlendTime ); 

	void					ResetItemList();
	void					AddItem( idStr itemName, int slot );
	bool					SpawnDefaultItems();
	void					SpawnItems();
	void					SpawnIdleFX();

	bool					HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );

protected:
	void					Event_AppendFxToIdleList( hhEntityFx* fx );
	void					Event_Activate( idEntity* pActivator );
	void					Event_EnableItemClip();
	virtual void			Event_PostSpawn(void);

protected:
	int						animDoneTime;

	idList< idEntityPtr<idEntityFx> >	idleFxList;

	idEntityPtr<hhItem>		itemList[ CABINET_MAX_ITEMS ];
	idStr					boneList[ CABINET_MAX_ITEMS ];
};

#endif