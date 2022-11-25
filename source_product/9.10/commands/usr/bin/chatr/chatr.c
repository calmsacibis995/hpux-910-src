static char *HPUX_ID = "@(#) $Revision: 70.3 $";

#include <ar.h>
#ifdef hp9000s800
#include <a.out.h>
#include <magic.h>
#include <shl.h>
#include <dl.h>
#else
#include <a.out.h>
#endif
#include <stdio.h>
#include <signal.h>

char *malloc();
#ifndef hp9000s800
char *tempnam();
#endif
int  interrupt();

int argc;	        /* global argument counter 	                 */
char **argv;	        /* global argument array                         */
int argv_index;		/* current index into argv[] array 	         */

int err_file;		/* number of files for which chatr failed	 */
int err_opt;		/* 0 = all options valid, 1 = invalid option	 */
int executable;		/* 0=not an executable file; 1=executable file   */
int archive;		/* 0=not an archive file; 1=archive file         */
int shared_library;	/* 0=not a shared library; 1=shared library      */
int modify;	        /* 0 = don't modify attributes, 1 = modify them  */
MAGIC magic_num;	/* file's magic number				 */
char arcmag[SARMAG+1];  /* buffer to hold archive magic numder           */
#ifdef hp9000s800
FILHDR filhdr;		/* object file header				 */
#else
struct exec filhdr;	/* object file header				 */
#endif
int set_demand;		/* -1 = don't change; 0 = mark file nondemand    */ 
			/* load; 1 = mark file demand load		 */
int set_shared;		/* -1 = don't change; 0 = mark code nonshared;	 */ 
			/* 1 = mark code shared				 */
#ifdef hp9000s300
int data_cache_mode;    /* 0 = no change, 1 = writethrough, 2 = copyback */
int stack_cache_mode;   /* 0 = no change, 1 = writethrough, 2 = copyback */
#endif

#ifdef hp9000s800
#define TRUE		1
#define FALSE		0

#define B_IMMEDIATE	0x1	  /* if -B immediate is specified             */
#define B_DEFERRED	0x2	  /* if -B deferred is specified              */
#define B_NONFATAL	0x4	  /* if -B nonfatal is specified              */
#define B_RESTRICTED	0x8	  /* if -B restricted is specified            */
				  /* if -B is specified                       */
#define B_SPECIFIED	(B_IMMEDIATE | B_DEFERRED | B_NONFATAL | B_RESTRICTED)

#define s_ENABLE	0x1	  /* user has enabled SHLIB_PATH usage        */
#define b_ENABLE	0x2	  /* user has enabled embedded path usage     */
#define s_DISABLE	0x4	  /* user has disabled SHLIB_PATH usage       */
#define b_DISABLE	0x8	  /* user has disabled embedded path usage    */
#define s_FIRST		0x10	  /* 1 = SHLIB_PATH first, 0 = embedded first */
#define s_SPECIFIED     (s_ENABLE | s_DISABLE) /* if +s specified             */
#define b_SPECIFIED     (b_ENABLE | b_DISABLE) /* if +b specified             */

unsigned int shlib_search_flags;  /* bit field to hold state for path search  */
unsigned int shlib_bind_flags;    /* bit field to hold state for binding      */

struct dl_header *dl_header;	  /* pointer to shared library dl header      */
struct shlib_list_entry *shlib_list; /* pointer to library list               */
char  *shlib_string_table;	  /* pointer to shared library strings        */
int dl_fileofs;	                  /* file offset of the dl header             */

struct shlib_lookup {		    /* structure for linked list of shared    */
    char *path;	                    /* libraries whose path lookup we want to */
    unsigned char dash_l_reference; /* change			              */
    struct shlib_lookup *next;
};
struct shlib_lookup *shlib_lookup_ptr; /* pointer to linked list of library   */
				       /* paths specified with -l and +l opts */
#endif

int silent;		/* 0 = print attribute values; 1 = don't print	 */ 
			/* attribute values				 */
FILE *orgfile;		/* stream ptr associated with file processing	 */
#ifndef hp9000s800
char *temp;		/* name of temporary file */
FILE *tempfile;		/* stream ptr associated with temporary file 	 */
#endif


/*  Purpose:  print and/or change following memory management attributes:*/
/*		1)  shared --> demand load				 */
/*		2)  demand load --> shared                               */
/*              Series 300 only:                                         */
/*                  3) Change caching mode for data and/or stack         */

#ifdef hp9000s300
#define CM_NOCHANGE     0
#define CM_WRITETHROUGH 1
#define CM_COPYBACK     2
#define CM_BOTH         3 /* Error! */
#endif

