#ifndef GLQUAKE
#define GLQUAKE	//this is shit.
#endif
#include "quakedef.h"
#include "../plugin.h"
#include "com_mesh.h"
plugmodfuncs_t *modfuncs;
plugfsfuncs_t *filefuncs;

//#define ASEMODELS	//FIXME: TEST TEST TEST. shold be working iiuc
//#define LWOMODELS	//not working
#ifdef SKELETALMODELS
#define GLTFMODELS	//FIXME: not yet working properly.
extern qboolean Plug_GLTF_Init(void);
#endif

#ifdef ASEMODELS
struct aseimport_s
{
	char *file;

	struct asematerial_s
	{
		char name[MAX_QPATH];

		struct asemesh_s
		{
			struct asemesh_s *next;

			struct asevert_s
			{
				vec3_t xyz;
				vec3_t norm;
				vec2_t st;
			} *vert;
			unsigned int numverts;
			unsigned int maxverts;

			index_t *index;
			unsigned int numindexes;
			unsigned int maxindexes;

		} *meshes;
	} *materials;
	unsigned int nummaterials;
};

static char *ASE_GetToken(struct aseimport_s *ase, char *token, size_t tokensize)
{
	char *ret = token;
	tokensize--;

	while(*ase->file == ' ' || *ase->file == '\t')
		ase->file++;
	if (*ase->file == '\n')
	{
		*ret = 0;
		return ret;
	}

	if (*ase->file == '\"')
	{
		ase->file += 1;
		while(tokensize-- > 0)
		{
			char i = *ase->file;
			if (i == '\n')
				break;
			ase->file+=1;
			if (i == '\"')
				break;

			*token++ = i;
		}
	}
	else
	{
		while(tokensize-- > 0)
		{
			char i = *ase->file;
			if (i == ' ' || i == '\t' || i == '\r' || i == '\n')
				break;
			ase->file+=1;

			*token++ = i;
		}
	}
	*token = 0;
	return ret;
}

static void ASE_SkipLine(struct aseimport_s *ase)
{
	while (*ase->file)
	{
		if (*ase->file == '\n')
		{
			ase->file += 1;
			break;
		}
		ase->file += 1;
	}
}

static void ASE_SkipBlock(struct aseimport_s *ase)
{
	while (*ase->file)
	{
		if (*ase->file == '\n' || *ase->file == '{')
			break;
		ase->file += 1;
	}
	if (*ase->file == '{')
	{
		char token[1024];
		ASE_SkipLine(ase);

		while (*ase->file)
		{
			ASE_GetToken(ase, token, sizeof(token));
			if (!strcmp(token, "}"))
				break;
			else
			{
				ASE_SkipBlock(ase);
			}
			ASE_SkipLine(ase);
		}
	}
}

