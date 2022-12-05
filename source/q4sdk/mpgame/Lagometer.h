#ifndef __LAGOMETER_H__
#define __LAGOMETER_H__

#define LAGOMETER_HISTORY 64

class idLagometer
{
public:
	idLagometer();

	void Update(int ahead, int dupe);
	void Draw();
protected:
	int aheadOfServer[LAGOMETER_HISTORY];
	byte dupeUserCmds[LAGOMETER_HISTORY];

	int frameIndex;
};


#endif
