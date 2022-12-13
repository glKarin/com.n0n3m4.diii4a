#ifndef __HH_WEAPON_CRAWLERGRENADE_H
#define __HH_WEAPON_CRAWLERGRENADE_H

/***********************************************************************

  hhWeaponCrawlerGrenade
	
***********************************************************************/
class hhWeaponCrawlerGrenade : public hhWeapon {
	CLASS_PROTOTYPE( hhWeaponCrawlerGrenade );

	public:
		//rww - network friendliness
		virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );

	protected:
		void			Event_SpawnBloodSpray();
};

#endif