static void ASE_Material(struct aseimport_s *ase)
{
	int idx;
	char token[1024];
	ASE_GetToken(ase, token, sizeof(token));
	idx = atoi(token);

	if (idx < 0 || idx >= ase->nummaterials)
	{
		Con_Printf("invalid material index: %s\n", token);
		ASE_SkipBlock(ase);
		return;
	}

	ASE_GetToken(ase, token, sizeof(token));
	ASE_SkipLine(ase);
	if (strcmp(token, "{"))
		return;

	while (*ase->file)
	{
		ASE_GetToken(ase, token, sizeof(token));
		if (!strcmp(token, "}"))
			break;
		else if (!strcmp(token, "*MATERIAL_NAME"))
		{
			ASE_GetToken(ase, token, sizeof(token));
			Q_strlcpy(ase->materials[idx].name, token, sizeof(ase->materials[idx].name));
		}
		else if (!strcmp(token, "*MATERIAL_CLASS") || !strcmp(token, "*MATERIAL_DIFFUSE") || !strcmp(token, "*MATERIAL_SHADING") || !strcmp(token, "*MAP_DIFFUSE"))
			ASE_SkipBlock(ase);
		else
		{
			Con_Printf("Unknown top-level identifier: %s\n", token);
			ASE_SkipBlock(ase);
		}
		ASE_SkipLine(ase);
	}
}
static void ASE_MaterialList(struct aseimport_s *ase)
{
	char token[1024];
	ASE_GetToken(ase, token, sizeof(token));
	ASE_SkipLine(ase);
	if (strcmp(token, "{"))
		return;

	while (*ase->file)
	{
		ASE_GetToken(ase, token, sizeof(token));
		if (!strcmp(token, "}"))
			break;
		else if (!strcmp(token, "*MATERIAL_COUNT"))
		{
			ASE_GetToken(ase, token, sizeof(token));

			if (!ase->materials)
			{
				ase->nummaterials = atoi(token);
				ase->materials = malloc(sizeof(*ase->materials) * ase->nummaterials);
				memset(ase->materials, 0, sizeof(*ase->materials) * ase->nummaterials);
			}
		}
		else if (!strcmp(token, "*MATERIAL"))
		{
			ASE_Material(ase);
		}
		else
		{
			Con_Printf("Unknown top-level identifier: %s\n", token);
			ASE_SkipBlock(ase);
		}
		ASE_SkipLine(ase);
	}
}

