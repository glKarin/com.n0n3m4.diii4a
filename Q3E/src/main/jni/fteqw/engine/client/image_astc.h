//Note: this code does not claim to be bit-correct.
//It doesn't support volume textures.
//It doesn't validate block extents (and is generally unaware of more than one block anyway)
//It doesn't implement all validation checks, either.
//Do NOT use this code to validate any encoders...

//Based upon documentation here: https://www.khronos.org/registry/OpenGL/extensions/OES/OES_texture_compression_astc.txt

#ifndef ASTC_PUBLIC
#define ASTC_PUBLIC
#endif

#define ASTC_WITH_LDR			//comment out this line to disable pure-LDR decoding (the hdr code can still be used).
#define ASTC_WITH_HDR			//comment out this line to disable HDR decoding.
#define ASTC_WITH_HDRTEST		//comment out this line to disable checking for which profile is needed.
//#define ASTC_WITH_3D

#ifdef ASTC_WITH_LDR
	ASTC_PUBLIC void ASTC_Decode_LDR8(unsigned char *in, unsigned char *out, int pixstride/*outwidth*/, int layerstride/*outwidth*outheight*/, int bw,int bh,int bd);	//generates RGBA8 data (gives error colour for hdr blocks!)
#endif
#ifdef ASTC_WITH_HDR
	ASTC_PUBLIC void ASTC_Decode_HDR(unsigned char *in, unsigned short *out, int pixstride/*outwidth*/, int layerstride/*outwidth*outheight*/, int bw,int bh,int bd);	//generates RGBA16F data.
#endif
#ifdef ASTC_WITH_HDRTEST
	ASTC_PUBLIC int ASTC_BlocksAreHDR(unsigned char *in, size_t datasize, int bw, int bh, int bd);				//returns true if n consecutive blocks require the HDR profile (ie: detects when you need to soft-decode for drivers with partial support, as opposed to just always decompressing).
#endif



#include <math.h>
#include <stdio.h>
#include <string.h>
#ifndef Vector4Set
	#define Vector4Set(r,x,y,z,w) {(r)[0] = x; (r)[1] = y;(r)[2] = z;(r)[3]=w;}
#endif
#ifndef countof
	#define countof(array) (sizeof(array)/sizeof(array[0]))
#endif
#if defined(ASTC_WITH_LDR) || defined(ASTC_WITH_HDR)
	#define ASTC_WITH_DECODE
#endif
enum astc_status_e
{
	//valid blocks
	ASTC_OKAY,				//we can decode at least part of this normally (hdr endpoints may still result in per-endpoint errors).
	ASTC_VOID_LDR,			//not an error - the block is a single LDR colour, with an RGBA16 colour in the last 8 bytes.
	ASTC_VOID_HDR,			//not an error - the block is a single HDR colour, with an RGBA16F colour in the last 8 bytes.

	//invalid blocks
	ASTC_ERROR,				//validation errors
	ASTC_UNSUPPORTED,		//basically just volume textures
	ASTC_RESERVED,			//reserved bits. basically an error but might not be in the future.
};
struct astc_block_info
{
	unsigned char *in;			//the 16 bytes of the block
	unsigned char blocksize[3];	//block width, height, depth(1 for 2d).

	enum astc_status_e status;	//block status/type.
	unsigned char dualplane;	//two sets of weights instead of one.
	unsigned char ccs;			//second set applies to this component

	unsigned char precision;	//defines the precision of the weights

	int wcount[4];				//x,y,z,total weight counts
	int weight_bits;			//size of weights section.
	int config_bits;			//size of header before the endpoint bits
	int ep_bits;				//size available to endpoints
	unsigned char weights[64];	//official limit to the number of weights stored

	unsigned char partitions;	//number of active partitions to select from (and number of endpoints to read)
	unsigned short partindex; //used for deciding which partition each pixel belongs in
	struct astc_part
	{
		unsigned char mode;		//endpoint modes
#ifdef ASTC_WITH_HDR
		unsigned char hdr;		//endpoint colour mode - &1=rgb, &2=alpha
#endif
		int ep[2][4];
	} part[4];
};

static unsigned char ASTC_readbits(unsigned char *in, unsigned int offset, unsigned int count)
{	//only reads up to 9 bits, because offset 7 with 10 bits needs to read more than two bytes
	unsigned short s;
	in += offset>>3;
	offset &= 7;
	s = in[0];
	if (offset+count>8)
		s |= (in[1]<<8);
	s>>=offset;
	return s & ((1u<<count)-1);
}
static unsigned int ASTC_readmanybits(unsigned char *in, unsigned int offset, unsigned int count)
{
	unsigned int r = 0;
	while(count > 8)
	{
		count -= 8;
		r |= ASTC_readbits(in, offset+count, 8)<<count;
	}
	r |= ASTC_readbits(in, offset, count);
	return r;
}

//weights cover a range of 0-64 inclusive
//>32 is +1 (otherwise it would be 0-63)
//high bits are folded over
static unsigned char dequant_weight_1b[1<<1] = {0x00,0x40};
static unsigned char dequant_weight_2b[1<<2] = {0x00,0x15,0x2b,0x40};
static unsigned char dequant_weight_3b[1<<3] = {0x00,0x09,0x12,0x1b,0x25,0x2e,0x37,0x40};
static unsigned char dequant_weight_4b[1<<4] = {0x00,0x04,0x08,0x0c,0x11,0x15,0x19,0x1d,0x23,0x27,0x2b,0x2f,0x34,0x38,0x3c,0x40};
static unsigned char dequant_weight_5b[1<<5] = {0x00,0x02,0x04,0x06,0x08,0x0a,0x0c,0x0e,0x10,0x12,0x14,0x16,0x18,0x1a,0x1c,0x1e,0x22,0x24,0x26,0x28,0x2a,0x2c,0x2e,0x30,0x32,0x34,0x36,0x38,0x3a,0x3c,0x3e,0x40};
static unsigned char dequant_weight_0t[3] = {0,32,64};
static unsigned char dequant_weight_1t[6] = {0x00,0x40,0x0c,0x34,0x19,0x27};
static unsigned char dequant_weight_2t[12] = {0x00,0x40,0x11,0x2f,0x06,0x3a,0x17,0x29,0x0c,0x34,0x1d,0x23};
static unsigned char dequant_weight_3t[24] = {0x00,0x40,0x08,0x38,0x10,0x30,0x18,0x28,0x02,0x3e,0x0b,0x35,0x13,0x2d,0x1b,0x25,0x05,0x3b,0x0d,0x33,0x16,0x2a,0x1e,0x22};
static unsigned char dequant_weight_0q[5] = {0,16,32,48,64};
static unsigned char dequant_weight_1q[10] = {0x00,0x40,0x05,0x3b,0x0b,0x35,0x11,0x2f,0x17,0x29};
static unsigned char dequant_weight_2q[20] = {0x00,0x40,0x10,0x30,0x03,0x3d,0x13,0x2d,0x06,0x3a,0x17,0x29,0x09,0x37,0x1a,0x26,0x0d,0x33,0x1d,0x23};
static const struct
{
	unsigned char extra, bits, *dequant;
} astc_weightmode[] =
{
	{0,0, NULL}, //invalid
	{0,0, NULL}, //invalid
	{0,1, dequant_weight_1b}, //2
	{1,0, dequant_weight_0t}, //3
	{0,2, dequant_weight_2b}, //4
	{2,0, dequant_weight_0q}, //5
	{1,1, dequant_weight_1t}, //6
	{0,3, dequant_weight_3b}, //8
	{0,0, NULL}, //invalid
	{0,0, NULL}, //invalid
	{2,1, dequant_weight_1q}, //10
	{1,2, dequant_weight_2t}, //12
	{0,4, dequant_weight_4b}, //16
	{2,2, dequant_weight_2q}, //20
	{1,3, dequant_weight_3t}, //24
	{0,5, dequant_weight_5b}, //32
};
static unsigned int ASTC_DecodeSize(unsigned int count, unsigned int bits, unsigned char extra)
{
	return	((extra==1)?((count*8)+4)/5:0) +
			((extra==2)?((count*7)+2)/3:0) +
							 count*bits;
}


