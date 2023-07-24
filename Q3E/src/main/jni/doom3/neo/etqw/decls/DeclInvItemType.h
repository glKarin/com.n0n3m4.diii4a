// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLINVITEMTYPE_H__
#define __DECLINVITEMTYPE_H__

class sdDeclInvItemType : public idDecl {
public:
							sdDeclInvItemType( void );
	virtual					~sdDeclInvItemType( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	bool					IsVehicleEquipable( void ) const { return flags.vehicleEquipable; }
	bool					IsEquipable( void ) const { return flags.equipable; }

protected:
	typedef struct typeFlags_s {
		bool				equipable			: 1;
		bool				vehicleEquipable	: 1;
	} typeFlags_t;

	typeFlags_t				flags;
};

#endif // __DECLINVITEMTYPE_H__

