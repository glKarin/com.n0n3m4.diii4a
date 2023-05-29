#ifndef __HH_CAMERA_INTERPOLATOR_H
#define __HH_CAMERA_INTERPOLATOR_H

/**********************************************************************

hhCameraInterpolator

**********************************************************************/
class hhCameraInterpolator;

enum InterpolationType {
	IT_None,
	IT_VariableMidPointSinusoidal,
	IT_Linear,
	IT_Inverse,
	IT_NumTypes
};

const int INTERPOLATE_NONE = 0;
const int INTERPOLATE_POSITION = BIT( 1 );
const int INTERPOLATE_ROTATION = BIT( 2 );
const int INTERPOLATE_EYEOFFSET = BIT( 3 );

const int INTERPMASK_ALL		= -1; 
const int INTERPMASK_WALLWALK	= INTERPOLATE_POSITION | INTERPOLATE_ROTATION;


typedef float (*InterpFunc)( float&, float );

class hhPlayer;

class hhCameraInterpolator {
	public:
		hhCameraInterpolator();

		void				SetSelf( hhPlayer* self );

		float				GetCurrentEyeHeight() const;
		float				GetIdealEyeHeight() const;
		idVec3				GetCurrentEyeOffset() const;
		idVec3				GetEyePosition() const;
		idVec3				GetCurrentPosition() const;
		idVec3				GetIdealPosition() const;

		idVec3				GetCurrentUpVector() const;
		idMat3				GetCurrentAxis() const;
		idAngles			GetCurrentAngles() const;
		idQuat				GetCurrentRotation() const;
	
		idVec3				GetIdealUpVector() const;
		idMat3				GetIdealAxis() const;
		idAngles			GetIdealAngles() const;
		idQuat				GetIdealRotation() const;

		void				UpdateEyeOffset();

		void				UpdateTarget( const idVec3& idealPosition, const idMat3& idealAxis, const float eyeOffset, int interpFlags );
		void				SetTargetPosition( const idVec3& idealPos, int interpFlags );
		void				SetTargetAxis( const idMat3& idealAxis, int interpFlags );
		void				SetTargetEyeOffset( float idealEyeOffset, int interpFlags );
		void				SetIdealRotation( const idQuat& idealRotation ) { rotationInfo.end = idealRotation; }

		void				Setup( const float lerpScale, const InterpolationType type );

		void				Reset( const idVec3& position, const idVec3& idealUpVector, float eyeOffset );

		idAngles			UpdateViewAngles( const idAngles& viewAngles );

		InterpolationType	SetInterpolationType( InterpolationType type );
		void				Evaluate( float frameTime );

		idQuat				DetermineIdealRotation( const idVec3& idealUpVector, const idVec3& viewDir, const idMat3& untransformedViewAxis );

		void				Save( idSaveGame *savefile ) const;
		void				Restore( idRestoreGame *savefile );

		//rww - send over net
		void				WriteToSnapshot( idBitMsgDelta &msg, const hhPlayer *pl ) const;
		void				ReadFromSnapshot( const idBitMsgDelta &msg, hhPlayer *pl );

	protected:
		void				ClearFuncList();
		void				RegisterFunc( InterpFunc func, InterpolationType type );
		InterpFunc			DetermineFunc( InterpolationType type );

		idQuat				DetermineIdealRotation( const idVec3& idealUpVector );

		void				EvaluatePosition( float interpVal );
		void				EvaluateRotation( float interpVal );
		void				EvaluateEyeOffset( float interpVal );

		void				VerifyEyeOffset( float& eyeOffset );

	protected:
		template <class Type>
		struct LerpInfo_t {
			Type	start;
			Type	end;
			Type	current;
			float	interpVal;

			void	Set( Type endVal ) {
				end = endVal;
				start = current;
				interpVal = 0.0f;
			}
			
			void	Reset( Type endVal ) {
				end = endVal;
				start = end;
				current = start;
				interpVal = 1.0f;
			}

			void	IsDone() const {
				return interpVal >= 1.0f;
			}
		};

		LerpInfo_t<idQuat>	rotationInfo;
		LerpInfo_t<idVec3>	positionInfo;
		LerpInfo_t<float>	eyeOffsetInfo;

		InterpolationType	interpType;

		float				lerpScale;

		hhPlayer*			self;
		idList<InterpFunc>	funcList;

		idClipModel			clipBounds;
};

#endif