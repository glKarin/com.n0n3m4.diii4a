/*  Copyright (C) 2020 David Anderson.  2020.
    This small program is hereby
    placed into the public domain to be copied or
    used by anyone for any purpose.

    It is not tested nor normally built,
    it is just in case one wants
    to redo a crc calculation on a file.
    In a build directory compile something like this:

    cc -Ilibdwarf $HOME/dwarf/code/dwarfexample/simplecrc.c \
        libdwarf/.libs/libdwarf.a -lz
*/

#include <stdio.h>
#include <stdlib.h> /* for exit(), C89 malloc */
#include <unistd.h> /* for close */
#include <sys/types.h> /* for off_t ssize_t */
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h> /* for memset */
#include "dwarf.h"
#include "libdwarf.h"

static void
simple_test(const char *fname)
{
    int fd = 0;
    off_t size_left = 0;
    off_t fsize = 0;
    off_t lsval = 0;
    ssize_t readlen = 1000;
    unsigned char *readbuf = 0;
    ssize_t readval = 0;
    unsigned int tcrc = 0;
    unsigned int init = 0;

    printf("File: %s\n",fname);
    fd = open(fname,O_RDONLY);
    if (fd < 0) {
        printf("no such file\n");
            return;
    }
    {
        fsize = size_left = lseek(fd,0L,SEEK_END);
        printf("file size %lu (0x%lx)\n",(unsigned long)fsize,
            (unsigned long)fsize);
        if (fsize   == (off_t)-1) {
        printf("Fail 22\n");
            exit(EXIT_FAILURE);
        }
    }
    if (fsize <= (off_t)500) {
        /*  Not a real object file.
            A random length check. */
        printf("Fail 21\n");
        exit(EXIT_FAILURE);
    }
    lsval  = lseek(fd,0L,SEEK_SET);
    if (lsval < 0) {
        printf("Fail 1\n");
        exit(EXIT_FAILURE);
    }
    readbuf = (unsigned char *)malloc(readlen);
    if (!readbuf) {
        printf("Fail 2\n");
        exit(EXIT_FAILURE);
    }
    while (size_left > 0) {
        if (size_left < readlen) {
            readlen = size_left;
        }
        readval = read(fd,readbuf,readlen);
        if (readval != (ssize_t)readlen) {
            printf("Fail 3\n");
            exit(EXIT_FAILURE);
        }
        size_left -= readlen;
        tcrc = dwarf_basic_crc32(readbuf,readlen,init);
        init = tcrc;
    }
}

int
main(int argc, char **argv)
{
    const char *fname = 0;

    (void)argc;
    (void)argv;

    fname =    "/usr/lib/debug/.build-id/1c/"
        "2d642ffb01d1894d3c7dba050fcd160580a3e1.debug";
    simple_test(fname);

    fname =    "/usr/bin/gdb";
    simple_test(fname);
    fname = "/home/davea/dwarf/code/dwarfexample"
        "/dummyexecutable.debug";
    simple_test(fname);

}