static void ASTC_ReadBlockMode(struct astc_block_info *b)
{
	unsigned char *in = b->in;
	unsigned short s = ASTC_readmanybits(in, 0, 13);//in[0] | (in[1]<<8);
	b->config_bits = 13;

	if ((s&0x1ff)==0x1fc)
	{	//void extent
		if (s&0x200)
			b->status = ASTC_VOID_HDR;
		else
			b->status = ASTC_VOID_LDR;
		b->dualplane = b->precision = b->wcount[0] = b->wcount[1] = b->wcount[2] = b->partitions = 0;
		return;
	}
	b->status = ASTC_OKAY;
	b->dualplane = (s>>10)&1;		 //Dp
	b->precision = (s>>(9-3))&(1<<3);//P
	b->precision |= (s>>4)&1;		 //p0
	if (b->blocksize[2] != 1)
	{	//3d blocks have a different header layout
#ifdef ASTC_WITH_3D
		if (s&3)
		{
			b->precision|=(s&3)<<1;	//p2, p1
			b->wcount[0] = ((s>>5)&3)+2, b->wcount[1] = ((s>>7)&3)+2, b->wcount[2] = ((s>>2)&3)+2;
		}
		else
		{
			b->precision|=(s&0xc)>>1;	//p2, p1
			if ((s&0x180)!=0x180)
			{
				b->dualplane = 0;	//always single plane.
				b->precision &= 7;	//clear the high precision bit (reused for 'b')
				if (!(s&0x180))
					b->wcount[0] = 6, b->wcount[1] = ((s>>9)&3)+2, b->wcount[2] = ((s>>5)&3)+2;
				else if (!(s&0x80))
					b->wcount[0] = ((s>>5)&3)+2, b->wcount[1] = 6, b->wcount[2] = ((s>>9)&3)+2;
				else
					b->wcount[0] = ((s>>5)&3)+2, b->wcount[1] = ((s>>9)&3)+2, b->wcount[2] = 6;
			}
			else if ((s&0x60)!=0x60)
			{
				if (!(s&0x60))
					b->wcount[0] = 6, b->wcount[1] = 2, b->wcount[2] = 2;
				else if (!(s&0x20))
					b->wcount[0] = 2, b->wcount[1] = 6, b->wcount[2] = 2;
				else	//40
					b->wcount[0] = 2, b->wcount[1] = 2, b->wcount[2] = 6;
			}
			else
				b->status = ASTC_RESERVED; //reserved (or void extent, but those were handled above)
		}
#else
		b->status = ASTC_UNSUPPORTED;
#endif
	}
	else
	{
		b->wcount[2] = 1;
		if (s&3)
		{	//one of the first 5 layouts...
			b->precision|=(s&3)<<1;	//p2, p1
			if (!(s&8))
			{	//first two layouts...
				if (!(s&4))
				{	//layout0
					b->wcount[0] = ((s>>7)&3)+4;
					b->wcount[1] = ((s>>5)&3)+2;
				}
				else
				{	//layout1
					b->wcount[0] = ((s>>7)&3)+8;
					b->wcount[1] = ((s>>5)&3)+2;
				}
			}
			else if (!(s&4))
			{	//layout2
				b->wcount[0] = ((s>>5)&3)+2;
				b->wcount[1] = ((s>>7)&3)+8;
			}
			else if (!(s&256))
			{	//layout3
				b->wcount[0] = ((s>>5)&3)+2;
				b->wcount[1] = ((s>>7)&1)+6;
			}
			else
			{	//layout4
				b->wcount[0] = ((s>>7)&1)+2;
				b->wcount[1] = ((s>>5)&3)+2;
			}
		}
		else
		{	//one of the later layouts
			b->precision|=(s&0xc)>>1;	//p2, p1
			if (!(s&384))
			{
				b->wcount[0] = 12;
				b->wcount[1] = ((s>>5)&3)+2;
			}
			else if ((s&384)==128)
			{
				b->wcount[0] = ((s>>5)&3)+2;
				b->wcount[1] = 12;
			}
			else if ((s&480)==384)
			{
				b->wcount[0] = 6;
				b->wcount[1] = 10;
			}
			else if ((s&480)==416)
			{
				b->wcount[0] = 10;
				b->wcount[1] = 6;
			}
			else if ((s&384)==256)
			{
				b->wcount[0] = ((s>>5)&3)+6;
				b->wcount[1] = ((s>>9)&3)+6;
				b->dualplane = 0;	//forget the Dp bit, its reused in this layout
				b->precision &= 7;	//forget the P bit, too
			}
			else
				b->status = ASTC_RESERVED; //reserved
		}
	}
	b->partitions = ((s>>11)&3)+1;

	if (b->partitions > 3 && b->dualplane)
		b->status = ASTC_ERROR;	//apparently.

	if (b->wcount[0] > b->blocksize[0] || b->wcount[1] > b->blocksize[1] || b->wcount[2] > b->blocksize[2])
		b->status = ASTC_ERROR; //invalid weight counts.

	b->wcount[3] = b->wcount[0] * b->wcount[1] * b->wcount[2];
	b->wcount[3]<<=b->dualplane;	//dual-plane has twice the weights - interleaved.
	if (b->wcount[3] > countof(b->weights))
		b->status = ASTC_ERROR;	//more than 64 weights are banned, for some reason
	b->weight_bits = ASTC_DecodeSize(b->wcount[3], astc_weightmode[b->precision].bits, astc_weightmode[b->precision].extra);
}

static void ASTC_ReadPartitions(struct astc_block_info *b)
{
	int sel;
	int i;
	unsigned char *in = b->in;
	int weight_bits = b->weight_bits;

	if (b->partitions == 1)
	{	//single-partition mode, simple CEM
		b->partindex = 0;
		b->part[0].mode = ASTC_readbits(in, b->config_bits, 4);
		b->config_bits += 4;
	}
	else
	{	//multi
		b->partindex = ASTC_readmanybits(in, b->config_bits, 10);
		b->config_bits += 10;
		sel = ASTC_readbits(in, b->config_bits, 6);
		b->config_bits += 6;
		if (!(sel&3))
		{
			sel = (sel>>2)&0xf;
			for (i = 0; i < b->partitions; i++)
				b->part[i].mode = sel;	//all the same
		}
		else
		{
			int shift = 2;
			int highbits = b->partitions*3 - 4;

			weight_bits += highbits;
			sel |= ASTC_readbits(in, 128-weight_bits, highbits)<<6;	//I don't know why this is separate. it seems like an unnecessary complication to me.

			for (i = 0; i < b->partitions; i++, shift++)
			{
				b->part[i].mode = ((sel&3)-1)<<2;		//class groups
				b->part[i].mode += ((sel>>shift)&1)<<2;//class
			}
			for (i = 0; i < b->partitions; i++, shift+=2)
				b->part[i].mode += (sel>>shift)&3;		//specific mode info
		}
	}
	if (b->dualplane)
	{
		weight_bits += 2;
		b->ccs = ASTC_readbits(in, 128-weight_bits, 2);
	}
	else
		b->ccs = 0;

	b->ep_bits = 128 - weight_bits - b->config_bits;
	//weights are at 128-weight_bits to 128
	//epdata is at config_bits to config_bits+ep_bits
}

#ifdef ASTC_WITH_HDRTEST
ASTC_PUBLIC int ASTC_BlocksAreHDR(unsigned char *in, size_t datasize, int bw, int bh, int bd)
{
	struct astc_block_info b;
	int i;
	size_t blocks = datasize/16;
	b.in = in;
	b.blocksize[0] = bw;
	b.blocksize[1] = bh;
	b.blocksize[2] = bd;
	while(blocks --> 0)
	{
		ASTC_ReadBlockMode(&b);
		if (b.status == ASTC_VOID_HDR)
			return 1;	//if we're getting hdr blocks then we can decode properly only with hdr
		if (b.status == ASTC_VOID_LDR)
			return 0;	//if we're getting ldr blocks, then its unlikely that there's any hdr blocks in there.
		if (b.status != ASTC_OKAY)
			continue;
		ASTC_ReadPartitions(&b);
		for (i = 0; i < b.partitions; i++)
		{
			switch(b.part[i].mode)
			{
			case 2:
			case 3:
			case 7:
			case 11:
			case 14:
			case 15:
				return 1;
			 }
		}
		b.in += 16;
	}
	return 0;
}
#endif

#ifdef ASTC_WITH_DECODE
static unsigned char ASTC_readbits2(unsigned char *in, unsigned int *offset, unsigned int count)
{	//only reads up to 9 bits, because offset 7 with 10 bits needs to read more than two bytes
	unsigned char r = ASTC_readbits(in, *offset, count);
	*offset += count;
	return r;
}
static void ASTC_Decode(unsigned char *in, unsigned char *out, int count, unsigned int offset, int bits, int extra, unsigned char *dequant)
{
	unsigned char block[5];
	int j;

	//unfortunately these trits depend upon the values of the later bits in each block.
	//if only it were a nice simple modulo...
	if (extra==1)
	{
		//read it 5 samples at a time
		while(count > 0)
		{
			unsigned int t, c;

			block[0] = ASTC_readbits2(in, &offset, bits);
			t = ASTC_readbits2(in, &offset, 2);
			if (count > 1)
			{
				block[1] = ASTC_readbits2(in, &offset, bits);
				t |= ASTC_readbits2(in, &offset, 2)<<2;
			}
			else
				block[1] = 0;
			if (count > 2)
			{
				block[2] = ASTC_readbits2(in, &offset, bits);
				t |= ASTC_readbits2(in, &offset, 1)<<4;
			}
			else
				block[2] = 0;
			if (count > 3)
			{
				block[3] = ASTC_readbits2(in, &offset, bits);
				t |= ASTC_readbits2(in, &offset, 2)<<5;
			}
			else
				block[3] = 0;
			if (count > 4)
			{
				block[4] = ASTC_readbits2(in, &offset, bits);
				t |= ASTC_readbits2(in, &offset, 1)<<7;
			}
			else
				block[4] = 0;

			//okay, we read the block, now figure out the trits and pack them into the high part of the result
			if ((t&0x1c) == 0x1c)
			{
				c = ((t>>3)&0x1c) | (t&3);
				block[4] |= 2<<bits;
				block[3] |= 2<<bits;
			}
			else
			{
				c = t&0x1f;
				if ((t&0x60) == 0x60)
				{
					block[4] |= 2<<bits;
					block[3] |= (t>>7)<<bits;
				}
				else
				{
					block[4] |= (t>>7)<<bits;
					block[3] |= ((t>>5)&3)<<bits;
                }
			}
			if ((c&3)==3)
			{
				block[2] |= 2<<bits;
				block[1] |= ((c>>4)&1)<<bits;
				block[0] |= (((c>>2)&2) | ((c>>2)&~(c>>3)&1))<<bits;
			}
			else if ((c&0xc)==0xc)
			{
				block[2] |= 2<<bits;
				block[1] |= 2<<bits;
				block[0] |= (c&3)<<bits;
			}
			else
			{
				block[2] |= ((c>>4)&1)<<bits;
				block[1] |= ((c>>2)&3)<<bits;
				block[0] |= ((c&2)|(c&1&~(c>>1)))<<bits;
			}

			//spit out the result
			for (j = 0; j < 5 && j < count; j++)
				*out++ = dequant[block[j]];
			count -= 5;
		}
	}
	else if (extra == 2)
	{
		//read it 3 samples at a time
		while(count > 0)
		{
			unsigned int t, c;

			block[0] = ASTC_readbits2(in, &offset, bits);
			t = ASTC_readbits2(in, &offset, 3);
			if (count > 1)
			{
				block[1] = ASTC_readbits2(in, &offset, bits);
				t |= ASTC_readbits2(in, &offset, 2)<<3;
			}
			else
				block[1] = 0;
			if (count > 2)
			{
				block[2] = ASTC_readbits2(in, &offset, bits);
				t |= ASTC_readbits2(in, &offset, 2)<<5;
			}
			else
				block[2] = 0;

			//okay, we read the block, now figure out the trits and pack them into the high part of the result
			if ((t&6)==6 && !(t&0x60))
			{
				block[2] |= (((t&1)<<2) | (((t>>4)&~t&1)<<1) | ((t>>3)&~t&1))<<bits;
				block[1] |= 4<<bits;
				block[0] |= 4<<bits;
			}
			else
			{
				if ((t&6) == 6)
				{
					block[2] |= 4<<bits;
					c = ((t>>3)&3)<<3;
					c |= (~(t>>5)&3)<<1;
					c |= t&1;
				}
				else
				{
					block[2] |= ((t>>5)&3)<<bits;
					c = t&0x1f;
				}

				if ((c&7) == 5)
				{
					block[1] |= 4<<bits;
					block[0] |= ((c>>3)&3)<<bits;
				}
				else
				{
					block[1] |= ((c>>3)&3)<<bits;
					block[0] |= (c&7)<<bits;
				}
			}

			//spit out the result
			for (j = 0; j < 3 && j < count; j++)
				*out++ = dequant[block[j]];
			count -= 3;
		}
	}
	else while(count --> 0)	//pure bits, nice and simple
	{
		unsigned char val = ASTC_readbits2(in, &offset, bits);

		*out++ = dequant[val];
	}
}

