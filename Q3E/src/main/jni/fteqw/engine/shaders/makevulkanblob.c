#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../client/quakedef.h"
#include "../gl/shader.h"
void dumpprogblob(FILE *out, unsigned char *buf, unsigned int size)
{
	if (out)
	{
		fwrite(buf, 1, size, out);
		return;
	}
	else
		out = stdout;

	size_t totallen, i, linelen;
	totallen = 0;
	linelen = 32;
	fflush(out);
	fprintf(out, "\"");
	for (i=0;i<size;i++)
	{
		fprintf(out, "\\x%02X",buf[i]);
		if (i % linelen == linelen - 1)
			fprintf(out, "\"\n\"");
	}
	fprintf(out, "\"");
	fflush(out);
}

struct blobheader
{
	unsigned char blobmagic[4];	//\xffSPV
	unsigned int blobversion;
	unsigned int defaulttextures;	//s_diffuse etc flags
	unsigned int numtextures;		//s_t0 count
	unsigned int permutations;		//

	unsigned int cvarsoffset;	//double-null terminated string. I,XYZW prefixes
	unsigned int cvarslength;

	unsigned int vertoffset;
	unsigned int vertlength;
	unsigned int fragoffset;
	unsigned int fraglength;
};
void generateprogsblob(struct blobheader *prototype, FILE *out, FILE *vert, FILE *frag)
{
	struct blobheader *blob;
	int fraglen, vertlen, blobsize, cvarlen;
	cvarlen = prototype->cvarslength;
	cvarlen = (cvarlen + 3) & ~3;	//round up for padding.
	fseek(vert, 0, SEEK_END);
	fseek(frag, 0, SEEK_END);
	vertlen = ftell(vert);
	fraglen = ftell(frag);
	fseek(vert, 0, SEEK_SET);
	fseek(frag, 0, SEEK_SET);
	blobsize = sizeof(*blob) + cvarlen + fraglen + vertlen;
	blob = malloc(blobsize);
	*blob = *prototype;
	blob->cvarsoffset = sizeof(*blob);
	blob->cvarslength = prototype->cvarslength;	//unpadded length
	blob->vertoffset = blob->cvarsoffset+cvarlen;
	blob->vertlength = vertlen;
	blob->fragoffset = blob->vertoffset+vertlen;
	blob->fraglength = fraglen;
	memcpy((char*)blob+blob->cvarsoffset, (char*)prototype+prototype->cvarsoffset, prototype->cvarslength);
	fread((char*)blob+blob->vertoffset, blob->vertlength, 1, vert);
	fread((char*)blob+blob->fragoffset, blob->fraglength, 1, frag);
	dumpprogblob(out, (unsigned char*)blob, blobsize);
	free(blob);
}


