
#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 70.18 $";
#endif

/*
 * savecore
 */
#include <stdio.h>
#include <nlist.h>
#include <errno.h>		/* EAGAIN and EACCESS */
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/types.h>

#ifndef _WSIO
#include <sys/libIO.h>
#endif /* _WSIO */

#ifndef _WSIO
#define SELECTIVE_RETRIEVE
#define MULTIPLE_DUMP
#define SAVECORE_MIRROR
#endif /* _WSIO */

/* IMPORTANT -- IMPORTANT -- IMPORTANT -- IMPORTANT -- IMPORTANT */
/* WARNING -- WARNING -- WARNING -- WARNING -- WARNING -- WARNING */ 
/* savecore.h (BEGIN) */
/*
** The following defines and declarations are supposed to be part of
** the savecore.h file.  However, due to the late creation of the
** header file in the 9.0 schedule, we (commands and kernel) decided
** to include this header information individually in each command 
** (savecore(1M) and scancore(1M)).  
** In 9.2 release, this problem should be corrected by creating this 
** header file in the kernel branch and be delivered by kernel.  
** Savecore(1M) owner should still be the owner of savecore.h.
** If there is any declaration needed to be changed in 9.0, the same
** changes has to also go to the owner of scancore(1M) in kernel team.
*/

#ifndef SAVECORE_H

#define SAVECORE_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

typedef struct savecore_index
{
	int magic;		/* savecore magic */
	int level;		/* save level - all, user or kernel pages */
	int size;		/* size in pages */
	int reserve[12];	/* reserve for future use */
	unsigned int map[1];
};

#define SAVECORE_MAGIC	(0xFAECEACF)	/* magic number */

#define ROUNDUP(n, shift)	((((n)+~(~0<<shift))>>shift)<<shift)
#define INDEX_SZ(pg)		(sizeof(struct savecore_index) + (((pg)>>4)<<2))
#define INDEX_MEM_SZ(pg)	ROUNDUP(INDEX_SZ((pg)), PGSHIFT)

#define ALL_PAGE	((unsigned int) 0x00000000) /* all pages */
#define USER_PAGE	((unsigned int) 0x00000001) /* user + kernel pages */
#define KERNEL_PAGE	((unsigned int) 0x00000002) /* kernel pages only */
#define NOT_USED_PAGE	((unsigned int) 0x00000011) /* not used */

#define SETMAP(index, n, val)	(index->map[(n)/16] |= (val << ((n)%16*2)))
#define GETMAP(index, n)	((index->map[(n)/16]>>((n)%16*2)) & (~(~0<<2)))

#endif /* SAVECORE_H */

/* savecore.h (END) */

typedef struct dumptab
{
        char *  blk_dev;                /* block device name */
        char *  chr_dev;                /* character device name */
        int     dumplo;                 /* dumplo of device */
        int     size;                   /* size of the dump image chunk */
        dev_t   dp_dev;                 /* higher level dev_t */
#ifdef MULTIPLE_DUMP
        dev_t   lv;
        int     locked;
#endif /* MULTIPLE_DUMP */
#ifdef SAVECORE_MIRROR
	struct  mirror * mirror;
#endif /* SAVECORE_MIRROR */
}; 


#define ERR_STR		static char * const
#define SAVECORE_LCK	"/etc/savecore.LCK"
#define SHUTDOWN_LOG	"/usr/adm/shutdownlog"
#define DEV_KMEM	"/dev/kmem"

#ifdef __hp9000s800
#define underscore(symbol)	symbol
#else	/* __hp9000s800 */
#define underscore(symbol)	"_" symbol
#endif	/* __hp9000s800 */

#ifdef	__hp9000s800
#define SYM_VERSION		"_release_version"
#else	/* __hp9000s800 */
#define SYM_VERSION		"_version"
#endif	/* __hp9000s800 */
#define SYM_DUMPLO		underscore("dumplo")
#define SYM_TIME		underscore("time")
#define SYM_DUMPSIZE		underscore("dumpsize")
#define SYM_PANICSTR		underscore("panicstr")
#define SYM_DUMPMAG		underscore("dumpmag")
#ifndef	_WSIO
#define SYM_DUMPDEVT		underscore("dumpdevt")
#define SYM_NUM_DUMPDEVS	underscore("num_dumpdevs")
#define SYM_SWDEVT		underscore("swdevt")
#define SYM_PDIR		underscore("pdir")
#define SYM_NPDIR		underscore("npdir")
#define SYM_HTBL		underscore("htbl")
#define SYM_NHTBL		underscore("nhtbl")
#define SYM_SAVECORE		underscore("savecore")
#else	/* ! _WSIO */
#define SYM_DUMPDEV		underscore("dumpdev")
#endif	 /* ! _WSIO */

extern int		g_vflg;
extern int		g_pflg;

extern int		g_dump_magic;
extern char * 		g_tdevice;
extern int 		g_mirconfig_fd;
extern struct dumptab	g_dumptab[];
extern int 		g_ndumptab;
extern char *		g_filename;
extern struct nlist 	* g_nldump, g_nlboot[];
extern char *		g_dsystem, * g_system, * g_dirname;
extern int		g_dumpsize;
extern int		g_devsize;
extern int		g_kernel_pages;
extern int		g_user_pages;

extern int		g_save_level;
extern struct dumptab	g_dumptab[];

extern void 	Error(char const * msg);
extern void	PERROR(char const * msg);
extern void	Warning(char const * msg);
extern void	CleanupAndExit(int);

extern char *	PathName(const char *, const char *, const int);
extern void	ReadFile(const char *, const long, void *, const size_t);
extern void	WriteFile(const char *, const long, const void *, const size_t);
extern int	LockReadThenIncrBound(const char *, const char *);
extern void	ExpandCore(void);

extern void	ReadSymbols(const char *, struct nlist *);
extern int	Value(const struct nlist [], const char *);
extern struct nlist * Clone_nlboot(void);

extern const char *	BAD_LOCK_FILE_MSG;
extern void	RunBackground();
extern void	CallSwapon(void);
extern time_t	GetMTime(const char *);
extern time_t	CreateLock(struct dumptab *, const int, const char *);

extern void	DumpTableInit(struct dumptab *);
extern int	DumpAccess(const int, long, void *, int);
extern void	DumpToFile(int, int *, int, const struct savecore_index *, unsigned int);

extern struct savecore_index * CreateIndex(void);

extern void	ClearDump(void);
extern int	CheckSpace(int *);
extern void	CheckVersion(void);
extern void	LogEntry(void);

extern void	MarkEOF(const int, const int);
extern void	Tape_write(int *, char *, size_t);
extern int	Tape_open(const char *, int);
extern void	GetCoreFromTape(void);

extern void	mir_cleanup(void);
extern int	mir_offline(const struct dumptab *);
extern void	mir_dump_dev(struct dumptab *);

/* 
** All the MACROS 
*/
#define MIN(a,b)		(((a)<(b))?(a):(b))
#define NLDUMP(str)		(Value(g_nldump, str))
#define NLBOOT(str)		(Value(g_nlboot, str))
#define READ_DUMP(a, b, c)      DumpAccess(O_RDONLY, a, b, c)
#define WRITE_DUMP(a, b, c)     DumpAccess(O_WRONLY, a, b, c)
#define READ_KMEM(a, b, c)	ReadFile(DEV_KMEM, (a), (b), (c))

#define LOCK_DEVID(dt)  (((dt)->lv == NODEV) ? (dt)->dp_dev : (dt)->lv)

#ifndef MAXDUMPDEV
#define MAXDUMPDEV	1
#endif	/* MAXDUMPDEV */

struct dumptab g_dumptab[MAXDUMPDEV];

/* Global variables */
int		g_pflg = 0;	/* non-zero implies partial dumps OK */
int		g_vflg = 0;	/* set if verbose mode */
char *		g_system = "/hp-ux";	/* boot system to nlist */
char *		g_dsystem = 0;		/* dump system to nlist */
char *		g_tdevice = 0;		/* tape device name */
char *		g_dirname;		/* directory to save dumps in */
static int	g_cflg = 0;	/* non-zero implies just clearing dump flag */
static int	g_nflg = 0;	/* zero implies that /hp-ux is to be copied */
static int	g_xflg = 0;	/* extract from tape flag */

static int	g_noclear = 0;	/* hidden option - for testing only */

/* 
** Used in selective retrieve 
*/
int	g_Sflg = 0;
static int	g_uflg = 0;
static int	g_kflg = 0;
static int	g_iflg = 0;
char *	g_filename = 0;
int	g_save_level = ALL_PAGE;

int g_ndumptab;

#ifdef	MULTIPLE_DUMP
#define	SWAPON_SKIP	'0'
#define	SWAPON_NOWAIT	'1'
#define	SWAPON_WAIT	'2'

#ifdef _WSIO
static	int	g_fflg = 1;		/* run foreground by default */
#else /* _WSIO */
static	int	g_fflg = 0;		/* run background by default */
#endif /* _WSIO */

static int   g_swaplevel = SWAPON_NOWAIT;
static dev_t g_pri_swap;
static int g_child_process = 0;

#endif	/* MULTIPLE_DUMP */

#define	TSIZE (16384)


void
PERROR(char const * msg)
{
	perror(msg);
	CleanupAndExit(1);
}

void
Error(char const * msg)
{
	fprintf(stderr, "%s\n", msg);
	CleanupAndExit(1);
}

void
Warning(char const * msg)
{
	fprintf(stderr, "%s\n", msg);
}


void 
CleanupAndExit(int sig)
{
#ifndef _WSIO
	mir_cleanup();	/* Fast reimage partner if we took it OFFLINE */
#endif /* !_WSIO */

#ifdef MULTIPLE_DUMP
	if (! access(SAVECORE_LCK, F_OK))
	{
		Warning("savecore: removing /etc/savecore.LCK file");
		if (unlink(SAVECORE_LCK) == -1)
			PERROR(SAVECORE_LCK);
	}

	CallSwapon();

#endif /* MULTIPLE_DUMP */

	exit(sig);
}


