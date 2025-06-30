          BINFILE
         =========

binfile is a class library for C++ for stream and file io. I needed
something one can easily enhance and use and made binfile. binfile offers a
set of implementations like files, tcp, http, wave output, memory files,
etc., but that's not all one can do with it by far.


binfile is under the GNU General Public License Version 2 (see file COPYING).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



      1. types, constants
     ---------------------

  intm
integer, at least 32 bits. (probably int)
  uintm
unsigned integer, at least 32 bits. (probably unsigned int)
  boolm
boolean type. (probably intm)
  errstat (return value)
negative: error, zero: ok
  binfilepos
file position, at least 32 bit integer
  modeopen
binfile is opened, only opened binfiles can be closed...
  moderead
binfile can be read from
  modewrite
binfile can be written to
  modeseek
binfile can be seeked. read and write streams are synchronized. (file)
  modeappend
binfile can changed in size. (I wonder if this is necessary.)



      2. basic io
     -------------

  int binfile::getmode();
  binfile::operator int();
returns the mode of this file. (mode...)

  binfilepos binfile::ioctl(int code);
  binfilepos binfile::ioctl(int code, binfilepos len);
  binfilepos binfile::ioctl(int code, void *buf, binfilepos len);
general function for anything special you can do. the codes are discussed
later.


    modeopen required

errstat binfile::close();
closes the binfile.


    moderead required

  binfilepos binfile::read(void *buf, binfilepos len);
reads len bytes from the stream into buf. returns the number of bytes read.
if buf is 0 len bytes of the input stream will be discarded.

  binfilepos binfile::peek(void *buf, binfilepos len);
same as binfile::read, only the read bytes are not removed from the stream
and will be read with the next read/peek again (no buf==0 please!). peek in
streams can only give you as much as can be buffered, otherwise the data
would be lost and that's not the sense of a peek, is it?

  boolm binfile::eread(void *buf, binfilepos len);
same as binfile::read, but returns true if read len bytes sucessfully.

  boolm binfile::epeek(void *buf, binfilepos len);
obvious.


    modewrite required

  binfilepos binfile::write(const void *buf, binfilepos len);
writes len bytes from buf to the stream. returns the number of bytes
written.

  boolm binfile::ewrite(const void *buf, binfilepos len);
yeah, you guessed it.


    modeseek required

  binfilepos binfile::seek(binfilepos pos);
seeks to byte pos. returns the actual position.

  binfile &operator [](binfilepos pos);
seeks to pos, but returns a reference to this binfile. use for
file[pos].read(buf,len); to read len bytes from pos into buf.

  binfilepos binfile::seekcur(binfilepos pos);
seeks pos from current position. returns the actual position.

  binfilepos binfile::seekend(binfilepos pos);
seeks pos from end of file (negative values!). returns the actual position.

  binfilepos binfile::tell();
tells you where we are.

  binfilepos binfile::lenght();
how long are we?

  boolm binfile::eof();
end of file?


    input bitstream functions (moderead)

when you switch from bit to byte operation make sure you synchronize to a
byte boundary to make sure you stay out of trouble.

  intm binfile::getrbitpos();
read bit position in a byte.

  void binfile::rflushbits(intm n);
tell the stream, you don't care about the next n bits and that it should go
fuck them.

  void binfile::rsyncbye();
lost in the middle of a byte? need some help? here comes the ultimate
solution: binfile::rsyncbyte() and you immediate get out of this shit.

  boolm binfile::peekbit();
what's the next bit? tell me! but keep quiet i asked, will ya?

  uintm binfile::peekbits(intm n);
you need more?

  boolm binfile::getbit();
Bitte ein Bit!

  uintm binfile::getbits(intm n);
