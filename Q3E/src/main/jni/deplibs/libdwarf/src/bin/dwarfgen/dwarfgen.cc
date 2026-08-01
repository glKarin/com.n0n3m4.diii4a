/*
  Copyright (C) 2010-2019 David Anderson.  All rights reserved.

  Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that the
  following conditions are met:

  * Redistributions of source code must retain the above
  copyright notice, this list of conditions and the following
  disclaimer.

  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials
  provided with the distribution.

  * Neither the name of the example nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior written
  permission.

  THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David
  Anderson BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
// dwarfgen.cc
//
// Using some information source, create a tree of dwarf
// information (speaking of a DIE tree).
// Turn the die tree into dwarfdata using libdwarf producer
// and write the resulting data in an object file.
// It is a bit inconsistent in error handling just to
// demonstrate the various possibilities using the producer
// library.
//
//  dwarfgen [-t def|obj|txt] [-o outpath] [-c cunum]  path

//  where -t means what sort of input to read
//         def means predefined (no input is read, the output
//         is based on some canned setups built into dwarfgen).
//         'path' is ignored in this case. This is the default.
//
//         obj means 'path' is required, it is the object file to
//             read (the dwarf sections are duplicated
//             in the output file)
//
//         txt means 'path' is required, path must contain plain text
//             (in a form rather like output by dwarfdump)
//             that defines the dwarf that is to be output.
//
//  where  -o means specify the pathname of the output object. If not
//         supplied testout.o is used as the default output path.
//  where -c supplies a CU number of the obj input to output
//         because the dwarf producer wants just one CU.
//         Default is -1 which won't match anything.

#include "config.h"

/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h> /* for exit() */
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstring> // For memcpy
#include <list>
#include <map>
#include <vector>
#include <string.h> /* for strchr etc */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>  /* For open() S_IRUSR off_t */
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>  /* For open() S_IRUSR etc */
#endif /* HAVE_SYS_STAT_H */
#include <fcntl.h> //open
#include "general.h"
#include "dg_getopt.h"
#include "strtabdata.h"
#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarfp.h"
#include "libdwarf_private.h"
#include "dwarf_elfstructs.h"
#include "dwarf_elf_defines.h"
#include "irepresentation.h"
#include "ireptodbg.h"
#include "createirepfrombinary.h"
#ifdef _WIN32
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */
#include <io.h>
#endif /* _WIN32 */

#ifdef _WIN32
#ifndef O_RDONLY
/*  This is for a Windows environment */
# define O_RDONLY _O_RDONLY
#endif
#ifdef _O_BINARY
/*  This is for a Windows environment */
#define O_BINARY _O_BINARY
#endif
#else /* !_WIN32 */
# ifndef O_BINARY
# define O_BINARY 0  /* So it does nothing in Linux/Unix */
# endif
#endif /* !_WIN32 */

/*  These are for a Windows environment */
#ifdef _WIN32
#ifdef _O_WRONLY
#define O_WRONLY _O_WRONLY
#endif
#ifdef _O_CREAT
#define O_CREAT _O_CREAT
#endif
#ifdef _O_TRUNC
#define O_TRUNC _O_TRUNC
#endif
#ifdef _S_IREAD
#define S_IREAD _S_IREAD
#endif
#ifdef _S_IWRITE
#define S_IWRITE _S_IWRITE
#endif
/* open modes S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH */
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif // S_IRUSR
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IRGRP
#define S_IRGRP 0
#endif
#ifndef S_IROTH
#define S_IROTH 0
#endif
#endif /* _WIN32 */

using std::string;
using std::cout;
using std::endl;
using std::vector;
extern "C" {
static int CallbackFunc(
    const char* name,
    int                 size,
    Dwarf_Unsigned      type,
    Dwarf_Unsigned      flags,
    Dwarf_Unsigned      link,
    Dwarf_Unsigned      info,
    Dwarf_Unsigned*     sect_name_symbol_index,
    void *              user_data,
    int*                error);
}
// End extern "C"

static void create_debug_sup_content(Dwarf_P_Debug dbg);
// FIXME. This is incomplete. See FIXME just below here.
#ifdef WORDS_BIGENDIAN
static void
_dwarf_memcpy_swap_bytes(void *s1, const void *s2, unsigned long len)
{
    unsigned char *targ = (unsigned char *) s1;
    const unsigned char *src = (const unsigned char *) s2;

    if (len == 4) {
        targ[3] = src[0];
        targ[2] = src[1];
        targ[1] = src[2];
        targ[0] = src[3];
    } else if (len == 8) {
        targ[7] = src[0];
        targ[6] = src[1];
        targ[5] = src[2];
        targ[4] = src[3];
        targ[3] = src[4];
        targ[2] = src[5];
        targ[1] = src[6];
        targ[0] = src[7];
    } else if (len == 2) {
        targ[1] = src[0];
        targ[0] = src[1];
    }
/* should NOT get below here: is not the intended use */
    else if (len == 1) {
        targ[0] = src[0];
    } else {
        memcpy(s1, s2, (size_t)len);
    }
    return;
}
#endif /* WORDS_BIGENDIAN */

//  There are issues 32/64 here, and endianness.
//  We are asserting here in dwarfgen that the target
//  object created is to be DW_DLC_TARGET_LITTLEENDIAN.
//  FIXME. not general.
//  op and ol aread MUST NOT overlap.
//  ip and il should be 2,4,or 8. Nothing else.
//  These could perfectly well be functions.
#ifdef WORDS_BIGENDIAN
#define ASNX(op,ol,ip,il)           \
    do {                            \
        if (ol > il) {              \
            Dwarf_Unsigned sbyte = 0;\
            memset(op,0,ol);        \
            sbyte = ol - il;        \
            const void * ipi =      \
                (const void *)ip;   \
            _dwarf_memcpy_swap_bytes(((char *)(op))+sbyte,\
                ipi,il);\
        } else {                    \
            Dwarf_Unsigned sbyte = 0;\
            sbyte = il - ol;        \
            const void * ipi =      \
                ((Dwarf_Small*)ip) +sbyte; \
            _dwarf_memcpy_swap_bytes((char *)(op),      \
                (const void *)ipi,  \
                ol);               \
        }                           \
    } while (0)
#else // LITTLEENDIAN
#define ASNX(op,ol,ip,il)           \
    do {                            \
        if (ol > il) {              \
            const void *ipi =       \
                (const void *)ip;   \
            memset(op,0,ol);        \
            memcpy(((char *)(op)),  \
                ipi,il);             \
        } else {                    \
            const void *ipi =       \
                (const void *)ip;   \
            memcpy((char *)(op),    \
                ipi,ol);            \
        }                           \
    } while (0)
#endif // ENDIANNESS

static void write_object_file(Dwarf_P_Debug dbg,
    IRepresentation &irep,
    unsigned machine,
    unsigned endian,
    unsigned long dwbitflags,
    void * user_data);

static void           create_initial_section(void);
static void           create_text_section(void);
static Dwarf_Unsigned create_namestr_section(void);
static void           write_generated_dbg(Dwarf_P_Debug dbg,
    IRepresentation &irep);

