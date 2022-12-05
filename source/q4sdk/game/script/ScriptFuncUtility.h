#ifndef __RV_SCRIPT_FUNC_UTILITY_H
#define __RV_SCRIPT_FUNC_UTILITY_H

class idThread;

enum sfuReturnType {
	SFU_NOFUNC = -1,
	SFU_ERROR = 0,
	SFU_OK = 1
};

class rvScriptFuncUtility {
public:
	rvScriptFuncUtility();
	explicit rvScriptFuncUtility( const rvScriptFuncUtility* sfu );
	explicit rvScriptFuncUtility( const rvScriptFuncUtility& sfu );
	explicit rvScriptFuncUtility( const char* source );
	explicit rvScriptFuncUtility( const idCmdArgs& args );

	sfuReturnType			Init( const char* source );
	sfuReturnType			Init( const idCmdArgs& args );
	void					Clear();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	idTypeDef*				GetParmType( int index ) const;
	idTypeDef*				GetReturnType() const ;
	int						NumParms() const;
	bool					ReturnsAVal() const;

	void					SetFunction( const function_t* func );
	void					SetParms( const idList<idStr>& parms );
	void					SetReturnKey( const char* key ) { returnKey = key; }

	const char*				GetFuncName() const;
	const function_t*		GetFunc() const { return func; }
	const char*				GetParm( int index ) const;
	const char*				GetReturnKey() const { return returnKey.c_str(); }

	void					InsertInt( int parm, int index );
	void					InsertFloat( float parm, int index );
	void					InsertVec3( const idVec3& parm, int index );
	void					InsertEntity( const idEntity* parm, int index );
	void					InsertString( const char* parm, int index );
	void					InsertBool( bool parm, int index );
	void					RemoveIndex( int index );

	const function_t*		FindFunction( const char* name ) const;
	void					CallFunc( idDict* returnDict ) const;

	bool					Valid() const;

	rvScriptFuncUtility&	Assign( const rvScriptFuncUtility* sfu );
	rvScriptFuncUtility&	operator=( const rvScriptFuncUtility* sfu );
	rvScriptFuncUtility&	operator=( const rvScriptFuncUtility& sfu );

	bool					IsEqualTo( const rvScriptFuncUtility* sfu ) const;
	bool					operator==( const rvScriptFuncUtility* sfu ) const;
	bool					operator==( const rvScriptFuncUtility& sfu ) const;

private:
	sfuReturnType			Init();

protected:
	const function_t*		func;
	idList<idStr>			parms;
	idStr					returnKey;
};

#endif
