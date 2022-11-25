/* @(#) $Revision: 70.20 $ */
/* LINTLIBRARY */

/*
** SYNOPSIS:
**	localedef [-c] [-f charmap] [-i inputfile] locale_name
**      localedef -d [-o format] locale_name
**      localedef -n [inputfile]
**      inputfile is the script file for the locale
**	locale_name is the target locale, and if it contains a "/", then
**	  it's interpreted as the path for installing locale.inf under.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <nl_ctype.h> 
#include <limits.h>

#include "global.h"
#include "define.h"

extern int cflag;
extern char fname[];
extern char fch[];		/* if no "-o", default to "-ox" */
extern char *langname;

/* main:
** command line parsing.  set up flags.
** if no "-d" option, call localedef() function.
** if "-d" option, call dumplang() function.
*/
main(argc, argv)
int argc;
char **argv;
{
	extern char *strcpy();
	extern char *optarg;
	extern int optind;
	extern int copy_flag;
	
	FILE *out;
	int c, fflag = 0;
	int dump = 0;
	int in_file =0;
	char *infile_name, *charmap_file;
	char locale_name[_POSIX_PATH_MAX];

	if (!setlocale(LC_ALL,""))
	{
		fputs(_errlocale("localedef"), stderr);
		catd = (nl_catd)-1;
	}
	else
		catd = catopen("localedef",0);

	if (!setlocale(LC_COLLATE,"C"))
	    fprintf(stderr,(catgets(catd,NL_SETN,1,"cannot set LC_COLLATE\n")));
	if (!setlocale(LC_CTYPE,"C"))
	    fprintf(stderr,(catgets(catd,NL_SETN,2,"cannot set LC_CTYPE\n")));

	/*
	** read command line options. Added new options c and i for localedef
	*/
	while ((c = getopt(argc, argv, "ndco:i:f:")) != EOF) {
		switch (c) {
			case 'n':
				no_install++;
				break;
			case 'd':
				dump++;
				break;
                        case 'c':
				cflag++;
				warn_flag++;
				break;
                        case 'i':
                                in_file++;
				/*
				  optarg is set to point to the start
				  of the option argument on return from getopt.
				 */
				infile_name = optarg;
				break;
				
			case 'f':
				if(optarg == NULL)
				{
				  error(NO_CHARMAP);
				  exit(5);
				}
				else
				{
				  charmap_file = optarg;
				  fflag =1;
				}
				break;
        	       case 'o':
				fch[0] = *optarg;
				break;
			default:
				usage();
				break;
		}
	}
	if ( (optind == (argc-1)) && (strchr(argv[optind],'/')!=NULL) )
	     (void) strcpy(locale_name,argv[optind]);
	else
	     locale_name[0] = '\0';

/*
 * Locales other than those supplied by the implementation can be created via
 * the localedef utility provided POSIX2_LOCALEDEF symbol is defined in limits.h
 */
	if (!POSIX2_LOCALEDEF)
	{
		exit(3); /* Capabibility to create new locales not supported */
	}

	if (fflag) {
		if(!in_file) usage();
		tmpnam(fname);
		if((out = fopen(fname,"w")) == NULL) {
			fprintf(stderr,(catgets(catd,NL_SETN,80, "cannot open temporary file %s\n")), fname);
			exit(10);
		}
		interpret(charmap_file, infile_name, out);
		fclose(out);
		infile_name = fname;
	}
	if (no_install && dump)		/* -n and -d are mutually exclusive */
		usage();

	/*
	** invoke localedef() or dumplang()
	*/
	if (dump) {			/* with "-d" option, dumplang() */
		if(optind >= argc) usage();
		dumplang(argv[optind], fch);
	}
	else				/* without "-d" option, localedef() */
	{
	   if(optind < argc) langname = argv[optind];		/* langname */
	   if(in_file) /* process file, else process stdin */
		if(freopen(infile_name, "r", stdin) == NULL) {
			fprintf(stderr,(catgets(catd,NL_SETN,9, "cant open input file %s\n")), infile_name);
			exit(10);
		}
	   localedef(stdin,locale_name);
	}
	if(fflag && in_file) unlink(stdin);
	return(0);
}

/* usage:
** print usage message and exit.
*/
usage()
{
	extern int fprintf();
	extern void exit();
	
	fprintf(stderr, (catgets(catd,NL_SETN,4, "Usage:\tlocaledef [-c] [-f charmap_file] [-i input_file] locale_name\n")));
	fprintf(stderr, (catgets(catd,NL_SETN,5, "or\tlocaledef -d [-o format] locale_name\n")));
	fprintf(stderr, (catgets(catd,NL_SETN,6, "or\tlocaledef -n input_file\n")));
	exit(6);
}

/* localedef:
** build and install a "locale.inf" file
*/
#define	CMDLEN	64
#define VISITED 1 /* used to quit category when it sees END LC_* */

localedef(fp,locale_name)
FILE *fp;
char *locale_name;
{
  	int pos_count=0;
	extern FILE *yyin;
	int ctype_count=0, coll_count =0, mon_count=0, num_count=0;
	int mod_count=0;
	int time_count=0, mesg_count=0, all_count=0;
	extern void *calloc();

	extern void name_init();
	extern void id_init();
	extern void rev_init();
	extern void mod_init();
	extern void all_init();
	/* extern void ctrang_init(); */
	extern void cty_init();
	/* extern void shrang_init(); */
	extern void con_init();
#ifdef EUC
	extern void cscheme_init();
	extern void cswidth_init();
#endif /* EUC */
	extern void order_init();
	extern void col_finish();
	extern void info_init();
	extern void era_init();
	extern void mntry_init();
	extern void numeric_init();

	extern void fill_lc_header();
	extern void flc_all();
	extern void flc_coll();
	extern void flc_ctype();
	extern void flc_mntry();
	extern void flc_nmrc();
	extern void flc_time();
	extern void flc_mesg();
	extern void write_locale();
	extern void debug_locale();

	extern char *strcpy();
	extern char *strcat();
	extern int fprintf(), sprintf();
	extern int system();
	/* Posix.2 LC_MONETARY */
	extern void grp_init();
	extern void grp_num();
	extern void grp_finish();
	extern char lname[], territory[], codeset[];
	int token;
	int i;
	char cmd[CMDLEN];

	/*
	** initialization
	 */
#ifdef EUC
	lctable_head.codeset = CODE1BYTE;

#endif /* EUC */
	lcctype_head.sh_low = DFL_LOWCHAR + 1;
	lcctype_head.sh_high = DFL_HICHAR;
	c_offset = ZERO - DFL_LOWCHAR;
	c2_offset = ZERO - DFL_LOWCHAR; /* Posix 11.2 */
	ctype1 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));
	ctype2 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));
	ctype3 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));
	ctype4 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));
	/* For ctype2 table printing */
	ctype5 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));

	s_offset = ZERO;
	upper = (unsigned char *)calloc(SHIFTSIZ, sizeof(unsigned char));
	lower = (unsigned char *)calloc(SHIFTSIZ, sizeof(unsigned char));
	for (i = s_offset; i <= DFL_HICHAR; i++)
		upper[i] = i;
	for (i = s_offset; i <= DFL_HICHAR; i++)
		lower[i] = i;

