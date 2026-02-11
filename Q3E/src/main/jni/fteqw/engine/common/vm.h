#ifndef _VM_H
#define _VM_H

#ifdef _WIN32
	#define EXPORT_FN __cdecl
#else
	#define EXPORT_FN
#endif

typedef qintptr_t (EXPORT_FN *sys_calldll_t) (qintptr_t arg, ...);
typedef int (*sys_callqvm_t) (void *offset, quintptr_t mask, int fn, const int *arg);

typedef struct vm_s vm_t;

// for syscall users
#define VM_LONG(x)		(*(int*)&(x))	//note: on 64bit platforms, the later bits can contain junk
#define VM_FLOAT(x)		(*(float*)&(x))	//note: on 64bit platforms, the later bits can contain junk
#define VM_POINTER(x)	((x)?(void*)((char *)offset+((x)%mask)):NULL)
#define VM_OOB(p,l)		(p + l >= mask || VM_POINTER(p) < offset)
// ------------------------- * interface * -------------------------

void VM_PrintInfo(vm_t *vm);
vm_t *VM_CreateBuiltin(const char *name, sys_calldll_t syscalldll, qintptr_t (*init)(qintptr_t *args));
vm_t *VM_Create(const char *dllname, sys_calldll_t syscalldll, const char *qvmname, sys_callqvm_t syscallqvm);
const char *VM_GetFilename(vm_t *vm);
void VM_Destroy(vm_t *vm);
//qboolean VM_Restart(vm_t *vm);
qintptr_t VARGS VM_Call(vm_t *vm, qintptr_t instruction, ...);
qboolean VM_NonNative(vm_t *vm);
void *VM_MemoryBase(vm_t *vm);
quintptr_t VM_MemoryMask(vm_t *vm);

#define VM_FS_READ 0
#define VM_FS_WRITE 1
#define VM_FS_APPEND 2
#define VM_FS_APPEND_SYNC 3	//I don't know, don't ask me. look at q3 source
qofs_t VM_fopen (const char *name, int *handle, int fmode, int owner);
int VM_FRead (char *dest, int quantity, int fnum, int owner);
int VM_FWrite (const char *dest, int quantity, int fnum, int owner);
qboolean VM_FSeek (int fnum, qofs_t offset, int seektype, int owner);
qofs_t VM_FTell (int fnum, int owner);
void VM_fclose (int fnum, int owner);
void VM_fcloseall (int owner);
int VM_GetFileList(const char *path, const char *ext, char *output, int buffersize);

typedef struct {
	int			handle;
	int			modificationCount;
	float		value;
	int			integer;
	char		string[256];
} q3vmcvar_t;
int VMQ3_Cvar_Register(q3vmcvar_t *v, char *name, char *defval, int flags);
int VMQ3_Cvar_Update(q3vmcvar_t *v);

typedef struct {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
} q3time_t;
qint64_t Q3VM_GetRealtime(q3time_t *qtime);

#endif
