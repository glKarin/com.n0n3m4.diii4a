/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/* This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book.  The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list */
#include "quakedef.h"

#ifdef HUFFNETWORK
#define NYT HMAX					/* NYT = Not Yet Transmitted */
#define INTERNAL_NODE (HMAX+1)

typedef struct nodetype {
	struct	nodetype *left, *right, *parent; /* tree structure */ 
	struct	nodetype *next, *prev; /* doubly-linked list */
	struct	nodetype **head; /* highest ranked node in block */
	int		weight;
	int		symbol;
} node_t;

#define HMAX 256 /* Maximum symbol */

typedef struct {
	int			blocNode;
	int			blocPtrs;

	node_t*		tree;
	node_t*		lhead;
	node_t*		ltail;
	node_t*		loc[HMAX+1];
	node_t**	freelist;

	node_t		nodeList[768];
	node_t*		nodePtrs[768];
} huff_t;

struct huffman_s{
	int counts[256];
	unsigned int crc;
	qboolean built;

	huff_t		compressor;
	huff_t		decompressor;
};
extern cvar_t net_compress;

static int			bloc = 0;
/*
static void	Huff_putBit( int bit, qbyte *fout, int *offset) {
	bloc = *offset;
	if ((bloc&7) == 0) {
		fout[(bloc>>3)] = 0;
	}
	fout[(bloc>>3)] |= bit << (bloc&7);
	bloc++;
	*offset = bloc;
}

static int		Huff_getBloc(void)
{
	return bloc;
}

static void	Huff_setBloc(int _bloc)
{
	bloc = _bloc;
}

static int		Huff_getBit( qbyte *fin, int *offset) {
	int t;
	bloc = *offset;
	t = (fin[(bloc>>3)] >> (bloc&7)) & 0x1;
	bloc++;
	*offset = bloc;
	return t;
}*/

/* Add a bit to the output file (buffered) */
static void huff_add_bit (char bit, qbyte *fout) {
	if ((bloc&7) == 0) {
		fout[(bloc>>3)] = 0;
	}
	fout[(bloc>>3)] |= bit << (bloc&7);
	bloc++;
}

/* Receive one bit from the input file (buffered) */
static int huff_get_bit (qbyte *fin) {
	int t;
	t = (fin[(bloc>>3)] >> (bloc&7)) & 0x1;
	bloc++;
	return t;
}

static node_t **huff_get_ppnode(huff_t* huff) {
	node_t **tppnode;
	if (!huff->freelist) {
		return &(huff->nodePtrs[huff->blocPtrs++]);
	} else {
		tppnode = huff->freelist;
		huff->freelist = (node_t **)*tppnode;
		return tppnode;
	}
}

static void huff_free_ppnode(huff_t* huff, node_t **ppnode) {
	*ppnode = (node_t *)huff->freelist;
	huff->freelist = ppnode;
}

/* Swap the location of these two nodes in the tree */
static void huff_swap (huff_t* huff, node_t *node1, node_t *node2) { 
	node_t *par1, *par2;

	par1 = node1->parent;
	par2 = node2->parent;

	if (par1) {
		if (par1->left == node1) {
			par1->left = node2;
		} else {
	      par1->right = node2;
		}
	} else {
		huff->tree = node2;
	}

	if (par2) {
		if (par2->left == node2) {
			par2->left = node1;
		} else {
			par2->right = node1;
		}
	} else {
		huff->tree = node1;
	}

	node1->parent = par2;
	node2->parent = par1;
}

/* Swap these two nodes in the linked list (update ranks) */
static void huff_swaplist(node_t *node1, node_t *node2) {
	node_t *par1;

	par1 = node1->next;
	node1->next = node2->next;
	node2->next = par1;

	par1 = node1->prev;
	node1->prev = node2->prev;
	node2->prev = par1;

	if (node1->next == node1) {
		node1->next = node2;
	}
	if (node2->next == node2) {
		node2->next = node1;
	}
	if (node1->next) {
		node1->next->prev = node1;
	}
	if (node2->next) {
		node2->next->prev = node2;
	}
	if (node1->prev) {
		node1->prev->next = node1;
	}
	if (node2->prev) {
		node2->prev->next = node2;
	}
}