//endpoints have a logical value between 0 and 255.
//bit replication is used to fill in missing precision
static unsigned char dequant_ep_1b[1<<1] = {0,255};
static unsigned char dequant_ep_2b[1<<2] = {0x00,0x55,0xaa,0xff};
static unsigned char dequant_ep_3b[1<<3] = {0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff};
static unsigned char dequant_ep_4b[1<<4] = {
	0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
static unsigned char dequant_ep_5b[1<<5] = {
	0x00,0x08,0x10,0x18,0x21,0x29,0x31,0x39,0x42,0x4a,0x52,0x5a,0x63,0x6b,0x73,0x7b,
	0x84,0x8c,0x94,0x9c,0xa5,0xad,0xb5,0xbd,0xc6,0xce,0xd6,0xde,0xe7,0xef,0xf7,0xff};
static unsigned char dequant_ep_6b[1<<6] = {
	0x00,0x04,0x08,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,
	0x41,0x45,0x49,0x4d,0x51,0x55,0x59,0x5d,0x61,0x65,0x69,0x6d,0x71,0x75,0x79,0x7d,
	0x82,0x86,0x8a,0x8e,0x92,0x96,0x9a,0x9e,0xa2,0xa6,0xaa,0xae,0xb2,0xb6,0xba,0xbe,
	0xc3,0xc7,0xcb,0xcf,0xd3,0xd7,0xdb,0xdf,0xe3,0xe7,0xeb,0xef,0xf3,0xf7,0xfb,0xff};
static unsigned char dequant_ep_7b[1<<7] = {
	0x00,0x02,0x04,0x06,0x08,0x0a,0x0c,0x0e,0x10,0x12,0x14,0x16,0x18,0x1a,0x1c,0x1e,
	0x20,0x22,0x24,0x26,0x28,0x2a,0x2c,0x2e,0x30,0x32,0x34,0x36,0x38,0x3a,0x3c,0x3e,
	0x40,0x42,0x44,0x46,0x48,0x4a,0x4c,0x4e,0x50,0x52,0x54,0x56,0x58,0x5a,0x5c,0x5e,
	0x60,0x62,0x64,0x66,0x68,0x6a,0x6c,0x6e,0x70,0x72,0x74,0x76,0x78,0x7a,0x7c,0x7e,
	0x81,0x83,0x85,0x87,0x89,0x8b,0x8d,0x8f,0x91,0x93,0x95,0x97,0x99,0x9b,0x9d,0x9f,
	0xa1,0xa3,0xa5,0xa7,0xa9,0xab,0xad,0xaf,0xb1,0xb3,0xb5,0xb7,0xb9,0xbb,0xbd,0xbf,
	0xc1,0xc3,0xc5,0xc7,0xc9,0xcb,0xcd,0xcf,0xd1,0xd3,0xd5,0xd7,0xd9,0xdb,0xdd,0xdf,
	0xe1,0xe3,0xe5,0xe7,0xe9,0xeb,0xed,0xef,0xf1,0xf3,0xf5,0xf7,0xf9,0xfb,0xfd,0xff};
static unsigned char dequant_ep_8b[1<<8] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};
static unsigned char dequant_ep_0t[3]  = {0,128,255};
static unsigned char dequant_ep_1t[6]  = {0x00,0xff,0x33,0xcc,0x66,0x99};
static unsigned char dequant_ep_2t[12] = {0x00,0xff,0x45,0xba,0x17,0xe8,0x5c,0xa3,0x2e,0xd1,0x74,0x8b};
static unsigned char dequant_ep_3t[24] = {0x00,0xff,0x21,0xde,0x42,0xbd,0x63,0x9c,0x0b,0xf4,0x2c,0xd3,0x4d,0xb2,0x6e,0x91,0x16,0xe9,0x37,0xc8,0x58,0xa7,0x79,0x86};
static unsigned char dequant_ep_4t[48] = {0x00,0xff,0x10,0xef,0x20,0xdf,0x30,0xcf,0x41,0xbe,0x51,0xae,0x61,0x9e,0x71,0x8e,0x05,0xfa,0x15,0xea,0x26,0xd9,0x36,0xc9,0x46,0xb9,0x56,0xa9,0x67,0x98,0x77,0x88,0x0b,0xf4,0x1b,0xe4,0x2b,0xd4,0x3b,0xc4,0x4c,0xb3,0x5c,0xa3,0x6c,0x93,0x7c,0x83};
static unsigned char dequant_ep_5t[96] = {0x00,0xff,0x08,0xf7,0x10,0xef,0x18,0xe7,0x20,0xdf,0x28,0xd7,0x30,0xcf,0x38,0xc7,0x40,0xbf,0x48,0xb7,0x50,0xaf,0x58,0xa7,0x60,0x9f,0x68,0x97,0x70,0x8f,0x78,0x87,0x02,0xfd,0x0a,0xf5,0x12,0xed,0x1a,0xe5,0x23,0xdc,0x2b,0xd4,0x33,0xcc,0x3b,0xc4,0x43,0xbc,0x4b,0xb4,0x53,0xac,0x5b,0xa4,0x63,0x9c,0x6b,0x94,0x73,0x8c,0x7b,0x84,0x05,0xfa,0x0d,0xf2,0x15,0xea,0x1d,0xe2,0x25,0xda,0x2d,0xd2,0x35,0xca,0x3d,0xc2,0x46,0xb9,0x4e,0xb1,0x56,0xa9,0x5e,0xa1,0x66,0x99,0x6e,0x91,0x76,0x89,0x7e,0x81};
static unsigned char dequant_ep_6t[192]= {0x00,0xff,0x04,0xfb,0x08,0xf7,0x0c,0xf3,0x10,0xef,0x14,0xeb,0x18,0xe7,0x1c,0xe3,0x20,0xdf,0x24,0xdb,0x28,0xd7,0x2c,0xd3,0x30,0xcf,0x34,0xcb,0x38,0xc7,0x3c,0xc3,0x40,0xbf,0x44,0xbb,0x48,0xb7,0x4c,0xb3,0x50,0xaf,0x54,0xab,0x58,0xa7,0x5c,0xa3,0x60,0x9f,0x64,0x9b,0x68,0x97,0x6c,0x93,0x70,0x8f,0x74,0x8b,0x78,0x87,0x7c,0x83,0x01,0xfe,0x05,0xfa,0x09,0xf6,0x0d,0xf2,0x11,0xee,0x15,0xea,0x19,0xe6,0x1d,0xe2,0x21,0xde,0x25,0xda,0x29,0xd6,0x2d,0xd2,0x31,0xce,0x35,0xca,0x39,0xc6,0x3d,0xc2,0x41,0xbe,0x45,0xba,0x49,0xb6,0x4d,0xb2,0x51,0xae,0x55,0xaa,0x59,0xa6,0x5d,0xa2,0x61,0x9e,0x65,0x9a,0x69,0x96,0x6d,0x92,0x71,0x8e,0x75,0x8a,0x79,0x86,0x7d,0x82,0x02,0xfd,0x06,0xf9,0x0a,0xf5,0x0e,0xf1,0x12,0xed,0x16,0xe9,0x1a,0xe5,0x1e,0xe1,0x22,0xdd,0x26,0xd9,0x2a,0xd5,0x2e,0xd1,0x32,0xcd,0x36,0xc9,0x3a,0xc5,0x3e,0xc1,0x42,0xbd,0x46,0xb9,0x4a,0xb5,0x4e,0xb1,0x52,0xad,0x56,0xa9,0x5a,0xa5,0x5e,0xa1,0x62,0x9d,0x66,0x99,0x6a,0x95,0x6e,0x91,0x72,0x8d,0x76,0x89,0x7a,0x85,0x7e,0x81};
static unsigned char dequant_ep_0q[5]  = {0,64,128,192,255};
static unsigned char dequant_ep_1q[10] = {0x00,0xff,0x1c,0xe3,0x38,0xc7,0x54,0xab,0x71,0x8e};
static unsigned char dequant_ep_2q[20] = {0x00,0xff,0x43,0xbc,0x0d,0xf2,0x50,0xaf,0x1b,0xe4,0x5e,0xa1,0x28,0xd7,0x6b,0x94,0x36,0xc9,0x79,0x86};
static unsigned char dequant_ep_3q[40] = {0x00,0xff,0x20,0xdf,0x41,0xbe,0x61,0x9e,0x06,0xf9,0x27,0xd8,0x47,0xb8,0x68,0x97,0x0d,0xf2,0x2d,0xd2,0x4e,0xb1,0x6e,0x91,0x13,0xec,0x34,0xcb,0x54,0xab,0x75,0x8a,0x1a,0xe5,0x3a,0xc5,0x5b,0xa4,0x7b,0x84};
static unsigned char dequant_ep_4q[80] = {0x00,0xff,0x10,0xef,0x20,0xdf,0x30,0xcf,0x40,0xbf,0x50,0xaf,0x60,0x9f,0x70,0x8f,0x03,0xfc,0x13,0xec,0x23,0xdc,0x33,0xcc,0x43,0xbc,0x53,0xac,0x64,0x9b,0x74,0x8b,0x06,0xf9,0x16,0xe9,0x26,0xd9,0x36,0xc9,0x47,0xb8,0x57,0xa8,0x67,0x98,0x77,0x88,0x09,0xf6,0x19,0xe6,0x2a,0xd5,0x3a,0xc5,0x4a,0xb5,0x5a,0xa5,0x6a,0x95,0x7a,0x85,0x0d,0xf2,0x1d,0xe2,0x2d,0xd2,0x3d,0xc2,0x4d,0xb2,0x5d,0xa2,0x6d,0x92,0x7d,0x82};
static unsigned char dequant_ep_5q[160]= {0x00,0xff,0x08,0xf7,0x10,0xef,0x18,0xe7,0x20,0xdf,0x28,0xd7,0x30,0xcf,0x38,0xc7,0x40,0xbf,0x48,0xb7,0x50,0xaf,0x58,0xa7,0x60,0x9f,0x68,0x97,0x70,0x8f,0x78,0x87,0x01,0xfe,0x09,0xf6,0x11,0xee,0x19,0xe6,0x21,0xde,0x29,0xd6,0x31,0xce,0x39,0xc6,0x41,0xbe,0x49,0xb6,0x51,0xae,0x59,0xa6,0x61,0x9e,0x69,0x96,0x71,0x8e,0x79,0x86,0x03,0xfc,0x0b,0xf4,0x13,0xec,0x1b,0xe4,0x23,0xdc,0x2b,0xd4,0x33,0xcc,0x3b,0xc4,0x43,0xbc,0x4b,0xb4,0x53,0xac,0x5b,0xa4,0x63,0x9c,0x6b,0x94,0x73,0x8c,0x7b,0x84,0x04,0xfb,0x0c,0xf3,0x14,0xeb,0x1c,0xe3,0x24,0xdb,0x2c,0xd3,0x34,0xcb,0x3c,0xc3,0x44,0xbb,0x4c,0xb3,0x54,0xab,0x5c,0xa3,0x64,0x9b,0x6c,0x93,0x74,0x8b,0x7c,0x83,0x06,0xf9,0x0e,0xf1,0x16,0xe9,0x1e,0xe1,0x26,0xd9,0x2e,0xd1,0x36,0xc9,0x3e,0xc1,0x46,0xb9,0x4e,0xb1,0x56,0xa9,0x5e,0xa1,0x66,0x99,0x6e,0x91,0x76,0x89,0x7e,0x81};
static const struct
{
	unsigned char extra, bits, *dequant;
} astc_epvmode[] =
{
	{0,1, dequant_ep_1b}, //2
	{1,0, dequant_ep_0t}, //3
	{0,2, dequant_ep_2b}, //4
	{2,0, dequant_ep_0q}, //5
	{1,1, dequant_ep_1t}, //6
	{0,3, dequant_ep_3b}, //8
	{2,1, dequant_ep_1q}, //10
	{1,2, dequant_ep_2t}, //12
	{0,4, dequant_ep_4b}, //16
	{2,2, dequant_ep_2q}, //20
	{1,3, dequant_ep_3t}, //24
	{0,5, dequant_ep_5b}, //32
	{2,3, dequant_ep_3q}, //40
	{1,4, dequant_ep_4t}, //48
	{0,6, dequant_ep_6b}, //64
	{2,4, dequant_ep_4q}, //80
	{1,5, dequant_ep_5t}, //96
	{0,7, dequant_ep_7b}, //128
	{2,5, dequant_ep_5q}, //160
	{1,6, dequant_ep_6t}, //192
	{0,8, dequant_ep_8b}, //256
	//other modes don't make any sense
};
/*static void ASTC_CalcDequant(void)
{
	int i;

	int extra = 0;
	int bits = 1;
	int isweight = 1;
	int targbits = isweight?6:8;
	int v;

	static qboolean nospam;
	if (nospam)
		return;
	nospam = true;

	//binary:
	if (!extra)
	{
		for (bits = 1; bits <= (isweight?5:8); bits++)
		{
			Con_Printf("table: %s_%ib", isweight?"weight":"ep", bits);
			for (i = 0; i < (1<<bits); i++)
			{
				v = i;
				v<<=(targbits-bits);
				v|=v>>bits;
				v|=v>>bits;
				v|=v>>bits;
				v|=v>>bits;
				v|=v>>bits;
				v|=v>>bits;
				v|=v>>bits;
				v|=v>>bits;

				if (isweight && v > 32)
					v++; //0-64 instead of 0-63

				Con_Printf("0x%02x,", v);
			}
			Con_Printf("\n");
		}
	}
	else if (extra == 1)
	{
		int A,B,C,D;

		for (bits = 1; bits <= (isweight?3:6); bits++)
		{
			Con_Printf("table: %s_%it:\n", isweight?"weight":"ep", bits);
			for (i = 0; i < ((2<<bits)|(1<<bits)); i++)
			{
				switch(bits)
				{
				case 1:
					A = (i&1)*(isweight?0x7f:0x1ff);
					B = 0;
					C = isweight?50:204;
					D = i>>bits;
					break;
				case 2:
					A = (i&1)*(isweight?0x7f:0x1ff);
					B = ((i>>1)&1) * (isweight?0b1000101:0b100010110);
					C = isweight?25:93;
					D = i>>bits;
					break;
				case 3:
					A = (i&1)*(isweight?0x7f:0x1ff);
					B = ((i>>1)&1) * (isweight?0b0100001:0b010000101);	//b
					B|= ((i>>2)&1) * (isweight?0b1000010:0b100001010);	//c
					C = isweight?11:44;
					D = i>>bits;
					break;
				case 4:
					A = (i&1)*0x1ff;
					B = ((i>>1)&1) * 0b001000001;	//b
					B|= ((i>>2)&1) * 0b010000010;	//c
					B|= ((i>>3)&1) * 0b100000100;	//d
					C = 22;
					D = i>>bits;
					break;
				case 5:
					A = (i&1)*0x1ff;
					B = ((i>>1)&1) * 0b000100000;	//b
					B|= ((i>>2)&1) * 0b001000000;	//c
					B|= ((i>>3)&1) * 0b010000001;	//d
					B|= ((i>>4)&1) * 0b100000010;	//e
					C = 11;
					D = i>>bits;
					break;
				case 6:
					A = (i&1)*0x1ff;
					B = ((i>>1)&1) * 0b000010000;	//b
					B|= ((i>>2)&1) * 0b000100000;	//c
					B|= ((i>>3)&1) * 0b001000000;	//d
					B|= ((i>>4)&1) * 0b010000000;	//e
					B|= ((i>>5)&1) * 0b100000001;	//f
					C = 5;
					D = i>>bits;
					break;
				}
				v = D * C + B;
				v = v ^ A;
				v = (A & (isweight?0x20:0x80)) | (v >> 2);

				if (isweight && v > 32)
					v++; //0-64 instead of 0-63

				Con_Printf("0x%02x,", v);
			}
			Con_Printf("\n");
		}
	}
	else if (extra == 2)
	{
		int A,B,C,D;

		for (bits = 1; bits <= (isweight?2:5); bits++)
		{
			Con_Printf("table: %s_%iq:\n", isweight?"weight":"ep", bits);
			for (i = 0; i < ((4<<bits)|(1<<bits)); i++)
			{
				switch(bits)
				{
				case 1:
					A = (i&1)*(isweight?0x7f:0x1ff);
					B = 0;
					C = isweight?23:113;
					D = i>>bits;
					break;
				case 2:
					A = (i&1)*(isweight?0x7f:0x1ff);
					B = ((i>>1)&1) * (isweight?0b1000010:0b100001100);
					C = isweight?13:54;
					D = i>>bits;
					break;
				case 3:
					A = (i&1)*0x1ff;
					B = ((i>>1)&1) * 0b010000010;	//b
					B|= ((i>>2)&1) * 0b100000101;	//c
					C = 26;
					D = i>>bits;
					break;
				case 4:
					A = (i&1)*0x1ff;
					B = ((i>>1)&1) * 0b001000000;	//b
					B|= ((i>>2)&1) * 0b010000001;	//c
					B|= ((i>>3)&1) * 0b100000010;	//d
					C = 13;
					D = i>>bits;
					break;
				case 5:
					A = (i&1)*0x1ff;
					B = ((i>>1)&1) * 0b000100000;	//b
					B|= ((i>>2)&1) * 0b001000000;	//c
					B|= ((i>>3)&1) * 0b010000000;	//d
					B|= ((i>>4)&1) * 0b100000001;	//e
					C = 6;
					D = i>>bits;
					break;
				}
				v = D * C + B;
				v = v ^ A;
				v = (A & (isweight?0x20:0x80)) | (v >> 2);

				if (isweight && v > 32)
					v++; //0-64 instead of 0-63

				Con_Printf("0x%02x,", v);
			}
			Con_Printf("\n");
		}
	}
}*/