main(pargc, pargv)
int pargc;		/* number of command line arguments */
char **pargv;		/* pointer to array of command line arguments */
{
	char *cp;

	/* initialize global variables */
	argc = pargc;
	argv = pargv;
	argv_index = 0;
	err_file = 0;
	err_opt = 0;
	modify = 0;
	silent = 0;
	set_demand = -1;
	set_shared = -1;
#ifdef hp9000s300
	data_cache_mode = CM_NOCHANGE;
	stack_cache_mode = CM_NOCHANGE;
#endif
#ifdef hp9000s800
        shlib_search_flags = 0;
        shlib_bind_flags = 0;
	dl_header = NULL;
	shlib_list = NULL;
	shlib_string_table = NULL;
	shlib_lookup_ptr = NULL;
#endif

	/* check for correct argument count */
	if (argc < 2)
	{
		error("argument count", "");
		exit(255);
	}

	/* parse command line */

	while (++argv_index < argc)
	{
		cp = argv[argv_index];

		if (*cp == '-') 
		{
			do_option_minus(++cp);
	        }
		else if (*cp == '+')
		{
			do_option_plus(++cp);
	        }
		else 
		{
			break; /* end of options */
		}
	}

	if (set_shared + set_demand == 2)
	{
		error("conflicting options: -n and -q", "");
		exit ((argc) ? (argc) : 255);
	}

#ifdef hp9000s800
	if (shlib_search_flags & s_ENABLE && shlib_search_flags & s_DISABLE)
	{
		error("conflicting options:",
                      "+s, cannot both enable and disable");
		exit ((argc) ? (argc) : 255);
	}

	if (shlib_search_flags & b_ENABLE && shlib_search_flags & b_DISABLE)
	{
		error("conflicting options:",
                      "+b, cannot both enable and disable");
		exit ((argc) ? (argc) : 255);
	}
	if (shlib_bind_flags & B_SPECIFIED) 
        {
		if (shlib_bind_flags & B_IMMEDIATE && 
                    shlib_bind_flags & B_DEFERRED)
		{
			error("conflicting options:",
                              "-B, cannot have both immediate and deferred");
			exit ((argc) ? (argc) : 255);
	        }
		if (!(shlib_bind_flags & B_IMMEDIATE) && 
                    !(shlib_bind_flags & B_DEFERRED))
		{
			error("incorrect options:",
                             "-B, must specify at least immediate or deferred");
			exit ((argc) ? (argc) : 255);
	        }
	}
#endif

#ifdef hp9000s300
	if (data_cache_mode == CM_BOTH)
	{
		error("conflicting data cache mode: d and D", "");
		exit ((argc) ? (argc) : 255);
	}

	if (stack_cache_mode == CM_BOTH)
	{
		error("conflicting stack cache mode: s and S", "");
		exit ((argc) ? (argc) : 255);
	}
#endif

	/* If user supplied invalid option return with number of file */
	/* name arguments.  If no file name arguments return 255      */
	/* (max allowable return value)				      */
	if (err_opt) exit ((argc) ? (argc) : 255);

	/* check to see if any file name arguments */
	if (argv_index >= argc)
	{
		error("file names missing", "");
		exit(255);
	}
	
#ifndef hp9000s800
	/* generate tempfile name if necc. */
	if (modify) temp = tempnam(NULL, "chatr");
#endif
	
	/* setup interrupt signal handling */
	signal(SIGINT, interrupt);

	/* process each file name argument */

	for (;argv_index < argc; argv_index++)
	{
		/* open file   */
		orgfile = fopen(argv[argv_index], "r");
		if (orgfile == NULL)
		{
			error("cannot open file", argv[argv_index]);
			err_file++;
			continue;
		}
		
		/* determine file's attributes */
		if (get_attributes(argv[argv_index]))  /* if true, error encountered */
		{
			fclose(orgfile);
			err_file++;
			continue;
		}

		/* if not modifying file characteristics and if not */
		/* in silent mode then print file's attributes      */
		if (!modify && !silent) 
		{
			printf("%s: \n", argv[argv_index]);
			print_attributes();
		}
		else if (modify)
		{
			/* if not in silent mode then print current values */
			if (!silent)
			{
				printf("%s: \n", argv[argv_index]);
				printf("   current values: \n");
				print_attributes();
			}

			/* can only modify file's type if it is executable */
#ifdef hp9000s800
			if (!executable && !shared_library)
#else
			if (!executable)
#endif
			{
				error(argv[argv_index], "is not executable");
				fclose(orgfile);
				err_file++;
				continue;
			}

			/* modify program attributes according to options */
			if (mod_attributes(argv[argv_index]))
			{
				err_file++;  /* error encountered */
				continue;
			}

			/* If not in silent mode then print new values */
			if (!silent)
			{
				printf("   new values: \n");
				print_attributes();
			}
		}
		fclose(orgfile);
	}
	
	/* exit with number of unsuccessful file requests */
	exit(err_file);
}

/* Purpose: handle all options that begin with '-' */

