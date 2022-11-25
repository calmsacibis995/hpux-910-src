/* @(#) $Revision: 64.5 $ */      
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define close _close
#define getpid _getpid
#define getenv _getenv
#define profil _profil
#define sprintf _sprintf
#define strcpy _strcpy
#define strrchr _strrchr
#define write _write
#define creat _creat
#define monitor _monitor
#endif

#define PROFDIR  "PROFDIR"
#define MON_OUT  "mon.out"
#define NULL     0
extern char **__argv_value ;  /* initialized to argv by mcrt0 */
static char mon_out[100];
extern char *getenv(), *strcpy(), *strrchr();

#ifdef _NAMESPACE_CLEAN
#undef monitor
#pragma _HP_SECONDARY_DEF _monitor monitor
#define monitor _monitor
#endif
monitor(lowpc, highpc, buffer, bufsiz, nfunc)
register unsigned long lowpc, highpc;
int buffer[];		/* includes phdr and nfunc*cnt structures */
register int bufsiz;	/* size of buffer in shorts */
{
	register int oh;
	register unsigned scale;
	register char *s, *end, *name = mon_out, buf[100];
	static int *sbuf, ssiz;
	int pid;
	struct phdr {	/* sizeof phdr used in mcrt0.s */
		int *lpc;
		int *hpc;
		int ncnt;
	};
	struct cnt {	/* sizeof cnt used in mcrt0.s */
		int *pc;
		long ncall;
	};

	
	if(lowpc == 0) {
		profil((char *) 0, 0, 0, 0);	/* 0 in 1st arg is a flag */
		if ((pid = getpid()) <=0)
			pid = 1;
		sprintf(buf, mon_out, pid);
		oh = creat(buf, 0666);
		write(oh, (char *) sbuf, ssiz);
		close(oh);
		return;
	}
	sbuf = buffer;
	ssiz = bufsiz;
	buffer[0] = lowpc;
	buffer[1] = highpc;
	buffer[2] = nfunc;
	oh = sizeof(struct phdr) + nfunc * sizeof(struct cnt);
	buffer = (int *) (((int) buffer) + oh);
	bufsiz -= oh;
	if(bufsiz <= 0)
		return;
               
	if ((s = getenv(PROFDIR)) == NULL)
		end = MON_OUT;    /* use default "mon.out" */
	else if (*s == '\0')      /* PROFDIR is null, so no profiling */
		return;
	else {			  /* create PROFDIR/pid.progname */
		while (*s != '\0')
			*name++ = *s++;
		*name++ = '/';
		*name++ = '%';
		*name++ = 'd';
		*name++ = '.';
		if (__argv_value != NULL)
			end = ((s = strrchr(__argv_value[0], '/')) != NULL) ?
			       s+1 :    /* use only file name part */
			       __argv_value[0]; /* use entire name */
	}
	strcpy(name, end);
	oh = highpc-lowpc;
	if (oh > bufsiz)
		scale = 0x10000L;
	else
		scale = 0xffffL;
	while (oh > bufsiz)
	{	scale >>= 1;
		oh >>= 1;
	}
	profil((char *) buffer, bufsiz, (int) lowpc, scale);
}
