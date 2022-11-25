/* @(#) $Revision: 70.3 $ */

#define MSUS	0xfffffedc	/* msus pointer */
#define	F_AREA	0xfffffed4	/* pointer to mb_ptr and mb_size */
#define	LOWRAM	0xfffffdce	/* lowest numbered byte of ram */
#define	SYSNAME 0xfffffdc2	/* location where bootname is stored */
#define HIGHRAM 0xfffffac0 	/* Highest ram location allowed */

#define	MB_PTR	0x10
#define	MB_SIZE	0x14

/*
** Space to reserve for KDB usage, normally 1/2 MB.  This value can not
** exceed 4MB (0x400000) and it MUST be a multiple of PAGESIZE (4096).
*/

/* 3 Mb.
#define KDB_SPACE (3*0x100000)
/**/

/* 1 Mb.
#define KDB_SPACE 0x100000
/**/

/* 1/2 Mb.
#define KDB_SPACE 0x080000
/**/

/* 640 Kb 
#define KDB_SPACE 0x0a0000
/**/

/* 1.5 MB  */
#define KDB_SPACE 0x180000
/**/

#define CALL(i) (*((int (*)())(i)))
#ifdef SDS
#define SDS_PSOPEN	*(int *)0xfff00002
#define SDS_PSCLOSE	*(int *)0xfff00006
#define SDS_PSREAD	*(int *)0xfff0000a
#define SDS_PSLSEEK	*(int *)0xfff00012
#endif /* SDS */
#define PSOPEN	*(int *)0xffff0802
#define PSCLOSE	*(int *)0xffff0806
#define PSREAD	*(int *)0xffff080a
#define PSLSEEK	*(int *)0xffff0812

#define PAGESIZE 4096

#define	SUPERVISOR	5	/* function code 5 = supervisor data */
#define USER		1	/* users space */
#define	INCDOTDOT(a) (dotdot + (a))
#define	inkdot(a) (dotdot + (a))

struct symentry {
	unsigned int value;
	unsigned long npEntry;
};

struct exception {
	unsigned short e_status;
	unsigned int   e_pc;
	unsigned short e_offset;
	unsigned short e_ssw;
	unsigned int   e_address;
	unsigned short e_stuff[22];
};

struct opdesc
{
	unsigned short mask, match;
	int (*opfun)();
	char *farg;
};

/*
** KDB's register save area.  This is where the kernel
** registers are saved when KDB is entered.
*/
extern short kdb_sr;		/* saved status register */
extern int kdb_pc;		/* saved program counter */
extern int kdb_vbr;		/* saved vector base offset */
extern int kdb_usp;		/* saved user stack pointer */
extern int kdb_kernelregs[];
extern int kdb_dregs[8];	/* saved data registers */
extern int kdb_aregs[8];	/* saved address registers */
extern int kdb_fregs[2];	/* saved function code registers */
extern unsigned short kdb_mregs[3];	/* saved mmu registers */

extern int kdb_saved_bus;
extern int kdb_saved_addr;
extern int kdb_saved_trap14;
extern int kdb_saved_trap15;
extern int kdb_saved_trace;