int do_option_minus(cp)
char *cp;
{
    char *tmp_ptr;

#ifdef hp9000s800
    struct shlib_lookup *new_node;
#endif
		
#ifdef hp9000s300
    char *cm_ptr;
#endif

    while (*cp != '\0') 
    {
	switch(*cp)
    	{
	    case 'n':	/* make file "shared" */
		set_shared = 1;
		modify = 1;
		break;

	    case 'q':	/* make file demand loaded */
		set_demand = 1;
		modify = 1;
		break;

	    case 's':	/* set silent mode */
		silent = 1;
		break;
		
#ifdef hp9000s800
	    case 'B':  
		modify = 1;
		set_str_parm(*cp, cp +1, &tmp_ptr);
		if (!tmp_ptr) 
		{
		    err_opt = 1;
		    return;
		}
		if (strcmp(tmp_ptr, "immediate") == 0) 
		    shlib_bind_flags |= B_IMMEDIATE;
	        else if (strcmp(tmp_ptr, "deferred") == 0)
		    shlib_bind_flags |= B_DEFERRED;
	        else if (strcmp(tmp_ptr, "nonfatal") == 0)
		    shlib_bind_flags |= B_NONFATAL;
	        else if (strcmp(tmp_ptr, "restricted") == 0)
		    shlib_bind_flags |= B_RESTRICTED;
		else 
		{
		    error("invalid -B argument", tmp_ptr);
		    err_opt = 1;
		}
		return;
	    case 'l':	/* specify shared library path to be dynamic */
                modify = 1;
		set_str_parm(*cp, cp +1, &tmp_ptr);
		if (!tmp_ptr) 
		{
		    err_opt = 1;
		    return;
		}
		new_node = malloc(sizeof(struct shlib_lookup));
		if (!new_node) 
		{
		    error("cannot allocate memory","");
		    exit(255);
		} 
		new_node->path = tmp_ptr;
		new_node->dash_l_reference = TRUE;
	        new_node->next = shlib_lookup_ptr;
		shlib_lookup_ptr = new_node;
		return;
#endif
		
#ifdef hp9000s300
	    case 'C':
		set_str_parm(*cp, cp +1, &tmp_ptr);
		if (!tmp_ptr) 
		{
		    err_opt = 1;
		    return;
		}
		cm_ptr = tmp_ptr;
		while (*cm_ptr != '\0')
		{
		    switch(*cm_ptr)
		    {
		        case 'd':
			    data_cache_mode |= CM_WRITETHROUGH;
			    modify = 1;
			    break;

			case 'D':
			    data_cache_mode |= CM_COPYBACK;
			    modify = 1;
			    break;

			case 's':
			    stack_cache_mode |= CM_WRITETHROUGH;
			    modify = 1;
			    break;

			case 'S':
			    stack_cache_mode |= CM_COPYBACK;
			    modify = 1;
			    break;

			default:
			    error("invalid cache mode specification", tmp_ptr);
			    err_opt = 1;
			    break;
		    }

		    if (err_opt)
			break;

		    cm_ptr++;
		}

		return;
#endif

	    default:	/* oops */
		fprintf(stderr, "%s: illegal option -- %c\n", argv[0], *cp);
		err_opt = 1;
		break;
	}
	++cp;
    }
}

/* Purpose: handle all options that begin with '+' */

do_option_plus(cp)
char *cp;
{

#ifdef hp9000s800
    char *tmp_ptr;
    struct shlib_lookup *new_node;
#endif

    while (*cp != '\0') 
    {
	switch(*cp)
    	{
#ifdef hp9000s800
	    case 'b':	/* enable or disable search of the embedded path list */
                modify = 1;
		set_str_parm(*cp, cp +1, &tmp_ptr);
		if (!tmp_ptr) 
		{
		    err_opt = 1;
		    return;
		}
		if (strcmp(tmp_ptr, "enable") == 0) 
		    shlib_search_flags |= b_ENABLE;
	        else if (strcmp(tmp_ptr, "disable") == 0)
		    shlib_search_flags |= b_DISABLE;
		else 
		{
		    error("invalid +b argument", tmp_ptr);
		    err_opt = 1;
		}
		return;
	    case 'l':	/* specify shared library path to be static */
                modify = 1;
		set_str_parm(*cp, cp +1, &tmp_ptr);
		if (!tmp_ptr) 
		{
		    err_opt = 1;
		    return;
		}
		new_node = malloc(sizeof(struct shlib_lookup));
 		if (!new_node)
                {
                    error("cannot allocate memory","");
                    exit(255);
                }
		new_node->path = tmp_ptr;
		new_node->dash_l_reference = FALSE;
	        new_node->next = shlib_lookup_ptr;
		shlib_lookup_ptr = new_node;
		return;
	    case 's':   /* enable or disable search of SHLIB_PATH */
                modify = 1;
		set_str_parm(*cp, cp +1, &tmp_ptr);
		if (!tmp_ptr) 
		{
		    err_opt = 1;
		    return;
		}
		if ( !(shlib_search_flags & b_SPECIFIED) )/* +b not specified */
		    shlib_search_flags |= s_FIRST;
		if (strcmp(tmp_ptr, "enable") == 0) 
		    shlib_search_flags |= s_ENABLE;
	        else if (strcmp(tmp_ptr, "disable") == 0)
		    shlib_search_flags |= s_DISABLE;
		else 
		{
		    error("invalid +s argument", tmp_ptr);
		    err_opt = 1;
		}
		return;
#endif
	    default:	/* oops */
		fprintf(stderr, "%s: illegal option -- %c\n", argv[0], *cp);
		err_opt = 1;
		break;
	}
	++cp;
    }
}

/* Purpose: to set "arg" to the argument supplied for option "opt". 	*/
/*          The argument may be separated from the option by whitespace */

set_str_parm(opt, src, arg)
char opt;
char *src;
char **arg;
{
    if (*src == '\0') 
    {
	src = argv[++argv_index];
	if (argv_index >= argc) 
	{
	    fprintf(stderr, "%s: option requires an argument -- %c\n", 
		    argv[0], opt);
	    *arg = NULL;
	    return;
	}
    }
    *arg = src;
}
	
