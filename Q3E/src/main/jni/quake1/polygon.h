
#ifndef POLYGON_H
#define POLYGON_H

/*
Polygon clipping routines written by Ashley Rose Hale (LadyHavoc) and placed into public domain.
*/

void PolygonF_QuadForPlane(float *outpoints, float planenormalx, float planenormaly, float planenormalz, float planedist, float quadsize);
void PolygonD_QuadForPlane(double *outpoints, double planenormalx, double planenormaly, double planenormalz, double planedist, double quadsize);
int PolygonF_Clip(int innumpoints, const float *inpoints, float planenormalx, float planenormaly, float planenormalz, float planedist, float epsilon, int outfrontmaxpoints, float *outfrontpoints);
int PolygonD_Clip(int innumpoints, const double *inpoints, double planenormalx, double planenormaly, double planenormalz, double planedist, double epsilon, int outfrontmaxpoints, double *outfrontpoints);
void PolygonF_Divide(int innumpoints, const float *inpoints, float planenormalx, float planenormaly, float planenormalz, float planedist, float epsilon, int outfrontmaxpoints, float *outfrontpoints, int *neededfrontpoints, int outbackmaxpoints, float *outbackpoints, int *neededbackpoints, int *oncountpointer);
void PolygonD_Divide(int innumpoints, const double *inpoints, double planenormalx, double planenormaly, double planenormalz, double planedist, double epsilon, int outfrontmaxpoints, double *outfrontpoints, int *neededfrontpoints, int outbackmaxpoints, double *outbackpoints, int *neededbackpoints, int *oncountpointer);

#endif
