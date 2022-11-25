/* HPUX_ID: @(#)boot.c	38.3     86/11/07  */
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <a.out.h>
#include "saio.h"
#define	SYSNAME	 0xfffffdc2	/* location where bootname is stored */
#define	MSUS	 0xfffffedc	/* location of device specifier */
#define HIGHRAM  0xfffffac0     /* highest ram location the o.s can use */
#define	LOWRAM	 0xfffffdce	/* lowest numbered byte of ram */
#define	F_AREA	 0xfffffed4	/* pointer to mb_ptr and mb_size */
#define	SYSFLAG2 0xfffffeda	/* processor type information */

#define	MB_PTR	0x10
#define	MB_SIZE	0x14

#define PAGESIZE 4096

#define IS_HPUX		1
#define RAMDISK         2
#define IS_TAPE		4
#define IS_CD_INSTALL	8

extern boot_errno;
extern char *HPUX_ID;
#ifdef SDS
unsigned int current_msus;
#endif /* SDS */

main()
{
	register loadpoint, io, i, processor;
#ifndef SDS
	register msus = *(int *) MSUS;
#endif /* ! SDS */
	register status = 0, how = 0;
	register char *addr;
	struct exec x;
	struct fs *fsptr;
	unsigned long ramfssize;
	unsigned long ramfsbegin;
	char msg[80];
	char filename[255];

#ifdef XTERMINAL
	int	isxterm = 0 ;
#endif /* XTERMINAL */
            
#ifdef SDS
	current_msus = *(int *) MSUS;
#endif /* SDS */

	connect_sti();			/* load display driver routines */
#ifdef SDS
	strcpy(msg, "SDS Secondary Loader ");
#else /* ! SDS */
	strcpy(msg, "Secondary Loader ");
#endif /* else ! SDS */
	strcat(msg, HPUX_ID+6);			/* Skip "@(#) $" */
	msg[strlen(msg)-1] = '\0';		/* Strip trailing $ */
	crtmsg(0, msg);

	io = *((int *) F_AREA);
	*((int *) (io + MB_PTR)) = *((int *) LOWRAM);
#ifdef SDS
	i = call_bootrom(MINIT, current_msus);
#else /* ! SDS */
	i = call_bootrom(MINIT, msus);
#endif /* else ! SDS */
	if (i != 0) {
#ifdef DEBUG
		dump_value("boot errno",boot_errno);
#endif
		_stop(8, "Cannot initialize boot device");
	}
/*
 * The following song and dance is to ensure that the bootstrap does not
 * overwrite bootrom driver data areas in lowram.  See the bootrom manual
 * for details.
 */
	io = *((int *)F_AREA);
	io = *((int *) (io + MB_SIZE));
	loadpoint = ((*((int *) LOWRAM) + io + PAGESIZE-1) & ~(PAGESIZE-1));
	loadpoint = (loadpoint + PAGESIZE-1) & ~(PAGESIZE-1);
#ifdef SDS
	io = sds_open();
#endif /* SDS */

	how = get_how();
/* 
 * Recognize the three standard HP-UX kernel names.  If the user creates
 * his own kernel, we could never recognize it.  This is only to protect
 * the user from trying to boot HP-UX on a 68000.
 */
	/* SYSNAME is limited by reboot to 10 bytes */
	strncpy(filename, ((char *) SYSNAME), 10);
	filename[10] = '\0';
	for (addr = filename+10; --addr >= filename; )
		if (*addr == ' ')	/* SRM strings are blank-terminated */
			*addr = '\0';

	if (strcmp(filename, "SYSLDR2") == 0) {		/* for SRM */
		status |= IS_HPUX;
		strcpy(filename, "SYSHPUX");
	}
	if (strcmp(filename, "SYSHPUX") == 0) {
	/* names have been changed to protect the innocent */
		strcpy(filename, "hp-ux");
		status |= IS_HPUX;
	}
	if (strcmp(filename, "SYSBCKUP") == 0)
		status |= IS_HPUX;
	if (strcmp(filename, "SYSDEBUG") == 0)
		status |= IS_HPUX;
	if (strcmp(filename, "SYSXDB") == 0)
		status |= IS_HPUX;
	if (strcmp(filename,"SYSRECOVER") == 0) {
		strcpy(filename, "hp-ux");
		status |= (IS_HPUX | RAMDISK);
	}
#ifdef XTERMINAL
        if (strcmp(filename,"SYSXTERM") == 0) {
		isxterm = 1;
		strcpy(filename, "usr/lib/xtkern.300");
		status |= IS_HPUX ;
                if (how == F_REMOTE)
			status |= RAMDISK ;
        }
#endif /* XTERMINAL */
	if (strcmp(filename,"SYSINSTALL") == 0) {
		if (how == F_REMOTE)
                {
		    strcpy(filename, "usr/lib/uxinstkern.300");
                    status |= (IS_HPUX | RAMDISK );
                }
		else
                {
		    strcpy(filename, "hp-ux");
                    status |= (IS_HPUX | RAMDISK | IS_TAPE);
                }
	}
        /*
         * Treat a CD-INSTALL as a RAMDISK, with at TAPE like layout.
         */
	if (strcmp(filename,"SYSCDINST") == 0)
	    status |= (IS_HPUX | RAMDISK | IS_CD_INSTALL | IS_TAPE);

	strcpy(msg, filename);		/* need to prepend "/" */
	filename[0] = '/';
	strcpy(&filename[1], msg);

#define FLAG2_68000	0x04
#define FLAG2_68010	0x00
#define FLAG2_68020	0x10
#define FLAG2_68030	0x14
#define FLAG2_CPU	0x14	/* SYSFLAG2 bit4 and bit2 */
#undef M68000
#undef M68010
#undef M68020
#undef M68030
#undef M68040
#define M68010	0
#define M68020	1
#define M68000	2
#define M68030	3
#define M68040	4
/* 
 * Recognize the three standard HP-UX kernel names.  If the user creates
 * his own kernel, we could never recognize it.	 This is only to protect
 * the user from trying to boot HP-UX on a 68000.
 */
	switch (*((u_char *) SYSFLAG2) & FLAG2_CPU) {
		case FLAG2_68000:	processor = M68000; break;
		case FLAG2_68010:	processor = M68010; break;
		case FLAG2_68020:	processor = M68020; break;
		case FLAG2_68030:	processor = M68030; break;
	}
#ifdef DEBUG
	dump_value("SYSFLAG2",*((u_char *)SYSFLAG2) & FLAG2_CPU);
	dump_value("processor",processor);
#endif
	if ((status & IS_HPUX) && (processor == M68000))
		_stop("hp-ux requires a 68010/68020/68030");

	ramfssize  = 0;
	ramfsbegin = 0;

	if ((status & RAMDISK) != 0) {
	    int flag = 0;
	    char *ram_fs;

	    if(processor == M68000 || processor == M68010)
		_stop("RAM file system boot not supported on 68000/68010");

	    /* 
	     * If device is a TAPE and RAMDISK driver requested - set flag 
	     * Also, treat a SYSCDINST as a raw (tape) device, the cdfs
	     * cannot be accessed.
	     */
	    if (how == F_ISATAPE || status & IS_TAPE) {
		status |= IS_TAPE; 
		flag = F_ISATAPE;
		ram_fs = "/RAMFS";
	    }
	    else /* remote */
	    {
#ifdef XTERMINAL
		if (isxterm)
		    ram_fs = "/usr/lib/xtfs.300";
		else
#endif /* XTERMINAL */
		    ram_fs = "/usr/lib/uxinstfs.300";
	    }
	    if ((io = open_CDF(ram_fs, flag)) < 0) {
		strcpy(msg, ram_fs);
		strcat(msg, ": cannot open, or not executable");
#ifdef DEBUG
		dump_value("boot errno",boot_errno);
#endif
		_stop(msg);
	    }
	    strcpy(msg, "Loading RAM Filesystem");
	    crtmsg(1, msg);

	    /*
	     * In the case of a CD-ROM install, the RAMFS and kernel
	     * is at a location after the CDFS filesystem.  This location
	     * is given by an offset pointer on the disk which is kept
	     * in the location 8k.
	     */
	    if (status & IS_CD_INSTALL ) 
	    {
		long ram_fs_offset;
		if ((lseek (io, 8192, 0) < 0)	       ||
		    (read(io, &ram_fs_offset, 4) != 4) ||
		    (lseek (io, ram_fs_offset, 0) < 0))
		{
		    _stop("Could not get RAMFS offset.");
		}
	    }

	    /* Read in the first 16K so that we can compute the size of */
	    /* the filesystem from the super block.                     */

	    if (read(io, (char *) loadpoint, 16384) != 16384)
		    goto shread;

	    /* Set fsptr to point at the super block */

	    fsptr = (struct fs *)(loadpoint + 8192);

	    /* Compute File system size */

	    ramfssize = fsptr->fs_size * fsptr->fs_fsize;

	    /* check size */

	    if (ramfssize < 32768)
		_stop("Invalid RAM file system size");

	    /* Check for enough memory */

	    if (((u_int) loadpoint <  *(u_int*) LOWRAM)
	     || ((u_int) ramfssize > ((u_int) HIGHRAM - (u_int) loadpoint)))
		_stop("Not enough memory");

	    /* Read in rest of File System */

	    i = ramfssize - 16384;
	    if (read(io, (char *) (loadpoint + 16384), i) != i)
		    goto shread;

	    /* Save address of file system */

	    ramfsbegin = loadpoint;

	    /* Readjust load point */

	    loadpoint = (loadpoint + ramfssize + (2 * PAGESIZE) - 1) & ~(PAGESIZE-1);
	    /* Close boot rom file descriptor for RAM Boot */
	    if (!(status & IS_TAPE))
	        close(io);
	}
	
	/* Tape files are read sequentially,  and we cannot close and
	 * reopen device.  All other files must be opened by filename.
	 */
	if (!(status & IS_TAPE) && (io = open_CDF(filename, 0)) < 0) {
		strcpy(msg, filename);
		strcat(msg, ": cannot open, or not executable");
#ifdef DEBUG
		dump_value("boot errno",boot_errno);
#endif
		_stop(msg);
	}

	strcpy(msg, "Booting ");
	strcat(msg, filename);
	crtmsg(2, msg);

retry_hdr:
	if (read(io, (char *)&x, sizeof(x)) != sizeof(x)) /* read a.out hdr */
		goto shread;
#ifdef DEBUG
	puts("a.out header");
	dump_mem(&x, sizeof(x));
#endif
	if ((x.a_magic.file_type != SHARE_MAGIC &&
	     x.a_magic.file_type != EXEC_MAGIC)) {
		_stop("Bad format");
	     }
/*
 *  Make sure operating system to be loaded will fit into the memory that
 * the o.s.  can use.  Don't check the debugger, since that goes into
 * noncontiguous memory, i.e.  below LOWRAM.  If PASCAL or BASIC, just
 * check on size of kernel.  If HP-UX, add fudge factor determined by
 * experimenting on 5.2, providing enough memory so that panic handler can
 * be installed.  Then user should never see "UNEXPECTED USE OF ..."  from
 * boot ROM because of lack of memory.
 */
	i = x.a_text;  i += x.a_data;  i += x.a_bss;
	if (status & IS_HPUX)
		i += 140000;
	if (((u_int) loadpoint <  *(u_int*) LOWRAM)
	 || ((u_int) i > ((u_int) HIGHRAM - (u_int) loadpoint)))
	    _stop("Not enough memory");

#ifdef DEBUG
	dump_value("text segment size",x.a_text);
#endif
	if (read(io, (char *) loadpoint, x.a_text) != x.a_text)
		goto shread;

	addr = (char *) (loadpoint + x.a_text);

	if (x.a_magic.file_type == SHARE_MAGIC)
		while ((int)addr & (PAGESIZE-1))
			*addr++ = 0;

#ifdef DEBUG
	dump_value("data segment size",x.a_data);
#endif
	if (read(io, addr, x.a_data) != x.a_data)
		goto shread;

	addr += x.a_data;
	for (i = x.a_bss; --i >= 0; )
		*addr++ = 0;

	lseek(io, addr, 0);

	if (x.a_magic.file_type == EXEC_MAGIC) {
		x.a_data += x.a_text;
		x.a_text = 0;
	}

	if (x.a_magic.file_type == SHARE_MAGIC) {
		x.a_text += PAGESIZE-1;
		x.a_text &= ~(PAGESIZE-1);
	}
	close(io);
	i = loadpoint;
	i += x.a_entry;
#ifdef DEBUG
	puts("jump to OS");
#endif
#ifdef SDS
	jump_to_os(i,*((int *)LOWRAM),loadpoint,x.a_text,
		   x.a_data, x.a_bss,0xdeadbeef,2,ramfsbegin,ramfssize);
#else /* ! SDS */
	jump_to_os(i,*((int *)LOWRAM),loadpoint,x.a_text,
		   x.a_data, x.a_bss,processor,2,ramfsbegin,ramfssize);
#endif /* else ! SDS */
shread:
	_stop("read error (check your data path)");

}

