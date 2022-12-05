// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __DECLAF_H__
#define __DECLAF_H__

/*
===============================================================================

	Articulated Figure

===============================================================================
*/

class idDeclAF;

typedef enum {
	DECLAF_CONSTRAINT_INVALID,
	DECLAF_CONSTRAINT_FIXED,
	DECLAF_CONSTRAINT_BALLANDSOCKETJOINT,
	DECLAF_CONSTRAINT_UNIVERSALJOINT,
	DECLAF_CONSTRAINT_HINGE,
	DECLAF_CONSTRAINT_SLIDER,
	DECLAF_CONSTRAINT_SPRING
} declAFConstraintType_t;

typedef enum {
	DECLAF_JOINTMOD_AXIS,
	DECLAF_JOINTMOD_ORIGIN,
	DECLAF_JOINTMOD_BOTH
} declAFJointMod_t;

typedef bool (*getJointTransform_t)( void *model, const idJointMat *frame, const char *jointName, idVec3 &origin, idMat3 &axis );

// RAVEN BEGIN
// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idAFVector : public Serializable<'AFV '> {
public:
	// Serialization methods
	void					Write(SerialOutputStream &stream) const;
	void					AddReferences() const;
							idAFVector(SerialInputStream &stream);


#else
class idAFVector {
public:
#endif
	enum idAFVectorType_t {
// RAVEN END
		VEC_COORDS = 0,
		VEC_JOINT,
		VEC_BONECENTER,
		VEC_BONEDIR
	}						type;
	idStr					joint1;
	idStr					joint2;

public:
							idAFVector( void );

	bool					Parse( idLexer &src );
	bool					Finish( const char *fileName, const getJointTransform_t GetJointTransform, const idJointMat *frame, void *model ) const;
	bool					Write( idFile *f ) const;
	const char *			ToString( idStr &str, const int precision = 8 );
	const idVec3 &			ToVec3( void ) const { return vec; }
	idVec3 &				ToVec3( void ) { return vec; }

private:
	mutable idVec3			vec;
	bool					negate;
};

// RAVEN BEGIN
// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclAF_Body : public Serializable<'DAFB'> {
public:

	// Serializable Methods
	void					Write( SerialOutputStream &stream ) const;
							idDeclAF_Body( SerialInputStream &stream );
	void					AddReferences() const;
#else
class idDeclAF_Body {
#endif
public:
							idDeclAF_Body();
// RAVEN END
	idStr					name;
	idStr					jointName;
	declAFJointMod_t		jointMod;
	int						modelType;
	idAFVector				v1, v2;
	int						numSides;
	float					width;
	float					density;
	idAFVector				origin;
	idAngles				angles;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	idMat3					inertiaScale;
	float					linearFriction;
	float					angularFriction;
	float					contactFriction;
	idStr					containedJoints;
	idAFVector				frictionDirection;
	idAFVector				contactMotorDirection;
public:
	void					SetDefault( const idDeclAF *file );
};

// RAVEN BEGIN
// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclAF_Constraint : public Serializable<'DAFC'> {
public:
	// Serializable Methods
	void					Write( SerialOutputStream &stream ) const;
							idDeclAF_Constraint( SerialInputStream &stream );
	void					AddReferences() const;
#else
class idDeclAF_Constraint {
public:
#endif
							idDeclAF_Constraint();
// RAVEN END
	idStr					name;
	idStr					body1;
	idStr					body2;
	declAFConstraintType_t	type;
	float					friction;
	float					stretch;
	float					compress;
	float					damping;
	float					restLength;
	float					minLength;
	float					maxLength;
	idAFVector				anchor;
	idAFVector				anchor2;
	idAFVector				shaft[2];
	idAFVector				axis;
// RAVEN BEGIN
// jsinger: added declAFLimitType_t to support serialization/deserialization of binary decls
	enum declAFLimitType_t {
		LIMIT_NONE = -1,
		LIMIT_CONE,
		LIMIT_PYRAMID
	}						limit;
// RAVEN END
	idAFVector				limitAxis;
	float					limitAngles[3];

public:
	void					SetDefault( const idDeclAF *file );
};

