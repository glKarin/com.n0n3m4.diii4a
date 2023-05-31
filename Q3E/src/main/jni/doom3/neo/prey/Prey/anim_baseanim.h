
#ifndef __PREY_ANIM_MD5ANIM_H__
#define __PREY_ANIM_MD5ANIM_H__

// Forward declar
class idMD5Bone;

class hhMD5Anim : public idMD5Anim {

public:

							hhMD5Anim(void);
	void					SetLimits(float start, float end) const;
	virtual void			ConvertTimeToFrame( int time, int cycleCount, frameBlend_t &frame ) const;
   	virtual int				Length( void ) const;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

protected:
	mutable float		start;
	mutable float		end;
										  

};


#endif /* __PREY_ANIM_MD5ANIM_H__ */