/*  Purpose:  get file's attributes */

get_attributes(file_name)
char *file_name;	/* pointer to current file name */
{

	executable = 0;	    /* start off as unrecognized file */
	archive = 0;
	shared_library = 0;

#ifdef hp9000s800
        /* seek to proper place to find magic number */
	fseek(orgfile, MAGIC_OFFSET, 0);
#else
	/* seek to beginning of the file and read in magic number */
	fseek(orgfile, 0, 0);
#endif
	if (fread(arcmag, 1, SARMAG, orgfile)
		!= SARMAG)
	{
		error("cannot read magic number in file", file_name);
		return(1);
	}
	if (strncmp(arcmag, ARMAG, SARMAG) == 0)
	{
		archive = 1;
		return(0);
	}
	/* seek to beginning of the file and read in magic number */
#ifdef hp9000s800
	fseek(orgfile, MAGIC_OFFSET, 0);
#else
	fseek(orgfile, 0, 0);
#endif
	if (fread((char *) &magic_num, 1, sizeof(magic_num), orgfile)
		!= sizeof(magic_num))
	{
		error("cannot read magic number in file", file_name);
		return(1);
	}

#ifdef hp9000s800
	if ( !_PA_RISC_ID(magic_num.system_id) )
#else
	if (magic_num.system_id != HP9000S200_ID)
#endif
	{
		error("unrecognized magic number in file", file_name);
		return(1);
	}

	if ((magic_num.file_type == EXEC_MAGIC) 
		|| (magic_num.file_type == DEMAND_MAGIC)
		|| (magic_num.file_type == SHARE_MAGIC))
	{
		executable = 1;
	}
	else if ((magic_num.file_type == RELOC_MAGIC) 
		|| (magic_num.file_type == AR_MAGIC))
	{
		return(0);
        }
	else if ((magic_num.file_type == SHL_MAGIC)
		|| (magic_num.file_type == DL_MAGIC))
	{
		shared_library = 1;
	}
	else
	{
		error("unrecognized magic number in file", file_name);
		return(1);
	}

	/* seek to beginning of file and read in object header */
	fseek(orgfile, 0, 0);
	if (fread((char *) &filhdr, 1, sizeof(filhdr), orgfile)
		!= sizeof(filhdr))
	{
		error("read object header error in file", file_name);
		return(1);
	}

#ifdef hp9000s800
	/* read in shared library tables */

	if (executable || shared_library) 
	{
		if (get_shared_library_info(file_name))
		        return(1);
	}
#endif

	return(0);
}

#ifdef hp9000s800

struct space_dictionary_record *get_space_info()
{
    struct space_dictionary_record      *space_ptr;

    space_ptr = (struct space_dictionary_record *) malloc (
	         filhdr.space_total * sizeof(struct space_dictionary_record));
    if (!space_ptr) {
        error("cannot allocate memory","");
        exit(255);
        }

    fseek(orgfile, filhdr.space_location, 0);
    fread(space_ptr, sizeof(struct space_dictionary_record),
	  filhdr.space_total, orgfile);

    return(space_ptr);
}
	
struct subspace_dictionary_record *get_subspace_info()
{
    struct subspace_dictionary_record   *subspace_ptr;

    subspace_ptr = (struct subspace_dictionary_record *) malloc (
	 filhdr.subspace_total * sizeof(struct subspace_dictionary_record));
    if (!subspace_ptr) {
        error("cannot allocate memory","");
        exit(255);
        }

    fseek(orgfile, filhdr.subspace_location, 0);
    fread(subspace_ptr, sizeof(struct subspace_dictionary_record),
          filhdr.subspace_total, orgfile);

    return(subspace_ptr);
}

int get_space_index(space_info_ptr, space_name)
struct space_dictionary_record *space_info_ptr;
char *space_name;
{
    int i;
    char *space_string_table;
    char *current_name;

    space_string_table = malloc(filhdr.space_strings_size);
    if (!space_string_table) {
        error("cannot allocate memory","");
        exit(255);
        }

    fseek(orgfile, filhdr.space_strings_location, 0);
    fread(space_string_table,filhdr.space_strings_size,1,orgfile);

    for (i=0; i<filhdr.space_total; i++) {
        current_name = space_string_table + space_info_ptr[i].name.n_strx;
        if (strcmp(space_name, current_name) == 0) {
            free(space_string_table);
            return(i);
	    }
        }

    free(space_string_table);
    return(-1);
}

