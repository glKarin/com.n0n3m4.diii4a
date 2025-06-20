
#pragma once
#if !defined(NODES_32BIT_COMPAT)
#define NODES_32BIT_COMPAT

//#include "nodes.h"

#if _GRAPH_VERSION != _GRAPH_VERSION_RETAIL

#include "stdint.h"

typedef int32_t PTR32;

class CGraph_Retail
{
public:

	BOOL        m_fGraphPresent;
	BOOL        m_fGraphPointersSet;
	BOOL        m_fRoutingComplete;

	PTR32       m_pNodes; // CNode*
	PTR32       m_pLinkPool; // CLink*
	PTR32       m_pRouteInfo; // signed char*

	int         m_cNodes;
	int         m_cLinks;
	int         m_nRouteInfo;

	PTR32       m_di; // DIST_INFO*
	int         m_RangeStart[3][NUM_RANGES];
	int         m_RangeEnd[3][NUM_RANGES];
	float       m_flShortest;
	int         m_iNearest;
	int         m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ;
	int         m_minBoxX, m_minBoxY, m_minBoxZ, m_maxBoxX, m_maxBoxY, m_maxBoxZ;
	int         m_CheckedCounter;
	float       m_RegionMin[3], m_RegionMax[3];
	CACHE_ENTRY m_Cache[CACHE_SIZE];


	int         m_HashPrimes[16];
	PTR32       m_pHashLinks; // short*
	int         m_nHashLinks;

	int         m_iLastActiveIdleSearch;

	int         m_iLastCoverSearch;


	void copyOverTo(CGraph *other) {
		other->m_fGraphPresent	= m_fGraphPresent;
		other->m_fGraphPointersSet	= m_fGraphPointersSet;
		other->m_fRoutingComplete	= m_fRoutingComplete;

		other->m_pNodes = NULL;
		other->m_pLinkPool = NULL;
		other->m_pRouteInfo = NULL;

		other->m_cNodes	= m_cNodes;
		other->m_cLinks	= m_cLinks;
		other->m_nRouteInfo	= m_nRouteInfo;

		other->m_di = NULL;

		memcpy( (void *) &other->m_RangeStart, (void *) m_RangeStart,
				offsetof(class CGraph, m_pHashLinks) - offsetof(class CGraph, m_RangeStart) );

#if 0	          // replacement routine in case a change in CGraph breaks the above memcpy
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < NUM_RANGES; ++j)
				other->m_RangeStart[i][j] = m_RangeStart[i][j];
//		m_RangeStart[3][NUM_RANGES]
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < NUM_RANGES; ++j)
				other->m_RangeEnd[i][j] = m_RangeEnd[i][j];
//		m_RangeEnd[3][NUM_RANGES]
		other->m_flShortest	= m_flShortest;
		other->m_iNearest	= m_iNearest;
		other->m_minX	= m_minX;
		other->m_minY	= m_minY;
		other->m_minZ	= m_minZ;
		other->m_maxX	= m_maxX;
		other->m_maxY	= m_maxY;
		other->m_maxZ	= m_maxZ;
		other->m_minBoxX	= m_minBoxX;
		other->m_minBoxY	= m_minBoxY;
		other->m_minBoxZ	= m_minBoxZ;
		other->m_maxBoxX	= m_maxBoxX;
		other->m_maxBoxY	= m_maxBoxY;
		other->m_maxBoxZ	= m_maxBoxZ;
		other->m_CheckedCounter	= m_CheckedCounter;
		for (int i = 0; i < 3; ++i)
			other->m_RegionMin[i]	= m_RegionMin[i];
//		m_RegionMin[3]
		for (int i = 0; i < 3; ++i)
			other->m_RegionMax[i]	= m_RegionMax[i];
//		m_RegionMax[3]
		for (int i = 0; i < CACHE_SIZE; ++i)
			other->m_Cache[i]	= m_Cache[i];
//		m_Cache[CACHE_SIZE]
		for (int i = 0; i < 16; ++i)
			other->m_HashPrimes[i]	= m_HashPrimes[i];
//		m_HashPrimes[16]
#endif

		other->m_pHashLinks = NULL;
		other->m_nHashLinks	= m_nHashLinks;

		other->m_iLastActiveIdleSearch	= m_iLastActiveIdleSearch;

		other->m_iLastCoverSearch	= m_iLastCoverSearch;

	}

};


class CLink_Retail
{

public:
	int		m_iSrcNode;
	int		m_iDestNode;
	PTR32	m_pLinkEnt; // entvars_t*
	char	m_szLinkEntModelname[ 4 ];
	int		m_afLinkInfo;
	float	m_flWeight;

	void copyOverTo(CLink* other) {
		other->m_iSrcNode	= m_iSrcNode;
		other->m_iDestNode	= m_iDestNode	;
		other->m_pLinkEnt	= NULL;
		for (int i = 0; i < 4; ++i)
			other->m_szLinkEntModelname[i]	= m_szLinkEntModelname[i];
//		m_szLinkEntModelname[ 4 ]
		other->m_afLinkInfo	= m_afLinkInfo;
		other->m_flWeight	= m_flWeight;
	}
};

#endif
#endif
