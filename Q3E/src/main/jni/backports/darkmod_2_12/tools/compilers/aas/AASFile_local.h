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

#ifndef __AASFILELOCAL_H__
#define __AASFILELOCAL_H__

/*
===============================================================================

	AAS File Local

===============================================================================
*/

class idAASFileLocal : public idAASFile {
	friend class idAASBuild;
	friend class idAASReach;
	friend class idAASCluster;
public:
								idAASFileLocal( void );
	virtual 					~idAASFileLocal( void ) override;

public:
	virtual idVec3				EdgeCenter( int edgeNum ) const override;
	virtual idVec3				FaceCenter( int faceNum ) const override;
	virtual idVec3				AreaCenter( int areaNum ) const override;

	virtual idBounds			EdgeBounds( int edgeNum ) const override;
	virtual idBounds			FaceBounds( int faceNum ) const override;
	virtual idBounds			AreaBounds( int areaNum ) const override;

	virtual int					PointAreaNum( const idVec3 &origin ) const override;
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const override;
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const override;
	virtual void				PushPointIntoAreaNum( int areaNum, idVec3 &point ) const override;
	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const override;
	virtual void				PrintInfo( void ) const override;

public:
	bool						Load( const idStr &fileName, const unsigned int mapFileCRC );
	bool						Write( const idStr &fileName, const unsigned int mapFileCRC );

	int							MemorySize( void ) const;
	void						ReportRoutingEfficiency( void ) const;
	void						Optimize( void );
	void						LinkReversedReachability( void );
	void						FinishAreas( void );

	void						Clear( void );
	void						DeleteReachabilities( void );
	void						DeleteClusters( void );

private:
	bool						ParseIndex( idLexer &src, idList<aasIndex_t> &indexes );
	bool						ParsePlanes( idLexer &src );
	bool						ParseVertices( idLexer &src );
	bool						ParseEdges( idLexer &src );
	bool						ParseFaces( idLexer &src );
	bool						ParseReachabilities( idLexer &src, int areaNum );
	bool						ParseAreas( idLexer &src );
	bool						ParseNodes( idLexer &src );
	bool						ParsePortals( idLexer &src );
	bool						ParseClusters( idLexer &src );

private:
	int							BoundsReachableAreaNum_r( int nodeNum, const idBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const;
	void						MaxTreeDepth_r( int nodeNum, int &depth, int &maxDepth ) const;
	int							MaxTreeDepth( void ) const;
	int							AreaContentsTravelFlags( int areaNum ) const;
	idVec3						AreaReachableGoal( int areaNum ) const;
	int							NumReachabilities( void ) const;
	void						FindAreasInBounds_r(const idBounds &bounds, idList<int> &areaNums, int nodeNum) const;
	int							FindAreasInBounds(const idBounds &bounds, idList<int> &areaNums) const;
};

#endif /* !__AASFILELOCAL_H__ */
