#ifndef _KARIN_OCCLUSIONTEST_H
#define _KARIN_OCCLUSIONTEST_H

class rvmOcclusionQuery;
class idRenderWorldLocal;

class sdOcclusionTestLocal : public sdOcclusionTest
{
	public:
							sdOcclusionTestLocal(void);
	virtual					~sdOcclusionTestLocal(void);
	virtual void			UpdateOcclusionTest( const occlusionTest_t *def );
	virtual void			FreeOcclusionTest(void);
	bool					IsVisible(void);
	int						CountVisible(void);
	int						GetResult(void) const;

public:
	int						index;
	idRenderWorldLocal		*world;
	occlusionTest_t			parms;

	private:
	qhandle_t				query;
};

#endif