Ich schmeiss ne Runde! (keep it below 24, though. 25 Bit are way too much
anyway, don't you think so?)


    output bitstream functions (modewrite)

please note that input and output bitstreams are synchonized and use the
same set of parameters if the binfile is a file (modeseek).

  intm binfile::getwbitpos();
returns bit position in a byte

  boolm binfile::wsyncbyte();
sync to next byte. returns success.

  boolm binfile::putbit(boolm bit);
write one bit.

  boolm binfile::putbits(uintm bits, int n);
write several bits.



      3. ioctls
     -----------

This is a general interface to do anything else with the binfile. there are
3 forms of ioctls:
  binfile::ioctl(binfile::ioctlxxx);
  binfile::ioctl(binfile::ioctlyyy, binfilepos par);
  binfile::ioctl(binfile::ioctlzzz, void *ptr, binfilepos len);
In the following list I will use a short form for these:
  ioctlxxx()
  ioctlyyy(par)
  ioctlzzz(ptr,len)
remember this will not work as code...

  ioctlrtell()
tells you the read position of a stream.
  ioctlwtell()
tells you the write position of a stream.
  ioctlrlen()
returns the read length of the stream.
  ioctlreof()
  ioctlweof()
returns if the end of the stream is reached.
  ioctlrfill(fill)
  ioctlrfillget()
sets/gets the behavior when a read/peek does not complete. when fill is –1
the unused buffer will not be touched, otherwise it will be filled with
fill. returns the last fill value.
  ioctlrbo(bitorder)
  ioctlrboget()
  ioctlwbo(bitorder)
  ioctlwboget()
sets/gets the bitorder for read/write bitstreams. 0 (default) means LSB
first, 1 MSB first. example: first byte in a stream is 0x71 (01000001). in
LSB mode a bit-wise read will give you 1,0,0,0,0,0,1,0, in MSB mode it is
0,1,0,0,0,0,0,1. a read of 2 bits however will give 01 in both cases.
returns the old bitorder.
  ioctlwbfill()
  ioctlwbfillget()
this is for the wsyncbyte method in output bitstream in output streams
(!modeseek). 0 means the byte will be filled up with zeroes, otherwise with
ones.
  ioctlrerr()
  ioctlrerrclr()
  ioctlwerr()
  ioctlwerrclr()
returns the last error in the read/write stream. 0 means no error. clr will
clear up the error afterwards, otherwise the error will be remembered.
  ioctlrbufset(len)
  ioctlwbufset(len)
installs a buffer of len. very small buffers are internal, bigger ones on
the heap. a len of 0 removes the buffer. returns 1 on success otherwise 0.
make sure the buffer is clean before you change it, otherwise the call will
fail. bigger buffers can fail due to a lack of memory.
the buffers are meant to reduce the number of system calls and speed up the
program.
this feature has not been tested extensively and I expect there are things
to be improved. (especially in connection with streams and blocking mode)
if something behaves odd, turn off the buffer and try again. this buffering
is so complex and I don't know what's the best way to do certain things.
  ioctlrbufgetlen()
  ioctlwbufgetlen()
returns the length of the buffer.
  ioctlrbufget()
  ioctlwbufget()
returns the number of bytes in the buffer.
  ioctlrmax()
  ioctlwmax()
returns the maximum number of bytes that can be read/written.
  ioctlrflush()
  ioctlrflushforce()
  ioctlwflush()
  ioctlwflushforce()
flushes the buffer if possible. if you use force everything's possible.
  ioctlrcancel()
  ioctlwcancel()
oops, ignore what's in the buffer. hands off! I have no idea if it works
satisfactory.
  ioctlrsetlog((void*)&file, xxx)
all reads will be logged into file.
  ioctlrshutdown()
  ioctlwshutdown()
  ioctlwshutdownforce()
shuts down the read/write streams. no more reads/writes will be permitted.
force is more radical.
  ioctltrunc()
  ioctltruncget()
if you enable trunc the file will be truncated where the file pointer
points when you close the file.
  ioctlblock()
  ioctlblockget()
if you enable block, calls to read will not return until at least one byte
is read or the end of the stream is reached. calls to write will not return
before all data is written.

Phew, I hope that's it.



      4. included classes
     ---------------------

Maybe you wondered why I never mentioned any open function. That's because
it's up to the class how to open a file. You might need a filename, if you
want a temp file you don't, there could be options, who says this is about
files anyway?


    1. nullfile: binfile <binfile.h>

You cannot open this file. it's the same as a null device, accessing it is
useless, however allowed.


    2. standard binfiles: sbinfile <binfstd.h>

Standard file as a binfile. No big deal. Open with sbinfile::open(name,
mode). If the open fails a negative value is returned otherwise 0. There
are 4 file modes (required) and 4 creation modes (optional):

  sbinfile::openis: open as input stream
  sbinfile::openos: open as output stream
  sbinfile::openro: open as read only file
  sbinfile::openrw: open as read/write file

  sbinfile::openex: open an existing file (default)
  sbinfile::opencr: create the file if it does not exist
  sbinfile::opentr: create/truncate the file
  sbinfile::opencn: create a new file


    3. archive binfiles: abinfile <binfarc.h>

binfile in binfile. With this you can make a binfile that refers to a
specified region of another binfile. Open with abinfile::open(file, ofs,
len);

    4. memory binfiles: mbinfile <binfmem.h>

binfile in memory. “virtual files”. If you have an image of a file in
memory you can use this to access it. You can also use mbinfile to accept
the output of something that writes to a binfile. Open with open(buf, len,
type) if you have a fixed buffer. Type can be either mbinfile::openro or
mbinfile::openrw. You can specify mbinfile::openfree to free the buffer on
close automatically. Open with opencs(buf, len, inc) for a variable buffer.
if the buffer is empty set buf to 0. When the file shall be appended to inc
specifies the number of bytes to add (to avoid many reallocs). When the
file is closed buf and len will contain the buffer pointer and the length.


    5. console binfiles <binfcon.h>

These binfiles are opened automatically. They are the corresponding
binfiles for the standard input and output. There are 4 types:

  consolebinfile: stdin/stdout
  iconsolebinfile: stdin
  oconsolebinfile: stdout
  econsolebinfile: stderr


    6. wave playback binfile

There are currently 3 such binfiles. One for Windows 95/NT, one for Linux
and one for DOS/SoundBlaster 16. Another one writes to a WAV file.
When you open the file you specify the output rate in Hz, the
stereo mode and the format of a sample. You may also have to specify the
buffer/block size, number of blocks and a file (and the estimated length of
the WAV, 0 is file if you don't know what to do) for the WAV. Mono (0) is
obvious and stereo (1) is interleaved as usual and left comes first.
Currently there are only two formats: unsigned 8 bit (0) and signed 16 bit
little endian (1). Open the stream and write to it. Writing will be blocked
and delayed until there is enough buffer to hold the data. If not enough
data is supplied, the sound will pause or repeat. On close the buffers will
be flushed.

  Windows: ntplaybinfile <binfplnt.h>
ntplaybinfile::open(rate, stereo, format, blksize, nblks);

  Linux: linuxplaybinfile <binfpllx.h>
linuxplaybinfile::open(rate, stereo, format);

  DOS/SB: sbplaybinfile <binfplsb.h>
sbplaybinfile::open(rate, stereo, format, bufsize);

  WAV: wavplaybinfile <binfplwv.h>
wavplaybinfile::open(file, rate, stereo, format, orglen);


    7. tcpbinfile <binftcp.h>

Internet the easy way. just tcpbinfile::open(addr, port), where addr is
either a string or an int, and you are connected. If you want to accept
connections you need a tcplistener. Open it with tcplistener::open(port,
num) to listen for num incoming connections on port port (or 0 for all).
Then tcpbinfile::open(listener) to accept a connection.
There are some special functions in tcpbinfile. Find out about them
yourself.


    8. httpbinfile <binfhttp.h>

To receive a file via HTTP. httpbinfile::open(url, proxy, off, len);
receive len bytes (0 for complete file) of url url from proxy proxy (0
without proxy) from byte position off. Note that not all servers support
partial files, ioctlrpos and ioctlrlen will contain the actual file
position and length (off+len).



      5. do it yourself
     -------------------

If you want to make your own binfile class you should overwrite at least
rawclose and one of rawread and rawwrite and supply at least one open
method.
The open method should call close() first or make sure an open instance
will not be opened again. then initialize its variables and the stream.
finally call openmode(mode, pos, len).
the rawclose() method calls closemode() and then closes the stream. if
rawclose should fail, do not call closemode().
rawread/write/peek(buf, len) read/write/peek the buffer from/to the stream
and return the number of bytes read/written/peeked.
rawseek(pos) seeks to a positision and returns the actual position.
rawioctl(code, buf, len) can handle some codes and must pass all others to
the rawioctl of the parent class.



      6. portability
     ----------------

I hope binfile is quite portable. make sure all the stuff in ptypes.h
is correct for your system. define BIGENDIAN if your machine is BIGENDIAN.
define UNIX for Unix systems, define LINUX for Linux, SUNOS4 for Sun
and ALPHA for Alpha machines. define NOUNISTD if you don't have the unix
functions open,read,close etc.



      7. comments
     -------------


sorry for the documentation, but who'll read it anyway?
binfile is not really finished. there are still problems with
blocking/nonblocking io, closing files, the buffer in certain cases.
i still haven't found a solution to special combinations of cases.
i'm not happy with ptypes.h (just why is this stuff not defined in c?)



      8. contact
     ------------

mailto:nbeisert@ph.tum.de
http://www.ph.tum.de/~nbeisert/binfile.html