static void
Usage(void)
{
	ERR_STR err_str = 
#ifdef _WSIO
		"usage: savecore [-nvcpx] [-d dumpsystem] [-t tapedevice] "
		"dirname [system] \n";
#else /* _WSIO */
		"usage: savecore [-nvcpx] [-d dumpsystem] [-t tapedevice] ...\n"
		"       [-F corefile] [-w [012]] [ukiSf] dirname [system] \n";
#endif /* _WSIO */
	fprintf(stderr, err_str);
	exit(1);
}


static void 
GetOpts(int argc, char * const argv[])
{
	int c;
	extern char *optarg;
	extern int optind;
#	ifdef _WSIO
	while ((c = getopt(argc, argv, "cpnvxd:t:Z:")) != EOF)
#	else   /* _WSIO */
	while ((c = getopt(argc, argv, "cpnvxd:t:ukifw:SF:Z:")) != EOF)
#	endif /* _WSIO */
	{
		switch (c)
		{
			case 'c':	g_cflg++;
					break;
			case 'p':	g_pflg++;
					break;
			case 'n':	g_nflg++;
					break;
			case 'v':	g_vflg++;
					break;
			case 'x':	g_xflg++;
					break;
			case 'd':	g_dsystem = optarg;
					break;
			case 't':	g_tdevice = optarg;
					break;
			case 'Z':	if (strcmp(optarg, "noclear"))
					{
						fprintf(stderr, "%s: "
						"illegal option -- Z\n",argv[0]);
						Usage();
					}
					else
						g_noclear = 1;
					break;
#			ifndef _WSIO
			case 'F':	g_filename = optarg;
					break;
			case 'u':	g_uflg++;
					g_save_level = USER_PAGE;
					break;
			case 'k':	g_kflg++;
					g_save_level = KERNEL_PAGE;
					break;
			case 'i':	g_iflg++;
					break;
			case 'S':	g_Sflg++;
					break;
			case 'w':	g_swaplevel = *optarg;
					break;
			case 'f':	g_fflg++;
					break;
#			endif /* ! _WSIO */
			case '?':	Usage();
			default:	Usage();
		}
	}

#	ifdef	MULTIPLE_DUMP

	if (g_swaplevel != SWAPON_SKIP && g_swaplevel != SWAPON_NOWAIT &&
		g_swaplevel != SWAPON_WAIT)
	{
		ERR_STR err_str = "savecore: Only level 0, 1 or 2 is allowed "
			"with -w\n";
		fputs(err_str, stderr);
		Usage();
	}

#	endif	/* MULTIPLE_DUMP */

	if (g_tdevice && g_nflg)
	{
		ERR_STR err_str = "savecore: -n and -t are mutually exclusive\n";
		fputs(err_str, stderr);
		Usage();
	}

	if (g_xflg && (! g_tdevice))
	{
		ERR_STR err_str = "savecore: the -x option requires -t\n";
		fputs(err_str, stderr);
		Usage();
	}

	if (!g_xflg && g_tdevice && g_Sflg)
	{
		ERR_STR err_str = "savecore: -t and -S are mutually exclusive "
			"without -x\n";
		fputs(err_str, stderr);
		Usage();
	}

#ifdef	SELECTIVE_RETRIEVE
	if (g_uflg && g_kflg)
	{
		ERR_STR err_str="savecore: -u and -k are mutually exclusive\n";
		fputs(err_str, stderr);
		Usage();
	}
	if (g_filename && (! g_dsystem))
	{
		ERR_STR err_str = "savecore: -F option requires -d\n";
		fputs(err_str, stderr);
		Usage();
	}

	if (g_filename && (! g_Sflg))
	{
		ERR_STR err_str = "savecore: -F option requires -S\n";
		fputs(err_str, stderr);
		Usage();
	}

	if (g_filename && g_nflg)
	{
		ERR_STR err_str="savecore: -F and -n are mutually exclusive\n";
		fputs(err_str, stderr);
		Usage();
	}
#endif	/* SELECTIVE_RETRIEVE */

	if (optind < argc)
		g_dirname = argv[optind++];
	else
		Usage();

	if (optind < argc)
		g_system = argv[optind++];

	if (argc > optind)
		Usage();
}


int
LockReadThenIncrBound(const char * dirname, const char * bound_file)
{
	static char bound_path[MAXPATHLEN+1];
	int fd;

	sprintf(bound_path, "%s/%s", dirname, bound_file);
	while (1)
	{
		if ((fd=open(bound_path,(O_RDWR|O_CREAT), 0777)) < 0) 
			PERROR(bound_path);

		if (lockf(fd, F_TLOCK, 0) == 0)
		{
			int value;
			char lin[80];
			if (! read(fd, lin, 80))
				value = 0;
			else
				sscanf(lin, "%d", &value);

			if (lseek(fd, 0L, SEEK_SET) < 0)
				PERROR(bound_path);

			sprintf(lin, "%d", value+1); 

			if (write(fd, lin, strlen(lin)) == -1)
				PERROR(bound_path);

			close(fd);
			return value;
		}
		else
		{
			if ((errno == EAGAIN) || (errno == EACCES))
				close(fd);
			else
				PERROR(bound_path);
		}
	}
}

#ifdef	MULTIPLE_DUMP

dev_t
GetPriSwap()
{
	struct swdevt swdevt[MAXNSWDEV];
	int size = MAXNSWDEV * sizeof(struct swdevt);
	dev_t pri_swap;

	/*
	** Read the swap device information from the running system.
	*/
	READ_KMEM(NLBOOT(SYM_SWDEVT), swdevt, size);
	pri_swap = swdevt->sw_dev;

	/*
	** Kludge 
	*/
	if (! pri_swap)
		pri_swap = NODEV;

	/*
	** major number 64 is the LVM driver.
	*/
	if (pri_swap != NODEV && major(pri_swap) != 64 && 
			map_mi_to_lu(&pri_swap) != SUCCESS)
		CleanupAndExit(1);

	return pri_swap;
}

#endif	/* MULTIPLE_DUMP */

static void
SaveCore(struct savecore_index const * index)
{
	int bound;
	int i;
	char * core_path, * sys_path;
	int remain = g_devsize;
	struct swdevt * swdevt;		/* only use for multiple dump */
	time_t mtime;
	int in_fd, out_fd;

	if (index && ((index->size << PGSHIFT) != g_dumpsize))
		Error("savecore: wrong size");

	/* 
	** Path of the files 
	*/
	if (g_tdevice)
	{
		core_path = sys_path = g_tdevice;
		out_fd = Tape_open(core_path, O_WRONLY);
	}
	else
	{
		bound = LockReadThenIncrBound(g_dirname, "bounds");
		core_path = PathName(g_dirname, "hp-core", bound);
		sys_path = PathName(g_dirname, "hp-ux", bound);
		if ((! g_Sflg) && g_save_level)
			(void) strcat(core_path,".I");

		if ((out_fd = open(core_path, O_WRONLY|O_CREAT, 0644)) < 0)
			PERROR(core_path);
	}

	printf("Saving core image to %s\n", (g_tdevice)?g_tdevice:core_path);

#ifdef	MULTIPLE_DUMP

	/*
	** Use the index format to save the core file.  
	*/
	if (! g_Sflg && g_save_level)
	{
		if (! index)
			Error("savecore: missing index");

		index->level = g_save_level;

		if (write(out_fd, index, INDEX_MEM_SZ(index->size)) < 0)
			PERROR(core_path);
	}

	/*
	** Check /etc/savecore.LCK file.
	*/
	if (! access(SAVECORE_LCK, F_OK))
	{
		ERR_STR err_str = 
		"WARNING:\n"
		"  Invalid /etc/savecore.LCK file from previous run.\n"
		"  Dump file is possibly corrupted.  Save dump anyway.\n\n";  
		if (! CheckLock(g_dumptab, SAVECORE_LCK))
			Warning(err_str);
	}

	mtime = CreateLock(g_dumptab, g_ndumptab, SAVECORE_LCK);

	/*
	** Setup all the signals.
	*/
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, CleanupAndExit);
	(void) signal(SIGQUIT, CleanupAndExit);
	(void) signal(SIGTERM, CleanupAndExit);

	if (!g_fflg && g_pri_swap!=LOCK_DEVID(g_dumptab))
		RunBackground();

#endif	/* MULTIPLE_DUMP */

	for (i = 0; remain > 0; remain -= g_dumptab[i].size, i++)
	{
		int in_fd;

		if (i > g_ndumptab)
			Error("no more dump device");

		if ((in_fd = open(g_dumptab[i].chr_dev, O_RDONLY)) < 0)
			PERROR(g_dumptab[i].chr_dev);

		if (lseek(in_fd, g_dumptab[i].dumplo, SEEK_SET) < 0)
			PERROR(g_dumptab[i].chr_dev);

		/* dumpsize is always >= dumptab->size */

		if (remain < g_dumptab[i].size)
			Error("Inconsistent core size");

		DumpToFile(in_fd, &out_fd, g_dumptab[i].size, index, 
			g_devsize - remain);

		if (close(in_fd) < 0)
			PERROR("savecore");

#ifdef	MULTIPLE_DUMP 

		if (!g_fflg && g_swaplevel != SWAPON_WAIT)
		{
			int disk, offset;

			if (WhereInDump(NLDUMP(SYM_DUMPMAG), &disk, &offset))
				Error("savecore: bad magic location");

			ReleaseLock(i, &mtime);

			if (disk == i) ClearDump();

			if (! g_child_process)
				if (g_ndumptab > 1)
					RunBackground();
				else
					;
			else
				CallSwapon();
		}

#endif	/* MULTIPLE_DUMP */
	}	/* for */

	if (i != g_ndumptab)
		Error("savecore: Inconsistent number of dump devices");

