/* @(#) $Revision: 70.5 $ */

/*
 *	dl.h
 *
 *	Series 300 dynamic loader
 *
 *	declarations and definitions for users
 *
 */

#ifndef		_DL_INCLUDED
#define		_DL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef 		__hp9000s300

#include	<a.out.h>
#include	<sys/param.h>

typedef struct dynamic *shl_t;

struct shl_descriptor
{
	unsigned long tstart;
	unsigned long tend;
	unsigned long dstart;
	unsigned long dend;
	shl_t handle;
	unsigned long spare1;
	char filename[MAXPATHLEN+1];
	void *initializer;
	unsigned long ref_count;
};

#define		NO_INITIALIZER	((void *)(-1))

#define		SHL_INIT	0
#define		SHL_BOR		6
#define		SHL_LOAD	12
#define		SHL_FINDSYM	18
#define		SHL_UNLOAD	24
#define		SHL_GET		30
#define		SHL_TERM	36
#define		SHL_GETHANDLE	42
#define		SHL_DEFINESYM	48

#define		DLD_PRIVATE	1
#define		DLD_NOFIXPLT	2

#if defined(__STDC__) || defined(__cplusplus)
extern shl_t shl_load (const char *path, int flags, long address);
extern int shl_findsym (shl_t *handle, const char *symname, short type, void *value);
extern int shl_unload (shl_t handle);
extern int shl_get (int index, struct shl_descriptor **desc);
extern int shl_gethandle (shl_t handle, struct shl_descriptor **desc);
#else
extern shl_t shl_load ();
extern int shl_findsym ();
extern int shl_unload ();
extern int shl_get ();
extern int shl_gethandle ();
#endif

extern struct dynamic _DYNAMIC;

#define		PROG_HANDLE	((shl_t)(&_DYNAMIC))

#endif		/* __hp9000s300 */

#ifdef 		__hp9000s800

/*
 *	dl.h
 *
 *	Series 800 dynamic loader
 *
 *	declarations and definitions for users
 *
 */


#include <sys/param.h>

typedef struct mapped_shl_entry *shl_t;

#define BIND_IMMEDIATE  0x0
#define BIND_DEFERRED   0x1
#define BIND_REFERENCE  0x2

#define BIND_FIRST	0x4
#define BIND_NONFATAL   0x8
#define BIND_NOSTART	0x10
#define BIND_VERBOSE	0x20
#define BIND_RESTRICTED	0x40  

/* #define BIND_RESERVED1	0x80  Reserve this bit for sharing between */
/* 			              shl_load() and shlib_list_entry{}    */

#define DYNAMIC_PATH	0x100

#define TYPE_UNDEFINED	0
#define TYPE_PROCEDURE	3
#define TYPE_DATA	2
#define TYPE_STORAGE	7

#define IMPORT_SYMBOLS	0x1
#define EXPORT_SYMBOLS  0x2

#define NO_VALUES	0x4
#define GLOBAL_VALUES   0x8

extern int __text_start;

#define	 PROG_HANDLE	((shl_t)(&__text_start))

/* Offsets into dldjmp table (__dld_loc) for the user-visible dld routines */

#define SHL_INIT   	0	
#define SHL_LOAD   	4
#define SHL_UNLOAD	8
#define SHL_FINDSYM	12
#define SHL_BOR		16
#define SHL_GET		20
#define SHL_TERM	24
#define SHL_GETHANDLE	28
#define SHL_DEFINESYM   32
#define SHL_GETSYMBOLS  36

#define NO_INITIALIZER 	((void *)(0))

struct shl_descriptor {
	unsigned long tstart;
	unsigned long tend;
	unsigned long dstart;
	unsigned long dend;
	void         *ltptr;
	shl_t	      handle;
	char filename[MAXPATHLEN+1];
	void         *initializer;
	unsigned long ref_count;
};

struct shl_symbol {
        char 	       *name;
        short 		type;
        void  	       *value;
        shl_t 		handle;
};

#if defined(__STDC__) || defined(__cplusplus)
extern shl_t shl_load (const char *path, int flags, long address);
extern int shl_findsym (shl_t *handle, const char *sym, short type,void *value);
extern int shl_unload (shl_t handle);
extern int shl_get (int index, struct shl_descriptor **desc);
extern int shl_gethandle (shl_t handle, struct shl_descriptor **desc);
extern int shl_definesym (const char *sym, short type, long value, int flags);
extern int shl_getsymbols (shl_t handle, short type, int flags, 
			   void *(*memory)(), struct shl_symbol **symbols); 
#else
extern shl_t shl_load ();
extern int shl_findsym ();
extern int shl_definesym ();
extern int shl_unload ();
extern int shl_get ();
extern int shl_gethandle ();
extern int shl_getsymbols ();
#endif

#endif		/* __hp9000s800 */

#ifdef __cplusplus
}
#endif

#endif		/* _DL_INCLUDED */


