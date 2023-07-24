// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __SIGNAL_H__
#define __SIGNAL_H__

class sdSignal {
public:
	static const int	WAIT_INFINITE = -1;

						sdSignal();
						~sdSignal();

	void				Set();
	void				Clear();
	bool				Wait( int timeout = WAIT_INFINITE );
	bool				SignalAndWait( sdSignal &signal, int timeout = WAIT_INFINITE );
protected:
	signalHandle_t		handle;
};

#endif /* !__SIGNAL_H__ */