#ifdef	MULTIPLE_DUMP
	if (g_fflg)
	{
		ClearDump();
		if (unlink(SAVECORE_LCK) < 0)
			PERROR(SAVECORE_LCK);
	}
	else
		if (g_swaplevel == SWAPON_WAIT)
		{
			if (mtime != GetMTime(SAVECORE_LCK))
				Warning(BAD_LOCK_FILE_MSG);

			if (unlink(SAVECORE_LCK) < 0)
				PERROR(SAVECORE_LCK);

			ClearDump();
			CallSwapon();
		}
		else
			if (unlink(SAVECORE_LCK) < 0)
				PERROR(SAVECORE_LCK);

#else	/* MULTIPLE_DUMP */
	ClearDump();
#endif /* MULTIPLE_DUMP */

	if (g_tdevice)
		MarkEOF(out_fd, 1);
	else
	{
		if (close(out_fd) < 0)
			PERROR(core_path);

#ifdef SELECTIVE_RETRIEVE
		/* 
		** Write the "save_level" to the core file to inform other
		** tools if any of the pages are invalid.
		*/
		if (g_Sflg)
			WriteFile(core_path, NLDUMP(SYM_SAVECORE), 
				&g_save_level, sizeof(g_save_level));
#endif /* SELECTIVE_RETRIEVE */

		if (! g_nflg)
			if ((out_fd=open(sys_path,O_WRONLY|O_CREAT,0644)) < 0)
				PERROR(sys_path);
	}

	if (! g_nflg)
	{
		struct stat stat_buf;

		stat(g_dsystem, &stat_buf);
		if ((in_fd = open(g_dsystem, O_RDONLY)) < 0)
			PERROR(g_dsystem);

		DumpToFile(in_fd, &out_fd, stat_buf.st_size, 0, 0);
	}

	if (g_tdevice)
		MarkEOF(out_fd, 2);
}


main(int argc, char * const argv[])
{
	int dumpmag;

	GetOpts(argc, argv);

	if (g_xflg) {
	    GetCoreFromTape();
	    exit(0);
	}

#	ifndef	_WSIO
	if (g_filename && g_Sflg)
	{
		ExpandCore();
		exit(0);
	}
#	endif	/* _WSIO */

	/*
	** File type conversion.
	*/
	if (g_filename)
	{
	/*
		struct savecore_index * index;

		ReadSymbols(g_dsystem, g_nlboot);
		g_nldump = g_nlboot;
		DumpTableInit_File(g_dumptab);
		index = CreateIndex();
		SaveCore(index);
		exit(0);
	*/
	}

	if (! g_dsystem)
	{
		ReadSymbols(g_system, g_nlboot);
		g_nldump = g_nlboot;
		g_dsystem = g_system;
	}
	else
	{
		g_nldump = Clone_nlboot();
		ReadSymbols(g_system, g_nlboot);
		ReadSymbols(g_dsystem, g_nldump);
	}

	DumpTableInit(g_dumptab);

#ifdef MULTIPLE_DUMP
	g_pri_swap = GetPriSwap();
#endif	/*  MULTIPLE_DUMP */

	if (! DumpExists())
	{
		CleanupAndExit(2);
	}

	{
		struct dumptab * end, * dumptab;
		g_devsize = 0;
		for (dumptab = g_dumptab, end = dumptab + g_ndumptab;
			dumptab < end; dumptab++)
			g_devsize += dumptab->size;
	}

	if (g_cflg)
	{
		ClearDump();
		Warning("Dump cleared, no other action taken.\n");
		exit(0);
	}
	else
	{
		struct savecore_index * index = 0;
		CheckVersion();
		LogEntry();

#		ifndef	_WSIO
		if (g_uflg || g_kflg || g_iflg)
		{
			if (g_dumpsize == g_devsize)
				index = CreateIndex();
			else
			{
				Warning("savecore: partial dump only - "
					"-u, -k or -i option ignored");
				g_uflg = g_kflg = g_iflg = 0;
				g_save_level = ALL_PAGE;
			}
		}
#		endif	/* ! _WSIO */

		if (GetCrashTime() && CheckSpace(&g_save_level))
		{
			SaveCore(index);
			CleanupAndExit(0);
		}
		else
			CleanupAndExit(1);
	}
}


char *
PathName(const char * dirname, const char * filename, const int bound)
{
	static char path[MAXPATHLEN];
	char * str;

	(void) sprintf(path, "%s/%s.%d", dirname, filename, bound);

	if (! (str = malloc(strlen(path) + 1)))
		PERROR("savecore");
		
	strcpy(str, path);;
	return str;
}


void
ReadFile(const char * name, const long addr, void * buf, const size_t size)
{
	int fd;

	if ((fd = open(name, O_RDONLY)) < 0)
		PERROR(name);
	if (lseek(fd, addr, SEEK_SET) != addr)
		PERROR(name);
	if (read(fd, buf, size) != size)
		PERROR(name);
	if (close(fd) < 0)
		PERROR(name);
}

void
WriteFile(const char * name, const long addr, const void * buf, const size_t size)
{
	int fd;

	if ((fd = open(name, O_WRONLY)) < 0)
		PERROR(name);
	if (lseek(fd, addr, SEEK_SET) != addr)
		PERROR(name);
	if (write(fd, buf, size) != size)
		PERROR(name);
	if (close(fd) < 0)
		PERROR(name);
}

/* LARRY */


/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* 
** Module - CORE
*/

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/vfs.h>

int g_dump_magic = 0;


int
DumpExists()
{
	int boot_magic;

	READ_KMEM(NLBOOT(SYM_DUMPMAG), &boot_magic, sizeof(boot_magic));

	(void)READ_DUMP(NLDUMP(SYM_DUMPMAG),&g_dump_magic,sizeof(g_dump_magic));

	if (g_vflg && (boot_magic != g_dump_magic))
	{
		(void) fprintf(stdout, "savecore: dump magic number mismatch: "
			"dump (0x%x) kernel (0x%x)\n",g_dump_magic, boot_magic);
	}

	return (boot_magic == g_dump_magic);
}


void
ClearDump()
{
	int magic;
	if (g_noclear)
		return;

	if (READ_DUMP(NLDUMP(SYM_DUMPMAG), &magic, sizeof(magic)) < 0)
		Error("savecore: cannot read magic");
	if (magic != g_dump_magic)
	{
		if (g_vflg)
			Warning("savecore: dump magic is clobbered by swap.");
		return;
	}
	magic = 0;
	if (WRITE_DUMP(NLDUMP(SYM_DUMPMAG), &magic, sizeof(magic)) < 0)
		Error("savecore: cannot clear magic");
}


void
CheckVersion(void)
{
#	define MAXVERSIONLEN	80
	char boot_version[MAXVERSIONLEN];
	char dump_version[MAXVERSIONLEN];
	READ_KMEM(NLBOOT(SYM_VERSION), boot_version, MAXVERSIONLEN);
	(void) READ_DUMP(NLDUMP(SYM_VERSION), dump_version, MAXVERSIONLEN);

	if ((! g_dsystem) && strcmp(boot_version, dump_version))
	{
		ERR_STR err_str = "savecore: Warning: version mismatch:\n"
			"%s:\t%sdump:\t%s";
		(void) fprintf(stderr, err_str, g_system, boot_version, 
			dump_version);
	}
}


#define MAXPANICMSGLEN	80

void
LogEntry(void)
{
	int	panicstr;
	char	panic_msg[MAXPANICMSGLEN+1];
	static	const char * DATE_FORMAT = "%H:%M  %a %b %d %Y";
	FILE *	fp;
	char	str[80];
	time_t	now;
	struct	tm * tm;

	(void) READ_DUMP(NLDUMP(SYM_PANICSTR), &panicstr, sizeof(panicstr));

	if (panicstr)
	{
		(void) READ_DUMP(panicstr, panic_msg, MAXPANICMSGLEN);
		/* To be on the safe size, terminate the panic message. */
		panic_msg[MAXPANICMSGLEN+1] = '\0';
	}

	(void) time(&now);
	tm = localtime(&now);
	/* "r+" open for update (not create) */
	if (! (fp = fopen(SHUTDOWN_LOG, "r+")))
		return;
	(void) fseek(fp, 0L, 2); 

	(void) strftime(str, 80, DATE_FORMAT, tm);

	if (panicstr)
		(void) fprintf(fp, "%s.  Reboot after panic: %s\n", str, 
			panic_msg);
	else
		(void) fprintf(fp, "%s.  Reboot (no panic)\n", str);
	if (fclose(fp))
		PERROR(SHUTDOWN_LOG);
}


int
GetCrashTime()
{
#define SECS_IN_A_DAY	(60L*60L*24L)
#define LEEWAY 		(90*SECS_IN_A_DAY)

	time_t now;
	time_t dumptime;

	(void) time(&now);	/* Get current time */

	(void) READ_DUMP(NLDUMP(SYM_TIME), &dumptime, sizeof(dumptime));
	if (! dumptime)
		return 0;
	(void) printf("System went down at %s", ctime(&dumptime));
	if (dumptime < now - LEEWAY || dumptime > now + LEEWAY)
	{
		(void) printf("Dump time is unreasonable\n");
		return 0;
	}
	return 1;
}

int
GetMinfree(void)
{
	FILE * fp;
	char path[MAXPATHLEN+1];
	int minfree;

	(void) sprintf(path, "%s/%s", g_dirname, "minfree");
	if (! (fp = fopen(path, "r")))
		minfree = 0;
	else
	{
		if (fscanf(fp, "%d", &minfree) != 1)
		{
			perror("savecore");
			minfree = 0;
		}
		if (fclose(fp))
			PERROR(path);
	}
	return minfree;
}