/* Do the increments */
static void huff_increment(huff_t* huff, node_t *node) {
	node_t *lnode;

	if (!node) {
		return;
	}

	if (node->next != NULL && node->next->weight == node->weight) {
		lnode = *node->head;
		if (lnode != node->parent) {
			huff_swap(huff, lnode, node);
		}
		huff_swaplist(lnode, node);
	}
	if (node->prev && node->prev->weight == node->weight) {
		*node->head = node->prev;
	} else {
		*node->head = NULL;
		huff_free_ppnode(huff, node->head);
	}
	node->weight++;
	if (node->next && node->next->weight == node->weight) {
		node->head = node->next->head;
	} else { 
		node->head = huff_get_ppnode(huff);
		*node->head = node;
	}
	if (node->parent) {
		huff_increment(huff, node->parent);
		if (node->prev == node->parent) {
			huff_swaplist(node, node->parent);
			if (*node->head == node) {
				*node->head = node->parent;
			}
		}
	}
}

static void Huff_addRef(huff_t* huff, qbyte ch) {
	node_t *tnode, *tnode2;
	if (huff->loc[ch] == NULL) { /* if this is the first transmission of this node */
		tnode = &(huff->nodeList[huff->blocNode++]);
		tnode2 = &(huff->nodeList[huff->blocNode++]);

		tnode2->symbol = INTERNAL_NODE;
		tnode2->weight = 1;
		tnode2->next = huff->lhead->next;
		if (huff->lhead->next) {
			huff->lhead->next->prev = tnode2;
			if (huff->lhead->next->weight == 1) {
				tnode2->head = huff->lhead->next->head;
			} else {
				tnode2->head = huff_get_ppnode(huff);
				*tnode2->head = tnode2;
			}
		} else {
			tnode2->head = huff_get_ppnode(huff);
			*tnode2->head = tnode2;
		}
		huff->lhead->next = tnode2;
		tnode2->prev = huff->lhead;

		tnode->symbol = ch;
		tnode->weight = 1;
		tnode->next = huff->lhead->next;
		if (huff->lhead->next) {
			huff->lhead->next->prev = tnode;
			if (huff->lhead->next->weight == 1) {
				tnode->head = huff->lhead->next->head;
			} else {
				/* this should never happen */
				tnode->head = huff_get_ppnode(huff);
				*tnode->head = tnode2;
			}
		} else {
			/* this should never happen */
			tnode->head = huff_get_ppnode(huff);
			*tnode->head = tnode;
		}
		huff->lhead->next = tnode;
		tnode->prev = huff->lhead;
		tnode->left = tnode->right = NULL;

		if (huff->lhead->parent) {
			if (huff->lhead->parent->left == huff->lhead) { /* lhead is guaranteed to by the NYT */
				huff->lhead->parent->left = tnode2;
			} else {
				huff->lhead->parent->right = tnode2;
			}
		} else {
			huff->tree = tnode2; 
		}

		tnode2->right = tnode;
		tnode2->left = huff->lhead;

		tnode2->parent = huff->lhead->parent;
		huff->lhead->parent = tnode->parent = tnode2;

		huff->loc[ch] = tnode;

		huff_increment(huff, tnode2->parent);
	} else {
		huff_increment(huff, huff->loc[ch]);
	}
}

/* Get a symbol */
static int Huff_Receive (node_t *node, int *ch, qbyte *fin) {
	while (node && node->symbol == INTERNAL_NODE) {
		if (huff_get_bit(fin)) {
			node = node->right;
		} else {
			node = node->left;
		}
	}
	if (!node) {
		return 0;
//		Com_Error(ERR_DROP, "Illegal tree!");
	}
	return (*ch = node->symbol);
}

/* Get a symbol */
static void Huff_offsetReceive (node_t *node, int *ch, qbyte *fin, int *offset) {
	bloc = *offset;
	while (node && node->symbol == INTERNAL_NODE) {
		if (huff_get_bit(fin)) {
			node = node->right;
		} else {
			node = node->left;
		}
	}
	if (!node) {
		*ch = 0;
		return;
//		Com_Error(ERR_DROP, "Illegal tree!");
	}
	*ch = node->symbol;
	*offset = bloc;
}

/* Send the prefix code for this node */
static void huff_send(node_t *node, node_t *child, qbyte *fout) {
	if (node->parent) {
		huff_send(node->parent, node, fout);
	}
	if (child) {
		if (node->right == child) {
			huff_add_bit(1, fout);
		} else {
			huff_add_bit(0, fout);
		}
	}
}