static string outfile("testout.o");
static string infile;
static enum  WhichInputSource { OptNone, OptReadText,
    OptReadBin,OptPredefined}
    whichinput(OptPredefined);

//  Use a generic call to open the file
//  (issues with Windows)
int open_a_file(const char * name);
void close_a_file(int f);
Dwarf_Unsigned write_to_object(void);

// This is a global so thet CallbackFunc can get to it
// If we used the dwarf_producer_init_c() user_data pointer
// creatively we would not need a global.
static IRepresentation Irep;

static strtabdata secstrtab;

CmdOptions cmdoptions = {
    false, //transformHighpcToConst
    DW_FORM_string, // defaultInfoStringForm
    false, //showrelocdetails
    false, //adddata16
    false, //addimplicitconst
    false, //addframeadvanceloc
    false, //addSUNfuncoffsets
    false, //add_debug_sup
    false, //addskipbranch
    false //addlanguageversion
};

// loff_t is signed for some reason (strange)
// but we make offsets unsigned.
#define LOFFTODWUNS(x)  ( (Dwarf_Unsigned)(x))

class Writer {
public:
    int fd_;
    string name_;
    Dwarf_Unsigned curoffset_;
    Writer() {
        fd_ = -1;
        curoffset_ =0;
    };

    void wwrite(Dwarf_Unsigned offset,
        Dwarf_Unsigned length,
        const void *bytes) {
        if (fd_ == -1) {
            cout << "dwarfgen: write with fd_ -1" << endl;
            exit(1);
        }
        if (curoffset_ != offset) {
            off_t v = lseek(fd_,offset,SEEK_SET);
            if (v == (off_t)-1) {
                cout << "dwarfgen: fseek to " << offset<<
                    " fails " << endl;
                exit(1);
            }
            curoffset_ = v;
        }
        ssize_t wrote = write(fd_,bytes,(size_t)length);
        if (wrote == -1) {
            cout << "dwarfgen: write length " << length<<
                "fails " << endl;
            exit(1);
        }
        curoffset_ += length;
    }
    void closeFile() {
        if (fd_ != -1) {
            close(fd_);
            fd_ = -1;
        }
        curoffset_ = 0;
    }
    ~Writer() {
        closeFile();
    };
    void openFile(string & name) {
        name_ = name;
        fd_ =  open(name_.c_str(), O_CREAT|O_WRONLY | O_BINARY,
            00666);
        if (fd_ ==-1) {
            cout << "dwarfgen: open " << name_ << " fails" << endl;
            exit(1);
        }
        curoffset_ =0;
    }
};

class ByteBlob {
public:
    ByteBlob() {bytes_ = 0; len_=0;};
    ByteBlob(unsigned char *bytes,size_t len) {
        bytes_ = bytes;
        len_ = len;
    };
    void setBlob(unsigned char *bytes,size_t len) {
        if (len_) {
            cout << "dwarfgen: Duplicate ByteBlob setting " <<
                "for length "<< len_ << " to length"
                <<  len << endl;
            exit(1);
        }
        bytes_ = bytes;
        len_ = len;
    };
    unsigned char *bytes_;
    Dwarf_Unsigned len_;
    ~ByteBlob() { bytes_ = 0; len_= 0; };
};

/*  See the Elf ABI for further definitions of these fields. */
//  By definition, this outputs data to file offset zero.
class ElfHeaderForDwarf {
public:
    unsigned char e_ident_[EI_NIDENT];
    Dwarf_Unsigned e_type_;
    Dwarf_Unsigned e_machine_;
    Dwarf_Unsigned e_version_;
    Dwarf_Unsigned e_entry_;
    Dwarf_Unsigned e_phoff_;
    Dwarf_Unsigned e_shoff_;
    Dwarf_Unsigned e_flags_;
    Dwarf_Unsigned e_ehsize_;
    Dwarf_Unsigned e_phentsize_;
    Dwarf_Unsigned e_phnum_;
    Dwarf_Unsigned e_shentsize_;
    Dwarf_Unsigned e_shnum_;
    Dwarf_Unsigned e_shstrndx_;
    bool           e_dwarf_32bit_;
    dw_elf32_ehdr e_32_;
    dw_elf64_ehdr e_64_;
    unsigned  char  e_ptrbytesize_;
    Dwarf_Unsigned  e_hdrlen_;
    bool elf_is_64bit() { return e_ptrbytesize_ == 8;};
    bool elf_is_32bit() { return e_ptrbytesize_ == 4;};
    bool dwarf_is_64bit() {return !e_dwarf_32bit_;};
    bool dwarf_is_32bit() {return e_dwarf_32bit_;};
    ElfHeaderForDwarf() {
        memset(&e_32_,0,sizeof(e_32_));
        memset(&e_64_,0,sizeof(e_64_));
        memset(e_ident_,0,sizeof(e_ident_));
        e_type_= 0; e_machine_= 0;; e_version_= 0;; e_entry_= 0;
        e_phoff_= 0; e_shoff_= 0; e_flags_= 0; e_ehsize_= 0;
        e_phentsize_= 0; e_phnum_= 0; e_shentsize_= 0; e_shnum_= 0;
        e_shstrndx_ = 0;
        e_dwarf_32bit_ = true;
        e_ptrbytesize_=0; e_hdrlen_ = 0;
    };
};

class SectionForDwarf {
public:
    SectionForDwarf():sh_name_(0),
        section_name_symidx_(0),
        sh_type_(0),
        sh_addr_(0),
        sh_offset_(0),
        sh_flags_(0),
        sh_size_(0),
        sh_link_(0), sh_info_(0),
        sh_addralign_(1),
        sh_entsize_(0),
        elf_sect_index_(0)
        {
            memset(&shdr64_,0,sizeof(shdr64_));
            memset(&shdr32_,0,sizeof(shdr32_));
        };
    SectionForDwarf(const string name,
        Dwarf_Unsigned type,
        Dwarf_Unsigned flags, Dwarf_Unsigned link,
        Dwarf_Unsigned info):
        name_(name),
        sh_type_(type),
        sh_addr_(0),
        sh_offset_(0),
        sh_flags_(flags),
        sh_size_(0),
        sh_link_(link),
        sh_info_(info),
        sh_addralign_(1),
        sh_entsize_(0),
        elf_sect_index_(0)
        {
            memset(&shdr64_,0,sizeof(shdr64_));
            memset(&shdr32_,0,sizeof(shdr32_));
            // Now create section name string section .
            sh_name_ = secstrtab.addString(name.c_str());
            ElfSymbols& es = Irep.getElfSymbols();
            // Now creat a symbol for the section name.
            // (which has its own string table)
            section_name_symidx_ = es.addElfSymbol(0,name);
        };
    ~SectionForDwarf() { };
    string    name_;
    Dwarf_Unsigned sh_name_; /* section_name_itself */
    ElfSymIndex    section_name_symidx_;
    Dwarf_Unsigned sh_type_;
    Dwarf_Unsigned sh_addr_;
    Dwarf_Unsigned sh_offset_;
    Dwarf_Unsigned sh_flags_;
    Dwarf_Unsigned sh_size_;
    Dwarf_Unsigned sh_link_;
    Dwarf_Unsigned sh_info_;
    Dwarf_Unsigned sh_addralign_;
    Dwarf_Unsigned sh_entsize_;

