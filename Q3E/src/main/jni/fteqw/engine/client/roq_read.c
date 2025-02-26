/* ------------------------------------------------------------------------
 * Id Software's RoQ video file format decoder
 *
 * Dr. Tim Ferguson, 2001.
 * For more details on the algorithm:
 *         http://www.csse.monash.edu.au/~timf/videocodec.html
 *
 * This is a simple decoder for the Id Software RoQ video format.  In
 * this format, audio samples are DPCM coded and the video frames are
 * coded using motion blocks and vector quantisation.
 *
 * Note: All information on the RoQ file format has been obtained through
 *   pure reverse engineering.  This was achieved by giving known input
 *   audio and video frames to the roq.exe encoder and analysing the
 *   resulting output text and RoQ file.  No decompiling of the Quake III
 *   Arena game was required.
 *
 * You may freely use this source code.  I only ask that you reference its
 * source in your projects documentation:
 *       Tim Ferguson: http://www.csse.monash.edu.au/~timf/
 * ------------------------------------------------------------------------ */

#include "quakedef.h"

#if defined(HAVE_MEDIA_DECODER) && defined(Q3CLIENT)


static int VFS_GETC(vfsfile_t *fp)
{
	unsigned char c;
	VFS_READ(fp, &c, 1);
	return c;
}

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include "roq.h"

//#define DBUG	1

#define FAST

/* -------------------------------------------------------------------------- */
static unsigned int get_word(vfsfile_t *fp)
{
unsigned int ret;

	ret  = ((VFS_GETC(fp)) & 0xff);
	ret |= ((VFS_GETC(fp)) & 0xff) << 8;
	return(ret);
}


/* -------------------------------------------------------------------------- */
static unsigned long get_long(vfsfile_t *fp)
{
unsigned long ret;

	ret  = ((VFS_GETC(fp)) & 0xff);
	ret |= ((VFS_GETC(fp)) & 0xff) << 8;
	ret |= ((VFS_GETC(fp)) & 0xff) << 16;
	ret |= ((VFS_GETC(fp)) & 0xff) << 24;
	return(ret);
}


