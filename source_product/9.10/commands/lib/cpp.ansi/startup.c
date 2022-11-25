/* $Revision: 70.16 $ */

/*
 * (c) Copyright 1989,1990,1991,1992 Hewlett-Packard Company, all rights reserved.
 *
 * #   #  ##  ##### ####
 * ##  # #  #   #   #
 * # # # #  #   #   ####
 * #  ## #  #   #   #
 * #   #  ##    #   ####
 *
 * Please read the README file with this source code. It contains important
 * information on conventions which must be followed to make sure that this
 * code keeps working on all the platforms and in all the environments in
 * which it is used.
 */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "support.h"
#include "file.h"

#ifdef __sun
#ifdef NLS
extern char * getenv();
#include <locale.h>
#endif
#else
extern char * getenv();
#endif

FILE *outfile;

#if defined(CXREF)
extern FILE *outfp;
extern int doCXREFdata;
#endif

boolean NoExitOnError;
boolean DebugOption;
boolean StaticAnalysisOption;
boolean NoWarningsOption;
boolean SuppressSynchronization;
boolean NLSOption;
boolean PassComments;
int num_file_params;
boolean ShowFiles;
int	FileNestLevel;

#if defined(HP_OSF) || defined(__sun)
boolean ShowDependents;
#endif /* HP_OSF */

#ifdef OPT_INCLUDE
boolean NoInclusionAvoidance;
#endif

#ifdef CPP_CSCAN
boolean K_N_R_Mode;		/* Do K&R comment processing if set */
#endif	

#ifdef hpe
#define DEFAULT_RECORD_SIZE "R512 "  /* Can override with the +C option */
#define DEFAULT_FILE_SIZE   "S7500"  /* Can override with the +F option */
int posix_mode;       /* Set if -x option is seen */
extern t_search_path *pre_search_path_list;
#endif

boolean C_Plus_Plus;

#ifdef APOLLO_EXT
char * UserSystype;
enum TokenState FirstToken;
#endif

startup(argc, argv)
int argc;
char *argv[];
{
	extern jmp_buf cpp_error_jmp_buf;

	/* If NoExitOnError is set by caller, then use cpp_error_jmp_buf
	 * to return here if a fatal error is encountered during startup. */
	if (NoExitOnError)
		if (setjmp (cpp_error_jmp_buf))
			return 0;

	/* If we're restarting on a new file, clear out allocated memory */
	temp_dealloc();
	perm_dealloc();
	set_time();

	/* Do all initialization required before processing a new file */
	init_charmap();
	init_define_module();
	init_error_module();
	init_if_module();
	init_file_module();
	init_substitute_module();

	parse_options(argc, argv);

	/* allocate the buffers needed by cpp.ansi */
	init_buffers();

	/* Search paths from -I options are held until now so that they
	 * can be given to add_search_path in increasing priority. */
	add_pre_search_paths();

	/* This is called after parse_options because -D and -U mark macros
	 * which will be set up in init_defines.  The reason for waiting is
	 * that all -U options take precedence over all -D options. */
	init_defines();

	/* Set up the pragma which should appear on the first line and
	 * the '#' line which should be the second line. */
	init_output_line();

	/* Indicate successful startup */
	return 1;
}

char charmap[ALFSIZ];

init_charmap()
{
char c,*p;

p = "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
while (c = *p++)
  charmap[c] |= (IDSTART | ID);
p = "0123456789";
while (c = *p++)
  charmap[c] |= ID;
p = "\"'";
while (c = *p++)
  charmap[c] |= QUOTE;
#if defined(APOLLO_EXT) /* || defined(HP_OSF) */
charmap['$'] |= ID;
#endif
}

#if defined(HP_OSF) || defined(__sun)
int linesize; /* room in stderr available for holding filenames */
#endif