    /*  type: SHT_REL, RELA: Section header index of the section
        relocation applies to.
        SHT_SYMTAB: Section header index of the
        associated string table. */
    unsigned int elf_sect_index_;
    dw_elf64_shdr shdr64_;
    dw_elf32_shdr shdr32_;
    // Blobs are not tiny, libdwarfp aggregates many blocks
    vector<ByteBlob> sectioncontent_;   // All (but not header)
    // content size is sh_size_ (over all blobs)

    void add_section_content(unsigned char *data,
        Dwarf_Unsigned datalen) {
        ByteBlob x(data,datalen);
        sectioncontent_.push_back(x);
        sh_size_ += datalen;
    };
    Dwarf_Unsigned getSectionNameSymidx() {
        return section_name_symidx_.getSymIndex(); };
    void setSectIndex(unsigned v) { elf_sect_index_ = v; };
    Dwarf_Unsigned getSectIndex() const { return elf_sect_index_;};
}; // SectionForDwarf

// The elf header, with native integers
ElfHeaderForDwarf       dwelfheader;

// Data for section header headers is kept in native integers
// Eventually contains the section headers
// and section content, including Elf Section zero.
// The vector is indexable by ElfSectIndex.getSectIndex()
vector<SectionForDwarf> dwsectab;

Writer                  dwwriter;

#if 0
static SectionForDwarf & FindMySection(
    const ElfSectIndex & elf_section_index)
{
    for (unsigned i =0; i < dwsectab.size(); ++i) {
        if (elf_section_index.getSectIndex() !=
            dwsectab[i].getSectIndex().getSectIndex()) {
            continue;
        }
        return dwsectab[i];
    }
    cout << "dwarfgen: Unable to find my dw sec data "
        "for elf section " <<
        elf_section_index.getSectIndex() << endl;
    exit(1);
}
#endif

static int
FindMySectionNum(const ElfSectIndex & elf_section_index)
{
    for (unsigned i =0; i < dwsectab.size(); ++i) {
        if (elf_section_index.getSectIndex() !=
            dwsectab[i].getSectIndex()) {
            continue;
        }
        return i;
    }
    cout << "dwarfgen: Unable to find my dw sec index "
        "for elf section " <<
        elf_section_index.getSectIndex() << endl;
    exit(1);
}

void
create_initial_section(void)
{
    unsigned sectindex = dwsectab.size();
    dwsectab.push_back(SectionForDwarf());
    cout << "New Elf section: " << "" <<
        " Type=" << 0 << " Flags=" << 0 <<
        " Elf secnum=" << sectindex <<
        " link section=" << 0 <<
        " info="<< 0 << endl;
}

static Dwarf_Unsigned
create_namestr_section(void)
{
    unsigned long sectindex = dwsectab.size();
    string s(".shstrtab");
    SectionForDwarf ds = SectionForDwarf(
        s,
        DWARF_T_BYTE,
        SHT_STRTAB,
        0,0);
    ds.setSectIndex(sectindex);
    ds.add_section_content(
        static_cast<unsigned char *>(secstrtab.exposedata()),
        secstrtab.exposelen());
    dwelfheader.e_shstrndx_ = sectindex;
    dwsectab.push_back(ds);
    cout << "New Elf section: " << s <<
        " Type=" << DWARF_T_BYTE << " Flags=" << SHT_STRTAB <<
        " Elf secnum=" << sectindex <<
        " link section=" << 0 <<
        " info="<< 0 << endl;
    return sectindex;
}

// This functional interface is defined by libdwarf.
// Please see the comments in libdwarf2p.1.pdf
// (libdwarf2p.1.mm)  on this callback interface.
// Returns (to libdwarf) an Elf section number, so
// since 0 is always empty and dwarfgen sets 1 to be a fake
// text section on the first call this returns 2, second 3, etc.
static int CallbackFunc(
    const char* name,
    int                 size,
    Dwarf_Unsigned      type,
    Dwarf_Unsigned      flags,
    Dwarf_Unsigned      link,
    Dwarf_Unsigned      info,
    Dwarf_Unsigned*     sect_name_symbol_index,
    void *              user_data,
    int*                error)
{
    // Create an elf section.
    // If the data is relocations, we suppress the generation
    // of a section when we intend to do the relocations
    // ourself (fine for dwarfgen but would
    // be really surprising for a normal compiler
    // back end using the producer code).
    (void)error;
    (void)size;
    (void)user_data;

    if (0 == strncmp(name,".rel",4))  {
        // It is relocation, create no section!
        return 0;
    }
    unsigned new_sect_index = dwsectab.size();
    SectionForDwarf ds(name,type,flags,link,info) ;
    ds.setSectIndex(new_sect_index);
    cout << "New Elf section: " << name <<
        " Type=" <<type << " Flags=" << flags <<
        " Elf secnum=" << new_sect_index <<
        " link section=" << link <<
        " info="<< info << endl;

    // In Elf, each section gets an elf symbol table entry.
    // So that relocations have an address to refer to.
    // You will create the Elf symbol table, so you have to tell
    // libdwarf the index to put into relocation records for the
    // section newly defined here.
    *sect_name_symbol_index = ds.getSectionNameSymidx();

    // Do all the data creation before pushing
    // (copying) ds onto dwsectab!
    dwsectab.push_back(ds);
    // The number returned is elf section which
    // is the same as dwsectab vector index;
    return ds.getSectIndex();
}

static void
setinput(enum  WhichInputSource *src,
    const string &type,
    bool *pathreq)
{
    if (type == "txt") {
        *src = OptReadText;
        *pathreq = true;
        return;
    } else if (type == "obj") {
        *src = OptReadBin;
        *pathreq = true;
        return;
    } else if (type == "def") {
        *src = OptPredefined;
        *pathreq = false;
        return;
    }
    cout << "dwarfgen: Giving up, only txt obj or def "
        "accepted after -t" << endl;
    exit(1);
}

