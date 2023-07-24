// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETTASK_H__ )
#define __SDNETTASK_H__

#include "SDNetErrorCode.h"

//===============================================================
//
//	sdNetTask
//
//===============================================================

class sdNetTask {
public:
	enum taskStatus_e {
		TS_INITIAL,
		TS_PENDING,
		TS_CANCELLING,
		TS_COMPLETING,
		TS_DONE
	};

	virtual						~sdNetTask() {}

	virtual void				Cancel( bool blocking = false ) = 0;

	virtual taskStatus_e		GetState() const = 0;
	virtual sdNetErrorCode_e	GetErrorCode() const = 0;

	virtual void				AcquireLock() = 0;
	virtual void				ReleaseLock() = 0;
};

#endif /* !__SDNETTASK_H__ */
