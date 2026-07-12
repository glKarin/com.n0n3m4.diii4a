#ifndef _RVM_OCCLUSION_QUERY_H
#define _RVM_OCCLUSION_QUERY_H

class rvmOcclusionQuery {
public:
	enum rvmOcclusionQueryState
	{
		OCCLUSION_QUERY_STATE_UNINITIALIZED = 0,
		OCCLUSION_QUERY_STATE_READY,
		OCCLUSION_QUERY_STATE_DRAW,
		OCCLUSION_QUERY_STATE_WAITING,
		OCCLUSION_QUERY_STATE_FINISH,
	};

	static const int RESULT_INVALID;

    rvmOcclusionQuery();

    //~rvmOcclusionQuery(){}

    bool IsQueryStale(void) const;
    int Query(int def = -1);

	static void BeginRender(void);
	static void EndRender(void);
	void Init(void);
	void Destroy(void);
	void SetMode(GLenum m);
	void Begin(int ms = 0);
	void End(void);
	void Sync(bool wait = false);
	int GetResult(void) const;
	void Next(void);
	bool IsWaiting(void) const;
	bool IsFinished(void) const;
	bool IsUninitialized(void) const;
	bool HasResult(void) const;
	rvmOcclusionQueryState State(void) const;
	GLuint QueryID(void) const;

private:
	void Reset(void);

private:
    int queryStartTime;
    int queryTimeOutTime;
    GLuint id;
    rvmOcclusionQueryState queryState;
	GLenum mode;
	int result;
};

class idOcclusionTestJob
{
public:
	enum updateType_e {
		UT_NONE = 0,
		UT_MANUAL = 1,
		UT_FRAME = 2,
	};
							idOcclusionTestJob(void);
	virtual					~idOcclusionTestJob(void);
	void					UpdateGeometry( const idBounds &bounds ); // frontend
	void					UpdateGeometry( const srfTriangles_t *tris ); // frontend
	void					UpdatePosition( const idVec3 &origin, const idMat3 &axis ); // frontend
	void					UpdateView( int viewId ); // frontend
	void					UpdateQueryMode( GLenum mode ); // frontend
	void					ActualFree(void); // backend
	void					Free(void); // frontend
	void					Render(void); // backend
	void					Start(updateType_e mode); // frontend
	void					Restart(void); // frontend
	void					Ready(void); // frontend
	bool					CanQuery(void) const; // backend
	void					Query(void); // backend

private:
	void					UpdateTriByBounds(void);
	void					UpdateTriByTris(void);
	void					MakeModelMatrix(void);
	enum {
		DIRTY_NONE = 0,
		DIRTY_BOUNDS = 1,
		DIRTY_MATRIX = 1 << 1,
		DIRTY_TRIS = 1 << 2,
	};

	struct frontEndInfo_t {
		idVec3		origin;
		idMat3		axis;
		idBounds	bounds;
		int			viewID;
		GLenum		mode;
		int			dirty;
		bool		start;
		const srfTriangles_t *tris;
	};

public:
	int						index;
	int						viewID;
	rvmOcclusionQuery		*query;
	srfTriangles_t			*tri;
	int						lastResult;
	float					modelMatrix[16]; // backend
	updateType_e			update;
	bool					running;

private:
	frontEndInfo_t			parms; // frontend
};



class idOcclusionTestManager
{
	public:
	void					Init(void);
	void					Shutdown(void);
	void					Update(void); // frontend, thread lock
	void					Ready(void); // backend, thread lock
	void					Render(void); // backend
	void					Query(void); // backend
	qhandle_t				Alloc(void);
	void					Free(qhandle_t handle);
	idOcclusionTestJob *	Get(qhandle_t handle);
	int						GetResult(qhandle_t handle) const;

private:
	void					HandleFree(void); // frontend
	void					HandleUpdate(void); // frontend
	void					HandleDelete(void); // backend
	void					HandleRender(void); // backend
	void					HandleQuery(void); // backend
	void					BeginRender();
	void					EndRender();

private:
	idList<idOcclusionTestJob *> list;
	idList<idOcclusionTestJob *> renderList; // backend
	idList<qhandle_t> 			 freeList; // frontend
	idList<idOcclusionTestJob *> deleteList; // backend
};

extern idOcclusionTestManager *occlusionTestManager;



ID_INLINE int rvmOcclusionQuery::GetResult(void) const
{
	return result;
}

ID_INLINE bool rvmOcclusionQuery::IsWaiting(void) const
{
	return queryState == OCCLUSION_QUERY_STATE_WAITING;
}

ID_INLINE bool rvmOcclusionQuery::IsFinished(void) const
{
	return queryState == OCCLUSION_QUERY_STATE_FINISH;
}

ID_INLINE bool rvmOcclusionQuery::IsUninitialized(void) const
{
	return queryState == OCCLUSION_QUERY_STATE_UNINITIALIZED;
}

ID_INLINE bool rvmOcclusionQuery::HasResult(void) const
{
	return result > RESULT_INVALID;
}

ID_INLINE rvmOcclusionQuery::rvmOcclusionQueryState rvmOcclusionQuery::State(void) const
{
	return queryState;
}

ID_INLINE GLuint rvmOcclusionQuery::QueryID(void) const
{
	return id;
}

#endif