int generatevulkanblobs(struct blobheader *blob, size_t maxblobsize, const char *glslname, int rayquery)
{
	char command[1024];
	char tempname[256];
	char tempvert[256];
	char tempfrag[256];
	char incpath[256], *sl;
	int inheader = 1;
	int i;
	unsigned short constid = 256;	//first few are reserved.

	const char *permutationnames[] =
	{
		"BUMP",
		"FULLBRIGHT",
		"UPPERLOWER",
		"REFLECTCUBEMASK",
		"SKELETAL",
		"FOG",
		"FRAMEBLEND",
		"LIGHTSTYLED",
		NULL
	};

	const char *tmppath = "/tmp/";

	char customsamplerlines[16][256];
	FILE *glsl, *temp;

	snprintf(tempname, sizeof(tempname), "%stemp.tmp", tmppath);
	snprintf(tempvert, sizeof(tempvert), "%stemp.vert", tmppath);
	snprintf(tempfrag, sizeof(tempfrag), "%stemp.frag", tmppath);

	memcpy(incpath, glslname, sizeof(incpath));
	if ((sl = strrchr(incpath, '/')))
		sl[1] = 0;
	else if ((sl = strrchr(incpath, '\\')))	//in case someone is on crappy windows.
		sl[1] = 0;

	memcpy(blob->blobmagic, "\xffSPV", 4);
	blob->blobversion = 1;
	blob->defaulttextures = 0;
	blob->numtextures = 0;
	blob->permutations = 0;
	blob->cvarsoffset = sizeof(*blob);
	blob->cvarslength = 0;

	if (!strncmp(glslname, "rq_", 3))
	{	//hack, to avoid copypasta
		rayquery = 2;
		glsl = fopen(glslname+3, "rt");
	}
	else
		glsl = fopen(glslname, "rt");
	if (!glsl)
	{
		printf("Unable to read %s\n", glslname);
		return 0;
	}
	temp = fopen(tempname, "wt");
	if (!temp)
		printf("Unable to write %s\n", tempname);
	while(fgets(command, sizeof(command), glsl))
	{
		if (inheader && !strncmp(command, "!!", 2))
		{
			if (!strncmp(command, "!!cvar", 6) || !strncmp(command, "!!arg", 5))
			{
				unsigned int type;
				unsigned int size;
				union
				{
					float f;
					unsigned int u;
				} u[4];
				char *arg;
				unsigned char *cb = (unsigned char*)blob + blob->cvarsoffset + blob->cvarslength;

				if (command[2] == 'a')
				{
					type = command[5] == 'i' || command[5] == 'f' || command[5] == 'b';
					size = type?1:(command[5]-'0');
					arg = strtok(command+7, " ,=\r\n");
					type = command[6-type] - 'a' + 'A';
				}
				else
				{
					type = command[6] == 'i' || command[6] == 'f' || command[6] == 'b';
					size = type?1:(command[6]-'0');
					arg = strtok(command+8, " ,=\r\n");
					type = command[7-type];
				}

				cb[0] = (constid>>8)&0xff;
				cb[1] = (constid>>0)&0xff;
				cb[2] = type;
				cb[3] = size + '0';
				cb += 4;
				while(*arg)
					*cb++ = *arg++;
				*cb++ = 0;

				for (i = 0; i < size; i++)
				{
					if (arg)
					{
						arg = strtok(NULL, " ,=\r\n");
						if (!arg)
						{
							printf(" cvar %s has no default value. Assuming 0\n",  (char*)blob + blob->cvarsoffset + blob->cvarslength+4);
							u[i].u = 0;	//0 either way.
						}
						else if (type == 'f' || type == 'F')
							u[i].f = atof(arg);
						else
							u[i].u = atoi(arg);
					}
					else
						u[i].u = 0;
					*cb++ = (u[i].u>>24)&0xff;
					*cb++ = (u[i].u>>16)&0xff;
					*cb++ = (u[i].u>>8)&0xff;
					*cb++ = (u[i].u>>0)&0xff;
				}
				blob->cvarslength = cb - ((unsigned char*)blob + blob->cvarsoffset);
				constid += size;
			}
			else if (!strncmp(command, "!!tess", 6))
				printf("!!tess not supported\n");
			else if (!strncmp(command, "!!geom", 6))
				printf("!!geom not supported\n");
			else if (!strncmp(command, "!!rayquery", 6))
				rayquery = true;
			else if (!strncmp(command, "!!permu", 7))
			{
				char *arg = strtok(command+7, " ,\r\n");
				for (i = 0; permutationnames[i]; i++)
				{
					if (!strcmp(arg, permutationnames[i]))
					{
						blob->permutations |= 1u<<i;
						break;
					}
				}
				if (!permutationnames[i])
				{
					printf("Unknown permutation: \"%s\"\n", arg);
					for (i = 0; permutationnames[i]; i++)
						printf("%s ", permutationnames[i]);
					printf("\n");
				}
			}
			else if (!strncmp(command, "!!samps", 7))
			{
				char *arg = strtok(command+7, " ,\r\n");
				do
				{
					//light
					if (!strcasecmp(arg, "shadowmap"))
						blob->defaulttextures |= 1u<<S_SHADOWMAP;
					else if (!strcasecmp(arg, "projectionmap"))
						blob->defaulttextures |= 1u<<S_PROJECTIONMAP;

					//material
					else if (!strcasecmp(arg, "diffuse"))
						blob->defaulttextures |= 1u<<S_DIFFUSE;
					else if (!strcasecmp(arg, "normalmap"))
						blob->defaulttextures |= 1u<<S_NORMALMAP;
					else if (!strcasecmp(arg, "specular"))
						blob->defaulttextures |= 1u<<S_SPECULAR;
					else if (!strcasecmp(arg, "upper"))
						blob->defaulttextures |= 1u<<S_UPPERMAP;
					else if (!strcasecmp(arg, "lower"))
						blob->defaulttextures |= 1u<<S_LOWERMAP;
					else if (!strcasecmp(arg, "fullbright"))
						blob->defaulttextures |= 1u<<S_FULLBRIGHT;
					else if (!strcasecmp(arg, "paletted"))
						blob->defaulttextures |= 1u<<S_PALETTED;
					else if (!strcasecmp(arg, "reflectcube"))
						blob->defaulttextures |= 1u<<S_REFLECTCUBE;
					else if (!strcasecmp(arg, "reflectmask"))
						blob->defaulttextures |= 1u<<S_REFLECTMASK;
					else if (!strcasecmp(arg, "displacement"))
						blob->defaulttextures |= 1u<<S_DISPLACEMENT;
					else if (!strcasecmp(arg, "occlusion"))
						blob->defaulttextures |= 1u<<S_OCCLUSION;
					else if (!strcasecmp(arg, "transmission"))
						blob->defaulttextures |= 1u<<S_TRANSMISSION;
					else if (!strcasecmp(arg, "thickness"))
						blob->defaulttextures |= 1u<<S_THICKNESS;

					//batch
					else if (!strcasecmp(arg, "lightmap"))
						blob->defaulttextures |= 1u<<S_LIGHTMAP0;
					else if (!strcasecmp(arg, "deluxemap") || !strcasecmp(arg, "deluxmap"))
						blob->defaulttextures |= 1u<<S_DELUXEMAP0;
					else if (!strcasecmp(arg, "lightmaps"))
						blob->defaulttextures |= 1u<<S_LIGHTMAP0 | 1u<<S_LIGHTMAP1 | 1u<<S_LIGHTMAP2 | 1u<<S_LIGHTMAP3;
					else if (!strcasecmp(arg, "deluxemaps") || !strcasecmp(arg, "deluxmaps"))
						blob->defaulttextures |= 1u<<S_DELUXEMAP0 | 1u<<S_DELUXEMAP1 | 1u<<S_DELUXEMAP2 | 1u<<S_DELUXEMAP3;

					//shader pass
					else if ((i=atoi(arg)))
					{	//legacy
						if (blob->numtextures < i)
							blob->numtextures = i;
					}
					else
					{
						char *eq = strrchr(arg, '=');
						if (eq)
						{
							char *type = strrchr(arg, ':');
							int pass = atoi(eq+1);
							*eq = 0;
							if (type)
								*type++ = 0;
							else
								type = "2D";
							if (pass < 16)
							{
								snprintf(customsamplerlines[pass], sizeof(customsamplerlines[pass]), "uniform sampler%s s_%s;\n", type, arg);
								if (blob->numtextures < ++pass)
									blob->numtextures = pass;
							}
							else printf("sampler binding too high:   %s:%s=%i\n", arg, type, pass);
						}
						else
							printf("Unknown texture: \"%s\"\n", arg);
					}
				} while((arg = strtok(NULL, " ,\r\n")));
			}
			continue;
		}
		else if (inheader && !strncmp(command, "//", 2))
			continue;
		else if (inheader)
		{
			const char *specialnames[] =
			{
				//light
				"uniform sampler2DShadow s_shadowmap;\n",
				"uniform samplerCube s_projectionmap;\n",

				//material
				"uniform sampler2D s_diffuse;\n",
				"uniform sampler2D s_normalmap;\n",
				"uniform sampler2D s_specular;\n",
				"uniform sampler2D s_upper;\n",
				"uniform sampler2D s_lower;\n",
				"uniform sampler2D s_fullbright;\n",
				"uniform sampler2D s_paletted;\n",
				"uniform samplerCube s_reflectcube;\n",
				"uniform sampler2D s_reflectmask;\n",
				"uniform sampler2D s_displacement;\n",
				"uniform sampler2D s_occlusion;\n",
				"uniform sampler2D s_transmission;\n",
				"uniform sampler2D s_thickness;\n",

				//batch
				"uniform sampler2D s_lightmap;\n#define s_lightmap0 s_lightmap\n",
				"uniform sampler2D s_deluxemap;\n#define s_deluxemap0 s_deluxemap\n",
				"uniform sampler2D s_lightmap1;\n",
				"uniform sampler2D s_lightmap2;\n",
				"uniform sampler2D s_lightmap3;\n",
				"uniform sampler2D s_deluxmap1;\n",
				"uniform sampler2D s_deluxmap2;\n",
				"uniform sampler2D s_deluxmap3;\n"
			};
			int binding = 2;	//defined in sys/defs.h
			inheader = 0;
			if (rayquery == 2)
				blob->defaulttextures &= ~(1u<<S_SHADOWMAP);	//part of the earlier hack.
			if (rayquery)
				fprintf(temp, "#define RAY_QUERY\n");
			fprintf(temp, "#define s_deluxmap s_deluxemap\n");
			fprintf(temp, "#define OFFSETMAPPING (cvar_r_glsl_offsetmapping>0)\n");
			fprintf(temp, "#define SPECULAR (cvar_gl_specular>0)\n");
			fprintf(temp, "#ifdef FRAGMENT_SHADER\n");
			if (rayquery)
				fprintf(temp, "layout(set=0, binding=%u) uniform accelerationStructureEXT toplevelaccel;\n", binding++);
			for (i = 0; i < sizeof(specialnames)/sizeof(specialnames[0]); i++)
			{
				if (blob->defaulttextures & (1u<<i))
					fprintf(temp, "layout(set=0, binding=%u) %s", binding++, specialnames[i]);
			}
			for (i = 0; i < blob->numtextures; i++)
			{
				fprintf(temp, "layout(set=0, binding=%u) ", binding++);
				if (*customsamplerlines[i])
					fprintf(temp, "%s", customsamplerlines[i]);
				else
					fprintf(temp, "uniform sampler2D s_t%u;\n", i);
			}
			fprintf(temp, "#endif\n");

			//cvar specialisation constants
			{
				unsigned char *cb = (unsigned char*)blob + blob->cvarsoffset;
				while (cb < (unsigned char*)blob + blob->cvarsoffset + blob->cvarslength)
				{
					union
					{
						float f;
						unsigned int u;
					} u[4];
					unsigned short id;
					unsigned char type;
					unsigned char size;
					char *name;
					id = *cb++<<8;
					id |= *cb++;
					type = *cb++;
					size = (*cb++)-'0';
					name = cb;
					cb += strlen(name)+1;
					for (i = 0; i < size; i++)
					{
						u[i].u = (cb[0]<<24)|(cb[1]<<16)|(cb[2]<<8)|(cb[3]<<0);
						cb+=4;
					}
#if 0 //all is well
					if (size == 1 && type == 'b')
						fprintf(temp, "layout(constant_id=%u) const bool cvar_%s = %s;\n", id, name, (int)u[0].u?"true":"false");
					else if (size == 1 && type == 'i')
						fprintf(temp, "layout(constant_id=%u) const int cvar_%s = %i;\n", id, name, (int)u[0].u);
					else if (size == 1 && type == 'f')
						fprintf(temp, "layout(constant_id=%u) const float cvar_%s = %f;\n", id, name, u[0].f);
					else if (size == 3 && type == 'f')
					{
						fprintf(temp, "layout(constant_id=%u) const float cvar_%s_x = %f;\n", id+0, name, u[0].f);
						fprintf(temp, "layout(constant_id=%u) const float cvar_%s_y = %f;\n", id+1, name, u[1].f);
						fprintf(temp, "layout(constant_id=%u) const float cvar_%s_z = %f;\n", id+2, name, u[2].f);
						fprintf(temp, "vec3 cvar_%s = vec3(cvar_%s_x, cvar_%s_y, cvar_%s_z);\n", name, name, name, name);
					}
					else 	if (size == 1 && type == 'B')
						fprintf(temp, "layout(constant_id=%u) const bool arg_%s = %s;\n", id, name, (int)u[0].u?"true":"false");
					else 	if (size == 1 && type == 'I')
						fprintf(temp, "layout(constant_id=%u) const int arg_%s = %i;\n", id, name, (int)u[0].u);
					else if (size == 1 && type == 'F')
						fprintf(temp, "layout(constant_id=%u) const float arg_%s = %i;\n", id, name, u[0].f);
					else if (size == 3 && type == 'F')
					{
						fprintf(temp, "layout(constant_id=%u) const float arg_%s_x = %f;\n", id+0, name, u[0].f);
						fprintf(temp, "layout(constant_id=%u) const float arg_%s_y = %f;\n", id+1, name, u[1].f);
						fprintf(temp, "layout(constant_id=%u) const float arg_%s_z = %f;\n", id+2, name, u[2].f);
						fprintf(temp, "vec3 arg_%s = vec3(arg_%s_x, arg_%s_y, arg_%s_z);\n", name, name, name, name);
					}
#else
					//these initialised values are fucked up because glslangvalidator's spirv generator is fucked up and folds specialisation constants.
					//we get around this by ensuring that all such constants are given unique values to prevent them being folded, with the engine overriding everything explicitly.
					if (size == 1 && type == 'b')
					{
						fprintf(temp, "layout(constant_id=%u) const int _cvar_%s = %i;\n", id, name, id);//(int)u[0].u?"true":"false");
						fprintf(temp, "#define cvar_%s (_cvar_%s!=0)\n", name, name);
					}
					else if (size == 1 && type == 'i')
						fprintf(temp, "layout(constant_id=%u) const int cvar_%s = %i;\n", id, name, id);//(int)u[0].u);
					else if (size == 1 && type == 'f')
						fprintf(temp, "layout(constant_id=%u) const float cvar_%s = %i;\n", id, name, id);//u[0].f);
					else if (size == 3 && type == 'f')
					{
						fprintf(temp, "layout(constant_id=%u) const float cvar_%s_x = %i;\n", id+0, name, id+0);//u[0].f);
						fprintf(temp, "layout(constant_id=%u) const float cvar_%s_y = %i;\n", id+1, name, id+1);//u[1].f);
						fprintf(temp, "layout(constant_id=%u) const float cvar_%s_z = %i;\n", id+2, name, id+2);//u[2].f);
						fprintf(temp, "vec3 cvar_%s = vec3(cvar_%s_x, cvar_%s_y, cvar_%s_z);\n", name, name, name, name);
					}
					else 	if (size == 1 && type == 'B')
					{
						fprintf(temp, "layout(constant_id=%u) const int _arg_%s = %i;\n", id, name, id);//(int)u[0].u?"true":"false");
						fprintf(temp, "#define arg_%s (_arg_%s!=0)\n", name, name);
					}
					else 	if (size == 1 && type == 'I')
						fprintf(temp, "layout(constant_id=%u) const int arg_%s = %i;\n", id, name, id);//(int)u[0].u);
					else if (size == 1 && type == 'F')
						fprintf(temp, "layout(constant_id=%u) const float arg_%s = %i;\n", id, name, id);//u[0].f);
					else if (size == 3 && type == 'F')
					{
						fprintf(temp, "layout(constant_id=%u) const float arg_%s_x = %i;\n", id+0, name, id+0);//u[0].f);
						fprintf(temp, "layout(constant_id=%u) const float arg_%s_y = %i;\n", id+1, name, id+1);//u[1].f);
						fprintf(temp, "layout(constant_id=%u) const float arg_%s_z = %i;\n", id+2, name, id+2);//u[2].f);
						fprintf(temp, "vec3 arg_%s = vec3(arg_%s_x, arg_%s_y, arg_%s_z);\n", name, name, name, name);
					}
#endif
				}
			}
			//permutation stuff
			for (i = 0; i < sizeof(specialnames)/sizeof(specialnames[0]); i++)
			{
				if (blob->permutations & (1<<i))
				{
#if 0 //all is well
					fprintf(temp, "layout(constant_id=%u) const bool %s = %s;\n", 16+i, permutationnames[i], "false");
#else
					fprintf(temp, "layout(constant_id=%u) const int _%s = %i;\n", 16+i, permutationnames[i], 16+i);
					fprintf(temp, "#define %s (_%s!=0)\n", permutationnames[i], permutationnames[i]);
#endif
				}
			}
		}
		fputs(command, temp);
	}
	fclose(temp);
	fclose(glsl);

	temp = fopen(tempvert, "wt");
	fprintf(temp, "#version 460 core\n");
	fclose(temp);

	temp = fopen(tempfrag, "wt");
	fprintf(temp, "#version 460 core\n");
	if (rayquery)
		fprintf(temp, "#extension GL_EXT_ray_query : require\n");
	fclose(temp);

	snprintf(command, sizeof(command),
		/*preprocess the vertex shader*/
		"cpp %s -I%s -DVULKAN -DVERTEX_SHADER -P >> %s && "

		/*preprocess the fragment shader*/
		"cpp %s -I%s -DVULKAN -DFRAGMENT_SHADER -P >> %s && "

		/*convert to spir-v (annoyingly we have no control over the output file names*/
		"glslangValidator -g0 -V -l -d %s %s"

		/*strip stuff out, so drivers don't glitch out from stuff that we don't use*/
//		" && spirv-remap -i vert.spv frag.spv -o vulkan/remap"

		,tempname, incpath, tempvert	//vertex shader args
		,tempname, incpath, tempfrag	//fragment shader args
		,tempvert, tempfrag);			//compile/link args.

	system(command);

//	remove(tempname);
//	remove(tempvert);
//	remove(tempfrag);

	if (rayquery)
		blob->permutations |= 1u<<31;
	return 1;
}