/* Send a symbol */
static void Huff_transmit (huff_t *huff, int ch, qbyte *fout) {
	int i;
	if (huff->loc[ch] == NULL) { 
		/* node_t hasn't been transmitted, send a NYT, then the symbol */
		Huff_transmit(huff, NYT, fout);
		for (i = 7; i >= 0; i--) {
			huff_add_bit((char)((ch >> i) & 0x1), fout);
		}
	} else {
		huff_send(huff->loc[ch], NULL, fout);
	}
}

static void Huff_offsetTransmit (huff_t *huff, int ch, qbyte *fout, int *offset) {
	bloc = *offset;
	huff_send(huff->loc[ch], NULL, fout);
	*offset = bloc;
}

static void Huff_Decompress(sizebuf_t *mbuf, int offset) {
	int			ch, cch, i, j, size;
	qbyte		seq[65536];
	qbyte*		buffer;
	huff_t		huff;

	size = mbuf->cursize - offset;
	buffer = mbuf->data + offset;

	if ( size <= 0 ) {
		return;
	}

	memset(&huff, 0, sizeof(huff_t));
	// Initialize the tree & list with the NYT node 
	huff.tree = huff.lhead = huff.ltail = huff.loc[NYT] = &(huff.nodeList[huff.blocNode++]);
	huff.tree->symbol = NYT;
	huff.tree->weight = 0;
	huff.lhead->next = huff.lhead->prev = NULL;
	huff.tree->parent = huff.tree->left = huff.tree->right = NULL;

	cch = buffer[0]*256 + buffer[1];
	// don't overflow with bad messages
	if ( cch > mbuf->maxsize - offset ) {
		cch = mbuf->maxsize - offset;
	}
	bloc = 16;

	for ( j = 0; j < cch; j++ ) {
		ch = 0;
		// don't overflow reading from the messages
		// FIXME: would it be better to have an overflow check in get_bit ?
		if ( (bloc >> 3) > size ) {
			seq[j] = 0;
			break;
		}
		Huff_Receive(huff.tree, &ch, buffer);				/* Get a character */
		if ( ch == NYT ) {								/* We got a NYT, get the symbol associated with it */
			ch = 0;
			for ( i = 0; i < 8; i++ ) {
				ch = (ch<<1) + huff_get_bit(buffer);
			}
		}

		seq[j] = ch;									/* Write symbol */

		Huff_addRef(&huff, (qbyte)ch);								/* Increment node */
	}
	mbuf->cursize = cch + offset;
	memcpy(mbuf->data + offset, seq, cch);
}

static void Huff_Compress(sizebuf_t *mbuf, int offset) {
	int			i, ch, size;
	qbyte		seq[65536];
	qbyte*		buffer;
	huff_t		huff;

	size = mbuf->cursize - offset;
	buffer = mbuf->data+ + offset;

	if (size<=0) {
		return;
	}

	memset(&huff, 0, sizeof(huff_t));
	// Add the NYT (not yet transmitted) node into the tree/list */
	huff.tree = huff.lhead = huff.loc[NYT] =  &(huff.nodeList[huff.blocNode++]);
	huff.tree->symbol = NYT;
	huff.tree->weight = 0;
	huff.lhead->next = huff.lhead->prev = NULL;
	huff.tree->parent = huff.tree->left = huff.tree->right = NULL;

	seq[0] = (size>>8);
	seq[1] = size&0xff;

	bloc = 16;

	for (i=0; i<size; i++ ) {
		ch = buffer[i];
		Huff_transmit(&huff, ch, seq);						/* Transmit symbol */
		Huff_addRef(&huff, (qbyte)ch);								/* Do update */
	}

	bloc += 8;												// next byte

	mbuf->cursize = (bloc>>3) + offset;
	memcpy(mbuf->data+offset, seq, (bloc>>3));
}

static void Huff_Init(huffman_t *huff)
{
	int i, j;

	memset(&huff->compressor, 0, sizeof(huff_t));
	memset(&huff->decompressor, 0, sizeof(huff_t));

	// Initialize the tree & list with the NYT node 
	huff->decompressor.tree = huff->decompressor.lhead = huff->decompressor.ltail = huff->decompressor.loc[NYT] = &(huff->decompressor.nodeList[huff->decompressor.blocNode++]);
	huff->decompressor.tree->symbol = NYT;
	huff->decompressor.tree->weight = 0;
	huff->decompressor.lhead->next = huff->decompressor.lhead->prev = NULL;
	huff->decompressor.tree->parent = huff->decompressor.tree->left = huff->decompressor.tree->right = NULL;

	// Add the NYT (not yet transmitted) node into the tree/list */
	huff->compressor.tree = huff->compressor.lhead = huff->compressor.loc[NYT] =  &(huff->compressor.nodeList[huff->compressor.blocNode++]);
	huff->compressor.tree->symbol = NYT;
	huff->compressor.tree->weight = 0;
	huff->compressor.lhead->next = huff->compressor.lhead->prev = NULL;
	huff->compressor.tree->parent = huff->compressor.tree->left = huff->compressor.tree->right = NULL;

	for(i=0;i<256;i++)
	{
		for (j=0;j<huff->counts[i];j++)
		{
			Huff_addRef(&huff->compressor,	(qbyte)i);
			Huff_addRef(&huff->decompressor,	(qbyte)i);
		}
	}
	huff->built = true;
}




