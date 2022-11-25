/* @(#) $Revision: 70.6 $ */     
/* lerror2.c
 *	This file contains routines for message and error handling for
 *	the second lint pass (lint2).
 *
 *	Functions:
 *	==========
 *		buffer		buffer a message
 *		catchsig	set up signals
 *		lerror		lint error message
 *		onintr		clean up after an interrupt
 *		tmpopen		open intermediate and temporary files
 *		un2buffer	dump second pass messages
 *		unbuffer	dump header messages from first pass
 */

# include	<stdio.h>
# include	<signal.h>
# include	<ctype.h>
# include 	<string.h>
# include 	<malloc.h>
# include	<sys/types.h>
# include	"messages.h"
# include	"lerror.h"
# include	"manifest" 
# include	"lmanifest"
# include	"lpass2.h"
#ifdef APEX
# include       "apex.h"
extern long stds[NUM_STD_WORDS];
extern long target_option[NUM_STD_WORDS];
extern long origin;
int num_stds = 0;
int num_hints = 0;
int num_undef = 0;
extern int symtab_name_sort();
extern int multiple_targets;
#endif

extern void exit();

#ifdef BBA_COMPILE
#pragma BBA_IGNORE
#endif
void
nomem(s) char *s;
{
char ebuf[BUFSIZ];

	(void)strcpy(ebuf, "out of memory  ");
	(void)strcat(ebuf, s);
	lerror(ebuf, HCLOSE | ERRMSG | FATAL );
}


/* tmpopen
 *	open source message buffer file for writing
 *  open header message file for reading
 *    initialize header file name and count list from header message file
 *
 *  changed to in-memory array of array of C2RECORDS - 9/6/89
 */

char		*htmpname = NULL;
static FILE	*htmpfile = NULL;

C2RECORD * msg2head[NUM2MSGS]; /* ptr to array of entries for each category */
int	msg2totals[ NUM2MSGS ];		/* # entries currently in each array */
int	msg2max[ NUM2MSGS ];		/* current max size of each array */


