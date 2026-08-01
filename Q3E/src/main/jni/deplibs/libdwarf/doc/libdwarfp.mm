\." $Revision: 1.12 $
\." $Date: 2002/01/14 23:40:11 $
\."
\."
\." the following line may be removed if the ff ligature works on your machine
.lg 0
\." set up heading formats
.ds HF 3 3 3 3 3 2 2
.ds HP +2 +2 +1 +0 +0
.nr Hs 5
.nr Hb 5
\." Increment body point size
.S +2
\." ==============================================
\." Put current date in the following at each rev
.ds vE Rev 4.2, 16 May 2025 2.3.2 
\." ==============================================
\." ==============================================
.ds | |
.ds ~ ~
.ds ' '
.if t .ds Cw \&\f(CW
.if n .ds Cw \fB
.de Cf          \" Place every other arg in Cw font, beginning with first
.if \\n(.$=1 \&\*(Cw\\$1\fP
.if \\n(.$=2 \&\*(Cw\\$1\fP\\$2
.if \\n(.$=3 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP
.if \\n(.$=4 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4
.if \\n(.$=5 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP
.if \\n(.$=6 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP\\$6
.if \\n(.$=7 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP\\$6\*(Cw\\$7\fP
.if \\n(.$=8 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP\\$6\*(Cw\\$7\fP\\$8
.if \\n(.$=9 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP\\$6\*(Cw\\$7\fP\\$8\
*(Cw
..
.nr Cl 3
.SA 1
.TL
A Producer Library Interface to DWARF
.AF ""
.AU "David Anderson"
.PF "'\*(vE '- \\\\nP -''"
.AS 1
This document describes an interface to a library of functions
to create DWARF debugging information entries and DWARF line number
information. It does not make recommendations as to how the functions
described in this document should be implemented nor does it
suggest possible optimizations. 
.P
The document is oriented to creating DWARF version 2.
Support for creating DWARF3 and DWARF4
and DWARF5 is only partial: various features
since DWARF2 cannot be created.
.P
\*(vE 
.AE
.MT 4
.H 1 "INTRODUCTION"
This document describes an interface to \f(CWlibdwarf\fP, a
library of functions to provide creation of DWARF debugging information
records, DWARF line number information, DWARF address range and
pubnames information, weak names information, and DWARF frame description 
information.


.H 2 "Copyright"
Copyright 1993-2006 Silicon Graphics, Inc.

Copyright 2007-2021 David Anderson.

Permission is hereby granted to copy or republish or use any
or all of this document without restriction except that when
publishing more than a small amount of the document please
acknowledge Silicon Graphics, Inc and David Anderson.

This document is distributed in the hope that it would be
useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

.H 2 "Purpose and Scope"
The purpose of this document is to propose a library of
functions to create DWARF debugging information.  Reading
(consuming) of such records is discussed in a separate
document.

The functions in this document have mostly been implemented
at Silicon Graphics and used by the SGI code generator to
provide DWARF2 debugging information in the 1990's.
Some functions (and support
for some extensions) were provided by Sun Microsystems.

Example code showing one use of the functionality
may be found in the dwarfgen 
\f(CWdwarfgen\fP
and
\f(CWsimpleexample\fP
application
(provided in the source distribution along with libdwarf).

.P
The focus of this document is the functional interface,
and as such, implementation and optimization issues are
intentionally ignored.

.P
Error handling, error codes, and certain 
\f(CWLibdwarf\fP codes are discussed
in the "\fIA Consumer Library Interface to DWARF\fP",
which should 
be read before reading this document.
.P
Before December 2018 
very few functions in the Producer Library follow the error-returns
as defined in "\fIA Consumer Library Interface to DWARF\fP".
.P
As of December 2018 every Producer Library call has
a version that supports that Consumer Library Interface
and returns DW_DLV_OK or DW_DLV_ERROR (the Producer Library
has no use of DW_DLV_NO_ENTRY).
The table of contents of this document lists the latest
version of each function. However, all the earlier
documentation is present here immediately following
the documentation of the latest, and preferred, interface. 
All the earlier interfaces are supported in the library.
.P
Early interfaces (before December 2018)
The general style of functions here 
in the producer library is rather C-traditional
with various types as return values (quite different
from the consumer library interfaces).
The style
generally follows the style of the original DWARF1 reader
proposed as an interface to DWARF.
When the style of the reader interfaces was changed (1994) in the
dwarf reader ( See the "Document History"
section of "A Consumer Library Interface to DWARF")
the interfaces here were not changed as it seemed like
too much of a change for the two applications then using
the interface!  So this interface remains in the traditional C style
of returning various data types with various (somewhat inconsistent)
means of indicating failure.
.P
December 2018 and later function interfaces
all return either DW_DLV_OK or DW_DLV_ERROR
in a simple int.
.P
The error handling code in the library may either
return a value or abort. 
The library user can provide a function that the producer code
will call on errors (which would allow callers avoid testing
for error returns if the user function exits or aborts).
See the  \f(CWdwarf_producer_init()\fP
description below for more details.

.H 2 "Document History"
This document originally prominently referenced
"UNIX International Programming Languages Special Interest Group " 
(PLSIG).
Both  UNIX International and the
affiliated  Programming Languages Special Interest Group
are defunct
(UNIX is a registered trademark of UNIX System Laboratories, Inc.
in the United States and other countries).
Nothing except the general interface style is actually
related to anything shown to the PLSIG
(this document was open sourced with libdwarf in the mid 1990's).
.P
See "http://www.dwarfstd.org" for information on current
DWARF standards and committee activities.

.H 2 "Definitions"
DWARF debugging information entries (DIEs) are the segments of information 
placed in the \f(CW.debug_info\fP  and related
sections by compilers, assemblers, and linkage 
editors that, in conjunction with line number entries, are necessary for 
symbolic source-level debugging.  
Refer to the document 
"\fIDWARF Debugging Information Format\fP" from UI PLSIG for a more complete 
description of these entries.

.P
This document adopts all the terms and definitions in
"\fIDWARF Debugging Information Format\fP" version 2.
and the "\fIA Consumer Library Interface to DWARF\fP".

.P
In addition, this document refers to Elf, the ATT/USL System V
Release 4 object format.
This is because the library was first developed for that object
format.
Hopefully the functions defined here can easily be
applied to other object formats.

.H 2 "Overview"
The remaining sections of this document describe a proposed producer 
(compiler or assembler) interface to \fILibdwarf\fP, first by describing 
the purpose of additional types defined by the interface, followed by 
descriptions of the available operations.  
This document assumes you 
are thoroughly familiar with the information contained in the 
\fIDWARF 
Debugging Information Format\fP document, and 
"\fIA Consumer Library Interface to DWARF\fP".

.P
The interface necessarily knows a little bit about the object format
(which is assumed to be Elf).  We make an attempt to make this knowledge 
as limited as possible.  For example, \fILibdwarf\fP does not do the 
writing of object data to the disk.  The producer program does that.

.H 2 "Revision History"
.VL 15
.LI "March 1993"
Work on dwarf2 sgi producer draft begins
.LI "March 1999"
Adding a function to allow any number of trips
through the dwarf_get_section_bytes_a() call.
.LI "April 10 1999"
Added support for assembler text output of dwarf
(as when the output must pass through an assembler).
Revamped internals for better performance and
simpler provision for differences in ABI.
.LI "Sep 1, 1999"
Added support for little- and cross- endian
debug info creation.
.LI "May 7  2007"
This library interface now cleans up, deallocating
all memory it uses (the application simply calls
dwarf_producer_finish(dbg)).
.LI "September 20  2010"
Now documents the marker feature of DIE creation.
.LI "May 01  2014"
The dwarf_producer_init() code has a new interface
and DWARF is configured at run time by its arguments.
The producer code used to be configured at configure
time, but the configure time producer configure options
are no longer used. 
The configuration was unnecessarily complicated:
the run-time configuration is simpler to understand.
.LI "September 10, 2016"
Beginning the process of creating new interfaces
so that checking for error is consistent across all
calls (as is done in the consumer library).
The old interfaces are kept and supported so
we have binary and source compatibility with
old code.
.LI "December 01, 2018"
All function interfaces now have a version
that returns only DW_DLV_OK or DW_DLV_ERROR
and pointer and other values are returned
through pointer arguments.
For example, dwarf_add_frame_info_c()
is the December 2018 version, while
dwarf_add_frame_info(), 
dwarf_add_frame_info_b()
are earlier versions.
.LI "July 14, 2020"
To enable testing of reading the DWARF5 section .debug_sup
the new function dwarf_add_debug_sup() is added.
dwarfgen can call this function, though dwarfgen
presently only fills out a bogus .debug_sup
section to enable simple testing.
.LI "January 25, 2021"
dwarf_add_AT_block_a() now also supports
the DWARF5 form DW_FORM_exprloc.
.LI "June 2021"
Removing the obsolete functions that return Dwarf_Unsigned
etc and required ugly casting
to check success/fail. The ones returning int
DW_DLV_OK etc are the only ones that should be used.
The library is now in its own file (libdwarfp.a
or libdwarfp.so) and the source
in its own directory (src/lib/libdwarfp).
libdwarfp is only built if a build of dwarfgen
is requested.  Meaning with standard builds
this library is not provided.
It only creates DWARF2 with any completeness,
so it's not clear how it could possibly be
generally useful.

.LE

.H 1 "Type Definitions"

.H 2 "General Description"
The \fIlibdwarf.h\fP 
header file contains typedefs and preprocessor 
definitions of types and symbolic names 
used to reference objects of and types
used by 
\fIlibdwarf\fP
and some declarations
needed by
\fIlibdwarfp\fP
The \fIlibdwarfp.h\fP 
header file defines producer functions and
type specifically used by
\fIlibdwarfp\fP.
The types defined by typedefs contained in 
\fI libdwarf.h\fP 
and
\fI libdwarfp.h\fP 
all use the convention of adding \fI Dwarf_ \fP 
as a prefix to
indicate that they refer to objects used by Libdwarf.  
The prefix \fI Dwarf_P_\fP is used for objects 
referenced by the \fI Libdwarf\fP 
Producer when there are similar but distinct 
objects used by the Consumer.

.H 2 "Namespace issues"
Application programs should avoid creating names
beginning with 
\f(CWDwarf_\fP
\f(CWdwarf_\fP
or 
\f(CWDW_\fP
as these are reserved to dwarf and libdwarf.

.H 1 "libdwarf and Elf and relocations"
Much of the description below presumes that 
Elf is the object
format in use.
The library is probably usable with other object formats
that allow arbitrary sections to be created.
The library does not write anything to disk.
Instead it provides access so that callers
can do that in whatever object format
is appropriate.

.H 2 "binary or assembler output"
With 
\f(CWDW_DLC_STREAM_RELOCATIONS\fP 
(see below)
it is assumed that the calling app will simply
write the streams and relocations directly into
an Elf file, without going through an assembler.

With 
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP 
the calling app must either 
A) generate binary relocation streams and write
the generated debug information streams and
the relocation streams direct to an elf file
or
B) generate assembler output text for an assembler
to read and produce an object file. 

With case B) the libdwarf-calling application must
use the relocation information to change
points of each binary stream into references to 
symbolic names.  
It is necessary for the assembler to be
willing to accept and generate relocations
for references from arbitrary byte boundaries.
For example:
.sp
.nf
.in +4
 .data 0a0bcc    #producing 3 bytes of data.
 .word mylabel   #producing a reference
 .word endlabel - startlabel #producing absolute length
.in -4
.fi
.sp




.H 2 "libdwarf relationship to Elf"
When the documentation below refers to 'an elf section number'
it is really only dependent on getting (via the callback
function passed by the caller of
\f(CWdwarf_producer_init()\fP.
a sequence of integers back (with 1 as the lowest).

When the documentation below refers to 'an Elf symbol index'
it is really dependent on 
Elf symbol numbers
only if
\f(CWDW_DLC_STREAM_RELOCATIONS\fP 
are being generated (see below).
With 
\f(CWDW_DLC_STREAM_RELOCATIONS\fP 
the library is generating Elf relocations
and the section numbers in binary form so
the section numbers and symbol indices must really 
be Elf (or elf-like) numbers.

With
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP 
the values passed as symbol indexes can be any
integer set or even pointer set.
All that libdwarf assumes is that where values
are unique they get unique values.
Libdwarf does not generate any kind of symbol table
from the numbers and does not check their
uniqueness or lack thereof.

.H 2 "libdwarf and relocations"
With
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP 
libdwarf creates binary streams of debug information
and arrays of relocation information describing
the necessary relocation.
The Elf section numbers and symbol numbers appear
nowhere in the binary streams. Such appear
only in the relocation information and the passed-back
information from calls requesting the relocation information.
As a consequence, the 'symbol indices' can be
any pointer or integer value as the caller must
arrange that the output deal with relocations.

With 
\f(CWDW_DLC_STREAM_RELOCATIONS\fP 
all the relocations are directly created by libdwarf
as binary streams (libdwarf only creates the streams
in memory,
it does not write them to disk).

.H 2 "symbols, addresses, and offsets"
The following applies to calls that
pass in symbol indices, addresses, and offsets, such
as
\f(CWdwarf_add_AT_targ_address_c() \fP 
\f(CWdwarf_add_arange_c()\fP 
and
\f(CWdwarf_add_frame_fde_c()\fP.

With 
\f(CWDW_DLC_STREAM_RELOCATIONS\fP 
a passed in address is one of:
a) a section offset and the (non-global) symbol index of
a section symbol.
b) A symbol index (global symbol) and a zero offset.

With \f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP
the same approach can be used, or, instead,
a passed in address may be
c) a symbol handle and an offset.
In this case, since it is up to the calling app to
generate binary relocations (if appropriate)
or to turn the binary stream into 
a text stream (for input to an assembler, if appropriate)
the application has complete control of the interpretation
of the symbol handles.

.H 1 "Memory Management"

Several of the functions that comprise the \fILibdwarf\fP 
producer interface dynamically allocate values and some
return pointers to those spaces.
The dynamically allocated spaces 
can not be reclaimed  (and must
not be freed) except that
all such libdwarf-allocated memory
is freed by 
\f(CWdwarf_producer_finish_a(dbg)\fP  .

All data for a particular \f(CWDwarf_P_Debug\fP descriptor
is separate from the data for any other 
\f(CWDwarf_P_Debug\fP descriptor in use in the library-calling
application.

.H 2 "Read Only Properties"
The read-only properties
specified in the
consumer interface document do not
generally apply to the functions
described here.

.H 2 "Storage Deallocation"
Calling \f(CWdwarf_producer_finish_a(dbg)\fP frees all the space, and 
invalidates all pointers returned from \f(CWLibdwarf\fP functions on 
or descended from \f(CWdbg\fP).

.H 2 "Error Handling"
In general any error detected by the producer
should be considered fatal. 
That is, it is impossible to produce correct
output so producing anything seems questionable.
.P
The original producer interfaces tended to return
a pointer or a large integer as a result and
required the caller to cast that value to determine
if it was actually a -1 meaning there was an error.
.P
Beginning in September 2016 additional interfaces
are being added to eliminate the necessity for callers
to do this ugly casting of results.
In December 2018 that process has reached completion.
The revised functions return
\f(CWDW_DLV_OK\fP,
or
\f(CWDW_DLV_ERROR\fP.
(which are small signed integers) and will
have an additional pointer argument that will provide
the value that used to be the return value.
This will make the interfaces type-safe.
.P
The function
\f(CWdwarf_get_section_bytes_a()\fP
can also return
\f(CWDW_DLV_NO_ENTRY\fP.
.P

.H 1 "Functional Interface"
This section describes the functions available in the \fILibdwarf\fP
library.  Each function description includes its definition, followed 
by a paragraph describing the function's operation.
.P
The following sections describe these functions.
.P
The functions may be categorized into groups: 
\fIinitialization and termination operations\fP,
\fIdebugging information entry creation\fP,
\fIElf section callback function\fP,
\fIattribute creation\fP,
\fIexpression creation\fP, 
\fIline number creation\fP, 
\fIfast-access (aranges) creation\fP, 
\fIfast-access (pubnames) creation\fP, 
\fIfast-access (weak names) creation\fP,
\fImacro information creation\fP, 
\fIlow level (.debug_frame) creation\fP, 
and
\fIlocation list (.debug_loc) creation\fP. 
.P
.H 2 "Initialization and Termination Operations"
These functions setup \f(CWLibdwarf\fP to accumulate debugging information
for an object, usually a compilation-unit, provided by the producer.
The actual addition of information is done by functions in the other
sections of this document.  Once all the information has been added,
functions from this section are used to transform the information to
appropriate byte streams, and help to write out the byte streams to
disk.

Typically then, a producer application 
would create a \f(CWDwarf_P_Debug\fP 
descriptor to gather debugging information for a particular
compilation-unit using \f(CWdwarf_producer_init()\fP.  

The producer application would 
use this \f(CWDwarf_P_Debug\fP descriptor to accumulate debugging 
information for this object using functions from other sections of 
this document.  
Once all the information had been added, it would 
call \f(CWdwarf_transform_to_disk_form_a()\fP to convert the accumulated 
information into byte streams in accordance with the \f(CWDWARF\fP 
standard.  
The application would then repeatedly call 
\f(CWdwarf_get_section_bytes_a()\fP 
for each of the \f(CW.debug_*\fP created.  
This gives the producer 
information about the data bytes to be written to disk.  
At this point, 
the producer would release all resource used by \f(CWLibdwarf\fP for 
this object by calling \f(CWdwarf_producer_finish_a()\fP.

It is also possible to create assembler-input character streams
from the byte streams created by this library.
This feature requires slightly different interfaces than
direct binary output.
The details are mentioned in the text.

.H 3 "dwarf_producer_init()"

.DS
\f(CWint dwarf_producer_init(
        Dwarf_Unsigned flags,
        Dwarf_Callback_Func func,
        Dwarf_Handler errhand,
        Dwarf_Ptr errarg,
        void *    user_data      
        const char *isa_name,
        const char *dwarf_version,
        const char *extra, 
        Dwarf_P_Debug *dbg_returned,
        Dwarf_Error *error) \fP
.DE
.P
The function \f(CWdwarf_producer_init() \fP returns a new 
\f(CWDwarf_P_Debug\fP descriptor that can be used to add
\f(CWDwarf\fP 
information to the object.  
On success it returns \f(CWDW_DLV_OK\fP.  
On error it returns \f(CWDW_DLV_ERROR\fP.  
\f(CWflags\fP determine whether the target object is 64-bit or 32-bit.  
\f(CWfunc\fP
is a pointer to a function called-back from \f(CWLibdwarf\fP 
whenever \f(CWLibdwarf\fP needs to create a new object section (as it will 
for each .debug_* section and related relocation section).  

.P
The \f(CWflags\fP
values (to be OR'd together in the flags field
in the calling code) are as follows:
.in +4
\f(CWDW_DLC_WRITE\fP 
is required.
The values
\f(CWDW_DLC_READ\fP  
\f(CWDW_DLC_RDWR\fP
are not supported by the producer and must not be passed.

The flag bit
\f(CWDW_DLC_POINTER64\fP
(or
\f(CWDW_DLC_SIZE_64\fP)
Indicates the target has a 64 bit (8 byte) address size.
The flag bit
\f(CWDW_DLC_POINTER32\fP
(or
\f(CWDW_DLC_SIZE_32\fP)
Indicates the target has a 32 bit (4 byte) address size.
If none of these pointer sizes is passed in 
\f(CWDW_DLC_POINTER32\fP 
is assumed.

The flag bit
\f(CWDW_DLC_OFFSET32\fP
indicates that 32bit offsets should be used in the generated DWARF.
The flag bit
\f(CWDW_DLC_OFFSET64\fP
\f(CWDW_DLC_OFFSET_SIZE_64\fP
indicates that 64bit offsets should be used in the generated DWARF.

The flag bit
\f(CWDW_DLC_IRIX_OFFSET64\fP
indicates that the generated DWARF should use the
early (pre DWARF3) IRIX method of generating 64 bit offsets.
In this case \f(CWDW_DLC_POINTER64\fP should also be passed in,
and the \f(CWisa_name\fP
passed in (see below) should be "irix".



If
\f(CWDW_DLC_TARGET_BIGENDIAN\fP
or
\f(CWDW_DLC_TARGET_LITTLEENDIAN\fP
is not ORed into \f(CWflags\fP
then 
endianness the same as the host is assumed.
If both 
\f(CWDW_DLC_TARGET_LITTLEENDIAN\fP
and
\f(CWDW_DLC_TARGET_BIGENDIAN\fP
are OR-d in it is an error.



Either one of two output forms is specifiable:
\f(CWDW_DLC_STREAM_RELOCATIONS\fP 
or
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP .

The default is
\f(CWDW_DLC_STREAM_RELOCATIONS\fP . 
The
\f(CWDW_DLC_STREAM_RELOCATIONS\fP
are relocations in a binary stream (as used
in a MIPS/IRIX Elf object).

The
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP
are the same relocations but expressed in an
array of structures defined by libdwarf,
which the caller of the relevant function
(see below) must deal with appropriately.
This method of expressing relocations allows 
the producer-application to easily produce
assembler text output of debugging information.

When
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP
is ORed into \f(CWflags\fP
then relocations are returned not as streams
but through an array of structures.

.in -4

.P
The function 
\f(CWfunc\fP 
must be provided by the user of this library.
Its prototype is:
.DS
\f(CWtypedef int (*Dwarf_Callback_Func)(
    char* name,
    int                 size,
    Dwarf_Unsigned      type,
    Dwarf_Unsigned      flags,
    Dwarf_Unsigned      link,
    Dwarf_Unsigned      info,
    Dwarf_Unsigned*     sect_name_index,
    void *              user_data,
    int*                error) \fP
.DE
For each section in the object file that \f(CWlibdwarf\fP
needs to create, it calls this function once (calling it
from \f(CWdwarf_transform_to_disk_form()\fP), passing in
the section \f(CWname\fP, the section \f(CWtype\fP,
the section \f(CWflags\fP, the \f(CWlink\fP field, and
the \f(CWinfo\fP field.  
For an Elf object file these values
should be appropriate Elf section header values.
For example, for relocation callbacks, the \f(CWlink\fP
field is supposed to be set (by the app) to the index
of the symtab section (the link field passed through the
callback must be ignored by the app).
And, for relocation callbacks, the \f(CWinfo\fP field
is passed as the elf section number of the section
the relocations apply to.
.P
The  \f(CWsect_name_index\fP field is a field you use
to pass a symbol index back to libdwarf.
In Elf, each section gets an elf symbol table entry
so that relocations have an address to refer to
(relocations rely on addresses in the Elf symbol table).
You will create the Elf symbol table, so you have to tell
libdwarf the index to put into relocation records for the
section newly defined here.
.P
On success
the user function should return the Elf section number of the
newly created Elf section.
.P
On success, the function should also set the integer
pointed to by \f(CWsect_name_index\fP to the
Elf symbol number assigned in the Elf symbol table of the
new Elf section.
This symbol number is needed with relocations
dependent on the relocation of this new section.
.P
Use the 
\f(CWdwarf_producer_init_c()\fP
interface instead of this interface.
.P
For example, the \f(CW.debug_line\fP section's third
data element (in a compilation unit) is the offset from the
beginning of the \f(CW.debug_info\fP section of the compilation
unit entry for this \f(CW.debug_line\fP set.
The relocation entry in \f(CW.rel.debug_line\fP
for this offset
must have the relocation symbol index of the 
symbol \f(CW.debug_info\fP  returned
by the callback of that section-creation through 
the pointer \f(CWsect_name_index\fP.
.P
On failure, the function should return -1 and set the \f(CWerror\fP
integer to an error code.
.P
Nothing in libdwarf actually depends on the section index
returned being a real Elf section.
The Elf section is simply useful for generating relocation
records.
Similarly, the Elf symbol table index returned through 
the \f(CWsect_name_index\fP must be an index
that can be used in relocations against this section.
The application will probably want to note the
values passed to this function in some form, even if
no Elf file is being produced.

.P
\f(CWerrhand\fP 
is a pointer to a function that will be used as
a default fall-back function for handling errors detected 
by \f(CWLibdwarf\fP.  

.P
\f(CWerrarg\fP is the default error argument used 
by the function pointed to by \f(CWerrhand\fP.
.P
For historical reasons the error handling is complicated
and the following three paragraphs describe the three
possible scenarios when a producer function detects an error.
In all cases a short error message is printed on
stdout if the error number
is negative (as all such should be, see libdwarf.h).
Then further action is taken as follows.
.P
First,
if the  Dwarf_Error argument to any specific producer function
(see the functions documented below)  is non-null
the \f(CWerrhand\fP argument here is ignored in that call and
the specific producer function sets the Dwarf_Error and returns
some specific value (for dwarf_producer_init it is DW_DLV_OK
as mentioned just above) indicating there is an error.
.P
Second,
if the  Dwarf_Error argument to any specific producer function 
(see the functions documented below)  is NULL and the
\f(CWerrarg\fP
to \f(CWdwarf_producer_init()\fP is non-NULL
then on an error in the producer code the Dwarf_Handler function is called
and if that called function returns the producer code returns
a specific value (for dwarf_producer_init it is DW_DLV_OK
as mentioned just above) indicating there is an error.
.P
Third,
if the  Dwarf_Error argument to any specific producer function
(see the functions documented below)  is NULL and the
\f(CWerrarg\fP
to \f(CWdwarf_producer_init()\fP is NULL
then on an error \f(CWabort()\fP is called.

.P
The \f(CWuser_data\fP argument is not examined by libdwarf.
It is passed to user code in all 
calls by libdwarf to  the \f(CWDwarf_Callback_Func()\fP
function and 
may be used by consumer code for the consumer's own purposes.
Typical uses might be to pass in a pointer to some user
data structure or to pass an integer that somehow
is useful to the libdwarf-using code.

.P
The \f(CWisa_name\fP argument 
must be non-null and contain one of the
strings defined in the isa_relocs array
in pro_init.c: "irix","mips","x86",
"x86_64","arm","arm64","ppc","ppc64",
"sparc". 
The names are not strictly ISA
names (nor ABI names) but a hopefully-meaningful
mixing of the concepts of ISA and ABI.
The intent is mainly to 
define relocation codes applicable to DW_DLC_STREAM_RELOCATIONS.
New \f(CWisa_name\fP values will be provided as users
request. In the "irix" case a special relocation is defined
so a special CIE reference field can be created (if and
only if the augmentation
string is "z").
.P
The \f(CWdwarf_version\fP argument 
should be one of 
"V2",
"V3",
"V4",
"V5"
to indicate which DWARF version is the overall format
to be emitted.  Individual section version numbers will obey
the standard for that overall DWARF version.
.P
The \f(CWextra\fP argument 
is supports a comma-separated
list of options.
Passing in a null pointer or an empty string
is acceptable if no such options are needed 
or used.  
All-lowercase option names are reserved to
the libdwarf implementation itself (specific implementations
may want to use a leading upper-case letter for
additional options).
.P
The available options are
.DS
"default_is_stmt",
"address_size",
"minimum_instruction_length",
"maximum_operations_per_instruction",
"opcode_base",
"line_base",
"line_range",
"linetable_version",
"segment_selector_size",
and
"segment_size".
.DE
.P
For example, to set the line-table generation
default value of is_stmt to 0
pass in 
.DS
"default_is_stmt=0".
.DE
To also set the minimum_instruction_length
used in calculating line table address-advance
values to one one would pass 
in 
.DS
"default_is_stmt=0,minimum_instruction_length=1".
.DE
It's appropriate to add 
.DS
"opcode_base=13"
.DE
for
DWARF3 through DWARF5.
All these default to something, but the something
depends on environment what macro names 
are set by the environment or a just constants
which makes it difficult to alter these values.
See pro_line.h for the use of line-table related
constants (which will vary depending on the target ISA
and ABI and compilers).

.P
The \f(CWerror\fP argument 
is set through the pointer to return specific error  
if \f(CWerror\fP is non-null and
and there is an error.  The error details
will be passed back through this pointer argument.

.H 3 "dwarf_pro_set_default_string_form()"

.DS
\f(CWint dwarf_pro_set_default_string_form(
    Dwarf_P_Debug *dbg,
    int            desired_form,
    Dwarf_Error *error) \fP
.DE
.P
The function 
\f(CWdwarf_pro_set_default_string_form()\fP 
sets the 
\f(CWDwarf_P_Debug\fP descriptor to favor one of
the two allowed values:
\f(CWDW_FORM_string\fP
(the default)
or
\f(CWDW_FORM_strp\fP.
.P
When
\f(CWDW_FORM_strp\fP
is selected very short names will still
use form
\f(CWDW_FORM_string\fP .
.P
The function should be called immediately after a successful call
to 
\f(CWdwarf_producer_init()\fP.
.P
Strings for 
\f(CWDW_FORM_strp\fP
are not duplicated in the 
\f(CW.debug_str\fP
section: each unique string
appears exactly once.
.P
On success it returns \f(CWDW_DLV_OK\fP.
On error it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_transform_to_disk_form_a()"
.DS
\f(CWint dwarf_transform_to_disk_form_a(
        Dwarf_P_Debug dbg,
        Dwarf_Signed *chunk_count_out,
        Dwarf_Error* error)\fP
.DE
The function
\f(CWdwarf_transform_to_disk_form_a()\fP 
is new in September 2016. 
It produces the same result as
\f(CWdwarf_transform_to_disk_form()\fP 
but returns the count through the new
pointer argument
\f(CWchunk_count_out\fP . 
.P
On success it returns
\f(CWDW_DLV_OK\fP
and sets
\f(CWchunk_count_out\fP 
to
the number of chunks of section data to be
accessed by
\f(CWdwarf_get_section_bytes_a()\fP .
.P
It
turns the DIE and other information specified
for this \f(CWDwarf_P_Debug\fP into a stream of
bytes for each section being produced.
These byte streams can be retrieved from
the \f(CWDwarf_P_Debug\fP by calls to
\f(CWdwarf_get_section_bytes_a()\fP (see below).
.P
In case of error
\f(CWdwarf_transform_to_disk_form_a()\fP 
returns
\f(CWDW_DLV_ERROR\fP.
.P
The number of chunks
is used to access data 
by
\f(CWdwarf_get_section_bytes_a()\fP
(see below) and the section data
provided your code will insert
into an object file or the like.
Each section of the resulting object is typically
many small chunks. 
Each chunk has a section index
and a length as well as a pointer to a block of data
(see
\f(CWdwarf_get_section_bytes_a()\fP
).
.P
For each unique section being produced 
\f(CWdwarf_transform_to_disk_form_a()\fP 
calls the
\f(CWDwarf_Callback_Func\fP exactly once. 
The callback provides the connection
between Elf sections (which we presume
is the object format to be emitted) and
the 
\f(CWlibdwarf()\fP 
internal section numbering.
.P
For \f(CWDW_DLC_STREAM_RELOCATIONS\fP
a call to 
\f(CWDwarf_Callback_Func\fP is made
by libdwarf for each relocation section.
Calls to \f(CWdwarf_get_section_bytes_a()\fP (see below).
allow the 
\f(CWdwarf_transform_to_disk_form_a()\fP  caller
to get byte streams and write them to
an object file as desired, just as with
the other sections of the object being created.
.P
For \f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP
the user code should use
\f(CWdwarf_get_relocation_info_count()\fP
and
\f(CWdwarf_get_relocation_info()\fP
to retrieve the relocation info
generated by 
\f(CWdwarf_transform_to_disk_form()\fP
and do something with it.

.P
On failure it returns 
\f(CWDW_DLV_ERROR\fP
and returns an error pointer through
\f(CW*error\fP .


.H 4 "dwarf_transform_to_disk_form()"
.DS
\f(CWDwarf_Signed dwarf_transform_to_disk_form(
        Dwarf_P_Debug dbg,
        Dwarf_Error* error)\fP
.DE
The function
\f(CWdwarf_transform_to_disk_form()\fP 
is the original call to generate output
and a better interface is used by
\f(CWdwarf_transform_to_disk_form_a()\fP 
though both do the same work and have the
same meaning.


.H 3 "dwarf_get_section_bytes_a()"

.DS
\f(CWint dwarf_get_section_bytes_a(
        Dwarf_P_Debug dbg,
        Dwarf_Signed dwarf_section,
        Dwarf_Signed *elf_section_index, 
        Dwarf_Unsigned *length,
        Dwarf_Ptr      *section_bytes,
        Dwarf_Error* error)\fP
.DE
The function \f(CWdwarf_get_section_bytes_a() \fP 
must be called repetitively, 
with the index 
\f(CWdwarf_section\fP starting at 0 and continuing for the 
number of sections 
returned by 
\f(CWdwarf_transform_to_disk_form_a() \fP.
.P
It returns 
\f(CWDW_DLV_NO_ENTRY\fP
to indicate that there are no more sections of 
\f(CWDwarf\fP information.  
Normally one would index through using the sectioncount
from dwarf_transform_to_disk_form_a() so
\f(CWDW_DLV_NO_ENTRY\fP
would never be seen.

For each successful return (return value
\f(CWDW_DLV_OK\fP),
\f(CW*section_bytes\fP
points to \f(CW*length\fP bytes of data that are normally
added to the output 
object in \f(CWElf\fP section \f(CW*elf_section\fP by the producer application.
It is illegal to call these in
any order other than 0 through N-1 where
N is the number of dwarf sections
returned by \f(CWdwarf_transform_to_disk_form_a() \fP.
The elf section number is returned through the pointer
\f(CWelf_section_index\fP.

The \f(CWdwarf_section\fP
number is ignored: the data is returned as if the
caller passed in the correct dwarf_section numbers in the
required sequence.
.P
In case of an error, 
\f(CWDW_DLV_ERROR\fP
is returned and 
the \f(CWerror\fP argument is set to indicate the error.
.P
There is no requirement that the section bytes actually 
be written to an elf file.
For example, consider the .debug_info section and its
relocation section (the call back function would resulted in
assigning 'section' numbers and the link field to tie these
together (.rel.debug_info would have a link to .debug_info).
One could examine the relocations, split the .debug_info
data at relocation boundaries, emit byte streams (in hex)
as assembler output, and at each relocation point,
emit an assembler directive with a symbol name for the assembler.
Examining the relocations is awkward though. 
It is much better to use \f(CWdwarf_get_section_relocation_info() \fP
.P

The memory space of the section byte stream is freed
by the \f(CWdwarf_producer_finish_a() \fP call
(or would be if the \f(CWdwarf_producer_finish_a() \fP
was actually correct), along
with all the other space in use with that Dwarf_P_Debug.


.H 3 "dwarf_get_relocation_info_count()"
.DS
\f(CWint dwarf_get_relocation_info_count(
        Dwarf_P_Debug dbg,
        Dwarf_Unsigned *count_of_relocation_sections ,
	int            *drd_buffer_version,
        Dwarf_Error* error)\fP
.DE
The function \f(CWdwarf_get_relocation_info() \fP
returns, through  the pointer \f(CWcount_of_relocation_sections\fP, the
number of times that \f(CWdwarf_get_relocation_info() \fP
should be called.

The function \f(CWdwarf_get_relocation_info() \fP
returns DW_DLV_OK if the call was successful (the 
\f(CWcount_of_relocation_sections\fP is therefore meaningful,
though \f(CWcount_of_relocation_sections\fP
could be zero).

\f(CW*drd_buffer_version\fP
is the value 2.
If the structure pointed to by
the \f(CW*reldata_buffer\fP
changes this number will change.
The application should verify that the number is
the version it understands (that it matches
the value of DWARF_DRD_BUFFER_VERSION (from libdwarf.h)).
The value 1 version was never used in production 
MIPS libdwarf (version 1 did exist in source).

It returns DW_DLV_NO_ENTRY if 
\f(CWcount_of_relocation_sections\fP is not meaningful
because \f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP was not
passed to the
\f(CWdwarf_producer_init_c()\fP
\f(CWdwarf_producer_init_b()\fP or 
\f(CWdwarf_producer_init()\fP  call
(whichever one was used).

It returns DW_DLV_ERROR if there was an error,
in which case
\f(CWcount_of_relocation_sections\fP is not meaningful.

.H 3 "dwarf_get_relocation_info()"
.DS
\f(CWint dwarf_get_relocation_info(
        Dwarf_P_Debug dbg,
        Dwarf_Signed *elf_section_index, 
	Dwarf_Signed *elf_section_index_link, 
        Dwarf_Unsigned *relocation_buffer_count,
	Dwarf_Relocation_Data *reldata_buffer,
        Dwarf_Error* error)\fP
.DE

The function \f(CWdwarf_get_relocation_info() \fP 
should normally be called repetitively, 
for the number of relocation sections that 
\f(CWdwarf_get_relocation_info_count() \fP
indicated exist.

It returns \f(CWDW_DLV_OK\fP to indicate that 
valid values are returned through the pointer arguments.
The \f(CWerror\fP argument is not set.

It returns DW_DLV_NO_ENTRY if there are no entries
(the count of relocation arrays is zero.).
The \f(CWerror\fP argument is not set.

It returns \f(CWDW_DLV_ERROR\fP if there is an error.
Calling \f(CWdwarf_get_relocation_info() \fP
more than the number of times indicated by
\f(CWdwarf_get_relocation_info_count() \fP
(without an intervening call to
\f(CWdwarf_reset_section_bytes() \fP )
results in a return of \f(CWDW_DLV_ERROR\fP once past
the valid count.
The \f(CWerror\fP argument is set to indicate the error.

Now consider the returned-through-pointer values for
\f(CWDW_DLV_OK\fP .

\f(CW*elf_section_index\fP
is the 'elf section index' of the section implied by
this group of relocations.


\f(CW*elf_section_index_link\fP
is the section index of the section that these
relocations apply to.

\f(CW*relocation_buffer_count\fP
is the number of array entries of relocation information
in the array pointed to by
\f(CW*reldata_buffer\fP .


\f(CW*reldata_buffer\fP
points to an array of 'struct Dwarf_Relocation_Data_s'
structures.

The version 2 array information is as follows:

.nf
enum Dwarf_Rel_Type {dwarf_drt_none,
                dwarf_drt_data_reloc,
                dwarf_drt_segment_rel,
                dwarf_drt_first_of_length_pair,
                dwarf_drt_second_of_length_pair
};
typedef struct Dwarf_Relocation_Data_s  * Dwarf_Relocation_Data;
struct Dwarf_Relocation_Data_s {
        unsigned char        drd_type;   /* contains Dwarf_Rel_Type */
	unsigned char        drd_length; /* typically 4 or 8 */
        Dwarf_Unsigned       drd_offset; /* where the data to reloc is */
        Dwarf_Unsigned       drd_symbol_index;
};

.fi

The \f(CWDwarf_Rel_Type\fP enum is encoded (via casts if necessary)
into the single unsigned char \f(CWdrd_type\fP field to control
the space used for this information (keep the space to 1 byte).

The unsigned char \f(CWdrd_length\fP field
holds the size in bytes of the field to be relocated.
So for elf32 object formats with 32 bit apps, \f(CWdrd_length\fP
will be 4.  For objects with MIPS -64 contents,
\f(CWdrd_length\fP will be 8.  
For some dwarf 64 bit environments, such as ia64, \f(CWdrd_length\fP
is 4 for some relocations (file offsets, for example) 
and 8 for others (run time
addresses, for example).

If \f(CWdrd_type\fP is \f(CWdwarf_drt_none\fP, this is an unused slot
and it should be ignored.

If \f(CWdrd_type\fP is \f(CWdwarf_drt_data_reloc\fP 
this is an ordinary relocation.
The relocation type means either
(R_MIPS_64) or (R_MIPS_32) (or the like for
the particular ABI.
\f(CWdrd_length\fP gives the length of the field to be relocated.
\f(CWdrd_offset\fP
is an offset (of the
value to be relocated) in
the section this relocation stuff is linked to.
\f(CWdrd_symbol_index\fP
is the symbol index (if elf symbol
indices were provided) or the handle to arbitrary
information (if that is what the caller passed in 
to the relocation-creating dwarf calls) of the symbol
that the relocation is relative to.


When \f(CWdrd_type\fP is \f(CWdwarf_drt_first_of_length_pair\fP
the next data record will be \f(CWdrt_second_of_length_pair\fP
and the \f(CWdrd_offset\fP of the two data records will match.
The relevant 'offset' in the section this reloc applies to
should contain a symbolic pair like
.nf
.in +4
 .word    second_symbol - first_symbol
.in -4
.fi
to generate a length.
\f(CWdrd_length\fP gives the length of the field to be relocated.

\f(CWdrt_segment_rel\fP
means (R_MIPS_SCN_DISP)
is the real relocation (R_MIPS_SCN_DISP applies to
exception tables and this part may need further work).
\f(CWdrd_length\fP gives the length of the field to be relocated.

.P
The memory space of the section byte stream is freed
by the \f(CWdwarf_producer_finish_a() \fP call
(or would be if the \f(CWdwarf_producer_finish_a() \fP
was actually correct), along
with all the other space in use with that Dwarf_P_Debug.

.H 3 "dwarf_reset_section_bytes()"

.DS
\f(CWvoid dwarf_reset_section_bytes(
        Dwarf_P_Debug dbg
        )\fP
.DE
The function \f(CWdwarf_reset_section_bytes() \fP 
is used to reset the internal information so that
\f(CWdwarf_get_section_bytes_a() \fP will begin (on the next
call) at the initial dwarf section again.
It also resets so that calls to 
\f(CWdwarf_get_relocation_info() \fP
will begin again at the initial array of relocation information.

Some dwarf producers need to be able to run through
the \f(CWdwarf_get_section_bytes_a()\fP
and/or
the \f(CWdwarf_get_relocation_info()\fP
calls more than once and this call makes additional 
passes possible.
The set of Dwarf_Ptr values returned is identical to the
set returned by the first pass.
It is acceptable to call this before finishing a pass
of \f(CWdwarf_get_section_bytes_a()\fP
or
\f(CWdwarf_get_relocation_info()\fP
calls.
No errors are possible as this just resets some
internal pointers.
It is unwise to call this before
\f(CWdwarf_transform_to_disk_form() \fP has been called.
.P

.H 3 "dwarf_pro_get_string_stats()"
.DS
\f(CWint dwarf_pro_get_string_stats(
    Dwarf_P_Debug dbg,
    Dwarf_Unsigned * str_count,
    Dwarf_Unsigned * str_total_length,
    Dwarf_Unsigned * strp_count_debug_str,
    Dwarf_Unsigned * strp_len_debug_str,
    Dwarf_Unsigned * strp_reused_count,
    Dwarf_Unsigned * strp_reused_len,
    Dwarf_Error* error) \fP
.DE
If it returns 
\f(CWDW_DLV_OK\fP 
the function 
\f(CWdwarf_pro_get_string_stats()\fP 
returns information about how 
\f(CWDW_AT_name\fP 
etc strings were stored in the output object.
The values suggest how much string duplication
was detected in the DWARF being created.
.P
Call it after calling
\f(CWdwarf_transform_to_disk_form()\fP
and before calling
\f(CWdwarf_producer_finish_a()\fP .
It has no effect on the object being output.
.P
On error it returns
\f(CWDW_DLV_ERROR\fP 
and sets
\f(CWerror\fP 
through the pointer.


.H 3 "dwarf_producer_finish_a()"
.DS
\f(CWint dwarf_producer_finish_a(
        Dwarf_P_Debug dbg,
        Dwarf_Error* error) \fP
.DE
This is new in September 2016 and has the newer interface style,
but is otherwise identical to
\f(CWdwarf_producer_finish() \fP.
.P
The function 
\f(CWdwarf_producer_finish_a() \fP
should be called after all 
the bytes of data have been copied somewhere
(normally the bytes are written to disk).  
It frees all dynamic space 
allocated for \f(CWdbg\fP, include space for the structure pointed to by
\f(CWdbg\fP.  
This should not be called till the data have been 
copied or written 
to disk or are no longer of interest.  
It returns 
\f(CWDW_DLV_OK\fP 
if successful.
.P
On error it returns
\f(CWDW_DLV_ERROR\fP 
and sets
\f(CWerror\fP 
through the pointer.

.H 2 "Debugging Information Entry Creation"
The functions in this section add new \f(CWDIE\fPs to the object,
and also the relationships among the \f(CWDIE\fP to be specified
by linking them up as parents, children, left or right siblings
of each other.  
In addition, there is a function that marks the
root of the graph thus created.

.H 3 "dwarf_add_die_to_debug_a()"
.DS
\f(CWint dwarf_add_die_to_debug_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die first_die,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_die_to_debug_a() \fP indicates to 
\f(CWLibdwarf\fP
the root 
\f(CWDIE\fP of the 
\f(CWDIE\fP graph that has been built so far.  
It is intended to mark the compilation-unit \f(CWDIE\fP for the 
object represented by \f(CWdbg\fP.  
The root \f(CWDIE\fP
is specified 
by \f(CWfirst_die\fP.
.P
It returns 
\f(CWDW_DLV_OK\fP on success, and 
\f(CWDW_DLV_error\fP on error.

.H 3 "dwarf_new_die_a()"
.DS
\f(CWint dwarf_new_die_a(
        Dwarf_P_Debug dbg, 
        Dwarf_Tag new_tag,
        Dwarf_P_Die parent,
        Dwarf_P_Die child,
        Dwarf_P_Die left_sibling, 
        Dwarf_P_Die right_sibling,
        Dwarf_P_Die *die_out,
        Dwarf_Error *error) \fP
.DE
On success 
\f(CWdwarf_new_die_a() \fP 
returns DW_DLV_OK
and
creates a new \f(CWDIE\fP with
its parent, child, left sibling, and right sibling \f(CWDIE\fPs
specified by 
\f(CWparent\fP, 
\f(CWchild\fP, 
\f(CWleft_sibling\fP,
and \f(CWright_sibling\fP, respectively.  
The new die is passed to the caller via the
argument
\f(CWdie_out() \fP .
There is no requirement
that all of these \f(CWDIE\fPs
be specified, i.e. any of these
descriptors may be \f(CWNULL\fP.  
If none is specified, this will
be an isolated \f(CWDIE\fP.  
A \f(CWDIE\fP is 
transformed to disk form by \f(CWdwarf_transform_to_disk_form() \fP 
only if there is a path from
the 
\f(CWDIE\fP
specified by 
\f(CWdwarf_add_die_to_debug\fP to it.
.P
The value of
\f(CWnew_tag\fP
is the tag which is given to the new
\f(CWDIE\fP.
\f(CWparent\fP,
\f(CWchild\fP, 
\f(CWleft_sibling\fP, and
\f(CWright_sibling\fP are pointers to establish links to existing 
\f(CWDIE\fPs.
Only one of 
\f(CWparent\fP, 
\f(CWchild\fP, 
\f(CWleft_sibling\fP, and 
\f(CWright_sibling\fP may be non-NULL.
If \f(CWparent\fP
(\f(CWchild\fP) is given, the \f(CWDIE\fP is 
linked into the list after (before) the \f(CWDIE\fP pointed to.  
If \f(CWleft_sibling\fP
(\f(CWright_sibling\fP) is given, the 
\f(CWDIE\fP
is linked into the list after (before) the \f(CWDIE\fP 
pointed to.
.P
To add attributes to the new \f(CWDIE\fP, 
use the \f(CWAttribute Creation\fP 
functions defined in the next section.
.P
On failure
\f(CWdwarf_new_die_a() \fP 
returns DW_DLV_ERROR and sets 
\f(CW*error\fP. 

.H 3 "dwarf_die_link_a()"
.DS
\f(CWint dwarf_die_link_a(
        Dwarf_P_Die die, 
        Dwarf_P_Die parent,
        Dwarf_P_Die child,
        Dwarf_P_Die left-sibling, 
        Dwarf_P_Die right_sibling,
        Dwarf_Error *error) \fP
.DE
On success
the function 
\f(CWdwarf_die_link_a() \fP 
returns 
\f(CWDW_DLV_OK\fP
and
links an existing 
\f(CWDIE\fP
described by the given
\f(CWdie\fP to other existing 
\f(CWDIE\fPs.
The given
\f(CWdie\fP can be linked to a parent
\f(CWDIE\fP, a child
\f(CWDIE\fP, a left sibling
\f(CWDIE\fP, or a right sibling
\f(CWDIE\fP
by specifying non-NULL 
\f(CWparent\fP, 
\f(CWchild\fP, 
\f(CWleft_sibling\fP,
and \f(CWright_sibling\fP 
\f(CWDwarf_P_Die\fP
descriptors.  

Only one of \f(CWparent\fP,
\f(CWchild\fP,
\f(CWleft_sibling\fP,
and \f(CWright_sibling\fP may be non-NULL.  
If \f(CWparent\fP
(\f(CWchild\fP) is given, the \f(CWDIE\fP is linked into the list 
after (before) the \f(CWDIE\fP pointed to.  
If \f(CWleft_sibling\fP
(\f(CWright_sibling\fP) is given, the 
\f(CWDIE\fP
is linked into 
the list after (before) the \f(CWDIE\fP pointed to.  
Non-NULL links
overwrite the corresponding links the given \f(CWdie\fP may have
had before the call to 
\f(CWdwarf_die_link_a() \fP.

If there is an error
\f(CWdwarf_die_link_a() \fP 
returns 
\f(CWDW_DLV_ERROR\fP 
and sets
\f(CWerror\fP 
with the specific applicable error code.

.H 2 "DIE Markers"
DIE markers provide a way for a producer to extract DIE offsets
from DIE generation.  The markers do not influence the
generation of DWARF, they simply allow a producer to
extract .debug_info offsets for whatever purpose the
producer finds useful (for example, a producer might
want some unique other section unknown
to libdwarf to know a particular DIE offset).

One marks one or more DIEs as desired any time before
calling \f(CWdwarf_transform_to_disk_form()\fP.

After calling \f(CWdwarf_transform_to_disk_form()\fP
call 
\f(CWdwarf_get_die_markers()\fP 
which has the offsets where the marked DIEs were written
in the generated .debug_info data.

.H 3 "dwarf_add_die_marker_a()"
.DS
\f(CWint dwarf_add_die_marker_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die die,
        Dwarf_Unsigned marker,
        Dwarf_Error *error) \fP
.DE
This is preferred over
\f(CWdwarf_add_die_marker()\fP.
The function 
\f(CWdwarf_add_die_marker_a()\fP 
writes the
value 
\f(CWmarker\fP
to the \f(CWDIE\fP descriptor given by
\f(CWdie\fP.  
Passing in a marker of 0 means 'there is no marker'
(zero is the default in DIEs).

It returns \f(CWDW_DLV_OK\fP, on success.  
On error it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_get_die_marker_a()"
.DS
\f(CWint dwarf_get_die_marker_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die die,
        Dwarf_Unsigned *marker,
        Dwarf_Error *error) \fP
.DE
The function \f(CWdwarf_get_die_marker_a() \fP returns the
current marker value for this DIE
through the pointer \f(CWmarker\fP.
A marker value of 0 means 'no marker was set'.

It returns \f(CWDW_DLV_OK\fP, on success.  
On error it returns \f(CWDW_DLV_ERROR\fP.


.H 3 "dwarf_get_die_markers_a()"
.DS
\f(CWint dwarf_get_die_markers_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Marker * marker_list,
        Dwarf_Unsigned *marker_count,
        Dwarf_Error *error) \fP
.DE 
The function \f(CWdwarf_get_die_markers_a()\fP returns
a pointer to an array of \f(CWDwarf_P_Marker\fP pointers to
\f(CWstruct Dwarf_P_Marker_s\fP structures through
the pointer \f(CWmarker_list\fP.
The array length is returned through the
pointer \f(CWmarker_count\fP.
    
The call is only meaningful after 
a call to \f(CWdwarf_transform_to_disk_form()\fP as the
transform call creates the \f(CWstruct Dwarf_P_Marker_s\fP
structures, one for each DIE generated for .debug_info
(but only for DIEs that had a non-zero marker value).
The field \f(CWma_offset\fP in the structure is set
during generation of the .debug_info byte stream.
The field \f(CWma_marker\fP in the structure is a copy
of the DIE marker of the DIE given that offset.

It returns \f(CWDW_DLV_OK\fP, on success.
On error it returns \f(CWDW_DLV_ERROR\fP (if there are no
markers it returns \f(CWDW_DLV_ERROR\fP).


.H 4 "dwarf_get_die_markers()"
.DS
\f(CWDwarf_Signed dwarf_get_die_markers(
        Dwarf_P_Debug dbg,
        Dwarf_P_Marker * marker_list,
        Dwarf_Unsigned *marker_count,
        Dwarf_Error *error) \fP
.DE
The function \f(CWdwarf_get_die_marker()\fP returns
a pointer to an array of \f(CWDwarf_P_Marker\fP pointers to
\f(CWstruct Dwarf_P_Marker_s\fP structures through
the pointer \f(CWmarker_list\fP.
The array length is returned through the
pointer \f(CWmarker_count\fP.

The call is only meaningful after 
a call to \f(CWdwarf_transform_to_disk_form()\fP as the
transform call creates the \f(CWstruct Dwarf_P_Marker_s\fP
structures, one for each DIE generated for .debug_info
(but only for DIEs that had a non-zero marker value).
The field \f(CWma_offset\fP in the structure is set
during generation of the .debug_info byte stream.
The field \f(CWma_marker\fP in the structure is a copy
of the DIE marker of the DIE given that offset.

It returns \f(CW0\fP, on success.
On error it returns \f(CWDW_DLV_BADADDR\fP (if there are no
markers it returns \f(CWDW_DLV_BADADDR\fP).

.H 2 "Attribute Creation"
The functions in this section add attributes to a \f(CWDIE\fP.
These functions return a \f(CWDwarf_P_Attribute\fP descriptor 
that represents the attribute added to the given \f(CWDIE\fP.  
In most cases the return value is only useful to determine if 
an error occurred.

Some of the attributes have values that are relocatable.  
They
need a symbol with respect to which the linker will perform
relocation.  
This symbol is specified by means of an index into
the Elf symbol table for the object
(of course, the symbol index can be more general than an index).

.H 3 "dwarf_add_AT_location_expr_a()"
.DS
\f(CWint dwarf_add_AT_location_expr_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die ownerdie,
        Dwarf_Half attr,
        Dwarf_P_Expr loc_expr,
        Dwarf_P_Attr *attr_out,
        Dwarf_Error *error) \fP
.DE
On success the function
\f(CWdwarf_add_AT_location_expr_a()\fP
returns
\f(CWDW_DLV_OK\fP
and adds the attribute
specified by
\f(CWattr\fP
to the
\f(CWDIE\fP descriptor given by
\f(CWownerdie\fP.
The new attribute is passed back
to the caller through the pointer
\f(CWattr_out\fP.

The attribute should be one that has a location
expression as its value.
The location expression that is the value
is represented by the
\f(CWDwarf_P_Expr\fP descriptor
\f(CWloc_expr\fP.

If the expression has a 
\f(CWDW_OP_addr\fP
the code simply assumes that
\f(CWDW_OP_addr\fP
is
the first operation and
bases the only relocation
that can be created
on that assumption.

On error it returns 
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_name_a()"
.DS
\f(CWint dwarf_add_AT_name_a(
        Dwarf_P_Die ownerdie, 
        char *name,
        Dwarf_P_Attribute * attr_out,
        Dwarf_Error *error) \fP
.DE

The function \f(CWdwarf_add_AT_name_a() \fP adds
the string specified by
\f(CWname\fP as the
value of the \f(CWDW_AT_name\fP attribute for the
given \f(CWDIE\fP, \f(CWownerdie\fP.  It returns
DW_DLV_OK on success and assigns the new
attribute descriptor to
\f(CW*attr_out\fP.

On error it returns
\f(CWDW_DLV_ERROR\fP
and does not set
\f(CW*attr_out\fP.

.H 3 "dwarf_add_AT_comp_dir_a()"
.DS
\f(CWint dwarf_add_AT_comp_dir_a(
        Dwarf_P_Die ownerdie,
        char *current_working_directory,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_comp_dir_a\fP adds the string given by
\f(CWcurrent_working_directory\fP as the value of the
\f(CWDW_AT_comp_dir\fP
attribute for the
\f(CWDIE\fP described by the given 
\f(CWownerdie\fP.  
On success it returns 
\f(CWDW_DLV_OK\fP
and sets
\f(CW*attr_out\fP to the new attribute.
.P
On error, it returns 
\f(CWDW_DLV_ERROR\fP
and does not touch \f(CWattr_out\fP .

.H 3 "dwarf_add_AT_producer_a()"
.DS
\f(CWint dwarf_add_AT_producer_a(
        Dwarf_P_Die ownerdie,
        char *producer_string,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_producer_a()\fP
adds the string given by
\f(CWproducer_string\fP as the value of the 
\f(CWDW_AT_producer\fP
attribute for the 
\f(CWDIE\fP given by 
\f(CWownerdie\fP.

On success it returns
\f(CWDW_DLV_OK\fP
and returns the new attribute descriptor 
representing this attribute
through the pointer argument
\f(CWattr_out\fP.

On error, it returns 
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_any_value_sleb_a()"
.DS
\f(CWint dwarf_add_AT_any_value_sleb_a(
        Dwarf_P_Die ownerdie,
        Dwarf_Half  attrnum,
        Dwarf_Signed signed_value,
        Dwarf_P_Attribute *out_attr,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_any_value_sleb_a()\fP adds the
given 
\f(CWDwarf_Signed\fP value 
\f(CWsigned_value\fP as the value
of the 
\f(CWDW_AT_const_value\fP attribute for the 
\f(CWDIE\fP
described by the given \f(CWownerdie\fP.

The FORM of the output value is 
\f(CWDW_FORM_sdata\fP (signed leb number)
and the attribute will be \f(CWDW_AT_const_value\fP.

On success it returns
\f(CWDW_DLV_OK\fP
and sets
\f(CW*out_attr\fP to the created attribute.

On error, it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_const_value_signedint_a()"
.DS
\f(CWint dwarf_add_AT_const_value_signedint_a(
        Dwarf_P_Die ownerdie,
        Dwarf_Signed signed_value,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_const_value_signedint_a\fP adds the
given 
\f(CWDwarf_Signed\fP value 
\f(CWsigned_value\fP as the value
of the \f(CWDW_AT_const_value\fP attribute for the \f(CWDIE\fP
described by the given \f(CWownerdie\fP.

The FORM of the output value is 
\f(CWDW_FORM_data<n>\fP (signed leb number)
and the attribute will be \f(CWDW_AT_const_value\fP.

With this interface and output, there is no way for consumers
to know from the FORM that the value is signed.


On success it returns \f(CWDW_DLV_OK\fP
and sets *attr_out to the created attribute.

On error, it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_implicit_const()"
.DS
\f(CWint dwarf_add_AT_implicit_const(Dwarf_P_Die ownerdie,
    Dwarf_Half attrnum,
    Dwarf_Signed signed_value,
    Dwarf_P_Attribute *outattr,
    Dwarf_Error * error); \fP
.DE
The function
\f(CWdwarf_add_AT_implicit_const\fP
creates a new attribute and
adds the signed value to the abbreviation entry
for this new attribute and attaches the new attribute
to the DIE passed in.
.P
The new attribute has
 \f(CWattrnum\fP attribute for the \f(CWDIE\fP described
by the given \f(CWownerdie\fP.
The
\f(CWform\fP in the generated attribute is
\f(CWDW_FORM_implicit_const.\fP
The
\f(CWsigned_value\fP argument will be inserted
in the abbreviation table as a signed leb
value.
.P
For a successful call the function returns
\f(CWDW_DLV_OK.\fP
and a pointer to the created argument is returned
through the pointer 
\f(CWoutaddr.\fP

.P
In case of error the function returns
\f(CWDW_DLV_ERROR\fP
and no attribute is created.

.H 3 "dwarf_add_AT_any_value_uleb_a()"
.DS
\f(CWint dwarf_add_AT_any_value_uleb_a(
        Dwarf_P_Die ownerdie,
        Dwarf_Half  attrnum,
        Dwarf_Unsigned unsigned_value,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_any_value_uleb_a\fP adds the
given 
\f(CWDwarf_Unsigned\fP value 
\f(CWunsigned_value\fP as the value
of the 
\f(CWattrnum\fP attribute for the 
\f(CWDIE\fP described
by the given \f(CWownerdie\fP.

The FORM of the output value is 
\f(CWDW_FORM_udata\fP (unsigned leb number)
and the attribute is \f(CWattrnum\fP.

On success it returns
\f(CWDW_DLV_OK\fP
and sets
\f(CW*attr_out\fP
to the newly created attribute.

On error, it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_const_value_unsignedint_a()"
.DS
\f(CWint dwarf_add_AT_const_value_unsignedint_a(
        Dwarf_P_Die ownerdie,
        Dwarf_Unsigned unsigned_value,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_add_AT_const_value_unsignedint_a\fP adds the
given 
\f(CWDwarf_Unsigned\fP value 
\f(CWunsigned_value\fP as the value
of the 
\f(CWDW_AT_const_value\fP attribute for the 
\f(CWDIE\fP described
by the given 
\f(CWownerdie\fP.

The FORM of the output value is 
\f(CWDW_FORM_data<n>\fP
and the attribute will be 
\f(CWDW_AT_const_value\fP.

With this interface and output, there is no way for consumers
to know from the FORM that the value is signed.

On success it returns
\f(CWDW_DLV_OK\fP.
and sets \f(CW*attr_out\fP
to the newly created attribute.

On error, it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_const_value_string_a()"
.DS
\f(CWint dwarf_add_AT_const_value_string_a(
    Dwarf_P_Die ownerdie,
    char *string_value,
    Dwarf_P_Attribute *attr_out,
    Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_const_value_string_a()\fP adds the
string value given by 
\f(CWstring_value\fP as the value of the
\f(CWDW_AT_const_value\fP attribute for the 
\f(CWDIE\fP described
by the given 
\f(CWownerdie\fP.  
.P
On success it returns 
\f(CWDW_DLV_OK\fP
\f(CW*attr_out\fP
to a newly created attribute.
.P
On error, it returns
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_targ_address_c()"
.DS
\f(CWint dwarf_add_AT_targ_address_c(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die ownerdie,
        Dwarf_Half attr,
        Dwarf_Unsigned pc_value,
        Dwarf_Unsigned sym_index,  
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_targ_address_c()\fP 
is identical to 

.P
\f(CWsym_index\fP is guaranteed to
be large enough that it can contain a pointer to
arbitrary data (so the caller can pass in a real elf
symbol index, an arbitrary number, or a pointer
to arbitrary data).
The ability to pass in a pointer through 
\f(CWsym_index\fP
is only usable with
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP.

On success the function returns
\f(CWDW_DLV_OK\fP
\f(CWDwarf_P_Attribute\fP
and
\f(CWpc_value\fP
is put into the section stream output and
the 
\f(CWsym_index\fP is applied to the relocation
information.

On failure it returns
\f(CWDW_DLV_ERROR\fP.

Do not use this function for attr 
\f(CWDW_AT_high_pc\fP
if the value to be recorded is an offset (not a pc)
[ use 
\f(CWdwarf_add_AT_unsigned_const_afP
or  
\f(CWdwarf_add_AT_any_value_uleb_afP
instead].

On failure the function returns
\f(CWDW_DLV_ERROR\fP

.H 3 "dwarf_add_AT_block_a()"
.DS
\f(CWint dwarf_add_AT_block_a(
    Dwarf_P_Debug       dbg,
    Dwarf_P_Die         ownerdie,
    Dwarf_Half          attr,
    Dwarf_Small         *block_data,
    Dwarf_Unsigned      block_size,
    Dwarf_P_Attribute*  attr_out,
    Dwarf_Error         *error)
.DE
This function works with
all
\f(CWDW_FORM_block\fP
forms
as well as
\f(CWDW_FORM_exprloc\fP.

On success this returns 
\f(CWDW_DLV_OK\fP
an attribute
with a
\f(CWDW_FORM_block\fP
instance
(does not create 
\f(CWDW_FORM_block1\fP,
\f(CWDW_FORM_block2\fP, or
\f(CWDW_FORM_block4\fP
at present)
and returns a pointer to the new attribute
through the pointer
\f(CWattr_out\fP.

On failure this returns
\f(CWDW_DLV_ERROR\fP

.H 3 "dwarf_add_AT_dataref_a()"
.DS
\f(CWint dwarf_add_AT_dataref_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die ownerdie,
        Dwarf_Half attr,
        Dwarf_Unsigned pc_value,
        Dwarf_Unsigned sym_index,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error) \fP
.DE
This is very similar to 
\f(CWdwarf_add_AT_targ_address_b\fP
but results in a different FORM (results in DW_FORM_data4
or DW_FORM_data8).

Useful for adding relocatable addresses in location lists.

\f(CWsym_index\fP is guaranteed to
be large enough that it can contain a pointer to
arbitrary data (so the caller can pass in a real elf
symbol index, an arbitrary number, or a pointer
to arbitrary data).
The ability to pass in a pointer through 
\f(CWsym_index\fP
is only usable with
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP.

On success it returns 
\f(CWDW_DLV_OK\fP
and
the 
\f(CWpc_value\fP
is put into the section stream output and
the 
\f(CWsym_index\fP is applied to the relocation
information.

Do not use this function for 
\f(CWDW_AT_high_pc\fP, use
\f(CWdwarf_add_AT_unsigned_const\fP
or 
\f(CWdwarf_add_AT_any_value_uleb\fP
[ if the value to be recorded is
an offset of 
\f(CWDW_AT_low_pc\fP]
or 
\f(CWdwarf_add_AT_targ_address_b\fP [ if the value
to be recorded is an address].

.H 3 "dwarf_add_AT_ref_address_a"
.DS
\f(CWint dwarf_add_AT_ref_address_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die ownerdie,
        Dwarf_Half attr,
        Dwarf_Unsigned pc_value,
        Dwarf_Unsigned sym_index,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error) \fP
.DE

This is very similar to 
\f(CWdwarf_add_AT_targ_address_c\fP
but results in a different FORM (results in 
\f(CWDW_FORM_ref_addr\fP
being generated).

Useful for  
\f(CWDW_AT_type\fP and 
\f(CWDW_AT_import\fP attributes.

\f(CWsym_index() \fP is guaranteed to
be large enough that it can contain a pointer to
arbitrary data (so the caller can pass in a real elf
symbol index, an arbitrary number, or a pointer
to arbitrary data).
The ability to pass in a pointer through \f(CWsym_index() \fP
is only usable with
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP.

On success the function returns
\f(CWDW_DLV_OK\fP and
\f(CWpc_value\fP
is put into the section stream output and
the 
\f(CWsym_index\fP is applied to the relocation
information.

On failure the function returns
\f(CWDW_DLV_ERROR\fP.

Do not use this function for 
\f(CWDW_AT_high_pc\fP.  

.H 3 "dwarf_add_AT_unsigned_const_a()"
.DS
\f(CWint dwarf_add_AT_unsigned_const_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die ownerdie,
        Dwarf_Half attr,
        Dwarf_Unsigned value,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_unsigned_const_a()\fP adds an attribute
with a 
\f(CWDwarf_Unsigned\fP value belonging to the "constant" class,
to the 
\f(CWDIE\fP specified by 
\f(CWownerdie\fP.  The object that
the 
\f(CWDIE\fP belongs to is specified by 
\f(CWdbg\fP.  The attribute
is specified by 
\f(CWattr\fP, and its value is specified by 
\f(CWvalue\fP.

The FORM of the output will be one of the 
\f(CWDW_FORM_data<n>\fP forms.


On success it returns 
\f(CWDW_DLV_OK\fP
and sets
\f(CW*attr_out\fP to the newly created attribute.

It returns
\f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_add_AT_signed_const_a()"
.DS
\f(CWint dwarf_add_AT_signed_const_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die ownerdie,
        Dwarf_Half attr,
        Dwarf_Signed value,
        Dwarf_P_Attribute *out_addr,
        Dwarf_Error *error) \fP
.DE
The function 
\f(CWdwarf_add_AT_signed_const_a()\fP adds an attribute
with a 
\f(CWDwarf_Signed\fP value belonging to the "constant" class,
to the 
\f(CWDIE\fP specified by 
\f(CWownerdie\fP.  The object that
the 
\f(CWDIE\fP belongs to is specified by 
\f(CWdbg\fP.  The attribute
is specified by 
\f(CWattr\fP, and its value is specified by 
\f(CWvalue\fP.

On success it returns
\f(CWDW_DLV_OK\fP and sets
\f(CW*out_addr\fP
with a pointer to the new attribute.

On error it returns
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_reference_c()"
.DS
\f(CWint dwarf_add_AT_reference_c(
        Dwarf_P_Debug dbg,
        Dwarf_Half attr,
        Dwarf_P_Die ownerdie,
        Dwarf_P_Die otherdie,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error)\fP
.DE

\f(CWdwarf_add_AT_reference_c()\fP
accepts a NULL 
\f(CWotherdie\fP with the assumption
that 
\f(CWdwarf_fixup_AT_reference_die()\fP
will be called by user code
to fill in the missing 
\f(CWotherdie\fP
before the DIEs are transformed to disk form.

On success it returns
\f(CWDW_DLV_OK\fP
and returns a pointer to the new attribute
through \f(CW*attr_out\fP.

On failure it returns
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_fixup_AT_reference_die()"
.DS
\f(CWint dwarf_fixup_AT_reference_die(
        Dwarf_Half attrnum,
        Dwarf_P_Die ownerdie,
        Dwarf_P_Die otherdie,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_fixup_AT_reference_die()\fP
is provided to set the NULL \f(CWotherdie\fP
that  
\f(CWdwarf_add_AT_reference_c()\fP allows
to the reference target DIE.
This must be done before transforming to disk form.
\f(CWattrnum()\fP should be the 
attribute number of the attribute of \fCWownerdie\fP which is
to be updated.
For example, if a local forward reference
was in a \fCWDW_AT_sibling\fP attribute in ownerdie, pass
the value \fCWDW_AT_sibling\fP as attrnum.
.P
Since no attribute number can appear more than once on a
given DIE
the 
\f(CWattrnum()\fP suffices to uniquely identify which
attribute of \fCWownerdie\fP to update
.P
It returns either \f(CWDW_DLV_OK\fP (on success) 
or \f(CWDW_DLV_ERROR\fP (on error).
Calling this on an attribute where \f(CWotherdie\fP
was already set is an error.

.H 3 "dwarf_add_AT_flag_a()"
.DS
\f(CWint dwarf_add_AT_flag_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die ownerdie,
        Dwarf_Half attr,
        Dwarf_Small flag,
        Dwarf_P_Attribute *attr_out, 
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_add_AT_flag_a()\fP adds an attribute with
a 
\f(CWDwarf_Small\fP value belonging to the "flag" class, to the
\f(CWDIE\fP specified by 
\f(CWownerdie\fP.
The object that the
\f(CWDIE\fP belongs to is specified by 
\f(CWdbg\fP.
The attribute
is specified by 
\f(CWattr\fP, and its value is specified by 
\f(CWflag\fP.

On success it returns
\f(CWDW_DLV_OK\fP
and passes back a pointer to the new attribute
through
\f(CW*attr_out\fP.

On error it returns
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_string_a()"
.DS
\f(CWint dwarf_add_AT_string_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die ownerdie,
        Dwarf_Half attr,
        char *string,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_add_AT_string()\fP adds an attribute with a
value that is a character string to the 
\f(CWDIE\fP specified by
\f(CWownerdie\fP.  The object that the 
\f(CWDIE\fP belongs to is
specified by 
\f(CWdbg\fP.  The attribute is specified by 
\f(CWattr\fP,
and its value is pointed to by 
\f(CWstring\fP.

On success
it returns 
\f(CWDW_DLV_OK\fP
and set \f(CW*attr_out\fP
with a pointer to the new attribute.

On failure it returns 
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_with_ref_sig8_a()"
.DS
\f(CWint dwarf_add_AT_with_ref_sig8_a(
        Dwarf_P_Die ownerdie,
        Dwarf_Half attrnum,
        const Dwarf_Sig8 *sig8_in,
        Dwarf_P_Attribute *attr_out,
        Dwarf_Error * error)\fP
.DE
The function
\f(CWdwarf_add_AT_with_sig8_a\fP
creates an attribute containing
the 8-byte signature block
pointed to by 
\f(CWsig8_in\fP
\f(CWDW_FORM_ref_sig8\fP
with form
\f(CWDW_FORM_ref_sig8\fP.


On success it returns 
\f(CWDW_DLV_OK\fP
and sets *attr_out
\f(CW*attr_out\fP 
to the newly created
attribute.

On failure it returns 
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_AT_data16()"
.DS
\f(CWint dwarf_add_AT_data16(
        Dwarf_P_Die ownerdie,
        Dwarf_Half attrnum,
        Dwarf_Form_Data16 *ptr_to_val,
        Dwarf_P_Attribute * attr_out,
        Dwarf_Error * error)\fP
.DE
The
DWARF5
standard refers to 16 byte as simply data.
It is up to the eventual reader of the 
DWARF
entry this call creates 
to understand what the sixteen
bytes mean.
.P
On success it returns
\f(CWDW_DLV_OK\fP
and returns the new attribute through
the pointer
\f(CWattr_out\fP.


On failure it returns 
\f(CWDW_DLV_ERROR\fP.


.H 3 "dwarf_compress_integer_block()"
.DS
\f(CWvoid* dwarf_compress_integer_block(
    Dwarf_P_Debug    dbg,
    Dwarf_Bool       unit_is_signed,
    Dwarf_Small      unit_length_in_bits,
    void*            input_block,
    Dwarf_Unsigned   input_length_in_units,
    Dwarf_Unsigned*  output_length_in_bytes_ptr,
    Dwarf_Error*     error)
.DE
This was created in 2016 in support of the attribute
DW_AT_SUN_func_offsets but the particular DWARF project
involving this seems to have died. 
We have not provided a way to create
the attribute.
So this is pretty useless at this time.

.H 2 "Expression Creation"
The following functions are used to convert location expressions into
blocks so that attributes with values that are location expressions
can store their values as a \f(CWDW_FORM_blockn\fP value.  This is for 
both .debug_info and .debug_loc expression blocks.

To create an expression, first call \f(CWdwarf_new_expr_a()\fP to get 
a \f(CWDwarf_P_Expr\fP descriptor that can be used to build up the
block containing the location expression.  Then insert the parts of 
the expression in prefix order (exactly the order they would be 
interpreted in in an expression interpreter).  The bytes of the 
expression are then built-up as specified by the user.

.H 3 "dwarf_new_expr_a()"
.DS
\f(CWint dwarf_new_expr_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Expr *expr_out,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_new_expra()\fP creates a new expression area 
in which a location expression stream can be created.

On success
it returns
\f(CWDW_DLV_OK\fP
and returns a 
Dwarf_Expr
\f(CWDwarf_Expr\fP
through
the pointer 
which can be used to add operators
a to build up a location expression.

On failure it returns
\f(CWDW_DLV_OK\fP.

.H 3 "dwarf_add_expr_gen_a()"
.DS
\f(CWint dwarf_add_expr_gen_a(
        Dwarf_P_Expr expr,
        Dwarf_Small opcode,
        Dwarf_Unsigned val1,
        Dwarf_Unsigned val2,
        Dwarf_Unsigned *stream_length_out,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_add_expr_gen()\fP takes an operator specified
by 
\f(CWopcode\fP, along with up to 2 operands specified by 
\f(CWval1\fP,
and 
\f(CWval2\fP, converts it into the 
\f(CWDwarf\fP representation and
appends the bytes to the byte stream being assembled for the location
expression represented by 
\f(CWexpr\fP.
The first operand, if present,
to 
\f(CWopcode\fP is in 
\f(CWval1\fP, and the second operand, if present,
is in 
\f(CWval2\fP.  Both the operands may actually be signed or unsigned
depending on 
\f(CWopcode\fP.

On success it returns 
\f(CWDW_DLV_OK\fP and sets
\f(CW*stream_length_out\fP
to
the number of bytes in the byte
stream for 
\f(CWexpr\fP currently generated, i.e. after the addition of
\f(CWopcode\fP.

It returns
\f(CWDW_DLV_ERROR\fP on error.

The function 
\f(CWdwarf_add_expr_gen_a()\fP works for all opcodes except
those that have a target address as an operand.  This is because 
the function cannot
not set up a relocation record that is needed when target addresses are
involved.

.H 3 "dwarf_add_expr_addr_c()"
.DS
\f(CWint dwarf_add_expr_addr_c(
        Dwarf_P_Expr expr,
        Dwarf_Unsigned address,
        Dwarf_Unsigned sym_index,
        Dwarf_Unsigned *stream_length_out,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_add_expr_addr_c()\fP is
identical to 
\f(CWdwarf_add_expr_addr_b()\fP
except that 
\f(CWdwarf_add_expr_addr_c()\fP 
returns a simple integer code.
.P
\f(CWsym_index() \fP is guaranteed to
be large enough that it can contain a pointer to
arbitrary data (so the caller can pass in a real elf
symbol index, an arbitrary number, or a pointer
to arbitrary data).
The ability to pass in a pointer through 
\f(CWsym_index() \fP
is only usable with
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP.

On success the function returns
\f(CWDW_DLV_OK\fP and sets
\f(CW*stream_length_out\fP
to to the total length of the expression stream
in
\f(CWexpr\fP.

On failure the function returns
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_expr_current_offset_a()"
.DS
\f(CWint dwarf_expr_current_offset_a(
        Dwarf_P_Expr expr, 
        Dwarf_Unsigned *stream_offset_out,
        Dwarf_Error *error)\fP
.DE
On success
the function 
\f(CWdwarf_expr_current_offset_a()\fP
returns
\f(CWDW_DLV_OK\fP
and sets 
\f(CW*stream_offset_out\fP
to the current length in bytes of the expression
stream.

On failure the function returns
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_expr_into_block_a()"
.DS
\f(CWint dwarf_expr_into_block_a(
        Dwarf_P_Expr expr,
        Dwarf_Unsigned *length,
        Dwarf_Small    **address,
        Dwarf_Error *error)\fP
.DE
On success
the function 
\f(CWdwarf_expr_into_block_a()\fP
returns
\f(CWDW_DLV_OK\fP
and sets the length of the
\f(CWexpr\fP expression
into 
\f(CW*length\fP
and sets the value of a pointer
into memory where the expression
is currently held in the executing libdwarf into
\f(CW*address\fP.
.P
On failure it returns
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_expr_reset()"
.DS
\f(CWvoid dwarf_expr_reset(
        Dwarf_P_Expr expr,
        Dwarf_Error *error)\fP
.DE
This resets the expression content of
\f(CWexpr()\fP
to be empty.


.H 2 "Line Number Operations"
These are operations on the .debug_line section.  
They provide 
information about instructions in the program and the source 
lines the instruction come from.  
Typically, code is generated 
in contiguous blocks, which may then be relocated as contiguous 
blocks.  
To make the provision of relocation information more 
efficient, the information is recorded in such a manner that only
the address of the start of the block needs to be relocated.  
This is done by providing the address of the first instruction 
in a block using the function \f(CWdwarf_lne_set_address()\fP.  
Information about the instructions in the block are then added 
using the function \f(CWdwarf_add_line_entry_c()\fP, which specifies
offsets from the address of the first instruction.  
The end of 
a contiguous block is indicated by calling the function 
\f(CWdwarf_lne_end_sequence()\fP.
.P
Line number operations do not support
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP.

.H 3 "dwarf_add_line_entry_c()"
.DS
\f(CWint dwarf_add_line_entry_c(
        Dwarf_P_Debug dbg,
        Dwarf_Unsigned file_index, 
        Dwarf_Addr code_offset,
        Dwarf_Unsigned lineno, 
        Dwarf_Signed column_number,
        Dwarf_Bool is_source_stmt_begin, 
        Dwarf_Bool is_basic_block_begin,
        Dwarf_Bool is_epilogue_begin,
        Dwarf_Bool is_prologue_end,
        Dwarf_Unsigned  isa,
        Dwarf_Unsigned  discriminator,
        Dwarf_Error *error)\fP
.DE

The function \f(CWdwarf_add_line_entry_c()\fP
adds an entry to the
section containing information about source lines.  
It specifies
in \f(CWcode_offset\fP, the address of this line.
The function subtracts \f(CWcode_offset\fP from the value given
as the address of a previous line call to compute an offset,
and the offset is what is recorded in the line instructions
so no relocation will be needed on the line instruction generated.
.P
The source file that gave rise
to the instruction is specified by \f(CWfile_index\fP, the source
line number is specified by \f(CWlineno\fP, and the source column 
number is specified by \f(CWcolumn_number\fP
(column numbers begin at 1)
(if the source column is unknown, specify 0).  
\f(CWfile_index\fP 
is the index of the source file in a list of source files which is 
built up using the function \f(CWdwarf_add_file_decl()\fP. 

\f(CWis_source_stmt_begin\fP is a boolean flag that is true only if 
the instruction at \f(CWcode_address\fP is the first instruction in 
the sequence generated for the source line at \f(CWlineno\fP.  Similarly,
\f(CWis_basic_block_begin\fP is a boolean flag that is true only if
the instruction at \f(CWcode_address\fP is the first instruction of
a basic block.

\f(CWis_epilogue_begin\fP is a boolean flag that is true only if 
the instruction at \f(CWcode_address\fP is the first instruction in 
the sequence generated for the function epilogue code.

Similarly, \f(CWis_prolgue_end\fP is a boolean flag that is true only if
the instruction at \f(CWcode_address\fP is the last instruction of
the sequence generated for the function prologue.

\f(CWisa\fP should be zero unless the code 
at \f(CWcode_address\fP is generated in a non-standard isa.
The values assigned to non-standard isas are defined by the compiler
implementation. 

\f(CWdiscriminator\fP should be zero unless the line table
needs to distinguish among multiple blocks
associated with the same source file, line, and column.
The values assigned to \f(CWdiscriminator\fP are defined by the compiler
implementation.

It returns 
\f(CWDW_DLV_OK\fP on success, and 
\f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_lne_set_address_a()"
.DS
\f(CWint dwarf_lne_set_address_a(
        Dwarf_P_Debug dbg,
        Dwarf_Addr offs,
        Dwarf_Unsigned symidx,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_lne_set_address_a()\fP sets the target address
at which a contiguous block of instructions begin.
Information about
the instructions in the block is added to .debug_line using calls to
\f(CWdwarfdwarf_add_line_entry_c()\fP
which specifies the offset of each
instruction in the block relative to the start of the block.  
This is
done so that a single relocation record can be used to obtain the final
target address of every instruction in the block.

The relocatable address of the start of the block of instructions is
specified by 
\f(CWoffs\fP.
The symbol used to relocate the address
is given by 
\f(CWsymidx\fP, which is normally the index of the symbol in the
Elf symbol table.

It returns 
\f(CWDW_DLV_OK\fP on success, and 
\f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_lne_end_sequence_a()"
.DS
\f(CWint dwarf_lne_end_sequence_a(
        Dwarf_P_Debug dbg,
        Dwarf_Addr    address;
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_lne_end_sequence_a()\fP
indicates the end of a
contiguous block of instructions.
\f(CWaddress()\fP
should be just higher than the end of the last address in the
sequence of instructions.
Before the next
block of instructions (if any) a call to 
\f(CWdwarf_lne_set_address_a()\fP will
have to be made to set the address of the start of the target address
of the block, followed by calls to 
\f(CWdwarf_add_line_entry_a()\fP for
each of the instructions in the block.

It returns 
\f(CWDW_DLV_OK\fP
on success and
\f(CWDW_DLV_ERROR\fP
on error.

.H 3 "dwarf_add_directory_decl_a()"
.DS
\f(CWint dwarf_add_directory_decl_a(
        Dwarf_P_Debug dbg,
        char *name,
        Dwarf_Unsigned *index_in_directories,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_add_directory_decl()\fP adds the string
specified by \f(CWname\fP to the list of include directories in
the statement program prologue of the .debug_line section.
The
string should therefore name a directory from which source files
have been used to create the present object.

On success it returns 
\f(CWDW_DLV_OK\fP
and
sets the index of the string just added, in the list of include
directories for the object.
This index is then used to refer to this
string.
The index is passed back through
the pointer argument
\f(CWindex_in_directories\fP

The first successful call of this function
returns one, not zero, to be consistent with the directory indices
that \f(CWdwarf_add_file_decl_a()\fP (below) expects..
DWARF5 is a bit different.
TBD FIXME

It returns 
\f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_add_file_decl_a()"
.DS
\f(CWint dwarf_add_file_decl_a(
        Dwarf_P_Debug dbg,
        char *name,
        Dwarf_Unsigned dir_idx,
        Dwarf_Unsigned time_mod,
        Dwarf_Unsigned length,
        Dwarf_Unsigned *file_entry_count_out,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_add_file_decl_a()\fP adds the name of a source
file that contributed to the present object.
The name of the file is
specified by \f(CWname\fP (which must not be the empty string
or a null pointer, it must point to
a string with length greater than 0).

In case the name is not a fully-qualified
pathname, it is
considered prefixed with the name of the directory specified by
\f(CWdir_idx\fP
(which does not mean the 
\f(CWname\fP
is changed or physically prefixed by
this producer function, we simply describe the meaning here).
\f(CWdir_idx\fP
is the index of the directory to be
prefixed in the list builtup using 
\f(CWdwarf_add_directory_decl_a()\fP.
As specified by the DWARF spec, a
\f(CWdir_idx\fP
of zero will be
interpreted as meaning the directory of the compilation and
another index must refer to a valid directory as
FIXME

.P
\f(CWtime_mod\fP
gives the time at which the file was last modified,
and 
\f(CWlength\fP gives the length of the file in bytes.
.P
On success,
it returns
\f(CWDW_DLV_OK\fP
and returns the index of the source file in the list built up so far
through the pointer
\f(CWfile_entry_count_out\fP.
This index can then be used to
refer to this source file in calls to 
\f(CWdwarf_add_line_entry_a()\fP.

On error, it returns \f(CWDW_DLV_ERROR\fP.

.H 2 "Fast Access (aranges) Operations"
These functions operate on the .debug_aranges section.  

.H 3 "dwarf_add_arange_c()"
.DS
\f(CWint dwarf_add_arange_c(
        Dwarf_P_Debug dbg,
        Dwarf_Addr begin_address,
        Dwarf_Unsigned length,
        Dwarf_Unsigned symbol_index,
        Dwarf_Unsigned end_symbol_index,
        Dwarf_Addr     offset_from_end_symbol,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_add_arange_c()\fP adds another address range
to be added to the section containing
address range information, .debug_aranges.

If
\f(CWend_symbol_index is not zero\fP
we are using two symbols to create a length
(must be \f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP to be useful)
.sp
.in +2
\f(CWbegin_address\fP
is the offset from the symbol specified by
\f(CWsymbol_index\fP .
\f(CWoffset_from_end_symbol\fP
is the offset from the symbol specified by
\f(CWend_symbol_index\fP.
\f(CWlength\fP is ignored.
This begin-end pair will be show up in the
relocation array returned by
\f(CWdwarf_get_relocation_info() \fP
as a
\f(CWdwarf_drt_first_of_length_pair\fP
and
\f(CWdwarf_drt_second_of_length_pair\fP
pair of relocation records.
The consuming application will turn that pair into
something conceptually identical to
.sp
.nf
.in +4
 .word end_symbol + offset_from_end - \\
   ( start_symbol + begin_address)
.in -4
.fi
.sp
The reason offsets are allowed on the begin and end symbols
is to allow the caller to re-use existing labels
when the labels are available
and the corresponding offset is known  
(economizing on the number of labels in use).
The  'offset_from_end - begin_address'
will actually be in the binary stream, not the relocation
record, so the app processing the relocation array
must read that stream value into (for example)
net_offset and actually emit something like
.sp
.nf
.in +4
 .word end_symbol - start_symbol + net_offset
.in -4
.fi
.sp
.in -2

If
\f(CWend_symbol_index\fP is zero
we must be given a length
(either
\f(CWDW_DLC_STREAM_RELOCATIONS\fP
or
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP
):
.sp
.in +2
The relocatable start address of the range is
specified by \f(CWbegin_address\fP, and the length of the address
range is specified by \f(CWlength\fP.  
The relocatable symbol to be
used to relocate the start of the address range is specified by
\f(CWsymbol_index\fP, which is normally
the index of the symbol in the Elf
symbol table.
The 
\f(CWoffset_from_end_symbol\fP
is ignored.
.in -2

The function returns
\f(CWDW_DLV_OK\fP
on success and
\f(CWDW_DLV_ERROR\fP
on error.


.H 2 "DWARF5 .debug_sup section creation"
The .debug_sup section (see the DWARF5 standard)
enables symbolically linking two DWARF5 
object files together.
.sp
.H 3 "dwarf_add_debug_sup()"
This call provides all the information
that the .debug_sup section has.
.DS
\f(CWint dwarf_add_debug_sup(
    Dwarf_P_Debug dbg,
    Dwarf_Half      version,
    Dwarf_Small     is_supplementary,
    char          * filename,
    Dwarf_Unsigned  checksum_len,
    Dwarf_Small   * checksum,
    Dwarf_Error *error)\fP
.DE
On success it returns
\f(CWDW_DLV_OK\fP
and records the fields for
creating the section.
.sp
The fields are as follows.
.sp
\f(CWversion\fP
should be passed in as 2.
.sp
\f(CWfilename\fP
must be a null-terminated string.
.sp
\f(CWis_supplementary\fP
should be passed in as 0 or 1
depending on which type of
object file is involved 
(see the DWARF5 standard).
.sp
\f(CWchecksum\fP must be a byte
array of length 
\f(CWchecksum_len\fP
used to validate (by a debugger)
the use of the target object file.
.sp
\f(CWDW_DLV_NO ENTRY\fP is never returned.
.sp
\f(CWDW_DLV_ERROR\fP is returned in case
of an error, and
\f(CW*error\fP is set as usual in libdwarf.


.H 2 "Fast Access (pubnames) Operations"
These functions operate on the .debug_pubnames section.
.sp
.H 3 "dwarf_add_pubname_a()"
.DS
\f(CWint dwarf_add_pubname_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die die,
        char *pubname_name,
        Dwarf_Error *error)\fP
.DE
It returns
\f(CWDW_DLV_OK\fP
on success and
\f(CWDW_DLV_ERROR\fP
on error.

.H 2 "Fast Access (pubtypes) Operations"
These functions operate on the .debug_pubtypes section.
An SGI-defined extension.
Not part of standard DWARF.
.sp
.H 3 "dwarf_add_pubtype_a()"
.DS
\f(CWint dwarf_add_pubtype_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die die,
        char *pubname_name,
        Dwarf_Error *error)\fP
.DE
It returns
\f(CWDW_DLV_OK\fP
on success and
\f(CWDW_DLV_ERROR\fP
on error.

.H 2 "Fast Access (weak names) Operations"
These functions operate on the .debug_weaknames section.
An SGI-defined extension.
Not part of standard DWARF.

.H 3 "dwarf_add_weakname_a()"
.DS
\f(CWint dwarf_add_weakname_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die die,
        char *weak_name,
        Dwarf_Error *error)\fP
.DE
It returns
\f(CWDW_DLV_OK\fP
on success and
\f(CWDW_DLV_ERROR\fP
on error.

.H 2 "Static Function Names Operations"
The .debug_funcnames section contains the names of static function 
names defined in the object, and also the offsets of the \f(CWDIE\fPs
that represent the definitions of the functions in the .debug_info 
section.
An SGI-defined extension.
Not part of standard DWARF.

.H 3 "dwarf_add_funcname_a()"
.DS
\f(CWint dwarf_add_funcname_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die die,
        char *func_name,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_add_funcname_a()\fP adds the name of a static
function specified by 
\f(CWfunc_name\fP to the section containing the
names of static functions defined in the object represented by \f(CWdbg\fP.
The 
\f(CWDIE\fP that represents the definition of the function is
specified by 
\f(CWdie\fP.
.P
It returns 
\f(CWDW_DLV_OK\fP on success.
.P
It returns 
\f(CWDW_DLV_ERROR\fP on error.

.H 2 "File-scope User-defined Type Names Operations"
The .debug_typenames section contains the names of file-scope
user-defined types in the given object, and also the offsets 
of the \f(CWDIE\fPs that represent the definitions of the types 
in the .debug_info section.
An SGI-defined extension.
Not part of standard DWARF.

.H 3 "dwarf_add_typename_a()"
.DS
\f(CWint dwarf_add_typename_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die die,
        char *type_name,
        Dwarf_Error *error)\fP
.DE
This the same as
\f(CWdwarf_add_typename()\fP
except that on success this returns
\f(CWDW_DLV_OK\fP
and on failure this returns
\f(CWDW_DLV_ERROR\fP.

.H 2 "File-scope Static Variable Names Operations"
The .debug_varnames section contains the names of file-scope static
variables in the given object, and also the offsets of the 
\f(CWDIE\fPs
that represent the definition of the variables in the .debug_info
section.
An SGI-defined section.

.H 3 "dwarf_add_varname_a()"
.DS
\f(CWint dwarf_add_varname_a(
        Dwarf_P_Debug dbg,
        Dwarf_P_Die die,
        char *var_name,
        Dwarf_Error *error)\fP
.DE
This the same as
\f(CWdwarf_add_varname()\fP
except that on success this returns
\f(CWDW_DLV_OK\fP
and on failure this returns
\f(CWDW_DLV_ERROR\fP.

.H 2 "Macro Information Creation"
All strings passed in by the caller are copied by these
functions, so the space in which the caller provides the strings
may be ephemeral (on the stack, or immediately reused or whatever)
without this causing any difficulty.

.H 3 "dwarf_def_macro()"
.DS
\f(CWint dwarf_def_macro(Dwarf_P_Debug dbg,
	Dwarf_Unsigned lineno,
	char *name
	char *value,
        Dwarf_Error *error);\fP
.DE
Adds a macro definition.
The \f(CWname\fP argument should include the parentheses
and parameter names if this is a function-like macro.
Neither string should contain extraneous whitespace.
\f(CWdwarf_def_macro()\fP adds the mandated space after the
name and before the value  in the
output DWARF section(but does not change the
strings pointed to by the arguments).
If this is a definition before any files are read,
\f(CWlineno\fP should be 0.
Returns \f(CWDW_DLV_ERROR\fP
and sets \f(CWerror\fP
if there is an error.
Returns \f(CWDW_DLV_OK\fP if the call was successful.


.H 3 "dwarf_undef_macro()"
.DS
\f(CWint dwarf_undef_macro(Dwarf_P_Debug dbg,
	Dwarf_Unsigned lineno,
	char *name,
        Dwarf_Error *error);\fP
.DE
Adds a macro un-definition note.
If this is a definition before any files are read,
\f(CWlineno\fP should be 0.
Returns \f(CWDW_DLV_ERROR\fP
and sets \f(CWerror\fP
if there is an error.
Returns \f(CWDW_DLV_OK\fP if the call was successful.


.H 3 "dwarf_start_macro_file()"
.DS
\f(CWint dwarf_start_macro_file(Dwarf_P_Debug dbg,
	Dwarf_Unsigned lineno,
        Dwarf_Unsigned fileindex,
        Dwarf_Error *error);\fP
.DE
\f(CWfileindex\fP is an index in the .debug_line header: 
the index of
the file name.
See the function \f(CWdwarf_add_file_decl()\fP.
The \f(CWlineno\fP should be 0 if this file is
the file of the compilation unit source itself
(which, of course, is not a #include in any
file).
Returns \f(CWDW_DLV_ERROR\fP
and sets \f(CWerror\fP
if there is an error.
Returns \f(CWDW_DLV_OK\fP if the call was successful.


.H 3 "dwarf_end_macro_file()"
.DS
\f(CWint dwarf_end_macro_file(Dwarf_P_Debug dbg,
        Dwarf_Error *error);\fP
.DE
Returns \f(CWDW_DLV_ERROR\fP
and sets \f(CWerror\fP
if there is an error.
Returns \f(CWDW_DLV_OK\fP if the call was successful.

.H 3 "dwarf_vendor_ext()"
.DS
\f(CWint dwarf_vendor_ext(Dwarf_P_Debug dbg,
    Dwarf_Unsigned constant,
    char *         string,
    Dwarf_Error*   error); \fP
.DE
The meaning of the \f(CWconstant\fP and the\f(CWstring\fP
in the macro info section
are undefined by DWARF itself, but the string must be
an ordinary null terminated string.
This call is not an extension to DWARF. 
It simply enables storing
macro information as specified in the DWARF document.
Returns \f(CWDW_DLV_ERROR\fP
and sets \f(CWerror\fP
if there is an error.
Returns \f(CWDW_DLV_OK\fP if the call was successful.


.H 2 "Low Level (.debug_frame) operations"
These functions operate on the .debug_frame section.  Refer to 
\f(CWlibdwarf.h\fP for the register names 
and register assignment 
mapping.  
Both of these are necessarily machine dependent.

.H 3 "dwarf_new_fde_a()"
.DS
\f(CWint dwarf_new_fde_a(
        Dwarf_P_Debug dbg, 
        Dwarf_P_Fde *fde_out,
        Dwarf_Error *error)\fP
.DE
On success the function 
\f(CWdwarf_new_fde_a()\fP returns
\f(CWDW_DLV_OK\fP
and returns a pointer to the fde
through
\f(CWfde_out\fP.
The
descriptor should be used to build a complete 
\f(CWFDE\fP.  
Subsequent calls to routines that build up the 
\f(CWFDE\fP should use
the same 
\f(CWDwarf_P_Fde\fP descriptor.
.P
It returns
\f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_add_frame_cie_a()"
.DS
\f(CWint dwarf_add_frame_cie_a(
        Dwarf_P_Debug dbg,
        char *augmenter,
        Dwarf_Small code_align,
        Dwarf_Small data_align,
        Dwarf_Small ret_addr_reg,
        Dwarf_Ptr init_bytes,
        Dwarf_Unsigned init_bytes_len,
        Dwarf_Unsigned *cie_index_out,
        Dwarf_Error *error);\fP
.DE
On success
The function
\f(CWdwarf_add_frame_cie_a()\fP
returns 
\f(CWDW_DLV_OK\fP,
creates a 
\f(CWCIE\fP,
and returns an index to it
through the pointer
\f(CWcie_index_out\fP.
.P
\f(CWCIE\fPs are used by 
\f(CWFDE\fPs to setup
initial values for frames. 
The augmentation string for the 
\f(CWCIE\fP
is specified by 
\f(CWaugmenter\fP. 
The code alignment factor,
data alignment factor, and the return address register for the
\f(CWCIE\fP are specified by 
\f(CWcode_align\fP, 
\f(CWdata_align\fP,
and 
\f(CWret_addr_reg\fP respectively.
\f(CWinit_bytes\fP points
to the bytes that represent the instructions for the 
\f(CWCIE\fP
being created, and 
\f(CWinit_bytes_len\fP specifies the number
of bytes of instructions.
.P
There is no convenient way to generate the  
\f(CWinit_bytes\fP
stream.
One just
has to calculate it by hand or separately
generate something with the
correct sequence and use dwarfdump -v and readelf (or objdump)
and some
kind of hex dumper to see the bytes.
This is a serious inconvenience!

On error it returns 
\f(CWDW_DLV_ERROR\fP.


.H 3 "dwarf_add_frame_fde_c()"
.DS
\f(CWint dwarf_add_frame_fde_c(
    Dwarf_P_Debug dbg,
    Dwarf_P_Fde fde,
    Dwarf_P_Die die,
    Dwarf_Unsigned cie,
    Dwarf_Addr virt_addr,
    Dwarf_Unsigned  code_len,
    Dwarf_Unsigned sym_idx,
    Dwarf_Unsigned sym_idx_of_end,
    Dwarf_Addr     offset_from_end_sym,
    Dwarf_Unsigned *index_to_fde,
    Dwarf_Error* error)\fP
.DE

This function is like 
\f(CWdwarf_add_frame_fde()\fP 
except that 
\f(CWdwarf_add_frame_fde_c()\fP 
has new arguments to allow use
with
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP
and a new argument to return the fde index..

The function \f(CWdwarf_add_frame_fde_c()\fP 
adds the 
\f(CWFDE\fP
specified by \f(CWfde\fP to the list of 
\f(CWFDE\fPs for the
object represented by the given 
\f(CWdbg\fP.  

\f(CWdie\fP specifies
the 
\f(CWDIE\fP that represents the function
whose frame information
is specified by the given 
\f(CWfde\fP.  
If the MIPS/IRIX specific DW_AT_MIPS_fde attribute is not
needed  in .debug_info pass in 0 as the \f(CWdie\fP argument.

\f(CWcie\fP specifies the
index of the 
\f(CWCIE\fP that should be used to setup the initial
conditions for the given frame.  
\f(CWvirt_addr\fP represents the
relocatable address at which the code
for the given function begins,
and 
\f(CWsym_idx\fP gives the index of the relocatable symbol to
be used to relocate this address (\f(CWvirt_addr\fP that is).
\f(CWcode_len\fP specifies the 
size in bytes of the machine instructions
for the given function.

If \f(CWsym_idx_of_end\fP is zero 
(may  be 
\f(CWDW_DLC_STREAM_RELOCATIONS\fP 
or
\f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP 
):
.sp
.in +2
\f(CWvirt_addr\fP represents the
relocatable address at which the code for the given function begins,
and \f(CWsym_idx\fP gives the index of the relocatable symbol to
be used to relocate this address (\f(CWvirt_addr\fP that is).
\f(CWcode_len\fP 
specifies the size in bytes of the machine instructions
for the given function.
\f(CWsym_idx_of_end\fP
and
\f(CWoffset_from_end_sym\fP
are unused.
.in -2
.sp


If \f(CWsym_idx_of_end\fP is non-zero 
(must be \f(CWDW_DLC_SYMBOLIC_RELOCATIONS\fP to be useful):
.sp
.in +2
\f(CWvirt_addr\fP
is the offset from the symbol specified by
\f(CWsym_idx\fP .
\f(CWoffset_from_end_sym\fP
is the offset from the symbol specified by
\f(CWsym_idx_of_end\fP.
\f(CWcode_len\fP is ignored.
This begin-end pair will be show up in the
relocation array returned by
\f(CWdwarf_get_relocation_info() \fP
as a
\f(CWdwarf_drt_first_of_length_pair\fP
and
\f(CWdwarf_drt_second_of_length_pair\fP
pair of relocation records.
The consuming application will turn that pair into
something conceptually identical to
.sp
.nf
.in +4
 .word end_symbol + begin - \\
   ( start_symbol + offset_from_end)
.in -4
.fi
.sp
The reason offsets are allowed on the begin and end symbols
is to allow the caller to re-use existing labels
when the labels are available
and the corresponding offset is known
(economizing on the number of labels in use).
The  'offset_from_end - begin_address'
will actually be in the binary stream, not the relocation
record, so the app processing the relocation array
must read that stream value into (for example)
net_offset and actually emit something like
.sp
.nf
.in +4
 .word end_symbol - start_symbol + net_offset
.in -4
.fi
.sp
.in -2

On success it
returns
\f(CWDW_DLV_OK\fP
and returns index to the given \f(CWfde\fP
through the pointer
\f(CWindex_to_fde\fP.

On error, it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_add_frame_info_c()"
.DS
\f(CWint dwarf_add_frame_info_c(
        Dwarf_P_Debug   dbg,
        Dwarf_P_Fde     fde,    
        Dwarf_P_Die     die,    
        Dwarf_Unsigned  cie,
        Dwarf_Addr      virt_addr,
        Dwarf_Unsigned  code_len,
        Dwarf_Unsigned  sym_idx,
        Dwarf_Unsigned  end_symbol_index,
        Dwarf_Addr      offset_from_end_symbol,
        Dwarf_Signed    offset_into_exception_tables,
        Dwarf_Unsigned  exception_table_symbol,
        Dwarf_Unsigned *index_to_fde,
        Dwarf_Error*    error)\fP
.DE
.P
On success 
The function 
\f(CWdwarf_add_frame_fde_c()\fP
returns 
\f(CWDW_DLV_OK\fP,
adds the 
\f(CWFDE\fP
specified by 
\f(CWfde\fP to the list of 
\f(CWFDE\fPs for the 
object represented by the given 
\f(CWdbg\fP, and.
passes the index of the fde back through 
the pointer 
\f(CWindex_to_fde\fP

On failure it returns 
\f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_fde_cfa_offset_a()"
.DS
\f(CWint dwarf_fde_cfa_offset_a( Dwarf_P_Fde fde,
        Dwarf_Unsigned reg,
        Dwarf_Signed offset,
        Dwarf_Error *error)\fP
.DE
.P
The function 
\f(CWdwarf_fde_cfa_offset()\fP appends a 
\f(CWDW_CFA_offset\fP
operation to the 
\f(CWFDE\fP, specified by 
\f(CWfde\fP,  being constructed.
The first operand of the 
\f(CWDW_CFA_offset\fP operation is specified by
\f(CWreg\fP.  
The register specified should not exceed 6 bits.  
The second
operand of the 
\f(CWDW_CFA_offset\fP operation is specified by 
\f(CWoffset\fP.
.P
It returns \f(CWDW_DLV_OK\fP on success.
.P
It returns \f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_add_fde_inst_a()"
.DS
\f(CWint dwarf_add_fde_inst_a(
        Dwarf_P_Fde fde,
        Dwarf_Small op,
        Dwarf_Unsigned val1,
        Dwarf_Unsigned val2,
        Dwarf_Error *error)\fP
.DE
The function 
\f(CWdwarf_add_fde_inst()\fP adds the operation specified
by 
\f(CWop\fP to the 
\f(CWFDE\fP specified by 
\f(CWfde\fP.  Up to two
operands can be specified in 
\f(CWval1\fP, and 
\f(CWval2\fP.  Based on
the operand specified 
\f(CWLibdwarf\fP decides how many operands are
meaningful for the operand.
It also converts the operands to the
appropriate datatypes (they are passed to 
\f(CWdwarf_add_fde_inst\fP
as 
\f(CWDwarf_Unsigned\fP).
.P
It returns
\f(CWDW_DLV_OK\fP on success.

It returns
\f(CWDW_DLV_ERROR\fP
on error.

.H 3 "dwarf_insert_fde_inst_bytes()"
.DS
\f(CWint dwarf_insert_fde_inst_bytes(
        Dwarf_P_Debug dbg,
        Dwarf_P_Fde  fde,
        Dwarf_Unsigned len,
        Dwarf_Ptr ibytes,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_insert_fde_inst_bytes()\fP inserts
the byte array (pointed at by \f(CWibytes\fP and of length \f(CWlen\fP)
of frame instructions into the fde \f(CWfde\fP.
It is incompatible with \f(CWdwarf_add_fde_inst()\fP, do not use
both functions on any given Dwarf_P_Debug.
At present it may only be called once on a given \f(CWfde\fP.
The \f(CWlen\fP bytes \f(CWibytes\fP may be constructed in any way, but
the assumption is they were copied from an object file
such as is returned by the libdwarf consumer function
\f(CWdwarf_get_fde_instr_bytes\fP.

It returns \f(CWDW_DLV_OK\fP on success, and \f(CWDW_DLV_ERROR\fP
on error.


.S
.TC 1 1 4
.CS
