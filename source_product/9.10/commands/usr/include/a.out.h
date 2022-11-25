/* @(#) $Revision: 70.1 $ */    

#ifndef _A_OUT_INCLUDED /* allow multiple inclusions */
#define _A_OUT_INCLUDED

#ifdef	__hp9000s300
/*
 *  HP 9000 Series 300
 *  Object File Format Definitions
 *
 *	Layout of a.out file :
 *	_________________________
 *	| header		|
 *	|_______________________|
 *	| text section		|
 *	|_______________________|
 *	| data section		|
 *	|_______________________|
 *	| pascal section	|
 *	|_______________________|
 *	| symbol table		|
 *	|_______________________|
 *	| supsym table		|
 *	|_______________________|
 *	| text relocation area	|
 *	|_______________________|
 *	| data relocation area	|
 *	|_______________________|
 *
 *	header:			0
 *	text:			sizeof(header)
 *	data:			sizeof(header)+textsize
 *	MIS:			sizeof(header)+textsize+datasize
 *	LEST:			sizeof(header)+textsize+datasize+ 
 *				MISsize 
 *	SUPSYM			sizeof(header)+textsize+datasize+
 *				MISsize+LESTsize
 *	text relocation:	sizeof(header)+textsize+datasize+
 *				MISsize+LESTsize+SUPSYMsize
 *	data relocation:	sizeof(header)+textsize+datasize+
 *				MISsize+LESTsize+VTsize+rtextsize
 *
 *	NOTE - header, data, and text are padded to a multiple of
 *	       EXEC_PAGESIZE bytes (using EXEC_ALIGN macro) in
 *	       demand-load files and shared/dynamic load libraries
 *
 */

/* header of a.out files */

#ifdef	KERNEL
#include	"../h/magic.h"
#else	/* KERNEL */
#include	<magic.h>
#include	<nlist.h>	/* included for all machines */
#include	<shl.h>
#include	<debug.h>
#endif	/* KERNEL */

struct exec {	
/*  0 */	MAGIC	a_magic;		/* magic number */
/*  4 */	short	a_stamp;		/* version id */
/*  6 */	short	a_highwater;		/* shlib highwater mark */
/*  8 */	long	a_miscinfo;		/* miscellaneous info */
/* 12 */	long	a_text;			/* size of text segment */
/* 16 */	long	a_data;			/* size of data segment */
/* 20 */	long	a_bss;			/* size of bss segment */
/* 24 */	long	a_trsize;		/* text relocation size */
/* 28 */	long	a_drsize;		/* data relocation size */
/* 32 */	long	a_pasint;		/* pascal interface size */
/* 36 */	long	a_lesyms;		/* symbol table size */
/* 40 */	long	a_spared;
/* 44 */	long	a_entry;		/* entry point */
/* 48 */	long	a_spares;
/* 52 */	long	a_supsym;		/* supplementary symtab size */
/* 56 */	long	a_drelocs;		/* nonpic relocations */
/* 60 */	long	a_extension;		/* file offset of extension */
};

#define	EXEC_PAGESIZE	4096	/* not always the same as the MMU page size */
#define	EXEC_PAGESHIFT	12	/* log2(EXEC_PAGESIZE) */
#define	EXEC_ALIGN(bytes)	(((bytes)+EXEC_PAGESIZE-1) & ~(EXEC_PAGESIZE-1))

# define SUPSYM_SIZE(hdr)	(hdr).a_supsym

# define TEXT_OFFSET(hdr)	(((hdr).a_magic.file_type == DEMAND_MAGIC || \
				(hdr).a_magic.file_type == SHL_MAGIC || \
				(hdr).a_magic.file_type == DL_MAGIC) ? \
				EXEC_ALIGN(sizeof(hdr)) : sizeof(hdr))

# define DATA_OFFSET(hdr) 	(((hdr).a_magic.file_type == DEMAND_MAGIC || \
				(hdr).a_magic.file_type == SHL_MAGIC || \
				(hdr).a_magic.file_type == DL_MAGIC) ? \
				EXEC_ALIGN(sizeof(hdr)) +        \
					EXEC_ALIGN((hdr).a_text) : \
				sizeof(hdr) + (hdr).a_text)

# define MODCAL_OFFSET(hdr)	(((hdr).a_magic.file_type == DEMAND_MAGIC || \
				(hdr).a_magic.file_type == SHL_MAGIC || \
				(hdr).a_magic.file_type == DL_MAGIC) ? \
				EXEC_ALIGN(sizeof(hdr)) +        \
					EXEC_ALIGN((hdr).a_text) + \
					EXEC_ALIGN((hdr).a_data) : \
				sizeof(hdr) + (hdr).a_text + (hdr).a_data)

# define LESYM_OFFSET(hdr)	MODCAL_OFFSET(hdr) + (hdr).a_pasint
# define SUPSYM_OFFSET(hdr)	LESYM_OFFSET(hdr) + (hdr).a_lesyms
# define RTEXT_OFFSET(hdr)	SUPSYM_OFFSET(hdr) + (hdr).a_supsym
# define RDATA_OFFSET(hdr)	RTEXT_OFFSET(hdr) + (hdr).a_trsize
# define EXT_OFFSET(hdr)	(hdr).a_extension

#ifndef	KERNEL

