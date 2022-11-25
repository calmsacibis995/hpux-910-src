/* @(#) $Revision: 70.2 $ */

/*
 *	crt0.c
 *
 *	merged sources for S300 startup code for C, Pascal, Fortran
 *	includes startup code for prof and gprof code
 *
 *	Several #define's control the compilation of this source.
 *	On the global features level, _NAMESPACE_CLEAN toggles the leading
 *	underscores in front of symbols such as "environ".  It is currently
 *	OFF, but it is expected that the makefile will turn it on.
 *	DLD_SUPPORT controls whether dynamic loader support is to be included,
 *	and is turned ON by default.  BBA may be turned on in addition to
 *	force the program to dump dynamic loader branch coverage data
 *	(assuming an appropriate version of /lib/dld.sl is used)
 *	DEBUG means different things at different times, and is normally OFF.
 *	STACK_CLEAN causes crt0.o to clean up all stack bytes it uses.
 *	STACK_ZERO caues crt0.o to skip "___stack_zero" bytes on the stack
 *	before invoking the dynamic loader.
 *
 *	The following #define's are used to control production of the various
 *	flavors of startup code:
 *
 *	FORTRAN: turn ON for Fortran, off for C & Pascal (default OFF)
 *	MCRT0: turn ON for prof versions (mcrt0.o, mfrt0.o) (default OFF)
 *	GCRT0: turn ON for gprof versions (gcrt0.o, gfrt0.o) (default OFF)
 */

#define DLD_SUPPORT
#define STACK_ZERO
#define STACK_EVEN
#define SHLIB_PATH

#define NULL 0

unsigned int _SYSTEM_ID = 0x20C;	/* CPU_HP_MC68020 from unistd.h */

#pragma OPT_LEVEL 1

#ifdef _NAMESPACE_CLEAN
#define	_environ __environ
#define _write __write
#define _monstartup __monstartup
#define _monitor __monitor
#define _atexit  __atexit
#endif /* _NAMESPACE_CLEAN */

#ifdef FORTRAN
#define _main main
#endif