static void ASE_GeomObject(struct aseimport_s *ase)
{
	size_t materialidx = 0;
	size_t numverts = 0;
	size_t numtverts = 0;
	size_t numtris = 0;
	struct
	{
		vec3_t xyz;
		vec3_t norm;
	} *verts = NULL;
	struct
	{
		vec3_t st;
	} *tverts = NULL;
	struct
	{
		index_t vidx[3];
		index_t tidx[3];
	} *tris = NULL;
	size_t idx;

	char token[1024];
	ASE_GetToken(ase, token, sizeof(token));
	ASE_SkipLine(ase);
	if (strcmp(token, "{"))
		return;

	while (*ase->file)
	{
		ASE_GetToken(ase, token, sizeof(token));
		if (!strcmp(token, "}"))
			break;
		else if (!strcmp(token, "*MATERIAL_REF"))
		{
			ASE_GetToken(ase, token, sizeof(token));
			materialidx = atoi(token);
		}
		else if (!strcmp(token, "*MESH"))
		{
			ASE_SkipLine(ase);

			while (*ase->file)
			{
				ASE_GetToken(ase, token, sizeof(token));
				if (!strcmp(token, "}"))
					break;
				else if (!strcmp(token, "*TIMEVALUE"))
					ASE_SkipBlock(ase);	//not useful
				else if (!strcmp(token, "*MESH_NUMVERTEX"))
				{
					ASE_GetToken(ase, token, sizeof(token));
					if (!verts)
					{
						numverts = atoi(token);
						verts = malloc(sizeof(*verts) * numverts);
						memset(verts, 0, sizeof(*verts) * numverts);
					}
				}
				else if (!strcmp(token, "*MESH_NUMFACES"))
				{
					ASE_GetToken(ase, token, sizeof(token));
					if (!tris)
					{
						numtris = atoi(token);
						tris = malloc(sizeof(*tris) * numtris);
						memset(tris, 0, sizeof(*tris) * numtris);
					}
				}
				else if (!strcmp(token, "*COMMENT"))
					ASE_SkipBlock(ase);	//unused
				else if (!strcmp(token, "*MESH_VERTEX_LIST"))
				{
					ASE_SkipLine(ase);
					while (*ase->file)
					{
						ASE_GetToken(ase, token, sizeof(token));
						if (!strcmp(token, "}"))
							break;
						else if (!strcmp(token, "*MESH_VERTEX"))
						{
							ASE_GetToken(ase, token, sizeof(token));
							idx = atoi(token);
							if (idx >= 0 && idx < numverts)
							{
								ASE_GetToken(ase, token, sizeof(token));
								verts[idx].xyz[0] = atof(token);
								ASE_GetToken(ase, token, sizeof(token));
								verts[idx].xyz[1] = atof(token);
								ASE_GetToken(ase, token, sizeof(token));
								verts[idx].xyz[2] = atof(token);
							}
						}
						else
						{
							Con_Printf("Unknown MESH_VERTEX_LIST identifier: %s\n", token);
							ASE_SkipBlock(ase);
						}
						ASE_SkipLine(ase);
					}
				}
				else if (!strcmp(token, "*MESH_NORMALS"))
				{
					ASE_SkipLine(ase);
					while (*ase->file)
					{
						ASE_GetToken(ase, token, sizeof(token));
						if (!strcmp(token, "}"))
							break;
						else if (!strcmp(token, "*MESH_FACENORMAL"))
							ASE_SkipBlock(ase);	//we don't give a fuck about these. its not usable for us. we'll calculate these if we actually need them, that way we're sure it actually matches the geometry and doesn't bug out.
						else if (!strcmp(token, "*MESH_VERTEXNORMAL"))
						{
							ASE_GetToken(ase, token, sizeof(token));
							idx = atoi(token);
							if (idx >= 0 && idx < numverts)
							{
								ASE_GetToken(ase, token, sizeof(token));
								verts[idx].norm[0] = atof(token);
								ASE_GetToken(ase, token, sizeof(token));
								verts[idx].norm[1] = atof(token);
								ASE_GetToken(ase, token, sizeof(token));
								verts[idx].norm[2] = atof(token);
							}
						}
						else
						{
							Con_Printf("Unknown MESH_NORMALS identifier: %s\n", token);
							ASE_SkipBlock(ase);
						}
						ASE_SkipLine(ase);
					}
				}
				else if (!strcmp(token, "*MESH_FACE_LIST"))
				{
					ASE_SkipLine(ase);
					while (*ase->file)
					{
						ASE_GetToken(ase, token, sizeof(token));
						if (!strcmp(token, "}"))
							break;
						else if (!strcmp(token, "*MESH_FACE"))
						{
							ASE_GetToken(ase, token, sizeof(token));
							idx = atoi(token);
							if (idx >= 0 && idx < numtris)
							{
								ASE_GetToken(ase, token, sizeof(token));
								while(*token)
								{
									if (!strcmp(token, "A:"))
									{
										ASE_GetToken(ase, token, sizeof(token));
										tris[idx].vidx[0] = atoi(token);
									}
									else if (!strcmp(token, "B:"))
									{
										ASE_GetToken(ase, token, sizeof(token));
										tris[idx].vidx[1] = atoi(token);
									}
									else if (!strcmp(token, "C:"))
									{
										ASE_GetToken(ase, token, sizeof(token));
										tris[idx].vidx[2] = atoi(token);
									}
									else if (!strcmp(token, "AB:") || !strcmp(token, "BC:") || !strcmp(token, "CA:") || !strcmp(token, "*MESH_SMOOTHING") || !strcmp(token, "*MESH_MTLID"))
									{
										ASE_GetToken(ase, token, sizeof(token));
									}
									else
									{
										Con_Printf("Unknown MESH_FACE identifier: %s\n", token);
										ASE_GetToken(ase, token, sizeof(token));
									}

									ASE_GetToken(ase, token, sizeof(token));
								}
								tris[idx].tidx[0] = atoi(token);
								ASE_GetToken(ase, token, sizeof(token));
								tris[idx].tidx[1] = atoi(token);
								ASE_GetToken(ase, token, sizeof(token));
								tris[idx].tidx[2] = atoi(token);
							}
						}	
						else
						{
							Con_Printf("Unknown MESH_FACE_LIST identifier: %s\n", token);
							ASE_SkipBlock(ase);
						}
						ASE_SkipLine(ase);
					}
				}
				else if (!strcmp(token, "*MESH_NUMTVERTEX"))
				{
					ASE_GetToken(ase, token, sizeof(token));
					if (!tverts)
					{
						numtverts = atoi(token);
						tverts = malloc(sizeof(*tverts) * numtverts);
						memset(tverts, 0, sizeof(*tverts) * numtverts);
					}
				}
				else if (!strcmp(token, "*MESH_TVERTLIST"))
				{
					ASE_SkipLine(ase);
					while (*ase->file)
					{
						ASE_GetToken(ase, token, sizeof(token));
						if (!strcmp(token, "}"))
							break;
						else if (!strcmp(token, "*MESH_TVERT"))
						{
							ASE_GetToken(ase, token, sizeof(token));
							idx = atoi(token);
							if (idx >= 0 && idx < numverts)
							{
								ASE_GetToken(ase, token, sizeof(token));
								tverts[idx].st[0] = atof(token);
								ASE_GetToken(ase, token, sizeof(token));
								tverts[idx].st[1] = atof(token);
								ASE_GetToken(ase, token, sizeof(token));
								tverts[idx].st[2] = atof(token);
							}
						}
						else
						{
							Con_Printf("Unknown MESH_TVERTLIST identifier: %s\n", token);
							ASE_SkipBlock(ase);
						}
						ASE_SkipLine(ase);
					}
				}
				else if (!strcmp(token, "*MESH_NUMTVFACES"))
					ASE_SkipBlock(ase);	//should be equal to MESH_NUMFACES
				else if (!strcmp(token, "*MESH_TFACELIST"))
				{
					ASE_SkipLine(ase);
					while (*ase->file)
					{
						ASE_GetToken(ase, token, sizeof(token));
						if (!strcmp(token, "}"))
							break;
						else if (!strcmp(token, "*MESH_TFACE"))
						{
							ASE_GetToken(ase, token, sizeof(token));
							idx = atoi(token);
							if (idx >= 0 && idx < numtris)
							{
								ASE_GetToken(ase, token, sizeof(token));
								tris[idx].tidx[0] = atoi(token);
								ASE_GetToken(ase, token, sizeof(token));
								tris[idx].tidx[1] = atoi(token);
								ASE_GetToken(ase, token, sizeof(token));
								tris[idx].tidx[2] = atoi(token);
							}
						}
						else
						{
							Con_Printf("Unknown MESH_TFACELIST identifier: %s\n", token);
							ASE_SkipBlock(ase);
						}
						ASE_SkipLine(ase);
					}
				}
				else
				{
					Con_Printf("Unknown top-level identifier: %s\n", token);
					ASE_SkipBlock(ase);
				}
				ASE_SkipLine(ase);
			}
		}
		else if (!strcmp(token, "*NODE_NAME") || !strcmp(token, "*NODE_TM")
			|| !strcmp(token, "*PROP_MOTIONBLUR")|| !strcmp(token, "*PROP_CASTSHADOW")|| !strcmp(token, "*PROP_RECVSHADOW"))
			ASE_SkipBlock(ase);
		else
		{
			Con_Printf("Unknown top-level identifier: %s\n", token);
			ASE_SkipBlock(ase);
		}
		ASE_SkipLine(ase);
	}

	//merge into a mesh
	if (materialidx >= 0 && materialidx < ase->nummaterials)
	{
		size_t v, tri, idx;
		struct asemesh_s *mesh;
		struct asevert_s asevert;
		mesh = ase->materials[materialidx].meshes;

		//don't let any single mesh exceed 65k verts. stuff bugs out then.
		if (!mesh || mesh->numverts + numtris*3 > 0xffff)
		{
			mesh = malloc(sizeof(*mesh));
			memset(mesh, 0, sizeof(*mesh));
			mesh->next = ase->materials[materialidx].meshes;
			ase->materials[materialidx].meshes = mesh;
		}

		//make sure there's going to be enough space
		if (mesh->numverts + numtris*3 > mesh->maxverts)
		{
			mesh->maxverts = (mesh->maxverts+numtris*3)*2;
			mesh->vert = realloc(mesh->vert, sizeof(*mesh->vert) * mesh->maxverts);
		}
		if (mesh->numindexes + numtris*3 > mesh->maxindexes)
		{
			mesh->maxindexes = (mesh->maxindexes+numtris*3)*2;
			mesh->index = realloc(mesh->index, sizeof(*mesh->index) * mesh->maxindexes);
		}

		//insert each triangle into the mesh
		for (tri = 0; tri < numtris; tri++)
		{
			for (v = 3; v --> 0; )
			{
				VectorCopy(verts[tris[tri].vidx[v]].xyz, asevert.xyz);
				VectorCopy(verts[tris[tri].vidx[v]].norm, asevert.norm);
				Vector2Copy(tverts[tris[tri].tidx[v]].st, asevert.st);

				for (idx = 0; idx < mesh->numverts; idx++)
				{
					if (!memcmp(&mesh->vert[idx], &asevert, sizeof(asevert)))
						break;
				}
				if (idx == mesh->numverts)
					mesh->vert[mesh->numverts++] = asevert;
				mesh->index[mesh->numindexes++] = idx;
			}
		}
	}

	free(verts);
	free(tverts);
	free(tris);
}