int 
CheckSpace(int * save_level)
{
	struct statfs dsb;
	int level;
	int size;
	long sys_blks;		/* size in 512 blocks */
	long spacefree;		/* size in 512 blocks */
	long all_blks;		/* size in 512 blocks */
	long kernel_blks;	/* size in 512 blocks */
	long user_blks;		/* size in 512 blocks */
	long spaceavail;	/* size in 512 blocks */
	long minfree;		/* size in 512 blocks */

	level = * save_level;

	if (g_tdevice)
		return 1;

	if (g_iflg && level != ALL_PAGE)
		Error("savecore: incorrect save level");

	if (statfs(g_dirname, &dsb) < 0)
		PERROR(g_dirname);

	spaceavail = dsb.f_bavail * (dsb.f_bsize / 512);
	spacefree = dsb.f_bfree * (dsb.f_bsize / 512);

	if (! g_nflg)	/* need to savecore dump system also */
	{
		struct stat stat_buf;
		stat(g_dsystem, &stat_buf);
		sys_blks = stat_buf.st_size / 512;
		sys_blks += (stat_buf.st_size % 512) ? 1 : 0;
	}

	all_blks = g_devsize/512 + sys_blks;
	kernel_blks = g_kernel_pages * (NBPG/512) + sys_blks;
	user_blks = g_user_pages * (NBPG/512) + kernel_blks + sys_blks;

	minfree = GetMinfree();

	while (level <= KERNEL_PAGE)
	{
		switch (level)
		{
			case ALL_PAGE:
				size = all_blks;
				break;
			case USER_PAGE:
				size = user_blks;
				break;
			case KERNEL_PAGE:
				size = kernel_blks;
				break;
		}

		size += minfree;

		if (level == KERNEL_PAGE)
			break;

		/* spacefree is always larger than spaceavail */
		if (spacefree >= size)
			break;

		if (g_iflg)
			level++;
		else
			break;
	}

	*save_level = level;

	switch (level)
	{
		case ALL_PAGE:
			if (spacefree >= size)
			{
				if (spaceavail >= (size-minfree))
					Warning("savecore: save all pages.");
				else
					Warning("savecore: save all pages "
						"but free space threshold "
						"crossed.");
				return 1;
			}
			if (g_pflg)
			{
				Warning("savecore: not enough space to save "
					"all pages, do partial");
				return 1;
			}
			Warning("savecore: not enough space to save all pages, "
				"dump omitted.");
			return 0;
		case USER_PAGE:
			if (spacefree >= size)
			{
				if (spaceavail >= (size-minfree))
					Warning("savecore: save user and "
						"kernel pages. ");
				else
					Warning("savecore: save user and "
						"kernel pages but free space "
						"threshold crossed.");
				return 1;
			}
			if (g_pflg)
			{
				Warning("savecore: not enough space to save "
					"user and kernel pages, do partial");
				return 1;
			}
			Warning("savecore: not enough space to save user and "
				"kernel pages, dump omitted.");
			return 0;
		case KERNEL_PAGE:
			if (spacefree >= size)
			{
				if (spaceavail >= (size-minfree))
					Warning("savecore: save only "
						"kernel pages. ");
				else
					Warning("savecore: save only "
						"kernel pages but free space "
						"threshold crossed.");
				return 1;
			}
			if (g_pflg)
			{
				Warning("savecore: not enough space to save "
					"kernel pages, do partial");
				return 1;
			}
			Warning("savecore: not enough space to save "
				"kernel pages, dump omitted.");
			return 0;
		default:
			Error("savecore: invalid page");
	}
}


/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* 
** Module - INDEX
*/

#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <machine/cpu.h>

int g_kernel_pages = 0;
int g_user_pages = 0;

#ifndef _WSIO

#include <machine/pde.h>

static void
SetPageMode(unsigned int const mode, unsigned int const phys_pg, 
	struct savecore_index * index)
{
	register unsigned int ar = mode & 0xff;

	if (! phys_pg)
		ar = ar;

	if(ar == PDE_AR_KR || ar == PDE_AR_KRW ||
		ar == PDE_AR_KRX || ar == PDE_AR_KRWX ||
		ar == PDE_AR_URKW || ar == PDE_AR_URWKW ||
		ar == PDE_AR_URXKW || ar == PDE_AR_URWXKW)
	{
		g_kernel_pages++;
/*
printf("KERNEL PAGE: %d\n", phys_pg);
*/
		SETMAP(index, phys_pg, KERNEL_PAGE);
	}
	else if (ar == PDE_AR_UR || ar == PDE_AR_URW ||
		ar == PDE_AR_URX || ar == PDE_AR_URWX ||
		ar == PDE_AR_UX)
	{
		g_user_pages++;
		/*
		** If two virtual pages are mapped to the same physical
		** page, mark the page with a higher precedence.
		*/
		if (GETMAP(index, phys_pg) < USER_PAGE)
		{
/*
printf("USER PAGE: %d\n", phys_pg);
*/
			SETMAP(index, phys_pg, USER_PAGE);
		}
	}
	else
	{
		g_kernel_pages++;
		SETMAP(index, phys_pg, KERNEL_PAGE);
	}
}


struct savecore_index *
CreateIndex(void)
{
	int npdir, nhtbl;
	int vpdir;
	struct hpde *pdir;
	struct hpde * htbl, *ht;
	struct savecore_index * index;

	if (READ_DUMP(NLDUMP(SYM_NPDIR), &npdir,sizeof(npdir)) < 0)
		return 0;

	if (READ_DUMP(NLDUMP(SYM_NHTBL), &nhtbl,sizeof(nhtbl)) < 0)
		return 0;

	/* Read in pdir */
	{
		int size = npdir * sizeof(struct hpde);

		if (! (pdir = (struct hpde *) malloc(size)))
			PERROR("malloc");

		if (READ_DUMP(NLDUMP(SYM_PDIR), &vpdir, sizeof(vpdir)) < 0)
			return 0;

		if (READ_DUMP(vpdir, pdir, size) < 0)
			return 0;
	}

	/* Read in hash table */
	{
		int size = nhtbl * sizeof(struct hpde);

		if (! (htbl = (struct hpde *) malloc(size)))
			PERROR("malloc");

		/* actually read in the hash table */
		{
			int htbl_addr;		/* temporary address */
			if (READ_DUMP(NLDUMP(SYM_HTBL), &htbl_addr,
				sizeof(htbl_addr)) < 0)
				return 0;

			if (READ_DUMP(htbl_addr, htbl, size) < 0)
				return 0;
		}
	}

	/* Allocate index memory */
	{
		int pages = g_dumpsize >> PGSHIFT;

		if (! (index = (struct savecore_index *) malloc(INDEX_MEM_SZ(pages))))
			PERROR("savecore");

		memset(index, 0, INDEX_MEM_SZ(pages));
		index->magic = SAVECORE_MAGIC;
		index->size = pages;
	}

#define ODD_HALF_USED	0x01
#define EVEN_HALF_USED	0x02
#define IO_PAGE(n)	(n >= btop(IOSPACE))			

	for (ht = htbl; ht < (htbl + nhtbl); ht++) 
	{
		static struct hpde * list;

		if (ht->pde_valid_o || ht->pde_os & ODD_HALF_USED)
			if (! (IO_PAGE(ht->pde_phys_o)))
				SetPageMode(ht->pde_ar_o, ht->pde_phys_o, 
					index);
		if (ht->pde_valid_e || ht->pde_os & EVEN_HALF_USED)
			if (! (IO_PAGE(ht->pde_phys_e)))
				SetPageMode(ht->pde_ar_e, ht->pde_phys_e, 
					index);

		for (list = ht->pde_next; list; list = list->pde_next)
		{
			if (list >= ((struct hpde *) vpdir) + npdir)
				PERROR("Cannot find pdir");

#			define INT(n) ((int) n)

			list = (struct hpde *) (INT(pdir) + INT(list) - vpdir);

			if (list->pde_valid_o || list->pde_os & ODD_HALF_USED)
				if (! (IO_PAGE(ht->pde_phys_o)))
					SetPageMode(list->pde_ar_o, 
						list->pde_phys_o, index);
			if (list->pde_valid_e || list->pde_os & EVEN_HALF_USED)
				if (! (IO_PAGE(ht->pde_phys_e)))
					SetPageMode(list->pde_ar_e, 
						list->pde_phys_e, index);
		}	/* for hpde list */
	} 	/* for htbl */
	return index;
}

#endif /*  _WSIO */


