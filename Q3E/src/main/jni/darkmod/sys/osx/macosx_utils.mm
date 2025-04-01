/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
******************************************************************************/

// -*- mode: objc -*-
#import "../../idlib/precompiled.h"

#import <Foundation/Foundation.h>
#import <mach/mach.h>
#import <mach/vm_map.h>


#define CD_MOUNT_NAME "DOOM"

#include "macosx_local.h"
#include "macosx_sys.h"

//extern "C" void *vm_allocate(unsigned, unsigned*, unsigned, int);
//extern "C" void vm_deallocate(unsigned, unsigned*, unsigned);

const char *macosx_scanForLibraryDirectory(void)
{
    return "/Library/DOOM";
}

// EEEK!
static long size_save;

void *osxAllocateMemoryNV(long size, float readFrequency, float writeFrequency, float priority)
{
    kern_return_t kr;    
    vm_address_t buffer;
    
    kr = vm_allocate(mach_task_self(),
                    (vm_address_t *)&buffer,
                    size,
                    VM_FLAGS_ANYWHERE);
    if(kr == 0) {
        size_save = size;
        return buffer;
    }
    else
    {
        size_save = 0;
        return NULL;
    }

}

void osxFreeMemoryNV(void *pointer)
{
    if(size_save) {
        vm_deallocate(mach_task_self(),
                      (vm_address_t)pointer,
                      size_save);
        size_save = 0;
    }
}

void *osxAllocateMemory(long size)
{
    kern_return_t kr;    
    vm_address_t buffer;
    
    size += sizeof( int );
    kr = vm_allocate(mach_task_self(),
                    (vm_address_t *)&buffer,
                    size,
                    VM_FLAGS_ANYWHERE);
    if(kr == 0) {
        int *ptr = buffer;
        *ptr = size;
        ptr = ptr + 1;
        return ptr;
    }
    else
    {
        return NULL;
    }

}

void osxFreeMemory(void *pointer)
{
    int size;
    int *ptr = pointer;
    ptr = ptr - 1;
    size = *ptr;
    vm_deallocate(mach_task_self(), (vm_address_t)ptr, size);
}

static inline void __eieio(void)
{
	__asm__ ("eieio");
}

static inline void __sync(void)
{
	__asm__ ("sync");
}

static inline void __isync(void)
{
	__asm__ ("isync");
}

static inline void __dcbf(void *base, unsigned long offset)
{
        __asm__ ("dcbf %0, %1"
                :
                : "r" (base), "r" (offset)
                : "r0");
}

static inline void __dcbst(void *base, unsigned long offset)
{
        __asm__ ("dcbst %0, %1"
                :
                : "r" (base), "r" (offset)
                : "r0");
}

static inline void __dcbz(void *base, unsigned long offset)
{
        __asm__ ("dcbz %0, %1"
                :
                : "r" (base), "r" (offset)
                : "r0");
}

void	Sys_FlushCacheMemory( void *base, int bytes ) {
	unsigned long i;
        
        for(i = 0; i <  bytes; i+= 32) {
            __dcbf(base,i);
        }
        __sync();
        __isync();
        __dcbf(base, i);
        __sync(); 
        __isync();
        *(volatile unsigned long *)(base + i);
        __isync(); 
}