int
main(int argc, char **argv)
{
    try {
        int opt;
        bool pathrequired(false);
        long cu_of_input_we_output = -1;
        bool force_empty_dnames = false;

        // Overriding macro constants from pro_line.h
        // so we can choose at runtime
        // and avoid modifying header files.
        // This is the original set of constants
        // used for testing.
        // see also output_v4_test (--output-v4-test
        // longopt).
        const char *dwarf_extras ="opcode_base=13,"
            "minimum_instruction_length=1,"
            "line_base=-1,"
            "line_range=4";
        int endian =  DW_DLC_TARGET_LITTLEENDIAN;
        const char *dwarf_version = "V2";
        const char * isa_name = "x86";
        unsigned long ptrsizeflagbit = DW_DLC_POINTER32;
        unsigned long dwarfoffsetsizeflagbit = DW_DLC_OFFSET32;
        /*  DW_DLC_ELF_OFFSET_SIZE 32/64 winds up determining
            the ELFCLASS 32/64 in write_object_file() */
        unsigned long elfoffsetsizeflagbit =
            DW_DLC_ELF_OFFSET_SIZE_32;
        unsigned machine = EM_386; /* from elf.h */
        int output_v4_test = 0;

        unsigned global_elfclass = 0;

        int longindex;
        static struct dwoption longopts[] = {
            {"adddata16",dwno_argument,0,1000},
            {"force-empty-dnames",dwno_argument,0,1001},
            {"add-implicit-const",dwno_argument,0,1002},
            {"add-frame-advance-loc",dwno_argument,0,1003},
            {"add-sun-func-offsets",dwno_argument,0,1004},
            {"add-debug-sup",dwno_argument,0,1006},
            {"output-pointer-size",dwrequired_argument,0,'p'},
            {"output-offset-size",dwrequired_argument,0,'f'},
            {"output-dwarf-version",dwrequired_argument,0,'v'},
            {"output-v4-test",dwno_argument,0,1005},
            {"default-form-strp",dwno_argument,0,'s'},
            {"show-reloc-details",dwno_argument,0,'r'},
            {"high-pc-as-const",dwno_argument,0,'h'},
            {"add-skip-branch-ops",dwno_argument,0,1007},
            {"add-language-version",dwno_argument,0,1008},
            {0,0,0,0},
        };
        // -p is pointer size
        // -f is offset size, overriding the above
        // -v is version number for dwarf output

        while((opt=dwgetopt_long(argc,argv,
            "o:t:c:hsrv:p:f:",
            longopts,&longindex)) != -1) {
            switch(opt) {
            case 1000:
                if (longindex == 0) {
                    // To test adding the DWARF5
                    // DW_FORM_data16
                    // libdwarf reading is thus testable.
                    cmdoptions.adddata16 = true;
                } else {
                    cout << "dwarfgen: Invalid lnogoption input " <<
                        longindex << endl;
                    exit(1);
                }
                break;
            case 1001:
                // To test having an empty .debug_dnames
                // section.
                // libdwarf reading is thus testable.
                force_empty_dnames = true;
                break;
            case 1002:
                // To test creating DWARF5
                // DW_FORM_implicit_const.
                // libdwarf reading is thus testable.
                cmdoptions.addimplicitconst = true;
                break;
            case 1003:
                // To test dwarf_add_fde_inst_a().
                // libdwarf reading is thus testable.
                cmdoptions.addframeadvanceloc = true;
                break;
            case 1004:
                //{"add-sun-func-offsets",dwno_argument,0,1004},
                // To test creating DWARF5
                // DW_AT_SUN_func_offsets.
                // libdwarf reading is thus testable.
                cmdoptions.addSUNfuncoffsets = true;
                break;
            case 1005:
                //{"output-v4-test",dwno_argument,0,1005}
                output_v4_test = 1;
                break;
            case 1006:
                //{"add-debug-sup",dwno_argument,0,1006}
                cmdoptions.adddebugsup = true;
                break;
            case 1007:
                //{"add-skip-branch-ops",dwno_argument,0,1007},
                cmdoptions.addskipbranch = true;
                break;
            case 1008:
                //{"add-language-version",dwno_argument,0,1008},
                cmdoptions.addlanguageversion = true;
                break;
            case 'c':
                // At present we can only create a single
                // cu in the output of the libdwarf producer.
                cu_of_input_we_output = atoi(dwoptarg);
                break;
            case 'r':
                cmdoptions.showrelocdetails=true;
                break;
            case 's': // --default-form-strp
                cmdoptions.defaultInfoStringForm = DW_FORM_strp;
                break;
            case 'p': /* pointer size: value 4 or 8. */
                if (!strcmp("4",dwoptarg)) {
                    ptrsizeflagbit = DW_DLC_POINTER32;
                    elfoffsetsizeflagbit = DW_DLC_ELF_OFFSET_SIZE_32;
                } else if (!strcmp("8",dwoptarg)) {
                    ptrsizeflagbit = DW_DLC_POINTER64;
                    elfoffsetsizeflagbit = DW_DLC_ELF_OFFSET_SIZE_64;
                } else {
                    cout << "dwarfgen: Invalid p option input " <<
                        dwoptarg << endl;
                    exit(1);
                }
                break;
            case 'f': /*  offset size for DWARF: value 4 or 8. */
                if (!strcmp("4",dwoptarg)) {
                    dwarfoffsetsizeflagbit = DW_DLC_OFFSET32;
                } else if (!strcmp("8",dwoptarg)) {
                    dwarfoffsetsizeflagbit = DW_DLC_OFFSET64;
                } else {
                    cout << "dwarfgen: Invalid f option input " <<
                        dwoptarg << endl;
                    exit(1);
                }
                break;
            case 'v': /* Version 2 3 4 or 5 */
                if (!strcmp("5",dwoptarg)) {
                    dwarf_version = "V5";
                } else if (!strcmp("4",dwoptarg)) {
                    dwarf_version = "V4";
                } else if (!strcmp("3",dwoptarg)) {
                    dwarf_version = "V3";
                } else if (!strcmp("2",dwoptarg)) {
                    dwarf_version = "V2";
                } else {
                    cout << "dwarfgen: Invalid v option input " <<
                        dwoptarg << endl;
                    exit(1);
                }
                break;
            case 't':
                setinput(&whichinput,dwoptarg,&pathrequired);
                break;
            case 'h':
                cmdoptions.transformHighpcToConst = true;
                break;
            case 'o':
                outfile = dwoptarg;
                break;
            case '?':
                cout << "dwarfgen: Invalid quest? option input "
                    << endl;
                exit(1);
            default:
                cout << "dwarfgen: Invalid option input " << endl;
                exit(1);
            }
        }
        if ( (dwoptind >= argc) && pathrequired) {
            cout << "dwarfgen: Expected argument after options!"
                " Giving up."
                << endl;
            exit(EXIT_FAILURE);
        }
        if (pathrequired) {
            infile = argv[dwoptind];
        }
        if (output_v4_test) {
            dwarf_extras ="opcode_base=13,"
                "minimum_instruction_length=1,"
                "line_base=-1,"
                "line_range=4";
            endian =  DW_DLC_TARGET_LITTLEENDIAN;
            dwarf_version = "V4";
            isa_name = "x86_64";
            ptrsizeflagbit = DW_DLC_POINTER64;
            dwarfoffsetsizeflagbit = DW_DLC_OFFSET32;
            elfoffsetsizeflagbit = DW_DLC_ELF_OFFSET_SIZE_64;
            machine = EM_X86_64; /* from elf.h */
        }

        if (whichinput == OptReadBin) {
            createIrepFromBinary(infile,Irep);
        } else if (whichinput == OptReadText) {
            cout << "dwarfgen: dwarfgen: text read not supported yet"
                << endl;
            exit(EXIT_FAILURE);
        } else if (whichinput == OptPredefined) {
            cout << "dwarfgen: predefined not supported yet" << endl;
            exit(EXIT_FAILURE);
        } else {
            cout << "dwarfgen: Impossible: unknown input style."
                << endl;
            exit(EXIT_FAILURE);
        }

        // We use the latest calls returning
        // DW_DLV_OK, DW_DLV_NO_ENTRY, or DW_DLV_ERROR
        // as an int.
        Dwarf_Ptr errarg = 0;
        Dwarf_Error err = 0;

        // The point of user_data is, as here, to
        // have crucial data available to the callback
        // function implementation.
        void *user_data = &global_elfclass;

        Dwarf_P_Debug dbg = 0;
        unsigned long dwbitflags =
            endian |
            ptrsizeflagbit|
            elfoffsetsizeflagbit|
            dwarfoffsetsizeflagbit|
            DW_DLC_SYMBOLIC_RELOCATIONS;

        // We use DW_DLC_SYMBOLIC_RELOCATIONS so we can
        // read the relocations and do our own relocating.
        // See calls of dwarf_get_relocation_info().
        int res = dwarf_producer_init(
            dwbitflags,
            CallbackFunc,
            0, // errhand
            errarg,
            user_data,
            isa_name,
            dwarf_version,
            dwarf_extras,
            &dbg,
            &err);
        if (res == DW_DLV_NO_ENTRY) {
            cout << "dwarfgen: Failed dwarf_producer_init() NO_ENTRY"
                << endl;
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_ERROR) {
            cout << "dwarfgen: Failed dwarf_producer_init() ERROR"
                << endl;
            cout << "dwarfgen errmsg " << dwarf_errmsg(err)<<endl;
            exit(EXIT_FAILURE);
        }
        res = dwarf_pro_set_default_string_form(dbg,
            cmdoptions.defaultInfoStringForm,&err);
        if (res != DW_DLV_OK) {
            cout << "dwarfgen: Failed " <<
                "dwarf_pro_set_default_string_form" << endl;
            exit(EXIT_FAILURE);
        }
        if (force_empty_dnames) {
            /*  Fills out a default dnames for testing. */
            res = dwarf_force_dnames(dbg,0,&err);
            if (res != DW_DLV_OK) {
                cout << "dwarfgen: "
                    "Failed dwarf_force_debug_names"
                    << endl;
                exit(EXIT_FAILURE);
            }
        }
        if (cmdoptions.adddebugsup) {
            create_debug_sup_content(dbg);
        }
        transform_irep_to_dbg(dbg,Irep,cu_of_input_we_output);
        write_object_file(dbg,Irep,machine,endian, dwbitflags,
            user_data);

        Dwarf_Unsigned str_count = 0;
        Dwarf_Unsigned str_len = 0;
        Dwarf_Unsigned debug_str_count = 0;
        Dwarf_Unsigned debug_str_len = 0;
        Dwarf_Unsigned reused_count = 0;
        Dwarf_Unsigned reused_len = 0;
        res = dwarf_pro_get_string_stats(dbg,
            &str_count,&str_len,
            &debug_str_count,
            &debug_str_len,
            &reused_count,
            &reused_len,&err);
        if (res != DW_DLV_OK) {
            cout << "Unable to get string statistics. ERROR."
                << endl;
        } else {
            cout << "Debug_Str: debug_info str count " <<str_count <<
                ", byte total len " <<str_len << endl;
            cout << "Debug_Str: count " <<debug_str_count <<
                ", byte total len " <<debug_str_len << endl;
            cout << "Debug_Str: Reused count " <<reused_count <<
                ", byte total len not emitted " <<reused_len << endl;
        }
        dwarf_producer_finish_a( dbg, 0);
        return 0;
    } // End try
    catch (std::bad_alloc &ba) {
        cout << "dwarfgen FAIL:bad alloc caught " <<
            ba.what() << endl;
        exit(EXIT_FAILURE);
    }
    catch (std::exception &e) {
        cout << "dwarfgen FAIL:std lib exception " <<
            e.what() << endl;
        exit(EXIT_FAILURE);
    }
    catch (...) {
        cout << "dwarfgen FAIL:other failure " << endl;
        exit(EXIT_FAILURE);
    }
    exit(1);
}

