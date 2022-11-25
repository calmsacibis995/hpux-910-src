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

extern boot_errno;
extern char *HPUX_ID;

main()
{
	register loadpoint, io, i, processor;
	register msus = *(int *) MSUS;
	register status = 0;
	register char *addr;
	struct exec x;
	char msg[80];
	char filename[255];

	strcpy(msg, "Secondary Loader ");
	strcat(msg, HPUX_ID+6);
	crtmsg(0, msg);

	io = *((int *) F_AREA);
	*((int *) (io + MB_PTR)) = *((int *) LOWRAM);
	i = call_bootrom(MINIT, msus);
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
/* 
 * Recognize the three standard HP-UX kernel names.  If the user creates
 * his own kernel, we could never recognize it.  This is only to protect
 * the user from trying to boot HP-UX on a 68000.
 */
	/* SYSNAME is limited by reboot to 10 bytes */
	strncpy(filename, ((char *) SYSNAME), 10);
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
	strcpy(msg, filename);		/* need to prepend "/" */
	filename[0] = '/';
	strcpy(&filename[1], msg);

#define FLAG2_68000	0x04
#define FLAG2_68010	0x00
#define FLAG2_68020	0x10
#define FLAG2_68030	0x14
#define FLAG2_CPU	0x14	/* SYSFLAG2 bit4 and bit2 */
#define M68010	0
#define M68020	1
#define M68000	2
#define M68030	3
/* 
 * Recognize the three standard HP-UX kernel names.  If the user creates
 * his own kernel, we could never recognize it.  This is only to protect
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

	strcpy(msg, "Booting ");
	strcat(msg, filename);
	crtmsg(1, msg);

	if ((io = open_CDF(filename)) < 0) {
		strcpy(msg, filename);
		strcat(msg, ": cannot open, or not executable");
#ifdef DEBUG
		dump_value("boot errno",boot_errno);
#endif
		_stop(msg);
	}
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
	(*((int (*)())(i))) (*((int *)LOWRAM), loadpoint,
		x.a_text, x.a_data, x.a_bss, processor);   /* jump to OS */

shread:
	_stop("read error (check your data path)");

}

#define	push_regs()	{ asm("	movm.l	%d0-%d7/%a0-%a5,-(%sp)	"); }
#define	pop_regs()	{ asm("	movm.l	(%sp)+,%d0-%d7/%a0-%a5	"); }

#ifdef DEBUG

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

puts(msg)
register char *msg;
{
	register int i;
	register char *d;
	char buffer[84];

	push_regs();
	crtmsg(crt_line,msg);
	if (++crt_line >= 23)		/* bump line number */
		crt_line = 4;
	for (d = buffer; d < buffer+80; *(d++) = ' ')
		;
	buffer[80] = '\0';
	crtmsg(crt_line,buffer);	/* clear next line */
	pop_regs();
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

dump_value(s,i)
char *s;
{
	char msg[88];

	push_regs();
	htoa(msg,i);
	msg[8] = ' ';
	msg[9] = '\0';
	strcat(msg,s);
	puts(msg);
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