static huffman_t q3huff = 
{
	{
	0x3D1CB, 0x0A0E9, 0x01894, 0x01BC2, 0x00E92, 0x00EA6, 0x017DE, 0x05AF3,
	0x08225, 0x01B26, 0x01E9E, 0x025F2, 0x02429, 0x0436B, 0x00F6D, 0x006F2,
	0x02060, 0x00644, 0x00636, 0x0067F, 0x0044C, 0x004BD, 0x004D6, 0x0046E,
	0x006D5, 0x00423, 0x004DE, 0x0047D, 0x004F9, 0x01186, 0x00AF5, 0x00D90,
	0x0553B, 0x00487, 0x00686, 0x0042A, 0x00413, 0x003F4, 0x0041D, 0x0042E,
	0x006BE, 0x00378, 0x0049C, 0x00352, 0x003C0, 0x0030C, 0x006D8, 0x00CE0,
	0x02986, 0x011A2, 0x016F9, 0x00A7D, 0x0122A, 0x00EFD, 0x0082D, 0x0074B,
	0x00A18, 0x0079D, 0x007B4, 0x003AC, 0x0046E, 0x006FC, 0x00686, 0x004B6,
	0x01657, 0x017F0, 0x01C36, 0x019FE, 0x00E7E, 0x00ED3, 0x005D4, 0x005F4,
	0x008A7, 0x00474, 0x0054B, 0x003CB, 0x00884, 0x004E0, 0x00530, 0x004AB,
	0x006EA, 0x00436, 0x004F0, 0x004F2, 0x00490, 0x003C5, 0x00483, 0x004A2,
	0x00543, 0x004CC, 0x005F9, 0x00640, 0x00A39, 0x00800, 0x009F2, 0x00CCB,
	0x0096A, 0x00E01, 0x009C8, 0x00AF0, 0x00A73, 0x01802, 0x00E4F, 0x00B18,
	0x037AD, 0x00C5C, 0x008AD, 0x00697, 0x00C88, 0x00AB3, 0x00DB8, 0x012BC,
	0x00FFB, 0x00DBB, 0x014A8, 0x00FB0, 0x01F01, 0x0178F, 0x014F0, 0x00F54,
	0x0131C, 0x00E9F, 0x011D6, 0x012C7, 0x016DC, 0x01900, 0x01851, 0x02063,
	0x05ACB, 0x01E9E, 0x01BA1, 0x022E7, 0x0153D, 0x01183, 0x00E39, 0x01488,
	0x014C0, 0x014D0, 0x014FA, 0x00DA4, 0x0099A, 0x0069E, 0x0071D, 0x00849,
	0x0077C, 0x0047D, 0x005EC, 0x00557, 0x004D4, 0x00405, 0x004EA, 0x00450,
	0x004DD, 0x003EE, 0x0047D, 0x00401, 0x004D9, 0x003B8, 0x00507, 0x003E5,
	0x006B1, 0x003F1, 0x004A3, 0x0036F, 0x0044B, 0x003A1, 0x00436, 0x003B7,
	0x00678, 0x003A2, 0x00481, 0x00406, 0x004EE, 0x00426, 0x004BE, 0x00424,
	0x00655, 0x003A2, 0x00452, 0x00390, 0x0040A, 0x0037C, 0x00486, 0x003DE,
	0x00497, 0x00352, 0x00461, 0x00387, 0x0043F, 0x00398, 0x00478, 0x00420,
	0x00D86, 0x008C0, 0x0112D, 0x02F68, 0x01E4E, 0x00541, 0x0051B, 0x00CCE,
	0x0079E, 0x00376, 0x003FF, 0x00458, 0x00435, 0x00412, 0x00425, 0x0042F,
	0x005CC, 0x003E9, 0x00448, 0x00393, 0x0041C, 0x003E3, 0x0042E, 0x0036C,
	0x00457, 0x00353, 0x00423, 0x00325, 0x00458, 0x0039B, 0x0044F, 0x00331,
	0x0076B, 0x00750, 0x003D0, 0x00349, 0x00467, 0x003BC, 0x00487, 0x003B6,
	0x01E6F, 0x003BA, 0x00509, 0x003A5, 0x00467, 0x00C87, 0x003FC, 0x0039F,
	0x0054B, 0x00300, 0x00410, 0x002E9, 0x003B8, 0x00325, 0x00431, 0x002E4,
	0x003F5, 0x00325, 0x003F0, 0x0031C, 0x003E4, 0x00421, 0x02CC1, 0x034C0
	},
	0x286f2e8d
};