static void ASE_TopLevel(struct aseimport_s *ase)
{
	char token[1024];
	while (*ase->file)
	{
		ASE_GetToken(ase, token, sizeof(token));
		if (!strcmp(token, "*3DSMAX_ASCIIEXPORT"))
		{
			ASE_GetToken(ase, token, sizeof(token));	//version
		}
		else if (!strcmp(token, "*COMMENT") || !strcmp(token, "*SCENE"))
			ASE_SkipBlock(ase);
		else if (!strcmp(token, "*MATERIAL_LIST"))
			ASE_MaterialList(ase);
		else if (!strcmp(token, "*GEOMOBJECT"))
			ASE_GeomObject(ase);
		else
		{
			Con_Printf("Unknown top-level identifier: %s\n", token);
			ASE_SkipBlock(ase);
		}

		ASE_SkipLine(ase);
	}
}
static qboolean QDECL Mod_LoadASEModel (struct model_s *mod, void *buffer, size_t fsize)
{
	galiaspose_t *pose;
	galiasinfo_t *surf;
	size_t i, m;
	struct aseimport_s ase;
	memset(&ase, 0, sizeof(ase));
	ase.file = buffer;
	ASE_TopLevel(&ase);

	//we need to generate engine meshes and clean up any dynamic memory
	for (m = 0; m < ase.nummaterials; m++)
	{
		struct asemesh_s *mesh;
		while(ase.materials[m].meshes)
		{
			mesh = ase.materials[m].meshes;
			ase.materials[m].meshes = mesh->next;

			surf = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*surf));
			surf->nextsurf = mod->meshinfo;
			mod->meshinfo = surf;

			surf->numverts = mesh->numverts;
			surf->numindexes = mesh->numindexes;

			surf->ofs_indexes = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*surf->ofs_indexes) * mesh->numindexes);
			for (i = 0; i < mesh->numindexes; i++)
				surf->ofs_indexes[i] = mesh->index[i];

			surf->numanimations = 1;
			surf->ofsanimations = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*surf->ofsanimations));
			surf->ofsanimations->poseofs = pose = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*pose));
			surf->ofsanimations->loop = true;
			surf->ofsanimations->numposes = 1;
			surf->ofsanimations->rate = 10;

			pose->ofsverts = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*pose->ofsverts) * mesh->numverts);
			pose->ofsnormals = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*pose->ofsnormals) * mesh->numverts);
			surf->ofs_st_array = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*surf->ofs_st_array) * mesh->numverts);

			for (i = 0; i < mesh->numverts; i++)
			{
				VectorCopy(mesh->vert[i].xyz, pose->ofsverts[i]);
				VectorCopy(mesh->vert[i].norm, pose->ofsnormals[i]);
				Vector2Copy(mesh->vert[i].st, surf->ofs_st_array[i]);
			}

			surf->numskins = 1;
			surf->ofsskins = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*surf->ofsskins));
			surf->ofsskins->numframes = 1;
			surf->ofsskins->skinspeed = 0.1;
			surf->ofsskins->frame = modfuncs->ZG_Malloc(&mod->memgroup, sizeof(*surf->ofsskins->frame));
			Q_strlcpy(surf->ofsskins->frame->shadername, ase.materials[m].name, sizeof(surf->ofsskins->frame->shadername));
			Q_strlcpy(surf->ofsskins->name, ase.materials[m].name, sizeof(surf->ofsskins->name));

			free(mesh->vert);
			free(mesh->index);
			free(mesh);
		}
	}
	free(ase.materials);

	mod->type = mod_alias;
	return !!mod->meshinfo;
}
#endif