static void ASTC_blue_contract(int *out, int r, int g, int b, int a)
{
    out[0] = (r+b) >> 1;
    out[1] = (g+b) >> 1;
    out[2] = b;
    out[3] = a;
}
static int ASTC_bit_transfer_signed(int a, unsigned char *b)	//returns new value for a.
{
    *b >>= 1;
    *b |= a & 0x80;
    a >>= 1;
    a &= 0x3F;
    if((a&0x20)!=0)
		a=a-0x40;
	return a;
}
static void ASTC_clamp_unorm8(int *c)
{
	c[0] = bound(0, c[0], 255);
	c[1] = bound(0, c[1], 255);
	c[2] = bound(0, c[2], 255);
	c[3] = bound(0, c[3], 255);
}

#ifdef ASTC_WITH_HDR
static void ASTC_HDR_Mode_2(struct astc_part *p, unsigned char *v)
{
	int y0,y1;
	if(v[1] >= v[0])
	{
		y0 = (v[0] << 4);
		y1 = (v[1] << 4);
	}
	else
	{
		y0 = (v[1] << 4) + 8;
		y1 = (v[0] << 4) - 8;
	}
	Vector4Set(p->ep[0], y0, y0, y0, 0x780);
	Vector4Set(p->ep[1], y1, y1, y1, 0x780);
	p->hdr = 0xf;
}
static void ASTC_HDR_Mode_3(struct astc_part *p, unsigned char *v)
{
	int y0, y1, d;
	if((v[0]&0x80) != 0)
	{
		y0 = ((v[1] & 0xE0) << 4) | ((v[0] & 0x7F) << 2);
		d  =  (v[1] & 0x1F) << 2;
	}
	else
	{
		y0 = ((v[1] & 0xF0) << 4) | ((v[0] & 0x7F) << 1);
		d  =  (v[1] & 0x0F) << 1;
	}

	y1 = y0 + d;
	if(y1 > 0xFFF)
		y1 = 0xFFF;

	Vector4Set(p->ep[0], y0, y0, y0, 0x780);
	Vector4Set(p->ep[1], y1, y1, y1, 0x780);
	p->hdr = 0xf;
}
static void ASTC_HDR_Mode_7(struct astc_part *p, unsigned char *v)
{
	int modeval = ((v[0]&0xC0)>>6) | ((v[1]&0x80)>>5) | ((v[2]&0x80)>>4);
	int majcomp;
	int mode;
	static const int shamts[6] = { 1,1,2,3,4,5 };
	int shamt,t;

	int red, green, blue, scale;
	int x0,x1,x2,x3,x4,x5,x6,ohm;

	if( (modeval & 0xC ) != 0xC )
	{
		majcomp = modeval >> 2;
		mode = modeval & 3;
	}
	else if( modeval != 0xF )
	{
		majcomp = modeval & 3;
		mode = 4;
	}
	else
	{
		majcomp = 0; mode = 5;
	}

	red   = v[0] & 0x3f;
	green = v[1] & 0x1f;
	blue  = v[2] & 0x1f;
	scale = v[3] & 0x1f;

	x0 = (v[1] >> 6) & 1; x1 = (v[1] >> 5) & 1;
	x2 = (v[2] >> 6) & 1; x3 = (v[2] >> 5) & 1;
	x4 = (v[3] >> 7) & 1; x5 = (v[3] >> 6) & 1;
	x6 = (v[3] >> 5) & 1;

	ohm = 1 << mode;
	if( ohm & 0x30 ) green |= x0 << 6;
	if( ohm & 0x3A ) green |= x1 << 5;
	if( ohm & 0x30 ) blue |= x2 << 6;
	if( ohm & 0x3A ) blue |= x3 << 5;
	if( ohm & 0x3D ) scale |= x6 << 5;
	if( ohm & 0x2D ) scale |= x5 << 6;
	if( ohm & 0x04 ) scale |= x4 << 7;
	if( ohm & 0x3B ) red |= x4 << 6;
	if( ohm & 0x04 ) red |= x3 << 6;
	if( ohm & 0x10 ) red |= x5 << 7;
	if( ohm & 0x0F ) red |= x2 << 7;
	if( ohm & 0x05 ) red |= x1 << 8;
	if( ohm & 0x0A ) red |= x0 << 8;
	if( ohm & 0x05 ) red |= x0 << 9;
	if( ohm & 0x02 ) red |= x6 << 9;
	if( ohm & 0x01 ) red |= x3 << 10;
	if( ohm & 0x02 ) red |= x5 << 10;

	shamt = shamts[mode];
	red <<= shamt; green <<= shamt; blue <<= shamt; scale <<= shamt;

	if( mode != 5 ) { green = red - green; blue = red - blue; }

	if( majcomp == 1 )
	{
		t = red;
		red = green;
		green = t;
	}
	if( majcomp == 2 )
	{
		t = red;
		red = blue;
		blue = t;
	}

	p->ep[1][0] = bound( 0, red, 0xFFF );
	p->ep[1][1] = bound( 0, green, 0xFFF );
	p->ep[1][2] = bound( 0, blue, 0xFFF );

	p->ep[0][0] = bound( 0, red - scale, 0xFFF );
	p->ep[0][1] = bound( 0, green - scale, 0xFFF );
	p->ep[0][2] = bound( 0, blue - scale, 0xFFF );

	p->ep[1][3] = p->ep[0][3] = 0x780;

	p->hdr = 0xf;
}
static void ASTC_HDR_Mode_11(struct astc_part *p, unsigned char *v)
{
	static const int dbitstab[8] = {7,6,7,6,5,6,5,6};
	int shamt;
	int majcomp = ((v[4] & 0x80) >> 7) | ((v[5] & 0x80) >> 6);
	int mode,va,vb0,vb1,vc,vd0,vd1;
	int x0,x1,x2,x3,x4,x5,ohm;

	if( majcomp == 3 )
	{
		Vector4Set(p->ep[0], v[0] << 4, v[2] << 4, (v[4] & 0x7f) << 5, 0x780);
		Vector4Set(p->ep[1], v[1] << 4, v[3] << 4, (v[5] & 0x7f) << 5, 0x780);
		p->hdr = 0xf;
		return;
	}

	mode = ((v[1]&0x80)>>7) | ((v[2]&0x80)>>6) | ((v[3]&0x80)>>5);
	va  = v[0] | ((v[1] & 0x40) << 2);
	vb0 = v[2] & 0x3f;
	vb1 = v[3] & 0x3f;
	vc  = v[1] & 0x3f;
	vd0 = v[4] & 0x7f;
	vd1 = v[5] & 0x7f;

	if (vd0 & (1<<(dbitstab[mode]-1)))
		vd0 |= -1 & ~((1u<<dbitstab[mode])-1);
	if (vd1 & (1<<(dbitstab[mode]-1)))
		vd1 |= -1 & ~((1u<<dbitstab[mode])-1);

	x0 = (v[2] >> 6) & 1;
	x1 = (v[3] >> 6) & 1;
	x2 = (v[4] >> 6) & 1;
	x3 = (v[5] >> 6) & 1;
	x4 = (v[4] >> 5) & 1;
	x5 = (v[5] >> 5) & 1;

	ohm = 1 << mode;
	if( ohm & 0xA4 ) va |= x0 << 9;
	if( ohm & 0x08 ) va |= x2 << 9;
	if( ohm & 0x50 ) va |= x4 << 9;
	if( ohm & 0x50 ) va |= x5 << 10;
	if( ohm & 0xA0 ) va |= x1 << 10;
	if( ohm & 0xC0 ) va |= x2 << 11;
	if( ohm & 0x04 ) vc |= x1 << 6;
	if( ohm & 0xE8 ) vc |= x3 << 6;
	if( ohm & 0x20 ) vc |= x2 << 7;
	if( ohm & 0x5B ) vb0 |= x0 << 6;
	if( ohm & 0x5B ) vb1 |= x1 << 6;
	if( ohm & 0x12 ) vb0 |= x2 << 7;
	if( ohm & 0x12 ) vb1 |= x3 << 7;

	// Now shift up so that major component is at top of 12-bit value
	shamt = (mode >> 1) ^ 3;
	va <<= shamt; vb0 <<= shamt; vb1 <<= shamt;
	vc <<= shamt; vd0 <<= shamt; vd1 <<= shamt;

	p->ep[1][0] = bound( 0, va, 0xFFF );
	p->ep[1][1] = bound( 0, va - vb0, 0xFFF );
	p->ep[1][2] = bound( 0, va - vb1, 0xFFF );

	p->ep[0][0] = bound( 0, va - vc, 0xFFF );
	p->ep[0][1] = bound( 0, va - vb0 - vc - vd0, 0xFFF );
	p->ep[0][2] = bound( 0, va - vb1 - vc - vd1, 0xFFF );

	if( majcomp == 1 )
	{
		p->ep[0][3] = p->ep[0][0];
		p->ep[0][0] = p->ep[0][1];
		p->ep[0][1] = p->ep[0][3];
		p->ep[1][3] = p->ep[1][0];
		p->ep[1][0] = p->ep[1][1];
		p->ep[1][1] = p->ep[1][3];
	}
	else if( majcomp == 2 )
	{
		p->ep[0][3] = p->ep[0][0];
		p->ep[0][0] = p->ep[0][2];
		p->ep[0][2] = p->ep[0][3];
		p->ep[1][3] = p->ep[1][0];
		p->ep[1][0] = p->ep[1][2];
		p->ep[1][2] = p->ep[1][3];
	}

	p->ep[0][3] = p->ep[1][3] = 0x780;

	p->hdr = 0xf;
}
static void ASTC_HDR_Mode_14(struct astc_part *p, unsigned char *v)
{
	ASTC_HDR_Mode_11(p, v);

	p->ep[0][3] = v[6];
	p->ep[1][3] = v[7];
	p->hdr &= 0x7;
}
static void ASTC_HDR_Mode_15(struct astc_part *p, unsigned char *v)
{
	int v6=v[6], v7=v[7];
	int mode;
	ASTC_HDR_Mode_11(p,v);

	mode = ((v6 >> 7) & 1) | ((v7 >> 6) & 2);
	v6 &= 0x7F;
	v7 &= 0x7F;

	if(mode==3)
	{
		p->ep[0][3] = v6 << 5;
		p->ep[1][3] = v7 << 5;
	}
	else
	{
		v6 |= (v7 << (mode+1)) & 0x780;
		v7 &= (0x3F >> mode);
		v7 ^= 0x20 >> mode;
		v7 -= 0x20 >> mode;
		v6 <<= (4-mode);
		v7 <<= (4-mode);

		v7 += v6;
		v7 = bound(0, v7, 0xFFF);
		p->ep[0][3] = v6;
		p->ep[1][3] = v7;
	}
}
#endif