// RAVEN BEGIN
// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclAF : public idDecl, public Serializable<'DAF '> {
public:
							idDeclAF( SerialInputStream &stream );
	void					Write( SerialOutputStream &stream ) const;
	void					AddReferences() const;
#else
class idDeclAF : public idDecl {
#endif
// RAVEN END
	friend class idAFFileManager;
public:
							idDeclAF( void );
	virtual					~idDeclAF( void );

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength, bool noCaching );
	virtual void			FreeData( void );

// RAVEN BEGIN
// scork: for detailed error-reporting
	virtual bool			Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END

	virtual void			Finish( const getJointTransform_t GetJointTransform, const idJointMat *frame, void *model ) const;

	bool					Save( void );

	void					NewBody( const char *name );
	void					RenameBody( const char *oldName, const char *newName );
	void					DeleteBody( const char *name );

	void					NewConstraint( const char *name );
	void					RenameConstraint( const char *oldName, const char *newName );
	void					DeleteConstraint( const char *name );

	static int				ContentsFromString( const char *str );
	static const char *		ContentsToString( const int contents, idStr &str );

	static declAFJointMod_t	JointModFromString( const char *str );
	static const char *		JointModToString( declAFJointMod_t jointMod );

public:
	bool					modified;
	idStr					model;
	idStr					skin;
	float					defaultLinearFriction;
	float					defaultAngularFriction;
	float					defaultContactFriction;
	float					defaultConstraintFriction;
	float					totalMass;
	idVec2					suspendVelocity;
	idVec2					suspendAcceleration;
	float					noMoveTime;
	float					noMoveTranslation;
	float					noMoveRotation;
	float					minMoveTime;
	float					maxMoveTime;
	int						contents;
	int						clipMask;
	bool					selfCollision;
// RAVEN BEGIN
// rjohnson: fast AF eval to skip some things that are not needed for specific circumstances
	bool					fastEval;
// RAVEN END
	idList<idDeclAF_Body *>			bodies;
	idList<idDeclAF_Constraint *>	constraints;

private:
	bool					ParseContents( idLexer &src, int &c ) const;
	bool					ParseBody( idLexer &src );
	bool					ParseFixed( idLexer &src );
	bool					ParseBallAndSocketJoint( idLexer &src );
	bool					ParseUniversalJoint( idLexer &src );
	bool					ParseHinge( idLexer &src );
	bool					ParseSlider( idLexer &src );
	bool					ParseSpring( idLexer &src );
	bool					ParseSettings( idLexer &src );

	bool					WriteBody( idFile *f, const idDeclAF_Body &body ) const;
	bool					WriteFixed( idFile *f, const idDeclAF_Constraint &c ) const;
	bool					WriteBallAndSocketJoint( idFile *f, const idDeclAF_Constraint &c ) const;
	bool					WriteUniversalJoint( idFile *f, const idDeclAF_Constraint &c ) const;
	bool					WriteHinge( idFile *f, const idDeclAF_Constraint &c ) const;
	bool					WriteSlider( idFile *f, const idDeclAF_Constraint &c ) const;
	bool					WriteSpring( idFile *f, const idDeclAF_Constraint &c ) const;
	bool					WriteConstraint( idFile *f, const idDeclAF_Constraint &c ) const;
	bool					WriteSettings( idFile *f ) const;

	bool					RebuildTextSource( void );
};

// RAVEN BEGIN
class rvDeclAFEdit
{
public:
	virtual ~rvDeclAFEdit() { }
	virtual bool 			Save( idDeclAF *edit ) = 0;
	virtual void 			NewBody( idDeclAF *edit, char const *name ) = 0;
	virtual void 			RenameBody( idDeclAF *edit, char const *oldName, char const *newName ) = 0;
	virtual void 			DeleteBody( idDeclAF *edit, char const *name ) = 0;
	virtual void 			NewConstraint( idDeclAF *edit, char const *name ) = 0;
	virtual void 			RenameConstraint( idDeclAF *edit, char const *oldName, char const *newName ) = 0;
	virtual void 			DeleteConstraint( idDeclAF *edit, char const *name ) = 0;
};

extern rvDeclAFEdit			*declAFEdit;
// RAVEN END

#endif /* !__DECLAF_H__ */
