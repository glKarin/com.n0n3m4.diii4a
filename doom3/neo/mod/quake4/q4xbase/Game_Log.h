
#ifndef __GAME_LOG_H__
#define	__GAME_LOG_H__

//============================================================================

class rvGameLogLocal : public rvGameLog {
public:

	rvGameLogLocal ( void );

	virtual void		Init		( void );
	virtual void		Shutdown	( void );

	virtual void		BeginFrame	( int time );
	virtual void		EndFrame	( void );

	virtual	void		Set			( const char* keyword, int value );
	virtual void		Set			( const char* keyword, float value );
	virtual void		Set			( const char* keyword, const char* value );
	virtual void		Set			( const char* keyword, bool value );
	
	virtual void		Add			( const char* keyword, int value );
	virtual void		Add			( const char* keyword, float value );

protected:

	int			lastTime;
	int			indexCount;
	idStrList	index;
	idStrList	frame;
	idStrList	oldframe;
	idFile*		file;
	bool		initialized;
	idTimer		timer_fps;
};

extern rvGameLogLocal		gameLogLocal;

#endif	// __GAME_LOG_H__
