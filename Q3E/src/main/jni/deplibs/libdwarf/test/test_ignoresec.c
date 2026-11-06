/*
Copyright (c) 2023, David Anderson
All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <config.h>

#include <stdio.h>  /* printf() */
#include <stdlib.h> /* exit() */
#include <string.h> /* strcmp() strlen() */

#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dwarf_secname_ck.h"

static int errcount;

static void
check_instance(const char *msg,int indx,
    int expect,int got,int line)
{
    if (got == expect) {
        return;
    }
    printf("FAIL [%d]  %s expected %d got %d test line %d\n",
        indx,msg,expect,got,line);
    ++errcount;
}

static const char *ignores[] = {
".bss",
".comment",
".data",
".fini",
".fini_array",
".got",
".init",
".interp",
".jcr",
".plt",
".rela.data",
".rela.got",
".rela.plt",
".rela.text",
".rel.data",
".rel.got",
".rel.plt",
".rel.text",
".sbss",
".text",
".bssa",
".commentb",
".datac",
".finid",
".fini_arraye",
".gotf",
".initg",
".interp.h",
".jcr.q",
".pltr",
".rela.datas",
".rela.got.t",
".rela.plt.u",
".rela.textv",
".rel.dataw",
".rel.got.x",
".rel.plty",
".rel.text.z",
".sbss.aa",
".text.ab",
0
};

static void
test_ignores(void)
{
    int i = 0;
    for ( ; ignores[i]; ++i) {
        int res = _dwarf_ignorethissection(ignores[i]);
        check_instance(ignores[i],i,TRUE,res,__LINE__);
    }
}

static const char *keeps[] = {
".debug_info",
".eh_frame",
".rel.debug_info",
0
};

static void
test_keeps(void)
{
    int i = 0;
    for ( ; keeps[i]; ++i) {
        int res = _dwarf_ignorethissection(keeps[i]);
        check_instance(keeps[i],i,FALSE,res,__LINE__);
    }
}

int main(void)
{
    test_ignores();
    test_keeps();
    if (errcount) {
        exit(EXIT_FAILURE);
    }
    exit(0);
}