int main(int argc, const char **argv)
{
	const char *inname = argv[1];
	const char *blobname = argv[2];
	FILE *v, *f, *o;
	char proto[8192];
	char line[256];
	int rayquery = (argc>=4)?!!strstr(argv[3], "rq"):0;
	int r = 1;

	if (argc == 1)
	{
		printf("%s input.glsl output.fvb\n", argv[0]);
		return 1;
	}

	if (!generatevulkanblobs((struct blobheader*)proto, sizeof(proto), inname, rayquery))
		return 1;
	//should have generated two files
	v = fopen("vert.spv", "rb");
	f = fopen("frag.spv", "rb");
	if (f && v)
	{
		if (argc < 3)
		{
			generateprogsblob((struct blobheader*)proto, NULL, v, f);
			r = 0;
		}
		else
		{
			o = fopen(blobname, "wb");
			if (o)
			{
				generateprogsblob((struct blobheader*)proto, o, v, f);
				fclose(o);
				r = 0;
			}
			else
				printf("Unable to write blob %s\n", blobname);
		}
	}
	if (f)
		fclose(f);
	else
		printf("Unable to read frag.spv\n");
	if (v)
		fclose(v);
	else
		printf("Unable to read vert.spv\n");
	remove("vert.spv");
	remove("frag.spv");

	return r;
}