get_shared_library_info(file_name)
char *file_name;
{
    int sub_index;
    int space_index;
    struct space_dictionary_record      *space_info_ptr;
    struct subspace_dictionary_record   *subspace_info_ptr;

    if (dl_header) 
	free(dl_header);
    if (shlib_list) 
	free(shlib_list);
    if (shlib_string_table)
	free(shlib_string_table);

    dl_header = shlib_list = shlib_string_table = NULL;

    space_info_ptr = get_space_info();
    subspace_info_ptr = get_subspace_info();

    if ( (space_index = get_space_index(space_info_ptr, "$TEXT$")) == -1) 
        return(1);

    sub_index = space_info_ptr[space_index].subspace_index;

    dl_header = malloc(sizeof(struct dl_header));
    if (!dl_header) {
        error("cannot allocate memory","");
        exit(255);
        }

    dl_fileofs = subspace_info_ptr[sub_index].file_loc_init_value;

    fseek(orgfile, dl_fileofs, 0);
    fread((char *)dl_header, sizeof(struct dl_header), 1, orgfile);

    if (dl_header->hdr_version != DL_HDR_VERSION_ID) {
	free(dl_header);
	dl_header = NULL;
	return(0);		/* no shared libraries used */
	}

    shlib_string_table = malloc(dl_header->string_table_size);
    if (!shlib_string_table) {
        error("cannot allocate memory","");
        exit(255);
        }

    fseek(orgfile, subspace_info_ptr[sub_index].file_loc_init_value
	  +dl_header->string_table_loc, 0);
    fread(shlib_string_table, dl_header->string_table_size, 1, orgfile);

    shlib_list = malloc( sizeof(struct shlib_list_entry) * 
			 dl_header->shlib_list_count);
    if (!shlib_list) {
        error("cannot allocate memory","");
        exit(255);
        }

    fseek(orgfile, subspace_info_ptr[sub_index].file_loc_init_value
	  +dl_header->shlib_list_loc, 0);
    fread(shlib_list, sizeof(struct shlib_list_entry), 
	  dl_header->shlib_list_count, orgfile);

    free(space_info_ptr);
    free(subspace_info_ptr);

    return(0);
}

#endif

/*  Purpose:  print file's attributes */

print_attributes()
{
	/* print magic number */
	if (archive) printf("         archive file \n");
	else
	{
	    switch (magic_num.file_type)
	    {
		case RELOC_MAGIC:
			printf("         relocatable \n");
			break;
		case EXEC_MAGIC:
			printf("         normal executable \n");
			break;
		case SHARE_MAGIC:
			printf("         shared executable \n");
			break;
		case DEMAND_MAGIC:
			printf("         demand load executable \n");
			break;
		case SHL_MAGIC:
			printf("         shared library \n");
			break;
		case DL_MAGIC:
			printf("         dynamic load library \n");
			break;
		case AR_MAGIC:
			printf("         ***OLD*** archive file \n");
			printf("Warning: use arcv to convert to new format\n");
			break;
		default:
			printf("         unknown file type \n");
	    }

#ifdef hp9000s300
	    if (executable)
	    {
		printf("         caching modes:\n");
		if ((filhdr.a_miscinfo & M_DATA_WTHRU) == 0)
		    printf("             data:  copyback\n");
		else
		    printf("             data:  writethrough\n");

		if ((filhdr.a_miscinfo & M_STACK_WTHRU) == 0)
		    printf("             stack: copyback\n");
		else
		    printf("             stack: writethrough\n");
	    }
#endif
#ifdef hp9000s800
	    if (dl_header)
	    {
		int i;

		if (executable) {

		  printf("         shared library dynamic path search:\n");
		  printf("             SHLIB_PATH     %-9s %-7s\n",
		    dl_header->flags & SHLIB_PATH_ENABLE ? "enabled":"disabled",
		    dl_header->flags & SHLIB_PATH_FIRST  ? "first"  :"second");
		  printf("             embedded path  %-9s %-7s",
		    dl_header->flags & EMBED_PATH_ENABLE ? "enabled":"disabled",
		    dl_header->flags & SHLIB_PATH_FIRST  ? "second" :"first");
	          printf("%s\n", dl_header->embedded_path ? 
	            shlib_string_table+dl_header->embedded_path:"Not Defined");
		}

		if (dl_header->shlib_list_count) {

		  printf("         shared library list:\n");
		  for (i=0; i<dl_header->shlib_list_count; i++) 
                  {
		     printf("             %-9s %s\n",
		       shlib_list[i].dash_l_reference ? "dynamic" : "static",
		       shlib_list[i].shlib_name+shlib_string_table);
		  }
		}  	

		if (executable) {
		  printf("         shared library binding:\n");
		  printf("             %s%s%s\n",
		    shlib_list[0].bind & BIND_DEFERRED ? "deferred " : 
				                         "immediate ",
		    shlib_list[0].bind & BIND_NONFATAL ? "nonfatal " : "",
		    shlib_list[0].bind & BIND_RESTRICTED ? "restricted " :"");
		}	

	    } /* if (dl_header) */
#endif
	}

	/* flush stdout so that it does not interfere with stderr */
	fflush(stdout);
}

#ifdef hp9000s800

/*  Purpose:  change file's attributes */

