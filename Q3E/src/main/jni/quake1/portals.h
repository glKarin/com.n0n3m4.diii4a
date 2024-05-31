
#ifndef PORTALS_H
#define PORTALS_H

int Portal_CheckPolygon(model_t *model, vec3_t eye, float *polypoints, int numpoints);
int Portal_CheckBox(model_t *model, vec3_t eye, vec3_t a, vec3_t b);
void Portal_Visibility(model_t *model, const vec3_t eye, int *leaflist, unsigned char *leafpvs, int *numleafspointer, int *surfacelist, unsigned char *surfacepvs, int *numsurfacespointer, const mplane_t *frustumplanes, int numfrustumplanes, int exact, const float *boxmins, const float *boxmaxs, float *updateleafsmins, float *updateleafsmaxs, unsigned char *shadowtrispvs, unsigned char *lighttrispvs, unsigned char *visitingleafpvs);

#endif