#ifdef EUC
	e_cset = (unsigned char *)malloc(SHIFTSIZ * sizeof(unsigned char));
	for (i = 0; i < SHIFTSIZ/2; i++)
		e_cset[i] = 0;
	for (i = SHIFTSIZ/2; i < SHIFTSIZ; i++)
		e_cset[i] = 1;

	ein_csize = (unsigned char *)malloc(SHIFTSIZ * sizeof(unsigned char));
	eout_csize = (unsigned char *)malloc(SHIFTSIZ * sizeof(unsigned char));
	for (i = 0; i < SHIFTSIZ; i++)
		ein_csize[i] = 1;
	for (i = 0; i < SHIFTSIZ; i++)
		eout_csize[i] = 1;

#endif /* EUC */

	for (i = 0; i < TOT_ELMT; i++) {
		seq_tab[i].seq_no = UNDEF; /* changed from 0 to posix UNDEF */
		seq_tab[i].type_info = DC;
	}

#ifdef	DBG
	/*
	** open "debug" output file
	*/
	if((dfp = fopen("debug_file", "w")) == NULL)
		fprintf(stderr,(catgets(catd,NL_SETN,7, "can't open 'debug' file\n")));
#endif

	yyin = fp;

	/*
	** read & process input file (localedef script)
	*/
	lctable_head.comment_char = '#';
	lctable_head.escape_char = '\\';
	while (token = yylex()) {
	    switch (token) {
		case LANG_NAME:
		    name_init(token);
		    break;
		case LANG_ID:
		    id_init(token);
		    break;
		case HPREVISION:
		case RE_VISION:
		    rev_init(token);
		    break;
		case STRING:
		    (*string)();
		    break;
		case NUMBER:
		    if (number == NULL) error(STATE);
		    (*number)(); 
		    break;
		/* Posix 11.2 */
		case ESCAPE:
		   switch (yylex()) {
                      case BAD_LEXEME:
                        lctable_head.escape_char = yytext[0];
                        break;
                    case NUMBER:
                        lctable_head.escape_char = num_value;
                        break;
                    default:
                        error(CHAR);
                    }
		    break;
		case COMMENT:
		   switch (yylex()) {
		      case BAD_LEXEME:
			lctable_head.comment_char = yytext[0];
			break;
		      case NUMBER:
			lctable_head.comment_char = num_value;
			break;
		      default:
			error(CHAR);
		   }
		   break;
	       /* 
		* Posix.2: Similar to LC_ALL
		*/
                case TLC_MESG:
		  copy_flag=FALSE;
		  cur_cat = LC_MESSAGES;
		  if (mesg_count == VISITED)
		    break;
		  mesg_count++;
		  cur_mod = 0;
		  cmlog[LC_MESSAGES].n_mod++;
		  gotinfo = FALSE;
		  while ((token = yylex()) && (token != END_LC)) {
		      if ( token==EOL )
			continue;
		      if ( copy_flag )
			error (KEY);
		      switch(token) {
		      	case MODIFIER:
			   mod_init();
			   break;
		      	case COPY:
			   copy_init(token);
			   break;
		      	case MESG:
			   switch (num_value) {
			        case YESEXPR:
			        case NOEXPR:
         			case YESSTR:
		        	case NOSTR:
                  			info_init(token);
					break;
                                default:
					error(STATE);
					  break;
			   }
			   gotinfo = TRUE;
			   break;
                         case STRING:
			   (*string)();
			   break;
		         default:
			   error(STATE);
			   break;
		    }
		  } /* while */
		    if ( !copy_flag )
		  	flc_mesg();
		  break;
		case TLC_ALL:
		    copy_flag=FALSE;
		    cur_cat = LC_ALL;
		    if (all_count == VISITED)
		      break;
		    all_count++;
		    cur_mod = 0;
		    cmlog[LC_ALL].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
		      	if ( token==EOL )
				continue;
		      	if ( copy_flag )
				error (KEY);
			switch (token) {
                            case COPY: 
				copy_init(token);
				break;
			    case MODIFIER:
				mod_init(token);
				break;
			    case ALL:
				switch (num_value) {
			 	    case YESSTR:
				    case NOSTR:
				    case DIRECTION:
				    case CONTEXT:
					info_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    /* Posix.2 */
                            case COLL_SYM:
				error(SYM_NOT_FOUND);
				break;
				
			    default:
				error(STATE);
				break;
			}
		    }
		    if ( !copy_flag )
		    	flc_all();
		    break;
		case TLC_COLLATE:
		    copy_flag=FALSE;
		    if (coll_count == VISITED)
		      break;
		    coll_count++;
		    cur_cat = LC_COLLATE;
		    cur_mod = 0;
		    cmlog[LC_COLLATE].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
		      	if ( token==EOL )
				continue;
		      	if ( copy_flag )
				error (KEY);
			switch (token) {
			    case COPY:
				copy_init(token);
				break;
			    case MODIFIER:
				mod_init(token);
				if (mod_count > 0)
					gotinfo = TRUE; 
				mod_count++;
				break;
			    case STRING:
				(*string)();
				break;
			    case NUMBER:
				if (number == NULL) error(STATE);
				(*number)(); 
				break;
			    /* Posix 11.2 */
                            case ELLIPSIS:
				ellipsis = TRUE;
			   /* Revisit: check for character included in 
			      ellipsis that is explicit elsewhere => error()
                            */
				break;
                           /* Posix 11.2 code */
			      case ELEM:
				collate =0;
				elem_init();
				break;
			      case SYM:
				sym_init();
				break;
			      case COLL_SYM:
				(*collate_sym)();
				break;
			      case FROM:
				break;
			      case S_ORDER:
				pos_count = 0 ;
				collate++;
				/*
				sort_rule_init();
				*/
				while ((token = yylex()) != EOL)
				  {
                                switch(token){
				  case FORWARD:
				  case REVERSE:
				  case POSITION:
				    sort_rule_enter(token, pos_count);
				    break;
				  case COMMA:
				    break;
				  case SEMI:
				    pos_count++;
				    /* Revisit
				     * If number of 
				     * sort_rules > COLL_WEIGHTS_MAX
				     * error();
				     */
				    break;

				 }
			      }
			    /* 
			     * Set up function pointers, trackers and
			     * stuff needed for interpreting collate sequence
			     */
				order_init();
				/* 
				 * Run an outer loop until "order_end"
				 */
				while((token = yylex()) != E_ORDER)
				  {
				    /*
				     * Call the function that reads a 
				     * collating-entry, puts the collating 
				     * element and the weights in a structure
				     * The current and last entry are always 
				     * available
				     */
				    get_entry(token);
				    /*
				     * The type is passed as an arguement
				     * to a function that stores the 
				     * information in the structure and takes
				     * other actions based on the type of entry
				     */
				    process_entry(token);
				  } /* while */
				break; /* end of S_ORDER */

		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
                      
			    default:
				error(STATE);
				break;
			}
		    }
		    col_finish();
		    if ( !copy_flag )
		    	flc_coll();
		    break;
		case TLC_CTYPE:
		    copy_flag=FALSE;
		    cur_cat = LC_CTYPE;
		    if (ctype_count == VISITED)
		      break;
		    ctype_count++;
		    cur_mod = 0;
		    cmlog[LC_CTYPE].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
		      	if ( token==EOL )
				continue;
		      	if ( copy_flag )
				error (KEY);
			switch (token) {
			    /* Posix 11.2 */
			    case COPY:
				copy_init(token);
				break;
                            case COLL_SYM:
				warning(SYM_NOT_FOUND);
				break;
			    case MODIFIER:
				mod_init(token);
				break;
			    case ISUPPER:
				cty_init(token,_U,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISLOWER:
				cty_init(token,_L,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISDIGIT:
				cty_init(token,_N,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISSPACE:
				cty_init(token,_S,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISPUNCT:
				cty_init(token,_P,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISCNTRL:
				cty_init(token,_C,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISBLANK:
				cty_init(token,_B,ctype1+c_offset);
				gotinfo = TRUE;
				/* used in cty_defs() - ctype.c */
				blank_found = TRUE; 
				break;
			    case ISXDIGIT:
				cty_init(token,_X,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISFIRST:
				is1st++;
				cty_init(token,_K1,ctype2+c_offset);
				gotinfo = TRUE;
				break;
			    case ISSECOND:
				cty_init(token,_K2,ctype2+c_offset);
				gotinfo = TRUE;
				break;
			    /* case SHIFTRANGE:
				shrang_init(token);
				break; */
			    case TOUPPER:
				con_init(token);
				gotinfo = TRUE;
				break;
			    case TOLOWER:
				con_init(token);
				gotinfo = TRUE;
				found_tolower = TRUE; /* ??? used in conv.c */
				break;
#ifdef EUC
			    case CODE__SCHEME:
				cscheme_init(token);
				gotinfo = TRUE;
				break;
			    case CS_WIDTH:
				cswidth_init(token);
				gotinfo = TRUE;
				break;
#endif /* EUC */
			    case CTYPE:
				switch (num_value) {
			 	    case BYTES_CHAR:
				    case ALT_PUNCT:
				        ctype_flag = TRUE;
					info_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
			    case NUMBER:
				if (number == NULL) error(STATE);
				(*number)(); 
				break;
			      case ISGRAPH:
				cty_init(token,_G,ctype5+c2_offset);
				break;
			      case ISPRINT:
				cty_init(token,_PR,ctype5+c2_offset);
				break;
			      case ISALPHA:
				cty_init(token,_A,ctype5+c2_offset);
				break;
			      case LP:
				lp += 1;
				if (l_paren == NULL) error(STATE);
				(*l_paren)();
				break;
			      case RP:
				rp += 1;
				if (r_paren == NULL) error(STATE);
				(*r_paren)();
				break;
			      case ELLIPSIS:
				ellipsis = TRUE;
				break;
			      case SEMI:
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
        		    case BAD_KEYWORD:
				error(KEY);
				break;
			    case COMMA:
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    /* cty_defs is a function which prohibits incorrect
		       ctype definitions 
		     */
	/*          cty_defs();   */
		    if ( !copy_flag )
		    	flc_ctype();
		    ctype_flag = FALSE;
		    break;
		case TLC_MONETARY:
		    copy_flag=FALSE;
		    cur_cat = LC_MONETARY;
		    if (mon_count == VISITED)
		      break;
		    mon_count++;
		    cur_mod = 0;
		    cmlog[LC_MONETARY].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
		      	if ( token==EOL )
				continue;
		      	if ( copy_flag )
				error (KEY);
			switch (token) {
			    case COPY:
				copy_init(token);
				break;
			    case MODIFIER:
				mod_init(token);
				break;
			    case MONEY:
				switch (num_value) {
			 	    case INT_CURR:	/*int_curr_symbol*/
			 	    case CURRENCY_LC:	/*currency_symbol*/
			 	    case CURRENCY_LI:	/*crncystr*/
			 	    case MON_DECIMAL:	/*mon_decimal_point*/
			 	    case MON_THOUSANDS:	/*mon_thousands_sep*/
			 	    case POSITIVE_SIGN: /*positive_sign*/
			 	    case NEGATIVE_SIGN:	/*negative_sign*/

			 	    case INT_FRAC:	/*int_frac_digits*/
			 	    case FRAC_DIGITS:	/*frac_digits*/
			 	    case P_CS:		/*p_cs_precedes*/
			 	    case P_SEP:		/*p_sep_by_space*/
			 	    case N_CS:		/*n_cs_precedes*/
			 	    case N_SEP:		/*n_sep_by_space*/
			 	    case P_SIGN:	/*p_sign_posn*/
			 	    case N_SIGN:	/*n_sign_posn*/
					mntry_init(token);
					op=num_value;
					break;
			 	    case MON_GROUPING:
					grp_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
			    case NUMBER:
				if (number == NULL) error(STATE);
				(*number)();
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    /* Posix 11.2 */
                            case COLL_SYM:
				error(SYM_NOT_FOUND);
				break;
			    case SEMI:
				semi_flag = TRUE;
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    if ( !copy_flag )
		    	flc_mntry();
		    break;
		case TLC_NUMERIC:
		    copy_flag=FALSE;
		    cur_cat = LC_NUMERIC;
		    if (num_count == VISITED)
		      break;
		    num_count++;
		    cur_mod = 0;
		    cmlog[LC_NUMERIC].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
		      	if ( token==EOL )
				continue;
		      	if ( copy_flag )
				error (KEY);
			switch (token) {
			    case COPY:
				copy_init(token);
				break;
			    case MODIFIER:
				mod_init(token);
				break;
			    case NUMERIC:
				switch (num_value) {
			 	    case GROUPING:
				          grp_init(token);
					  break;
			 	    case DECIMAL_P:
			 	    case THOUSANDS_S:
					numeric_init(token);
					break;
			 	    case ALT_DIGIT:
					info_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
                            case NUMBER:
				if (number == NULL) error(STATE);
                                (*number)();
                                break;				
			      case SEMI:
				semi_flag = TRUE;
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    /* Posix 11.2 */
                            case COLL_SYM:
				error(SYM_NOT_FOUND);
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    if ( !copy_flag )
		    	flc_nmrc();
		    break;
		case TLC_TIME:
		    copy_flag=FALSE;
		    cur_cat = LC_TIME;
		    if (time_count == VISITED)
		      break;
		    time_count++;
		    cur_mod = 0;
		    cmlog[LC_TIME].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
		      	if ( token==EOL )
				continue;
		      	if ( copy_flag )
				error (KEY);
			switch (token) {
			    /* Posix.2 */
			    case COPY:
				copy_init(token);
				break;
			    case ERA_YEAR:
			    case ALT_DIGITS:
			          warning(NOT_SUPP);
				  while ( yylex() != EOL) ;
				  break;
			    case MODIFIER:
				mod_init(token);
				break;
			    case TIME:
				switch (num_value) {
			 	    case D_T_FMT:
			 	    case D_FMT:
			 	    case T_FMT:
			 	    case YEAR_UNIT:
			 	    case MON_UNIT:
			 	    case DAY_UNIT:
			 	    case HOUR_UNIT:
			 	    case MIN_UNIT:
			 	    case SEC_UNIT:
			 	    case ERA_FMT:
				    case T_FMT_AMPM: /* Posix T_FMT_AMPM */
					time_init(token);
					break;
					/* Posix */
				    case DAY_1:
					while(num_value <= DAY_7) {
					    time_init(token);
					    token = yylex();
					    switch(token){
					      case SEMI:
					      	num_value++;
					      	break;
					      case STRING:
					      	(*string)();
					      	break;
					      case EOL:
					      	num_value++;
					      	break;
					      default:
					      	error(CHAR);
					      	break;
					    }
					} 
				        break;
				    case ABDAY_1:
                                        while(num_value <= ABDAY_7) {
                                            time_init(token);
                                            token = yylex();
                                            switch(token){
                                              case SEMI:
                                              	num_value++;
                                              	break;
                                              case STRING:
                                              	(*string)();
                                              	break;
                                              case EOL:
                                              	num_value++;
                                             	break;
                                              default:
                                              	error(CHAR);
                                              	break;
                                            }
                                        } 
				        break;
				    case MON_1:
					while(num_value <= MON_12) {
                       		            time_init(token);
					    token = yylex();
					    switch(token){
					      case SEMI:
					      	num_value++;
					      	break;
					      case STRING:
					      	(*string)();
					    	  break;
					      case EOL:
					      	num_value++;
					      	break;
					      default:
					      	error(CHAR);
					      	break;
					    }
					} 
				        break;
                                    case ABMON_1:
				        while(num_value <= ABMON_12) {
                                            time_init(token);
                                            token = yylex();
                                            switch(token){
                                              case SEMI:
                                              	num_value++;
                                              	break;
                                              case STRING:
                                              	(*string)();
                                              	break;
                                              case EOL:
                                              	num_value++;
                                              	break;
                                              default:
                                              	error(CHAR);
                                              	break;
                                            }
                                        } 
				        break;
				    /* token of AM_PM returns AM_STR */
				    case AM_STR:
           				time_init(token);
					while( (token=yylex()) != EOL ) {
					    extern int gotstr;
					    switch(token){
					      case SEMI:
						gotstr=FALSE;
						num_value++;
					      	break;
					      case STRING:
					      	(*string)();
					      	break;
					      default:
					      	error(CHAR);
					      	break;
					    }
				        } 
				        break;
                                    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
                            case COLL_SYM:
				error(SYM_NOT_FOUND);
				break;
			    case ERA:
				era_init(token);
				break;
			    case STRING:
				(*string)();
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			   /* Posix.2 */
			    default:
				error(STATE);
				break;
			}
		    }
		    if ( !copy_flag )
		    	flc_time();
		    break;
		case BAD_LEXEME:
		    error(CHAR);
		    break;
		case BAD_KEYWORD:
		    error(KEY);
		    break;
                case EOL:
		    break;
		    
		}
	}
	if (finish == NULL) error(STATE);
	(*finish)(); 

	/* if langname or langid is not present in
	 * the script ...
	 */
	chk_name_str();
	chk_id_num();

	/*
	** check the completeness & consistency of the localedef script
	*/
	if (*lctable_head.lang == '\0')
		fprintf(stderr,(catgets(catd,NL_SETN,10, "no 'langname' specification\n")));

	if (lctable_head.nl_langid == 0)
		fprintf(stderr,(catgets(catd,NL_SETN,11, "no 'langid' specification\n")));

	if (newlang && !newid || newid && !newlang)
		fprintf(stderr,(catgets(catd,NL_SETN,12, "specification conflict in 'langname' and 'langid'\n")));

	if (is1st && !LANG2BYTE || LANG2BYTE && !is1st)
		fprintf(stderr,(catgets(catd,NL_SETN,13, "specification conflict in 'isfirst' and 'byte_chars'\n")));

	if (LANG2BYTE && collate)
		fprintf(stderr,
			(catgets(catd,NL_SETN,14, "WARNING: sequence specification will be ignored\n")));

	/* make sure there is no conflict between yes and no strings */
	if (*info_tab[YESSTR] != NULL && *info_tab[NOSTR] != NULL) {
		if (LANG2BYTE) {
			if (!strncmp((char *)info_tab[YESSTR],
		    	    (char *)info_tab[NOSTR], 2)) 
				error(SAME_YESNO);
			if (!strncmp((char *) info_tab[YESSTR],
			    (char *)info_tab[NOSTR], 1) &&
				  !(ctype2[*info_tab[YESSTR]+c_offset]&_K1))
				error(SAME_YESNO);
		}
		else {
			if (!strncmp((char *) info_tab[YESSTR],
			    (char *)info_tab[NOSTR], 1))
				error(SAME_YESNO);
		}
	}

	/* process UNDEFINED keyword */
	/* get last sequence number if UNDEFINED did not exist */
	if(undef_seq == UNDEF) undef_seq = seq_no++;

	/* put all undefined chars in the same class with different
	 * priority numbers -- defined by their binary values 
	 */
	{
	int j = 0;
	for (i = 0; i < TOT_ELMT; i++) {
		if(seq_tab[i].seq_no == UNDEF) {
			seq_tab[i].seq_no = undef_seq;
			seq_tab[i].type_info = j;
		}
		if(j < 0x3f) j++;	/* we cannot have more than 64
					 * chars per class, since upper 2 bits
					 * cannot be used */
	}
	}


	/*
	** fill in all locale.inf header information
	*/
	fill_lc_header();

	/** write header & data to the "locale.inf" file
	*/
	write_locale();

	/*
	** for debugging purpose only (output to stdout)
	*/
	debug_locale();
#ifdef	DBG
	fclose(dfp);
#endif

	/*
	** if the "-n" option is not specified
	** install the "locale.inf" file and update the "config" file
	*/
	if (!no_install) {			/* install "locale.inf" file */
		if (newlang || newid)		/* update "config" file */
#ifndef DBG
			sprintf(cmd, "/usr/bin/instlang_inf y %d %s \"%s\" \"%s\" \"%s\" \"%s\"",
				lctable_head.nl_langid, lctable_head.lang,
				lname, territory, codeset,locale_name);
#else DBG
			sprintf(cmd, "./instlang_inf y %d %s \"%s\" \"%s\" \"%s\" \"%s\"",
				lctable_head.nl_langid, lctable_head.lang,
				lname, territory, codeset,locale_name);
#endif DBG
		else				/* don't update "config" file */
#ifndef DBG
			sprintf(cmd, "/usr/bin/instlang_inf n %s %s %s",
				lname, territory, codeset);
#else DBG
			sprintf(cmd, "./instlang_inf n %s %s %s",
				lname, territory, codeset);
#endif DBG
		if (freopen("/dev/tty","r",fp) == NULL) {
			fprintf(stderr,(catgets(catd,NL_SETN,3, "cannot reopen stdin\n")));
			exit (10);
		}
		system(cmd);
	}
}


/* fill_lc_header:
** fill in each locale.inf section header
** fill in locale.inf file header 
** fill in each catinfo structure
*/
#define	CATINFO_SIZE	(sizeof(struct catinfotype))
void
fill_lc_header()
{
	int mod_addr = sizeof(lctable_head) + (N_CATEGORY * CATINFO_SIZE);
	int m = N_CATEGORY;
	int cat_total = 0;
	int i;

	/*
	** fill in locale.inf FILE HEADER
	*/
	if (LANG2BYTE && collate) {
		cmlog[LC_COLLATE].n_mod = 0;
		cmlog[LC_COLLATE].cm[0].size = 0;
		(void) strcpy(cmlog[LC_COLLATE].cm[0].mod_name, "");
		cmlog[LC_COLLATE].cm[0].tmp = NULL;
	}

	for (i = 0; i < N_CATEGORY; i++) {
		if (cmlog[i].n_mod > 0) {
			cat_total += cmlog[i].cm[0].size;
			lctable_head.cat_no++;
			lctable_head.mod_no += cmlog[i].n_mod;
		}
	}
	lctable_head.mod_no -= lctable_head.cat_no;
	lctable_head.cat_no = N_CATEGORY;
	lctable_head.size = sizeof(lctable_head) +
			   (N_CATEGORY + lctable_head.mod_no) * CATINFO_SIZE;

#ifdef EUC
	if (LANG2BYTE && lctable_head.codeset == CODE1BYTE)
		lctable_head.codeset = CODE_HP15;
#else /* !EUC */
	if (LANG2BYTE)
		lctable_head.codeset = CODE2BYTE;
	else
		lctable_head.codeset = CODE1BYTE;
#endif /* !EUC */

	/*
	** fill in LC_ALL category and/or modifier catinfo
	*/
	catinfo[LC_ALL].size = cmlog[LC_ALL].cm[0].size;
	catinfo[LC_ALL].addr = 0;
	catinfo[LC_ALL].catid = LC_ALL;
	(void) strcpy(catinfo[LC_ALL].mod_name, cmlog[LC_ALL].cm[0].mod_name);
	if (cmlog[LC_ALL].n_mod <= 1)
		catinfo[LC_ALL].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_ALL].n_mod; i++) {
			if (i == 1)
				catinfo[LC_ALL].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_ALL].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_ALL;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_ALL].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_COLLATE category and/or modifier catinfo
	*/
	catinfo[LC_COLLATE].size = cmlog[LC_COLLATE].cm[0].size;
	catinfo[LC_COLLATE].addr = catinfo[LC_ALL].addr +
				    catinfo[LC_ALL].size;
	catinfo[LC_COLLATE].catid = LC_COLLATE;
	(void) strcpy(catinfo[LC_COLLATE].mod_name,
		      cmlog[LC_COLLATE].cm[0].mod_name);
	if (cmlog[LC_COLLATE].n_mod <= 1)
		catinfo[LC_COLLATE].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_COLLATE].n_mod; i++) {
			if (i == 1)
				catinfo[LC_COLLATE].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_COLLATE].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_COLLATE;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_COLLATE].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_CTYPE catinfo
	*/
	catinfo[LC_CTYPE].size = cmlog[LC_CTYPE].cm[0].size;
	catinfo[LC_CTYPE].addr = catinfo[LC_COLLATE].addr +
				  catinfo[LC_COLLATE].size;
	catinfo[LC_CTYPE].catid = LC_CTYPE;
	(void) strcpy(catinfo[LC_CTYPE].mod_name,
		      cmlog[LC_CTYPE].cm[0].mod_name);
	if (cmlog[LC_CTYPE].n_mod <= 1)
		catinfo[LC_CTYPE].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_CTYPE].n_mod; i++) {
			if (i == 1)
				catinfo[LC_CTYPE].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_CTYPE].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_CTYPE;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_CTYPE].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_MONETARY catinfo
	*/
	catinfo[LC_MONETARY].size = cmlog[LC_MONETARY].cm[0].size;
	catinfo[LC_MONETARY].addr = catinfo[LC_CTYPE].addr +
				     catinfo[LC_CTYPE].size;
	catinfo[LC_MONETARY].catid = LC_MONETARY;
	strcpy(catinfo[LC_MONETARY].mod_name,
		      cmlog[LC_MONETARY].cm[0].mod_name);
	if (cmlog[LC_MONETARY].n_mod <= 1)
		catinfo[LC_MONETARY].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_MONETARY].n_mod; i++) {
			if (i == 1)
				catinfo[LC_MONETARY].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_MONETARY].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_MONETARY;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_MONETARY].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_NUMERIC catinfo
	*/
	catinfo[LC_NUMERIC].size = cmlog[LC_NUMERIC].cm[0].size;
	catinfo[LC_NUMERIC].addr = catinfo[LC_MONETARY].addr +
				    catinfo[LC_MONETARY].size;
	catinfo[LC_NUMERIC].catid = LC_NUMERIC;
	strcpy(catinfo[LC_NUMERIC].mod_name,
		      cmlog[LC_NUMERIC].cm[0].mod_name);
	if (cmlog[LC_NUMERIC].n_mod <= 1)
		catinfo[LC_NUMERIC].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_NUMERIC].n_mod; i++) {
			if (i == 1)
				catinfo[LC_NUMERIC].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_NUMERIC].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_NUMERIC;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_NUMERIC].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_TIME catinfo
	*/
	catinfo[LC_TIME].size = cmlog[LC_TIME].cm[0].size;
	catinfo[LC_TIME].addr = catinfo[LC_NUMERIC].addr +
				 catinfo[LC_NUMERIC].size;
	catinfo[LC_TIME].catid = LC_TIME;
	strcpy(catinfo[LC_TIME].mod_name,
		      cmlog[LC_TIME].cm[0].mod_name);
	if (cmlog[LC_TIME].n_mod <= 1)
		catinfo[LC_TIME].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_TIME].n_mod; i++) {
			if (i == 1)
				catinfo[LC_TIME].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_TIME].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_TIME;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_TIME].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}
	/*
	** fill in LC_MESSAGES catinfo
	*/
	catinfo[LC_MESSAGES].size = cmlog[LC_MESSAGES].cm[0].size;
	catinfo[LC_MESSAGES].addr = catinfo[LC_TIME].addr +
				 catinfo[LC_TIME].size;
	catinfo[LC_MESSAGES].catid = LC_MESSAGES;
	strcpy(catinfo[LC_MESSAGES].mod_name,
		      cmlog[LC_MESSAGES].cm[0].mod_name);
	if (cmlog[LC_MESSAGES].n_mod <= 1)
		catinfo[LC_MESSAGES].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_MESSAGES].n_mod; i++) {
			if (i == 1)
				catinfo[LC_MESSAGES].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_MESSAGES].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_MESSAGES;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_MESSAGES].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}
}

