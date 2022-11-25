/* @(#) $Revision: 70.4 $ */

/*
 *	dld.h
 *
 *	Series 300 dynamic loader
 *
 *	declarations and definitions for all modules
 *
 */


#include	<stddef.h>
#include	<stdlib.h>
#include	<string.h>
#include	<setjmp.h>
#include	<errno.h>

#include	<dl.h>

#include	<sys/types.h>
#include	<sys/mman.h>

extern int _setjmp (jmp_buf env);
extern void _longjmp (jmp_buf env, int val);

#if	defined(PURGE_CACHE) || defined(PURGE2)
extern int cachectl (int function, char *address, int length);
#endif
#ifdef MMAP_UNSUPPORTED
extern caddr_t mmap (const caddr_t addr, size_t len, int prot, int flags, int fd, off_t off);
extern int munmap (caddr_t addr, int len);
#endif


typedef unsigned long ulong;
typedef unsigned char uchar;
#if 0
typedef unsigned int uint;
typedef unsigned short ushort;
#endif

typedef const struct header_extension *dl_header_t;
typedef const struct shl_entry *shlt_t;
typedef const struct import_entry *import_t;
typedef const struct export_entry *export_t;
typedef const struct shl_export_entry *shl_export_t;
typedef const struct hash_entry *hash_t;
typedef const char *string_t;
typedef const struct relocation_entry *dreloc_t;

typedef struct dynamic *dynamic_t;
typedef struct dlt_entry *dlt_t;
typedef struct plt_entry *plt_t;

typedef const struct module_entry *module_t;
typedef const struct dmodule_entry *dmodule_t;


extern void fatal (int code, const char *msg);
extern dynamic_t load (shlt_t target_shl, caddr_t addr);
extern int bindfile (dynamic_t target_DYNAMIC);
extern void smart_bind (dynamic_t current_DYNAMIC, dmodule_t dmodule);
extern int patch (ulong address, char length, ulong value);
extern int findsym (import_t import, dynamic_t current_DYNAMIC, int mode, dynamic_t *target_DYNAMIC, export_t *target_export);
extern ulong resolve (dynamic_t target_DYNAMIC, export_t target_export);
extern shl_t _shl_load (const char *path, int flags, long address, int *uerrno);
extern int _shl_findsym (shl_t *handle, const char *symname, short type, long *value, int *uerrno);
extern int _shl_unload (shl_t handle, int *uerrno);
extern int _shl_get (int index, struct shl_descriptor **desc);
#ifdef	GETHANDLE
extern int _shl_gethandle (shl_t handle, struct shl_descriptor **desc);
#endif
extern void shl_bor (void);
extern int ExpLookup (const char *name, int highwater, export_t *symbol, string_t string, export_t export, hash_t hasht, int hashsize);
extern int hash (const char *name);
extern ulong getsp (void);

#ifdef	ELABORATOR
int bind_initial (dynamic_t current_DYNAMIC);
#endif


#ifdef	SEARCH_ORDER
extern dynamic_t head_DYNAMIC;
#endif
extern dynamic_t exec_DYNAMIC;
extern const char *badsym;
extern jmp_buf except;
extern int dld_last_hash;
extern int g_eindex;
extern int risky_load;
extern ulong dirty_sp;

#ifdef	ELABORATOR
extern int initializers;
#endif

#ifdef	HOOKS
unsigned long dld_flags;
int (*dld_hook)();
#endif


#define		zero_fd		0

#define		ADDR(mem)	((mem##_t)((int)(current_DYNAMIC->dl_header->e_spec.dl_header.mem)+(int)(current_DYNAMIC->dl_header)))

#define		SYMBOL_ERR	1
#define		DRELOC_ERR	2

#define		IGNORE_SHL_PROC	0x0001
#define		IGNORE_EXEC	0x0002