void
ExpandCore(void)
{
	struct savecore_index * index, * tmp_index;
	char buffer[NBPG];
	int nbpg = NBPG;
	char out_file[MAXPATHLEN];
	int in_fd, out_fd;

	Warning("savecore: expand compact core to sparse core file");

	/* 
	** filename has to have ".I" suffix 
	*/
	if (strcmp(".I", g_filename + (strlen(g_filename) - 2)))
		Error("savecore: Incorrect compact file suffix");

	if ((in_fd = open(g_filename, O_RDONLY)) < 0)
		PERROR(g_filename);
	
	if (read(in_fd, buffer, nbpg) < 0)
		PERROR(g_filename);

	tmp_index = (struct savecore_index *) buffer;

	/* 
	** index header has to have the magic number 
	*/
	if (tmp_index->magic != SAVECORE_MAGIC)
		Error("savecore: Incorrect magic number");

	/* 
	** recreate the index file from compacted core 
	*/
	{
		int index_size = INDEX_MEM_SZ(tmp_index->size);

		if (! (index = malloc(index_size)))
			PERROR("savecore");

		(void) memcpy(index, tmp_index, nbpg);

		if (index_size & (nbpg-1))
			Error("savecore: invalid index size");

		if (index_size - nbpg)
			if (read(in_fd,((char*)index)+nbpg,index_size-nbpg)<0)
				PERROR(g_filename);
	}

	/* get output filename */
	{
		char * p;
		sprintf(out_file,"%s/", g_dirname);
		if (! (p = strrchr(g_filename, '/')))
			p = g_filename;
		else
			p++;
		strncat(out_file, p, strlen(p) - strlen(".I"));
	}
		
	switch (index->level)
	{
		case USER_PAGE:
			fprintf(stderr, "savecore: expand user and kernel pages to "
				"%s\n", out_file);
			break;
		case KERNEL_PAGE:
			fprintf(stderr, "savecore: expand only kernel pages to "
				"%s\n", out_file);
			break;
		default:
			Error("savecore: invalid save level in core");
	}

	{
		int i;
		int n_pages = index->size;
		register int ret;

		if ((out_fd = open(out_file, O_WRONLY|O_CREAT|O_EXCL, 0644)) < 0)
			PERROR(out_file);

		for (i = 0; i < n_pages; i++)
		{
			if (GETMAP(index, i) >= index->level)
			{
/*
printf("READ: %d\n", i);
*/
				if ((ret = read(in_fd, buffer, NBPG)) < 0)
					PERROR(g_filename);
				if (! ret)
					break;	/* reached EOF */
				if (write(out_fd, buffer, ret) < 0)
					PERROR(out_file);
			}
			else
			{
/*
printf("SEEK: %d\n", i);
*/
				if (lseek(out_fd, NBPG, SEEK_CUR) < 0)
					PERROR(out_file);
			}
		}

		if (close(out_fd) < 0)
			PERROR(out_file);
	}

#ifndef _WSIO
	/* 
	** mark the save level back to the core file 
	*/
	{
		g_nldump = g_nlboot;
		ReadSymbols(g_dsystem, g_nldump);
		WriteFile(out_file, NLDUMP(SYM_SAVECORE), &index->level, 
			sizeof(index->level));
	}
#endif	/*  _WSIO */
}



/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* 
** Module - MIRROR
*/

/* Extra code (this ifdef section) for mirrored dumps */

#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <memory.h>      /* mirror */
#ifndef _WSIO
#include <sys/mirror.h>  /* mirror */
#endif /* _WSIO */

#ifndef _WSIO

typedef struct mirror
{
	struct  pair * pair;
	int	partner_offline;
	ushort	dump_state;
};

static struct	pair * find_dump_mirror(dev_t);
static int 	dooffline(dev_t mirror, int half);
static void	doreimage(dev_t mirror);
static int	open_mirconfig(void);

int g_mirconfig_fd = -1;

#define MIR_PRIMARY	1
#define MIR_SECONDARY	2

/* name of mirror configuration file */
char MIRCONFIG_NAME[] = "/dev/rdsk/mirconfig";

int
mir_offline(const struct dumptab * dumptab)
{
	return dumptab->mirror && 
		(dumptab->mirror->dump_state == MIR_OFFLINE);
}


/*
 * mir_dump_dev
 *
 *	If the dump device is mirrored, do the following:
 *		(1) Determine whether we must take the partner OFFLINE
 *		    before copying data,
 *		(2) Determine which device to use to read the dump based
 *		    on the state of the mirror and whether the dump device
 *		    is the primary or secondary half of the mirror.
 *		(3) Record the mirror devices and states.
 *
 */
void
mir_dump_dev(struct dumptab * dumptab)
{
	struct pair * pair;
	struct mirror * mirror;
	ushort dump_state, partner_state;

	pair = find_dump_mirror(dumptab->dp_dev);

	if (pair == NULL)
	{
		dumptab->mirror = NULL;
		return;
	}

	if (! (mirror = malloc(sizeof(struct mirror))))
	{
		if (g_vflg)
			perror("savecore: mir_dump_dev()");
		dumptab->mirror = NULL;
		return;
	}

	if (dumptab->dp_dev == pair->dev_pri) 
	{
		mirror->dump_state = dump_state = pair->state_pri;
		partner_state = pair->state_sec;
	} 
	else 
	{
		mirror->dump_state = dump_state = pair->state_sec;
		partner_state = pair->state_pri;
	}

	mirror->pair = pair;

	switch (dump_state)
	{
		case MIR_ONLINE:
			if (partner_state == MIR_ONLINE)
				if (! dooffline(pair->dev_pri, 
					(dumptab->dp_dev == pair->dev_pri) ? 
						MIR_SECONDARY : MIR_PRIMARY))
					mirror->partner_offline = 1;

			/* 
			** The online half of the mirror is accessed
			** via the physical address of primary device.
			*/
			dumptab->dp_dev == pair->dev_pri;
			break;

		case MIR_OFFLINE:
			/* 
			** The offline half of the mirror is accessed
			** via the physical address of secondary device.
			*/
			dumptab->dp_dev == pair->dev_sec;
			break;

		case MIR_REIMAGE:
			Warning("savecore: mirrored dump was overwritten by "
				"reimage\n"
				"savecore: cannot determine if core exists\n");
				CleanupAndExit(1);
			break;

		default:
			free(mirror);
			mirror = NULL;
			break;
	}

	dumptab->mirror = mirror;
	return;
}


/*
 * find_dump_mirror
 *
 *	If the dump device is mirrored, return the struct pair of that mirror.
 */
static struct pair *
find_dump_mirror(dev_t dev_id)
{
	static struct mir_state *ms = 0;
	struct pair *dumpmir;
	int ret, i;

	if (g_mirconfig_fd < 0)
	{
		g_mirconfig_fd = open_mirconfig();

		if (g_mirconfig_fd < 0)
			return NULL;

		ms = (struct mir_state *) malloc(mir_state_size(MAX_PAIRCOUNT));
		if (! ms)
		{
			if (g_vflg)
				perror("savecore mirror check");
			return NULL;
		}
		
		ms->paircount = MAX_PAIRCOUNT;
		ms->subcommand = GSTATE;

		ret = ioctl(g_mirconfig_fd, CIOC_MIRROR, ms);
		if (ret < 0)
			return NULL;
	}

	if (! ms)
		return NULL;

	for (i=0; i < ms->paircount; i++)
		if (ms->pairs[i].dev_pri == dev_id ||
		    ms->pairs[i].dev_sec == dev_id)
			break;
	if (i == ms->paircount || ms->pairs[i].error)
		return NULL;

	if (! (dumpmir = (struct pair *) malloc(sizeof(struct pair))))
	{
		if (g_vflg)
			perror("savecore: struct pair -");
		return NULL;
	}

	(void) memcpy(dumpmir, &ms->pairs[i], sizeof(struct pair));
	free(ms);

	return dumpmir;
}



/*
 * mir_cleanup
 *
 */
void
mir_cleanup()
{
	dev_t dev_primary;
	struct dumptab * dumptab, * end;

	for (dumptab = g_dumptab, end = g_dumptab + g_ndumptab;
		dumptab < end;
		dumptab++)
	{
		struct mirror * mirror = dumptab->mirror;
		struct pair * pair;

		if (! mirror)
			continue;

		pair = mirror->pair;
		if (pair->state_pri == MIR_ONLINE && 
				pair->state_sec == MIR_ONLINE &&
				mirror->partner_offline)
			doreimage(pair->dev_pri);
	}
}


/*
 * dooffline
 *
 *	Take the indicated half of the given mirror offline.
 */
static int
dooffline(dev_t mirror, int half)
{
	int ret;
	struct mir_state s_ctl;

	s_ctl.subcommand = SSTATE;
	s_ctl.pairs[0].dev_pri = mirror;
	if (half == MIR_PRIMARY) {
		s_ctl.pairs[0].state_pri = MIR_OFFLINE;
		s_ctl.pairs[0].state_sec = MIR_ONLINE;
	}
	else {
		s_ctl.pairs[0].state_pri = MIR_ONLINE;
		s_ctl.pairs[0].state_sec = MIR_OFFLINE;
	}
	s_ctl.pairs[0].flags = FLAG_BITMAP;
	s_ctl.paircount = 1;

	ret = ioctl(g_mirconfig_fd, CIOC_MIRROR, &s_ctl);

	return (ret != 0)  ?  ret  :  s_ctl.pairs[0].error;
}


/*
 * doreimage()
 *
 * perform a table reimage on a mirror
 * we do a GSTATE to see which is OFFLINE
 * then an SSTATE with the state to REIMAGE
 * then we're done (driver does SSTATE to ONLINE)
 */
static void 
doreimage(dev_t mirror)
{
	int ret;
	struct mir_state s_ctl;

	/* find out about this mirror */
	s_ctl.subcommand = GSTATE;
	s_ctl.paircount = 1;
	s_ctl.pairs[0].dev_pri = mirror;

	ret = ioctl(g_mirconfig_fd, CIOC_MIRROR, &s_ctl);

	/* should come back error-free */
	if (ret || s_ctl.pairs[0].error) 
		return;

	/*
 	 * GSTATE allows either the primary or the secondary to name
	 * the mirror.  Be sure user named primary.
	 */
	if (s_ctl.pairs[0].dev_pri != mirror
	   || s_ctl.pairs[0].state_pri == MIR_UNCONFIG)
		return;
		
	/*
	 * change the non-ONLINE side to REIMAGE
	 * if neither ONLINE, the driver will complain
	 * don't look for OFFLINE specifically, because
	 * the state may already be REIMAGE.
	 */
	if (s_ctl.pairs[0].state_pri != MIR_ONLINE)
		s_ctl.pairs[0].state_pri = MIR_REIMAGE;
	else
	if (s_ctl.pairs[0].state_sec != MIR_ONLINE)
		s_ctl.pairs[0].state_sec = MIR_REIMAGE;
	else
		return;

	/*
	 * We request bitmap.  Return if none exists.
	 */
	if ((s_ctl.pairs[0].flags & FLAG_BITMAP) == 0)
		return;

	/* Clear all flags except FLAG_BITMAP */
	s_ctl.pairs[0].flags &= FLAG_BITMAP;

	/* change the state to REIMAGE */
	s_ctl.subcommand = SSTATE;

	ret = ioctl(g_mirconfig_fd, CIOC_MIRROR, &s_ctl);
}