/* flc_all:
** fill in LC_ALL section header & category size
*/
void
flc_all()
{
	lcall_head.yes_addr = sizeof(lcall_head);
	lcall_head.no_addr = lcall_head.yes_addr +
				strlen(info_tab[YESSTR]) + 1;
	lcall_head.direct_addr = lcall_head.no_addr +
				strlen(info_tab[NOSTR]) + 1;
	lcall_head.context_addr = lcall_head.direct_addr +
				strlen(info_tab[DIRECTION]) + 1;

	cmlog[LC_ALL].cm[cur_mod].size = lcall_head.context_addr +
					  strlen(info_tab[CONTEXT]) + 1;
	cmlog[LC_ALL].cm[cur_mod].pad =
			(4 - (cmlog[LC_ALL].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_ALL].cm[cur_mod].size += cmlog[LC_ALL].cm[cur_mod].pad;
	
	if (cmlog[LC_ALL].cm[cur_mod].size > _LC_ALL_SIZE)
		fprintf(stderr,(catgets(catd,NL_SETN,15, "LC_ALL category exceeds the maximum size allowed\n")));
}
/*
 * Posix.2: flc_mesg:
** fill in LC_MESSAGES section header & category size
 */
void
flc_mesg()
{
	lcmesg_head.yes_addr = sizeof(lcmesg_head);
	lcmesg_head.no_addr = lcmesg_head.yes_addr +
				strlen(info_tab[YESEXPR]) + 1;
	lcmesg_head.yesstr_addr = lcmesg_head.no_addr +
	                             strlen(info_tab[NOEXPR]) +1;
	lcmesg_head.nostr_addr = lcmesg_head.yesstr_addr +
	                                 strlen(info_tab[YESSTR]) +1;
	cmlog[LC_MESSAGES].cm[cur_mod].size = lcmesg_head.nostr_addr +
					  strlen(info_tab[NOSTR]) + 1;
	cmlog[LC_MESSAGES].cm[cur_mod].pad =
			(4 - (cmlog[LC_MESSAGES].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_MESSAGES].cm[cur_mod].size += cmlog[LC_MESSAGES].cm[cur_mod].pad;
	
	if (cmlog[LC_MESSAGES].cm[cur_mod].size > _LC_MESSAGES_SIZE)
		fprintf(stderr,(catgets(catd,NL_SETN,16, "LC_MESSAGES category exceeds the maximum size allowed\n")));
}


/* flc_coll:
** fill in LC_COLLATE section header & catinfo size
*/
void
flc_coll()
{
	if (two1_len)
		lccol_head.nl_map21 = TRUE;
	else
		lccol_head.nl_map21 = FALSE;

	if (max_pri_no)
		lccol_head.nl_onlyseq = FALSE;
	else
		lccol_head.nl_onlyseq = TRUE;
       lccol_head.sortrule[0] = sort_rules[0];
       lccol_head.sortrule[1] = sort_rules[1];
	lccol_head.seqtab_addr = sizeof(lccol_head);
	lccol_head.pritab_addr = lccol_head.seqtab_addr + TOT_ELMT;
	lccol_head.two1tab_addr = lccol_head.pritab_addr + TOT_ELMT;
	lccol_head.one2tab_addr = lccol_head.two1tab_addr + two1_len;

	cmlog[LC_COLLATE].cm[cur_mod].size = lccol_head.one2tab_addr + one2_len;
	cmlog[LC_COLLATE].cm[cur_mod].pad =
			(4 - (cmlog[LC_COLLATE].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_COLLATE].cm[cur_mod].size += cmlog[LC_COLLATE].cm[cur_mod].pad;

	if (cmlog[LC_COLLATE].cm[cur_mod].size > _LC_COLLATE_SIZE)
		fprintf(stderr,(catgets(catd,NL_SETN,17, "LC_COLLATE category exceeds the maximum size allowed\n")));
}

/* flc_ctype:
** fill in LC_CTYPE section header & catinfo size
*/
void
flc_ctype()
{
	lcctype_head.ctype_addr = sizeof(lcctype_head);
	lcctype_head.ctype2_addr = lcctype_head.ctype_addr+ CTYPESIZ;
	lcctype_head.kanji1_addr = lcctype_head.ctype2_addr + CTYPESIZ;
	lcctype_head.kanji2_addr = lcctype_head.kanji1_addr + CTYPESIZ;
	lcctype_head.upshift_addr = lcctype_head.kanji2_addr + CTYPESIZ;
	lcctype_head.downshift_addr = lcctype_head.upshift_addr + SHIFTSIZ;
	lcctype_head.byte_char_addr = lcctype_head.downshift_addr + SHIFTSIZ;
	lcctype_head.alt_punct_addr = lcctype_head.byte_char_addr +
				strlen(info_tab[BYTES_CHAR]) + 1;
#ifdef EUC
	lcctype_head.io_csize_addr = lcctype_head.alt_punct_addr +
				strlen(info_tab[ALT_PUNCT]) + 1;
	lcctype_head.e_cset_addr = lcctype_head.io_csize_addr +
				(2 * MAX_CODESET);
	lcctype_head.ein_csize_addr = lcctype_head.e_cset_addr + SHIFTSIZ;
	lcctype_head.eout_csize_addr = lcctype_head.ein_csize_addr + SHIFTSIZ;
	lcctype_head.code_scheme_addr = lcctype_head.eout_csize_addr +
	                                      SHIFTSIZ;
	lcctype_head.cswidth_addr = lcctype_head.code_scheme_addr +
	                                   strlen(info_tab[CODE_SCHEME]) + 1;
#endif /* EUC */

#ifdef EUC
	cmlog[LC_CTYPE].cm[cur_mod].size = lcctype_head.cswidth_addr +
	                                    strlen(info_tab[CSWIDTH]) + 1;
#else /* EUC */
	cmlog[LC_CTYPE].cm[cur_mod].size = lcctype_head.alt_punct_addr +
					    strlen(info_tab[ALT_PUNCT]) + 1;
#endif /* EUC */
	cmlog[LC_CTYPE].cm[cur_mod].pad =
			(4 - (cmlog[LC_CTYPE].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_CTYPE].cm[cur_mod].size += cmlog[LC_CTYPE].cm[cur_mod].pad;

	if (cmlog[LC_CTYPE].cm[cur_mod].size > _LC_CTYPE_SIZE)
		fprintf(stderr,(catgets(catd,NL_SETN,18, "LC_CTYPE category exceeds the maximum size allowed\n")));
}

/* flc_mntry:
** fill in LC_MONETARY section header & catinfo size
*/
void
flc_mntry()
{
	lcmonetary_head.int_frac_digits = sizeof(lcmonetary_head);
	lcmonetary_head.frac_digits = lcmonetary_head.int_frac_digits +
				      strlen(mntry_tab[INT_FRAC]) + 1;
	lcmonetary_head.p_cs_precedes = lcmonetary_head.frac_digits +
				        strlen(mntry_tab[FRAC_DIGITS]) + 1;
	lcmonetary_head.p_sep_by_space = lcmonetary_head.p_cs_precedes +
				         strlen(mntry_tab[P_CS]) + 1;
	lcmonetary_head.n_cs_precedes = lcmonetary_head.p_sep_by_space +
				        strlen(mntry_tab[P_SEP]) + 1;
	lcmonetary_head.n_sep_by_space = lcmonetary_head.n_cs_precedes +
				         strlen(mntry_tab[N_CS]) + 1;
	lcmonetary_head.p_sign_posn = lcmonetary_head.n_sep_by_space +
				      strlen(mntry_tab[N_SEP]) + 1;
	lcmonetary_head.n_sign_posn = lcmonetary_head.p_sign_posn +
				      strlen(mntry_tab[P_SIGN]) + 1;
	lcmonetary_head.currency_symbol_lc = lcmonetary_head.n_sign_posn +
				          strlen(mntry_tab[N_SIGN]) + 1;
	lcmonetary_head.currency_symbol_li = lcmonetary_head.currency_symbol_lc
					   + strlen(mntry_tab[CURRENCY_LC]) + 1;
	lcmonetary_head.mon_decimal_point = lcmonetary_head.currency_symbol_li +
				          strlen(mntry_tab[CURRENCY_LI]) + 1;
	lcmonetary_head.int_curr_symbol = lcmonetary_head.mon_decimal_point +
				          strlen(mntry_tab[MON_DECIMAL]) + 1;
	lcmonetary_head.mon_thousands_sep = lcmonetary_head.int_curr_symbol +
				            strlen(mntry_tab[INT_CURR]) + 1;
	lcmonetary_head.mon_grouping = lcmonetary_head.mon_thousands_sep +
				       strlen(mntry_tab[MON_THOUSANDS]) + 1;
	lcmonetary_head.positive_sign = lcmonetary_head.mon_grouping +
				        strlen(mntry_tab[MON_GROUPING]) + 1;
	lcmonetary_head.negative_sign = lcmonetary_head.positive_sign +
				        strlen(mntry_tab[POSITIVE_SIGN]) + 1;

	cmlog[LC_MONETARY].cm[cur_mod].size = lcmonetary_head.negative_sign +
					strlen(mntry_tab[NEGATIVE_SIGN]) + 1;
	cmlog[LC_MONETARY].cm[cur_mod].pad =
			(4 - (cmlog[LC_MONETARY].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_MONETARY].cm[cur_mod].size +=
					cmlog[LC_MONETARY].cm[cur_mod].pad;

	if (cmlog[LC_MONETARY].cm[cur_mod].size > _LC_MONETARY_SIZE)
		fprintf(stderr,(catgets(catd,NL_SETN,19, "LC_MONETARY category exceeds the maximum size allowed\n")));
}

/* flc_nmrc:
** fill in LC_NUMERIC section header & catinfo size
*/
void
flc_nmrc()
{
	lcnumeric_head.grouping = sizeof(lcnumeric_head);
	lcnumeric_head.decimal_point = lcnumeric_head.grouping +
				       strlen(nmrc_tab[GROUPING]) + 1;
	lcnumeric_head.thousands_sep = lcnumeric_head.decimal_point +
				       strlen(nmrc_tab[DECIMAL_P]) + 1;
	lcnumeric_head.alt_digit_addr = lcnumeric_head.thousands_sep +
				        strlen(nmrc_tab[THOUSANDS_S]) + 1;

	cmlog[LC_NUMERIC].cm[cur_mod].size = lcnumeric_head.alt_digit_addr +
			 		      strlen(info_tab[ALT_DIGIT]) + 1;
	cmlog[LC_NUMERIC].cm[cur_mod].pad =
			(4 - (cmlog[LC_NUMERIC].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_NUMERIC].cm[cur_mod].size += cmlog[LC_NUMERIC].cm[cur_mod].pad;

	if (cmlog[LC_NUMERIC].cm[cur_mod].size > _LC_NUMERIC_SIZE)
		fprintf(stderr,(catgets(catd,NL_SETN,20, "LC_NUMERIC category exceeds the maximum size allowed\n")));
}

/* flc_time:
** fill in LC_TIME section header & catinfo size
*/
void
flc_time()
{
	pad_byte = 0;

	lctime_head.d_t_fmt = sizeof(lctime_head);
	lctime_head.d_fmt = lctime_head.d_t_fmt + strlen(info_tab[D_T_FMT]) + 1;
	lctime_head.t_fmt = lctime_head.d_fmt + strlen(info_tab[D_FMT]) + 1;

	lctime_head.day_1 = lctime_head.t_fmt + strlen(info_tab[T_FMT]) + 1;
	lctime_head.day_2 = lctime_head.day_1 + strlen(info_tab[DAY_1]) + 1;
	lctime_head.day_3 = lctime_head.day_2 + strlen(info_tab[DAY_2]) + 1;
	lctime_head.day_4 = lctime_head.day_3 + strlen(info_tab[DAY_3]) + 1;
	lctime_head.day_5 = lctime_head.day_4 + strlen(info_tab[DAY_4]) + 1;
	lctime_head.day_6 = lctime_head.day_5 + strlen(info_tab[DAY_5]) + 1;
	lctime_head.day_7 = lctime_head.day_6 + strlen(info_tab[DAY_6]) + 1;

	lctime_head.abday_1 = lctime_head.day_7 +
				strlen(info_tab[DAY_7]) + 1;
	lctime_head.abday_2 = lctime_head.abday_1 +
				strlen(info_tab[ABDAY_1]) + 1;
	lctime_head.abday_3 = lctime_head.abday_2 +
				strlen(info_tab[ABDAY_2]) + 1;
	lctime_head.abday_4 = lctime_head.abday_3 +
				strlen(info_tab[ABDAY_3]) + 1;
	lctime_head.abday_5 = lctime_head.abday_4 +
				strlen(info_tab[ABDAY_4]) + 1;
	lctime_head.abday_6 = lctime_head.abday_5 +
				strlen(info_tab[ABDAY_5]) + 1;
	lctime_head.abday_7 = lctime_head.abday_6 +
				strlen(info_tab[ABDAY_6]) + 1;

	lctime_head.mon_1 = lctime_head.abday_7 + strlen(info_tab[ABDAY_7]) + 1;
	lctime_head.mon_2 = lctime_head.mon_1 + strlen(info_tab[MON_1]) + 1;
	lctime_head.mon_3 = lctime_head.mon_2 + strlen(info_tab[MON_2]) + 1;
	lctime_head.mon_4 = lctime_head.mon_3 + strlen(info_tab[MON_3]) + 1;
	lctime_head.mon_5 = lctime_head.mon_4 + strlen(info_tab[MON_4]) + 1;
	lctime_head.mon_6 = lctime_head.mon_5 + strlen(info_tab[MON_5]) + 1;
	lctime_head.mon_7 = lctime_head.mon_6 + strlen(info_tab[MON_6]) + 1;
	lctime_head.mon_8 = lctime_head.mon_7 + strlen(info_tab[MON_7]) + 1;
	lctime_head.mon_9 = lctime_head.mon_8 + strlen(info_tab[MON_8]) + 1;
	lctime_head.mon_10 = lctime_head.mon_9 + strlen(info_tab[MON_9]) + 1;
	lctime_head.mon_11 = lctime_head.mon_10 + strlen(info_tab[MON_10]) + 1;
	lctime_head.mon_12 = lctime_head.mon_11 + strlen(info_tab[MON_11]) + 1;

	lctime_head.abmon_1 = lctime_head.mon_12 +
				strlen(info_tab[MON_12]) + 1;
	lctime_head.abmon_2 = lctime_head.abmon_1 +
				strlen(info_tab[ABMON_1]) + 1;
	lctime_head.abmon_3 = lctime_head.abmon_2 +
				strlen(info_tab[ABMON_2]) + 1;
	lctime_head.abmon_4 = lctime_head.abmon_3 +
				strlen(info_tab[ABMON_3]) + 1;
	lctime_head.abmon_5 = lctime_head.abmon_4 +
				strlen(info_tab[ABMON_4]) + 1;
	lctime_head.abmon_6 = lctime_head.abmon_5 +
				strlen(info_tab[ABMON_5]) + 1;
	lctime_head.abmon_7 = lctime_head.abmon_6 +
				strlen(info_tab[ABMON_6]) + 1;
	lctime_head.abmon_8 = lctime_head.abmon_7 +
				strlen(info_tab[ABMON_7]) + 1;
	lctime_head.abmon_9 = lctime_head.abmon_8 +
				strlen(info_tab[ABMON_8]) + 1;
	lctime_head.abmon_10 = lctime_head.abmon_9 +
				strlen(info_tab[ABMON_9]) + 1;
	lctime_head.abmon_11 = lctime_head.abmon_10 +
				strlen(info_tab[ABMON_10]) + 1;
	lctime_head.abmon_12 = lctime_head.abmon_11 +
				strlen(info_tab[ABMON_11]) + 1;

	lctime_head.am_str = lctime_head.abmon_12 +
				strlen(info_tab[ABMON_12]) + 1;
	lctime_head.pm_str = lctime_head.am_str +
				strlen(info_tab[AM_STR]) + 1;

	lctime_head.year_unit = lctime_head.pm_str +
				strlen(info_tab[PM_STR]) + 1;
	lctime_head.mon_unit = lctime_head.year_unit +
				strlen(info_tab[YEAR_UNIT]) + 1;
	lctime_head.day_unit = lctime_head.mon_unit +
				strlen(info_tab[MON_UNIT]) + 1;
	lctime_head.hour_unit = lctime_head.day_unit +
				strlen(info_tab[DAY_UNIT]) + 1;
	lctime_head.min_unit = lctime_head.hour_unit +
				strlen(info_tab[HOUR_UNIT]) + 1;
	lctime_head.sec_unit = lctime_head.min_unit +
				strlen(info_tab[MIN_UNIT]) + 1;

	lctime_head.era_fmt = lctime_head.sec_unit +
				strlen(info_tab[SEC_UNIT]) + 1;
	lctime_head.era_count = era_num;
	lctime_head.era_names = lctime_head.era_fmt +
				strlen(info_tab[ERA_FMT]) + 1;
	lctime_head.era_addr = lctime_head.era_names + era_len2;

	pad_byte = (4 - (lctime_head.era_addr % 4)) % 4;/* make sure era_addr starts */ 
	lctime_head.era_addr += pad_byte;		/* at word boundary. */
	/* Posix 11.2 */
	lctime_head.t_fmt_ampm = lctime_head.era_addr + era_len;

	/* The line commented  is buildlang code
	cmlog[LC_TIME].cm[cur_mod].size = lctime_head.era_addr + era_len; 
	 */
        cmlog[LC_TIME].cm[cur_mod].size = lctime_head.t_fmt_ampm +
					    strlen(info_tab[T_FMT_AMPM]) + 1;
	cmlog[LC_TIME].cm[cur_mod].pad =
			(4 - (cmlog[LC_TIME].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_TIME].cm[cur_mod].size += cmlog[LC_TIME].cm[cur_mod].pad;

	if (cmlog[LC_TIME].cm[cur_mod].size > _LC_TIME_SIZE)
		fprintf(stderr,(catgets(catd,NL_SETN,21, "LC_TIME category exceeds the maximum size allowed\n")));
}

/* write_locale:
** write the "locale.inf" file
*/
void
write_locale()
{
	extern void plc_all();
	extern void plc_coll();
	extern void plc_ctype();
	extern void plc_mntry();
	extern void plc_nmrc();
	extern void plc_time();

	FILE *fp, *tfp, *fopen();
	int i, j, k;
	char ch;

  	/*
  	** create the "locale.inf" file
  	*/ 
  	if ((fp = fopen(FILENAME, "w")) == NULL)
		fprintf(stderr,(catgets(catd,NL_SETN,22, "can't create 'locale.inf' file\n")));
  
  	/*
  	** write out the LOCALE file header section
  	*/
	for (i = 0; i < sizeof(lctable_head); i++)
	{
		(void) fprintf(fp, "%c", ((char *)&lctable_head)[i]);
		}

  	/*
  	** write out the CATEGORY/MODIFIER catinfo structure section
  	*/
	for (i = 0; i < N_CATEGORY; i++)
		for (j = 0; j < sizeof(struct catinfotype); j++)
			(void) fprintf(fp, "%c", ((char *)&catinfo[i])[j]);
	for (i = N_CATEGORY; i < (N_CATEGORY+lctable_head.mod_no); i++)
		for (j = 0; j < sizeof(struct catinfotype); j++)
			(void) fprintf(fp, "%c", ((char *)&catinfo[i])[j]);

	/*
	** write out the LC_ALL section (header & data)
	*/
	tfp = cmlog[LC_ALL].cm[0].tmp;
	if (cmlog[LC_ALL].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_all(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_ALL].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_COLLATE section (header & data)
	*/
	tfp = cmlog[LC_COLLATE].cm[0].tmp;
	if (cmlog[LC_COLLATE].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_coll(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_COLLATE].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_CTYPE section (header & data)
	*/
	tfp = cmlog[LC_CTYPE].cm[0].tmp;
	if (cmlog[LC_CTYPE].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_ctype(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_CTYPE].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_MONETARY section (header & data)
	*/
	tfp = cmlog[LC_MONETARY].cm[0].tmp;
	if (cmlog[LC_MONETARY].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_mntry(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_MONETARY].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_NUMERIC section (header & data)
	*/
	tfp = cmlog[LC_NUMERIC].cm[0].tmp;
	if (cmlog[LC_NUMERIC].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_nmrc(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_NUMERIC].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_TIME section (header & data)
	*/
	tfp = cmlog[LC_TIME].cm[0].tmp;
	if (cmlog[LC_TIME].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_time(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_TIME].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

		/*
		** write out the LC_MESSAGES section (header & data)
		*/
		tfp = cmlog[LC_MESSAGES].cm[0].tmp;
		if (cmlog[LC_MESSAGES].n_mod > 0) {
		if (tfp == NULL)
	        	(void) plc_messages(fp, 0);
		else {
		  for (i = 0; i < cmlog[LC_MESSAGES].cm[0].size; i++) {
		    ch = getc(tfp);
	            (void) fprintf(fp, "%c", ch);
		   }
	       }
	}

	/*
	** write out the MODIFIER section (header & data)
	*/
	for (i = 0; i < N_CATEGORY; i++) {
		for (j = 1; j < cmlog[i].n_mod; j++) {
			tfp = cmlog[i].cm[j].tmp;
			if (tfp == NULL) {
				switch (i) {
				    case LC_ALL:
					(void) plc_all(fp, j);
					break;
				    case LC_COLLATE:
					(void) plc_coll(fp, j);
					break;
				    case LC_CTYPE:
					(void) plc_ctype(fp, j);
					break;
				    case LC_MONETARY:
					(void) plc_mntry(fp, j);
					break;
				    case LC_NUMERIC:
					(void) plc_nmrc(fp, j);
					break;
				    case LC_TIME:
					(void) plc_time(fp, j);
					break;
	                            case LC_MESSAGES:
					(void) plc_messages(fp,j);
					break;
				}
			}
			else {
				for (k = 0; k < cmlog[i].cm[j].size; k++) {
				    ch = getc(tfp);
				    (void) fprintf(fp, "%c", ch);
				}
			}
		}
	}

	fclose(fp);
}

