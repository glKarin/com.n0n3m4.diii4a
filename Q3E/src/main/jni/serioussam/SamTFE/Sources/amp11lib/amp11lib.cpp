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

// amp11lib - library interface layer and asynchronious player

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "binfile/binfstd.h"
#include "binfile/binfarc.h"
#include "ampdec.h"

#include "amp11lib.h"

// ------------------------------------

// type of stream indentifier for each stream handle
enum StreamType {
  ST_INVALID=0,
  ST_BEFOREVALID, // low marker for valid values
  ST_UNUSED,
  ST_INPUT,
  ST_DECODER,
  ST_SUBFILE,
  ST_AFTERVALID, // high marker for valid values
};

#define SLAVESPERSTREAM 2
#define SLAVE_REDIR           0     // base slave used for redirection
#define SLAVE_DECODERSOURCE   1     // mpeg decoder stream reads mpeg from another file

struct Stream {
  ALsint32 st_nReferences;                // reference counter
  StreamType st_stType;                   // type of the stream, or unused
  ALhandle st_ahSlaves[SLAVESPERSTREAM];  // slave streams referenced by this one
  float st_fBytesPerSec;        // bytes per second for decoder streams (used for seeking)
  union {     // holds pointer to the actual stream, NULL if unused
    binfile *st_binfile;
    sbinfile *st_sbinfile;
    ampegdecoder *st_ampegdecoder;
    abinfile *st_abinfile;
  };
};

// stream structures are stored here
// stream number 0 is never used, so that handle 0 is used for errors
#define MAX_STREAMS 64
static struct Stream _astStreams[MAX_STREAMS];

// set between calls to alInitSystem() and alEndSystem()
static ALbool _bLibraryInitialized = ALfalse;

//------------------------------------

// check if a stream handle is valid
static ALbool IsStreamHandleValid(ALhandle hStream)
{
  // if library is not initialized
  if (!_bLibraryInitialized) {
    // no streams are valid
    assert(ALfalse);
    return ALfalse;
  }

  // if handle is out of range
  if (hStream<=0 || hStream>MAX_STREAMS) {
    assert(ALfalse);
    return ALfalse;
  }
  Stream &st = _astStreams[hStream];
  // if handle is not used or invalid type
  if (st.st_stType==ST_UNUSED||st.st_stType==ST_INVALID) {
    assert(ALfalse);
    return ALfalse;
  }

  // if bad pointer (paranoia check - this should not be external error)
  if (st.st_binfile==NULL) {
    assert(ALfalse);
    return ALfalse;
  }

  // reference count should be valid (paranoia check)
  if (st.st_nReferences<=0) {
    assert(ALfalse);
    return ALfalse;
  }

  return ALtrue;
}

// find an unused stream handle
static ALhandle FindFreeHandle(void)
{
  // for each stream (note that stream 0 is not used!)
  for(int ist=1; ist<MAX_STREAMS; ist++) {
    assert(
      _astStreams[ist].st_stType>ST_BEFOREVALID &&
      _astStreams[ist].st_stType<ST_AFTERVALID);
    // if unused
    if (_astStreams[ist].st_stType == ST_UNUSED) {
      // use that one
      return ist;
    }
  }
  // if not found, error
  return 0;
}

// add one reference to a stream
static ALbool AddStreamReference(ALhandle hStream)
{
  // if handle is invalid
  if (!IsStreamHandleValid(hStream)) {
    // it should not happen
    assert(ALfalse);
    // do nothing
    return ALfalse;
  }

  // this just increases the reference count
  _astStreams[hStream].st_nReferences++;
  return ALtrue;
}

// remove one reference from a stream - if no references, stream is closed
static void RemStreamReference(ALhandle hStream)
{
  // if handle is invalid
  if (!IsStreamHandleValid(hStream)) {
    // it should not happen
    assert(ALfalse);
    // do nothing
    return;
  }

  Stream &st = _astStreams[hStream];

  // decrease the reference count
  st.st_nReferences--;
  // if the stream is not used anymore
  if (st.st_nReferences<=0) {
    // delete the stream's binfile
    delete _astStreams[hStream].st_binfile;
    // mark as unused
    _astStreams[hStream].st_binfile = NULL;
    _astStreams[hStream].st_stType = ST_UNUSED;

    // for each possible slave stream
    for (int iSlave=0; iSlave<SLAVESPERSTREAM; iSlave++) {
      // if there is a slave
      if (_astStreams[hStream].st_ahSlaves[iSlave]!=0) {
        // remove reference from the slave stream
        RemStreamReference(_astStreams[hStream].st_ahSlaves[iSlave]);
        _astStreams[hStream].st_ahSlaves[iSlave] = 0;
      }
    }
  }
}

