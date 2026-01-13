
#ifndef __DECLPREYBEAM_H__
#define __DECLPREYBEAM_H__

/*
===============================================================================

	hhDeclBeam

===============================================================================
*/

#define MAX_BEAMS				8

typedef enum beamCmdTypes_s {
	BEAMCMD_SplineLinearToTarget,
	BEAMCMD_SplineArcToTarget,
	BEAMCMD_SplineAdd,
	BEAMCMD_SplineAddSin,
	BEAMCMD_SplineAddSinTim,
	BEAMCMD_SplineAddSinTimeScaled,
	BEAMCMD_ConvertSplineToNodes,
	BEAMCMD_NodeLinearToTarget,
	BEAMCMD_NodeElectric,
} beamCmdTypes_t;

typedef struct beamCmd_s {
	beamCmdTypes_t		type;
	int					index; // optional data
	idVec3				offset; // optional data
	idVec3				phase; // optional data
} beamCmd_t;

class hhDeclBeam : public idDecl {
public:
	int					numNodes;
	int					numBeams;

	float				thickness[MAX_BEAMS];
	bool				bFlat[MAX_BEAMS];
	bool				bTaperEndPoints[MAX_BEAMS];
	const idMaterial	*shader[MAX_BEAMS];
	const idMaterial	*quadShader[MAX_BEAMS][2];
	float				quadSize[MAX_BEAMS][2];
	idList<beamCmd_t>	cmds[MAX_BEAMS];

						hhDeclBeam();
	virtual const char	*DefaultDefinition() const;
	virtual bool		Parse( const char *text, const int textLength );
	virtual void		FreeData();
};

#endif /* !__DECLPREYBEAM_H__ */