mod_attributes(file_name)
char *file_name;	/* pointer to current file name */
{
	int i;
	int found;
	struct shlib_lookup *ptr;

	/* open file to be modified */
	if ( (orgfile = fopen(file_name, "r+")) == NULL)
	{
		error("cannot open  file for writing", file_name);
		return(1);
	}

	if (set_demand == 1)  {
	  /* check if file is shared */
	  if (magic_num.file_type != SHARE_MAGIC) {
	    error("only shared-code files can be made demand loadable","");
	    return(1);
	  }
	    filhdr.a_magic = magic_num.file_type = DEMAND_MAGIC;
	}
	else if (set_shared == 1)  {
	  /* check if file is shared */
	  if (magic_num.file_type != DEMAND_MAGIC) {
	    error("only demand loaded files can be made shared","");
	    return(1);
	  }
	  filhdr.a_magic = magic_num.file_type = SHARE_MAGIC;
	} 

	if (dl_header) {
	  if (executable) {
	    if (shlib_search_flags & s_ENABLE) 
	      dl_header->flags |= SHLIB_PATH_ENABLE;
	    else if (shlib_search_flags & s_DISABLE) 
	      dl_header->flags &= ~SHLIB_PATH_ENABLE;

	    if (shlib_search_flags & b_ENABLE) {
	      if (!dl_header->embedded_path) {
		error("+b enable, embedded path not defined at link time","");
	        return(1);
	      }
	      dl_header->flags |= EMBED_PATH_ENABLE;
            }
	    else if (shlib_search_flags & b_DISABLE) 
	      dl_header->flags &= ~EMBED_PATH_ENABLE;

	    /* if both +s and +b were specified, use s_FIRST setting.        */
	    /* if either +s and +b were specified, default is embedded first */

	    if ( (shlib_search_flags & s_SPECIFIED) &&
	         (shlib_search_flags & b_SPECIFIED) ) 
	      if (shlib_search_flags & s_FIRST) 
	        dl_header->flags |= SHLIB_PATH_FIRST;
	      else
	        dl_header->flags &= ~SHLIB_PATH_FIRST;
	    else if (shlib_search_flags & (s_SPECIFIED | b_SPECIFIED))
	      dl_header->flags &= ~SHLIB_PATH_FIRST;

	    if (shlib_bind_flags & B_SPECIFIED) {
	      shlib_list[0].bind = 0;
	      if (shlib_bind_flags & B_IMMEDIATE)	
		shlib_list[0].bind |= BIND_IMMEDIATE;
	      if (shlib_bind_flags & B_DEFERRED)	
		shlib_list[0].bind |= BIND_DEFERRED;
	      if (shlib_bind_flags & B_NONFATAL)	
		shlib_list[0].bind |= BIND_NONFATAL;
	      if (shlib_bind_flags & B_RESTRICTED)	
		shlib_list[0].bind |= BIND_RESTRICTED;
	    }

	  } /* if executable */

	  if (executable || shared_library) {
	    for (ptr = shlib_lookup_ptr; ptr != NULL; ptr = ptr->next) {
	      found = FALSE;
	      for (i = 0; i < dl_header->shlib_list_count; i++) {
	        if (strcmp(ptr->path, 
		  	   shlib_list[i].shlib_name+shlib_string_table) == 0) {
		  if (i == 0 && executable) {
		    error("cannot change dynamic path search for program --", 
			  ptr->path);
		    return(1);
		  }
	          shlib_list[i].dash_l_reference = ptr->dash_l_reference;
		  found = TRUE;
		  /* should not be duplicates, but we will loop just for fun */
	        }
	      }
	      if (!found) {
	        error("shared library path not found in the list --",ptr->path);
	        return(1);
	      }
	    }  
	  } /* if (executable || shared_library) */

	  fseek(orgfile, dl_fileofs, 0);
	  if (fwrite((char *) dl_header, 1, sizeof(struct dl_header), orgfile)
		  != sizeof(struct dl_header))
	  {
		  error("write shared library header error in file", file_name);
		  return(1);
	  }

	  fseek(orgfile, dl_fileofs + dl_header->shlib_list_loc, 0);
	  i = sizeof(struct shlib_list_entry) * dl_header->shlib_list_count;
	  if (fwrite((char *) shlib_list, i, 1, orgfile) != 1) 
	  {
		  error("write shared library list error in file", file_name);
		  return(1);
	  }

	} /* if dl_header */
     
	fseek(orgfile, 0, 0);
	if (fwrite((char *) &filhdr, 1, sizeof(filhdr), orgfile)
		!= sizeof(filhdr))
	{
		error("write object header error in file", file_name);
		return(1);
	}

	fclose(orgfile);
	return(0);
}

#else

/*  Purpose:  change file's attributes */

mod_attributes(file_name)
char *file_name;	/* pointer to current file name */
{
	if (set_demand == 1) return (shared_to_demand(file_name));
	if (set_shared == 1) return (demand_to_shared(file_name));
	return(mod_cache_mode(file_name));
}


/*  Purpose:  change file from shared to demand-loaded */