static void ASTC_DecodeEndpoints(struct astc_block_info *b, unsigned char *v)
{
	int i, t0, t1, t3, t5, t7;

	for (i = 0; i < b->partitions; i++)
	{
#ifdef ASTC_WITH_HDR
		b->part[i].hdr = 0;
#endif
		switch (b->part[i].mode & 15)
		{
#ifdef ASTC_WITH_HDR
		case 2:		//HDR Luminance, large range
			ASTC_HDR_Mode_2(&b->part[i], v);
			break;
		case 3:		//HDR Luminance, small range
			ASTC_HDR_Mode_3(&b->part[i], v);
			break;
		case 7:		//HDR RGB, base+scale
			ASTC_HDR_Mode_7(&b->part[i], v);
			break;
		case 11:	//HDR RGB
			ASTC_HDR_Mode_11(&b->part[i], v);
			break;
		case 14:	//HDR RGB + LDR Alpha
			ASTC_HDR_Mode_14(&b->part[i], v);
			break;
		case 15:	//HDR RGB + HDR Alpha
			ASTC_HDR_Mode_15(&b->part[i], v);
			break;
#endif
		default:	//the error colour - for unsupported hdr endpoints. unreachable when hdr is enabled. just fill it with the error colour.
			Vector4Set(b->part[i].ep[0], 0xff, 0, 0xff, 0xff);
			Vector4Set(b->part[i].ep[1], 0xff, 0, 0xff, 0xff);
			break;

		case 0:	//LDR Luminance, direct
			Vector4Set(b->part[i].ep[0], v[0], v[0], v[0], 0xff);
			Vector4Set(b->part[i].ep[1], v[1], v[1], v[1], 0xff);
			break;
		case 1:	//LDR Luminance, base+offset
			t0 = (v[0]>>2)|(v[1]&0xc0);
			t1 = t0+(v[1]&0x3f);
			if (t1>0xff)
				t1=0xff;
			Vector4Set(b->part[i].ep[0], t0, t0, t0, 0xff);
			Vector4Set(b->part[i].ep[1], t1, t1, t1, 0xff);
			break;
		case 4:	//LDR Luminance+Alpha,direct
			Vector4Set(b->part[i].ep[0], v[0], v[0], v[0], v[2]);
			Vector4Set(b->part[i].ep[1], v[1], v[1], v[1], v[3]);
			break;
		case 5:	//LDR Luminance+Alpha, base+offset
			t1 = ASTC_bit_transfer_signed(v[1],&v[0]);
			t3 = ASTC_bit_transfer_signed(v[3],&v[2]);
			Vector4Set(b->part[i].ep[0],v[0],v[0],v[0],v[2]);
			Vector4Set(b->part[i].ep[1],v[0]+t1,v[0]+t1,v[0]+t1,v[2]+t3);
			ASTC_clamp_unorm8(b->part[i].ep[0]);
			ASTC_clamp_unorm8(b->part[i].ep[1]);
			break;
		case 6:	//LDR RGB, base+scale
			Vector4Set(b->part[i].ep[0], ((int)v[0]*(int)v[3])>>8, ((int)v[1]*(int)v[3])>>8, ((int)v[2]*(int)v[3])>>8, 0xff);
			Vector4Set(b->part[i].ep[1], v[0], v[1], v[2], 0xff);
			break;
		case 8:	//LDR RGB, Direct
			t0 = (int)v[0]+(int)v[2]+(int)v[4];
			t1 = (int)v[1]+(int)v[3]+(int)v[5];
			if (t1>=t0)
			{
				Vector4Set(b->part[i].ep[0], v[0],v[2],v[4],0xff);
				Vector4Set(b->part[i].ep[1], v[1],v[3],v[5],0xff);
			}
			else
			{
				ASTC_blue_contract(b->part[i].ep[0], v[1],v[3],v[5], 0xff);
				ASTC_blue_contract(b->part[i].ep[1], v[0],v[2],v[4], 0xff);
			}
			break;
		case 9:	//LDR RGB, base+offset
			t1 = ASTC_bit_transfer_signed(v[1],&v[0]);
			t3 = ASTC_bit_transfer_signed(v[3],&v[2]);
			t5 = ASTC_bit_transfer_signed(v[5],&v[4]);
			if(t1+t3+t5 >= 0)
			{
				Vector4Set(b->part[i].ep[0],v[0],v[2],v[4],0xff);
				Vector4Set(b->part[i].ep[1],v[0]+t1,v[2]+t3,v[4]+t5,0xff);
			}
			else
			{
				ASTC_blue_contract(b->part[i].ep[0], v[0]+t1,v[2]+t3,v[4]+t5, 0xff);
				ASTC_blue_contract(b->part[i].ep[1], v[0],v[2],v[4], 0xff);
			}
			ASTC_clamp_unorm8(b->part[i].ep[0]);
			ASTC_clamp_unorm8(b->part[i].ep[1]);
			break;
		case 10:	//LDR RGB, base+scale plus two A
			Vector4Set(b->part[i].ep[0], ((int)v[0]*v[3])>>8, ((int)v[1]*v[3])>>8, ((int)v[2]*v[3])>>8, v[4]);
			Vector4Set(b->part[i].ep[1], v[0], v[1], v[2], v[5]);
			break;
		case 12:	//LDR RGBA, direct
			if (v[1]+(int)v[3]+v[5]>=v[0]+(int)v[2]+v[4])
			{
				Vector4Set(b->part[i].ep[0], v[0],v[2],v[4],v[6]);
				Vector4Set(b->part[i].ep[1], v[1],v[3],v[5],v[7]);
			}
			else
			{
				ASTC_blue_contract(b->part[i].ep[0], v[1],v[3],v[5],v[7]);
				ASTC_blue_contract(b->part[i].ep[1], v[0],v[2],v[4],v[6]);
			}
			break;
		case 13:	//LDR RGBA, base+offset
			t1 = ASTC_bit_transfer_signed(v[1],&v[0]);
			t3 = ASTC_bit_transfer_signed(v[3],&v[2]);
			t5 = ASTC_bit_transfer_signed(v[5],&v[4]);
			t7 = ASTC_bit_transfer_signed(v[7],&v[6]);
			if(t1+t3+t5>=0)
			{
				Vector4Set(b->part[i].ep[0], v[0],v[2],v[4],v[6]);
				Vector4Set(b->part[i].ep[1], v[0]+t1,v[2]+t3,v[4]+t5,v[6]+t7);
			}
			else
			{
				ASTC_blue_contract(b->part[i].ep[0], v[0]+t1,v[2]+t3,v[4]+t5,v[6]+t7);
				ASTC_blue_contract(b->part[i].ep[1], v[0],v[2],v[4],v[6]);
			}
			ASTC_clamp_unorm8(b->part[i].ep[0]);
			ASTC_clamp_unorm8(b->part[i].ep[1]);
			break;
		}
		v += ((b->part[i].mode>>2)+1)<<1;
	}
}
static void ASTC_ReadEndpoints(struct astc_block_info *b)
{
	int i;
	int cembits;

	unsigned char epv[18];		//maximum raw endpoint values,
	char epvalues;
	unsigned char gahffs[16], t;

	//figure out how many raw values we need
	epvalues = 0;
	for (i = 0; i < b->partitions; i++)
		epvalues += ((b->part[i].mode>>2)+1)<<1;
	if (epvalues > countof(epv))
	{
		b->status = ASTC_ERROR;
		return;
	}

	//the endpoint bits are encoded using the largest size available that'll still fit, yielding raw values between 0-255.
	for(i = countof(astc_epvmode)-1; i >= 0; i--)
	{
		cembits = ASTC_DecodeSize(epvalues, astc_epvmode[i].bits, astc_epvmode[i].extra);
		if(cembits <= b->ep_bits)
		{
			//read the values.
			ASTC_Decode(b->in, epv, epvalues, b->config_bits, astc_epvmode[i].bits, astc_epvmode[i].extra, astc_epvmode[i].dequant);
			//and decode them.
			ASTC_DecodeEndpoints(b, epv);

			//weight bits are backwards (gah! ffs!)
			//so swap them around so our decode function doesn't need to care
			for (i = 0; i < countof(gahffs); i++)
			{
				t = b->in[i];
				t = (t>>4)|(t<<4);
				t = ((t&0xcc)>>2)|((t&0x33)<<2);
				t = ((t&0xaa)>>1)|((t&0x55)<<1);
				gahffs[15-i] = t;
			}
			//weights are aligned at the end... now the start. gah! ffs!
			ASTC_Decode(gahffs, b->weights, b->wcount[3], 0, astc_weightmode[b->precision].bits, astc_weightmode[b->precision].extra, astc_weightmode[b->precision].dequant);
			return;
		}
	}
	b->status = ASTC_ERROR;
}