#ifdef LWOMODELS
static char *LWO_ReadString(unsigned char **file)
{	//strings are just null terminated.
	//however, they may have an extra byte of padding. yay shorts...
	//so skip the null and round up the length.
	char *ret = *file;
	size_t len = strlen(ret);
	*file = *file + ((len + 2)&~1);
	return ret;
}

static qboolean QDECL Mod_LoadLWOModel (struct model_s *mod, void *buffer, size_t fsize)
{
	unsigned char *file = buffer, *fileend;
	char *tags;
	vec3_t *points;
	size_t numpoints;
	unsigned int tagssize;
	size_t subsize;
	subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
	if (strncmp(file, "FORM", 4) || subsize+8 != fsize)
		return false;	//not an lwo, or corrupt, or something
	file += 8;
	fileend = file + subsize;
	if (strncmp(file, "LWO2", 4))
		return false;
	file += 4;
	while (file < fileend)
	{
		if (!strncmp(file, "TAGS", 4))
		{
			tagssize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			tags = file+8;
			file += 8+tagssize;
		}
		else if (!strncmp(file, "LAYR", 4))
		{
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			//fixme:
			file += 8+subsize;
		}
		else if (!strncmp(file, "PNTS", 4))
		{
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			numpoints = subsize / sizeof(vec3_t);
			points = (vec3_t*)(file+8);
			file += 8+subsize;
		}
		else if (!strncmp(file, "BBOX", 4))
		{
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			//we don't really care.
			file += 8+subsize;
		}
		else if (!strncmp(file, "VMAP", 4))
		{
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			if (!strcmp(file, "TXUV"))
			{
			}
//			else if (!strcmp(file, "RGBA") || !strcmp(file, "RGB"))
//			{
//			}
			file += 8+subsize;
		}
		else if (!strncmp(file, "POLS", 4))
		{
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			file += 8;
			if (!strcmp(file, "FACE") || !strcmp(file, "PTCH"))
			{

			}
			file += subsize;
		}
		else if (!strncmp(file, "PTAG", 4))
		{
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			//fixme:
			file += 8+subsize;
		}
		else if (!strncmp(file, "VMAD", 4))
		{
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			//fixme:
			file += 8+subsize;
		}
		else if (!strncmp(file, "SURF", 4))
		{
			char *surfend;
			char *surfname, *sourcename;
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			file += 8;
			surfend = file + subsize;
			surfname = LWO_ReadString(&file);
			sourcename = LWO_ReadString(&file);
			Con_Printf("surf=%s source=%s\n", surfname, sourcename);
			while (file < surfend)
			{
				if (!strncmp(file, "COLR", 4))
				{
					subsize = file[5] | (file[4]<<8);
					//fixme
					file += 6+subsize;
				}
				else if (!strncmp(file, "DIFF", 4))
				{
					subsize = file[5] | (file[4]<<8);
					//fixme
					file += 6+subsize;
				}
				else if (!strncmp(file, "SMAN", 4))
				{
					subsize = file[5] | (file[4]<<8);
					//fixme
					file += 6+subsize;
				}
				else if (!strncmp(file, "BLOK", 4))
				{
					char *blokend;
					subsize = file[5] | (file[4]<<8);
					file += 6;
					blokend = file + subsize;
					while (file < blokend)
					{
						if (!strncmp(file, "IMAP", 4))
							;
						else if (!strncmp(file, "TMAP", 4))
							;
						else if (!strncmp(file, "PROJ", 4))
							;
						else if (!strncmp(file, "AXIS", 4))
							;
						else if (!strncmp(file, "IMAG", 4))
							;
						else if (!strncmp(file, "WRAP", 4))
							;
						else if (!strncmp(file, "WRPW", 4))
							;
						else if (!strncmp(file, "WRPH", 4))
							;
						else if (!strncmp(file, "VMAP", 4))
							;
						else if (!strncmp(file, "AAST", 4))
							;
						else if (!strncmp(file, "PIXB", 4))
							;
						else
							Con_Printf("Unknown BLOK ID: %c%c%c%c\n", file[0], file[1], file[2], file[3]);
						subsize = file[5] | (file[4]<<8);
						file += 6+subsize;
					}
					file = blokend;
				}
				else
				{
					Con_Printf("Unknown SURF ID: %c%c%c%c\n", file[0], file[1], file[2], file[3]);
					subsize = file[5] | (file[4]<<8);
					file += 6+subsize;
				}
			}
			file = surfend;
		}
		else
		{
			Con_Printf("Unknown ID: %c%c%c%c\n", file[0], file[1], file[2], file[3]);
			subsize = file[7] | (file[6]<<8) | (file[5]<<16) | (file[4]<<24);
			file += 8+subsize;
			continue;
		}
	}

	return false;
}
#endif