shared_to_demand(file_name)
char *file_name;	/* pointer to current file name */
{
	int x, here;

	/* check cache mode conflict & set proper bits in a_miscinfo field */

	if (cm_check())
	    return(1);

	/* check if file is shared */
	if (magic_num.file_type != SHARE_MAGIC)
	{
		error("only shared-code files can be made demand loadable","");
		return(1);
	}
	magic_num.file_type = DEMAND_MAGIC;

	/* open temporary file */
	if ( (tempfile = fopen(temp, "w+")) == NULL)
	{
		error("cannot open temporary file for writing", temp);
		return(1);
	}

	filhdr.a_magic.file_type = DEMAND_MAGIC;
	fseek(tempfile, 0, 0);
	if (fwrite((char *) &filhdr, 1, sizeof(filhdr), tempfile)
		!= sizeof(filhdr))
	{
		error("write object header error in file", file_name);
		return(1);
	}

	fseek(tempfile, TEXTPOS, 0);
	fseek(orgfile, sizeof(filhdr), 0);
	dcopy(orgfile, tempfile, filhdr.a_text, file_name);
	fseek(tempfile, DATAPOS, 0);
	dcopy(orgfile, tempfile, filhdr.a_data, file_name);
	x = filhdr.a_lesyms + filhdr.a_supsym;
	if (x == 0)
	{
		char null = 0;
		fseek(tempfile, MODCALPOS -1, 0);
		fwrite(&null, sizeof(null), 1, tempfile);
	}
	else
	{
		fseek(tempfile, LESYMPOS, 0);
		dcopy(orgfile, tempfile, x, file_name);
	}

	/* copy any remaining junk (debug info, etc) */

	here = ftell(orgfile);
	fseek(orgfile,0L,SEEK_END);

	x = ftell(orgfile) - here;
	fseek(orgfile,here,SEEK_SET);

	if (x)
		dcopy(orgfile,tempfile,x,file_name);
	if (update_extensions(SHARE_MAGIC,DEMAND_MAGIC))
		return(1);

	fclose(orgfile);
	fclose(tempfile);
	if (fcopy(temp, file_name))
	{
		error("copy error from temporary file to original file:", file_name);
		return(1);
	}

	return(0);
}


/*  Purpose:  change file from demand-loaded to shared */

demand_to_shared(file_name)
char *file_name;	/* pointer to current file name */
{
	int x, here;

	/* check cache mode conflict & set proper bits in a_miscinfo field */

	if (cm_check())
	    return(1);

	/* check if file is shared */
	if (magic_num.file_type != DEMAND_MAGIC)
	{
		error("only demand loaded files can be made shared","");
		return(1);
	}
	magic_num.file_type = SHARE_MAGIC;

	/* open temporary file */
	if ( (tempfile = fopen(temp, "w+")) == NULL)
	{
		error("cannot open temporary file for writing", temp);
		return(1);
	}

	filhdr.a_magic.file_type = SHARE_MAGIC;
	fseek(tempfile, 0, 0);
	if (fwrite((char *) &filhdr, 1, sizeof(filhdr), tempfile)
		!= sizeof(filhdr))
	{
		error("write object header error in file", file_name);
		return(1);
	}

	/* ugly way to use XXXPOS macros */
	filhdr.a_magic.file_type = DEMAND_MAGIC;
	fseek(orgfile, TEXTPOS, 0);
	dcopy(orgfile, tempfile, filhdr.a_text, file_name);
	fseek(orgfile, DATAPOS, 0);
	dcopy(orgfile, tempfile, filhdr.a_data, file_name);
	x = filhdr.a_lesyms + filhdr.a_supsym;
	if (x != 0)
	{
		fseek(orgfile, LESYMPOS, 0);
		dcopy(orgfile, tempfile, x, file_name);
	}
	/* copy any remaining junk (debug info, etc) */

	here = ftell(orgfile);
	fseek(orgfile,0L,SEEK_END);

	x = ftell(orgfile) - here;
	fseek(orgfile,here,SEEK_SET);

	if (x)
		dcopy(orgfile,tempfile,x,file_name);
	if (update_extensions(DEMAND_MAGIC,SHARE_MAGIC))
		return(1);

	fclose(orgfile);
	fclose(tempfile);
	if (fcopy(temp, file_name))
	{
		error("copy error from temp to original", file_name);
		return(1);
	}

	return(0);
}


/* update_extensions - trudge through extension headers and fix links */

update_extensions (fromtype, totype)
int fromtype, totype;
{
	struct header_extension ext;
	long text_adjust, data_adjust, other_adjust;
	int oldtype;

	fseek(tempfile,0L,SEEK_SET);
	fread(&ext,sizeof ext,1,tempfile);
	if (!ext.e_extension)
		return(0);
	oldtype = filhdr.a_magic.file_type;
	filhdr.a_magic.file_type = totype;
	text_adjust = TEXTPOS;
	data_adjust = DATAPOS;
	other_adjust = MODCALPOS;
	filhdr.a_magic.file_type = fromtype;
	text_adjust -= TEXTPOS;
	data_adjust -= DATAPOS;
	other_adjust -= MODCALPOS;
	do
	{
		if (ext.e_extension >= TEXTPOS)
		{
			if (ext.e_extension < DATAPOS)
				ext.e_extension += text_adjust;
			else if (ext.e_extension < MODCALPOS)
				ext.e_extension += data_adjust;
			else
				ext.e_extension += other_adjust;
			fseek(tempfile,-(sizeof ext),SEEK_CUR);
			fwrite(&ext,sizeof ext,1,tempfile);
		}
		fseek(tempfile,ext.e_extension,SEEK_SET);
		fread(&ext,sizeof ext,1,tempfile);
		if (ext.e_header == DEBUG_HEADER)
		{
			ext.e_spec.debug_header.header_offset += other_adjust;
			ext.e_spec.debug_header.gntt_offset += other_adjust;
			ext.e_spec.debug_header.lntt_offset += other_adjust;
			ext.e_spec.debug_header.slt_offset += other_adjust;
			ext.e_spec.debug_header.vt_offset += other_adjust;
			ext.e_spec.debug_header.xt_offset += other_adjust;
			fseek(tempfile,-(sizeof ext),SEEK_CUR);
			fwrite(&ext,sizeof ext,1,tempfile);
		}
	} while (ext.e_extension);
	filhdr.a_magic.file_type = oldtype;
	return(0);
}