/* -------------------------------------------------------------------------- */
static int roq_parse_file(vfsfile_t *fp, roq_info *ri)
{
unsigned int head1, head3, chunk_id;//, chunk_arg;
long head2, chunk_size;
qofs_t fpos;
#ifndef FAST
int max_frame;
#endif

#ifndef FAST
	ri->num_audio_bytes = ri->num_frames = max_frame = 0;
	ri->audio_channels = 0;
	ri->frame_offset = NULL;
#endif
	ri->buf_size = 0;
	head1 = get_word(fp);
	head2 = get_long(fp);
	head3 = get_word(fp);
	if(head1 != 0x1084 && head2 != 0xffffffff && head3 != 0x1e)
	{
		return 1;
	}

	ri->roq_start = fpos = VFS_TELL(fp);
	while(fpos+8 <= ri->maxpos)
	{
#if DBUG > 20
		Con_Printf("---------------------------------------------------------------------------\n");
#endif
		VFS_SEEK(fp, fpos);

		chunk_id = get_word(fp);
		chunk_size = get_long(fp);
		/*chunk_arg =*/ get_word(fp);
		fpos += 8 + chunk_size;
		if (chunk_size == 0xffffffff || fpos > ri->maxpos)	//FIXME: THIS SHOULD NOT HAPPEN
			break;
		if (chunk_size > ri->buf_size)
			ri->buf_size = chunk_size;
#if DBUG > 20
		Con_Printf("%03d  0x%06lx: chunk: 0x%02x size: %ld  cells: 2x2=%d,4x4=%d\n", i,
			fpos, chunk_id, chunk_size, v1>>8,v1&0xff);
#endif

		if(chunk_id == RoQ_INFO)		/* video info */
		{
			ri->width = get_word(fp);
			ri->height = get_word(fp);
			get_word(fp);
			get_word(fp);
#ifdef FAST
			return 0;	//we have all the data we need now. We always find a sound chunk first, or none at all.
#endif
		}
#ifndef FAST
		else if(chunk_id == RoQ_QUAD_VQ)
		{
			ri->num_frames++;
			if(ri->num_frames > max_frame)
			{
				max_frame += 5000;
					if((ri->frame_offset = BZ_Realloc(ri->frame_offset, sizeof(long) * max_frame)) == NULL)
						return 1;
				}
				ri->frame_offset[ri->num_frames] = fpos;
			}
		}
#endif
		else if(chunk_id == RoQ_SOUND_MONO || chunk_id == RoQ_SOUND_STEREO)
		{
			if(chunk_id == RoQ_SOUND_MONO)
				ri->audio_channels = 1;
			else
				ri->audio_channels = 2;
#ifndef FAST
			ri->num_audio_bytes += chunk_size;
#endif
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
static void apply_vector_2x2(roq_info *ri, int x, int y, roq_cell_rgba *cell)
{
	// place 2x2 vector codeword in framebuffer

	int idxa = (y * ri->width) + x;
	int idxb = 0;

	int *ptra = (int*) ri->rgba[0][idxa];
	int *ptrb = (int*) &cell->p[idxb];

	ptra[0] = ptrb[0];
	ptra[1] = ptrb[1];
	ptra += ri->width;
	ptra[0] = ptrb[2];
	ptra[1] = ptrb[3];
}

/* -------------------------------------------------------------------------- */
static void apply_vector_4x4(roq_info *ri, int x, int y, roq_cell_rgba *cell)
{
	// upsample 2x2 vector codeword to 4x4 and place in framebuffer

	int idxa = (y * ri->width) + x;
	int idxb = 0;

	int *ptra = (int*) ri->rgba[0][idxa];
	int *ptrb = (int*) &cell->p[idxb];

	int i;
	for(i = 0; i < 4; i++) {
		ptra[0] = ptrb[0];
		ptra[1] = ptrb[0];
		ptra[2] = ptrb[1];
		ptra[3] = ptrb[1];

		ptra += ri->width;
		if(i & 0x1) { // increase src pointer only every second dest line
			ptrb += 2;
		}
	}
}


/* -------------------------------------------------------------------------- */
static void apply_motion_4x4(roq_info *ri, int x, int y, unsigned char mv, char mean_x, char mean_y)
{
	int mx = x + 8 - (mv >> 4) - mean_x;
	int my = y + 8 - (mv & 0xf) - mean_y;
	
	int idxa = (y * ri->width) + x;
	int idxb = (my * ri->width) + mx;

	int *ptra = (int*) ri->rgba[0][idxa];
	int *ptrb = (int*) ri->rgba[1][idxb];
	
	int i;
	for(i = 0; i < 4; i++) {
		ptra[0] = ptrb[0];
		ptra[1] = ptrb[1];
		ptra[2] = ptrb[2];
		ptra[3] = ptrb[3];

		ptra += ri->width;
		ptrb += ri->width;
	}
}


/* -------------------------------------------------------------------------- */
static void apply_motion_8x8(roq_info *ri, int x, int y, unsigned char mv, char mean_x, char mean_y)
{
	int mx = x + 8 - (mv >> 4) - mean_x;
	int my = y + 8 - (mv & 0xf) - mean_y;

	int idxa = (y * ri->width) + x;
	int idxb = (my * ri->width) + mx;
	
	int *ptra = (int*) ri->rgba[0][idxa];
	int *ptrb = (int*) ri->rgba[1][idxb];

	int i;
	for(i = 0; i < 8; i++) {
		ptra[0] = ptrb[0];
		ptra[1] = ptrb[1];
		ptra[2] = ptrb[2];
		ptra[3] = ptrb[3];
		ptra[4] = ptrb[4];
		ptra[5] = ptrb[5];
		ptra[6] = ptrb[6];
		ptra[7] = ptrb[7];

		ptra += ri->width;
		ptrb += ri->width;
	}
}


/* -------------------------------------------------------------------------- */
roq_info *roq_open(char *fname)
{
vfsfile_t *fp;
roq_info *ri;
int i;

	if((fp = FS_OpenVFS(fname, "rb", FS_GAME)) == NULL)
	{
		if((fp = FS_OpenVFS(va("video/%s", fname), "rb", FS_GAME)) == NULL)	//for q3 compat
			return NULL;
	}

	if((ri = BZF_Malloc(sizeof(roq_info))) == NULL)
	{
		Con_Printf("Error allocating memory.\n");
		return NULL;
	}

	memset(ri, 0, sizeof(roq_info));

	ri->maxpos = VFS_TELL(fp)+VFS_GETLEN(fp);//no adds/subracts for fileoffset here

	ri->fp = fp;
	if(roq_parse_file(fp, ri))
		return NULL;
#ifndef FAST
	ri->stream_length = (ri->num_frames * 1000)/30;
#endif
	for(i = 0; i < 128; i++)
	{
		ri->snd_sqr_arr[i] = i * i;
		ri->snd_sqr_arr[i + 128] = -(i * i);
	}

	for(i = 0; i < 2; i++)
	{
		if((ri->rgba[i] = BZF_Malloc(ri->width * ri->height * sizeof(byte_vec4_t))) == NULL)
		{
			Con_Printf("Memory allocation error.\n");
			return NULL;
		}
	}

	ri->buf_size *= 2;
	if((ri->buf = BZF_Malloc(ri->buf_size)) == NULL)
	{
		Con_Printf("Memory allocation error.\n");
		return NULL;
	}
	ri->audio_buf_size = 0;
	ri->audio = NULL;

	ri->frame_num = 0;
	ri->aud_pos = ri->vid_pos = ri->roq_start;

	return ri;
}

//reset enough that we'll start decoding from the start next time we try to read a frame.
void roq_rewind(roq_info *ri)
{
	ri->frame_num = 0;
	ri->aud_pos = ri->vid_pos = ri->roq_start;
}

/* -------------------------------------------------------------------------- */
void roq_close(roq_info *ri)
{
int i;

	if(ri == NULL)
		return;
	VFS_CLOSE(ri->fp);
	for(i = 0; i < 2; i++)
	{
		if(ri->rgba[i] != NULL)
			BZ_Free(ri->rgba[i]);
	}
	if(ri->buf != NULL)
		BZ_Free(ri->buf);
	if (ri->audio)
		BZ_Free(ri->audio);
	BZ_Free(ri);
}

/* -------------------------------------------------------------------------- */
#define LIMIT(x) ((((x) > 0xffffff) ? 0xff0000 : (((x) <= 0xffff) ? 0 : (x) & 0xff0000)) >> 16)
void roq_cells_to_rgba(roq_info *ri)
{
	unsigned char *pptr;
	int i, r, g, b, y, u, v, t;
	for(i = 0; i < 256; i++) {
		pptr = ri->cells_rgba[i].p;

		u = ri->cells[i].u - 128;
		v = ri->cells[i].v - 128;

		r = 91881 * v;
		g = -22554 * u + -46802 * v;
		b = 116130 * u;

		// first pixel
		y = (ri->cells[i].y0) << 16;
		t = r + y;
		pptr[0] = LIMIT(t); // R
		t = g + y;
		pptr[1] = LIMIT(t); // G
		t = b + y;
		pptr[2] = LIMIT(t); // B
		pptr[3] = 255;      // A

		// second pixel
		y = (ri->cells[i].y1) << 16;
		t = r + y;
		pptr[4] = LIMIT(t); // R
		t = g + y;
		pptr[5] = LIMIT(t); // G
		t = b + y;
		pptr[6] = LIMIT(t); // B
		pptr[7] = 255;      // A

		// third pixel
		y = (ri->cells[i].y2) << 16;
		t = r + y;
		pptr[8] = LIMIT(t); // R
		t = g + y;
		pptr[9] = LIMIT(t); // G
		t = b + y;
		pptr[10] = LIMIT(t); // B
		pptr[11] = 255;      // A

		// fourth pixel
		y = (ri->cells[i].y3) << 16;
		t = r + y;
		pptr[12] = LIMIT(t); // R
		t = g + y;
		pptr[13] = LIMIT(t); // G
		t = b + y;
		pptr[14] = LIMIT(t); // B
		pptr[15] = 255;      // A
	}
}


/* -------------------------------------------------------------------------- */
int roq_read_frame(roq_info *ri)
{
vfsfile_t *fp = ri->fp;
unsigned int chunk_id = 0, chunk_arg = 0;
unsigned long chunk_size = 0;
int i, j, k, nv1, nv2, vqflg = 0, vqflg_pos = -1, vqid, bpos, xpos, ypos, xp, yp, x, y;
unsigned char *buf;
int frame_stats[2][4] = {{0},{0}};
roq_qcell *qcell;

qofs_t fpos = ri->vid_pos;

	VFS_SEEK(fp, fpos);
	while(fpos+8 < ri->maxpos)
	{
		chunk_id = get_word(fp);
		chunk_size = get_long(fp);
		chunk_arg = get_word(fp);
 		fpos += 8+chunk_size;
		if (chunk_size == 0xffffffff || fpos > ri->maxpos)
			return -1;
		if (chunk_id == RoQ_QUAD_VQ)
			break;
		if(chunk_id == RoQ_QUAD_CODEBOOK)
		{
			if((nv1 = chunk_arg >> 8) == 0)
				nv1 = 256;
			if((nv2 = chunk_arg & 0xff) == 0 && nv1 * 6 < chunk_size)
				nv2 = 256;
			VFS_READ(fp, ri->cells, nv1 * sizeof(roq_cell));
			roq_cells_to_rgba(ri);
			for(i = 0; i < nv2; i++)
				for(j = 0; j < 4; j++) ri->qcells[i].idx[j] = VFS_GETC(fp);
		}
		else
			VFS_SEEK(fp, fpos);
	}

	if(chunk_id != RoQ_QUAD_VQ)
	{
		ri->vid_pos = fpos;
		return 0;
	}

	ri->frame_num++;
	if(ri->buf_size < chunk_size)
	{
		ri->buf_size *= 2;
		if (ri->buf_size < chunk_size)	//double wasn't enough
			ri->buf_size = chunk_size;
		BZ_Free(ri->buf);
		if((ri->buf = BZ_Malloc(ri->buf_size)) == NULL)
		{
			Con_Printf("Memory allocation error.\n");
			return -1;
		}
	}
	VFS_READ(fp, ri->buf, chunk_size);
	buf = ri->buf;

	bpos = xpos = ypos = 0;
	while(bpos < chunk_size)
	{
		for(yp = ypos; yp < ypos + 16; yp += 8)
			for(xp = xpos; xp < xpos + 16; xp += 8)
			{
				if(vqflg_pos < 0)
				{
					vqflg = buf[bpos++]; vqflg |= (buf[bpos++] << 8);
					vqflg_pos = 7;
				}
				vqid = (vqflg >> (vqflg_pos * 2)) & 0x3;
				frame_stats[0][vqid]++;
				vqflg_pos--;

				switch(vqid)
				{
					case RoQ_ID_MOT: break;
					case RoQ_ID_FCC:
						apply_motion_8x8(ri, xp, yp, buf[bpos], (char)(chunk_arg >> 8), (char)(chunk_arg & 0xff));
						bpos++;
						break;
					case RoQ_ID_SLD:
						qcell = ri->qcells + buf[bpos++];
						apply_vector_4x4(ri, xp, yp, ri->cells_rgba + qcell->idx[0]);
						apply_vector_4x4(ri, xp+4, yp, ri->cells_rgba + qcell->idx[1]);
						apply_vector_4x4(ri, xp, yp+4, ri->cells_rgba + qcell->idx[2]);
						apply_vector_4x4(ri, xp+4, yp+4, ri->cells_rgba + qcell->idx[3]);
						break;
					case RoQ_ID_CCC:
						for(k = 0; k < 4; k++)
						{
							x = xp; y = yp;
							if(k & 0x01) x += 4;
							if(k & 0x02) y += 4;

							if(vqflg_pos < 0)
							{
								vqflg = buf[bpos++]; vqflg |= (buf[bpos++] << 8);
								vqflg_pos = 7;
							}
							vqid = (vqflg >> (vqflg_pos * 2)) & 0x3;
							frame_stats[1][vqid]++;
							vqflg_pos--;
							switch(vqid)
							{
								case RoQ_ID_MOT: break;
								case RoQ_ID_FCC:
									apply_motion_4x4(ri, x, y, buf[bpos], (char)(chunk_arg >> 8), (char)(chunk_arg & 0xff));
									bpos++;
									break;
								case RoQ_ID_SLD:
									qcell = ri->qcells + buf[bpos++];
									apply_vector_2x2(ri, x, y, ri->cells_rgba + qcell->idx[0]);
									apply_vector_2x2(ri, x+2, y, ri->cells_rgba + qcell->idx[1]);
									apply_vector_2x2(ri, x, y+2, ri->cells_rgba + qcell->idx[2]);
									apply_vector_2x2(ri, x+2, y+2, ri->cells_rgba + qcell->idx[3]);
									break;
								case RoQ_ID_CCC:
									apply_vector_2x2(ri, x, y, ri->cells_rgba + buf[bpos]);
									apply_vector_2x2(ri, x+2, y, ri->cells_rgba + buf[bpos+1]);
									apply_vector_2x2(ri, x, y+2, ri->cells_rgba + buf[bpos+2]);
									apply_vector_2x2(ri, x+2, y+2, ri->cells_rgba + buf[bpos+3]);
									bpos += 4;
									break;
							}
						}
						break;
					default:
						Con_Printf("Unknown vq code: %d\n", vqid);
				}
			}

		xpos += 16;
		if(xpos >= ri->width)
		{
			xpos -= ri->width;
			ypos += 16;
		}
		if(ypos >= ri->height) break;
	}

#if 0
	frame_stats[0][3] = 0;
	Con_Printf("<%d  0x%04x -> %d,%d>\n", ri->frame_num, chunk_arg, (char)(chunk_arg >> 8), (char)(chunk_arg & 0xff));
	Con_Printf("for 08x08 CCC = %d, FCC = %d, MOT = %d, SLD = %d, PAT = 0\n", frame_stats[0][3], frame_stats[0][1], frame_stats[0][0], frame_stats[0][2]);
	Con_Printf("for 04x04 CCC = %d, FCC = %d, MOT = %d, SLD = %d, PAT = 0\n", frame_stats[1][3], frame_stats[1][1], frame_stats[1][0], frame_stats[1][2]);
#endif

	ri->vid_pos = fpos;

	if(ri->frame_num == 1)
	{
		memcpy(ri->rgba[1], ri->rgba[0], ri->width * ri->height * sizeof(byte_vec4_t));
	}
	else
	{
		byte_vec4_t *tp = ri->rgba[0];
		ri->rgba[0] = ri->rgba[1];
		ri->rgba[1] = tp;
	}

	return 1;
}


/* -------------------------------------------------------------------------- */
int roq_read_audio(roq_info *ri)
{
vfsfile_t *fp = ri->fp;
unsigned int chunk_id = 0, chunk_arg = 0;
unsigned long chunk_size = 0;
int i, snd_left, snd_right;

long fpos;

	fpos = ri->aud_pos;

	ri->audio_size = 0;

	for(;;)
	{
		VFS_SEEK(fp, fpos);
		if(fpos >= ri->maxpos)
			return -1;
		chunk_id = get_word(fp);
		chunk_size = get_long(fp);
		chunk_arg = get_word(fp);
		fpos += 8+chunk_size;
		if (chunk_size == 0xffffffff || fpos > ri->maxpos)
			return -1;
		if (chunk_id == RoQ_SOUND_MONO || chunk_id == RoQ_SOUND_STEREO)
			break;
	}

	if(ri->audio_buf_size < chunk_size*2)
	{
		if(ri->audio != NULL) BZ_Free(ri->audio);
		ri->audio=NULL;
		ri->audio_buf_size = chunk_size * 3;
		if (ri->audio_buf_size <= 0)
			return -1;
		if((ri->audio = BZ_Malloc(ri->audio_buf_size)) == NULL) return -1;
	}
	if (ri->audio_buf_size < 0)
		return -1;

	if(chunk_id == RoQ_SOUND_MONO)
	{
		ri->audio_size = chunk_size;
		snd_left = chunk_arg;
		for(i = 0; i < chunk_size; i++)
		{
			snd_left += (int)ri->snd_sqr_arr[(unsigned)VFS_GETC(fp)];
			*(short *)&ri->audio[i * 2] = snd_left;
		}
		ri->aud_pos = fpos;
		return chunk_size;
	}

	if(chunk_id == RoQ_SOUND_STEREO)
	{
		ri->audio_size = chunk_size;
		snd_left = (chunk_arg & 0xFF00);
		snd_right = (chunk_arg & 0xFF) << 8;
		for(i = 0; i < chunk_size; i += 2)
		{
			snd_left += (int)ri->snd_sqr_arr[(unsigned)VFS_GETC(fp)];
			snd_right += (int)ri->snd_sqr_arr[(unsigned)VFS_GETC(fp)];
			*(short *)&ri->audio[i * 2] = snd_left & 0xffff;
			*(short *)&ri->audio[i * 2 + 2] = snd_right & 0xffff;
		}
		ri->aud_pos = fpos;
		return chunk_size;
	}

	ri->aud_pos = fpos;
	return 0;
}

#endif