/*
 *
 * open_mirconfig()
 *
 * gets a file descriptor for subsequent ioctl calls
 *
 */
static int 
open_mirconfig(void)
{
	int mir_fd;
	struct stat statb;

	mir_fd = open(MIRCONFIG_NAME, O_RDONLY);
	if (mir_fd < 0) {
		return;
	}

	/* be sure it has the right mode and major/minor number */
	if (fstat(mir_fd, &statb) < 0) {
		mir_fd = -1;
		return;
	}
#ifdef MIRCONFIG_MAJ
	if ((statb.st_mode & S_IFMT) != S_IFCHR ||
	    statb.st_rdev != makedev(MIRCONFIG_MAJ, MIRCONFIG_MIN)) 
		mir_fd = -1;
#endif
	return mir_fd;
}

#endif /*  _WSIO */



/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* 
** Module - SWAPON
*/

#include <stdio.h>
#include <signal.h>
#include <sys/conf.h>
#include <sys/param.h>
#include <sys/stat.h>

#ifndef _WSIO

const char * BAD_LOCK_FILE_MSG=
	"WARNING:\n"
	"\t/etc/savecore.LCK has been modified by other process,\n"
	"\tpossibly by swapon(1M).  Dump can be corrupted.\n"
	"\tSave core anyway.\n";



void
RunBackground()
{
	if (g_child_process)
		return;

	/*
	** PSD is not used as dump device, safe to 
	** savecore in the background.
	*/
	switch (fork())
	{
		case -1:
			PERROR("savecore");
			break;
		case 0:
			/* 
			** child (background) process!
			** Re-setup all the signal.
			*/
#ifdef DEBUG
			{
				int count = 1;
				while (count) 
					sleep(60); 
			}
#endif /* ! DEBUG */
			(void) signal(SIGHUP, SIG_IGN);
			(void) signal(SIGINT, CleanupAndExit);
			(void) signal(SIGQUIT, CleanupAndExit);
			(void) signal(SIGTERM, CleanupAndExit);
			g_child_process = 1;
			break;
		default:
			fprintf(stderr, "savecore is running "
				"in the background\n");
			exit(0);
	}
}


void
CallSwapon()
{
	/*
	** Call swapon() should only continue if and only if it is
	** a child process AND user did not turn off swapon.
	*/
	if ((g_swaplevel == SWAPON_SKIP) || (! g_child_process))
		return;

	switch (vfork())
	{
		case -1:
			PERROR("savecore");
			break;
		case 0:
			close(1);
			close(2);
			open("/dev/null", O_WRONLY);
			open("/dev/null", O_WRONLY);
			execl("/etc/swapon", "swapon", "-a", NULL);
			Error("Cannot execute /etc/swapon");
			break;
		default:
			wait((int *) 0);
			return;
	}
}



time_t
GetMTime(const char * lck)
{
	static struct stat stat_buf;

	if (stat(lck, &stat_buf) == -1)
		return (time_t) 0;

	return stat_buf.st_mtime;
}


time_t
CreateLock(struct dumptab * dumptab, const int n, const char * lck)
{
	FILE * fp;
	struct dumptab * end;
	struct stat stat_buf;

	if (! (fp = fopen(lck, "w")))
		PERROR(lck);

	for (end = dumptab + n; dumptab < end; dumptab++)
		if (dumptab->locked)
			fprintf(fp, "%08x\n", LOCK_DEVID(dumptab));

	if (fclose(fp))
		PERROR(lck);

	if (stat(lck, &stat_buf) == -1)
		return 0;
	else
		return stat_buf.st_mtime;
}


int
CheckLock(struct dumptab * dumptab, char * const lck)
{
	dev_t id_tbl[MAXDUMPDEV];
	int n;

	/* 
	** Read the content of the lock file.
	*/
	{
		FILE * fp;
		if (! (fp = fopen(lck, "r")))
			return 0;

		for (n = 0; fscanf(fp, "%x", id_tbl + n) != EOF; n++)
			;
		fclose(fp);
	} 

	if (g_ndumptab != n)	/* should have the same number of entries */
		return 0;

	for (n = 0; n < g_ndumptab; n++)
	{
		dev_t dev_id = LOCK_DEVID(dumptab + n);

		if (dev_id == NODEV)
			Error("savecore: bad dev_t");

		if (dev_id != id_tbl[n])
			return 0;
	}
	return 1;	/* everything is okay if we get here */
}


ReleaseLock(const int entry, time_t * mtime)
{

	if (*mtime != GetMTime(SAVECORE_LCK))
		Warning(BAD_LOCK_FILE_MSG);

	if (! g_dumptab[entry].locked)
		Error("savecore: invalid lock value");
	else
		g_dumptab[entry].locked = 0;

	*mtime = CreateLock(g_dumptab, g_ndumptab, SAVECORE_LCK);

	if (*mtime != GetMTime(SAVECORE_LCK))
		Warning(BAD_LOCK_FILE_MSG);
}

#endif /*  _WSIO */



/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* 
** Module - SYMBOL
*/

#include <stdio.h>
#include <nlist.h>

struct nlist g_nlboot[] =
{
	{ SYM_VERSION },
	{ SYM_DUMPLO },
	{ SYM_TIME },
	{ SYM_DUMPSIZE },
	{ SYM_PANICSTR },
	{ SYM_DUMPMAG },
#ifndef	_WSIO
	{ SYM_DUMPDEVT },
	{ SYM_NUM_DUMPDEVS },
	{ SYM_SWDEVT },
	{ SYM_PDIR },
	{ SYM_NPDIR },
	{ SYM_HTBL },
	{ SYM_NHTBL },
	{ SYM_SAVECORE },
#else /* ! _WSIO */
	{ SYM_DUMPDEV },
#endif	/* _WSIO */
	{ 0 }	/* IMPORTANT - DO NOT LEAVE OUT THIS ZERO */
};

struct nlist * g_nldump;

void
ReadSymbols(const char * system, struct nlist * list)
{

	if (nlist(system, list) == -1)
		PERROR(system);

	for (; list->n_name; list++)
	{
		ERR_STR err_str = "savecore: %s: %s not in namelist\n";
		if (! list->n_value)
		{
			fprintf(stderr, err_str, system, list->n_name);
        		exit(1);
		}
	} 
}


int Value(const struct nlist nlist[], const char * symbol)
{
	for (; nlist->n_name; nlist++)
		if (! strcmp(nlist->n_name, symbol))
			return nlist->n_value;
	Error("savecore: cannot find symbol");
	CleanupAndExit(1);
}


struct nlist *
Clone_nlboot(void)
{
	char * buf;
	int size = sizeof(g_nlboot);

	if (! (buf = (char *) malloc(size)))
		PERROR("savecore");
	(void) memcpy(buf, g_nlboot, size);
	return (struct nlist *) buf;
}


/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* 
** Module - DUMPTAB
*/

#include <stdio.h>
#include <string.h>

#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/conf.h>
#ifdef __hp9000s800
#include <sys/libIO.h>
#endif /* __hp9000s800 */

#ifdef __hp9000s300
#define MAX_READ	16
#else	/*  __hp9000s300 */
#define MAX_READ	128
#endif /* __hp9000s300 */

int g_dumpsize;
int g_devsize;


#ifdef NO_DEVNM
#include <sys/dir.h>

int
devnm(int devtype, dev_t devid, char* path, int pathlen, int cache)
{
/* When chaning DEVDSKDIR, also change the hardcoded constant in the
 * strcpy() below.
 */
#define DEVDSKDIR "/dev/dsk/"
	register DIR * dfd = opendir(DEVDSKDIR);
	struct direct *dir;
	struct stat statb;
	char *dp;
	char *raw;

	(void) strcpy(path, DEVDSKDIR);
	while ((dir = readdir(dfd))) {
		(void) strcpy(path + strlen(DEVDSKDIR), dir->d_name);
		if (stat(path, &statb)) {
			perror(path);
			continue;
		}
		if ((statb.st_mode&S_IFMT) != devtype)
			continue;
		if (devid == statb.st_rdev) {
			closedir(dfd);
			return 0;
		}
	}
	closedir(dfd);
	fprintf(stderr, "savecore: Can't find a file in %s with "
		"device major=%d, minor=0x%06x\n",
		DEVDSKDIR, major(devid), minor(devid));
	return -2;
}
#else
#include <devnm.h>
#endif /* NO_DEVNM */

/* FindDev() */
/* Look at devnm() man page. */
static char *
FindDev(int type, dev_t devid)
{
#	define CACHE 1		/* cache the search path */
	static char path[MAXPATHLEN+1];
	switch (devnm(type, devid, path, MAXPATHLEN+1, CACHE))
	{
		char * name;

		case 0:
			if (! (name = (char *) malloc(strlen(path) + 1)))
				PERROR("savecore");
			else
			{
				(void) strcpy(name, path);
				return name;
			}
		case -1:
			/*
			** "errno" is the value returned from ftw().
			*/
			perror("savecore");
			return NULL;
		case -2:
			return NULL;
		case -3:
			return NULL;
	}
}


/*
 *      rawname - If cp is a block device file, try to figure out the
 *      corresponding character device name by adding an r where we think
 *      it might be.
 */