tmpopen( )
{
int i;

    catchsig( );
    for (i=0; i<NUM2MSGS; i++)
	{
	    if ((msg2head[i]=(C2RECORD *)malloc(INIT2MSG*C2RECSZ)) == NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		nomem("(message buffer)");
	    msg2max[i] = INIT2MSG;
	    msg2totals[i] = 0;
	}

    if ( htmpname == NULL )
	return;
    if ( (htmpfile = fopen( htmpname, "r" )) == NULL )
	lerror( "cannot open header message buffer file", FATAL | ERRMSG );
}
/* lerror - lint error message routine
 *  if code is HCLOSE error close and unlink header file
 *  if code is FATAL exit
 */

lerror( message, code ) char *message; int code;
{
    if ( code & ERRMSG )
		(void)fprntf( stderr, "lint pass2 error: %s\n", message );

    if ( code & HCLOSE )
		if ( htmpfile != NULL ) {
			(void)fclose( htmpfile );
			(void)unlink( htmpname );
		}
    if ( code & FATAL ) exit( FATAL );
}

/* gethstr - reads in a null terminated string from htmpfile and
*		returns a pointer to it.
*/

char *
gethstr()
{
	static char buf[BUFSIZ];
	register char *cp = buf;
	register int ch;

	while ( ( ch = getc( htmpfile ) ) != EOF && cp < &buf[BUFSIZ-2] )
	{
		*cp++ = ch;
		if ( ch == '\0' )
			break;
	}
	if (ch != '\0')
		{
		buf[BUFSIZ-1] = '\0';
		while ( (ch=getc(htmpfile) != EOF) && (ch != '\0') )
			/* scan and discard excessively long string */ ;
		}
	return ( buf );
}


#define NUMHDRS	100
#define HDRPOOLSZ 4096
#define HDRNMINC 200
#define HPOOLINC BUFSIZ*4  /* bigger than BUFSIZ to guarantee buf will fit */
char **hdrnames;	/* array of pointers into hdrpool */
char *hdrpool;		/* names strings of header file names */
int hfree = 0;		/* next available slot in hdrnames */
char * hnmptr;		/* next available slot in hdrpool */
size_t hdrnmsize;	/* current size of hdrnames */
size_t hdrstrsize;	/* current size of hdrpool */

/* If 'name' is a previously-encountered header file name, return 1
 * to indicate the following errors are duplicates and should be skipped,
 * else add the new header file to the list, and return 0,
 */
int
newheader(newhname) char * newhname;
{
int i;
char *newpool;
char *newptr;

	for (i=0; i<hfree; i++)
		if (strcmp(hdrnames[i], newhname) == 0) return 1;
	/* First encounter with this name - add it to this list */
	if (hfree == hdrnmsize)
		{
#ifdef BBA_COMPILE
/* Can't hit because "too many errors" from lint1 will occur first */
#		pragma BBA_IGNORE
#endif
		hdrnmsize += HDRNMINC;
		if ( (hdrnames=(char **)realloc((void *)hdrnames, hdrnmsize*sizeof(char *))) == NULL)
			nomem("(growing header names)");
		}
	if (hnmptr-hdrpool+strlen(newhname)+1 >= hdrstrsize)
		{
		hdrstrsize += HPOOLINC;
		if ( (newpool=(char *)malloc(hdrstrsize*sizeof(char))) == NULL)
#ifdef BBA_COMPILE
/* Can't hit because "too many errors" from lint1 will occur first */
#			pragma BBA_IGNORE
#endif
			nomem("(growing header pool)");
		newptr = newpool;
		for (i=0; i<hfree; i++)
			{
			(void)strcpy(newptr, hdrnames[i]);
			hdrnames[i] = newptr;
			newptr += (strlen(hdrnames[i]) + 1);
			}
		free((void *)hdrpool);
		hdrpool = newpool;
		hnmptr = newptr;
		}
	/* Finally, both arrays are big enough - add the header name */
	hdrnames[hfree++] = hnmptr;
	(void)strcpy(hnmptr, newhname);
	hnmptr += (strlen(newhname)+1);
	return(0);	/* skipping flag */

}

/* unbuffer - writes out information saved in htmpfile */

#define SMALLBUF 32

unbuffer( )
{
    int		skipping=0;
    HRECORD	record;
    char tmpname[BUFSIZ];
    char tmp3name[SMALLBUF];
    char *nameptr;

    	/* create initial malloc'ed areas */
	hdrnmsize = NUMHDRS;
	if ( (hdrnames = (char **)malloc(hdrnmsize*sizeof(char *) )) == NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		nomem("(header file names)");

	hdrstrsize = HDRPOOLSZ*sizeof(char);
	if ( (hdrpool = (char *)malloc( hdrstrsize )) == NULL)
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		nomem("(header name pool)");
	hnmptr = hdrpool;

	while ( fread( (void *) &record, HRECSZ, 1, htmpfile ) == 1 ) 
	    {
		if (record.msgndx == HDRFILE)
		    {
			(void)strcpy(tmpname,  gethstr() );
			skipping = newheader(tmpname);
			if (!skipping)
			    (void)prntf( "\n%s  (as included in %s)\n==============\n",
				tmpname, gethstr() );
			else
				(void)gethstr();
		    }
		else if (!skipping)
		    {

			(void)prntf( "(%d)  ", record.lineno );
			if ( record.code & WERRTY ) 
				(void)prntf( "warning: " );

			switch( record.code & ~( WERRTY | SIMPL ) ) {

			case TRIPLESTR:
				(void)strncpy(tmp3name, gethstr(), SMALLBUF);
				tmp3name[SMALLBUF-1] = '\0';
				(void)strcpy(tmpname, gethstr());
				nameptr = gethstr();
				(void)prntf( msgtext[ record.msgndx ],
					tmp3name, tmpname, nameptr );
				break;

			case DBLSTRTY:
				(void)strcpy(tmpname, gethstr());
				nameptr = gethstr();
				(void)prntf( msgtext[ record.msgndx ],
					tmpname, nameptr );
				break;

			case STRINGTY:
				nameptr = gethstr();
				(void)prntf( msgtext[ record.msgndx ], nameptr );
				break;

			case CHARTY:
				(void)prntf( msgtext[ record.msgndx ], record.arg1.char1 );
				break;

			case NUMTY:
				(void)prntf( msgtext[ record.msgndx ], record.arg1.number );
				break;

			case CHARTY|STR2TY:
				nameptr = gethstr();
				(void)prntf( msgtext[ record.msgndx ], record.arg1.char1, nameptr );
				break;

			case NUMTY|STR2TY:
				nameptr = gethstr();
				(void)prntf( msgtext[ record.msgndx ], record.arg1.number, nameptr );
				break;

			case CHARTY|HEX2TY:
				(void)prntf( msgtext[ record.msgndx ], record.arg1.char1, record.arg2.num2 );
				break;

			case CHARTY|CHAR2TY:
				(void)prntf( msgtext[ record.msgndx ], record.arg1.char1, record.arg2.char2 );
				break;

			case NUMTY|CHAR2TY:
				(void)prntf( msgtext[ record.msgndx ], record.arg1.number, record.arg2.char2 );
				break;

			default:
				(void)prntf( msgtext[ record.msgndx ] );
				break;

			}
		(void)prntf( "\n" );
		}
    }
    free((void *)hdrpool);
    free((void *)hdrnames);
    (void)fclose( htmpfile );
    (void)unlink( htmpname );
}
/*  onintr - clean up after an interrupt
 *  ignores signals (interrupts) during its work
 */
void
onintr( )
{
    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGHUP, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) signal(SIGPIPE, SIG_IGN);
    (void) signal(SIGTERM, SIG_IGN);

	(void)putc( '\n', stderr);
    lerror( "interrupt", HCLOSE | FATAL );
    /* note that no message is printed */
}

/*  catchsig - set up signal handling */

catchsig( )
{
    if ((signal(SIGINT, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGINT, onintr);

    if ((signal(SIGHUP, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGHUP, onintr);

    if ((signal(SIGQUIT, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGQUIT, onintr);

    if ((signal(SIGPIPE, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGPIPE, onintr);

    if ((signal(SIGTERM, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGTERM, onintr);
}


/* VARARGS2 */
buffer( msgndx, symptr, digit ) int	msgndx; STAB *symptr; int digit;
{

    extern int		cfno;
    extern union rec	r;

    C2RECORD		record;

    if ( ( msgndx < 0 ) || ( msgndx > NUM2MSGS ) )
#ifdef BBA_COMPILE
#pragma BBA_IGNORE
#endif
		lerror( "message buffering scheme flakey", FATAL | ERRMSG );

    if ( !(msg2info[ msgndx ].msg2class & warnmask) )
	return;

    if ( msg2totals[ msgndx ] >= msg2max[msgndx] ) {
	msg2max[msgndx] += ADDL2MSG;
        msg2head[msgndx] = (C2RECORD *)realloc((void *)msg2head[msgndx], 
						C2RECSZ*msg2max[msgndx]);
	}
    record.name = symptr->name;

    switch( msg2info[msgndx].msg2type ) {

	    case ND2FNLN:
		record.number = digit;
		/* no break */

	    case NM2FNLN:
		record.file2 = cfno;
		record.line2 = r.l.fline;
		/* no break */

	    case NMFNLN:
		record.file1 = symptr->fno;
		record.line1 = symptr->fline;
		break;

#ifdef APEX
            case NMFNSTD:
                record.file1 = cfno;
                record.line1 = r.l.fline;
                record.file2 = (int)symptr; /* kludge */
                record.line2 = digit;   /* 1 == hint only */
                break;

#endif

	    default:
		break;
		}

    msg2head[msgndx][ msg2totals[msgndx]++ ] = record;
}
/* un2buffer - dump the second pass messages */

extern char		**fnm;

un2buffer( )
{

    int		i, j, stop;
    int		toggle;
    int		codeflag;
    C2RECORD	record;

    codeflag = FALSE;

		/* note: ( msgndx == NUM2MSGS ) --> dummy message */
    for ( i = 0; i < NUM2MSGS ; ++i ) {


		if ( msg2totals[ i ] != 0 ) {
			if ( codeflag == FALSE ) {
				(void)prntf( "\n\n==============\n" );
				codeflag = TRUE;
			}
			toggle = 0;

			stop = msg2totals[ i ];

#ifdef APEX
                        if (msg2info[i].msg2type == NMFNSTD)
                                sort_stds(msg2head[i], stop, i);
                        else
#endif
			{
			(void)prntf( "%s\n", msg2info[i].msg2text );
			for ( j = 0; j < stop; ++j ) {
				record = msg2head[i][j];
				switch( msg2info[i].msg2type ) {

				case NM2FNLN:
					(void)prntf( "    %s   \t%s(%d) :: %s(%d)\n",
					  record.name, fnm[ record.file1 ], record.line1,
					  fnm[ record.file2 ], record.line2 );
					break;

				case NMFNLN:
					(void)prntf( "    %s   \t%s(%d)\n",
					  record.name,
					  fnm[ record.file1 ], record.line1 );
					break;

				case NMONLY:
					(void)prntf( "    %s", record.name );
					if ( ++toggle == 3 ) {
						(void)prntf( "\n" );
						toggle = 0;
					}
					else (void)prntf( "\t" );
					break;

				case ND2FNLN:
					(void)prntf( "    %s( arg %d )   \t%s(%d) :: %s(%d)\n",
					  record.name, record.number, fnm[ record.file1 ],
					  record.line1, fnm[ record.file2 ], record.line2 );
					break;

				default:
					break;
				}
			}
			if ( toggle != 0 ) (void)prntf( "\n" );
			}
	    	} /* if */
	    } /* end for */

#ifdef APEX
        if ( apex_flag && show_sum) {
            if ( codeflag == 0 ) {
                (void)prntf( "\n\n==============\n" );
            }
            /* separate from previous report category */
            if ( codeflag > 0 ) {
                (void)prntf( "\n" );
            }
            print_sum();
        }
#endif

}




#ifdef APEX

print_sum()
{
char *name;
int i;

	printf("standards/portability summary\n");
	printf("%6d  non-standard include file", num_hdr_stds);
	if (num_hdr_stds != 1)
	    printf("s");
	printf(" found\n");
	printf("%6d  include file portability hint", num_hdr_hints);
	if (num_hdr_hints != 1)
	    printf("s");
	printf(" reported\n", num_hdr_hints);
	if (further_hdr_detail)
	    printf("\t\t(additional hints available at -detail %d)\n", 
			further_hdr_detail);
	printf("%6d  standards violation", num_stds);
	if (num_stds != 1)
	    printf("s");
	printf(" reported\n");
	printf("%6d  portability hint", num_hints);
	if (num_hints != 1)
	    printf("s");
	printf(" printed\n");
	if (further_detail)
	    printf("\t\t(additional hints available at -detail %d)\n", 
			further_detail);
	printf("%6d  name", num_undef);
	if (num_undef != 1)
	    printf("s");
	printf(" used but not defined\n", num_undef);
}

int symtab_name_sort(sym1, sym2)
    C2RECORD *sym1, *sym2;
{
    return strcmp(sym1->name, sym2->name);
}

int symtab_file_sort(sym1, sym2)
    C2RECORD *sym1, *sym2;
{
    return sym1->file1 - sym2->file1;
}

int symtab_line_sort(sym1, sym2)
    C2RECORD *sym1, *sym2;
{
    return sym1->line1 - sym2->line1;
}


sort_stds(recs, num, indx)
    C2RECORD	recs[];
    int num, indx;
{
int i;
int start, end, file_start, file_end;
char *curr_name;
STAB *sym;
long origin_array[NUM_STD_WORDS];

    printf("%s", msg2info[indx].msg2text);
    printf("for origin ");
    std_zero(origin_array);
    origin_array[0] = origin;
    std_print(origin_array, origin_list);
    printf("; target ");
    std_print(target_option, target_list);
    prntf("\n");

    /* sort the records by symbol name */
    qsort(recs, num, sizeof(C2RECORD), symtab_name_sort);

    curr_name = NULL;
    start = 0;
    while (start < num) {
	sym = (STAB *)recs[start].file2;
	if (strcmp(sym->name, curr_name)) {
	    curr_name = sym->name;
	    print_hints(sym, recs[start].line2);
	}

	end = start+1;
	while (end < num && 
	    !strcmp(sym->name, ((STAB *)recs[end].file2)->name) ) {
	    end++;
	}

	if (show_calls) {
	    /* start to end-1 is a block of records for the same name */
	    /* sort by file number */
	    qsort(&recs[start], end-start, sizeof(C2RECORD), symtab_file_sort);

	    /* next sort by line number within each filename */
	    file_end = start;
	    while (file_end < end) {
		file_start = file_end;
		file_end = file_start + 1;
		while (file_end < end && 
				recs[file_end].file1 == recs[file_start].file1)
			file_end++;
		qsort(&recs[file_start], file_end-file_start, sizeof(C2RECORD),
							    symtab_line_sort);
	    }

	    for (i=start; i<end; i++)
		prntf("\t\t%s(%d)\n", fnm[recs[i].file1], recs[i].line1);
	}

	printf("\n");
	start = end;
    }


}


print_hints(sym, flg)
STAB *sym;
int flg;	/* print all matching hints */
{
struct db *hint_ptr, *prev_hint;
struct db *std_ptr, *tmp;
STAB *p;
long see_stds[NUM_STD_WORDS], any_bit[NUM_STD_WORDS];
int any_std, this_std, std_present;
int any_hint = FALSE;

    printf("    %s\n", sym->name);

    /* pre-pass to accumulate applicable stds if no entries match the target
     */
    std_zero(see_stds);
    any_std = FALSE;
    std_present = FALSE;
    p = sym;
    while (p) {
	this_std = FALSE;
	std_ptr = p->std_ptr;
	while (std_ptr) {
	    if (std_overlap(std_ptr->stds, stds) )
		this_std = TRUE;
	    else
		std_or(std_ptr->stds, see_stds, see_stds);

	    std_ptr = std_ptr->next;

	    std_present = TRUE;
	}
	if (this_std && show_hints) {
	    any_hint = print_hint_chain(p);
	    any_std = TRUE;
	}
		
	p = p->alias;
    }

    if ( !any_std ) {
    /* got through all aliases and found no standards match -
     * print the standards violation, and go through all aliases again,
     * printing any matching hints
     */

	if (std_present) {
	    num_stds++;
	    if (show_stds) {
		set_bit(any_bit, ALL_STD_BITS);		/* set all bits */
		if (std_overlap(see_stds, any_bit) ) {
		    printf("\tnot found in ");
		    std_print(stds, target_list);
		    printf("; see ");
		    std_print(see_stds, target_list);
		} else {
		    printf("\tnot found in ");
		    std_print(stds, target_list);
		}
		printf("\n");
	    }
	}
	if (show_hints) {
	    p = sym;
	    while (p) {
		any_hint = print_hint_chain(p);
		p = p->alias;
	    }
	}
    }

    if (any_hint)
	num_hints++;
}


/* print all hints attached to a symbol table entry that meet the detail
 * level requirements.  If there are several pertinent targets, label each
 * category with the target name that triggered the match.
 */
print_hint_chain(p)
STAB *p;
{
struct db *hint_ptr;
long section[NUM_STD_WORDS];
int need_labels, any_hint, first_label;

	any_hint = FALSE;
	first_label = TRUE;

	/* scan the list to see if there are multiple target sections */
	/* If multiple standards are given in the -target option, always
	 * label the hints so the user can tell to which standard the
	 * hint pertains.  In addition, if there are multiple permutations
	 * involving the one standard given, label each of the combinations.
	 */
	need_labels = multiple_targets;
	hint_ptr = p->hint_ptr;
	if (!hint_ptr)
	    return FALSE;	/* no hints printed */

	while (hint_ptr) {
	    if (hint_ptr->detail_min <= detail && 
		    hint_ptr->detail_max >= detail) {
		if (first_label) {
		    std_cpy(section, hint_ptr->stds);
		    first_label = FALSE;
		} else {
		    if (!std_equal(section, hint_ptr->stds) ) {
			need_labels = TRUE;
			break;
		    }
		}
	    }
	    hint_ptr = hint_ptr->next;
	}

	hint_ptr = p->hint_ptr;
	std_zero(section);

	while (hint_ptr) {
	if( (detail >= hint_ptr->detail_min && 
		detail <= hint_ptr->detail_max)) {
		    if (need_labels && !std_equal(section, hint_ptr->stds) ) {
			std_cpy(section, hint_ptr->stds);
			printf("      ");
			std_print(section, target_list);
			printf("\n");
		    }
		    indent_text(hint_ptr->comment);
		    any_hint = TRUE;
	    }
	    hint_ptr = hint_ptr->next;
	}

	return any_hint;
}


/* print a hint, properly indented.  Insert a tab following each newline
 * so that the hint is indented from the name heading, but don't add a
 * tab if one exists in the hint text itself.
 */
indent_text(p)
char *p;
{
	putchar('\t');
	while (*p) {
	    if (*p == '\n' && *(p+1)!='\t' && *(p+1)!='\0') {
		/* print all consecutive blank lines */
		while (*p == '\n') {
		    putchar(*p);
		    p++;
		}
		/* insert a tab if we're not done, and one isn't supplied */
		if (*p != '\0' && *p != '\t')
		    putchar('\t');
	    }
	    putchar(*p);
	    p++;
	}
	putchar('\n');
}

#endif	/* APEX */
