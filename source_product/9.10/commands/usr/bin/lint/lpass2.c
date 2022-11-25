/* @(#) $Revision: 70.23 $ */      
/* lpass2.c
 *	This file includes the main functions for the second lint pass.
 *
 *	Functions:
 *	==========
 *		chkcompat	check for various sorts of compatibility
 *		chktype		check two types to see if they are compatible
 *		cleanup		do wrapup checking
 *		lastone		check set/use in a last pass over the symbol table
 *		lread		read in a line from the intermediate file
 *		main		the driver for the second pass (lint2)
 *		mloop		the main loop of the second pass
 *		setfno		set file number (keep track of source file names)
 *		setuse		fix set/use and other information for objects
 */


# include <malloc.h>
# include <string.h>
# include <sys/types.h>
# include "manifest" 
# include "lerror.h"
# include "lmanifest"
# include "lpass2.h"
#ifdef APEX
# include "apex.h"

#define MAX(a,b)        (a>b ? a : b)
extern int opterr;
extern int *install_dims();
#endif


#ifdef STATS_UDP
/* instrumentation for beta releases. */
#define INST_UDP_PORT		42963
#define INST_UDP_ADDR		0x0f01780f 
#define INST_VERSION		1 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
/* udp packet definition */
struct udp_packet {
    int size;			/* size of the source file in bytes */
    char name[16];		/* basename of the file */
    unsigned long inode;	/* inode */
    int flag_ansi:1;		/* -Aa used */
    int flag_a:1;	/* -a used */
    int flag_b:1;	/* -b used */
    int flag_h:1;	/* -h used */
    int flag_p:1;	/* -p used */
    int flag_u:1;	/* -u used */
    int flag_v:1;	/* -v used */
    int flag_x:1;	/* -x used */
    int flag_y:1;	/* -y used */
    int flag_N:1;	/* -N used (table size reset) */
    int flag_Z:1;	/* -Z used (lint2 table size) */
    int flag_w:1;
    int dummy:16;
    unsigned int pass:2;	/* lint1 vs. lint2 */
    int version:2;		/* compiler version id */
};
/* The following defines should be passed in, the examples below were used
 * in the prototype: -DINST_UDP_PORT=49963 -DINST_UDP_ADDR=0x0f01780d */
/* pick a udp port that is unlikely to be used elsewhere
/* compiler version id 
#define INST_VERSION		1 */
#endif /* STATS_UDP */

# define USED 01
# define VUSED 02
# define EUSED 04
# define RVAL 010
# define VARARGS 0100

# define NSZ 1024
#	define TYSZ 3500	/* should be 2500 temp fix for mjs */
#	define TYINC 1024
#	define FSZ 150		/* used to be 80 */
#	define FNMINC FSZ

STAB *stab;			/* second pass symbol table */
STAB *stab_blk;			/* pool of symtab entries */

#ifdef APEX
static STAB *curr_ftn = NULL;  /* remember the current entry so APEX
                                   comments can be appended */
char *inbuf;
#define DEF_INBUF_SZ  1024
size_t inbuf_sz = DEF_INBUF_SZ;
int *argdimary;
#define ARGDIMSZ        128
int argdimsz = ARGDIMSZ;
int argdfree;
int language;
char **fmtstrings;
size_t unshroud();
#define DIMARYSZ 1024
int *dimary;
int *dfree;
#endif

unsigned stab_nextfree = SYMTBLKSZ;
#ifndef APEX
char strbuf[BUFSIZ];
	char *getstr();
#endif
char *savestr();		/* copy string into permanent string storage */
STAB *lread();
STAB *mloop();
void chkuse();

ATYPE *tary;

	char **fnm;	/* stack of file names */

unsigned int tfree=0;  /* used to allocate types */
int ffree=0;  /* next available filename entry (fnm) */

#define MAXNUMARGS 64
int numargs = MAXNUMARGS;	/* current size of argument type buffer */
ATYPE *atyp;
union rec r;			/* where all the input ends up */

int hflag = 1;			/* 28 feb 80  reverse sense of hflag */
int pflag = 0;
int xflag = 1;			/* 28 feb 80  reverse sense of xflag */
int uflag = 1;
int nuflag = 0;			/* warn about externals not used */
int ndflag = 0;			/* warn about externals not defined */
/* added -y option not to distinguish between longs and ints  */
int yflag = 0;
size_t nsize = NSZ;
size_t tsize = TYSZ;
size_t fsize = FSZ;
int ansimode = 0;

static int idebug = 0;
static int odebug = 0;
int warnmask = WALLMSGS;

#ifdef APEX
struct std_defs *target_list, *origin_list;
int stds[NUM_STD_WORDS];
int target_option[NUM_STD_WORDS];
#ifdef DOMAIN
#define DEFAULT_ORIGIN 2
#else
#define DEFAULT_ORIGIN 1        /* HPUX */
#endif
int origin = DEFAULT_ORIGIN;
int detail = 255;
char *target_file = "/usr/apex/lib/targets";
char *origin_file = "/usr/apex/lib/origins";
int info_only = FALSE;          /* for -t? and -o? */
int apex_flag = 0;
int show_calls = 0;
int show_sum = TRUE;
int show_hints = TRUE;
int show_stds = TRUE;
int check_typedef = FALSE;
int check_common  = FALSE;
int check_format  = FALSE;
int check_dims = FALSE;
int further_detail = 0;         /* more detail available at level... */
int further_hdr_detail = 0;
int num_hdr_stds = 0;           /* total number header file
                                 * violations detected by hdck */
int num_hdr_hints = 0;
char *next_target, *comma_ptr;          /* for parsing -t */
int target_bit;
int target_set = FALSE;         /* flag to indicate that -t seen */
int multiple_targets = FALSE;	/* multiple targets means that all hints
				 * should be labeled */

extern int readtype();
extern char *lookup_typename();
extern int num_undef;		/* Defined in lerror2.c - count of used but
				 * not defined routines */
#endif


extern char	*htmpname;
extern char *optarg;
extern int optind;
int cfno;				/* current file number */
int cmno;				/* current module number */
/* main program for second pass */