static char *
RawName(char * cp)
{
	static char rawbuf[MAXPATHLEN + 1];
	char *dp = strrchr(cp, '/');

	if (dp == 0)
		return (0);
	while ( dp >= cp && *--dp != '/' );

	/*  Check for AT&T Sys V disk naming convention
	 *    (i.e. /dev/dsk/c0d0s3).
	 *  If it doesn't fit the Sys V naming convention and
	 *    this is a series 200/300, check for the old naming
	 *    convention (i.e. /dev/hd).
	 *  If it's neither, change the name as if it's got Sys V
	 *    naming.
 	 */
#ifdef __hp9000s300
	if ( strncmp(dp, "/dsk", 4) ) {
		dp = (char *) strrchr(cp, '/');
		if ( strncmp(dp, "/hd", 2) )
			while ( dp >= cp && *--dp != '/' );
	}
#endif /* __hp9000s300 */

	*dp = 0;
	(void) strcpy(rawbuf, cp);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp+1);

	{
		char * buf;
		if (! (buf = (char *) malloc(strlen(rawbuf) + 1)))
			PERROR("savecore");
		else
		{
			(void) strcpy(buf, rawbuf);
			return buf;
		}
	}
}


#ifdef	 __hp9000s800 /* [map_mi_to_lu(devp)] */
#define FAILURE		1  /* SUCCESS is defined in libIO.h */
#define LU_MASK		0x0000ff00
#define LU_SHIFT	8
#define LU(dev)		((dev & LU_MASK) >> LU_SHIFT)
#define MI(dev)		((dev & LU_MASK) >> LU_SHIFT)


/*
 * NOTE:  This is an s800 only routine.  It uses routines from the libIO.a
 * library which are not supported for the s700.  This routine should not be
 * called if executing on a s700.
 */

int
map_mi_to_lu(dev_t * devp)
{
	io_node_type	io_node;
	int		status, mgr_index, type, try=0;

	if ((status = io_init(O_RDONLY)) != SUCCESS) {
		(void)print_libIO_status("savecore", _IO_INIT, status, 
			O_RDONLY);
		return(FAILURE);
	}

	io_node.b_major = major(*devp);
	mgr_index = MI(*devp);

	do {
		type = SEARCH_FIRST;
		do 
		{
			if ((status = io_search_tree(type, KEY_B_MAJOR, 
				QUAL_NONE, &io_node)) < 0) 
			{
				if (status == STRUCTURES_CHANGED)
					break;
				else 
				{
					(void)print_libIO_status("savecore", 
						_IO_SEARCH_TREE, status, type, 
						KEY_B_MAJOR, QUAL_NONE, 
						&io_node);
					io_end();
					return(FAILURE);
				}
			}
	    		if (io_node.mgr_index == mgr_index) 
			{
				if (io_node.lu == NONE) 
				{
		    			fprintf(stderr,
						"savecore: No logical unit "
						"for dump device\n");
		    			io_end();
		    			return(FAILURE);
	        		} 
				else 
				{
					*devp = (*devp & ~LU_MASK) | 
						(io_node.lu << LU_SHIFT);
					io_end();
					return(SUCCESS);
				}
			}
			type = SEARCH_NEXT;
		} while (status > 0);
	} while (status == STRUCTURES_CHANGED && ++try < MAX_TRIES);

	if (status == STRUCTURES_CHANGED)
		(void) print_libIO_status("savecore", _IO_SEARCH_TREE, status,
		type, KEY_B_MAJOR, QUAL_NONE, &io_node);
	else	/* Couldn't find the manager index in the kernel */
        	fprintf(stderr, "savecore: No dump device configured\n");
	io_end();
	return(FAILURE);
}

#endif /* __hp9000s800 [map_mi_to_lu(devp)] */


static void
ReadDeviceName(struct dumptab * dumptab)
{
	dev_t dumpdev_id;

#ifdef __hp9000s800
	/* Replace the manager index in dumpdev with the
	** logical unit number on the s800 for autoconfig.
	** Also, if the dump device is mirrored, we may have to use
	** the partner device to get a core, so use that devno 
	** instead.
	** Don't do for the s700.
	*/
	if (sysconf(_SC_IO_TYPE) == IO_TYPE_SIO)
	{
		if (map_mi_to_lu(&dumptab->dp_dev) != SUCCESS)
			CleanupAndExit(1);
	}

#ifdef SAVECORE_MIRROR
	mir_dump_dev(dumptab);
#endif	/* SAVECORE_MIRROR */

#endif /* __hp9000s800 */

	dumpdev_id = dumptab->dp_dev;

	if (! (dumptab->blk_dev = FindDev(S_IFBLK, dumpdev_id)))
	{
		fprintf(stderr, "savecore: Can't find a file "
			"with device major=%d, minor=0x%06x\n",
			major(dumpdev_id), minor(dumpdev_id));
		CleanupAndExit(1);
	}

	/* If no char special file, use block special file. */
	{
		struct stat statb;
		dumptab->chr_dev = RawName(dumptab->blk_dev);
		if (stat(dumptab->chr_dev, &statb) < 0)
			dumptab->chr_dev = dumptab->blk_dev;
	}

	return;
}


void 
DumpTableInit(struct dumptab * dumptab)
#ifdef _WSIO
{
	int dumpdev_id;

	READ_KMEM(NLBOOT(SYM_DUMPDEV), &dumpdev_id, sizeof(dumpdev_id));

	dumptab->dp_dev = dumpdev_id;
	ReadDeviceName(dumptab);

	READ_KMEM(NLBOOT(SYM_DUMPLO),&dumptab->dumplo,sizeof(dumptab->dumplo));
	dumptab->dumplo *= DEV_BSIZE;

	/*
	** Use only block device to read in dumpsize since
	** this is not a location accessable to the raw device.
	*/
	ReadFile(dumptab->blk_dev, dumptab->dumplo + NLDUMP(SYM_DUMPSIZE),
		&g_dumpsize, sizeof(g_dumpsize));
	g_dumpsize *= sysconf(_SC_PAGE_SIZE);
	dumptab->size = g_dumpsize;

	g_ndumptab = 1;
}
#else /* ! _WSIO */
{
	int ndumpdevs;
	struct dumpdevt * dumpdevt, * dt;

	READ_KMEM(NLBOOT(SYM_NUM_DUMPDEVS),&ndumpdevs,sizeof(ndumpdevs));

	if (ndumpdevs <= 0 || ndumpdevs > MAXDUMPDEV)
	{
		if (g_vflg)
			Warning("savecore: No dump device found.");
		exit(2);
	}

	/* Reading in structure dumpdevt from the system */
	{
		int size = sizeof(struct dumpdevt) * ndumpdevs;
		if (! (dumpdevt = (struct dumpdevt *) malloc(size)))
			PERROR("savecore");
		READ_KMEM(NLBOOT(SYM_DUMPDEVT), dumpdevt, size);
	}

	memset(dumptab, 0, MAXDUMPDEV * sizeof(struct dumptab));

	g_ndumptab = 0;

	/* 
	** Initializes the dump table. 
	*/
	for (dt = dumpdevt; dt < dumpdevt + ndumpdevs; dt++)
	{
		if (dt->used && dt->initialized)
		{
#ifdef SAVECORE_MIRROR
			dumptab->mirror = 0;
#endif /* SAVECORE_MIRROR */
			dumptab->dp_dev = dt->dp_dev;
			ReadDeviceName(dumptab);
			dumptab->lv = dt->lv;
			dumptab->dumplo = dt->dumplo * DEV_BSIZE;
			dumptab->size = (dt->size - dt->dumplo) * DEV_BSIZE; 
			dumptab->locked = 1;
			dumptab++;
			g_ndumptab++;
		}
	}

	if (READ_DUMP(NLDUMP(SYM_DUMPSIZE),&g_dumpsize,sizeof(g_dumpsize)) < 0)
		CleanupAndExit(2);
	else
		g_dumpsize *= sysconf(_SC_PAGE_SIZE);

	if (g_ndumptab == 0)
	{
		if (g_vflg)
			Warning("savecore: No dump devices configured");
		exit(2);
	}
}
#endif /* _WSIO */

#ifdef	SELECTIVE_RETRIEVE
static void 
DumpTableInit_File(struct dumptab * dumptab)
{
	/* Setup dumpdev */
	dumptab->blk_dev = dumptab->chr_dev = g_filename;
	dumptab->dumplo = 0;
	ReadFile(g_filename, NLDUMP(SYM_DUMPSIZE), &dumptab->size, 
		sizeof(dumptab->size));
	dumptab->size *= sysconf(_SC_PAGE_SIZE);
	g_dumpsize = dumptab->size;

	g_ndumptab = 1;
}
#endif	/* SELECTIVE_RETRIEVE */


int
WhereInDump(int addr, int * disk, int * offset)
{
	int i;

	for (i = 0; i < g_ndumptab; i++)
	{
		if (addr < g_dumptab[i].size)
		{
			*disk = i;
			*offset = addr;
			return 0;
		}
		else
			addr -= g_dumptab[i].size;
	}
	return -1;
}


int 
DumpAccess(const int type, long addr, void * buf, int size)
{
	struct dumptab * dumptab;
	int disk;
	int offset;
	int sz;

	if (WhereInDump(addr, & disk, & offset))
		return -1;

	dumptab = g_dumptab + disk;

	sz = MIN((dumptab->size - offset), size);

	switch (type)
	{
		case O_RDONLY:
			ReadFile(dumptab->blk_dev, dumptab->dumplo + offset, 
				buf, sz);
			break;
		case O_WRONLY:
#ifdef SAVECORE_MIRROR
			if (mir_offline(dumptab))
			{
				Warning(
					"Core on OFFLINE half of mirror"
					"- still active on dump "
					"device\n");
						CleanupAndExit(0);
			}
			else
#endif /* SAVECORE_MIRROR */
				WriteFile(dumptab->blk_dev,
					dumptab->dumplo + offset, buf, sz);
				break;
		default:
			Error("savecore: invalid dump access");
	}

	/* WARNING - RESURSION */
	/*
	** If the memory chunk span more than one devices, do a recursive
	** read/write.
	*/
	if (size -= sz)
		DumpAccess(type, addr + sz, ((char *) buf) + sz, size);
	else
		return 0;
}


