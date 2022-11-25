/* @(#) $Revision: 70.5 $ */     

/* Because of "asm" statements: */
#pragma OPT_LEVEL 1

#include "Kreg.h"
#include "Kdb.h"
#include "Ktty.h"
#include <a.out.h>
#include "cdb.h"
#include "macdefs.h"
#include <machine/cpu.h>
#include <machine/pte.h>
#ifdef CDBKDB
#include <fcntl.h>
int	vpc = 0;
#endif /* CDBKDB */

#  define MINISC                0
#  define EXTERNALISC           32
#  define MAXEXTERNALISC        63
#  define MIN_DIOII_ISC         132
#  define MAXISC                255
#  define ISC_TABLE_SIZE        (MAXISC + 1)

#  define DIOII_START_ADDR      0x1000000
#  define DIOII_SC_SIZE         0x400000
#  define START_ADDR (0x600000 + LOG_IO_OFFSET)

typedef int     (*Fint)();

extern char *vsbSymfile;
#ifdef KASSERT
extern FLAGT vfRunAssert;
#endif
#ifndef CDBKDB
extern int kdb_tick_count;
extern int mapper_off;
#endif /* CDBKDB */

extern char *kdbgetline();
extern struct exception kdb_exception;
extern char kdb_interrupt;
extern int kdb_processor, hpux_processor;
extern struct opdesc opdecode[];
#ifndef CDBKDB
extern char kdb_tos[];
#endif /* CDBKDB */
extern int end;
extern int kdb();
extern int kdbstart();
extern int nprintf();
extern int nscanf();
extern int ngets();
extern int crtmsg();
extern int dbg_addr_present;
extern int dbg_addr;
#ifndef CDBKDB
extern int kdb_loadpoint;
#endif /* CDBKDB */
extern int pmmu_exist;

struct exec execSym;		/* a.out header of kernel file */
struct header_extension ext_header;	/* a.out extended header (/w debuf info) */
int loadpoint;			/* physical addr kernel loaded at */

/* run the kernel as if it had this much physmem */
export unsigned int memsize = 0;

char Kname[25];			/* contains name of kernel file */
#ifdef KDBKDB
char Kfile[16]="";		/* kernel file to auto-load */
#else
char Kfile[16]="hp-ux";		/* kernel file to auto-load */
#endif

int npcis =  27000;		/* number of loops on pci_check */
static int nobp = 0x880000;	/* an address that will never have a bp */
static int first_time;
static int no_tick_count;	/* use this addr if tick count not found */

int goVal;		/* global boot control flag, set by Boot. 
			 * 0 -- Load kernel, do not run it
			 * 1 -- Load kernel, run it until map turned on
			 * 2 -- Load and Boot kernel
			 */
int AdrSysmap;
int AdrU;
int Adrkpte;
int TextPgCnt;

export int noprotect;	/* global debugger variable (like scroll); default==0
			 * 0 - protect kernel text when exiting to HP-UX.
			 * 1 - keep text pages writeable when exiting to HP-UX.
			 */
export int vimap, vpid;

int	vrgOffset[] = {
	R0,  R1,  R2,  R3,
	R4,  R5,  R6,  R7,
	AR0, AR1, AR2, AR3,
	AR4, AR5, AR6, AR7,
	PC,  RPS, AR6, SP,
	USP
	};

#ifndef CDBKDB
int vcbKdbMem  = (int)kdbstart;
#endif /* CDBKDB */
int kdb_trace;
int bss_addr;
char badboot[] ="old secondary loader--cannot boot SYSDEBUG";
char sdsbadboot[] ="sds old secondary loader--cannot boot SYSDEBUG";

#define pKrev &sccsid[4]

static	MODER	amode;		/* instance of struct	*/
export	pMODER	vmode;		/* a ptr to it		*/

export int display_headers;
#ifdef CDBKDB
export int sym_tab_size;

kdb_purge()
{
}

/***********************************************************************
 * m a i n
 *
 * Entry point if compiled with CDBKDB set
 */
main(argc,argv)
int argc;
char *argv[];
{

	display_headers = YesNo("display headers? ");

	if (display_headers)
		printf("\na.out header information for file: %s\n\n\n", argv[1]);
	
	InitAll();

	if (boot(argv[1], &execSym) == -1)
		exit(-1);

	bss_addr = (int)Malloc(execSym.a_lesyms,"create BSS space");
	sym_tab_size = execSym.a_lesyms;

	Kinit();
	close(vfnSym);

	DebugIt();
}

#else /* CDBKDB */

/***********************************************************************
 * R E L O C A T E
 *
 * This is the routine that gets called to relocate the kernel
 * debugger. Be very careful with this routine--it must run
 * completely pc-relative
 */

#ifdef SDS
relocate(addr_present,addr,entry,junk1,junk2,loadpoint,tsize,dsize,bsize,proc)
    unsigned long addr_present, addr, entry, junk1, junk2, loadpoint, 
		  tsize, dsize, bsize, proc;
#else /* ! SDS */
relocate(addr_present,addr,entry,junk1,junk2,loadpoint,tsize,dsize,bsize)
    unsigned long addr_present, addr, entry, junk1, junk2, loadpoint, 
		  tsize, dsize, bsize;