static void
create_debug_sup_content(Dwarf_P_Debug dbg)
{
    int res = 0;
    Dwarf_Error err = 0;
    char x_content[] = "fake checksum_content";
    Dwarf_Small * content = (Dwarf_Small *)x_content;

    res = dwarf_add_debug_sup(dbg,
        2,0,
        (char *)"fake-file/fake-file-name",
        sizeof(x_content),
        content,&err);
    if (res != DW_DLV_OK) {
        printf("FAIL creating .debug_sup data in dbg "
            " due to error  %s\n",
            dwarf_errmsg(err));
        exit(1);
    }
    return;
}

// Layout is Elf Hdr, Section content, section headers
static void
calculate_all_offsets(void)
{
    Dwarf_Unsigned total_length = 0;
    Dwarf_Unsigned ehdr_length = 0;
    Dwarf_Unsigned section_headers_length = 0;
    Dwarf_Unsigned sechdr_length = 0;
    Dwarf_Unsigned section_content_len = 0;

    if (dwelfheader.elf_is_32bit()) {
        ehdr_length = sizeof(dw_elf32_ehdr);
        sechdr_length = sizeof(dw_elf32_shdr);
        total_length +=  ehdr_length;
    } else {
        ehdr_length += sizeof(dw_elf64_ehdr);
        sechdr_length = sizeof(dw_elf64_shdr);
        total_length += ehdr_length;
    }
    dwelfheader.e_shentsize_ = sechdr_length;
    dwelfheader.e_hdrlen_ = ehdr_length;
    dwelfheader.e_shnum_ = dwsectab.size();
    dwelfheader.e_ehsize_ = ehdr_length;

    for (vector<SectionForDwarf>::iterator it = dwsectab.begin();
        it != dwsectab.end();
        it++) {
        SectionForDwarf &sec = *it;
        sec.sh_offset_ = total_length;
        section_content_len += sec.sh_size_;
        total_length += sec.sh_size_;
    }
    section_headers_length = sechdr_length*dwsectab.size();
    total_length += section_headers_length;

    Dwarf_Unsigned curoff = ehdr_length;
    for (vector<SectionForDwarf>::iterator it = dwsectab.begin();
        it != dwsectab.end();
        it++) {
        SectionForDwarf &sec = *it;
        sec.sh_offset_= curoff;
        curoff += sec.sh_size_;
    }
    dwelfheader.e_shoff_ = ehdr_length + section_content_len;
    dwelfheader.e_shnum_ = dwsectab.size();
}

