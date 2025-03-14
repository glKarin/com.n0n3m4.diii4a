//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef DETOUROBSTACLEAVOIDANCE_H
#define DETOUROBSTACLEAVOIDANCE_H

struct dtObstacleCircle
{
	float p[3];				// Position of the obstacle
	float vel[3];			// Velocity of the obstacle
	float dvel[3];			// Velocity of the obstacle
	float rad;				// Radius of the obstacle
	float dp[3], np[3];		// Use for side selection during sampling.
	float dist;
};

struct dtObstacleSegment
{
	float p[3], q[3];		// End points of the obstacle segment
	bool touch;
	float dist;
};

static const int RVO_SAMPLE_RAD = 15;
static const int MAX_RVO_SAMPLES = (RVO_SAMPLE_RAD*2+1)*(RVO_SAMPLE_RAD*2+1) + 100;

class dtObstacleAvoidanceDebugData
{
public:
	dtObstacleAvoidanceDebugData();
	~dtObstacleAvoidanceDebugData();
	
	bool init(const int maxSamples);
	void reset();
	void addSample(const float* vel, const float ssize, const float pen,
				   const float vpen, const float vcpen, const float spen, const float tpen);
	
	void normalizeSamples();
	
	inline int getSampleCount() const { return m_nsamples; }
	inline const float* getSampleVelocity(const int i) const { return &m_vel[i*3]; }
	inline float getSampleSize(const int i) const { return m_ssize[i]; }
	inline float getSamplePenalty(const int i) const { return m_pen[i]; }
	inline float getSampleDesiredVelocityPenalty(const int i) const { return m_vpen[i]; }
	inline float getSampleCurrentVelocityPenalty(const int i) const { return m_vcpen[i]; }
	inline float getSamplePreferredSidePenalty(const int i) const { return m_spen[i]; }
	inline float getSampleCollisionTimePenalty(const int i) const { return m_tpen[i]; }

private:
	int m_nsamples;
	int m_maxSamples;
	float* m_vel;
	float* m_ssize;
	float* m_pen;
	float* m_vpen;
	float* m_vcpen;
	float* m_spen;
	float* m_tpen;
};

dtObstacleAvoidanceDebugData* dtAllocObstacleAvoidanceDebugData();
void dtFreeObstacleAvoidanceDebugData(dtObstacleAvoidanceDebugData* ptr);


class dtObstacleAvoidanceQuery
{
public:
	dtObstacleAvoidanceQuery();
	~dtObstacleAvoidanceQuery();
	
	bool init(const int maxCircles, const int maxSegments);
	
	void reset();

	void addCircle(const float* pos, const float rad,
				   const float* vel, const float* dvel,
				   const float dist);
				   
	void addSegment(const float* p, const float* q, const float dist);

	inline void setSamplingGridSize(int n) { m_gridSize = n; }
	inline void setSamplingGridDepth(int n) { m_gridDepth = n; }
	inline void setVelocitySelectionBias(float v) { m_velBias = v; }
	inline void setDesiredVelocityWeight(float w) { m_weightDesVel = w; }
	inline void setCurrentVelocityWeight(float w) { m_weightCurVel = w; }
	inline void setPreferredSideWeight(float w) { m_weightSide = w; }
	inline void setCollisionTimeWeight(float w) { m_weightToi = w; }
	inline void setTimeHorizon(float t) { m_horizTime = t; }	

	void sampleVelocity(const float* pos, const float rad, const float vmax,
						const float* vel, const float* dvel,
						float* nvel,
						dtObstacleAvoidanceDebugData* debug = 0);

	void sampleVelocityAdaptive(const float* pos, const float rad, const float vmax,
								const float* vel, const float* dvel,
								float* nvel,
								dtObstacleAvoidanceDebugData* debug = 0);
	
	inline int getObstacleCircleCount() const { return m_ncircles; }
	const dtObstacleCircle* getObstacleCircle(const int i) { return &m_circles[i]; }

	inline int getObstacleSegmentCount() const { return m_nsegments; }
	const dtObstacleSegment* getObstacleSegment(const int i) { return &m_segments[i]; }

private:

	void prepare(const float* pos, const float* dvel);

	float processSample(const float* vcand, const float cs,
						const float* pos, const float rad,
						const float vmax, const float* vel, const float* dvel,
						dtObstacleAvoidanceDebugData* debug);

	dtObstacleCircle* insertCircle(const float dist);
	dtObstacleSegment* insertSegment(const float dist);

	int m_gridSize;
	int m_gridDepth;
	float m_velBias;
	float m_weightDesVel;
	float m_weightCurVel;
	float m_weightSide;
	float m_weightToi;
	float m_horizTime;
	
	int m_maxCircles;
	dtObstacleCircle* m_circles;
	int m_ncircles;

	int m_maxSegments;
	dtObstacleSegment* m_segments;
	int m_nsegments;
};

dtObstacleAvoidanceQuery* dtAllocObstacleAvoidanceQuery();
void dtFreeObstacleAvoidanceQuery(dtObstacleAvoidanceQuery* ptr);


#endif // DETOUROBSTACLEAVOIDANCE_H