parse_options(argc, argv)
int argc;
char *argv[];
{
	int c;
	extern char *optarg;
	extern int optind;
	char *ch_ptr;
	char optstr[30];
	char *source_file = "";
	char *source_path;
	int open_fout = 0;
	char cpp_out_open [35];
#ifdef hpe
	char *Cflag = 0;   /* Set if +C option is seen */
	char *Fflag = 0;   /* Set if +F option is seen */
#endif

#ifdef CPP_CSCAN
	boolean Set_std_defines = TRUE;
#endif	

	extern char *basename (/*char * */);	/* from error.c */
	char *execname = basename(argv[0]);

#ifdef HP_OSF
#	define BASEOPTSTR "AB:CD:EGhH:I:MPU:Zgp:vwy:$"
#else
#ifdef hpe
#	define BASEOPTSTR "AB:CD:EGhH:I:PU:Zgwx$"
#else
#ifdef __sun
#	define BASEOPTSTR "BCD:GhH:I:PRU:Y:gu:wy$" /* different meaning for -B*/
#else
#ifdef __aix
#	define BASEOPTSTR "GCD:I:PU:Zq:w"
#else
#ifdef APOLLO_EXT
#	define BASEOPTSTR "AB:CD:EGHI:PU:Zgw$"
#else
#	define BASEOPTSTR "AB:CD:EGhH:I:PU:Zgw$"
#endif /* APOLLO_EXT */
#endif /* __aix */
#endif /* __sun */
#endif /* hpe */
#endif /* HP_OSF */

	strcpy(optstr,BASEOPTSTR);

#ifdef APOLLO_EXT
	strcat(optstr,"t:T:Y:");
#else
#ifndef __sun
	strcat(optstr, "Y");
#endif
#endif

#ifdef OPT_INCLUDE
	strcat(optstr,"s");
#endif

#ifdef CXREF
	strcat(optstr,"F:");
#endif

	DebugOption = FALSE;
	StaticAnalysisOption = FALSE;
	NoWarningsOption = FALSE;
	SuppressSynchronization = FALSE;
	NLSOption = FALSE;
	PassComments = FALSE;
	strcpy (cpp_out_open, "w");
	num_file_params = 0;
	ShowFiles = FALSE;
	FileNestLevel = -1;

#if defined(HP_OSF) || defined(__sun)
	ShowDependents = FALSE;
#endif /* HP_OSF */

#ifdef OPT_INCLUDE
	NoInclusionAvoidance = FALSE;
#endif

#ifdef APOLLO_EXT
	UserSystype = NULL;
	FirstToken = is_first;
	default_dir_set = FALSE;
#endif

#ifdef hpe
	posix_mode = FALSE;
#endif

	/* Check to see if its C++ cpp */
#if defined(APOLLO_EXT) || defined(HP_OSF)
	C_Plus_Plus = (strncmp (execname, "c++", 3) == 0);
#else
	C_Plus_Plus = (strncmp (execname, "Cpp", 3) == 0);
#endif

#ifdef CPP_CSCAN
	K_N_R_Mode  = (strstr(execname, "ansi") == 0);

#ifdef NLS
#ifdef __sun
       {
	   /* For SUN, we turn on processing if LANG is set */
	   char * lang = getenv("LANG");

	   if (lang && (strcmp(lang, "C") != 0))
	   {
	       if (setlocale(LC_ALL, ""))
		   NLSOption = TRUE;
	       else
		   warning("Language \"%s\" not available, using default", lang);
	   }
       }
	
#endif /* __sun */
#endif /* NLS */

#endif /* CPP_CSCAN */

	outfile = stdout;
	optind = 1;

	while(optind < argc)
	{
		switch(c = getopt(argc, argv, optstr))
		{
#if defined(CXREF)
			case 'F':
				doCXREFdata = 1;
				/* open file for cxref data */
				if ((outfp = fopen (optarg, "w")) == NULL)
					fatal_cpp_error ("Can't open %s\n",optarg);
				break;
#endif
			case 'A':
				/* Used to be FullAnsiStandard option. */
				break;

			case 'B':
#ifdef __sun
				C_Plus_Plus = TRUE;
#else
				/* modify buffersize */
				if (optarg != 0)
					buffersize = atoi(optarg);
				if (buffersize == 0)
					{
					error("Invalid buffersize specified");
					buffersize = DEFAULT_BUFFER_SIZE;
					}
#endif /* __sun */
				break;

			case 'E':
				/* only run the preprocessor */
				break;
			case 'D':
				handle_pre_define(optarg);
				break;
			case 'U':
				handle_pre_undefine(optarg);
				break;
			case 'I':
				handle_pre_search_path(optarg);
				break;
#ifdef HP_OSF
			case 'v':
				print_version();
				break;
			case 'y':
				handle_sys_pre_search_path(optarg);
				break;
			case 'p':
				handle_pri_pre_search_path(optarg);
				break;
#endif /* HP_OSF */
#ifdef hpe
			case 'x':
				posix_mode = TRUE;
				break;
#endif
			case 'g':
				DebugOption = TRUE;
				break;
#ifdef __sun
			case 'y':
#endif
			case 'G':
				DebugOption = TRUE;
				StaticAnalysisOption = TRUE;
				break;
#ifdef APOLLO_EXT
			case 'H':
				ShowFiles = TRUE;
				break;
#else
			/* DTS # CLLbs00233: klee 6/8/92 
			 * change old -H option to -h and make -H option an 
			 * alias of -B for non-domain users.
			 */
			case 'H':
				/* modify buffersize */
				if (optarg != 0)
					buffersize = atoi(optarg);
				if (buffersize == 0)
					{
					error("Invalid buffersize specified");
					buffersize = DEFAULT_BUFFER_SIZE;
					}
				break;
			case 'h':
				ShowFiles = TRUE;
				break;
#endif
#if defined(HP_OSF) || defined(__sun)
			case 'M':
				ShowDependents = TRUE;
				break;
#endif /* HP_OSF || __sun */
#ifdef __sun
			case 'R':
				/* Recursive macros ... not implemented */
				break;
#endif /* __sun */
			case 'w':
				NoWarningsOption = TRUE;
				break;
			case 'P':
				SuppressSynchronization = TRUE;
				break;
			case 'C':
				PassComments = TRUE;
				break;
#ifdef __sun
			case 'p':
				/* Use only first 8 chars ... not implemented */
				/* Complain if text after #endif's, etc */
				break;

			case 'u':
				/* -undef is only known switch */
				Set_std_defines = FALSE;
				break;
#endif /* __sun */

#ifndef __sun
   /* 'Y' conflicts with another case below.  For SUN, we always */
   /* will have NLS support turned on.				 */
#ifdef NLS
			case 'Y':
				NLSOption = TRUE;
# ifndef hpe
				if(langinit(getenv("LANG")))
			    	warning("Language \"%s\" not available, using \"n-computer\"\n", getenv("LANG"));
# endif /* hpe */
				break;
#endif /* NLS */
#endif /* __sun */

#ifndef __sun
			case 'Z':
				C_Plus_Plus = TRUE;
				break;
#endif /* __sun */
#ifdef APOLLO_EXT
			case 't':
			case 'T':
				handle_systype (optarg);
				break;
#endif /* APOLLO_EXT */
#if defined(APOLLO_EXT) || defined(__sun)
			case 'Y':
                                add_search_path(optarg);
                                default_dir_set = TRUE;
                                /* If default dir is set more than once, last 
                                   one wins, due to the way the list 
                                   is set up */
				break;
#endif /* APOLLO_EXT || __sun */
#ifdef __aix
			case 'q':  /* we already know what language level */
				   /* from how we were called; also we're */
				   /* ignoring the -qDBCS option for the */
				   /* moment */
				break;
#endif
				
#ifdef OPT_INCLUDE
			case 's':
				NoInclusionAvoidance = TRUE;
				break;
#endif
			case '$': /* make $ an id character */
				charmap['$'] |= ID;
				break;
			case '?':
#ifdef CPP_CSCAN
				warning("Illegal option to cpp.cscan ignored");
#else
				error("Illegal option to cpp");
#endif
				break;
			case EOF:
				/* Ignore lone '-'. */
				if(strcmp(argv[optind], "-") == 0)
				{
					optind++;
					continue;
				}
#ifdef hpe
                                if (argv[optind][0] == '+')
                                {
                                   switch (argv[optind][1])
                                   {
                                      case 'C': Cflag = argv[optind] + 1;
                                                Cflag[0] = 'R';
                                                break;
                                      case 'F': Fflag = argv[optind] + 1;
                                                Fflag[0] = 'S';
                                                break;
                                      default : error("Illegal (plus) option to cpp\n");
                                                break;

                                   }
                      /* Must increment optind for next pass thru while loop. */
                                   optind++;
                                   continue;
                                }
#endif
				num_file_params++;
				if(num_file_params == 1)
				{
					source_file = argv[optind];
				}
				else if(num_file_params == 2)
				{
                                        /* optind is the position of the out
                                           put file in argv list. Save it! */
                                        open_fout = optind;
				}
				else
#ifdef CPP_CSCAN
					fatal_cpp_error("Too many arguments to cpp.cscan");
#else
					fatal_cpp_error("Too many arguments to cpp");
#endif
				optind++;
		}
	}

#ifdef hpe
	if ( posix_mode ) {
		t_search_path *p;
		/* handle_pre_search_path() could't add '/' to each name on the
		 * search_path_list for MPE/iX. Because cpp.ansi was still pars-
		 * ing the options and had no idea whether the posix_mode was on
		 * or not at that time. So we do the job here right after parsi-
		 * ing options.
		 */
		for ( p = pre_search_path_list;  p ; p = p->next )
			p->pathname[strlen(p->pathname)] = '/';

		/* cpp output is a byte stream file */
		strcat (cpp_out_open, " Bs S2000000000 ");
		}
	else
	{
		strcat(cpp_out_open, " V d1 ");
	        if (!SuppressSynchronization)
	           strcat(cpp_out_open, "D2 ");  /* D2 means open as temporary file */
	
	        if (Cflag)
	        {
	           strcat(cpp_out_open, Cflag);
	           strcat(cpp_out_open, " ");
	
	        }
	        else
	           strcat(cpp_out_open, DEFAULT_RECORD_SIZE);
	
	        if (Fflag)
	        {
	           strcat(cpp_out_open, Fflag);
	           strcat(cpp_out_open, " ");
	        }
	        else
	           strcat(cpp_out_open, DEFAULT_FILE_SIZE);
	}

#endif
	if(num_file_params == 0)
	{
		outfile = stdout;
		if(!start_file(TRUE,NULL))	/* open stdin as input file */
			fatal_cpp_error("Unable to open stdin");
	}
	else
	{
#if defined(HP_OSF) || defined(__sun)
		if (ShowDependents)
			{
			char *object_file;
			int len = strlen(source_file);

			object_file = perm_alloc(len+1+2); 
			/* we needs two extra bytes for the filename which 
			   is not ended with ".c" */
			strcpy(object_file,source_file);
			if( strcmp(object_file+len-2, ".c") == 0 )
				*(object_file+len-1) = 'o';
			else
				strcat(object_file,".o");
			fprintf(stderr, "%s :", object_file);
			linesize = LINESIZE - strlen(object_file) - 2;
			}
#endif /* HP_OSF */
		if(!start_file(FALSE,source_file))
			fatal_cpp_error("Unable to open source file '%s'", source_file);
	}

        if (open_fout)
           if ( (outfile = fopen(argv[open_fout], cpp_out_open)) == NULL)
              fatal_cpp_error("Unable to open output file '%s'", argv[open_fout]);

#ifdef CPP_CSCAN
       if (Set_std_defines)
       {
           /*  This CPP never sets any default defines, so the -undef 
               option really has no effect.  This assumes all the 
               compilers stuff out and -undef followed by -D<define>
               for everything that is appropriate.  As of 4.1.1 all
               sun compilers and gcc do this.  If not, the following
               set of defines might be added:

	       "unix";
	       "sun";
	       "sparc";
	       "sun4";
	       "sun4c";
           */
       }	       

#endif /* CPP_CSCAN */
	{
	char *val;
	extern boolean macrodebug;
	if ( (val = getenv("DEBUGCPPANSI")) && atoi(val) == 1 )
        	macrodebug = TRUE;
	else
        	macrodebug = FALSE;
	}
}

#ifdef HP_OSF

/*
**	print_version:  Print out version string and exit.
*/

print_version()
{
   printf ("\tcpp version: $Revision: 70.16 $\n");
   exit (0);
}

#endif /* HP_OSF */