mod_cache_mode(file_name)
char *file_name;	/* pointer to current file name */
{
	/* check cache mode conflict & set proper bits in a_miscinfo field */

	if (cm_check())
	    return(1);

	fclose(orgfile);
	if ( (orgfile = fopen(file_name, "r+")) == NULL)
	{
		error("cannot open  file for writing", file_name);
		return(1);
	}

	/* seek to beginning of file and write  object header */

	fseek(orgfile, 0, 0);
	if (fwrite((char *) &filhdr, 1, sizeof(filhdr), orgfile)
		!= sizeof(filhdr))
	{
		error("write object header error in file", file_name);
		return(1);
	}

	return(0);
}

/* cm_check - Check for a conflict in cache mode specification. */
/*            If none, then set/clear  appropriate bits in      */
/*            a_miscinfo field of filhdr.                       */

cm_check()
{
    int cur_mode;

    /* Check data */

    if ((filhdr.a_miscinfo & M_DATA_WTHRU) == 0)
	cur_mode = CM_COPYBACK;
    else
	cur_mode = CM_WRITETHROUGH;

    if (data_cache_mode == CM_NOCHANGE)
	data_cache_mode = cur_mode;
    else {
	if (data_cache_mode == cur_mode) {
	    if (cur_mode == CM_COPYBACK)
		error("data cache mode is already copyback","");
	    else
		error("data cache mode is already writethrough","");
	    return(1);
	}
    }

    /* Check stack */

    if ((filhdr.a_miscinfo & M_STACK_WTHRU) == 0)
	cur_mode = CM_COPYBACK;
    else
	cur_mode = CM_WRITETHROUGH;

    if (stack_cache_mode == CM_NOCHANGE)
	stack_cache_mode = cur_mode;
    else {
	if (stack_cache_mode == cur_mode) {
	    if (cur_mode == CM_COPYBACK)
		error("stack cache mode is already copyback","");
	    else
		error("stack cache mode is already writethrough","");
	    return(1);
	}
    }

    /* Set/Clear appropriate bits */

    if (data_cache_mode == CM_WRITETHROUGH)
	filhdr.a_miscinfo |= M_DATA_WTHRU;
    else
	filhdr.a_miscinfo &= ~M_DATA_WTHRU;

    if (stack_cache_mode == CM_WRITETHROUGH)
	filhdr.a_miscinfo |= M_STACK_WTHRU;
    else
	filhdr.a_miscinfo &= ~M_STACK_WTHRU;

    return(0);
}

/* dcopy -      Copy input to output, checking for errors */

dcopy(in, out, count,file_name)
register FILE *in, *out;
register long count;
char *file_name;
{
        register int c;

        if (count < 0) error("internal error: bad count to dcopy", "");
        while(count--)
        {
                if ((c = getc(in)) == EOF)
                {
                        if (feof(in)) error("premature end of file", file_name);
                        if (ferror(out)) error("write to temp file", "");
		}
                putc(c, out);
        }
}

/*  Purpose:  copy temp file to original file */

fcopy(temp, org)
char *temp;
char *org;
{
	register int c;

	if ((orgfile = fopen(org, "w")) == NULL)
	{
		error("cannot open file for writing:", org);
		goto bad;
	}
	if ((tempfile = fopen(temp, "r")) == NULL)
	{
		error("cannot open temporary file for reading:", temp);
		goto bad;
	}
	fseek(tempfile, 0, 0);
	while ((c = getc(tempfile)) != EOF)
	{
		putc(c, orgfile);
		if (ferror(orgfile))
		{
			error("write error in file", org);
			goto bad;
		}
	}
	if (ferror(tempfile))
	{
		error("read error on temporary file", temp);
		goto bad;
	}

	fclose(orgfile);
	fclose(tempfile);
	unlink(temp);
	return(0);

bad:	
	if (orgfile != NULL) fclose(orgfile);
	if (tempfile != NULL) fclose(tempfile);
	unlink(temp);
	return(1);
}
#endif

/*  Purpose:  interrupt signal handler */

interrupt()
{
#ifndef hp9000s800
	if (tempfile != NULL)
	{
		fclose(tempfile);
		tempfile = NULL;
		unlink(temp);
	}
#endif
	if (orgfile != NULL) 
	{
		fclose(orgfile);
		orgfile = NULL;
	}

	exit(255);
}

#ifndef hp9000s800
/*  Purpose:  print warning message */

warning(str1, str2)
char *str1, *str2;
{
	fprintf(stderr, "chatr:(warning) - %s %s \n", str1, str2);
} 
#endif

/*  Purpose:  print error message */

error(str1, str2)
char *str1, *str2;
{
	fprintf(stderr, "chatr:(error) - %s %s \n", str1, str2);
}

