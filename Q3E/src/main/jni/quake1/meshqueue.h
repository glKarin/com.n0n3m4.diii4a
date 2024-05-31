
#ifndef MESHQUEUE_H
#define MESHQUEUE_H

// VorteX: seems this value is hardcoded in other several defines as it's changing makes mess
#define MESHQUEUE_TRANSPARENT_BATCHSIZE 256

void R_MeshQueue_BeginScene(void);
void R_MeshQueue_AddTransparent(dptransparentsortcategory_t category, const vec3_t center, void (*callback)(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist), const entity_render_t *ent, int surfacenumber, const rtlight_t *rtlight);
void R_MeshQueue_RenderTransparent(void);

#endif
