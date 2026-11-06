#ifndef DD_ELF_CPUTYPE_H
#define DD_ELF_CPUTYPE_H

#define EM_NONE        0  /* No machine */
#define EM_M32         1  /* AT&T WE 32100 */
#define EM_SPARC   2  /* SUN SPARC */
#define EM_386         3  /* Intel 80386 */
#define EM_68K         4  /* Motorola m68k family */
#define EM_88K         5  /* Motorola m88k family */
#define EM_IAMCU   6  /* Intel MCU */
#define EM_860         7  /* Intel 80860 */
#define EM_MIPS        8  /* MIPS R3000 big-endian */
#define EM_S370        9  /* IBM System/370 */
#define EM_MIPS_RS3_LE    10  /* MIPS R3000 little-endian */
#define EM_PARISC 15  /* HPPA */
#define EM_VPP500 17  /* Fujitsu VPP500 */
#define EM_SPARC32PLUS    18  /* Sun's "v8plus" */
#define EM_960        19  /* Intel 80960 */
#define EM_PPC        20  /* PowerPC */
#define EM_PPC64  21  /* PowerPC 64-bit */
#define EM_S390       22  /* IBM S390 */
#define EM_SPU        23  /* IBM SPU/SPC */
#define EM_ARM        40  /* ARM */
#define EM_SPARCV9    43  /* SPARC v9 64-bit */
#define EM_IA_64  50  /* Intel Merced */
#define EM_MIPS_X 51  /* Stanford MIPS-X */
#define EM_X86_64     62  /* AMD x86-64 architecture */
#define EM_SVX        73  /* Silicon Graphics SVx */
#define EM_VAX        75  /* Digital VAX */
#define EM_AARCH64    183 /* ARM AARCH64 */
#define EM_RISCV  243 /* RISC-V */

static const struct base_elf_cpu_s {
    Dwarf_Unsigned value;
    const char *name;
} elf_cpubase [] = {
{ EM_NONE,  "Zero:No machine"},
{ EM_M32,   "AT&T WE 32100"},
{ EM_SPARC, "SUN SPARC "},
{ EM_386,   "Intel 80386 "},
{ EM_68K,   "Motorola m68k"},
{ EM_88K,   "Motorola m88k"},
{ EM_860,   "Intel 80860 "},
{ EM_MIPS,  "MIPS R3000 big-endian"},
{ EM_S370,  "IBM System/370"},
{ EM_MIPS_RS3_LE, "MIPS R3000 little-endian"},
{ EM_PARISC,     "HPPA"},
{ EM_VPP500,     "Fujitsu VPP500"},
{ EM_SPARC32PLUS, "Sun v8plus"},
{ EM_960,         "Intel 80960"},
{ EM_PPC,         "PowerPC"},
{ EM_PPC64,       "PowerPC 64-bit"},
{ EM_S390,        "IBM S390"},
{ EM_SPU,    "IBM SPU/SPC"},
{ EM_ARM,    "ARM"},
{ EM_SPARCV9,"SPARC v9 64-bit"},
{ EM_IA_64,  "Intel Merced"},
{ EM_MIPS_X, "Stanford MIPS-X"},
{ EM_X86_64, "AMD x86_64"},
{ EM_VAX,    "Digital VAX"},
{ EM_AARCH64,"ARM AARCH64"},
{ EM_RISCV,  "RISC-V"},
{0,0}

};

static const char *
dd_elf_arch_name(Dwarf_Unsigned val)
{
    const struct  base_elf_cpu_s *v = elf_cpubase;

    for ( ; v->name; ++v) {
        if (v->value == val) {
            return v->name;
        }
    }
    return "Unlisted Elf cpu architecture";
}

#endif /* DD_ELF_CPUTYPE_H */