// Gets all the data from libdwarfp and writes
// an Elf object to outfile.c_str()
static void
write_object_file(Dwarf_P_Debug dbg,
    IRepresentation &irep,
    unsigned machine,
    unsigned endian,
    unsigned long dwbitflags,
    void *user_data)
{
    unsigned elfclass = (dwbitflags & DW_DLC_ELF_OFFSET_SIZE_64)?
        ELFCLASS64:
        ELFCLASS32;
    unsigned elfendian = (endian & DW_DLC_TARGET_LITTLEENDIAN)?
        ELFDATA2LSB:
        ELFDATA2MSB;
    bool dwarfoffset32 = (dwbitflags&DW_DLC_OFFSET64)?true:false;
    bool elfpointer64  = (dwbitflags&DW_DLC_POINTER64)?true:false;
    *(unsigned *)user_data = elfclass;
    dwwriter.openFile(outfile);
    dwelfheader = ElfHeaderForDwarf();
    dwelfheader.e_ident_[EI_MAG0] = ELFMAG0;
    dwelfheader.e_ident_[EI_MAG1] = ELFMAG1;
    dwelfheader.e_ident_[EI_MAG2] = ELFMAG2;
    dwelfheader.e_ident_[EI_MAG3] = ELFMAG3;
    dwelfheader.e_ident_[EI_CLASS] = elfclass;
    dwelfheader.e_ident_[EI_DATA] = elfendian;
    dwelfheader.e_ident_[EI_VERSION] = EV_CURRENT;
    dwelfheader.e_machine_ = machine;
    dwelfheader.e_type_ = ET_REL;
    dwelfheader.e_version_ = EV_CURRENT;
    dwelfheader.e_dwarf_32bit_ = dwarfoffset32;
    dwelfheader.e_ptrbytesize_ =  elfpointer64?8:4;

    create_initial_section();
    create_text_section();

    // Write the DWARF to our section data in memory.
    write_generated_dbg(dbg,irep);
    // Create the section name string section,
    // set e_shstrndx.
    create_namestr_section();
    calculate_all_offsets();
    Dwarf_Unsigned finalsize = write_to_object();
    cout << " output image size in bytes " << finalsize << endl;
    dwwriter.closeFile();
}

static unsigned char text[4] = {0,0,0,0};

// an object section with fake .text data (just as an example).
static void
create_text_section(void)
{
    string s(".text");
    unsigned sectindex = dwsectab.size();

    SectionForDwarf ds(s,
        (Dwarf_Unsigned)SHT_PROGBITS,
        (Dwarf_Unsigned)0,(Dwarf_Unsigned)0,
        (Dwarf_Unsigned) 0);
    cout << "New Elf section: " << s <<
        " Type=" <<SHT_PROGBITS << " Flags=" << 0 <<
        " Elf secnum=" << sectindex <<
        " link section=" << 0 <<
        " info="<< 0 << endl;
    ds.sh_addralign_ = dwelfheader.elf_is_64bit()?8:4;
    ds.add_section_content(text,sizeof(text));
    ds.setSectIndex(sectindex);
    dwsectab.push_back(ds);
}

// Take the lists of data blocks and build a single
// malloc block to contain it all pointing
// to it with the public struct DW_Elf_Data that
// _dwarf_elf_newdata() creates.
// d is dwarfgen section index
static void
InsertDataIntoElf(Dwarf_Unsigned d,Dwarf_P_Debug dbg)
{
    Dwarf_Unsigned dw_section_index = 0;
    Dwarf_Unsigned length = 0;
    int res = 0;
    Dwarf_Ptr bytes = 0;

    res = dwarf_get_section_bytes_a(dbg,d,
        &dw_section_index,&length,&bytes,0);
    if (res != DW_DLV_OK) {
        cout << "dwarfgen: get_section_bytes_a problem " << d
            << endl;
        exit(1);
    }
    SectionForDwarf &ds = dwsectab[dw_section_index];
    ds.add_section_content((unsigned char *)bytes,length);
    cout << "Inserted " << length <<
        " bytes into elf section index "
        << dw_section_index << endl;
    return;
}

#if 0
{
    if (!scn) {
        cout <<
            "dwarfgen: Unable to _dwarf_elf_getscn "
            "on disk transform # "
            << d << endl;
        exit(1);
    }

    ElfSectIndex si(elf_section_index);
    SectionForDwarf & sfd  = FindMySection(si);

    DW_Elf_Data* ed = _dwarf_elf_newdata(scn);
    if (!ed) {
        cout <<
            "dwarfgen: _dwarf_elf_newdata died on transformed index "
            << d << endl;
        exit(1);
    }
    ed->d_buf = bytes;
    ed->d_type =  DWARF_T_BYTE;
    ed->d_size = length;
    ed->d_off = sfd.getNextOffset();
    sfd.setNextOffset(ed->d_off + length);
    ed->d_align = 1;
    ed->d_version = EV_CURRENT;
    cout << "Inserted " << length << " bytes into elf section index "
        << elf_section_index << endl;
}
#endif

#if 0
static string
printable_rel_type(unsigned char reltype)
{
    enum Dwarf_Rel_Type t = (enum Dwarf_Rel_Type)reltype;
    switch(t) {
    case   dwarf_drt_none:
        return "dwarf_drt_none";
    case   dwarf_drt_data_reloc:
        return "dwarf_drt_data_reloc";
    case   dwarf_drt_segment_rel:
        return "dwarf_drt_segment_rel";
    case   dwarf_drt_first_of_length_pair:
        return "dwarf_drt_first_of_length_pair";
    case   dwarf_drt_second_of_length_pair:
        return "dwarf_drt_second_of_length_pair";
    default:
        break;
    }
    return "drt-unknown (impossible case)";
}
#endif

static Dwarf_Unsigned
FindSymbolValue(ElfSymIndex symi,IRepresentation &irep)
{
    ElfSymbols & syms = irep.getElfSymbols();
    ElfSymbol & es =  syms.getElfSymbol(symi);
    Dwarf_Unsigned symv = es.getSymbolValue();
    return symv;
}

#if 0
static int
dump_bytes(const char *msg,void *val,int len)
{
    char *p = (char *)val;
    int i = 0;
    cout << msg << " ";
    for (; i < len; ++i) {
        char x = *(p+i);
        cout << IToHex(x,2) <<" ";
    }
    cout << endl;
}
#endif

/* Lets not assume that the quantities are aligned. */
static void
bitreplace(char *buf, unsigned int targlen, Dwarf_Unsigned newval,
    size_t newvalsize)
{
    (void)newvalsize;
    if (targlen == 4) {
        Dwarf_Unsigned my4 = newval;
        Dwarf_Unsigned oldval = 0;
        ASNX(&oldval,sizeof(oldval),buf,(unsigned)targlen);
        oldval += my4;
        ASNX(buf,(unsigned)targlen,&oldval,sizeof(oldval));
    } else if (targlen == 8) {
        Dwarf_Unsigned my8 = newval;
        Dwarf_Unsigned oldval = 0;
        memcpy(&oldval,buf,targlen);
        oldval += my8;
        memcpy(buf,&oldval,targlen);
    } else {
        cout << "dwarfgen:  Relocation is length " << targlen <<
            " which we do not yet handle." << endl;
        exit(1);
    }
}

// This remembers nothing, so is slow.
// Used solely with relocations
static char *
findelfbuf(SectionForDwarf &scn,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned length)
{
    Dwarf_Unsigned bloboff = 0;

    for (vector<ByteBlob>::iterator it =
        scn.sectioncontent_.begin();
        it != scn.sectioncontent_.end();
        it++) {
        ByteBlob &bb = *it;
        if (offset >= (bb.len_ + bloboff)) {
            bloboff += bb.len_;
            continue;
        }
        Dwarf_Unsigned localoff = offset - bloboff;
        if ((localoff + length) > bb.len_) {
            cout << "dwarfgen:  Relocation at offset  " <<
                offset << " cannot be accomplished, no buffer. "
                << endl;
            exit(1);
        }
        char *lclptr = reinterpret_cast<char *>(bb.bytes_) +
            localoff;
        return lclptr;
    }
    cout << " Relocation at offset  " << offset  <<
        " cannot be accomplished,  past end of buffers" << endl;
    return 0;
}

