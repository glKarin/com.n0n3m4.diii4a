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

DLLFUNCTION( A11L, void, alInitLibrary, (void), 0,0);
DLLFUNCTION( A11L, void, alEndLibrary, (void), 0,0);
DLLFUNCTION( A11L, ALhandle, alOpenInputFile, (const char *strFileName), 4,0);
DLLFUNCTION( A11L, ALhandle, alOpenDecoder, (ALhandle hFile), 4,0);
DLLFUNCTION( A11L, ALhandle, alOpenSubFile, (ALhandle hFile, ALsize sOffset, ALsize sSize), 12,0);
DLLFUNCTION( A11L, ALbool, alGetMPXHeader, (ALhandle hFile, ALsint32 *piLayer, 
  ALsint32 *piVersion, ALsint32 *piFrequency, ALbool *pbStereo, ALsint32 *piRate), 24, 0);
DLLFUNCTION( A11L, void, alClose, (ALhandle hStream), 4,0);
DLLFUNCTION( A11L, ALsize, alRead, (ALhandle hStream, void *pvBuffer, ALsize size), 12,0);
DLLFUNCTION( A11L, void, alDecSeekAbs, (ALhandle hDecoder, ALfloat fSeconds), 8,0);
DLLFUNCTION( A11L, ALfloat, alDecGetLen, (ALhandle hDecoder), 4,0);
