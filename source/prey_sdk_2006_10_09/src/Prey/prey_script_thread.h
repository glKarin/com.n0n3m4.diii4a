#ifndef __HH_SCRIPT_THREAD_H__
#define __HH_SCRIPT_THREAD_H__

class hhThread : public idThread {
	CLASS_PROTOTYPE( hhThread );

	public:
									hhThread();
									hhThread( idEntity *self, const function_t *func );
									hhThread( const function_t *func );
									hhThread( idInterpreter *source, const function_t *func, int args );

		void						PushString( const char *text );
		void						PushFloat( float value );
		void						PushInt( int value );
		void						PushVector( const idVec3 &vec );
		void						PushEntity( const idEntity *ent );
		void						ClearStack();

		bool						ParseAndPushArgsOntoStack( const idCmdArgs& args, const function_t* function );
		bool						ParseAndPushArgsOntoStack( const idList<idStr>& args, const function_t* function );

	protected:
		void						PushParm( int value );
};

#endif