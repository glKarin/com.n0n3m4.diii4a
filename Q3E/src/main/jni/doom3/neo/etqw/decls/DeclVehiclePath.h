// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLVEHICLEPATH_H__
#define __DECLVEHICLEPATH_H__

class sdDeclVehiclePath : public idDecl {
public:
									sdDeclVehiclePath( void );
	virtual							~sdDeclVehiclePath( void );

	virtual const char*				DefaultDefinition( void ) const;
	virtual bool					Parse( const char *text, const int textLength );
	virtual void					FreeData( void );

	void							Set( const idMatX& input ) { assert( input.GetNumColumns() == input.GetNumRows() ); grid = input; RebuildTextSource(); }

	int								GetSize( void ) const { return grid.GetNumColumns(); }
	float							GetPointHeight( int x, int y ) const { return grid[ x ][ y ]; }

	virtual bool					RebuildTextSource( void );
	
private:
	idMatX							grid;
};

#endif // __DECLVEHICLEPATH_H__