static void
write_generated_dbg(Dwarf_P_Debug dbg,
    IRepresentation &irep)
{
    Dwarf_Error err = 0;

    // Sectioncount here is dwarfgen blob count, not Elf
    // or even section count
    Dwarf_Unsigned sectioncount = 0;

    //  This call does callbacks to inform of all the sections
    //  we need to create.
    int res = dwarf_transform_to_disk_form_a(dbg,&sectioncount,&err);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            string msg(dwarf_errmsg(err));
            cout << "Dwarfgen fails: " << msg << endl;
            exit(1);
        }
        /* ASSERT: rex == DW_DLV_NO_ENTRY */
        cout << "Dwarfgen fails, some internal error " << endl;
        exit(1);
    }
    Dwarf_Unsigned d = 0;
    for (d = 0; d < sectioncount ; ++d) {
        InsertDataIntoElf(d,dbg);
    }

    // Since we are emitting in final form sometimes, we may
    // do relocation processing here or we could (but do not)
    // emit relocation records into the object file.
    // The following is for DW_DLC_SYMBOLIC_RELOCATIONS.

    Dwarf_Unsigned reloc_sections_count = 0;
    int drd_version = 0;
    res = dwarf_get_relocation_info_count(dbg,&reloc_sections_count,
        &drd_version,&err);
    if (res != DW_DLV_OK) {
        cout << "dwarfgen: Error getting relocation info count."
            << endl;
        exit(1);

    }
    cout << "Relocations sections count= " << reloc_sections_count <<
        " relversion=" << drd_version << endl;
    for (Dwarf_Unsigned ct = 0; ct < reloc_sections_count ; ++ct) {
        // elf_section_index is the elf index of the
        // section to be relocated, and the section number
        // in the object file which we are creating.
        // In dwarfgen we do not use this as we do not create
        // relocation sections. Here it is always zero.
        Dwarf_Unsigned elf_section_index = 0;

        // elf_section_index_link is the elf index of the
        // section  the relocations apply to, such as .debug_info.
        // An elf index, not dwsectab[] index.
        Dwarf_Unsigned elf_section_index_link = 0;

        // relocation_buffer_count is the number of relocations
        // of this section.
        Dwarf_Unsigned relocation_buffer_count = 0;
        Dwarf_Relocation_Data reld;
        res = dwarf_get_relocation_info(dbg,&elf_section_index,
            &elf_section_index_link,
            &relocation_buffer_count,
            &reld,&err);
        // elf_section_index_link
        // refers to the output section numbers, not to dwsectab.
        if (res != DW_DLV_OK) {
            cout << "dwarfgen: Error getting relocation record " <<
                ct << "."  << endl;
            exit(1);
        }

        int dwseclink =  FindMySectionNum(elf_section_index_link);
        Dwarf_Unsigned sitarg = dwsectab[dwseclink].getSectIndex();
        string linktarg= dwsectab[dwseclink].name_;
        Dwarf_Unsigned targsec = sitarg;

        cout << "Relocs for sec=" << ct <<
            " ourlinkto="       << elf_section_index_link <<
            " linktoobjsecnum=" << targsec <<
            " name="            << linktarg <<
            " reloc-count="     << relocation_buffer_count << endl;
        SectionForDwarf &scn =  dwsectab[elf_section_index_link];

        for (Dwarf_Unsigned r = 0;
            r < relocation_buffer_count; ++r) {
            Dwarf_Relocation_Data rec = reld+r;
            ElfSymIndex symi(rec->drd_symbol_index);
            Dwarf_Unsigned newval = FindSymbolValue(symi,irep);
            char *buf_to_update = findelfbuf(scn,
                rec->drd_offset,rec->drd_length);
            if (buf_to_update) {
                if (cmdoptions.showrelocdetails) {
                    cout << "Reloc "<< r <<
                        " symindex=" << rec->drd_symbol_index <<
                        " targoffset= " << IToHex(rec->drd_offset) <<
                        " newval = " << IToHex(newval) << endl;
                }
                bitreplace(buf_to_update,rec->drd_length,
                    newval,sizeof(newval));
            } else {
                if (cmdoptions.showrelocdetails) {
                    cout << "Reloc "<< r << "does nothing"<<endl;
                }
            }
        }
    }
}

static void
write_elf_header(void)
{
    unsigned char *datap = 0;

    if (dwelfheader.elf_is_32bit()) {
        dw_elf32_ehdr &ehp =  dwelfheader.e_32_;
        memset(&ehp,0,sizeof(ehp));
        ehp.e_ident[EI_MAG0] = dwelfheader.e_ident_[EI_MAG0];
        ehp.e_ident[EI_MAG1] = dwelfheader.e_ident_[EI_MAG1];
        ehp.e_ident[EI_MAG2] = dwelfheader.e_ident_[EI_MAG2];
        ehp.e_ident[EI_MAG3] = dwelfheader.e_ident_[EI_MAG3];
        ehp.e_ident[EI_CLASS] = dwelfheader.e_ident_[EI_CLASS];
        ehp.e_ident[EI_DATA] = dwelfheader.e_ident_[EI_DATA];
        ehp.e_ident[EI_VERSION] = dwelfheader.e_ident_[EI_VERSION];
        ASNX(ehp.e_machine,sizeof(ehp.e_machine),
            &dwelfheader.e_machine_,
            sizeof(dwelfheader.e_machine_));
        ASNX(ehp.e_type,sizeof(ehp.e_type),
            &dwelfheader.e_type_,sizeof(dwelfheader.e_type_));
        ASNX(ehp.e_version,sizeof(ehp.e_version),
            &dwelfheader.e_version_,
            sizeof(dwelfheader.e_version_));
        ASNX(ehp.e_shnum,sizeof(ehp.e_shnum),
            &dwelfheader.e_shnum_,
            sizeof(dwelfheader.e_shnum_));
        ASNX(ehp.e_shentsize,sizeof(ehp.e_shentsize),
            &dwelfheader.e_shentsize_,
            sizeof(dwelfheader.e_shentsize_));
        ASNX(ehp.e_shstrndx,sizeof(ehp.e_shstrndx),
            &dwelfheader.e_shstrndx_,
            sizeof(dwelfheader.e_shstrndx_));
        ASNX(ehp.e_shoff,sizeof(ehp.e_shoff),
            &dwelfheader.e_shoff_,
            sizeof(dwelfheader.e_shoff_));
        ASNX(ehp.e_ehsize,sizeof(ehp.e_ehsize),
            &dwelfheader.e_ehsize_,
            sizeof(dwelfheader.e_ehsize_));
        datap = (unsigned char *)&dwelfheader.e_32_;
    } else {
        dw_elf64_ehdr &ehp =  dwelfheader.e_64_;
        ehp.e_ident[EI_MAG0] = dwelfheader.e_ident_[EI_MAG0];
        ehp.e_ident[EI_MAG1] = dwelfheader.e_ident_[EI_MAG1];
        ehp.e_ident[EI_MAG2] = dwelfheader.e_ident_[EI_MAG2];
        ehp.e_ident[EI_MAG3] = dwelfheader.e_ident_[EI_MAG3];
        ehp.e_ident[EI_CLASS] = dwelfheader.e_ident_[EI_CLASS];
        ehp.e_ident[EI_DATA] = dwelfheader.e_ident_[EI_DATA];
        ehp.e_ident[EI_VERSION] = dwelfheader.e_ident_[EI_VERSION];
        ASNX(ehp.e_machine,sizeof(ehp.e_machine),
            &dwelfheader.e_machine_,
            sizeof(dwelfheader.e_machine_));
        ASNX(ehp.e_type,sizeof(ehp.e_type),
            &dwelfheader.e_type_,sizeof(dwelfheader.e_type_));
        ASNX(ehp.e_version,sizeof(ehp.e_version),
            &dwelfheader.e_version_,
            sizeof(dwelfheader.e_version_));
        ASNX(ehp.e_shnum,sizeof(ehp.e_shnum),
            &dwelfheader.e_shnum_,
            sizeof(dwelfheader.e_shnum_));
        ASNX(ehp.e_shentsize,sizeof(ehp.e_shentsize),
            &dwelfheader.e_shentsize_,
            sizeof(dwelfheader.e_shentsize_));
        ASNX(ehp.e_shstrndx,sizeof(ehp.e_shstrndx),
            &dwelfheader.e_shstrndx_,
            sizeof(dwelfheader.e_shstrndx_));
        ASNX(ehp.e_shoff,sizeof(ehp.e_shoff),
            &dwelfheader.e_shoff_,
            sizeof(dwelfheader.e_shoff_));
        ASNX(ehp.e_ehsize,sizeof(ehp.e_ehsize),
            &dwelfheader.e_ehsize_,
            sizeof(dwelfheader.e_ehsize_));
        datap = (unsigned char *)&dwelfheader.e_64_;
    }
    dwwriter.wwrite(0,dwelfheader.e_hdrlen_,datap);
}