#endif /* else ! SDS */
{
    register int io;
    register int i;
    register int kloadpoint;
    register int whereoff;
    register char *m1ptr, *m2ptr;
    register struct r_info *rtable;
    char filename[16];
    Fint newentry;
    long osize = tsize + dsize;
    struct exec execkdb;
    register struct exec *execp = &execkdb;
    int relsize;
    int roffset;
    long *where;

    /* Make sure that this system has the right secondary loader */

#ifdef SDS
    if (proc == 0xdeadbeef) {
	    if ( (SDS_PSOPEN & 0xfff00000) != 0xfff00000) {	
                i = loadpoint+(int)crtmsg;
		CALL(i)(3,loadpoint+(int)sdsbadboot);
		goto badload;
	    }
    } else {
	    if ( (PSOPEN & 0xffff0000) != 0xffff0000) {	
                i = loadpoint+(int)crtmsg;
		CALL(i)(3,loadpoint+(int)badboot);
		goto badload;
	    }
    }
#else /* not SDS */
    if ( (PSOPEN & 0xffff0000) != 0xffff0000)
    {	i = loadpoint+(int)crtmsg;
	CALL(i)(3,loadpoint+(int)badboot);
	goto badload;
    }
#endif /* else not SDS */

    /*
     * The following song and dance is to ensure that the bootstrap does not
     * overwrite bootrom driver data areas in lowram.  See the bootrom manual
     * for details.
     */

    if (addr_present)
    {	kloadpoint = addr;

	/* Copy KDB */

	m1ptr = (char *)loadpoint;
	m2ptr = (char *)kloadpoint;
	for (i = 0; i < osize; i++)
		*m2ptr++ = *m1ptr++;
    }
    else
    {
	kloadpoint = loadpoint;
    }

    /* Compute new entry point */

    newentry = (Fint)(kloadpoint + entry);

    /* open the kernel debugger file */
	m1ptr = filename;
	*m1ptr++ = '/';
	/* SYSNAME is limited by reboot to 10 bytes */
	for (m2ptr = (char *)SYSNAME, i=0; i<10; i++, m1ptr++)
	{	*m1ptr = *m2ptr++;
		if (*m1ptr == ' ')	/* SRM strings are blank-terminated */
			*m1ptr = '\0';
	}

#ifdef SDS
	if (proc == 0xdeadbeef) {
		if ((io = CALL(SDS_PSOPEN)(filename)) < 0) {
			goto badopen;
		}
	} else {
		if ((io = CALL(PSOPEN)(filename)) < 0) {
			goto badopen;
		}
	}
#else /* ! SDS */
	if ((io = CALL(PSOPEN)(filename)) < 0) {
		goto badopen;
	}
#endif /* else ! SDS */

#ifdef SDS
	if (proc == 0xdeadbeef)
		i = CALL(SDS_PSREAD)(io, execp, sizeof(struct exec));
	else
		i = CALL(PSREAD)(io, execp, sizeof(struct exec));
#else /* ! SDS */
	i = CALL(PSREAD)(io, execp, sizeof(struct exec));
#endif /* else ! SDS */

	if (i != sizeof(struct exec) ||
	   ((execp->a_magic.file_type != RELOC_MAGIC) &&
	    (execp->a_magic.file_type != EXEC_MAGIC)) ){
		goto badmagic;
	}

	relsize = execp->a_trsize + execp->a_drsize;

	/* Seek to beginning of text relocation table (This code assumes that */
	/* the text relocation table and the data relocation table are        */
	/* contiguous.                                                        */

	roffset = RTEXT_OFFSET(execkdb);
#ifdef SDS
	if (proc == 0xdeadbeef) {
		if (CALL(SDS_PSLSEEK)(io,roffset,0) != 0) {
			goto badseek;
		}
	} else {
		if (CALL(PSLSEEK)(io,roffset,0) != 0) {
			goto badseek;
		}
	}
#else /* ! SDS */
	if (CALL(PSLSEEK)(io,roffset,0) != 0) {
		goto badseek;
	}
#endif /* else ! SDS */

	/* read the relocation records into the unused bss segment */
	rtable = (struct r_info *)(loadpoint + osize);
#ifdef SDS
	if (proc == 0xdeadbeef) {
		if (CALL(SDS_PSREAD)(io,rtable,relsize) != relsize)
			goto badread;
	} else {
		if (CALL(PSREAD)(io,rtable,relsize) != relsize)
			goto badread;
	}
#else /* ! SDS */
	if (CALL(PSREAD)(io,rtable,relsize) != relsize)
		goto badread;
#endif /* else ! SDS */
	
#ifdef SDS
	if (proc == 0xdeadbeef)
		CALL(SDS_PSCLOSE)(io);
	else
		CALL(PSCLOSE)(io);
#else /* ! SDS */
	CALL(PSCLOSE)(io);
#endif /* else ! SDS */

    /* Relocate KDB */

    whereoff = kloadpoint;
    for (i = 0; i < relsize; i += sizeof(struct r_info), rtable++) {

	if (i == execp->a_trsize) 
	    whereoff += execp->a_text;

	if (rtable->r_segment == RNOOP) 
	    continue;

	if (rtable->r_segment == RPC) 
	    continue;

	/* check to make sure it is long. It should be, but if it isn't */
	/* then flag it as an error.                                    */

	if (rtable->r_length != RLONG)
	    goto badlen;
	
	/* abort on records we don't know about */
	if (rtable->r_segment != RTEXT && rtable->r_segment != RDATA && rtable->r_segment != RBSS && rtable->r_segment != REXT) 
	    goto badrecord;

	/* Do the relocation */
	where = (long *)(whereoff + rtable->r_address);
	*where += kloadpoint;
    }

#ifdef SPECIAL_LD
	/* This section should not be needed once we get an archive	*/
	/* libc.a which is free of PIC					*/

    /* relocate another table given to us by special "+r" version of ld */
    /*   a_spared is the address of the beginning of this table		*/
    /*   a_spares is the size of the table				*/

    where = (long *)(execp->a_spared + kloadpoint);
    for (i=0; i<execp->a_spares; i++)
	where[i] += kloadpoint;
#endif /* SPECIAL_LD */


    (void) newentry();
    return;
bad:
    asm(" mov.l	&2,%d0");
    asm(" lea mytest1(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
/*    asm("mytest1:  asciz \"Cannot relocate debugger\" "); */
    asm("mytest1:  long 0x43616e6e,0x6f742072,0x656c6f63,0x61746500");

badopen: /* 1 */
    asm(" mov.l	&2,%d0");
    asm(" lea mytest3(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("mytest3:  long 0x3143616e,0x27742072,0x656c6f63,0x61746500");

badmagic: /* 2 */
    asm(" mov.l	&2,%d0");
    asm(" lea mytest4(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("mytest4:  long 0x3243616e,0x27742072,0x656c6f63,0x61746500");

badseek: /* 3 */
    asm(" mov.l	&2,%d0");
    asm(" lea mytest5(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("mytest5:  long 0x3343616e,0x27742072,0x656c6f63,0x61746500");

badread: /* 4 */
    asm(" mov.l	&2,%d0");
    asm(" lea mytest6(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("mytest6:  long 0x3443616e,0x27742072,0x656c6f63,0x61746500");

badload: /* 5 */
    asm(" mov.l	&2,%d0");
    asm(" lea mytest7(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("mytest7:  long 0x3543616e,0x27742072,0x656c6f63,0x61746500");

badlen: /* 6 */
    asm(" mov.l	&2,%d0");
    asm(" lea mytest8(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("mytest8:  long 0x3643616e,0x27742072,0x656c6f63,0x61746500");

badrecord: /* 7 */
    asm(" 	global _mytest9");
    asm(" mov.l	&2,%d0");
    asm(" lea _mytest9(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("_mytest9:  long 0x3743616e,0x27742072,0x656c6f63,0x61746500");

badext: /* 8 */
    asm(" mov.l	&2,%d0");
    asm(" lea mytesta(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("mytesta:  long 0x3843616e,0x27742072,0x656c6f63,0x61746500");

badleng: /* 9 */
    asm(" mov.l &2,%d0");
    asm(" lea mytestb(%pc),%a0");
    asm(" jsr 0x150.w");
    asm(" bra junk1");
    asm("mytestb:  long 0x39616e6e,0x6f742072,0x656c6f63,0x61746500");

    asm("junk1: ");
    while (1);
}

extern int current_io_base;

/*
 * List of select codes to search for the console.
 * The first entry is duplicated so that the user may overwrite it
 * with his own favority select code.
 */
int kdbselcode[] = {9,9,5,6,0};		/* select codes to search */
int id;
int testrret;
unsigned int saveaddr;

#define ID626   0x2
#define ID644   0x42

find_kdbtty()
{
	int i;
	unsigned addr;

	/* 
	 *	Try user optional select code first.
	 *	Then try standard 9,5,6.
	 */
	i = 0 ;
	do {
		switch (kdbselcode[i]) {
		case 5:
			addr = 0x41C040 + current_io_base;
			/* Apollo utility chip */
			kdb_tty.kdbt_type = APOLLO_UART;
			if (kdb_testr(addr))
				goto good;
			break ;
		case 6:
			addr = 0x41C060 + current_io_base;
			/* Apollo utility chip */
			kdb_tty.kdbt_type = APOLLO_UART;
			if (kdb_testr(addr))
				goto good;
			break ;
/*
 *
 *	If there is an apollo UART in this machine, the kdb console can
 *	be on the apollo UART at select code 9 or on the builtin
 *	UART at select code 9. The call to kdb_testr will be OK for
 *	both addresses. There is really no way to determine
 *	which of these addresses the kdb console is actually hooked up to. So,
 *	default select code 9 to be the HP builtin. Select code 9 on the
 *	apollo UART will not work with kdb. (RPC, 11/5/93)
 */
		case 9:
		default:
			kdb_tty.kdbt_type = HP_UART;
			addr = 0x600000 +(kdbselcode[i] << 16) +current_io_base;
			if (kdb_testr(addr + 1)) {
				id = *(char *)(addr+1) & 0x7F;
				if (id == ID626 || id == ID644)
					goto good;
			}
			break ;
		}
	} while (kdbselcode[++i] != 0) ;

	Panic("no tty");

good:	kdb_tty.kdbt_addr = (struct kdbpci *)addr;
}

/***********************************************************************
 * K D B  M A I N
 *
 * This is the routine that first gets control after the assembly
 * language routines are finished saving the kernel's context
 */

kdb_main(flag)
{	
	set_space(SUPERVISOR);

	switch (flag) {
	case 1:
		find_kdbtty();
		kdbttyinit(&kdb_tty);

		InitAll();
		if ( kdb_processor == 0 &&	/* 310 must use memory	*/
		     (int)kdbstart != dbg_addr )	/* at 0x880000	*/
		{	printf("Debugger for 310 must run at 0x880000\n");
			crtmsg(4,"Debugger for 310 must run at 0x880000\n");
			while (1);
		}
		printf("\n%s\n",pKrev);
		if (Kfile[0]) {
			int i, count, c;

			printf("Auto-booting %s: ",Kfile);
			printf("[space] to abort, 'b' to boot now\n");

			for (count=10; count>0; count--) {
#if WE_GET_THIS_TO_WORK
				for (i=76; i>=(10-count)*3; i--) {
					strout("\033&a");
					printf("%dC%d ", i, count);
					if (c = pci_check()) {
						printf("Top, c='%c'\n", c);
						goto out;
					}
				}
#else
				printf("%d ", count);
#endif
				for (i=npcis; --i>=0; ) {
					if (c = pci_check())
						goto out;
				}
			}
			strout("0 BLASTOFF!");
			for (count=10000; count>0; count--)
				kdb_purge();
			out:
			strout("\r\033K");
			switch (c) {
			case '0': Boot(Kfile,0); break;	/* like 0R command */
			case '1': Boot(Kfile,1); break;	/* like 1R command */
			case '\0':			/* timeout, like 'b' */
			case 'b':			/* Boot the system */
			case '2': Boot(Kfile,2); break;	/* like 2R command */
			case 'B': Boot("SYSBCKUP",2); break;
			}
		}
		break;
	case 0:
		find_kdbtty();
		/* The first time this is run is when the kernel gives KCDB
		 * control after turning on mapping. Continuing here will
		 * finish booting up the kernel
		 */
		if (first_time == 0) {	
			int iobase;
			
			iobase = current_io_base + PHYS_IO_BASE;
			if (iobase == PHYS_IO_BASE)
				printf("Internal I/O is mapped logical = physical at 0x%x\n", iobase);
			else
				printf("Internal I/O is mapped at 0x%x\n", iobase);

			first_time = 1;
			if (goVal > 1)
			{
				printf("continuing--booting up kernel\n");
				kdb_cont();
			}
		}
	default:
	 	IbpFBrkOut(&nobp);
		break;
	case 8:
		Panic("\nBUS ERROR at %x, access address = %x\n",
			kdb_exception.e_pc,kdb_exception.e_address);
		IbpFBrkOut(&nobp);
		break;
	case 12:
		Panic("\nADDRESS ERROR at %x, access address = %x\n",
			kdb_exception.e_pc,kdb_exception.e_address);
		IbpFBrkOut(&nobp);
		break;
	}	
#ifdef KASSERT
	vfRunAssert = true;
#endif
	DebugIt();
}
#endif /* CDBKDB */

/***********************************************************************
 * I N I T  A L L
 *
 * Initialize stuff for the debugger at powerup.
 */

int InitAll()
{
    vsbSymfile	= Kname;

    vilvMax = 26;			
    viadMax = 16;			

    vmode	= &amode;
    vmode->cnt	= 1;		
    vmode->len	=  32	  / 8;	
    vmode->df	=  dfHex;
    vmode->imap = 0;

    vibpMac  = 1;		

    InitSpc();			

#ifndef CDBKDB
    /* stash string at bottom of stack so you can tell if it has been 
       overwritten */
    strcpy(kdb_tos,"Stack is OK\n");
#endif /* CDBKDB */
} 

#ifndef CDBKDB
/***********************************************************************
 * B O O T
 *
 * Boot a kernel, starting it running in most cases.
 */

Boot(sbArgs,lgoVal)
char *sbArgs;
{
	register int *ap;

	goVal = lgoVal;
	Kname[0]='/';
	if (sbArgs == 0)
	{	strcpy(&Kname[1],"hp-ux");
	}
	else
	{
		if (sscanf(sbArgs," %20s",&Kname[1]) != 1)
		{	UError("error in arguments");
		}
	}

	loadpoint = boot(Kname,&execSym);

	if (loadpoint != -1) {	
		Kinit();
		close(vfnSym);
		kdb_pc = execSym.a_entry;
		kdb_aregs[7] = 0xfffffac0 - 0x38; 
		ap = (int *) (0xfffffac0 - 0x38);
		*ap++ = 1;		/* this is a relocating kcdb */
		*ap++ = (int)kdb;	/* this is the entry point of kcdb */
		*ap++ = loadpoint;
		/* spares contains the rounded text size */
		*ap++ = execSym.a_spares;
		*ap++ = execSym.a_data;
		*ap++ = execSym.a_bss;
#ifdef SDS
		*ap++ = hpux_processor;
#else /* ! SDS */
		*ap++ = kdb_processor;
#endif /* else ! SDS */
		*ap++ = KDB_SPACE / PAGESIZE;   /* # of pages to map for KDB, */
		*ap++ = (int)kdbstart;		/* the maximum is 1024 pages  */
		*ap++ = (int)nprintf;
		*ap++ = (int)nscanf;
		*ap++ = (int)ngets;
		kdb_aregs[1] = loadpoint;
		kdb_sr = 0x2700;

		TextPgCnt = execSym.a_spares / PAGESIZE;
		if (TextPgCnt <= 0)
			printf("\n\tERROR: There are no kernel text pages!\n");

		AdrSysmap = AdrFLabel("Sysmap");
		Adrkpte = AdrFLabel("ktext_ptes");

		if  ((AdrSysmap <= 0) && (Adrkpte <= 0)) {
			printf("\nERROR: Neither the Sysmap or ktext_ptes symbols can be found!\n");
			printf("Kernel has probably been stripped.\n");
			printf("SYSDEBUG will hang the system if the kernel text is write protected.\n\n");
		}

		AdrU = AdrFLabel("u");
#ifndef CDBKDB
		printf("kdb loadpoint: 0x%x  kernel loadpoint: 0x%x\n", kdb_loadpoint, loadpoint);
#endif /* CDBKDB */
/*
		printf("kdb processor: 0x%x  pmmu_exist: 0x%x\n", kdb_processor, pmmu_exist);
*/

		if (goVal > 0)
			kdb_cont();
		else
			printf("WARNING: mapping not turned on\n");
	}
}
#endif /* CDBKDB */

boot(filename,ap)
char *filename;
register struct exec *ap;
{
	register int io;
	register int loadpoint;
	register char *addr;
	register int i;
	int msus;
	register unsigned long offset;
	int fpos;
#ifndef CDBKDB
#ifdef KDBKDB
 	char *cp;

 	/* Put the filename into SYSNAME */
 	/* start at one since filename[0] is a slash */
 
 	for (i=1, io=0, cp = (char *)SYSNAME; i != 10; i++, cp++)
 	{	*cp = filename[i];
 		if (*cp == '\0' || io)
 		{	*cp = ' ';
 			io = 1;
 		}
 	}
#endif
 
	msus = *((int *)MSUS);
	printf("booting %s on 0x%x\n",filename,msus);
#ifdef KMINIT
	io = *((int *) F_AREA);
	*((int *) (io + MB_PTR)) = *((int *) LOWRAM);
	i = call_bootrom(MINIT, msus);
	if (i != 0) {
		printf("cannot initialize boot device\n");
		return(-1);
	}
#endif

	/* Compute loadpoint for kernel (use kloadpoint from Krelocate.c ) */

	if (dbg_addr_present)
	{
/*
 * The following song and dance is to ensure that the bootstrap does not
 * overwrite bootrom driver data areas in lowram.  See the bootrom manual
 * for details.
 */
		io = *((int *)F_AREA);
		io = *((int *) (io + MB_SIZE));
		loadpoint = ((*((int *) LOWRAM) + io + PAGESIZE-1) & ~(PAGESIZE-1));
		loadpoint = (loadpoint + PAGESIZE-1) & ~(PAGESIZE-1);
	}
	else
	{
		loadpoint = ((int)kdbstart+KDB_SPACE+PAGESIZE-1) & ~(PAGESIZE-1);
		loadpoint = (loadpoint + PAGESIZE-1) & ~(PAGESIZE-1);
	}
#endif /* not CDBKDB */

	if (memsize) {
		if (memsize > (0 - loadpoint)) {
			printf("memsize too large, ignored: memsize = ");
			printf("0x%x, physical ram size = 0x%x\n", 
						memsize, (0 - loadpoint));
			memsize = 0;
		} else {
			printf("WARNING: memory restricted to 0x%x bytes\n", 
								memsize);
		}
	}


	/* determine how much memory we will run in */
	if (!memsize)
		memsize = 0 - loadpoint;

	/* set loadpoint based on memsize */
	loadpoint = 0 - memsize;

	/* round up to next page boundry */
	loadpoint = (loadpoint + PAGESIZE-1) & ~(PAGESIZE-1);

#ifdef CDBKDB
	if ((io = open(filename, O_RDONLY)) < 0) {
#else /* CDBKDB */
	if ((io = opencdf(filename, 0)) < 0) {
#endif /* CDBKDB */
		printf("error on open\n");
		return(-1);
	}
	vfnSym = io;				/* cdb code uses this */
	fpos = 0;

	i = read(io, ap, sizeof(struct exec));
	fpos += i;

	if (i < sizeof(struct exec)) {
		printf("Bad a.out format\n");
		printf("number of bytes read = %d\n", i);
		close(io);
		return(-1);
	}

	if (ap->a_magic.file_type != SHARE_MAGIC &&
	    ap->a_magic.file_type != EXEC_MAGIC) {
		printf("Bad a.out format\n");
		printf("ap->a_magic.file_type = %d\n", ap->a_magic.file_type);
		printf("SHARE_MAGIC           = %d\n", SHARE_MAGIC);
		printf("EXEC_MAGIC            = %d\n", EXEC_MAGIC);
		close(io);
		return(-1);
	}

#ifdef CDBKDB
	if (display_headers) {
	    printf("exec.a_magic     (magic number)\n");
	    printf("            .system_id                           = %d\n",ap->a_magic.system_id);
	    printf("            .file_type                           = %d\n",ap->a_magic.file_type);
	    printf("exec.a_stamp     (version id)                    = %d\n", ap->a_stamp);
	    printf("exec.a_highwater (shlib highwater mark)          = %d\n", ap->a_highwater);
	    printf("exec.a_miscinfo  (miscellaneous info)            = %d\n", ap->a_miscinfo);
	    printf("exec.a_text      (size of text segment)          = %d\n", ap->a_text);
	    printf("exec.a_data      (size of data segment)          = %d\n", ap->a_data);
	    printf("exec.a_bss       (size of bss segment)           = %d\n", ap->a_bss);
	    printf("exec.a_trsize    (text relocation size)          = %d\n", ap->a_trsize);
	    printf("exec.a_drsize    (data relocation size)          = %d\n", ap->a_drsize);
	    printf("exec.a_pasint    (pascal interface size)         = %d\n", ap->a_pasint);
	    printf("exec.a_lesyms    (symbol table size)             = %d\n", ap->a_lesyms);
	    printf("exec.a_spared    (spared)                        = %d\n", ap->a_spared);
	    printf("exec.a_entry     (entry point)                   = %d\n", ap->a_entry);
	    printf("exec.a_spares    (spares)                        = %d\n", ap->a_spares);
	    printf("exec.a_supsym    (supplementary symtab size)     = %d\n", ap->a_supsym);
	    printf("exec.a_drelocs   (nonpic relocations)            = %d\n", ap->a_drelocs);
	    printf("exec.a_extension (file offset of extension)      = %d\n", ap->a_extension);
	}
#endif /* CDBKDB */

#ifndef CDBKDB
	if (read(io, (char *) loadpoint, ap->a_text) != ap->a_text)
		goto shread;
	fpos += ap->a_text;

	addr = (char *) (loadpoint + ap->a_text);

	if (ap->a_magic.file_type == SHARE_MAGIC)
		while ((int)addr & (PAGESIZE-1))
			*addr++ = 0;

	if (read(io, addr, ap->a_data) != ap->a_data)
		goto shread;
	fpos += ap->a_data;

	addr += ap->a_data;
	bss_addr = (int)addr;

	ap->a_entry = loadpoint + ap->a_entry;

	/* if it is a shared file (always is?) we need to bump the size
	   of the text segment. Use a spare field in the a.out struct to
	   pass this back.
	*/
	ap->a_spares = ap->a_text;
	if (ap->a_magic.file_type == SHARE_MAGIC)
	{	ap->a_spares += PAGESIZE-1;
		ap->a_spares &= ~(PAGESIZE-1);
	}

/*
 * Read in the extension headers to get at the debug information.
 */
	offset = ap->a_extension;
	while (offset) {
		int dbg;
		if ((dbg = seekread(io, offset, &ext_header, sizeof(ext_header), "extension header")) == -1) {
	
			offset = 0;
			break;
		}
		if (ext_header.e_header == DEBUG_HEADER) {
			break;
		}
		offset = ext_header.e_extension;
	}
	if (!offset)
		printf("No debug information in kernel.\n");

	return(loadpoint);

shread:
	printf("read error\n");
	close(io);
	return(-1);

#else /* CDBKDB */
	return(0);
#endif /* CDBKDB */

}

_rtt()
{}
/***********************************************************************
 * K I N I T
 *
 * Initialize stuff for this kernel after it has been loaded.
 */

Kinit()
{	
	vipxdbheader	= ext_header.e_spec.debug_header.header_offset;
	vipdfdmd	= ext_header.e_spec.debug_header.gntt_offset;
	vcbSymFirst	= ext_header.e_spec.debug_header.lntt_offset;
	visymMax	= ext_header.e_spec.debug_header.lntt_size / cbSYMR;
	vcbSltFirst	= ext_header.e_spec.debug_header.slt_offset;
	visltMax	= ext_header.e_spec.debug_header.slt_size / cbSLTR;
	vcbSbFirst	= ext_header.e_spec.debug_header.vt_offset;
	vcbSbCache	= ext_header.e_spec.debug_header.vt_size;
	vcbLstFirst = LESYM_OFFSET(execSym);

#ifdef CDBKDB

#define DBG	ext_header.e_spec.debug_header

	if (display_headers) {
	printf("\n\n_debug_header information:\n\n");
	printf("dbg.header_offset (offset for xdb/cdb hdr tbl)   = %d\n", DBG.header_offset);
	printf("dbg.header_size	 (length of xdb/cdb header tbl)	 = %d\n", DBG.header_size);
	printf("dbg.gntt_offset	 (file offset for GNTT)		 = %d\n", DBG.gntt_offset);
	printf("dbg.gntt_size	 (length of GNTT)		 = %d\n", DBG.gntt_size);
	printf("dbg.lntt_offset	 (file offset for LNTT)		 = %d\n", DBG.lntt_offset);
	printf("dbg.lntt_size	 (length of LNTT)		 = %d\n", DBG.lntt_size);
	printf("dbg.slt_offset	 (file offset for SLT)		 = %d\n", DBG.slt_offset);
	printf("dbg.slt_size	 (length of SLT)		 = %d\n", DBG.slt_size);
	printf("dbg.vt_offset	 (file offset for VT)		 = %d\n", DBG.vt_offset);
	printf("dbg.vt_size	 (length of VT)			 = %d\n", DBG.vt_size);
	printf("dbg.xt_offset	 (file offset for XT)		 = %d\n", DBG.xt_offset);
	printf("dbg.xt_size	 (length of XT)			 = %d\n", DBG.xt_size);
	printf("dbg.spare1	 (for future expansion)		 = %d\n", DBG.spare1);
	}
#endif /* CDBKDB */

	InitCache(ext_header.e_spec.debug_header.lntt_size);
	InitLst(execSym.a_lesyms);
	InitSlt(ext_header.e_spec.debug_header.slt_size);
	InitSs();
	InitFdPd();			
#ifdef KASSERT
	InitAssert();		
#endif

	vpid = 1;
#ifndef CDBKDB
	kdb_tick_count = AdrFLabel("tick_count");
	if (kdb_tick_count == 0)
	{	printf("tick count not found--kdb will not run in zero time\n");
		kdb_tick_count = (int)&no_tick_count;
	}
#endif /* CDBKDB */
}

/***********************************************************************
 *
 * This next set of routines is used to access the kernel registers as
 * well as kernel and user memory
 *
 **********************************************************************/

GetReg(reg)
{
#ifndef CDBKDB
	return kdb_kernelregs[vrgOffset[reg]];
#else /* CDBKDB */
	printf("*** GetReg called, reg = %d\n", reg);
	return 0;
#endif /* CDBKDB */
}

PutReg(reg,val)
{
#ifndef CDBKDB
	kdb_kernelregs[vrgOffset[reg]] = val;
#else /* CDBKDB */
	printf("*** PutReg called, reg = %d, val = %d\n", reg, val);
#endif /* CDBKDB */
}

GetByte(adr)
{
	return fetch(adr,1);
}

PutByte(adr,space,val)
char *adr, val;
{
	*adr = val;
}

GetWord(adr)
{
	return fetch(adr,4);
}

PutWord(adr,space,val)
int *adr;
{
	*adr = val;
}

GetBlock(adr,space,adrValue,cb)
register char *adr, *adrValue;
register int cb;
{
	register int i;
	for (i = 0; i < cb; i++)
		*adrValue++ = fetch(adr++,1);
}

PutBlock(adr,space,adrValue,cb)
register char *adr, *adrValue;
register int cb;
{
	register int i;
	for (i = 0; i < cb; i++)
		*adr++ = *adrValue++;
}

fetch(adr,len)
{	
#ifdef CDBKDB
	switch (len) {
		case 1: return 0x7f;
		case 2: return 0x7fff;
		case 4: return 0x7fffffff;
		}
	return -1;
#else /* CDBKDB */
	register int user_data;
	if (vimap)
	{
		set_space(USER);
		user_data = Ufetch(adr,len);
		set_space(SUPERVISOR);
		return user_data;
	}
	else
	{
		switch (len) {
		case 1: return *(unsigned char *) adr;
		case 2: return *(unsigned short *) adr;
		case 4: return *(unsigned long *) adr;
		}
		return -1;
	}
#endif /* CDBKDB */
}

#ifndef CDBKDB
/***********************************************************************
 *
 * The following set of routines replace system calls that cdb used.
 * Some of the routines are not used but are here to appease the linker
 * when it links in libc.a
 *
 **********************************************************************/

ptrace(request)
{
	switch (request) {
	case 9:
		if (single_step()) return 1;
		break;
	case 7:
		kdb_cont();
		break;
	}
	vdot = kdb_pc;
	CopyTy(vtyDot,vtyCnInt);
	return 0;
}

#define TRACE_BIT 0x8000

single_step()
{
	register struct opdesc *p;
	register short *addr = (short *)vpc;

	for (p = opdecode; p->mask; p++)
		if ((*addr & p->mask) == p->match)
			break;

	if (p->mask == 0) {
		UError("Illegal opcode %02x at ", *addr);
	} else if ((kdb_sr & 0x2000) == 0) {
		printf("Cannot step into user space\n");
		return(1);
	} else {
		kdb_sr |= TRACE_BIT;
		kdb_trace = 1;
		kdb_cont();
		kdb_sr &= ~TRACE_BIT;
		kdb_trace = 0;
	}
	return(0);
}

ADRT _curbrk = (int)(&end);		/* used by _sbrk and __brk	*/
					/* and referenced by malloc(3C)	*/

_sbrk(incr)
register int incr;
{
	register ADRT old_brk = _curbrk;
	register char* cp;

#ifdef DEBUG
	printf("sbrk: incr is %d, _curbrk is 0x%x\n",incr,_curbrk);
#endif
	if (((_curbrk += incr) >= (vcbKdbMem + KDB_SPACE)) || (_curbrk < (int)(&end)))
	{	_curbrk = old_brk;
#ifdef DEBUG
		printf("sbrk returning error\n");
#endif
		return -1;
	}
	for (cp = (char *)old_brk; cp < (char *)_curbrk; cp++)
		*cp = -1;
	return old_brk;
}

__brk(endds)
register ADRT endds;
{
	register char* cp;

#ifdef DEBUG
	printf("brk: endds is 0x%x, _curbrk is 0x%x\n",endds,_curbrk);
#endif
	if ((endds >= (vcbKdbMem + KDB_SPACE)) || (endds < (int)(&end)))
	{
#ifdef DEBUG
		printf("brk returning error\n");
#endif
		return -1;
	}
	for (cp = (char *)_curbrk; cp < (char *)endds; cp++)
		*cp = -1;
	return (_curbrk = endds);
}

_filbuf()
{
	Panic("filbuf called");
}

_flsbuf()
{
	Panic("flsbuf called");
}
#endif /* CDBKDB */

/***********************************************************************
 * B A C K T R
 *
 * Perform a stack bactrace. Stolen from adb.
 */

#define	CALL2	0x4E90		/* jsr (a0)    */
#define	CALL6	0x4EB9		/* jsr address */
#undef BSR			/* defined by <a.out.h> -> <shl.h> */
#define	BSR	0x6100		/* bsr address */
#define	BSRL	0x61FF		/* bsr address w/32 bit displacement */

#define	ADDQL	0x500F		/* addq.l #xxx,sp */
#define	ADDL	0xDEFC		/* add.l  #xxx,sp */
#define LEA	0x4fef		/* lea xxx(sp),sp */

#ifndef CDBKDB
backtr(link,cnt)
register int link;			/* value in register a6 */
{
	register int rtn, p; 
	register short inst;
	register int calladr, entadr;
	int i, argn, num;
	register int bottom;

	/* Old way to compute bottom. 
	bottom = (link & ~4095) + 4096;
	*/
	/* Now we allow for multiple stack pages. */
	bottom = (int)AdrU;

	if (cnt <= 0) cnt = 8;
	kdb_interrupt = 0;
	while(!kdb_interrupt && cnt--) {
		p = link; 
		calladr = -1; 
		entadr = -1;

		link = *((int *) p);	/* address of next link (a6) */
		p += 4;
		rtn = *((int *) p);	/* return address */

		if (*((short *) (rtn-6)) == CALL6) {
			entadr = *((int *) (rtn-4));
			calladr = rtn - 6;
		} else if (*((short *) (rtn-6)) == BSRL) {
			entadr = *((int *) (rtn-4)) + (rtn - 4);
			calladr = rtn - 6;
		} else if ((*((short *) (rtn-4)) & ~0xFFL) == BSR) {
			calladr = rtn - 4;
			entadr = (short) *((short *) (rtn-2)) + rtn-2;
		} else if ((*((short *) (rtn-2)) & ~0xFFL) == BSR) {
			calladr = rtn - 2;
			entadr = (char) *((short *) (rtn-2)) + rtn;
		} else if (*((short *) (rtn-2)) == CALL2)
			calladr = rtn - 2;

		inst = *((short *) rtn);

		if ((inst & 0xF13F) == ADDQL) {
			argn = (int) (inst>>9) & 07;
			if (argn == 0) argn = 8;
		} else if ((inst & 0xFEFF) == ADDL) {
			if (inst & 0x100)
				argn = (int) *((int *) (rtn + 2));
			else
				argn = (int) *((short *) (rtn + 2));
		} else if (inst == LEA)  {
				argn = (int) *((short *) (rtn + 2));
		} else	argn = 0;

		if (argn && (argn % 4))
			argn = (argn/4) + 1;
		else 
			argn /= 4;

		if (calladr != -1)
			print_symbol(calladr, ":    ");
		else 
			printf("???:    ");
		if (entadr != -1)
			print_symbol(entadr, "  (");
		else
			printf("???  (");

		/* Print the arguments */
		for (i=0; i<argn; i++) {
			p += 4;
			if (i>0)			/* not the first arg? */
				printf(", ");		/* put a comma after the previous one */
			num = *(int *) p;
			if (num>=0 && num<=9)		/* a single digit? */
				printf("%d", num);	/* hex or decimal are the same */
			else
				printf("0x%x", num);	/* big numbers are hex */
		}

		printf(")\n");

		if (link > bottom || link < 1)
			return;
		if ( ((kdb_processor == 1) && (link & 0x10000000)) ||
		     ((kdb_processor == 2) && (link & 0x10000000)) ||
		     ((kdb_processor == 3) && (link & 0x10000000)) ||
		     ((kdb_processor == 0) && (link > 0x00f00000)) )
			return;
	}
}
#endif /* CDBKDB */

/***********************************************************************
 * M O V E   B Y T E S
 *
 * Do a possibly-overlapping move, to the right or left, one byte at a
 * time, of an object in the debugger's memory space.
 */

MoveBytes (adrDest, adrSrc, cb)
char *	adrDest;	/* where to move from	*/
char *	adrSrc;		/* where to move to	*/
register int	cb;	/* bytes to move	*/
{
    register char *dest = adrDest;
    register char *src  = adrSrc;

    if (dest < src)			/* move left to right */
    {
	while (cb--)
	    *dest++ = *src++;
    }
    else {				/* move right to left */
	dest += cb;
	src  += cb;
	while (cb--)
	    *(--dest) = *(--src);
    }
} /* MoveBytes */

/***********************************************************************
 * N E X T  C M D
 *
 * Get the next command line from the debugger terminal
 */

NextCmd(sbCmd, cbCmd, sbPrompt)
register char *sbCmd;
char *sbPrompt;
{
	printf(sbPrompt);
	strcpy(sbCmd,kdbgetline(sbCmd));
	if (*sbCmd == 0)
	{	*sbCmd++ = '~';
		*sbCmd = 0;
	}
}

/***********************************************************************
 * S H O W  S T A T E
 *
 * show some state information
 */

ShowState()
{
	int iobase;

	printf("%s\n",pKrev);

	printf("Kernel file is %s\n",vsbSymfile);

	switch(kdb_processor) {
		case 1:
			printf("Processor is MC68020, ");
			if (pmmu_exist)
				printf("MMU is MC68851\n");
			else
				printf("MMU is HP\n");
			break;
		case 2:
			printf("Processor is MC68030 with on board MMU\n");
			break;
		case 3:
			printf("Processor is MC68040 with on board MMU\n");
			break;
		default:
			printf("Unknown processor type\n");
	}

	printf("SYSDEBUG physical loadpoint: 0x%x\n", kdb_loadpoint);
	printf("Kernel   physical loadpoint: 0x%x\n", loadpoint);


	printf("Physical address of highest malloc'ed memory is 0x%x\n",_sbrk(0));
#ifndef CDBKDB
	printf("%s",kdb_tos);
#endif /* CDBKDB */

	if (mapper_off)
		printf("WARNING: mapping not turned on\n");
	else {
		printf("MMU mapping is enabled\n");
		iobase = current_io_base + PHYS_IO_BASE;
		if (iobase == PHYS_IO_BASE)
			printf("Internal I/O is mapped logical = physical at 0x%x\n", iobase);
		else
			printf("Internal I/O is mapped logically at 0x%x\n", iobase);
	}

}

/***********************************************************************
 * Y E S   N O
 *
 * Print a prompt, read a yes/no answer, and return true for yes.
 */

int YesNo (sbPrompt)
    char	*sbPrompt;		/* string to prompt with */
{
    char	sbAnswer[80];	/* response read in */

    NextCmd   (sbAnswer, 80, sbPrompt);
    return    ((sbAnswer[0] == 'y') || (sbAnswer[0] == 'Y'));

} /* YesNo */

int Min (x, y)
{
    return ((x < y) ? x : y);
} 

int Max (x, y)
{
    return ((x > y) ? x : y);
} 

DebPr(val)
{
	printf("here 0x%x\n",val);
}

#ifndef CDBKDB
/* WriteProtect 
   Modify kernel page table entries such that kernel text pages cannot be
   written.  This routine should only be called if Virtual Address mapping
   for the kernel is enabled and no further text modifications are expected
   to occur in the debugger code prior to returning to the kernel.  

   A stripped kernel can not provide SYSDEBUG with Sysmap to point to the
   page tables.  In that case, we're basically headed for a crash.

   If the debugger variable $noprotect is set to 1, text pages will NOT be
   protected by this routine.  This is specifically intended for internal use
   programs which might have a need to modify kernel text. (ICA for example.)
*/

WriteProtect()
{ 
	register int *PtrSysmap;
	register int k;

	if (noprotect == 1)	/* If set, text pages are NOT write disabled */
		return(0);

	if (mapper_off)
		return;

	if ((AdrSysmap <=0) && (Adrkpte <= 0))
		return;

	if (AdrSysmap <= 0)
		PtrSysmap = *(int **) Adrkpte;
	else
		PtrSysmap = (int *) AdrSysmap;

	for (k= TextPgCnt-1; k >= 0; k--)
		PtrSysmap[k] |= 0x00000004;
	
}

/* WriteEnable
   Modify kernel page table entries such that writes to kernel text pages are
   legal.  Since the debugger might have any number of reasons for modifying
   just about any kernel text address, open all of the PTE's.  This routine
   should only be called if Virtual Address mapping is currently enabled and
   we are on the way into the debugger from the kernel.

   A stripped kernel can not provide SYSDEBUG with Sysmap to point to the page
   tables.  In that case, we're basically headed for a crash.
*/
WriteEnable()
{
	register int *PtrSysmap;
	register int k;

	if (mapper_off)
		return;


	if ((AdrSysmap <=0) && (Adrkpte <= 0))
		return;

	if (AdrSysmap <= 0)
		PtrSysmap = *(int **)Adrkpte;
	else
		PtrSysmap = (int *) AdrSysmap;

	for (k= TextPgCnt-1; k >= 0; k--)
		PtrSysmap[k] &= 0xfffffffb;

}
#endif /* CDBKDB */


seekread(fd, offset, buf, size, tag)
int fd;
long offset;
char *buf;
int size;
char *tag;
{
	static char tmpbuf[256];
	int rc, i;

	rc = lseek(fd, offset, 0);
	if (rc == -1) {
		printf("couldn't lseek to %d for %s\n", tag);
		return -1;
	}

	if (size>256) {
		rc = read(fd, buf, size);
		if (rc != size) {
			printf("tried to read %d bytes, got %d for %s\n", size, rc,tag);
			return -1;
		}
		return rc;
	}
	else {
		rc = read(fd, tmpbuf, sizeof(tmpbuf));
		lseek(fd, offset+size, 0);
		if (rc < size) {
			printf("tried to read %d bytes, got %d, needed %d for %s\n", sizeof(tmpbuf), rc,size,tag);
			return -1;
		}
		for (i=0; i<size; i++)
			*buf++ = tmpbuf[i];
		return size;
	}
}