static unsigned int hash52(unsigned int p)
{
    p ^= p >> 15;  p -= p << 17;  p += p << 7; p += p <<  4;
    p ^= p >>  5;  p += p << 16;  p ^= p >> 7; p ^= p >> 3;
    p ^= p <<  6;  p ^= p >> 17;
    return p;
}
static int ASTC_ChoosePartition(int seed, int x, int y, int z, int partitions, int smallblock)
{
	int sh1, sh2, sh3, a,b,c,d;
	unsigned int rnum;
	unsigned char seed1,seed2,seed3,seed4,seed5,seed6,seed7,seed8,seed9,seed10,seed11,seed12;
	if (partitions==1)
		return 0;
	if (smallblock)
	{
		x <<= 1;
		y <<= 1;
		z <<= 1;
	}
	seed += (partitions-1) * 1024;
	rnum = hash52(seed);
	seed1  =  rnum        & 0xF;
	seed2  = (rnum >>  4) & 0xF;
	seed3  = (rnum >>  8) & 0xF;
	seed4  = (rnum >> 12) & 0xF;
	seed5  = (rnum >> 16) & 0xF;
	seed6  = (rnum >> 20) & 0xF;
	seed7  = (rnum >> 24) & 0xF;
	seed8  = (rnum >> 28) & 0xF;
	seed9  = (rnum >> 18) & 0xF;
	seed10 = (rnum >> 22) & 0xF;
	seed11 = (rnum >> 26) & 0xF;
	seed12 = ((rnum >> 30) | (rnum << 2)) & 0xF;

	seed1  *= seed1;    seed2  *= seed2;
	seed3  *= seed3;    seed4  *= seed4;
	seed5  *= seed5;    seed6  *= seed6;
	seed7  *= seed7;    seed8  *= seed8;
	seed9  *= seed9;    seed10 *= seed10;
	seed11 *= seed11;   seed12 *= seed12;


	if (seed & 1)
	{
		sh1 = ((seed&2) ? 4:5);
		sh2 = ((partitions==3) ? 6:5);
	}
	else
	{
		sh1 = ((partitions==3) ? 6:5);
		sh2 = ((seed&2) ? 4:5);
	}
	sh3 = (seed & 0x10) ? sh1 : sh2;

	seed1 >>= sh1; seed2  >>= sh2; seed3  >>= sh1; seed4  >>= sh2;
	seed5 >>= sh1; seed6  >>= sh2; seed7  >>= sh1; seed8  >>= sh2;
	seed9 >>= sh3; seed10 >>= sh3; seed11 >>= sh3; seed12 >>= sh3;

	a = seed1*x + seed2*y + seed11*z + (rnum >> 14);
	b = seed3*x + seed4*y + seed12*z + (rnum >> 10);
	c = seed5*x + seed6*y + seed9 *z + (rnum >>  6);
	d = seed7*x + seed8*y + seed10*z + (rnum >>  2);

	a &= 0x3F; b &= 0x3F; c &= 0x3F; d &= 0x3F;

	if (partitions < 4)
		d = 0;
	if (partitions < 3)
		c = 0;

	if (a >= b && a >= c && a >= d)
		return 0;
	else if (b >= c && b >= d)
		return 1;
	else if (c >= d)
		return 2;
	else
		return 3;
}
#endif

