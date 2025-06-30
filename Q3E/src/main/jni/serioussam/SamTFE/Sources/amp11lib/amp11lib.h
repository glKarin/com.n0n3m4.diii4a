/* Copyright (c) 1997-2001 Niklas Beisert, Alen Ladavac. 
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

// AMP11LIB_EXPORTS should be defined when compiling the DLL, and not defined otherwise
#ifdef _WIN32
  #ifdef AMP11LIB_EXPORTS
    #define AMP11LIB_API __declspec(dllexport)
  #else
    #define AMP11LIB_API __declspec(dllimport)
  #endif

  #ifndef WINAPI
    #define WINAPI  __stdcall
  #endif
#else // not windows - don't need additional stuff in function sigs
  #define AMP11LIB_API __attribute__((visibility("default")))
  #define WINAPI
#endif // _WIN32



// general types used
typedef signed char ALsint8;
typedef unsigned char ALuint8;
typedef signed short ALsint16;
typedef unsigned short ALuint16;
typedef signed int ALsint32;
typedef unsigned int ALuint32;
typedef signed int ALsize;
typedef int ALbool;
typedef float ALfloat;
#define ALtrue  1
#define ALfalse 0

// handle type for amp11 streams
typedef ALsint32 ALhandle;

#ifdef __cplusplus
extern "C" {
#endif

////
// library init/end

// initialize amp11lib before calling any of its functions
AMP11LIB_API void WINAPI alInitLibrary(void);
// cleanup amp11lib when not needed anymore
AMP11LIB_API void WINAPI alEndLibrary(void);

////
// basic open/close functions

// open a standard file with given filename for reading
AMP11LIB_API ALhandle WINAPI alOpenInputFile(const char *strFileName);
// open a standard file with given filename for writing
AMP11LIB_API ALhandle WINAPI alOpenOutputFile(const char *strFileName);
// open http file with given url, using proxy (proxy is optional)
AMP11LIB_API ALhandle WINAPI alOpenHttpFile(const char *strURL, const char *strProxy);
// open mpeg decoder that reads from a file of given handle
AMP11LIB_API ALhandle WINAPI alOpenDecoder(ALhandle hFile);
// open a sub-file inside an archive file
AMP11LIB_API ALhandle WINAPI alOpenSubFile(ALhandle hFile, ALsize sOffset, ALsize sSize);
// open sound output with given settings (frequency is in Hz, prebuffer time in seconds,
// use device=-1 for default audio output device)
AMP11LIB_API ALhandle WINAPI alOpenPlayer(ALsint32 iDevice, ALsint32 iFreq, ALbool bStereo, 
    ALfloat fPrebuffer);

// get file header of mpx file from a file of given handle
AMP11LIB_API ALbool WINAPI alGetMPXHeader(ALhandle hFile, ALsint32 *piLayer, 
  ALsint32 *piVersion, ALsint32 *piFrequency, ALbool *pbStereo, ALsint32 *piRate);
// get descriptive name of a given player device (returns size of name, or 0 if error)
AMP11LIB_API ALsize WINAPI alDescribePlayerDevice(ALsint32 iDevice,
    char *strNameBuffer, ALsize sizeNameBuffer);

// close any open amp11 stream
AMP11LIB_API void WINAPI alClose(ALhandle hStream);

////
// standard stream read/write functions

// read a chunk of bytes from given stream
AMP11LIB_API ALsize WINAPI alRead(ALhandle hStream, void *pvBuffer, ALsize size);
// write a chunk of bytes to given stream
AMP11LIB_API ALsize WINAPI alWrite(ALhandle hStream, void *pvBuffer, ALsize size);

////
// decoder control functions

// set output volume (0-1)
AMP11LIB_API void WINAPI alDecSetVolume(ALhandle hDecoder, ALfloat fVolume);
// seek absolute/relative (in seconds)
AMP11LIB_API void WINAPI alDecSeekAbs(ALhandle hDecoder, ALfloat fSeconds);
AMP11LIB_API void WINAPI alDecSeekRel(ALhandle hDecoder, ALfloat fSecondsDelta);
// get current position (in seconds)
AMP11LIB_API ALfloat WINAPI alDecGetPos(ALhandle hDecoder);
// get total stream length (in seconds)
AMP11LIB_API ALfloat WINAPI alDecGetLen(ALhandle hDecoder);

////
// stream redirection functions

// enable stream redirection with given timer interval (in seconds)
AMP11LIB_API ALbool WINAPI alEnableRedirection(ALfloat fInterval);
// disable stream redirection
AMP11LIB_API void WINAPI alDisableRedirection(void);
// redirect source stream to the target stream
AMP11LIB_API ALbool WINAPI alSetRedirection(ALhandle hSource, ALhandle hTarget);
// get target stream for given source stream
AMP11LIB_API ALhandle WINAPI alGetRedirection(ALhandle hSource);

#ifdef __cplusplus
} // extern "C"
#endif
