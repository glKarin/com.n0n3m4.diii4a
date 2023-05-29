#ifndef __HH_WOUND_MANAGER_RENDERENTITY_H
#define __HH_WOUND_MANAGER_RENDERENTITY_H

class hhWoundManagerRenderEntity {
	public:
								hhWoundManagerRenderEntity( const idEntity* self );
		virtual					~hhWoundManagerRenderEntity();

		//Main entry point
		void					AddWounds( const idDeclEntityDef *def, surfTypes_t matterType, jointHandle_t jointNum, const idVec3& origin, const idVec3& normal, const idVec3& dir );
		virtual void			DetermineWoundInfo( const trace_t& trace, const idVec3 velocity, jointHandle_t& jointNum, idVec3& origin, idVec3& normal, idVec3& dir );

	protected:
		virtual void			ApplyWound( const idDict& damageDict, const char* woundKey, jointHandle_t jointNum, const idVec3 &origin, const idVec3 &normal, const idVec3 &dir );
		virtual void			ApplyMark( const idDict& damageDict, const char* impactMarkKey, const idVec3 &origin, const idVec3 &normal, const idVec3 &dir );
		virtual void			ApplySplatExit( const idDict& damageDict, const char* impactMarkKey, const idVec3 &origin, const idVec3 &normal, const idVec3 &dir );

	protected:
		idEntityPtr<idEntity>	self;
};

class hhWoundManagerAnimatedEntity : public hhWoundManagerRenderEntity {
	public:
								hhWoundManagerAnimatedEntity( const idEntity* self );
		virtual					~hhWoundManagerAnimatedEntity();

	protected:

		virtual void			ApplyMark( const idDict& damageDict, const char* impactMarkKey, const idVec3 &origin, const idVec3 &normal, const idVec3 &dir );
		virtual void			ApplyWound( const idDict& damageDict, const char* woundKey, jointHandle_t jointNum,  const idVec3 &origin, const idVec3 &normal, const idVec3 &dir );

};

#endif