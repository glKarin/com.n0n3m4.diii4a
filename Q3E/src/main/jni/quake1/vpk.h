/*
Copyright (C) 2020-2021 David Knapp (Cloudwalk)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef VPK_H
#define VPK_H

#include <stdint.h>

/*
 * The VPK format is Valve's package format for Source engine games,
 * used to store game content.
 * 
 * Game content is spread across multiple VPK files. A single, special
 * VPK file, ending in _dir.vpk, contains a centralized directory
 * tree for all of the other files, and has its own header.
 * Although content can be stored in the directory file.
 * 
 * This is useful for navigating game content without having
 * to guess which VPK some file belongs to, while also
 * making game updates more efficient by spreading content
 * across multiple files, where opening and closing thousands
 * of loose files to update them is less efficient.
 */

const uint32_t VPK_SIGNATURE = 0x55aa1234;

typedef struct dvpk_header_v1_s
{
	const uint32_t signature; // Should always be VPK_SIGNATURE
	const uint32_t version; // Should always be 1

	// Size of directory tree
	uint32_t tree_size;
} dvpk_header_v1_t;

typedef struct dvpk_header_v2_s
{
	const uint32_t signature; // Should always be VPK_SIGNATURE
	const uint32_t version; // Should always be 2

	// Size of directory tree
	uint32_t tree_size;

	// Section sizes
	uint32_t filedata_size;
	uint32_t archivemd5_size;
	uint32_t othermd5_size;
	uint32_t signature_size;
} dvpk_header_v2_t;

typedef struct dvpk_dir_entry_s
{
	uint32_t crc32;
	uint16_t preloadbytes;

	uint16_t archiveindex;
	uint32_t entryoffset;
	uint32_t entrylength;
	const uint16_t terminator; // Should always be 0xFFFF
} dvpk_dir_entry_t;

typedef struct dvpk_archive_md5_entry_s
{
	uint32_t archiveindex;
	uint32_t startingoffset;
	uint32_t count;
	int8_t md5sum[16];
} dvpk_archive_md5_entry_t;

typedef struct dvpk_other_md5_entry_s
{
	int8_t treesum[16];
	int8_t archivemd5sum[16];
	int8_t unknown[16]; // ??
} dvpk_other_md5_entry_t;

typedef struct dvpk_signature_entry_s
{
	uint32_t pubkeysize; // Always 160
	int8_t pubkey[160];
	uint32_t signaturesize; // Always 128
	int8_t signature[128];
} dvpk_signature_entry_t;

#endif