#ifdef ASTC_WITH_LDR
//Spits out 8-bit RGBA data for a single block. Any HDR blocks will result in the error colour.
//sRGB can be applied by the caller, if needed.
ASTC_PUBLIC void ASTC_Decode_LDR8(unsigned char *in, unsigned char *out, int pixstride, int layerstride, int bw, int bh, int bd)
{
	struct astc_block_info b;
	int x, y;
	int stride = pixstride*4;
#ifdef ASTC_WITH_3D
	int z;
	layerstride = layerstride*4-(stride*bh);
#else
	if (bd != 1)
		return;	//error!
#endif
	b.in = in;
	b.blocksize[0] = bw;
	b.blocksize[1] = bh;
	b.blocksize[2] = bd;
	ASTC_ReadBlockMode(&b);

	if (b.status == ASTC_VOID_LDR)
	{	//void extent
		//Note: we don't validate the extents.
		for (y = 0; y < bh; y++, out += stride)
			for (x = 0; x < bw; x++)
			{
				out[(x<<2)+0] = in[9];
				out[(x<<2)+1] = in[11];
				out[(x<<2)+2] = in[13];
				out[(x<<2)+3] = in[15];
			}
		return;
	}

	if (b.status == ASTC_OKAY)
		ASTC_ReadPartitions(&b);
	if (b.status == ASTC_OKAY)
		ASTC_ReadEndpoints(&b);

	if (b.status == ASTC_OKAY)
	{
		#define N b.wcount[0]
		#define M b.wcount[1]
		int s1=1<<b.dualplane,s2=N<<b.dualplane;	//values for 2d blocks (3d blocks will override)
		int s3=((bd!=1?N*M:0)+N+1)<<b.dualplane;	//small variation for 3d blocks.

		int smallblock = (b.blocksize[0]*b.blocksize[1]*b.blocksize[2])<31;
		int fs, s, ds = (1024+b.blocksize[0]/2)/(b.blocksize[0]-1);
		int ft, t, dt = (1024+b.blocksize[1]/2)/(b.blocksize[1]-1);
#ifdef ASTC_WITH_3D
		int fr, r, dr = (1024+b.blocksize[2]/2)/(b.blocksize[2]-1);
#endif
		int v0, w, w00,w01,w10,w11;
		struct astc_part *p;

#ifdef ASTC_WITH_HDR
		for (x = 0; x < b.partitions; x++)
		{	//the LDR profile treats HDR endpoints as the error colour. this is per-partition rather than per-block.
			if (b.part[x].hdr)
			{
				Vector4Set(b.part[x].ep[0], 0xff, 0, 0xff, 0xff);
				Vector4Set(b.part[x].ep[1], 0xff, 0, 0xff, 0xff);
			}
			//else FIXME: when spitting out 8bit, we're meant to have an extra 9th bit which is always set, in order to avoid round-to-zero biasing the result of the final 8 bits.
		}
#endif

#ifdef ASTC_WITH_3D
		for (z = 0; z < bd; z++, out += layerstride-stride*bh)
#endif
		{
#ifdef ASTC_WITH_3D
			r = ((dr*z)*(b.wcount[2]-1)+32)>>6;
			fr=r&0xf;
#endif
			for (y = 0; y < bh; y++, out += stride)
			{
				t = ((dt*y)*(b.wcount[1]-1)+32)>>6;
				ft=t&0xf;
				for (x = 0; x < bw; x++)
				{
					p = &b.part[ASTC_ChoosePartition(b.partindex, x,y,0, b.partitions, smallblock)];
					s = ((ds*x)*(b.wcount[0]-1)+32)>>6;
					fs=s&0xf;
#ifdef ASTC_WITH_3D
					if (bd != 1)
					{	//3d blocks use simplex interpolation instead of 8-way interpolation. its easier for hardware but more cycles for us.
						if (fs>fr)
						{	//figure out which weights/factors to use.
							if (ft>fr)
							{
								if (fs>ft)
									s1=1, s2=N, w00=16-fs, w01=fs-ft, w10=ft-fr, w11=fr;
								else
									s1=N, s2=1, w00=16-ft, w01=ft-fs, w10=fs-fr, w11=fr;
							}
							else
								s1=1, s2=N*M, w00=16-fs, w01=fs-fr, w10=fr-ft, w11=ft;
						}
						else
						{
							if (fs>ft)
								s1=N*M, s2=1, w00=16-fr, w01=fr-fs, w10=fs-ft, w11=ft;
							else
							{
								if (ft>fr)
									s1=N, s2=N*M, w00=16-ft, w01=ft-fr, w10=fr-fs, w11=fs;
								else
									s1=N*M, s2=N, w00=16-fr, w01=fr-ft, w10=ft-fs, w11=fs;
							}
						}

						s1 <<= b.dualplane;
						s2 <<= b.dualplane;
						s2+=s1;
						//s3 = (N*M+N+1)<<b.dualplane;
						v0 = ((s>>4)+(t>>4)*N+(r>>4)*N*M) << b.dualplane;
					}
					else
#endif
					{
						//s1 = 1<<b.dualplane;
						//s2 = (N)<<b.dualplane;
						//s3 = (N+1)<<b.dualplane;
						w11 = (fs*ft+8) >> 4;
						w10 = ft - w11;
						w01 = fs - w11;
						w00 = 16 - fs - ft + w11;
						v0 = ((s>>4)+(t>>4)*N) << b.dualplane;
					}
					w =	(	w00*b.weights[v0] +
							w01*b.weights[v0+s1] +
							w10*b.weights[v0+s2] +
							w11*b.weights[v0+s3] + 8) >> 4;
					out[(x<<2)+0] = ((64-w)*p->ep[0][0] + w*p->ep[1][0])>>6;
					out[(x<<2)+1] = ((64-w)*p->ep[0][1] + w*p->ep[1][1])>>6;
					out[(x<<2)+2] = ((64-w)*p->ep[0][2] + w*p->ep[1][2])>>6;
					out[(x<<2)+3] = ((64-w)*p->ep[0][3] + w*p->ep[1][3])>>6;

					if (b.dualplane)
					{	//dual planes has a second set of weights that override a single channel
						v0++;
						w =	(	w00*b.weights[v0] +
								w01*b.weights[v0+s1] +
								w10*b.weights[v0+s2] +
								w11*b.weights[v0+s3] + 8) >> 4;
						out[(x<<2)+b.ccs] = ((64-w)*p->ep[0][b.ccs] + w*p->ep[1][b.ccs])>>6;
					}
				}
			}
		}
		#undef N
		#undef M
	}
	else
	{	//error colour == magenta
#ifdef ASTC_WITH_3D
		for (z = 0; z < bd; z++, out += layerstride)
#endif
			for (y = 0; y < bh; y++, out += stride)
				for (x = 0; x < bw; x++)
				{
					out[(x<<2)+0] = 0xff;
					out[(x<<2)+1] = 0;
					out[(x<<2)+2] = 0xff;
					out[(x<<2)+3] = 0xff;
				}
	}
}
#endif