#define	push_regs()	{ asm("	movm.l	%d0-%d7/%a0-%a5,-(%sp)	"); }
#define	pop_regs()	{ asm("	movm.l	(%sp)+,%d0-%d7/%a0-%a5	"); }

#ifdef DEBUG

itoa(i, a)
unsigned int i;
char *a;
{
	int j;
	unsigned int mask = 0xf0000000;

	*a++ = '0';
	*a++ = 'x';

	for (j = 7; j >= 0; j--) {
		unsigned int temp;

		temp = i & mask;
		temp = temp >> (j * 4);

		switch (temp) {
		case 0xf: *a++ = 'f'; break;
		case 0xe: *a++ = 'e'; break;
		case 0xd: *a++ = 'd'; break;
		case 0xc: *a++ = 'c'; break;
		case 0xb: *a++ = 'b'; break;
		case 0xa: *a++ = 'a'; break;
		case 0x9: *a++ = '9'; break;
		case 0x8: *a++ = '8'; break;
		case 0x7: *a++ = '7'; break;
		case 0x6: *a++ = '6'; break;
		case 0x5: *a++ = '5'; break;
		case 0x4: *a++ = '4'; break;
		case 0x3: *a++ = '3'; break;
		case 0x2: *a++ = '2'; break;
		case 0x1: *a++ = '1'; break;
		case 0x0: *a++ = '0'; break;
		}
		
		mask = mask >> 4;
	}
	*a = '\0';
}

