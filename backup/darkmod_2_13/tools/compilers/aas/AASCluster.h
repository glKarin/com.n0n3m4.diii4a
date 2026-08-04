/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
 
#ifndef __AASCLUSTER_H__
#define __AASCLUSTER_H__

/*
===============================================================================

	Area Clustering

===============================================================================
*/

class idAASCluster {

public:
	bool					Build( idAASFileLocal *file );
	bool					BuildSingleCluster( idAASFileLocal *file );

private:
	idAASFileLocal *		file;
	bool					noFaceFlood;

private:
	bool					UpdatePortal( int areaNum, int clusterNum );
	bool					FloodClusterAreas_r( int areaNum, int clusterNum );
	void					RemoveAreaClusterNumbers( void );
	void					NumberClusterAreas( int clusterNum );
	bool					FindClusters( void );
	void					CreatePortals( void );
	bool					TestPortals( void );
	void					ReportEfficiency( void );
	void					RemoveInvalidPortals( void );
};

#endif /* !__AASCLUSTER_H__ */