extern qboolean QDECL Mod_LoadGLTFModel (struct model_s *mod, void *buffer, size_t fsize);
extern qboolean QDECL Mod_LoadGLBModel (struct model_s *mod, void *buffer, size_t fsize);
extern void Mod_ExportIQM(char *fname, int flags, galiasinfo_t *mesh);

#if !defined(SERVERONLY) && defined(SKELETALMODELS)
static void Mod_ExportIQM_f(void)
{
	char tok[128];
	model_t *mod;
	cmdfuncs->Argv(1, tok, sizeof(tok));
	mod = modfuncs->GetModel(tok, MLV_WARNSYNC);
	if (!mod || mod->type != mod_alias || !mod->meshinfo)
		Con_Printf("Couldn't load \"%s\"\n", tok);
	else
	{
		cmdfuncs->Argv(2, tok, sizeof(tok));
		Mod_ExportIQM(tok, mod->flags, mod->meshinfo);
	}
}
#endif

qboolean Plug_Init(void)
{
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (modfuncs && modfuncs->version != MODPLUGFUNCS_VERSION)
		modfuncs = NULL;

	if (modfuncs && filefuncs)
	{
#ifdef ASEMODELS
		modfuncs->RegisterModelFormatText("ASE models (ase)", "*3DSMAX_ASCIIEXPORT", Mod_LoadASEModel);
#endif
#ifdef LWOMODELS
		modfuncs->RegisterModelFormatMagic("LWO models (lwo)", (('M'<<24)+('R'<<16)+('O'<<8)+'F'), Mod_LoadLWOModel);
#endif
#ifdef GLTFMODELS
		Plug_GLTF_Init();
#endif

#if !defined(SERVERONLY) && defined(SKELETALMODELS)
		cmdfuncs->AddCommand("exportiqm", Mod_ExportIQM_f, NULL);
#endif
		return true;
	}
	return false;
}