/*
 * this code is useful for stand-alone debugging.  The 'puts' call writes a
 * line of code to the system console, using the 'crtmsg' bootrom call.  It
 * is approximately equal to the HP-UX 'puts' call, except that a newline
 * is automatically appended, and scrolling is done with wraparound.
 *
 * Boot ROM routines tend to randomly trash registers.  It is advisable
 * to save d0-d7/a0-a5 before calling them via the {push,pop}_regs macro.
 *
 * C routines tend to trash d0-d1/a0-a1.  *.s files may get upset.  Use
 * the push/pop_regs macro's.
 */

int crt_line = 4;
char puts_buffer[84];

dump_value(msg, value)
char *msg;
unsigned int value;
{
	char buf[80];
	char msgbuf[160];

	itoa(value, buf);
	strcpy(msgbuf, msg);
	strcat(msgbuf, buf);
	puts(msgbuf);
}

puts(msg)
register char *msg;
{
	register int i;
	register char *d;

	crtmsg(crt_line,msg);
	if (++crt_line >= 48)		/* bump line number */
		crt_line = 4;
	for (d = puts_buffer; d < puts_buffer+80; *(d++) = ' ')
		;
	puts_buffer[80] = '\0';
	crtmsg(crt_line,puts_buffer);	/* clear next line */
}