#ifdef ASTC_WITH_HDR
static unsigned short ASTC_GenHalffloat(int hdr, int rawval)
{
	if (hdr)
	{
		int fp16, m;
		fp16 = (rawval&0xF800) >> 1;
		m = rawval&0x7FF;
		if (m < 512)
			fp16 |= (3*m)>>3;
		else if (m >= 1536)
			fp16 |= (5*m - 2048)>>3;
		else
			fp16 |= (4*m - 512)>>3;
		return fp16;
	}
	else
	{
		union
		{
			float f;
			unsigned int u;
		} u = {rawval/65535.0};
		int e = 0;
		int m;

		e = ((u.u>>23)&0xff) - 127;
		if (e < -15)
			return 0; //too small exponent, treat it as a 0 denormal
		if (e > 15)
			m = 0; //infinity instead of a nan
		else
			m = (u.u&((1<<23)-1))>>13;
		return ((e+15)<<10) | m;
	}
}

//Spits out half-float RGBA data for a single block.
ASTC_PUBLIC void ASTC_Decode_HDR(unsigned char *in, unsigned short *out, int pixstride, int layerstride, int bw, int bh, int bd)
{
	int x, y;
	int stride = pixstride*4;
	struct astc_block_info b;
#ifdef ASTC_WITH_3D
	int z;
	layerstride = layerstride*4-(stride*bh);
#else
	if (bd != 1)
		return;	//error!
#endif
   	b.in = in;
	b.blocksize[0] = bw;
	b.blocksize[1] = bh;
	b.blocksize[2] = bd;

	ASTC_ReadBlockMode(&b);

	if (b.status == ASTC_VOID_HDR)
	{	//void extent
		//Note: we don't validate the extents.
		for (y = 0; y < bh; y++, out += stride)
			for (x = 0; x < bw; x++)
			{	//hdr void extents already use fp16
				out[(x<<2)+0] = in[8] | (in[9]<<8);
				out[(x<<2)+1] = in[10] | (in[11]<<8);
				out[(x<<2)+2] = in[12] | (in[13]<<8);
				out[(x<<2)+3] = in[14] | (in[15]<<8);
			}
		return;
	}
	if (b.status == ASTC_VOID_LDR)
	{	//void extent
		//Note: we don't validate the extents.
		for (y = 0; y < bh; y++, out += stride)
			for (x = 0; x < bw; x++)
			{
				out[(x<<2)+0] = ASTC_GenHalffloat(0, in[8] | (in[9]<<8));
				out[(x<<2)+1] = ASTC_GenHalffloat(0, in[10] | (in[11]<<8));
				out[(x<<2)+2] = ASTC_GenHalffloat(0, in[12] | (in[13]<<8));
				out[(x<<2)+3] = ASTC_GenHalffloat(0, in[14] | (in[15]<<8));
			}
		return;
	}

	if (b.status == ASTC_OKAY)
		ASTC_ReadPartitions(&b);
	if (b.status == ASTC_OKAY)
		ASTC_ReadEndpoints(&b);

	if (b.status == ASTC_OKAY)
	{
		#define N b.wcount[0]
		#define M b.wcount[1]
		int s1=1<<b.dualplane,s2=N<<b.dualplane;	//values for 2d blocks (3d blocks will override)
		int s3=((bd!=1?N*M:0)+N+1)<<b.dualplane;	//small variation for 3d blocks.

		int smallblock = (b.blocksize[0]*b.blocksize[1]*b.blocksize[2])<31;
		int fs, s, ds = (1024+b.blocksize[0]/2)/(b.blocksize[0]-1);
		int ft, t, dt = (1024+b.blocksize[1]/2)/(b.blocksize[1]-1);
#ifdef ASTC_WITH_3D
		int fr, r, dr = (1024+b.blocksize[2]/2)/(b.blocksize[2]-1);
#endif
		int v0, w, w00,w01,w10,w11;
		struct astc_part *p;

		for (x = 0; x < b.partitions; x++)
		{	//we need to do a little extra processing here
			for (y = 0; y < 4; y++)
			{
				if (b.part[x].hdr&(1<<y))
				{	//the 12bit endpoint values are shifted up to 16bit...
					b.part[x].ep[0][y] <<= 4;
					b.part[x].ep[1][y] <<= 4;
				}
				else
				{	//convert to unorm16.
					b.part[x].ep[0][y] |= b.part[x].ep[0][y] << 8;
					b.part[x].ep[1][y] |= b.part[x].ep[1][y] << 8;
				}
			}
		}

#ifdef ASTC_WITH_3D
		for (z = 0; z < bd; z++, out += layerstride)
#endif
		{
#ifdef ASTC_WITH_3D
			r = ((dr*z)*(b.wcount[2]-1)+32)>>6;
			fr=r&0xf;
#endif
			for (y = 0; y < bh; y++, out += stride)
			{
				t = ((dt*y)*(b.wcount[1]-1)+32)>>6;
				ft=t&0xf;
				for (x = 0; x < bw; x++)
				{
					p = &b.part[ASTC_ChoosePartition(b.partindex, x,y,0, b.partitions, smallblock)];
					s = ((ds*x)*(b.wcount[0]-1)+32)>>6;
					fs=s&0xf;
#ifdef ASTC_WITH_3D
					if (bd != 1)
					{	//3d blocks use simplex interpolation instead of 8-way interpolation. its easier for hardware but more cycles for us.
						if (fs>fr)
						{	//figure out which weights/factors to use.
							if (ft>fr)
							{
								if (fs>ft)
									s1=1, s2=N, w00=16-fs, w01=fs-ft, w10=ft-fr, w11=fr;
								else
									s1=N, s2=1, w00=16-ft, w01=ft-fs, w10=fs-fr, w11=fr;
							}
							else
								s1=1, s2=N*M, w00=16-fs, w01=fs-fr, w10=fr-ft, w11=ft;
						}
						else
						{
							if (fs>ft)
								s1=N*M, s2=1, w00=16-fr, w01=fr-fs, w10=fs-ft, w11=ft;
							else
							{
								if (ft>fr)
									s1=N, s2=N*M, w00=16-ft, w01=ft-fr, w10=fr-fs, w11=fs;
								else
									s1=N*M, s2=N, w00=16-fr, w01=fr-ft, w10=ft-fs, w11=fs;
							}
						}

						s1 <<= b.dualplane;
						s2 <<= b.dualplane;
						s2+=s1;
						//s3 = (N*M+N+1)<<b.dualplane;
						v0 = (((s>>4))+((t>>4)*N)+(r>>4)*N*M) << b.dualplane;
					}
					else
#endif
					{
						//s1 = 1<<b.dualplane;
						//s2 = (N)<<b.dualplane;
						//s3 = (N+1)<<b.dualplane;
						w11 = (fs*ft+8) >> 4;
						w10 = ft - w11;
						w01 = fs - w11;
						w00 = 16 - fs - ft + w11;

						v0 = (((s>>4))+(t>>4)*N) << b.dualplane;
					}
					w =	(	w00*b.weights[v0] +
							w01*b.weights[v0+s1] +
							w10*b.weights[v0+s2] +
							w11*b.weights[v0+s3] + 8) >> 4;
					out[(x<<2)+0] = ASTC_GenHalffloat(p->hdr&1, ((64-w)*p->ep[0][0] + w*p->ep[1][0])>>6);
					out[(x<<2)+1] = ASTC_GenHalffloat(p->hdr&1, ((64-w)*p->ep[0][1] + w*p->ep[1][1])>>6);
					out[(x<<2)+2] = ASTC_GenHalffloat(p->hdr&1, ((64-w)*p->ep[0][2] + w*p->ep[1][2])>>6);
					out[(x<<2)+3] = ASTC_GenHalffloat(p->hdr&8, ((64-w)*p->ep[0][3] + w*p->ep[1][3])>>6);

					if (b.dualplane)
					{	//dual planes has a second set of weights that override a single channel
						v0++;
						w =	(	w00*b.weights[v0] +
								w01*b.weights[v0+s1] +
								w10*b.weights[v0+s2] +
								w11*b.weights[v0+s3] + 8) >> 4;
						out[(x<<2)+b.ccs] = ASTC_GenHalffloat(p->hdr&(1<<b.ccs), ((64-w)*p->ep[0][b.ccs] + w*p->ep[1][b.ccs])>>6);
					}
				}
			}
		}
		#undef N
		#undef M
	}
	else
	{	//error colour == magenta
#ifdef ASTC_WITH_3D
		for (z = 0; z < bd; z++, out += layerstride)
#endif
			for (y = 0; y < bh; y++, out += stride)
				for (x = 0; x < bw; x++)
				{
					out[(x<<2)+0] = 0xf<<10;
					out[(x<<2)+1] = 0;
					out[(x<<2)+2] = 0xf<<10;
					out[(x<<2)+3] = 0xf<<10;
				}
	}
}
#endif