int Huff_PreferedCompressionCRC (void)
{
	if (!net_compress.ival)
		return 0;
	return q3huff.crc;
}
huffman_t *Huff_CompressionCRC(int crc)
{
	huffman_t *huff = NULL;

	if (crc == q3huff.crc)
		huff = &q3huff;
	else
		huff = NULL;

	if (huff && !huff->built)
		Huff_Init(huff);
	return huff;
}
void Huff_DecryptPacket(sizebuf_t *msg, int offset)
{
	//decompress using a dynamic from-nil tree
	Huff_Decompress(msg, offset);
}
void Huff_EncryptPacket(sizebuf_t *msg, int offset)
{
	//decompress using a dynamic from-nil tree
	Huff_Compress(msg, offset);
}
int Huff_GetByte(qbyte *buffer, int *count)
{
	int ch;
	Huff_offsetReceive (q3huff.decompressor.tree, &ch, buffer, count);
	return ch;
}
void Huff_EmitByte(int ch, qbyte *buffer, int *count)
{
	Huff_offsetTransmit(&q3huff.compressor, ch, buffer, count);
}

void Huff_CompressPacket(huffman_t *huff, sizebuf_t *msg, int offset)
{
	qbyte	buffer[MAX_OVERALLMSGLEN];
	qbyte	*data;
	int		outLen;
	int		inLen;
	int		i;

	data = msg->data + offset;
	inLen = msg->cursize - offset;	
	if (inLen <= 0 || inLen >= MAX_OVERALLMSGLEN)
	{
		//panic!
		return;
	}

	outLen = 0;
	for (i=0; i < inLen; i++)
	{
		Huff_offsetTransmit(&huff->compressor, data[i], buffer, &outLen);

		if (outLen > inLen)
			break;
		if (outLen > MAX_OVERALLMSGLEN-64)
		{
			Con_Printf("Huffman overflow\n");
			//panic
			data[0] = 0x80;
			msg->cursize = offset+1;
			return;
		}
	}

	if (outLen > inLen)
	{
		memmove(data+1, data, inLen);
		data[0] = 0x80;	//this would have grown the packet.
		msg->cursize+=1;
		return;	//cap it at only 1 qbyte growth.
	}

	msg->cursize = offset + outLen;
	{	//add the bitcount
		data[0] = (outLen<<3) - outLen;
		data+=1;
		msg->cursize+=1;
	}
	if (msg->cursize > msg->maxsize)
		Sys_Error("Compression became too large\n");
	memcpy(data, buffer, outLen);
}
void Huff_DecompressPacket(huffman_t *huff, sizebuf_t *msg, int offset)
{
	qbyte	buffer[MAX_OVERALLMSGLEN];
	qbyte	*data;
	int		outLen;
	int		inLen;
	int		i, ch;

	data = msg->data + offset;
	inLen = msg->cursize - offset;	
	if (inLen <= 0 || inLen >= MAX_OVERALLMSGLEN)
	{
		return;
	}

	inLen<<=3;
	{	//add the bitcount
		inLen = inLen-8-data[0];
		if (data[0]&0x80)
		{	//packet would have grown.
			msg->cursize -= 1;
			memmove(data, data+1, msg->cursize);
			return;	//this never happened, okay?
		}
		data+=1;
	}

	outLen = 0;
	for(i=0; outLen < inLen; i++)
	{
		if (i == MAX_OVERALLMSGLEN)
			Sys_Error("Decompression became too large\n");
		Huff_offsetReceive (huff->decompressor.tree, &ch, data, &outLen);
		buffer[i] = ch;
	}
	
	msg->cursize = offset + i;
	if (msg->cursize > msg->maxsize)
		Sys_Error("Decompression became too large\n");
	memcpy(msg->data + offset, buffer, i);
}
#endif