/* 
** IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT 
*/
void 
DumpToFile(int in_fd, int * p_out_fd , int bytes_to_copy, 
	const struct savecore_index * index, unsigned int pos)
{
	static char buffer[NBPG * MAX_READ];
	register int bufsz = NBPG * ((g_save_level) ? 1 : MAX_READ);
	register int size;


	for (size = MIN((bufsz - (pos%bufsz)), bytes_to_copy);
		bytes_to_copy > 0;
		bytes_to_copy -= size, pos += size, 
			size = MIN(bytes_to_copy,bufsz))
	{
#ifdef	SELECTIVE_RETRIEVE
		if (index && g_save_level && 
			(GETMAP(index,pos>>PGSHIFT) < g_save_level))
		{
			if (lseek(in_fd, size, SEEK_CUR) < 0)
				PERROR("savecore");
			if (g_Sflg)
			{
				if (lseek(*p_out_fd, size, SEEK_CUR) < 0)
					PERROR("savecore");
			}
		}
		else
#endif	/* SELECTIVE_RETRIEVE */
		{
			if (read(in_fd, buffer, size) != size)
				PERROR("savecore");
			if (g_tdevice)
				Tape_write(p_out_fd, buffer, size);
			else
				if (write(*p_out_fd, buffer, size) != size)
					PERROR("savecore");
		}
	}
}



/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* 
** Module - TAPE
*/

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <fcntl.h>

int g_tape_oflag;

static void
Prompt(const char * msg)
{
	int ans;
	ERR_STR str = "Enter <CR> to continue or 'q' to quit: ";
	for (;;)
	{
		fprintf(stderr, "%s\n%s", msg, str);
		ans = getc(stdin);
		if (ans == 'q' || ans == 'Q')
			Error("Quitting, dump file is not completely saved");
		if (ans == '\n')
			return;
	}
}


static int
EndOfTape(const int fd)
{
	static struct mtget mtget_buf;

	if (ioctl(fd, MTIOCGET, &mtget_buf) < 0)
		PERROR("EOM query");
	return (GMT_EOT(mtget_buf.mt_gstat));
}


static int
EndOfFile(const int fd)
{
	static struct mtget mtget_buf;

	if (ioctl(fd, MTIOCGET, &mtget_buf) < 0)
		PERROR("EOM query");
	return (GMT_EOF(mtget_buf.mt_gstat));
}


static int
BeginOfTape(int fd)
{
	struct mtget mtget_buf;

	if (ioctl(fd, MTIOCGET, &mtget_buf) < 0)
		PERROR("EOM query");
	return (GMT_BOT(mtget_buf.mt_gstat));
}


static int
TapeOnline(int fd)
{
	struct mtget mtget_buf;
	if (ioctl(fd, MTIOCGET, &mtget_buf) < 0)
		PERROR("EOM query");
	return GMT_ONLINE(mtget_buf.mt_gstat);
}


void
MarkEOF(const int fd, const int times)
{
	struct mtop mtop_buf;
	mtop_buf.mt_op = MTWEOF;
	mtop_buf.mt_count = times;
	if (ioctl(fd, MTIOCTOP, &mtop_buf) < 0)
		PERROR(g_tdevice);
}

struct tape_header
{
	int tcount;
	char id[256-sizeof(int)];
} g_tape_header;


static void
SwapTape(int * p_fd, const char * msg)
{
	if (close(* p_fd) < 0)
		PERROR(g_tdevice);

	Prompt(msg);

	while (1)
	{
		if ((*p_fd = open(g_tdevice, g_tape_oflag)) < 0)
		{
			ERR_STR msg = "Cannot open tape file";
			perror(g_tdevice);
			Prompt(msg);
		}
		else
			break;
	}

	/* Check the status of the tape */
	if (! BeginOfTape(*p_fd))
		Error("savecore: not beginning of tape");
}


static void
ReadTapeHeader(int * p_fd)
{
	static int tape = 1;
	static struct tape_header head;
	while (1)
	{
		static char msg[128];

		if (read(*p_fd, &head, sizeof(head)) < 0)
			PERROR(g_tdevice);

		if (head.tcount == tape && !strcmp(g_tape_header.id, head.id))
		{
			tape++;
			return;
		}

		sprintf(msg, "savecore: incorrect tape mounted.  "
			"Tape %d expected.\nMount the correct tape.\n", 
			tape);

		SwapTape(p_fd, msg);
	}
}


static void
WriteTapeHeader(int * p_fd)
{
	static int tape = 1;

	g_tape_header.tcount = tape;

	if (! BeginOfTape(*p_fd))
		Error("savecore: tape not at the beginning of tape");

	if (write(*p_fd, &g_tape_header, sizeof(g_tape_header)) < 0)
		PERROR(g_tdevice);
	tape++;
}


int 
Tape_open(const char * name, int oflag)
{
	int fd;

	if ((fd = open(name, g_tape_oflag = oflag)) < 0)
		PERROR(name);

	/* 
	** Check the status of the tape 
	*/
	if (! TapeOnline(fd))
		Error("savecore: tape is offline");

	if (! BeginOfTape(fd))
		Error("savecore: not beginning of tape");

	if (g_xflg)
		ReadTapeHeader(&fd);
	else
		WriteTapeHeader(&fd);

	return fd;
}


static int 
Tape_read(int * p_fd, void * buffer, int size)
{
	ERR_STR msg = "Savecore: End of tape encountered.  "
		"Mount the next tape.";
	int save_errno;
	int ret;

	if ((ret = read(* p_fd, buffer, size)) > 0)
		return ret;

	/* ret >= 0 here */

	save_errno = errno;

	if (EndOfTape(* p_fd))
	{
		SwapTape(p_fd, msg);
		ReadTapeHeader(p_fd);
		return Tape_read(p_fd, buffer, size);
	}

	if (EndOfFile(* p_fd))
		return 0;

	errno = save_errno;
	PERROR(g_tdevice);
}


#ifdef LARRY
static int 
Tape_read(int * p_fd, void * buffer, int size)
{
	ERR_STR msg = "Savecore: End of tape encountered.  "
		"Mount the next tape.";
	int save_errno = errno;
	int ret;

	if ((ret = read(* p_fd, buffer, size)) > 0)
		return ret;

	/* ret >= 0 here */

	if (! ret && EndOfFile(* p_fd))
		return 0;

	errno = save_errno;

	if (! EndOfTape(* p_fd))
	{
		errno = save_errno;
		PERROR(g_tdevice);
	}

	/* ... is EOT */
	SwapTape(p_fd, msg);
	ReadTapeHeader(p_fd);
	return Tape_read(p_fd, buffer, size);
}

#endif /*  LARRY */

#ifdef LARRY

static int 
Tape_read(int * p_fd, void * buffer, int size)
{
	int save_size = size;
	register char * buf = buffer;
	register int sz;
	register int ret = 0;

	for (sz = MIN(TSIZE, size); size; 
		size -= ret, buf += ret, sz = MIN(TSIZE, size))
	{
		if (EndOfTape(*p_fd))
		{
			ERR_STR msg = "Savecore: End of tape encountered.  "
				"Mount the next tape.";

			SwapTape(p_fd, msg);
			ReadTapeHeader(p_fd);
		}

		if ((ret = read(*p_fd, buf, sz)) < 0)
			PERROR(g_tdevice);

		if (EndOfFile(*p_fd))
			return (save_size - (size - ret));

	}

	return save_size;
}

#endif /* LARRY */

void
Tape_write(int * p_fd, char * buf, size_t bytes_to_copy)
{
	ERR_STR msg = "Savecore: End of tape encountered.  "
		"Mount the next tape.";
	int sz;
	int ret;
	int save_errno;

	for (sz = MIN(TSIZE, bytes_to_copy); bytes_to_copy > 0;
		bytes_to_copy-=ret, buf+=ret, sz=MIN(TSIZE, bytes_to_copy))
	{
		if ((ret = write(*p_fd, buf, sz)) > 0)
			continue;

		if (! ret || ! sz)
			Error("sz is equal to zero");

		save_errno = errno;

		if (! EndOfTape(*p_fd))
		{
			errno = save_errno;
			PERROR(g_tdevice);
		}

		/* Is EOT */
		SwapTape(p_fd, msg);
		WriteTapeHeader(p_fd);
		ret = 0;
	}
}


void
GetCoreFromTape(void)
{
	char * path;
	int size;
	char buffer[TSIZE];
	const int bufsz = TSIZE;
	int bound;
	int ofd;
	int ifd;

	ifd = Tape_open(g_tdevice, O_RDONLY);

	if ((size = Tape_read(&ifd, buffer, bufsz)) < 0)
		PERROR(g_tdevice);

	bound = LockReadThenIncrBound(g_dirname, "bounds");
	path = PathName(g_dirname, "hp-core", bound);

	if (((struct savecore_index *)buffer)->magic == SAVECORE_MAGIC)
		strcat(path, ".I");

	if ((ofd = open(path, O_WRONLY|O_CREAT, 0644)) < 0)
		PERROR(path);

	do
	{
		if ((write(ofd, buffer, size)) < 0)
			PERROR(path);
	} while (size = Tape_read(&ifd, buffer, bufsz));

	if (close(ofd) < 0)
		PERROR(path);

	path = PathName(g_dirname, "hp-ux", bound);
	size = Tape_read(&ifd, buffer, bufsz);
	if (! size)
	{
		close(ifd);
	}
	else
	{
		if ((ofd = open(path, O_WRONLY|O_CREAT, 0644)) < 0)
			PERROR(path);
		do
		{
			if (write(ofd, buffer, size) < 0)
				PERROR(path);
		} while (size = Tape_read(&ifd, buffer, bufsz));
		if (close(ofd) < 0)
			PERROR(path);
	}
	return;
}