main( argc, argv ) char *argv[]; 
{
	int c;
	int i;
	char		*ifilename = NULL;
	int Hflag = 0, Tflag = 0;
#ifdef APEX
        long dummy_std[NUM_STD_WORDS];  /* used to extract the origin word */
	int apex_mask = WALWAYS|WOBSOLETE|WHINTS;
	int format_magic;
#endif

	while((c=getopt(argc,argv,"abcd:hno:pst:uvw:xyAC:H:LN:O:PR:S:T:X:Z:")) != EOF)
		switch(c) {
		case 'h':
			hflag = 0;
			break;

		case 'p':
			pflag = 1;
			break;

		case 'w':
			{
			int k;
			char *nptr = optarg;
			for (nptr = optarg;*nptr;nptr++){
			    switch(*nptr){
			      case 'A':
				warnmask = ~warnmask & WALLMSGS;
				apex_mask = (~apex_mask & WALLMSGS) |
						(WALWAYS|WOBSOLETE|WHINTS);
				break;
			      case 'a':
				warnmask ^= WANSI;
				apex_mask ^= WANSI;
				break;
			      case 'c':
				warnmask ^= WUCOMPARE;
				apex_mask ^= WUCOMPARE;
				break;
			      case 'd':
				warnmask ^= WDECLARE;
				apex_mask ^= WDECLARE;
				break;
			      case 'h':
				warnmask ^= WHEURISTIC;
				apex_mask ^= WHEURISTIC;
				break;
			      case 'k':
				warnmask ^= WKNR;
				apex_mask ^= WKNR;
				break;
			      case 'l':
				warnmask ^= WLONGASSIGN;
				apex_mask ^= WLONGASSIGN;
				break;
			      case 'n':
				warnmask ^= WNULLEFF;
				apex_mask ^= WNULLEFF;
				break;
			      case 'o':
				warnmask ^= WEORDER;
				apex_mask ^= WEORDER;
				break;
			      case 'p':
				warnmask ^= WPORTABLE;
				apex_mask ^= WPORTABLE;
				break;
			      case 'r':
				warnmask ^= WRETURN;
				apex_mask ^= WRETURN;
				break;
			      case 'u':
				warnmask ^= WUSAGE;
				apex_mask ^= WUSAGE;
				break;
			      case 'C':
				warnmask ^= WCONSTANT;
				apex_mask ^= WCONSTANT;
				break;
			      case 'D':
				warnmask ^= WUDECLARE;
				apex_mask ^= WUDECLARE;
				break;
			      case 'O':
				warnmask ^= WOBSOLETE;
				apex_mask ^= WOBSOLETE;
				break;
			      case 'P':
				warnmask ^= WPROTO;
				apex_mask ^= WPROTO;
				break;
			      case 'R':
				warnmask ^= WREACHED;
				apex_mask ^= WREACHED;
				break;
			      case 'S':
				warnmask ^= WSTORAGE;
				apex_mask ^= WSTORAGE;
				break;
#ifdef APEX
			      case 'b':
				check_common = TRUE;
				break;
			      case 's':
				show_stds = !show_stds;
				break;
			      case 't':
				check_typedef = !check_typedef;
				break;
			      case 'H':
				show_hints = !show_hints;
				break;
			      case 'f':
				check_format = TRUE;
				break;
			      case 'i':
				/* suppress header file checker */
				break;
			      case 'I':
				check_dims = !check_dims;
				break;
			      case 'L':
				show_calls = !show_calls;
				break;
			      case 'N':
				show_sum = !show_sum;
				break;
			      case 'x':
			      case 'X':
				/* Domain extensions caught in the frontend */
				break;
#endif
			    }
			}
		    	}
			break;

		case 'x':
			xflag = 0;
			break;

		case 'y':
			yflag = 1;
			break;

#ifdef APEX
                case 'o':
                case 't':
                        /* set below, after we read the config file */
                        break;
#endif
		case 'u':
			uflag = 0;
			break;

		case 'A':
			ansimode = 1;
			break;

		case 'H':
			htmpname = optarg;
			Hflag = 1;
			break;

#ifdef APEX
                case 'd':
                        detail = atoi(optarg);
                        if (detail < 0 || detail > 255) {
                            fprintf(stderr, "Invalid detail level (ignored)\n");
                            detail = 255;
                        }
                        break;

                case 'O':
                        origin_file = optarg;
                        break;

                case 'P':
                        apex_flag = 1;
                        break;

                case 'S':
                        target_file = optarg;
                        break;
#endif
		case 'T':
			ifilename = optarg;
			Tflag = 1;
			break;

			/* debugging options */
		case 'X':
			for(; *optarg; optarg++)
				switch(*optarg) {
				case 'i':
					idebug = 1;
					break;
				case 'o':
					odebug = 1;
					break;
				} /*end switch(*optarg), for*/
			break;

			/* options to set table sizes */
		case 'Z':
			switch(*optarg) {
				case 'n':
					nsize = atoi(++optarg);
					break;
				case 't':
					tsize = atoi(++optarg);
					break;
				case 'f':
					/* +1 for llib-lc */
					fsize = atoi(++optarg) + 1;
					break;
				default:
					lerror("unrecognized -Z option",ERRMSG);
				} /*end switch(*optarg) */
			break;

		case 'a':		/* first pass option */
		case 'b':		/* first pass option */
		case 'c':		/* first pass option */
		case 'n':		/* ccom option to enable NLS */
		case 's':		/* first pass option */
		case 'v':		/* first pass option */
		case 'L':		/* first pass option */
		case 'N':		/* first pass option */
			break;
		} /*end switch for processing args*/
#ifdef APEX
        if (apex_flag) {
	    int flag_num;

            target_list = get_names(target_file);
            origin_list = get_names(origin_file);
            if (!target_list || !origin_list) {
                fprintf(stderr, "Cannot open configuration file: ",
                                target_file);
                if (!target_list)
                        fprintf(stderr, "%s\n", target_file);
                if (!origin_list)
                        fprintf(stderr, "%s\n", origin_file);
                exit(-1);
            }


	    warnmask = apex_mask;
            /* re-parse the target and origin option, now that we know the
             * available names.
             */
            opterr = 0;         /* no sense repeating any errors */
            optind = 1;         /* restart scan from beginning of argv */
            while((c=getopt(argc,argv,"abcd:hno:pr:st:uvw:xyAH:LN:O:PS:T:X:Z:"
)) != EOF)
                switch(c) {
                        case 'o':
                            if (*optarg == '?') {
                                fprintf(stderr, "Available origins:");
                                print_names(origin_list);
                                info_only = TRUE;
                            } else {
                                origin = get_bit(dummy_std, optarg, origin_list)
;
                                if (origin < 0) {
                                    fprintf(stderr,
                                            "Unrecognized origin: %s\n", optarg)
;
                                    fprintf(stderr,
                                            "Available origins:");
                                    print_names(origin_list);
                                    origin = DEFAULT_ORIGIN;
                                } else
                                    origin = dummy_std[0];
                            }
                        break;

                        case 't':
                            next_target = optarg;
                            std_zero(stds);
			    std_zero(target_option);
                            if (*optarg == '?') {
                                fprintf(stderr, "Available targets:");
                                print_names(target_list);
                                info_only = TRUE;
                            } else {
                                while (comma_ptr = strchr(next_target, ',') ) {
                                    *comma_ptr = '\0';
				    multiple_targets = TRUE;
                                    target_bit = get_bit(dummy_std, next_target,
 target_list);
                                    if (target_bit < 0)  {
                                        fprintf(stderr, "Unrecognized target: %s\n",
                                                    next_target);
                                        fprintf(stderr, "Available targets:");
                                        print_names(target_list);
                                        exit(-1);
                                    } else {
                                        std_or(stds, dummy_std, stds);
					std_zero(dummy_std);
					get_std_num(dummy_std, next_target, target_list);
					std_or(target_option, dummy_std, target_option);
                                        target_set = TRUE;
                                    }
                                    next_target = comma_ptr + 1;
                                }
                                /* now get the last name in the option */
				if (*next_target == NULL) {
				    fprintf(stderr, "Extra comma in -target ignored\n");
				} else {
				    target_bit = get_bit(dummy_std, next_target, target_list);
				    if (target_bit < 0)  {
					fprintf(stderr, "Unrecognized target: %s\n",
                                                next_target);
					fprintf(stderr, "Available targets:");
					print_names(target_list);
					exit(-1);
				    } else {
					std_or(stds, dummy_std, stds);
					std_zero(dummy_std);
					get_std_num(dummy_std, next_target, target_list);
					std_or(target_option, dummy_std, target_option);
					target_set = TRUE;
				    }
				}
                            }
                            break;

			default:
			    break;
		    }

                }
    if (info_only)
        exit(0);

    if (apex_flag && Tflag && !target_set) {
        fprintf(stderr, "-target option is required\n");
        exit(-1);
    }
#endif


	/* malloc arrays */
	if ( (stab = (STAB *) calloc(nsize * sizeof(STAB), 1)) == NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		lerror("out of memory (symbol table)", FATAL | ERRMSG);

	if ( (tary = (ATYPE *) malloc(tsize * sizeof(ATYPE))) == NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		lerror("out of memory (type pool)", FATAL | ERRMSG);
	if ( (fnm = (char **) malloc(fsize * sizeof(char *))) == NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		lerror("out of memory (file names)", FATAL | ERRMSG);
	if ( (atyp = (ATYPE *) malloc(numargs * sizeof(ATYPE)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		lerror("out of memory (arg buffer)", FATAL | ERRMSG);

#ifdef APEX
        if ( (argdimary = (int *) malloc(argdimsz * sizeof(int)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
                lerror("out of memory (dim buffer)", FATAL | ERRMSG);
        if ( (dimary = (int *) malloc(DIMARYSZ * sizeof(int)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
                lerror("out of memory (dim buffer)", FATAL | ERRMSG);
        dfree = dimary;
        if ( (inbuf = (char *) malloc(inbuf_sz * sizeof(char)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
                lerror("out of memory (inbuf)", FATAL | ERRMSG);
        if ( (fmtstrings = (char **) calloc(numargs, sizeof(char **)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
                lerror("out of memory (fmt strings)", FATAL | ERRMSG);

#else
        strbuf[BUFSIZ-1] = '\0';        /* for safety, string terminator */
#endif

	tmpopen( );
	if ( Hflag )
		unbuffer( );
	if ( !Tflag )
	{
		lerror( "", HCLOSE );	/* all done */
		return (0);
	}
	if ( !freopen( ifilename, "r", stdin ) )
		lerror( "cannot open intermediate file", FATAL | ERRMSG );

#ifdef STATS_UDP
	{
	    /* code to send out a UDP packet with information about
	     * the current compiler.  Requires a server that is 
	     * listening for packets.
	     */
	    int s = socket(AF_INET,SOCK_DGRAM,0);
	    struct udp_packet packet;
	    struct sockaddr_in address,myaddress;
	    struct stat statbuf;
	    /* initialize the data */
	    stat(ifilename,&statbuf);
	    packet.size = statbuf.st_size;
	    packet.inode = statbuf.st_ino;
	    (void)strncpy(packet.name,"",14);
	    packet.flag_ansi = 0;
	    packet.flag_a = 0;
	    packet.flag_b = 0;
	    packet.flag_h = 0;
	    packet.flag_p = (pflag != 0);
	    packet.flag_u = (uflag != 0);
	    packet.flag_v = 0;
	    packet.flag_x = (xflag != 0);
	    packet.flag_y = (yflag != 0);
	    packet.flag_N = 0;
	    packet.flag_Z = (nsize!=NSZ || tsize!=TYSZ || fsize!=FSZ);
	    packet.pass = 1;
	    packet.version = INST_VERSION;
	    /* set up the addresses */
	    address.sin_family = AF_INET;
	    address.sin_port = INST_UDP_PORT;
	    address.sin_addr.s_addr = INST_UDP_ADDR;
	    myaddress.sin_family = AF_INET;
	    myaddress.sin_port = 0;
	    myaddress.sin_addr.s_addr = INADDR_ANY;
	    /* try blasting a packet out, no error checking here */
	    bind(s,&myaddress,sizeof(myaddress));
	    sendto(s,&packet,sizeof(packet),0,&address,
		   sizeof(address));
	}
#endif /* STATS_UDP */

#ifdef APEX
	/* Avoid possible catastrophe caused by reading old-style .ln
	 * files.  This magic number is REC_FILE_FMT, byte count = 1,
	 * and a one-byte version number (whose lower order bit must be 1).
	 */
	if ( fread( (void *)&format_magic, 4, 1, stdin) <= 0 ) 
	    return(0); 
	if ( (format_magic & 0xffffff01) != 0x26000101 )
	    lerror("Incorrect .ln file format", FATAL | ERRMSG);
#endif

/* The 2 passes over the .ln file are divided into the following phases:
 *   1) In the first pass, all definitions are entered into the symbol
 *      table.  This ensures that the argument types of the definition
 *      are those that all other declarations and uses are compared against.
 *   2) Declarations (LDX|LDC) next set the appropriate bit in the symbol table
 *      dec field for declarations.  Examination of LDC must occur after all
 *	definitions (LDI...) because the simple declaration "int i;" is a
 *	legal external linkage that should not produce a "multiply declared"
 *	error if that LDC appears before the "int i=42;" LDI from another file.
 *      The final pass over the .ln file examines uses and reports "used but
 *      not defined" via chkuse(), and sets the q->uses field in the symbol
 *      table so that the end-of-processing scan of the symbol table in
 *      lastone() can report errors of defined or declared but not used,
 *      and return consistency.
 *
 */
	if ( idebug ) pif();
	mloop( LDI|LDS|LIB|LDB|LBS );
	rewind( stdin );
	mloop( LDC|LDX|LRV|LUV|LUE|LUM );
	if ( odebug ) pst();
	cleanup();
	un2buffer( );
	return(0);
} /*end main*/

/* symbol table routines */

/* stab_alloc: grab a free symtab entry to add to a hash chain
 *  There is only the stab_freelist pool (so no reuse as in pftn.c)
 */
STAB *
stab_alloc() {
int i;
STAB *q;
	
	if (stab_nextfree >= SYMTBLKSZ) 
	    {
	    if ((stab_blk = (STAB *) calloc(SYMTBLKSZ, sizeof(STAB))) == NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		lerror("out of memory (symtab pool)", FATAL | ERRMSG);
	    stab_nextfree = 0;
	    }
	q = &stab_blk[stab_nextfree++];
	q->decflag = 0;
	q->std_ptr = NULL;
	return q;
}


/*****************************************************************************
 * hash_namestring :
 *      calculate a hash value from the first HASHNCHNAM charaters of
 *      "name"
 *
 */
/* maximum number of characters to hash on in a name */
# define  HASHNCHNAM  8

int hash_namestring(name)
   char * name;
{
   int i,j;
   char *p;

   i = 0;
   for( p=name, j=0; *p != '\0'; ++p ){
        i = (i<<1)+*p;
        if( ++j >= HASHNCHNAM ) break;
        }
   /* this will not have overflowed, since HASHNCHNAM is small relative to
    * the wordsize... */
  return(i%nsize);
}



#define STRMATCH 0

/* lookup - return a pointer to a (possibly new) symtab entry corresponding
 *   to the string in the global buffer strbuf (filled from int. file by getstr)
 */
STAB *
#ifdef APEX
lookup(p)
  char *p;
#else
lookup()
#endif
{
int hashval;
STAB *sptr, *maybe=NULL;

#ifdef APEX
    hashval = hash_namestring(p);
#else
    hashval = hash_namestring(strbuf);
#endif
    sptr = &stab[hashval];
    if (sptr->decflag == 0) /* head of chain is unused */
	/* first entry for this hash value */
	{
#ifdef APEX
            curr_ftn = sptr;
            sptr->name = savestr(p);
#else
	    sptr->name = savestr();
#endif
	    return(sptr);	
	}

    while (sptr)
	{
	    /* if (sptr matches) return(sptr); */
#ifdef APEX
            if (strcmp(p, sptr->name) == STRMATCH)
#else
	    if (strcmp(strbuf, sptr->name) == STRMATCH) 
#endif
		{
#ifdef APEX
                    return(sptr);
#else
		    if (sptr->mno == cmno) return(sptr);
		    /* possible match if neither is static */
		    if ( !(sptr->decflag&LDS || r.l.decflag&LDS) ) maybe=sptr;
#endif
		}

	    if (sptr->next)
	 	    sptr = sptr->next;
	    else
		{
	 	    if (maybe) return(maybe);
		    /* add to end of chain */
		    sptr->next = stab_alloc();
		    sptr = sptr->next;
		    sptr->next = NULL;
		    sptr->decflag = 0;
#ifdef APEX
                    curr_ftn = sptr;
                    sptr->name = savestr(p);
#else
		    sptr->name = savestr();
#endif
		    return(sptr);
		}

	}
	/* dummy return for lint^2 benefit */
	return(sptr);
}


#ifdef APEX
/* add_hint - link in standards database info to the most recently entered
 * symbol table entry
 */
void
add_hint(this_origin, this_stds, detail_min, detail_max, 
	 cmt, rev_min, rev_max)
long this_origin;
int this_stds[4];	/* a bit mask of applicable standards */
int detail_min, detail_max, rev_min, rev_max;
char *cmt;
{
struct db *db_ptr, *dptr, *prev;

	/* Don't bother storing hints that don't match the selected origin
	 * or target.  We can't check detail at this point because
	 * a different alias in the symbol table may eventually be selected
	 * necessitating a different "more detail at level..." message.
	 */
	if (this_origin && (this_origin != origin) )
	    return;
	if (!std_overlap(this_stds, stds) )
	    return;
	if ( (db_ptr=(struct db *)calloc(1, sizeof(struct db))) == NULL) {
#ifdef BBA_COMPILE
#	    pragma BBA_IGNORE
#endif
	    lerror("out of memory (add_hint)", FATAL | ERRMSG);
	}
	db_ptr->origin = this_origin;
	std_cpy(db_ptr->stds, this_stds);
	db_ptr->detail_min = detail_min;
	db_ptr->detail_max = detail_max;
	db_ptr->comment = savestr(cmt);
	if (curr_ftn) {
	    /* sort the hint into position by target order, so that they
	     * can be labelled and grouped together in the output
	     */
	    if (curr_ftn->hint_ptr) {
		dptr = curr_ftn->hint_ptr;
		prev = NULL;
		while ( dptr && std_cmp(dptr->stds, this_stds) <= 0 ) {
		    prev = dptr;
		    dptr = dptr->next;
		}
		/* insert after prev */
		if (prev) {
		    db_ptr->next = dptr;
		    prev->next = db_ptr;
		} else {
		    db_ptr->next = curr_ftn->hint_ptr;
		    curr_ftn->hint_ptr = db_ptr;
		}
	    } else { /* first entry */
		db_ptr->next = NULL;
		curr_ftn->hint_ptr = db_ptr;
	    }
	}
	
}

void
add_std(this_origin, this_stds, detail_min, detail_max, 
	 rev_min, rev_max)
long this_origin;
int this_stds[4];	/* a bit mask of applicable standards */
int detail_min, detail_max, rev_min, rev_max;
{
struct db *db_ptr;

	if ( (db_ptr=(struct db *)calloc(1, sizeof(struct db))) == NULL) {
#ifdef BBA_COMPILE
#	    pragma BBA_IGNORE
#endif
	    lerror("out of memory (add_std)", FATAL | ERRMSG);
	}
	db_ptr->origin = this_origin;
	std_cpy(db_ptr->stds, this_stds);
	db_ptr->detail_min = detail_min;
	db_ptr->detail_max = detail_max;
	db_ptr->comment = NULL;
	if (curr_ftn) {
	    db_ptr->next = curr_ftn->std_ptr;
	    curr_ftn->std_ptr = db_ptr;
	}
	
}



#endif

/* mloop - main loop
 *	each pass of the main loop reads in names from the intermediate 
 *	file that have characteristics which overlap with the
 *	characteristics specified as the parameter
 */
STAB *
mloop( m )
{
  STAB *q;
	while( q=lread(m) ){
		if( q->decflag ) chkcompat(q);
		else setuse(q);

		/* This test was changed from m&LUM so that the LDX and LU*
		 * mloop passes could be combined.  I.e., call chkuse (which
		 * possibly generates a "used but not defined" message only
		 * if there is an actual LUM use, not just a hit in lread()
		 * on the loop that now includes declarations (LDX).
		 */
		if ((r.l.decflag& (LUM|LUE|LUV)) && uflag && !ndflag) chkuse(q);
	}
}
/* lread - read a line from intermediate file into r.l */

#ifdef APEX

STAB *
lread(m)
{
    register n;
    char tag;
    short len;
    char version;
    int count;
    int i,j;
    char *inptr;
    long tempdim[MAXDIMS];
    long origin;
    long stds[4];
    unsigned char detail_min, detail_max;
    char *message;
    short hdr_cnt;	/* temporary for hdck count values (used in summary) */
    char used_flag;	/* REC_NOTUSED, REC_NOTDEFINED flag */

    short num_args;
    int line_num;

	for(;;) {
		if ( fread( (void *)&tag, 1, 1, stdin) <= 0 ) 
		    return(0); 
		if (fread( (void *)&len, 2, 1, stdin) <= 0)
		    return(0); 
		if (len) {
		    if (len > inbuf_sz) {
			inbuf_sz = len;
			if ( (inbuf = (char *) malloc(inbuf_sz * sizeof(char)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
				lerror("out of memory (inbuf)", FATAL | ERRMSG);
		    }
		    if (fread( (void *)inbuf, len, 1, stdin) <= 0)
		    	return(0); 
		}
		inptr = inbuf;
		
		switch (tag) {
		    case REC_FILE_FMT:
			continue;

		    case REC_CFILE :
		    case REC_F77FILE :
		    case REC_CPLUSFILE:
		    case REC_PASFILE:
			/* new filename and module number */
			memmove(&version, inbuf, 1);
			inptr += 1;
			r.f.fn = inptr;
			inptr += unshroud(inptr);
			r.f.fn = savestr(r.f.fn);
			setfno( r.f.fn );
			if (version != 0)
			    fprintf(stderr, "warning: possible .ln file version mismatch in %s\n", r.f.fn);
			continue;

		    case REC_LDI:
			r.l.decflag = LDI;
			break;
		    case REC_LDI_NEW:
			r.l.decflag = LDI|LPR;
			break;
		    case REC_LFM:
			r.l.decflag = LDI|LFM;
			break;
		    case REC_LIB:
			r.l.decflag = LIB;
			break;
		    case REC_LIB_NEW:
			r.l.decflag = LIB|LPR;
			break;
		    case REC_LDC:
			r.l.decflag = LDC;
			break;
		    case REC_LDX:
			r.l.decflag = LDX;
			break;
		    case REC_LDX_NEW:
			r.l.decflag = LDX|LPR;
			break;
		    case REC_LRV:
			r.l.decflag = LRV;
			break;
		    case REC_LUV:
			r.l.decflag = LUV;
			break;
		    case REC_LUE:
			r.l.decflag = LUE;
			break;
		    case REC_LUE_LUV:
			r.l.decflag = LUE|LUV;
			break;
		    case REC_LUM:
			r.l.decflag = LUM;
			break;
		    case REC_LDS:
			r.l.decflag = LDS;
			break;
		    case REC_LDS_NEW:
			r.l.decflag = LDS|LPR;
			break;
		    case REC_STD_LPR:
			r.l.decflag = LDX|LBS|LPR;
			break;
		    case REC_STD_LDX:
			r.l.decflag = LDX|LBS;
			break;
		    case REC_COM:
#if 0
			/* sample code if common block checking is added */
			memmove(&r.l.fline, inptr, 4);
			inptr += 4;
			memmove(&savedflag, inptr, 1);
			inptr += 1;
			memmove(&count, inptr, 4);
			inptr += 4;
			r.l.name = inptr;
			inptr += unshroud(inptr);
			for (i=0; i<count; i++) {
			    inptr += readtype(&r.l.type, r.l.dims, inptr);
			    if (r.l.type.numdim)
				r.l.type.dimptr = r.l.dims;
			    else
				r.l.type.dimptr = (int *)NULL;
			}
#endif
			continue;

		    /* unimplemented */
		    case REC_F77SET:
		    case REC_F77PASS:
		    case REC_F77USE:
		    case REC_F77BBLOCK:
		    case REC_F77GOTO:
			continue;

		    case REC_PROCEND:
			curr_ftn = NULL;
			continue;
		    case REC_STD:
		    case REC_HINT:
			if (apex_flag) {
			    /* Test apex_flag here so that apex in lint 
			     * mode is efficient enough that apex and lint
			     * libraries can be shared.
			     */
			    char *hint_text_ptr;

			    memcpy((char *)&origin, inptr, 4);
			    inptr += 4;
			    memcpy((char *)stds, inptr, 16);
			    inptr += 16;
			    memcpy((char *)&detail_min, inptr, 1);
			    inptr += 1;
			    memcpy((char *)&detail_max, inptr, 1);
			    inptr += 1;

			    hint_text_ptr = inptr;
			    inptr += unshroud(inptr);
			    if( (LDB & m) && curr_ftn ) 
				if (tag == REC_STD)
				    add_std(origin, stds, 
					detail_min, detail_max, 0, 0);
				else
				    add_hint(origin, stds, 
					detail_min, detail_max, hint_text_ptr, 0, 0);

			    if (inptr - inbuf < len + 3) {
				/* reserved for 2 strings for release #'s */
			    }
			}
			continue;

		    case REC_HDR_ERR:
			if (LDI & m) {
			    memmove(&hdr_cnt, inptr, 2);
			    num_hdr_stds += hdr_cnt;
			}
			continue;

		    case REC_HDR_HINT:
			if (LDI & m) {
			    memmove(&hdr_cnt, inptr, 2);
			    num_hdr_hints += hdr_cnt;
			}
			continue;

		    case REC_HDR_DETAIL:
			if (LDI & m) {
			    memmove(&hdr_cnt, inptr, 2);
			    if (hdr_cnt > further_hdr_detail)
				    further_hdr_detail = hdr_cnt;
			}
			continue;

		    case REC_NOTUSED:
			memmove(&used_flag, inptr, 1);
			nuflag = used_flag;
			continue;

		    case REC_NOTDEFINED:
			memmove(&used_flag, inptr, 1);
			ndflag = used_flag;
			continue;

		    case REC_UNKNOWN:
			continue;

		    default:
			continue;
		}
		
		
		/* common form: [line#] 
				[name] 
				[type] 
				[# args]
				[# altrets]
					[type] for each arg
		*/
		memmove(&r.l.fline, inptr, 4);
		inptr += 4;
		r.l.name = inptr;
		inptr += unshroud(inptr);
		inptr += readtype(&r.l.type, r.l.dims, inptr);
		if (r.l.type.numdim)
		    r.l.type.dimptr = r.l.dims;
		else
		    r.l.type.dimptr = (int *)NULL;
		if ( ISFTN(r.l.type.aty) ) {
		    memmove(&r.l.nargs, inptr, 2);
		    inptr += 2;
		    memmove(&r.l.altret, inptr, 2);
		    inptr += 2;
		} else
		    r.l.nargs = 0;
		/* number of arguments is negative for VARARGS (plus one) */
		n = r.l.nargs;
		if( n<0 ) n = -n - 1;
		/* collect type info for all args */
		if( n ) {
		    int i;
		    if (n >= numargs) {
			/* malloc a new atyp */
			numargs = n;
			free(atyp);
			if ( (atyp = (ATYPE *) malloc(numargs * sizeof(ATYPE)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
			    lerror("out of memory (arg buffer)", FATAL | ERRMSG);
		    }
		    argdfree = 0;
		    for (i=0;i<n;i++){
			inptr += readtype(&atyp[i], tempdim, inptr);
			if (argdfree+atyp[i].numdim >= argdimsz) {
			    argdimsz += ARGDIMSZ;
			    if ((argdimary = (int *)realloc(argdimary, argdimsz*sizeof(int))) == NULL)
				lerror("out of memory (dim buffer)", FATAL | ERRMSG);
			}
			if (atyp[i].numdim) {
			    atyp[i].dimptr = (int *)argdfree;
			    for (j=0; j<atyp[i].numdim; j++)
				argdimary[argdfree++] = tempdim[j];
			} else
			    atyp[i].dimptr = (int *)NULL;
			/* read format string */
			inptr += unshroud( inptr );
		    }

		    /* Now that we know that argdimary has been expanded 
		     * if necessary, go through the atyp array again
		     * and convert atyp[].dimptr from indexes into argdimary
		     * into true pointers, so that chktype() can consistently
		     * treat dimptr as a true pointer.
		     */
		    for (i=0;i<n;i++){
			atyp[i].dimptr = &argdimary[(int)atyp[i].dimptr];
		    }
		}
	/* return with entry only if it has correct characteristics */
	if( ( r.l.decflag & m ) ) return( lookup(r.l.name) );
	}
}

int readtype(t, dims, in)
ATYPE *t;
char *in;
int dims[];
{
char *start;
char numdims;
int i;

	start = in;
	memmove(&t->aty, in, 4);
	in += 4;
	memmove(&t->extra, in, 4);
	in += 4;
	if (BTYPE(t->aty) == STRTY || BTYPE(t->aty) == UNIONTY) {
	    memmove(&t->stcheck, in, 4);
	    in += 4;
	    t->stname = in;
	    in += unshroud(in);
	}
	memmove(&numdims, in, 1);
	t->numdim = numdims;
	in += 1;
	for (i=0; i<numdims; i++)  {
	    memmove(&dims[i], in, 4);
	    in += 4;
	}
	t->typename = in;
	in += unshroud(in);
	if (*(t->typename) == '\0')
	    t->typename = NULL;
	else
	    t->typename = lookup_typename(t->typename);
	return (in-start);
}



char *lookup_typename()
{
	return NULL;	/* for now */
}

#else	/* non-APEX */

STAB *
lread(m)
{
	register n;

	for(;;) {
		/* read in line from intermediate file; exit if at end 
		 * This code used to fread the entire r structure, but was
		 * changed to an element at a time to produce a s300/s800
		 * portable structure.
		 *
		 * if( fread( (void *)&r, sizeof(r), 1, stdin ) <= 0 ) return(0); */
		if (( fread( (void *)&r.l.decflag,sizeof(r.l.decflag),
			    1,stdin) <=0 ) ||
		    ( fread( (void *)&r.l.name,sizeof(r.l.name),
			    1,stdin) <=0 ) ||
		    ( fread( (void *)&r.l.nargs,sizeof(r.l.nargs),
			    1,stdin) <=0 ) ||
		    ( fread( (void *)&r.l.fline,sizeof(r.l.fline),
			    1,stdin) <=0 ) ||
		    ( fread( (void *)&r.l.type.aty,sizeof(r.l.type.aty),
			    1,stdin) <=0 ) ||
		    ( fread( (void *)&r.l.type.extra,sizeof(r.l.type.extra),
			    1,stdin) <=0 )) return(0); 
		
		if( r.l.decflag & LFN ){
			/* new filename and module number */
			getstr();
			r.f.fn = savestr();
			setfno( r.f.fn );
			/* the purpose of the module number is to handle static scoping
			 *	correctly.  A module is a file with all its include files.
			 *	From a scoping point of view, there is no difference between
			 *	a variable in a file and a variable in an included file.
			 *	The module number itself is not meaningful; it must only
			 *	be unique to ensure that modules are distinguishable
			 */
			cmno = r.f.mno;
			continue;
		} else if ( r.l.decflag & (LNU|LND)) {
			if (r.l.decflag & LNU){
				if (r.l.nargs)
					nuflag = 1;
				else
					nuflag = 0;
			} else {
				if (r.l.nargs)
					ndflag = 1;
				else
					ndflag = 0;
			}
			continue;
		}
		/* number of arguments is negative for VARARGS (plus one) */
		getstr();
		n = r.l.nargs;
		if( n<0 ) n = -n - 1;
		/* collect type info for all args */
		if( n )
		    if ( n <= numargs){
		        int i;
			for (i=0;i<n;i++){
			  (void)fread( (void *)&atyp[i].aty, sizeof(atyp[i].aty), (size_t)1, stdin );
			  (void)fread( (void *)&atyp[i].extra, sizeof(atyp[i].extra), (size_t)1, stdin );
			}
		        }
		    else
			{
			int i;
			numargs = n;
			if ( (atyp=(ATYPE *)realloc((void *)atyp, n*sizeof(ATYPE))) 
									== NULL)
				lerror("out of memory (arg realloc)", FATAL |
						ERRMSG);
			for (i=0;i<n;i++){
			  (void)fread( (void *)&atyp[i].aty, sizeof(atyp[i].aty), (size_t)1, stdin );
			  (void)fread( (void *)&atyp[i].extra, sizeof(atyp[i].extra), (size_t)1, stdin );
			}
			}
		/* return with entry only if it has correct characteristics */
		if( ( r.l.decflag & m ) ) return( lookup() );
	}
}

#endif


/* setfno - set file number */
setfno( s ) char *s;
{
  int i;
        /* look up filename */
        for( i=0; i<ffree; ++i )
                if( fnm[i] == s ){
                        cfno = i;
                        return;
                }
        /* make a new entry */
        if ( ( cfno = ffree++ ) >= fsize )
                {
                    fsize += FNMINC;
                    if ((fnm=(char **)realloc((void *)fnm,fsize*sizeof(char *)))== NULL)
#ifdef BBA_COMPILE
#			pragma BBA_IGNORE
#endif
                        lerror( "out of memory (file names)", FATAL | ERRMSG );
                }
        fnm[cfno] = s;
}


/* chkcompat - check for compatibility
 *	compare item in symbol table (stab) with item just read in
 *	The work of this routine is not well defined; there are many
 *	checks that might be added or changes
 */

chkcompat(q) STAB *q; 
{
	register int i;
	unsigned qq;
	STAB *q1;

	/* check for multiple declaration - do this checking before setuse() 
	 * so that if an alias is entered, the previous file/line info is
	 * retained.
	 */

	if( (q->decflag & (LDI|LDS|LIB|LBS) ) && (r.l.decflag&(LDI|LDS|LIB|LBS)) )
		/* "%s multiply declared" */
#ifdef APEX
		if (q->std_ptr) {
		    /* allow aliases if all previous definitions include
		     * a STD description.
		     */
		    q1 = stab_alloc();
		    *q1 = *q;
		    q->decflag = r.l.decflag;
		    q->alias = q1;
		} else
#endif
		buffer( 3, q );
	setuse(q);

	/* do an argument check only if item is a function; if it is
	*  both a function and a variable, it will get caught later on
	*/

	if( ISFTN(r.l.type.aty) && ISFTN(q->symty.aty) )

		/* argument check */

		if( q->decflag & (LDI|LDS|LIB|LBS|LUV|LUE) )
			if( r.l.decflag & (LUV|LIB|LBS|LUE) ||
			    ((r.l.decflag==(LDX|LPR)) && ansimode) ){
				if( q->nargs != r.l.nargs ){
					if( !(q->use&VARARGS) )
						/* "%.8s: variable # of args." */
						/* "%s: variable # of args." */
						buffer( 7, q );
					if( r.l.nargs > q->nargs ) r.l.nargs = q->nargs;
					if( !(q->decflag & (LDI|LDS|LIB|LBS) ) ) {
						q->nargs = r.l.nargs;
						q->use |= VARARGS;
					}
				}
				for( i=0,qq=q->argidx; i<r.l.nargs; ++i,++qq)
					{
					ATYPE deftype; 
					TWORD ty; 
					deftype = tary[qq];
					ty = tary[qq].aty;

				    /* char/shorts used to be promoted to INT
				     * here (except on calls to new-style fncs)
				     * but that promotion should already have
				     * been done by lint1.
				     */
					if( chktype( &deftype, &atyp[i] ) )
						/* "%.8s, arg. %d used inconsistently" */
						/* "%s, arg. %d used inconsistently" */
						buffer( 6, q, i+1 );
					}

			}

	if( (q->decflag&(LDI|LDS|LIB|LBS|LDC|LUV)) && (r.l.decflag & ~LPR) == LUV )
		if( chktype( &r.l.type, &q->symty ) )
			/* "%.8s value used inconsistently" */
			/* "%s value used inconsistently" */
			buffer( 4, q );

	/* Check function declarations only (not uses like LUM or LUV).
	 * Qualified by "ansimode" so that llib-lc.ln can be built ANSI mode
	 */
	if( (q->decflag&LPR)^(r.l.decflag&LPR) && 
		(r.l.decflag&LDI || r.l.decflag&(LIB|LBS) || r.l.decflag&LDX ||
		 r.l.decflag&LDS) && 
		ansimode){
		/* mixed old-style/new-style function declaration */
#ifdef OSF
	        /* OSF lint libraries are not shipped with newstyle
                   definitions.  Only complain if new-style is in
                   library */
	        if (r.l.decflag&LIB && !(r.l.decflag&LPR))
                   /* nothing */;
                else if (q->decflag&LIB && !(q->decflag&LPR))
		   /* nothing */;
                else buffer(12,q);
#else
		buffer(12,q);
#endif
        }


	/* do a bit of checking of definitions and uses... */

	if( (q->decflag & (LDI|LDS|LIB|LBS|LDX|LDC|LUM))
	    && (r.l.decflag & (LDX|LDC|LUM)) )
	    /* && q->symty.t.aty != r.l.type.aty ) */
		if( chklong( &r.l.type, &q->symty ) || 
			chkattr( r.l.type, q->symty) )
		/* "%.8s value type declared inconsistently" */
		/* "%s value type declared inconsistently" */
		buffer( 5, q );

	/* better not call functions which are declared to be
		structure or union returning */

	if( (q->decflag & (LDI|LDS|LIB|LBS|LDX|LDC))
	    && (r.l.decflag & LUE)
	    && q->symty.aty != r.l.type.aty ){
		/* only matters if the function returns union or structure */
		TWORD ty;
		ty = q->symty.aty;
		if( ISFTN(ty) && ((ty = DECREF(ty))==STRTY || ty==UNIONTY ) )
			/* "%.8s function value type must be declared before use" */
			/* "%s function value type must be declared before use" */
			buffer( 8, q );
	}

#ifdef APEX
	if( (q->decflag & (LDI|LDS|LIB|LBS))
	    && (r.l.decflag & (LUE|LUM|LUV) ) ) {
		struct db *dbp;
		STAB *sym;
		int any_std = FALSE;
		int possible_std = FALSE;
		int any_hint = FALSE;
		int hint_match = FALSE;
		int this_std;
		int match_detail, any_detail;

		/* buffer a standards/portability message if, for all aliases:
		 *   1. no matching standard is found but at least one 
		 *	possibility was present, or
		 *   2. a standard match is found and some of its hints match
		 *
		 * "Possibility present" means that a STD or HINT is present,
		 * and the origin matches if specified.  The idea is to avoid
		 * issuing standards violations for non-applicable or users'
		 * "used but not defined" routines.
		 * If no standard matches, print the standards violation
		 * message and all hints that match.  If at least one standard
		 * matches, print no standards violation and restrict the hints
		 * to the aliases that match the standards.
		 */

		match_detail = further_detail;
		any_detail = further_detail;
		sym = q;
		while (sym) {
		    dbp = sym->std_ptr;
		    this_std = FALSE;
		    while (dbp) {
			if( (!dbp->origin || (origin & dbp->origin)) ) {
			    possible_std = TRUE;
			    if( std_overlap(stds, dbp->stds) ) {
				this_std = TRUE;
				any_std = TRUE;
			    }
			}
			dbp = dbp->next;
		    }
		    dbp = sym->hint_ptr;
		    while (dbp) {
			if( (!dbp->origin || (origin & dbp->origin)) &&
			    std_overlap(stds, dbp->stds) &&
			     detail <= dbp->detail_max && show_hints )
				if (detail >= dbp->detail_min) 
				    if (this_std)
					hint_match = TRUE;
				    else
					any_hint = TRUE;
				else
				    if (this_std)
					match_detail = 
					    MAX(match_detail, dbp->detail_min);
				    else
					any_detail =
					    MAX(any_detail, dbp->detail_min);
			    dbp = dbp->next;
		    }
		    sym = sym->alias;
		}

		if (any_std && match_detail > further_detail)
		    /* a hint message was suppressed on a matching-standard
		     * entry due to -detail not being set high enough.
		     */
		    further_detail = match_detail;

		if (!any_std && any_detail > further_detail)
		    /* similarly for a hint message that would have been
		     * printed because no standards entries match.
		     */
		    further_detail = any_detail;

		if( (!any_std && any_hint) || (hint_match && show_hints) 
				|| (possible_std && !any_std && show_stds) )
			buffer(13, q, !any_std);

	}
#endif

} /*end chkcompat*/

/* messages for defintion/use */
int	mess[2] = {
	1,
	2
};

lastone(q) STAB *q; 
{
	register nu, nd, uses, dec;
	nu = nd = 0;
	uses = q->use;
	dec = q->decflag & ~LPR & ~LBS;

	if( !(uses&USED) && !(dec&LIB) ) {
		if( strcmp(q->name,"main") ) nu = 1;
#ifdef APEX
		if( dec&LFM )  nu = 1;	/* implicit use of Fortran program */
#endif
	}

	/* treat functions and variables the same */
        if (dec & (LIB|LBS))
            nu = nd = 0;  /* don't complain about uses on libraries */
        else
	switch( dec ){

	case LDS:
		nu = 0;	/* lint1 has already complained about statics
			 * defined but never used
			 */
		break;
	case LDX:
		if( !xflag ) break;
	case LUV:
	case LUE:
/* 01/04/80 */	case LUV | LUE:
/* or'ed together for void */
	case LUM:
		nd = 1;
	}


	/* "used but not defined" message moved to setuse() so that lineno
	 * of use rather than declaration can be reported
	 */
#ifdef APEX
	/* ... but record the number for apex's summary section */
	/* This expression differs from the nd variable in that declarations
	 * don't count as offending uses without a definition
	 */
	if ( uses&USED && !(dec&(LDI|LIB|LDS|LDC)) )
		num_undef++;
#endif

	if( uflag && ( nu ) ) buffer( mess[nd], q );

	if( (uses&(RVAL+EUSED)) == (RVAL+EUSED) ){
		if ( uses & VUSED ) 
			/* "%.8s returns value which is sometimes ignored\n" */
			/* "%s returns value which is sometimes ignored\n" */
			buffer( 11, q );
		else 
			/* "%.8s returns value which is always ignored\n" */
			/* "%s returns value which is always ignored\n" */
			buffer( 10, q );
	}

	if( (uses&(RVAL+VUSED)) == (VUSED) && (q->decflag&(LDI|LDS|LIB)) )
		/* "%.8s value is used, but none returned\n" */
		/* "%s value is used, but none returned\n" */
		buffer( 9, q );
}


void
chkuse(q)
STAB *q;
{
int tmp_line, tmp_fno;

	if (q->decflag & (LDX|LUM|LUE|LUV) && !(q->decflag & LBS))
	    {
		/* report the line of use, not declaration */
		tmp_line = q->fline;
		tmp_fno = q->fno;
		q->fline = r.l.fline;
		q->fno = cfno;
		/* used but not defined */
		buffer(0, q);
		q->fline = tmp_line;
		q->fno = tmp_fno;
	    }
}



/* cleanup - call lastone and die gracefully */
cleanup()
{
	STAB *p,*q;
	for( q=stab; q < &stab[nsize]; ++q )
		for (p=q; p; p=p->next)
			if( p->decflag ) 
				lastone(p);
}
/* setuse - check new type to ensure that it is used */
setuse(q) STAB *q;
{
	if( !q->decflag ){ /* new one */
		q->decflag = r.l.decflag;
		q->symty = r.l.type;
#ifdef APEX
                /* copy the base type array dimension info to permanent store */
                q->symty.dimptr = install_dims(r.l.dims, r.l.type.numdim);
                q->symty.numdim = r.l.type.numdim;
#endif
		if( r.l.nargs < 0 ){
			q->nargs = -r.l.nargs - 1;
			q->use = VARARGS;
		}
		else {
			q->nargs = r.l.nargs;
			q->use = 0;
		}
		q->fline = r.l.fline;
		q->fno = cfno;
		q->mno = cmno;
		if( q->nargs ){
			int i;
			unsigned qq;
			q->argidx = tfree;
			/* stretch tary if necessary */
			tfree += q->nargs;
			if( tfree >= tsize ) {
			    tsize += MAX((tfree-tsize),TYINC);
			    if ( (tary=(ATYPE *)realloc((void *)tary, 
					    tsize*sizeof(ATYPE))) == NULL)
#ifdef BBA_COMPILE
#				pragma BBA_IGNORE
#endif
				lerror( "out of memory (type table)", 
						FATAL | ERRMSG );
			}
			for( i=0, qq=q->argidx; i<q->nargs; ++i, ++qq ){
				tary[qq] = atyp[i];
#ifdef APEX
                                tary[qq].dimptr = install_dims( atyp[i].dimptr,
                                        atyp[i].numdim);
#endif
			}
		}

	}

	switch( r.l.decflag & ~LPR ){

	case LRV:
		q->use |= RVAL;
		return;
	case LUV:
		q->use |= VUSED+USED;
		return;
	case LUE:
		q->use |= EUSED+USED;
		return;
	/* 01/04/80 */	case LUV | LUE:
	case LUM:
		q->use |= USED;
		return;

	}
	if (nuflag) q->use |= USED;
	/* 04/06/80 */
	if ( (q->decflag & LDX) && (r.l.decflag & LDC) ) {
	    /* "int i;" overrides a previous "extern int i;" */
	    q->decflag = LDC;
	    q->fline = r.l.fline;
	    q->fno = cfno;
	}
}
/* chktype - check the two type words to see if they are compatible */

chktype( pt1, pt2 ) register ATYPE *pt1, *pt2; 
{
	TWORD t1,t2;
        TWORD aty1, aty2;
        ATYPE *atype_tmp;
        int mixed_lang;

	/* Don't perform type checking if either type is undefined (as
	 * emitted by Domain Pascal frontend, for example).
	 */
	if( BTYPE(pt1->aty)==UNDEF || BTYPE(pt2->aty)==UNDEF )
	    return 0;

        if( ((pt1->extra & ATTR_F77) && !(pt2->extra & ATTR_F77)) ||
            ((pt2->extra & ATTR_F77) && !(pt1->extra & ATTR_F77)) )
            mixed_lang = TRUE;
        else
            mixed_lang = FALSE;

        aty1 = pt1->aty;
        aty2 = pt2->aty;

        /* When checking cross-language types, we need to map the Fortran
         * integral types to the corresponding size C types, and make the
         * C type signed (there is no Fortran equivalent to unsigned).
         */
        if( mixed_lang ) {
            /* for ease in conversion, make pt1 the C type, pt2 the Fortran */
            if( pt1->extra & ATTR_F77 ) {
                atype_tmp = pt1;
                pt1 = pt2;
                pt2 = atype_tmp;
            }
	    if (ISUNSIGNED(BTYPE(pt1->aty)))
		aty1 = (pt1->aty & ~BTMASK) | DEUNSIGN(BTYPE(pt1->aty));
	    else
		aty1 = pt1->aty;
            switch( BTYPE(pt2->aty) ) {
                /* case FTYCOMPLEX: case FTYDCOMPLEX: should already have been
                        converted by lintfor1 to STRUCT with appropriate bit
                        set in extra  */
		case LONG:  if ( (pt1->aty & BTMASK) == INT)
				aty2 = (pt2->aty & ~BTMASK) | INT;    	break;
                case FTYLOG2:   aty2 = (pt2->aty & ~BTMASK) | SHORT;    break;
                case FTYLOG4:   aty2 = (pt2->aty & ~BTMASK) | INT;      break;
                case FTYCHAR:   aty2 = (pt2->aty & ~BTMASK) | CHAR;     break;
                case FTYLBYTE:  aty2 = (pt2->aty & ~BTMASK) | CHAR;     break;
                case FTYIBYTE:  aty2 = (pt2->aty & ~BTMASK) | CHAR;     break;
            }
        }

	/* currently, enums are turned into ints and should be checked as such */
	if( aty1 == ENUMTY ) aty1 =  INT;
	if( aty2 == ENUMTY ) aty2 = INT;

	t1=BTYPE(aty1);
	t2=BTYPE(aty2);
	if( (t1==STRTY) || t1==UNIONTY || t2==STRTY || t2==UNIONTY)
#ifdef APEX
                if( mixed_lang )
                        return( aty1 != aty2 );
                else
                        return( aty1 != aty2 && pt1->stcheck != pt2->stcheck );
#else
		return( aty1!=aty2 || pt1->extra!=pt2->extra );
#endif

	if( pt2->extra & ATTR_ICON ){ /* constant passed in */
		if( sizeof(int) == sizeof(long) && !pflag && 
			ISINTEGRAL(aty1) )
				return( 0 );
		if( aty1 == UNSIGNED && aty2 == INT ) return( 0 );
		else if( aty1 == ULONG && aty2 == LONG ) return( 0 );
	}

	else if( pt1->extra  & ATTR_ICON){ /* for symmetry */
		if( sizeof(int) == sizeof(long) && !pflag &&
			ISINTEGRAL(aty2) ) 
				return( 0 );
		if( aty2 == UNSIGNED && aty1 == INT ) return( 0 );
		else if( aty2 == ULONG && aty1 == LONG ) return( 0 );
	}

#ifdef APEX
        if (check_dims) {
            int i;
            if (pt1->numdim != pt2->numdim)
                return( 1 );
            for (i=0; i<pt1->numdim; i++)
                if ( *(pt1->dimptr + i) != *(pt2->dimptr + i) )
                    return( 1 );
        }
#endif
	if (yflag && ((pt1->aty &(~BTMASK)) == (pt2->aty &(~BTMASK)))){
	    /* like chklong, but avoid mutually recursive calls */
	    if ( BTYPE(pt1->aty) == LONG && BTYPE(pt2->aty) == INT ) return (0);
	    if ( BTYPE(pt2->aty) == LONG && BTYPE(pt1->aty) == INT ) return (0);
	}

	return( aty1 != aty2 );
}

/* chklong - check if basic type is long/int and rest of type is same */
chklong( pt1, pt2 ) register ATYPE *pt1, *pt2; 
{
/* check if basic type is long/int and rest of type is same */
	if (yflag && ((pt1->aty &(~BTMASK)) == (pt2->aty &(~BTMASK)))){
		if ( BTYPE(pt1->aty) == LONG && BTYPE(pt2->aty) == INT ) return (0);
		if ( BTYPE(pt2->aty) == LONG && BTYPE(pt1->aty) == INT ) return (0);
	}

	/* Call chktype() for the remainder of type checking, because it 
	 * handles mixed language calls.
	 */
	return( chktype(pt1, pt2) );
}


/* Check if the const/volatile attributes match */
int 
chkattr( t1, t2) register ATYPE t1, t2;
{
	/* structs use the extra field for a struct id, not attributes 
	 * if t1 is a struct, chkcompat will have checked t2
	 */
	if ( BTYPE(t1.aty) == STRTY ) return(0);
	/* Mask off the non-type-encoding bits */
#define ATTR_ONLY (~(ATTR_ICON|ATTR_COMPLEX|ATTR_DCOMPLEX|ATTR_F77))
	return ( (t1.extra & ATTR_ONLY) != (t2.extra & ATTR_ONLY) );
}

# ifdef DEBUG
/* diagnostic output tools
 *	the following programs are used to print out internal information
 *	for diagnostic purposes
 *	Current options:
 *	-Xi
 *		turns on printing of intermediate file on entry to lint2
 *			(input information)
 *	-Xo
 *		turns on printing of lint symbol table on last pass of lint2
 *			(output information)
 */

struct tb { 
	int m; 
	char * nm; };
static struct tb dfs[] = {
	LDI, "LDI",
	LIB, "LIB",
	LDC, "LDC",
	LDX, "LDX",
	LRV, "LRV",
	LUV, "LUV",
	LUE, "LUE",
	LUM, "LUM",
	LDS, "LDS",
	LPR, "LPR",
	LBS, "LBS",
	LNU, "LNU",
	LND, "LND",
	LDB, "LDB",
	LFE, "LFE",
	0, "" };

static struct tb us[] = {
	USED, "USED",
	VUSED, "VUSED",
	EUSED, "EUSED",
	RVAL, "RVAL",
	VARARGS, "VARARGS",
	0,	0,
};


/* ptb - print a value from the table */
ptb( v, tp ) struct tb *tp; 
{
	int ptbflag = 0;
	for( ; tp->m; ++tp )
		if( v&tp->m ){
			if( ptbflag++ ) putchar( '|' );
			printf( "%s", tp->nm );
		}
}


#ifdef APEX
/* ptag - print a tag value from the table */
/* differs from ptb in treating v as a value, not a bit field */
ptag( v, tp ) struct tb *tp; 
{
	for( ; tp->m; ++tp )
		if( v == tp->m ){
			printf( "%s", tp->nm );
			return;
		}
}

static struct tb rec_tb[] = {
	REC_CFILE,"REC_CFILE",
	REC_F77FILE,"REC_F77FILE",
	REC_CPLUSFILE,"REC_CPLUSFILE",
	REC_LDI,"REC_LDI",
	REC_LDI_NEW,"REC_LDI_NEW",
	REC_LDI_MAIN,"REC_LDI_MAIN",
	REC_LDI_NEW_MAIN,"REC_LDI_NEW_MAIN",
	REC_LIB,"REC_LIB",
	REC_LIB_NEW,"REC_LIB_NEW",
	REC_LDC,"REC_LDC",
	REC_LDX,"REC_LDX",
	REC_LDX_NEW,"REC_LDX_NEW",
	REC_LRV,"REC_LRV",
	REC_LUV,"REC_LUV",
	REC_LUE,"REC_LUE",
	REC_LUM,"REC_LUM",
	REC_LDS,"REC_LDS",
	REC_LDS_NEW,"REC_LDS_NEW",
	REC_COM,"REC_COM",
	REC_F77SET,"REC_F77SET",
	REC_F77PASS,"REC_F77PASS",
	REC_F77BBLOCK,"REC_F77BBLOCK",
	REC_F77GOTO,"REC_F77GOTO",
	REC_PROCEND,"REC_PROCEND",
	REC_STD,"REC_STD",
	REC_HINT,"REC_HINT",
	REC_HDR_ERR,"REC_HDR_ERR",
	REC_PRINTF,"REC_PRINTF",
	REC_SCANF,"REC_SCANF",
	REC_LUE_LUV,"REC_LUE_LUV",
	REC_NOTUSED,"REC_NOTUSED",
	REC_NOTDEFINED,"REC_NOTDEFINED",
	REC_LFM, "REC_LRM",
	REC_STD_LDX, "REC_STD_LDX",
	REC_STD_LPR, "REC_STD_LPR",
	REC_FILE_FMT, "REC_FILE_FMT",
	REC_PASFILE, "REC_PASFILE",
	REC_UNKNOWN,"REC_UNKNOWN",
	0,	0,
};
/* pif - print intermediate file
 *	prints file written out by first pass of lint
 *  printing turned on by the debug option -Xi
 */
pif()
{
    register n;
    char tag;
    short len;
    char version;
    int count;
    int i;
    char *inptr;
    char *name;
    char *format;
    long org;
    long stds[NUM_STD_WORDS];
    unsigned char min, max;
    long tempdim[MAXDIMS];
    char used_flag;
    char savedflag;

    short hdr_cnt;
    short num_args;
    int line_num;

	printf("\n\tintermediate file printout:\n");
	printf("\t===========================\n");
	for(;;) {
		if ( fread( (void *)&tag, 1, 1, stdin) <= 0 ) 
		    return(0); 
		if (fread( (void *)&len, 2, 1, stdin) <= 0)
		    return(0); 
		if (len) {
                    if (len > inbuf_sz) {
                        inbuf_sz = len;
                        if ( (inbuf = (char *) malloc(inbuf_sz * sizeof(char)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
                                lerror("out of memory (inbuf)", FATAL | ERRMSG);
                    }
                    if (fread( (void *)inbuf, len, 1, stdin) <= 0)
                        return(0);
                }

		inptr = inbuf;
		ptag(tag, rec_tb);
		switch (tag) {
		    case REC_FILE_FMT:
			memmove(&version, inptr, 1);
			printf( "version %d\n", version );
			continue;

		    case REC_CFILE :
		    case REC_F77FILE :
		    case REC_CPLUSFILE:
		    case REC_PASFILE:
			/* new filename and module number */
			memmove(&version, inptr, 1);
			inptr++;
			(void)unshroud(inptr);
			printf( " %-15s, version %d\n", inptr, version );
			continue;

		    /* unimplemented */
		    case REC_F77SET:
		    case REC_F77PASS:
		    case REC_F77USE:
		    case REC_F77BBLOCK:
		    case REC_F77GOTO:
			/* fall through */

		    /* simple tag */
		    case REC_PROCEND:
			printf("\n");
			continue;

		    case REC_STD:
		    case REC_HINT:
			memmove(&org, inptr, 4);
			inptr += 4;
			memmove(stds, inptr, 16);
			inptr += 16;
			memmove(&min, inptr++, 1);
			memmove(&max, inptr++, 1);
			(void)unshroud(inptr);
			printf(" 0x%x -> [0x%x 0x%x 0x%x 0x%x][%d-%d] %s\n", 
				org, stds[0], stds[1], stds[2], stds[3], 
				min, max, inptr);
			continue;

		    case REC_HDR_ERR:
		    case REC_HDR_HINT:
		    case REC_HDR_DETAIL:
			memmove(&hdr_cnt, inptr, 2);
			printf(" count = %d\n", hdr_cnt);
			continue;

		    case REC_COM:
			memmove(&r.l.fline, inptr, 4);
			inptr += 4;
			memmove(&savedflag, inptr, 1);
			inptr += 1;
			memmove(&count, inptr, 4);
			inptr += 4;
			name = inptr;
			inptr += unshroud(inptr);
			printf(" line %d, saved: %d, common /%s/: %d elements\n", 
				r.l.fline, savedflag, name, count);
			for (i=0; i<count; i++) {
			    inptr += readtype(&r.l.type, tempdim, inptr);
			    r.l.type.dimptr = tempdim;
			    printf("\t%d:", i);
			    aprint(r.l.type);
			    printf("\n");
			}
			continue;

		    case REC_NOTUSED:
		    case REC_NOTDEFINED:
			memmove(&used_flag, inptr, 1);
			printf(" flag = %d\n", used_flag);
			continue;

		    default:
		        printf("\n");
			continue;	/* illegal tag */

		    case REC_LFM:
		    case REC_LDI:
		    case REC_LDI_NEW:
		    case REC_LIB:
		    case REC_LIB_NEW:
		    case REC_LDC:
		    case REC_LDX:
		    case REC_LDX_NEW:
		    case REC_LRV:
		    case REC_LUV:
		    case REC_LUE:
		    case REC_LUM:
		    case REC_LDS:
		    case REC_LDS_NEW:
		    case REC_LUE_LUV:
		    case REC_STD_LPR:
		    case REC_STD_LDX:
			/* fall through switch */ ;

		}
		
		
		memmove(&r.l.fline, inptr, 4);
		inptr += 4;
		r.l.name = inptr;
		inptr += unshroud(inptr);
		inptr += readtype(&r.l.type, tempdim, inptr);
		printf(" line %d,\t%s\t", r.l.fline, r.l.name);
		r.l.type.dimptr = tempdim;
	        aprint(r.l.type);
		if ( ISFTN(r.l.type.aty) ) {
		    memmove(&r.l.nargs, inptr, 2);
		    inptr += 2;
		    memmove(&r.l.altret, inptr, 2);
		    inptr += 2;
		    printf("  %d args, %d rets", r.l.nargs, r.l.altret);
		} else
		    r.l.nargs = 0;
		printf("\n");
		/* number of arguments is negative for VARARGS (plus one) */
		n = r.l.nargs;
		if( n<0 ) n = -n - 1;
		/* collect type info for all args */
		if( n ) {
		    int i;
		    for (i=0;i<n;i++){
			inptr += readtype(&atyp[0], tempdim, inptr);
			printf("\targ %d\t", i);
			atyp[0].dimptr = tempdim;
			aprint(atyp[0]);
			format = inptr;
			inptr += unshroud( inptr );
			if (format && *format)
				printf(" format = %s", format);
			printf("\n");
		    }
		}
	}

} /*end pif*/

#else /* non-APEX */

/* pif - print intermediate file
 *	prints file written out by first pass of lint
 *  printing turned on by the debug option -Xi
 */
pif()
{
TWORD ta=0;

	printf("\n\tintermediate file printout:\n");
	printf("\t===========================\n");
	while( (0 < fread( (char *)&r.l.decflag, sizeof(r.l.decflag), 1, stdin)) &&
	       (0 < fread( (char *)&r.l.name, sizeof(r.l.name), 1, stdin)) &&
	       (0 < fread( (char *)&r.l.nargs, sizeof(r.l.nargs), 1, stdin)) &&
	       (0 < fread( (char *)&r.l.fline, sizeof(r.l.fline), 1, stdin)) &&
	       (0 < fread( (char *)&r.l.type.aty, sizeof(r.l.type.aty), 1, stdin)) &&
	       (0 < fread( (char *)&r.l.type.extra, sizeof(r.l.type.extra), 1, stdin)) ) {
		if ( r.l.decflag & LFN )
		{
			getstr();
			printf( "\nFILE NAME: %-15s\n", strbuf );
		} else if ( r.l.decflag & (LNU|LND) ) {
			if (r.l.decflag & LNU)
				if (r.l.nargs)
					printf( "\nCOMMENT: /*NOTUSED*/\n" );
				else
					printf( "\nCOMMENT: /*USED*/\n" );
			else
				if (r.l.nargs)
					printf( "\nCOMMENT: /*NOTDEFINED*/\n" );
				else
					printf( "\nCOMMENT: /*DEFINED*/\n" );
		}
		else {
			getstr();
			printf( "\t%-8s  (", strbuf );
			ptb(r.l.decflag, dfs);
			printf(")\t line= %d", r.l.fline);
			if ( ISFTN(r.l.type.aty) ) printf("\tnargs= %d", r.l.nargs);
			else printf("\t\t\t");
			printf("\t type= ");
			tprint(r.l.type.aty, ta);
			printf("\n");
			if ( r.l.nargs ) {
				int n = 0;
				if ( r.l.nargs < 0 ) r.l.nargs= -r.l.nargs - 1;
				while ( ++n <= r.l.nargs ) {
					(void)fread( (char *)&atyp->aty, sizeof(atyp->aty), 1, stdin);
					(void)fread( (char *)&atyp->extra, sizeof(atyp->extra), 1, stdin);
					printf("\t\t arg(%d)= ",n);
					tprint(atyp[0].aty, ta); 
  					/* added -Suhaib */
  					printf("  Extra:%d", atyp[0].extra);
					printf("\n");
				}
			}
		}
	} /*while*/

	rewind( stdin );
} /*end pif*/

#endif	/* APEX */


/* pst - print lint symbol table
 *	prints symbol table created from intermediate file
 *  printing turned on by the debug option -Xo
 */
pst()
{
  TWORD ta = 0;
#ifdef APEX
  struct db *dp;
#endif
  int count = 0;
  STAB *p, *q;
	printf("\n\tsymbol table printout:\n");
	printf("\t======================\n");
	for( p=stab; p < &stab[nsize]; ++p) {
	    for (q=p; q; q = q->next)
		if( q->decflag ) {
			count++;
			printf( "\t%8s  (", q->name );
			ptb(q->decflag, dfs);
			printf(")\t line= %d", q->fline);
			if ( ISFTN(q->symty.aty) ) printf("\tnargs= %d", q->nargs);
			else printf("\t\t\t");
			printf("\t type= ");
			tprint(q->symty.aty, ta);
			printf("\t use= ");
			ptb(q->use, us);
			printf("\n");
#ifdef APEX
                        if (apex_flag) {
                            dp = q->std_ptr;
                            while (dp) {
                                    printf("\tstds=");
                                    std_print(dp->stds, target_list);
                                    dp = dp->next;
                            }
                            dp = q->hint_ptr;
                            while (dp) {
                                    printf("\thints=");
                                    std_print(dp->stds, target_list);
                                    printf("\t (%s)\n", dp->comment);
                                    dp = dp->next;
                            }
                        }
#endif
			if ( q->nargs ) {
				int n = 1;
				unsigned qq;
				for( qq=q->argidx; n <= q->nargs; ++qq) {
					printf("\t\t arg(%d)= ",n++);
#ifndef APEX
					tprint(tary[qq].aty, tary[qq].extra); 
#endif
					printf("\n");
				}
			}
		} /*end if(q->decflag)*/
	} /*end for p*/
	printf("\n%d symbol table entries\n",count);
}




	static char * tnames[] = {
		"undef",
		"farg",
		"char",
		"short",
		"int",
		"long",
		"float",
		"double",
		"strty",
		"unionty",
		"enumty",
		"moety",
		"uchar",
		"ushort",
		"unsigned",
		"ulong",
		"void",
		"long double",
		"signed char",
		"labty",
		"tnull",
#ifdef APEX
                "complex",
                "double complex",
                "short logical",
                "logical",
                "char",
                "byte",
                "byte",
#endif
		"?", "?"
		};

/* output a nice description of the type of t */
tprint( t, ta )  register TWORD t, ta; {
	for(;; t = DECREF(t) ){
                if (ISPVOL(ta)) printf("volatile ");
		if (ISPCON(ta)) printf("const ");
		ta = DECREF(ta);
		if( ISPTR(t) ) printf( "PTR " );
		else if( ISFTN(t) ) printf( "FTN " );
		else if( ISARY(t) ) printf( "ARY " );
		else if(t <= TNULL) {
                        if (ISVOL(ta)) printf("volatile ");
			if (ISCON(ta)) printf("const ");
			if (SCLASS(ta)==ATTR_REG) printf("reg_class ");
			printf( "%s", tnames[t] );
			return;
			}
		else {
			printf("unknown type in tprint %d",t);
			return;
		     }
		}
	}

#ifdef APEX
aprint( t )  ATYPE t; {

int i;
        tprint(t.aty, t.extra);
        if ( (BTYPE(t.aty)==STRTY || BTYPE(t.aty==UNIONTY)) && t.stname) {
                printf(" %s ", t.stname);
                printf(" (checksum %d) ", t.stcheck);
        }
        if (t.numdim)
                for (i=0; i<(t.numdim); i++)
                        printf("[%d]", *(t.dimptr+i));
        if (t.typename)
                printf(" %s ", t.typename);
        if (t.extra & ATTR_COMPLEX)
                printf(" complex ");
        else if (t.extra & ATTR_DCOMPLEX)
                printf(" double complex ");
}
#endif

# else
pif()
{ (void)prntf("intermediate file dump not available\n"); }
pst()
{ (void)prntf("symbol table dump not available\n"); }
# endif


#include <ctype.h>

#ifndef APEX
char *
getstr()   /* read arb. len. name string from input & place in global buffer */
{
	register char *cp = strbuf;
	register int c;

	while ( ( c = getchar() ) != EOF && cp < &strbuf[BUFSIZ-2] )
	{
		*cp++ = c;
		if ( c == '\0' )
			break;
	}
}
#endif

#define NSAVETAB	4096
char	*savetab;
int	saveleft=0;

/* copy string into permanent string storage */
char *
#ifdef APEX
savestr(cp)	
    char *cp;
#else
savestr( )		
#endif
{
#ifndef APEX
	register char *cp = strbuf;
#endif
	register size_t len;

	len = strlen( cp ) + 1;
	if ( len > saveleft )
	{
		saveleft = NSAVETAB;
		if ( len > saveleft )
			saveleft = len;
		savetab = (char *) malloc( (unsigned) saveleft );
		if ( savetab == 0 )
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
			lerror( "Ran out of memory [savestr()]",
				FATAL | ERRMSG );
	}
	(void) strncpy( savetab, cp, len );
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return ( cp );
}


#ifdef APEX

/* decrypt the string p in place.
 *   Input:  [len][crypted string]
 *   Output: [decrypted, null-terminated string]
 *   Return: #bytes in input
 */
size_t unshroud(p)
char *p;
{
short len;
char *in, *out;
int i;

	memmove(&len, p, 2);
	out = p;
	in = p+2;
	for (i=0; i<len; i++)  {
#ifdef CLEAR_TEXT
	    *out++ = *in++;		
#else
	    *out++ = (*in++ + 96) % 256;
#endif
	}
	*out = '\0';

	return len + 2;		/* string + length field */
}

/* Copy a block of array dimension information from temporary storage
 * (either the r.l.dims temporary array for the base type or the argdimary for
 *  argument types) into the permanent dimary.  
 */

int *install_dims(ary, num)
int ary[];
int num;
{
register int i;
int *start;

	if (num==0)
	    return NULL;
	if ( dfree - dimary + num  > DIMARYSZ ) {
	    if ( (dimary = (int *) malloc(DIMARYSZ * sizeof(int)))==NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		lerror("out of memory (dim buffer)", FATAL | ERRMSG);
	    dfree = dimary;
	}
	start = dfree;
	for (i=0; i<num; i++) {
	    *dfree++ = ary[i];
	}
	return start;
}

#endif  /* APEX */


# ifdef __lint
#   ifdef __hp9000s300
/*VARARGS1*/
/*ARGSUSED*/
int prntf(s) char *s; {return 0;}
/*VARARGS2*/
/*ARGSUSED*/
int fprntf(f,s) FILE *f; char *s; {return 0;}
#   endif
#endif