asm("
	#C startup, transliteration of crt0.s from unix
	text
	global	____exit,_environ,start,_main
	global	___argc_value,___argv_value
	global	float_soft,float_loc,flag_68881
	global	flag_68010,flag_fpa,fpa_loc,flag_68040
	global	__DYNAMIC,___dld_loc
");

#ifdef STACK_ZERO
asm("
	global	___stack_zero
");
#endif

#if defined(MCRT0) || defined(GCRT0)
asm("
	global	_monitor,__cleanup,__etext
	global	_monstartup,__mcleanup
");
#endif

#ifdef SHLIB_PATH
asm("
	global	___dld_shlib_path
");
#endif

asm("
	text
start:
	long	0		#Let dereferences of NULL pointers return 0
	sub.l	%a6,%a6		#mark end of stack for cdb
	subq	&1,%d0		#flip state of d0
	mov.w	%d0,float_soft	#1=>software, 0=>hardware float card
	mov.w	%d1,flag_68881	#1=>68881 present; 0=>68881 not present
	beq.b	start5S		#if 68881 is present...
	fmov.l	&0x7400,%fpcr
start5S:
	mov.l	%a0,%d0		#misc flags
	add.l	%d0,%d0		#set up flag_68010
	subx.w	%d1,%d1
	mov.w	%d1,flag_68010
	add.l	%d0,%d0		#set up flag_fpa
	subx.w	%d1,%d1
	mov.w	%d1,flag_fpa
	add.l	%d0,%d0		# set up flag_68040
	subx.w	%d1,%d1
	mov.w	%d1,flag_68040
#ifdef	USE_M_FP
	# a_miscinfo stuff
	tst.l	%d2
	ble.b	a_miscinfo	#skip if information not present
				#also, is FP save/restore necessary?
	lsl.w	flag_68881	# if not, change value of flags
	lsl.w	flag_fpa
a_miscinfo:
#endif
	mov.l	(4,%sp),%d0	#ptr to arg0 (set condition flag for below test)
	beq.w	start1S		#don't do clear if there are no args
	mov.l	%d0,%a0		#move to address register
	clr.l	(-4,%a0)	#make sure arg0 has a 0 word preceding
start1S:
	mov.l	%sp,%a0		#save ptr to nargs
#ifdef STACK_EVEN
	mov.l	%sp,%d5
	subq.l	&8,%d5
	and.l	&0xfffffff8,%d5
	mov.l	%d5,%sp
#endif /* STACK_EVEN */
	subq.l	&8,%sp		#allocate space for two words
	mov.l	(%a0),(%sp)	#copy down nargs
	mov.l	(%a0),___argc_value	#save argc for Pascal
	addq.l	&4,%a0		#bump ptr past nargs to arg0 ptr
	mov.l	%a0,(4,%sp)	#save this as second arg to main
	mov.l	%a0,___argv_value	#save argv for Pascal
start2S:
	tst.l	(%a0)+		#look for 0 word marking end of args
	bne.w	start2S		#not there yet
	mov.l	(4,%sp),%a1	#get ptr to ptr_to_arg0 ie argv
	cmp.l	%a0,(%a1)	#have we gone past *argv
	blt.w	start3S		#no, a0 must point to valid environ (blo?)
	subq.l	&4,%a0		#else make it point to zero word
start3S:
	mov.l	%a0,(8,%sp)	#this becomes third argument to main
	mov.l	%a0,_environ	#save it in global
");

#ifdef DLD_SUPPORT
asm("
	mov.l	&__DYNAMIC,%d0
	tst.l	%d0		# if (!&_DYNAMIC)
	beq.w	callmain	#	goto callmain;
");
#ifdef STACK_ZERO
asm("
	sub.l	&___stack_zero,%sp
");
#endif
asm("
	pea.l	0
	pea.l	dld
	mov.l	&open,%d0
	jsr	system		# fd = open(/lib/dld.sl,O_RDONLY);
	lea.l	8(%sp),%sp
	mov.l	%d0,%d3
	pea.l	64
	pea.l	filhdr
	mov.l	%d0,-(%sp)
	mov.l	&read,%d0
	jsr	system		# count = read(fd,filhdr,sizeof(struct exec));
	lea.l	12(%sp),%sp
	cmp.l	%d0,&64		# if (count != sizeof(struct exec))
	bne.w	fatal		#	goto fatal;
	lea.l	filhdr,%a2
	cmpi.l	(%a2),&0x020c010e	# if (N_BADMAG(filhdr))
	bne.w	fatal			#	goto fatal
	addi.l	&0x0fff,12(%a2)
	andi.l	&0xfffff000,12(%a2)
	addi.l	&0x0fff,16(%a2)
	andi.l	&0xfffff000,16(%a2)
	addi.l	&0x0fff,20(%a2)
	andi.l	&0xfffff000,20(%a2)
	pea.l	0x1000
	mov.l	%d3,-(%sp)
	pea.l	text_map
	pea.l	text_prot
	mov.l	12(%a2),-(%sp)
	pea.l	0
	mov.l	&mmap,%d0
	jsr	system		# t = mmap(t,text,READ,SHARED,fd,toff);
	lea	24(%sp),%sp
	mov.l	%d0,%d6
	mov.l	12(%a2),-(%sp)
	addi.l	&0x1000,(%sp)
	mov.l	%d3,-(%sp)
	pea.l	data_map
	pea.l	data_prot
	mov.l	16(%a2),-(%sp)
	add.l	12(%a2),%d0
	mov.l	%d0,-(%sp)
	mov.l	&mmap,%d0
	jsr	system		# d = mmap(d,data,ALL,PRIVATE,fd,doff);
	lea	24(%sp),%sp
	mov.l	%d0,___dld_loc	# __dld_loc = d;
	mov.l	%d0,%a3
	mov.l	%d3,-(%sp)
	mov.l	&close,%d0
	jsr	system		# close(fd);
	addq.l	&4,%sp
	mov.l	20(%a2),%d0
	beq.b	got_bss
	pea.l	0
	add.l	&-4,%sp
	pea.l	bss_map
	pea.l	bss_prot
	mov.l	%d0,-(%sp)
	mov.l	%a3,%d0
	add.l	16(%a2),%d0
	mov.l	%d0,-(%sp)
	mov.l	&mmap,%d0
	jsr	system		# b = mmap(b,bss,ALL,PRIVATE,zfd,0);
	lea	24(%sp),%sp
got_bss:
");
#ifdef SHLIB_PATH
asm("
	mov.l	_environ,%d0
	mov.l	%d0,___dld_shlib_path
");
#endif
asm("
	pea.l	0
	pea.l	0
	mov.l	%d6,-(%sp)
	mov.l	%d6,-(%sp)
	jsr	(%a3)		# rc = shl_init(t,t,0,0);
	lea.l	16(%sp),%sp
	tst.l	%d0		# if (rc != 0)
	bne.w	fatal		#	goto fatal;
");
#ifdef STACK_CLEAN
asm("
	clr.l	-8(%sp)
	clr.l	-12(%sp)
	clr.l	-16(%sp)
	clr.l	-20(%sp)
	clr.l	-24(%sp)
	clr.l	-28(%sp)
");
#endif
#ifdef STACK_ZERO
asm("
	add.l	&___stack_zero,%sp
");
#endif
asm("
callmain:
");
#endif /* DLD_SUPPORT */

#if defined(MCRT0) || defined(GCRT0)
asm("
	#profiling setup
	mov.l	&__etext,-(%sp)	#high pc
	mov.l	&eprol,-(%sp)	#immediately after profile is low pc
	jsr	_monstartup	#start profiling
	add.l	&8,%sp		#pop both args to monitor
");
#endif /* MCRT0 || GCRT0 */

/*
 * The following call to __start is to allow other libraries to
 * hook in their initialization routines.  The libc version of
 * __start does nothing.
 */
#ifdef STACK_ZERO
asm("
	sub.l	&___stack_zero,%sp
");
#endif
asm("
	jsr	__start		#call dummy _start routine
");
#ifdef STACK_ZERO
asm("
	add.l	&___stack_zero,%sp
");
#endif

asm("
	jsr	_main		#call user's main program
	addq.l	&8,%sp		#pop argc,argv
	mov.l	%d0,-(%sp)	#value of main is argument to exit
");

#ifdef _NAMESPACE_CLEAN
asm("
	jsr	____exit
");
#else /* _NAMESPACE_CLEAN */
asm("
	jsr	_exit
");
#endif /* _NAMESPACE_CLEAN */

asm("
	addq.l	&4,%sp
");

#if defined(MCRT0) || defined(GCRT0)
asm("
	mov.l	&msgend-msg,-(%sp)	#nbytes
	mov.l	&msg,-(%sp)		#ptr to message
	mov.l	&2,-(%sp)		#stderr
	jsr	_write			#tell the user about it
	lea.l	12(%sp),%sp
");
#endif /* MCRT0 || GCRT0 */


#ifdef DLD_SUPPORT

asm("
	pea.l	8
	movq	&1,%d0		#exit trap
	jsr	system
fatal:
	pea.l	msg_end-msg_start
	pea.l	msg_start
	pea.l	2
	mov.l	&write,%d0
	jsr	system		# write(2,msg,strlen(msg));
	lea.l	12(%sp),%sp
	pea.l	8
	movq	&1,%d0		# _exit(code);
	jsr	system
system:
	trap	&0
	bcs.b	fatal		# if (errno) goto fatal;
	rts
");

#ifdef BBA
asm("
	global	__exit
__exit:
	lea.l	___dld_loc,%a0
	tst.l	(%a0)
	beq.b	no_dld
	mov.l	(%a0),%a0
	lea	36(%a0),%a0
	jsr	(%a0)
no_dld:
	movq	&1,%d0
	trap	&0
	short	0x4e72
	short	0x2000
");
#endif /* BBA */

asm("
	set	read,3
	set	write,4
	set	open,5
	set	close,6
	set	mmap,71
	set	prot_none,0
	set	prot_read,1
	set	prot_write,2
	set	prot_exec,4
	set	map_shared,1
	set	map_private,2
	set	map_fixed,4
	set	map_replace,8
	set	map_anonymous,16
	set	zero_prot,prot_none
	set	zero_map,map_private+map_anonymous
");

#ifdef DEBUG
asm("
	set	text_prot,prot_read+prot_write+prot_exec
	set	map_text_base,map_private
");
#else /* DEBUG */
asm("
	set	text_prot,prot_read+prot_exec
	set	map_text_base,map_shared
");
#endif /*DEBUG */

asm("
	set	text_map,map_text_base
	set	data_prot,prot_read+prot_write+prot_exec
	set	data_map,map_private+map_fixed
	set	bss_prot,prot_read+prot_write+prot_exec
	set	bss_map,map_anonymous+map_private+map_fixed
");

#else /* DLD_SUPPORT */

asm("
	pea.l	8
	clr.l	-(%sp)		# fake a return address - any one will do
	movq	&1,%d0
	trap	&0
");

#endif /* DLD_SUPPORT */

asm("
	bss
	lalign	4
___argc_value:	space	4*(1)
	lalign	4
___argv_value:	space	4*(1)
	lalign	2
float_soft:	space	2*(1)		#is float card not present?
	lalign	2
flag_68881:	space	2*(1)		#is 68881 present?
	lalign	2
flag_68010:	space	2*(1)		#is 68010 present?
	lalign	2
flag_fpa:	space	2*(1)		#is Dragon present?
	lalign	2
flag_68040:	space	2*(1)		#is 68040 present?
	lalign	4
	set	float_loc,0xffffb000	#location of float card
	set	fpa_loc,0xfff08000	#location of Dragon card
");

#ifdef DLD_SUPPORT
asm("
	data
dld:
	lalign	1
");
#ifdef DEBUG
static char dld_name[] = "/tmp/dld.sl";
#else
static char dld_name[] = "/lib/dld.sl";
#endif
asm("msg_start: ");
#ifdef DEBUG
static char dld_msg[] = "/tmp/dld.sl: cannot execute\n";
#else
static char dld_msg[] = "/lib/dld.sl: cannot execute\n";
#endif
asm("
msg_end:
	lalign	1

	bss
filhdr:
	space	64
___dld_loc:
	space	4
");

#endif /* DLD_SUPPORT */

#if defined(MCRT0) || defined(GCRT0)
asm("
	data
msg:
	lalign	1
");
static char mon_msg[] =	"No space for monitor buffer(s)\n";
asm("
msgend:
	lalign	1
");
#endif /* MCRT0 || GCRT0 */

#ifdef _NAMESPACE_CLEAN
#undef _environ
#undef _write
#undef _monstartup
#undef _monitor
#undef _atexit
#endif /* _NAMESPACE_CLEAN */

#if defined(MCRT0) || defined(GCRT0)
_exit_monitor()
{
#ifdef _NAMESPACE_CLEAN
  _monitor(0);
#else
  monitor(0);
#endif
}
#endif /* MCRT0 || GCRT0 */


#if defined(MCRT0) || defined(GCRT0)

#pragma OPT_LEVEL 2

#ifdef _NAMESPACE_CLEAN
#define close _close
#define creat _creat
#define mcount _mcount
#define moncontrol _moncontrol
#define monitor _monitor
#define monstartup _monstartup
#define perror _perror
#define profil _profil
#define write _write
#define atexit _atexit
#endif /* _NAMESPACE_CLEAN */

#ifdef DEBUG
#include <stdio.h>
#endif /* DEBUG */

/* The following section is define's and struct's from gprof's gmon.h */
/*--------------------------- gmon.h ------------------------------*/
struct phdr {
    char	*lpc;
    char	*hpc;
    int		ncnt;
};

#define HISTCOUNTER	unsigned short
#define HISTFRACTION	2
#define HASHFRACTION	1
#define ARCDENSITY	5
#define MINARCS		50

struct tostruct {
    char		*selfpc;
    long		count;
    unsigned short	link;
};

struct rawarc {
#ifdef GCRT0
    unsigned long	raw_frompc;
#endif /* GCRT0 */
    unsigned long	raw_selfpc;
    long		raw_count;
};

#define ROUNDDOWN(x,y)	(((x)/(y))*(y))
#define ROUNDUP(x,y)	((((x)+(y)-1)/(y))*(y))
/*--------------------------- gmon.h ------------------------------*/

    /*
     *	froms is actually a bunch of unsigned shorts indexing tos
     */
static int		profiling = 3;
static int		prof_written = 0;
static unsigned short	*froms;
static struct tostruct	*tos = 0;
static long		tolimit = 0;
static char		*s_lowpc = 0;
static char		*s_highpc = 0;
static unsigned long	s_textsize = 0;

static int	ssiz;
static char	*sbuf;
static int	s_scale;
    /* see profil(2) where this is describe (incorrectly) */
#define		SCALE_1_TO_1	0x10000L

#ifdef _NAMESPACE_CLEAN
#undef monstartup
#pragma _HP_SECONDARY_DEF _monstartup monstartup
#define monstartup _monstartup
#endif /* _NAMESPACE_CLEAN */

monstartup(lowpc, highpc)
    char	*lowpc;
    char	*highpc;
{
    int			monsize;
    char		*buffer;
    char		*_sbrk_no_profile();
/*    extern char		*minbrk;
*/

	/*
	 *	round lowpc and highpc to multiples of the density we're using
	 *	so the rest of the scaling (here and in gprof) stays in ints.
	 */


    lowpc = (char *)
	    ROUNDDOWN((unsigned)lowpc, HISTFRACTION*sizeof(HISTCOUNTER));
    s_lowpc = lowpc;
    highpc = (char *)
	    ROUNDUP((unsigned)highpc, HISTFRACTION*sizeof(HISTCOUNTER));
    s_highpc = highpc;
    s_textsize = highpc - lowpc;
    monsize = (s_textsize / HISTFRACTION) + sizeof(struct phdr);
    buffer = _sbrk_no_profile( monsize );
    if ( buffer == (char *) -1 ) {
	write( 2 , mon_msg , sizeof mon_msg - 1 );
	return;
    }
    froms = (unsigned short *) _sbrk_no_profile( s_textsize / HASHFRACTION );
    if ( froms == (unsigned short *) -1 ) {
	write( 2 , mon_msg , sizeof mon_msg - 1 );
	froms = 0;
	return;
    }
    tolimit = s_textsize * ARCDENSITY / 100;
    if ( tolimit < MINARCS ) {
	tolimit = MINARCS;
    } else if ( tolimit > 65534 ) {
	tolimit = 65534;
    }
    tos = (struct tostruct *) _sbrk_no_profile( tolimit * sizeof( struct tostruct ) );
    if ( tos == (struct tostruct *) -1 ) {
	write( 2 , mon_msg , sizeof mon_msg - 1 );
	froms = 0;
	tos = 0;
	return;
    }
/*    minbrk = _sbrk_no_profile(0);
*/
    tos[0].link = 0;
    monitor( lowpc , highpc , buffer , monsize , tolimit );
    atexit(_exit_monitor);
}

extern char **__argv_value;

_mcleanup()
{
    int			fd;
    int			fromindex;
    int			endfrom;
    char		*frompc;
    int			toindex;
    struct rawarc	rawarc;
#ifdef MCRT0
    int			newcount;
    char		*s,*name,*end,monbuf[100],tmpbuf[100];
    char		*_getenv(), *_strcpy(), *_strrchr();
    int			pid, _getpid();

    if ((s = _getenv("PROFDIR")) == NULL)
	_strcpy(monbuf, "mon.out");
    else if (*s == '\0')      /* PROFDIR is null, so no profiling written out */
	return;
    else {			  /* create PROFDIR/pid.progname */
	name = tmpbuf;
	while (*s != '\0')
		*name++ = *s++;
	*name++ = '/';
	*name++ = '%';
	*name++ = 'd';
	*name++ = '.';
	if (__argv_value != NULL)
		end = ((s = _strrchr(__argv_value[0], '/')) != NULL) ?
		       s+1 :    /* use only file name part */
		       __argv_value[0]; /* use entire name */
	_strcpy(name,end);
	if ((pid = _getpid()) <= 0)
		pid = 1;
	_sprintf(monbuf,tmpbuf,pid);
	}

    fd = creat( monbuf , 0666 );
    if ( fd < 0 ) {
	perror( "mcount: mon.out");
	return;
    }
#endif /* MCRT0 */

#ifdef GCRT0
    fd = creat( "gmon.out" , 0666 );
    if ( fd < 0 ) {
	perror("mcount: gmon.out");
	return;
    }
#endif /* GCRT0 */

#   ifdef DEBUG
	fprintf( stderr , "[mcleanup] sbuf 0x%x ssiz %d\n" , sbuf , ssiz );
#   endif /* DEBUG */

#ifdef MCRT0
    newcount = 0;
    endfrom = s_textsize / (HASHFRACTION * sizeof(*froms));
    for ( fromindex = 0 ; fromindex < endfrom ; fromindex++ ) {
	if ( froms[fromindex] == 0 ) {
	    continue;
	}
	frompc = s_lowpc + (fromindex * HASHFRACTION * sizeof(*froms));
	for (toindex=froms[fromindex]; toindex!=0; toindex=tos[toindex].link) {
		newcount++;
	}
    }
    ( (struct phdr *) sbuf ) -> ncnt = newcount;
    write( fd , sbuf , 12);
#endif

#ifdef GCRT0
    write( fd , sbuf , ssiz );
#endif
    endfrom = s_textsize / (HASHFRACTION * sizeof(*froms));
    for ( fromindex = 0 ; fromindex < endfrom ; fromindex++ ) {
	if ( froms[fromindex] == 0 ) {
	    continue;
	}
	frompc = s_lowpc + (fromindex * HASHFRACTION * sizeof(*froms));
	for (toindex=froms[fromindex]; toindex!=0; toindex=tos[toindex].link) {
#	    ifdef DEBUG
		fprintf( stderr ,
			"[mcleanup] frompc 0x%x selfpc 0x%x count %d\n" ,
			frompc , tos[toindex].selfpc , tos[toindex].count );
#	    endif /* DEBUG */
#ifdef GCRT0
	    rawarc.raw_frompc = (unsigned long) frompc;
#endif /* GCRT0 */
	    rawarc.raw_selfpc = (unsigned long) tos[toindex].selfpc;
	    rawarc.raw_count = tos[toindex].count;
	    write( fd , &rawarc , sizeof rawarc );
	}
    }
#ifdef MCRT0
    write ( fd , sbuf+12, ssiz-12 );
#endif /* MCRT0 */
    close( fd );
}

#pragma OPT_LEVEL 1

#ifdef _NAMESPACE_CLEAN
#undef mcount
#pragma _HP_SECONDARY_DEF _mcount mcount
#define mcount _mcount
#endif /* _NAMESPACE_CLEAN */
mcount()
{
	static char TOLIMIT[] = "mcount: tos overflow\n";
	register char			*selfpc;	/* %a5 */
	register unsigned short		*frompcindex;	/* %a4 */
	register struct tostruct	*top;		/* r9  => r3 */
	register struct tostruct	*prevtop;	/* r8  => r2 */
	register long			toindex;	/* r7  => r1 */

#ifdef lint
	selfpc = (char *)0;
	frompcindex = 0;
#else /* lint */
	/*
	 *	find the return address for mcount,
	 *	and the return address for mcount's caller.
	 */
	asm("	mov.l	4(%a6),%a5");		/* selfpc = return addr */
#ifdef MCRT0
	asm("	mov.l	%a5,%a4");		/* frompcindex = selfpc */
#else
	asm("	mov.l	8(%a6),%a4");		/* frompcindex = next return */
#endif /* MCRT0 */
#endif /* lint */
	/*
	 *	check that we are profiling
	 *	and that we aren't recursively invoked.
	 */
	if (profiling) {
		goto out;
	}
	profiling++;
	/*
	 *	check that frompcindex is a reasonable pc value.
	 *	for example:	signal catchers get called from the stack,
	 *			not from text space.  too bad.
	 */
	frompcindex = (unsigned short *)((long)frompcindex - (long)s_lowpc);
	if ((unsigned long)frompcindex > s_textsize) {
		goto done;
	}
	frompcindex =
	    &froms[((long)frompcindex) / (HASHFRACTION * sizeof(*froms))];
	toindex = *frompcindex;
	if (toindex == 0) {
		/*
		 *	first time traversing this arc
		 */
		toindex = ++tos[0].link;
		if (toindex >= tolimit) {
			goto overflow;
		}
		*frompcindex = toindex;
		top = &tos[toindex];
		top->selfpc = selfpc;
		top->count = 1;
		top->link = 0;
		goto done;
	}
	top = &tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 *	arc at front of chain; usual case.
		 */
		top->count++;
		goto done;
	}
	/*
	 *	have to go looking down chain for it.
	 *	top points to what we are looking at,
	 *	prevtop points to previous top.
	 *	we know it is not at the head of the chain.
	 */
	for (; /* goto done */; ) {
		if (top->link == 0) {
			/*
			 *	top is end of the chain and none of the chain
			 *	had top->selfpc == selfpc.
			 *	so we allocate a new tostruct
			 *	and link it to the head of the chain.
			 */
			toindex = ++tos[0].link;
			if (toindex >= tolimit) {
				goto overflow;
			}
			top = &tos[toindex];
			top->selfpc = selfpc;
			top->count = 1;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
		/*
		 *	otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 *	there it is.
			 *	increment its count
			 *	move it to the head of the chain.
			 */
			top->count++;
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}

	}
done:
	profiling--;
	/* and fall through */
out:
	return;

overflow:
	profiling++; /* halt further profiling */
	write(2, TOLIMIT, sizeof TOLIMIT - 1);
	goto out;
}

asm("	sglobal	mcount");
asm("	set mcount,__mcount");

#pragma OPT_LEVEL 2

/*VARARGS1*/
#ifdef _NAMESPACE_CLEAN
#undef monitor
#pragma _HP_SECONDARY_DEF _monitor monitor
#define monitor _monitor
#endif /* _NAMESPACE_CLEAN */

monitor( lowpc , highpc , buf , bufsiz , nfunc )
    char	*lowpc;
    char	*highpc;
    char	*buf;	/* declared ``short buffer[]'' in monitor(3) */
    int		bufsiz;
    int		nfunc;	/* not used, available for compatability only */
{
    register o;

    if ((lowpc == 0) && (!prof_written)) {
	moncontrol(0);
	_mcleanup();
	prof_written++;
	return;
    }
    prof_written = 0;

    sbuf = buf;
    ssiz = bufsiz;
    ( (struct phdr *) buf ) -> lpc = lowpc;
    ( (struct phdr *) buf ) -> hpc = highpc;
    ( (struct phdr *) buf ) -> ncnt = ssiz;
    bufsiz -= sizeof(struct phdr);
    if ( bufsiz <= 0 )
	return;
    o = highpc - lowpc;
    if( bufsiz < o )
	s_scale = ( (float) bufsiz / o ) * SCALE_1_TO_1;
    else
	s_scale = SCALE_1_TO_1;
    moncontrol(1);
}

/*
 * Control profiling
 *	profiling is what mcount checks to see if
 *	all the data structures are ready.
 */
#ifdef _NAMESPACE_CLEAN
#undef moncontrol
#pragma _HP_SECONDARY_DEF _moncontrol moncontrol
#define moncontrol _moncontrol
#endif /* _NAMESPACE_CLEAN */

moncontrol(mode)
    int mode;
{
    if (mode) {
	/* start */
	profil(sbuf + sizeof(struct phdr), ssiz - sizeof(struct phdr),
		s_lowpc, s_scale);
	profiling = 0;
    } else {
	/* stop */
	profil((char *)0, 0, 0, 0);
	profiling = 3;
    }
}

#pragma OPT_LEVEL 1

#endif /* MCRT0 || GCRT0 */

#if defined(MCRT0) || defined(GCRT0)
asm("
	text
	lalign	4
eprol:				#mark end of startup
				#This is for alignment, and should assure that
				#eprol is ALWAYS on a 32 bit boundary.
#
	space	0
");
#endif /* MCRT0 || GCRT0 */
