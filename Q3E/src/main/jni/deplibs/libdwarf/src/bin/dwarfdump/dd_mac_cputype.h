/*     available from https://opensource.apple.com/
    see Developer Tools version 8.2.1. cctools-895/include/loader.h */
/*
* Copyright (c) 1999-2010 Apple Inc.  All Rights Reserved.
*
* @APPLE_LICENSE_HEADER_START@
*
* This file contains Original Code and/or
* Modifications of Original Code
* as defined in and that are subject to the
*  Apple Public Source License
* Version 2.0 (the 'License'). You may not use this file except in
* compliance with the License. Please obtain a copy of the License at
* http://www.opensource.apple.com/apsl/ and read it before using this
* file.
*
* The Original Code and all software distributed under
*  the License are
* distributed on an 'AS IS' basis, WITHOUT
*  WARRANTY OF ANY KIND, EITHER
* EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
* INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE,
*  QUIET ENJOYMENT OR NON-INFRINGEMENT.
* Please see the License for the
*  specific language governing rights and
* limitations under the License.
*
* @APPLE_LICENSE_HEADER_END@
*/
#ifndef DD_MAC_CPUTYPE_H
#define DD_MAC_CPUTYPE_H

#define CPU_ARCH_MASK     0xff000000 /* mask for architecture bits */
#define CPU_ARCH_ABI64     0x01000000 /* 64 bit ABI */
#define CPU_ARCH_ABI64_32  0x02000000 /* ABI: 64-bit hardware LP32 */

#define CPU_TYPE_VAX            ( 1)
#define CPU_TYPE_MC680x0        ( 6)
#define CPU_TYPE_X86            ( 7)
#define CPU_TYPE_I386           CPU_TYPE_X86 /* compatibility */
#define CPU_TYPE_X86_64         (CPU_TYPE_X86 | CPU_ARCH_ABI64)

/* skip CPU_TYPE_MIPS           ( 8)  */
#define CPU_TYPE_MC98000        ( 10)
#define CPU_TYPE_HPPA           ( 11)
#define CPU_TYPE_ARM            ( 12)
#define CPU_TYPE_ARM64          (CPU_TYPE_ARM | CPU_ARCH_ABI64)
#define CPU_TYPE_ARM64_32       (CPU_TYPE_ARM | CPU_ARCH_ABI64_32)
#define CPU_TYPE_MC88000        ( 13)
#define CPU_TYPE_SPARC          ( 14)
#define CPU_TYPE_I860           ( 15)
/* skip CPU_TYPE_ALPHA          ( 16)       */
#define CPU_TYPE_POWERPC        ( 18)
#define CPU_TYPE_POWERPC64      (CPU_TYPE_POWERPC | CPU_ARCH_ABI64)

static const struct base_mac_cpu_s {
    Dwarf_Unsigned value;
    const char *name;
} mac_cpubase [] = {
{0,"Unknown cpu type"},
{1,"CPU_TYPE_VAX"},
{6,"CPU_TYPE_MC680x0"},
{7,"CPU_TYPE_X86"},
{CPU_TYPE_X86,"CPU_TYPE_I386"},  /* CPU_TYPE_X86 compatibility */
{(CPU_TYPE_X86 | CPU_ARCH_ABI64), "CPU_TYPE_X86_64"},
{10,"CPU_TYPE_MC98000",},
{11,"CPU_TYPE_HPPA"},
{12,"CPU_TYPE_ARM"},
{(CPU_TYPE_ARM | CPU_ARCH_ABI64),"CPU_TYPE_ARM64"},
{(CPU_TYPE_ARM | CPU_ARCH_ABI64_32),"CPU_TYPE_ARM64_32"},
{13, "CPU_TYPE_MC88000"},
{14,"CPU_TYPE_SPARC"},
{15, "CPU_TYPE_I860"},
    /*{"", skip CPU_TYPE_ALPHA  (16)       */
    /*{"",  skip  (17)       */
{18,"CPU_TYPE_POWERPC"},
{(CPU_TYPE_POWERPC | CPU_ARCH_ABI64),"CPU_TYPE_POWERPC64"},
{0,0}
};

static const char *
dd_mach_arch_name(Dwarf_Unsigned val)
{
    const struct  base_mac_cpu_s *v = mac_cpubase;

    for ( ; v->name; ++v) {
        if (v->value == val) {
            return v->name;
        }
    }
    return "Unlisted MacOS cpu architecture";
}
#endif /* DD_MAC_CPUTYPE_H */