// add a slave stream to one stream
ALbool SetSlaveStream(ALhandle hStream, ALhandle hSlave, ALsint32 iSlave)
{
  assert(iSlave>=0 && iSlave<SLAVESPERSTREAM);

  // if handle is invalid
  if (!IsStreamHandleValid(hStream)) {
    // it should not happen
    assert(ALfalse);
    // do nothing
    return ALfalse;
  }

  Stream &st = _astStreams[hStream];

  // if already has that slave
  if (st.st_ahSlaves[iSlave]!=0) {
    // remove that reference
    RemStreamReference(st.st_ahSlaves[iSlave]);
    st.st_ahSlaves[iSlave] = 0;
  }

  // if setting slave to nothing
  if (hSlave==0) {
    // that's all
    return ALtrue;
  }

  // if handle is invalid
  if (!IsStreamHandleValid(hSlave)) {
    // it should not happen
    assert(ALfalse);
    // do nothing
    return ALfalse;
  }

  // add one reference to the slave
  if (!AddStreamReference(hSlave)) {
    return ALfalse;
  }

  // remember slave
  st.st_ahSlaves[iSlave] = hSlave;
  return ALtrue;
}

// ------------------------------------
// actual interface functions
  
// open a standard file with given filename for reading
AMP11LIB_API ALhandle WINAPI alOpenInputFile(const char *strFileName)
{
  // get new stream
  ALhandle hStream = FindFreeHandle();
  // if none available
  if (hStream==0) {
    // report error
    return 0;
  }
  Stream &st = _astStreams[hStream];

  // initialize it as binary input file
  st.st_stType = ST_INPUT;
  st.st_nReferences = 1;
  st.st_sbinfile = new sbinfile;

  // if cannot open it
  if (st.st_sbinfile->open(strFileName, sbinfile::openro) < 0) {
    // close it
    alClose(hStream);
    // report error
    return 0;
  }

  // if all fine, return the handle
  return hStream;
}

// open mpeg decoder that reads from a file of given handle
AMP11LIB_API ALhandle WINAPI alOpenDecoder(ALhandle hFile)
{
  // if the given file handle is invalid
  if (!IsStreamHandleValid(hFile)) {
    // report error
    return 0;
  }
  // get the source stream
  Stream &stFile = _astStreams[hFile];
  // if it is not valid for input
  if (stFile.st_stType!=ST_INPUT && stFile.st_stType!=ST_SUBFILE) {
    // report error
    return 0;
  }

  // get new stream
  ALhandle hStream = FindFreeHandle();
  // if none available
  if (hStream==0) {
    // report error
    return 0;
  }
  Stream &st = _astStreams[hStream];

  // initialize it as decoder
  st.st_stType = ST_DECODER;
  st.st_nReferences = 1;
  st.st_ampegdecoder = new ampegdecoder;
  // if cannot reference the source stream
  if (!SetSlaveStream(hStream, hFile, SLAVE_DECODERSOURCE)) {
    // close it
    alClose(hStream);
    // report error
    return 0;
  }

  // if cannot open it
  int freq,stereo;
  if (st.st_ampegdecoder->open(*stFile.st_binfile, freq, stereo, 1, 0, 2) < 0) {
    // close it
    alClose(hStream);
    // report error
    return 0;
  }

  // calculate stream bandwidth
  st.st_fBytesPerSec = (stereo?4:2)*freq;

  // if all fine, return the handle
  return hStream;
}

// open a sub-file inside an archive file
AMP11LIB_API ALhandle WINAPI alOpenSubFile(ALhandle hFile, ALsize sOffset, ALsize sSize)
{
   // if the given file handle is invalid
  if (!IsStreamHandleValid(hFile)) {
    // report error
    return 0;
  }
  // get the source stream
  Stream &stFile = _astStreams[hFile];
  // if it is not valid for input
  if (stFile.st_stType!=ST_INPUT && stFile.st_stType!=ST_SUBFILE) {
    // report error
    return 0;
  }

  // get new stream
  ALhandle hStream = FindFreeHandle();
  // if none available
  if (hStream==0) {
    // report error
    return 0;
  }
  Stream &st = _astStreams[hStream];

  // initialize it as subfile
  st.st_stType = ST_SUBFILE;
  st.st_nReferences = 1;
  st.st_abinfile = new abinfile;
  // if cannot reference the source stream
  if (!SetSlaveStream(hStream, hFile, SLAVE_DECODERSOURCE)) {
    // close it
    alClose(hStream);
    // report error
    return 0;
  }

  // if cannot open it
  if (st.st_abinfile->open(*stFile.st_binfile, sOffset, sSize) < 0) {
    // close it
    alClose(hStream);
    // report error
    return 0;
  }

  // if all fine, return the handle
  return hStream;
}

