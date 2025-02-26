#ifndef _av_roq_h
#define _av_roq_h

#define RoQ_INFO					0x1001
#define RoQ_QUAD_CODEBOOK		0x1002
#define RoQ_QUAD_VQ				0x1011
#define RoQ_SOUND_MONO			0x1020
#define RoQ_SOUND_STEREO		0x1021

#define RoQ_ID_MOT		0x00
#define RoQ_ID_FCC		0x01
#define RoQ_ID_SLD		0x02
#define RoQ_ID_CCC		0x03

typedef struct {
	unsigned char y0, y1, y2, y3, u, v;
} roq_cell;

typedef struct {
	char p[16];
} roq_cell_rgba;

typedef struct {
	int idx[4];
} roq_qcell;

typedef struct roq_info_s {
	vfsfile_t *fp;
	qofs_t maxpos;	//addition for pack files. all seeks add this, all tells subtract this.
	int buf_size;
	unsigned char *buf;
	roq_cell cells[256];
	roq_cell_rgba cells_rgba[256];
	roq_qcell qcells[256];
	short snd_sqr_arr[256];
	qofs_t roq_start, aud_pos, vid_pos;
	long *frame_offset;
	unsigned long num_frames, num_audio_bytes;
	int width, height, frame_num, audio_channels;
	byte_vec4_t *rgba[2];
	long stream_length;
	int audio_buf_size, audio_size;
	unsigned char *audio;
} roq_info;

/* -------------------------------------------------------------------------- */
//void roq_init(void);
//void roq_cleanup(void);
roq_info *roq_open(char *fname);
void roq_rewind(roq_info *ri);
void roq_close(roq_info *ri);
int roq_read_frame(roq_info *ri);
int roq_read_audio(roq_info *ri);

#endif