/*
 * delay() is useful for slowing down the display so you can get a good
 * look at what has been printed.
 */

delay(i)
register i;
{
	push_regs();
	while (--i)
		;
	pop_regs();
}

int regs[16];			/* d0..d7,a0...a7 */
dump_d_regs()
{
	char msg[88];

	push_regs();
asm("	movm.l	&0xffff,_regs	");
	msg[0] = 'd';
	fmt8(regs,msg+1);
	puts(msg);
}

dump_reg()
{
	char msg[88];

	push_regs();
asm("	movm.l	&0xffff,_regs	");
	msg[0] = 'd';
	fmt8(regs,msg+1);
	puts(msg);
	msg[0] = 'a';
	fmt8(&regs[8],msg+1);
	puts(msg);
	pop_regs();
}

dump_mem(p,i)
register int *p;
register i;
{
	char msg[88];

	push_regs();
	while (i>0) {
		htoa(msg,(int) p);
		fmt8(p,msg+8);
		p += 8;
		i -= 8*4;			/* bytes */
		puts(msg);
	}
	pop_regs();
}
#define LINES 3
dump_stack()
{
	int foo[2];		/* dummy - gives us a relative stack value */

	push_regs();
	foo[0] = 0x0bad0bad;	/* marker */
	foo[1] = 0x0bad0bad;	/* marker */
	dump_mem(foo,LINES*8*4);
	pop_regs();
}

fmt8(p,s)
register int *p;
register char *s;
{
	register i;

	push_regs();
	for (i = 8; --i >= 0; ) {
		*(s++) = ' ';
		htoa(s,*(p++));
		s += 8;
	}
	*(s++) = '\0';
	pop_regs();
}

htoa(s,i)
register char *s;
register i;
{
	push_regs();
	asm("	mov.l	%a5,%a1	");
	asm("	mov.l	%d7,%d0	");
	asm("	jsr	0x1AC	");	/* call boot rom PNT8HEX */
	asm("	mov.l	%a1,%a5 ");
	*s = '\0';			/* terminate string */
	pop_regs();
}
#endif
