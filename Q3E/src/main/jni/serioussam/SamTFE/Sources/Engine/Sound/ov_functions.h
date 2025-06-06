/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */


// DLLFUNCTION(dll, output, name, inputs, params, required)

DLLFUNCTION( OVF, int, ov_clear, (OggVorbis_File *vf), 4, 0)
DLLFUNCTION( OVF, int, ov_open, (FILE *f, OggVorbis_File *vf, char *initial, long ibytes), 16,0);
DLLFUNCTION( OVF, int, ov_open_callbacks, (void *datasource, OggVorbis_File *vf, char *initial, long ibytes, ov_callbacks callbacks), 32,0);
#ifdef USE_TREMOR
DLLFUNCTION( OVF, long, ov_read, (OggVorbis_File *vf,char *buffer,int length, int *bitstream), 16,0);
#else
DLLFUNCTION( OVF, long, ov_read, (OggVorbis_File *vf,char *buffer,int length, int bigendianp,int word,int sgned,int *bitstream), 28,0);
#endif
DLLFUNCTION( OVF, vorbis_info *, ov_info, (OggVorbis_File *vf,int link), 8,0);
#ifdef USE_TREMOR
DLLFUNCTION( OVF, int, ov_time_seek, (OggVorbis_File *vf, int64_t pos), 12,0);
#else
DLLFUNCTION( OVF, int, ov_time_seek, (OggVorbis_File *vf, double pos), 12,0);
#endif