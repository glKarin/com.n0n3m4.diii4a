/*  Derived from the mingw-w64 runtime package.
    There is absolutely no warrantee. */
#ifndef DD_PE_CPUTYPE_H
#define DD_PE_CPUTYPE_H

static const struct base_pe_cpu_s {
    const char *name;
    Dwarf_Unsigned value;
} pe_cpubase [] = {
{"I386", 0x014c},
{"R3000", 0x0162},
{"R4000", 0x0166},
{"R10000", 0x0168},
{"WCEMIPSV2", 0x0169},
{"ALPHA", 0x0184},
{"SH3", 0x01a2},
{"SH3DSP", 0x01a3},
{"SH3E", 0x01a4},
{"SH4", 0x01a6},
{"SH5", 0x01a8},
{"ARM", 0x01c0},
{"ARMV7", 0x01c4},
{"ARMNT", 0x01c4},
{"ARM64", 0xaa64},
{"THUMB", 0x01c2},
{"AM33", 0x01d3},
{"POWERPC", 0x01F0},
{"POWERPCFP", 0x01f1},
{"IA64", 0x0200},
{"MIPS16", 0x0266},
{"ALPHA64", 0x0284},
{"MIPSFPU", 0x0366},
{"MIPSFPU16", 0x0466},
{"TRICORE", 0x0520},
{"CEF", 0x0CEF},
{"EBC", 0x0EBC},
{"AMD64", 0x8664},
{"M32R", 0x9041},
{"CEE", 0xc0ee},
{0,0}
};

static const char *
dd_pe_arch_name(Dwarf_Unsigned val)
{
    const struct  base_pe_cpu_s *v = pe_cpubase;

    for ( ; v->name; ++v) {
        if (v->value == val) {
            return v->name;
        }
    }
    return "Unlisted pe cpu architecture";
}
#endif /* DD_PE_CPUTYPE.H */