# define M_FP_SAVE	0x80000000
# define M_INCOMPLETE	0x40000000
# define M_DEBUG	0x20000000
# define M_PIC		0x10000000
# define M_DL		0x08000000
# define M_DATA_WTHRU	0x04000000
# define M_STACK_WTHRU	0x02000000
# define M_VALID	0x00000001

/* macros which define various positions in file based on an exec: filhdr */
#define TEXTPOS		TEXT_OFFSET(filhdr)
#define DATAPOS 	DATA_OFFSET(filhdr)
#define MODCALPOS	MODCAL_OFFSET(filhdr)
#define LESYMPOS	LESYM_OFFSET(filhdr)
#define SUPSYMPOS	SUPSYM_OFFSET(filhdr)
#define RTEXTPOS	RTEXT_OFFSET(filhdr)
#define RDATAPOS	RDATA_OFFSET(filhdr)
#define EXT_POS		EXT_OFFSET(filhdr)

/* symbol management */
struct nlist_ {		
	long	n_value;
	unsigned char	n_type;
	unsigned char	n_length;	/* length of ascii symbol name */
	short	n_almod;
	int	n_dlt:1;
	int	n_plt:1;
	int	n_dreloc:1;
	int	n_list:1;	/* End of list flag for supsym entries */
	int	n_unused:12;
};

struct supsym_entry
{
	union 
	{
		long size;	/* size of external data item */
		long next;	/* pointer to next symbol with this address */
	} a;
	long b;
};

#define	A_MAGIC1	FMAGIC       	/* normal */
#define	A_MAGIC2	NMAGIC       	/* read-only text */

/* relocation commands */
struct r_info {		/* mit= reloc{rpos,rsymbol,rsegment,rsize,rdisp} */
	long r_address;		/* position of relocation in segment */
	unsigned short r_symbolnum;	/* id of the symbol of external relocations */
	char r_segment;		/* RTEXT, RDATA, RBSS, REXTERN, or RNOOP */
				/* or RPC, RDLT, or RPLT */
	char r_length;		/* RBYTE, RWORD, or RLONG */
};

/* extension header */
struct header_extension
{
	union
	{
		long spare1[13];
		struct _dl_header dl_header;
		struct _debug_header debug_header;
	} e_spec;
	short e_header;		/* type of extension header */
	short e_version;	/* header format version number */
	long e_size;		/* aggregate size of related sections */
	long e_extension;	/* file offset of next extension header */
};

/* extension types */
#define		DL_HEADER	0x01
#define		DEBUG_HEADER	0x02


/* symbol types */
#define EXTERN2 0100    /* secondary global symbol */
#define	EXTERN	040	/* = 0x20 */
#define ALIGN	020	/* = 0x10 */	/* special alignment symbol type */
#define	UNDEF	00
#define	ABS	01
#define	TEXT	02
#define	DATA	03
#define	BSS	04
#define	COMM	05	/* internal use only */
#define REG	06

/* relocation regions */
#define	RTEXT	00
#define	RDATA	01
#define	RBSS	02
#define	REXT	03
#define	RPC	04
#define	RDLT	05
#define	RPLT	06
#define	RNOOP	077	/* no-op relocation  record, does nothing */

/* relocation sizes */
#define RBYTE	00
#define RWORD	01
#define RLONG	02
#define RALIGN	03	/* special reloc flag to support .align symbols */

	/* values for type flag */
#define	N_UNDF	0	/* undefined */
#define	N_TYPE	037
#define	N_FN	037	/* file name symbol */
#define	N_ABS	01	/* absolute */
#define	N_TEXT	02	/* text symbol */
#define	N_DATA	03	/* data symbol */
#define	N_BSS	04	/* bss symbol */
#define	N_REG	024	/* register name */
#define	N_EXT	040	/* external bit, or'ed in */
#define	FORMAT	"%06o"	/* to print a value */
#define SYMLENGTH	255		/* maximum length of a symbol */

/* These suffixes must also be maintained in the cc shell file */

#define OUT_NAME "a.out"
#define OBJ_SUFFIX ".o"
#define C_SUFFIX ".c"
#define ASM_SUFFIX ".s"
#define SHLIB_SUFFIX ".sl"

#define N_BADTYPE(x) (((x).a_magic.file_type)!=EXEC_MAGIC&&\
	((x).a_magic.file_type)!=SHARE_MAGIC&&\
	((x).a_magic.file_type)!=DEMAND_MAGIC&&\
	((x).a_magic.file_type)!=RELOC_MAGIC&&\
	((x).a_magic.file_type)!=SHL_MAGIC&&\
	((x).a_magic.file_type)!=DL_MAGIC)

#define N_BADMACH(x) ((x).a_magic.system_id!=HP9000S200_ID&&\
	(x).a_magic.system_id!=HP98x6_ID)
#define N_BADMAG(x)  (N_BADTYPE(x) || N_BADMACH(x))

#endif	/* KERNEL */
#endif	/* __hp9000s300 */

#ifdef __hp9000s800
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 *
 *  $Revision: 70.1 $
 */

#include "nlist.h"

#include "filehdr.h"
#include "aouthdr.h"
#include "spacehdr.h"
#include "scnhdr.h"
#include "initptr.h"
#include "compunit.h"
#include "reloc.h"
#include "syms.h"
#endif /* __hp9000s800 */

#endif /* _A_OUT_INCLUDED */