// get file header of mpx file from a file of given handle
AMP11LIB_API ALbool WINAPI alGetMPXHeader(ALhandle hFile, ALsint32 *piLayer, 
  ALsint32 *piVersion, ALsint32 *piFrequency, ALbool *pbStereo, ALsint32 *piRate)
{
  // if the given file handle is invalid
  if (!IsStreamHandleValid(hFile)) {
    // report error
    return 0;
  }
  // get the source stream
  Stream &stFile = _astStreams[hFile];
  // if it is not valid for input
  if (stFile.st_stType!=ST_INPUT && stFile.st_stType!=ST_SUBFILE) {
    // report error
    return 0;
  }

  // get the header
  return ampegdecoder::getheader(*stFile.st_binfile, 
    *piLayer, *piVersion, *piFrequency, *pbStereo, *piRate);
}

// close any open amp11 stream
AMP11LIB_API void WINAPI alClose(ALhandle hStream)
{
   // just remove one reference from it - it will free it if not used anymore
  RemStreamReference(hStream);
}

// read a chunk of bytes from given stream
AMP11LIB_API ALsize WINAPI alRead(ALhandle hStream, void *pvBuffer, ALsize size)
{
   // if handle is invalid
  if (!IsStreamHandleValid(hStream)) {
    // do nothing
    return 0;
  }

  return _astStreams[hStream].st_binfile->read(pvBuffer, size);
}

AMP11LIB_API void WINAPI alDecSeekAbs(ALhandle hDecoder, ALfloat fSeconds)
{
   // if handle is invalid
  if (!IsStreamHandleValid(hDecoder)) {
    // do nothing
    return;
  }

  Stream &st = _astStreams[hDecoder];

  // if not a decoder
  if (st.st_stType != ST_DECODER) {
    // do nothing
    return;
  }

  // set position
  st.st_ampegdecoder->seek(int(st.st_fBytesPerSec*fSeconds));
}

AMP11LIB_API ALfloat WINAPI alDecGetLen(ALhandle hDecoder)
{
  // if handle is invalid
  if (!IsStreamHandleValid(hDecoder)) {
    // do nothing
    return 0.0f;
  }

  Stream &st = _astStreams[hDecoder];

  // if not a decoder
  if (st.st_stType != ST_DECODER) {
    // do nothing
    return 0.0f;
  }

  // read length
  return st.st_ampegdecoder->length()/st.st_fBytesPerSec;
}

//----------------------------------------------

// initialize amp11lib before calling any of its functions
AMP11LIB_API void WINAPI alInitLibrary(void)
{
  // if already initialized
  if (_bLibraryInitialized) {
    // do nothing
    return;
  }
  // set all streams to unused
  for(int ist=0; ist<MAX_STREAMS; ist++) {
    _astStreams[ist].st_stType      = ST_UNUSED;
    _astStreams[ist].st_nReferences = 0;
    _astStreams[ist].st_binfile     = NULL;
    for (int iSlave=0; iSlave<SLAVESPERSTREAM; iSlave++) {
      _astStreams[ist].st_ahSlaves[iSlave] = 0;
    }
  }
  // mark as initialized
  _bLibraryInitialized = ALtrue;
}

// cleanup amp11lib when not needed anymore
// NOTE: do not try to call this using atexit(). it just won't work
// because all threads are force-killed _before_ any at-exit function
// is called. sigh.
AMP11LIB_API void WINAPI alEndLibrary(void)
{
  // if not initialized
  if (!_bLibraryInitialized) {
    // do nothing
    return;
  }

  // disable possible redirection if on
  //alDisableRedirection(); // FIXME DG: I had to compile this out, is it useful?

  // free all streams that are not freed
  for(int ist=0; ist<MAX_STREAMS; ist++) {
    if (_astStreams[ist].st_stType != ST_UNUSED) {
      alClose(ist);
    }
  }

  // check that reference counting did its job fine

  // mark as not initialized
  _bLibraryInitialized = ALfalse;
}
