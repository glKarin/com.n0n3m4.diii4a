#include <zlib.h>
#include "../plugin.h"

static plugimagefuncs_t *imagefuncs = NULL;
static plugfsfuncs_t *filefuncs = NULL;

struct pendingtextureinfo *Image_ReadVTFFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize);

#ifndef FTEENGINE
qboolean COM_RequireExtension(char *path, const char *extension, int maxlen)
{
	qboolean okay = true;
	int plen = strlen(path);
	int elen = strlen(extension);

	//check if its aready suffixed
	if (plen >= elen)
	{
		if (!Q_strcasecmp(path+plen-elen, extension))
			return okay;
	}

	//truncate if required
	if (plen+1+elen > maxlen)
	{
		if (elen+1 > maxlen)
			Sys_Errorf("extension longer than path buffer");
		okay = false;
		plen = maxlen - 1+elen;
	}

	//do the copy
	while(*extension)
		path[plen++] = *extension++;
	path[plen] = 0;
	return okay;
}
#endif

static struct pendingtextureinfo *Image_ReadTTHFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	uint8_t num_mipmaps; // number of mipmaps in this texture
	uint32_t len_vtf_chunk; // size of uncompressed vtf chunk in header
	uint32_t len_vtf_file; // total size of uncompressed vtf file
	uint32_t len_ttz_tail; // compressed size of accompanying ttz file
	qbyte *vtf;
	qbyte *tail = NULL;
	size_t tailsize = 0;

	// check magic
	if (memcmp(filedata, "TTH\0", 4) != 0)
		return NULL;

	// check version
	if (filedata[4] != 1 || filedata[5] != 0)
		return NULL;

	// grab other data
	num_mipmaps = filedata[6];
	// aspect_flag skipped
	len_vtf_chunk = LittleLong(*(uint32_t *)&filedata[8]);

	// skip past mipmap flags to grab other other data
	len_vtf_file = LittleLong(*(uint32_t *)(filedata + 12 + (num_mipmaps * sizeof(uint64_t))));
	len_ttz_tail = LittleLong(*(uint32_t *)(filedata + 12 + (num_mipmaps * sizeof(uint64_t)) + 4));

	// load tail if it exists
	if (len_ttz_tail > 0)
	{
		// change extension
		char tailfilename[MAX_QPATH];
		size_t tailfilenamelen;
		Q_strlcpy(tailfilename, fname, sizeof(tailfilename));

		tailfilenamelen = strlen(tailfilename);
		if (tailfilename[tailfilenamelen - 1] == 'h')
			tailfilename[tailfilenamelen - 1] = 'z';
		else if (tailfilename[tailfilenamelen - 1] == 'H')
			tailfilename[tailfilenamelen - 1] = 'Z';
		else
			COM_RequireExtension(tailfilename, ".ttz", sizeof(tailfilename));

		// load tail
		tail = filefuncs->LoadFile(tailfilename, &tailsize);
		if (!tail)
		{
			Con_Printf("hl2: ERROR: couldn't load %s\n", tailfilename);
			return NULL;
		}

		// shouldn't happen
		if (tailsize != len_ttz_tail)
			Con_Printf("hl2: WARNING: %s size mismatch\n", tailfilename);
	}

	// allocate return vtf file
	vtf = plugfuncs->Malloc(len_vtf_file);
	if (!vtf)
		return NULL;

	// copy in vtf chunk
	memcpy(vtf, filedata + 12 + (num_mipmaps * sizeof(uint64_t)) + 8, len_vtf_chunk);

	// zlib decompress the tail onto the main vtf
	if (tail)
	{
		z_stream stream;

		memset(&stream, 0, sizeof(stream));
		stream.next_in = tail;
		stream.avail_in = len_ttz_tail;
		stream.next_out = vtf + len_vtf_chunk;
		stream.avail_out = len_vtf_file - len_vtf_chunk;

		inflateInit(&stream);
		inflate(&stream, Z_FINISH);
		inflateEnd(&stream);

		plugfuncs->Free(tail);
	}

	// now send it to the VTF loader
	return Image_ReadVTFFile(flags, fname, vtf, len_vtf_file);
}

static plugimageloaderfuncs_t tthfuncs =
{
	"Troika Texture File",
	sizeof(struct pendingtextureinfo),
	true,
	Image_ReadTTHFile,
};

qboolean TTH_Init(void)
{
	imagefuncs = plugfuncs->GetEngineInterface(plugimagefuncs_name, sizeof(*imagefuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!imagefuncs || !filefuncs)
		return false;
	return plugfuncs->ExportInterface(plugimageloaderfuncs_name, &tthfuncs, sizeof(tthfuncs));
}
