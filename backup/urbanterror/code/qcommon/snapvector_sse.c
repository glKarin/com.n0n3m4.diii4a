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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
* GNU inline asm version of qsnapvector - SSE version
*/

static unsigned char ssemask[16] __attribute__((aligned(16))) =
{
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00"
};


void Sys_SnapVector( float *vec )
{
       __asm__ volatile
        (
                "movaps (%0), %%xmm1\n"
                "movups (%1), %%xmm0\n"
                "movaps %%xmm0, %%xmm2\n"
                "andps %%xmm1, %%xmm0\n"
                "andnps %%xmm2, %%xmm1\n"
                "cvtps2dq %%xmm0, %%xmm0\n"
                "cvtdq2ps %%xmm0, %%xmm0\n"
                "orps %%xmm1, %%xmm0\n"
                "movups %%xmm0, (%1)\n"
                :
                : "r" (ssemask), "r" (vec)
                : "memory", "%xmm0", "%xmm1", "%xmm2"
        );

}