// Returns pointer to the content, len of each is
// dwelfheader.e_shentsize_
static unsigned char *
copy_section_hdr_data (SectionForDwarf&sec)
{
    if (dwelfheader.elf_is_32bit())
    {
        dw_elf32_shdr * shdr = &sec.shdr32_;
        ASNX(shdr->sh_name,sizeof(shdr->sh_name),
            &sec.sh_name_,
            sizeof(sec.sh_name_));
        ASNX(shdr->sh_type,sizeof(shdr->sh_type),
            &sec.sh_type_, sizeof(sec.sh_type_));
        ASNX(shdr->sh_flags,sizeof(shdr->sh_flags),
            &sec.sh_flags_, sizeof(sec.sh_flags_));
        ASNX(shdr->sh_addr,sizeof(shdr->sh_addr),
            &sec.sh_addr_, sizeof(sec.sh_addr_));
        ASNX(shdr->sh_offset,sizeof(shdr->sh_offset),
            &sec.sh_offset_,sizeof(sec.sh_offset_));
        ASNX(shdr->sh_size,sizeof(shdr->sh_size),
            &sec.sh_size_,sizeof(sec.sh_size_));
        ASNX(shdr->sh_link,sizeof(shdr->sh_link),
            &sec.sh_link_,sizeof(sec.sh_link_));
        ASNX(shdr->sh_info,sizeof(shdr->sh_info),
            &sec.sh_info_,sizeof(sec.sh_info_));
        ASNX(shdr->sh_addralign,sizeof(shdr->sh_addralign),
            &sec.sh_addralign_,sizeof(sec.sh_addralign_));
        ASNX(shdr->sh_entsize,sizeof(shdr->sh_entsize),
            &sec.sh_entsize_,sizeof(sec.sh_entsize_));
        return (unsigned char *)shdr;
    } else {
        dw_elf64_shdr * shdr = &sec.shdr64_;
        ASNX(shdr->sh_name,sizeof(shdr->sh_name),
            &sec.sh_name_,
            sizeof(sec.sh_name_));
        ASNX(shdr->sh_type,sizeof(shdr->sh_type),
            &sec.sh_type_, sizeof(sec.sh_type_));
        ASNX(shdr->sh_flags,sizeof(shdr->sh_flags),
            &sec.sh_flags_, sizeof(sec.sh_flags_));
        ASNX(shdr->sh_addr,sizeof(shdr->sh_addr),
            &sec.sh_addr_, sizeof(sec.sh_addr_));
        ASNX(shdr->sh_offset,sizeof(shdr->sh_offset),
            &sec.sh_offset_,sizeof(sec.sh_offset_));
        ASNX(shdr->sh_size,sizeof(shdr->sh_size),
            &sec.sh_size_,sizeof(sec.sh_size_));
        ASNX(shdr->sh_link,sizeof(shdr->sh_link),
            &sec.sh_link_,sizeof(sec.sh_link_));
        ASNX(shdr->sh_info,sizeof(shdr->sh_info),
            &sec.sh_link_,sizeof(sec.sh_link_));
        ASNX(shdr->sh_addralign,sizeof(shdr->sh_addralign),
            &sec.sh_addralign_,sizeof(sec.sh_addralign_));
        ASNX(shdr->sh_entsize,sizeof(shdr->sh_entsize),
            &sec.sh_entsize_,sizeof(sec.sh_entsize_));
        return (unsigned char *)shdr;
    }
    /* not reached */
}

static void
write_section_contents(void)
{
    for (vector<SectionForDwarf>::iterator it = dwsectab.begin();
        it != dwsectab.end();
        it++) {
        SectionForDwarf &sec = *it;
        Dwarf_Unsigned curoff = sec.sh_offset_;

        for (vector<ByteBlob>::iterator itb =
            sec.sectioncontent_.begin();
            itb != sec.sectioncontent_.end();
            itb++) {
            ByteBlob &bb = *itb;

            //cout << "Writing section content " <<i <<
            //    " at offset "  << curoff << endl;
            dwwriter.wwrite(curoff,bb.len_,bb.bytes_);
            curoff += bb.len_;
        }
    }
}

static void
write_section_headers()
{
    Dwarf_Unsigned headeroff = dwelfheader.e_shoff_;
    for (vector<SectionForDwarf>::iterator it = dwsectab.begin();
        it != dwsectab.end();
        it++) {
        //cout << "Writing section header " <<i <<
        //    " at offset "  << headeroff << endl;
        SectionForDwarf &sec = *it;
        Dwarf_Unsigned hdrlen = dwelfheader.e_shentsize_;

        unsigned char *data = copy_section_hdr_data(sec);
        dwwriter.wwrite(headeroff,hdrlen,data);
        headeroff += dwelfheader.e_shentsize_;
    }
}

//  We write  elf header, sections, section headers.
Dwarf_Unsigned
write_to_object(void)
{
    write_elf_header();
    write_section_contents();
    write_section_headers();

    Dwarf_Unsigned len = dwelfheader.e_shoff_+
        dwelfheader.e_shnum_* dwelfheader.e_shentsize_;
    return len;
}

int
open_a_file(const char * name)
{
    /* Set to a file number that cannot be legal. */
    int f = -1;

    f = open(name, O_RDONLY | O_BINARY);
    return f;
}

void
close_a_file(int f)
{
    close(f);
}
