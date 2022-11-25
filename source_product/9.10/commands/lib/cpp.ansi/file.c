/* $Revision: 72.1 $ */

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

#if defined(APOLLO_EXT) || defined(CPP_CSCAN)
boolean default_dir_set;
#endif

#ifdef hpe
#include <string.h>                   /* to use string manipulation functions */
#include "/libcxl/libc/include/errno.h"
#define CPP_IN_OPEN "r T R1024"
static int search_inc_file();
extern int posix_mode;
#endif

extern boolean NoExitOnError;
extern boolean DebugOption;
extern boolean StaticAnalysisOption;
extern boolean SuppressSynchronization;
extern boolean ShowFiles;
#if defined(HP_OSF) || defined(__sun)
extern boolean ShowDependents;
#endif /* HP_OSF */
extern int FileNestLevel;
#ifdef OPT_INCLUDE
extern boolean NoInclusionAvoidance;
#endif
extern int current_lineno;
static int num_lines_to_finish_file;

#if defined(CXREF)
extern int  doCXREFdata; /* control data production */
#endif

/* The initial value of output_line is changed in init_output_line to be the
 * header stuff that needs to appear as the first output. */
char *output_line;
static int output_lineno;
static char *output_filename;
#define MAX_SYNC_LINES 5

/* DTS # CLLbs00124. klee 920310
 * Maximum of levels of nested include files.
 */
#define MAX_NESTED_LEVELS (_NFILE - 5)

/* This keeps track of the number of lines read during each call to get_sub_line. */
int num_newlines_in_output;

/* This points to a ^B or ^C sequence which passes information about macro names
 * in '#' directives to consumer tools. */
char *directive_macro_info;

/* If set, this function pointer is used to notify the caller of cpp
 * that a file is about to be opened. If the function result is non-zero,
 * it means the caller already has info for the file and it need not be
 * processed. */
int (*notify_file)();

/* If set, this function pointer is used to notify the caller of cpp
 * that an included directive was just processed. */
int (*notify_include)();

#ifdef OPT_INCLUDE
/*
 * Structure to hold a binary tree sorted on file name for recovering
 * data from previous file opens.  We use this to hold knowledge about
 * whether there is an enclosing #if which allows us not to open a file
 * in the first place.
 */

typedef struct file_tree {
	char *filename;
	int avoidable:1;
	int if_type:2;
	char *condition;
	struct file_tree *left, *right;
} t_file_tree;

static t_file_tree *known_files;	/* top node of file data tree */

static char *copy_string(str)
char *str;
{
	char *new_str;

	new_str = perm_alloc(strlen(str) + 1);
	/* klee 920303. Make sure not to dereference a NULL pointer. 
	 * Mike Beckmann found a bug in cpp.ansi ( if use the -z option to 
	 * build cpp.ansi ).
	 */
	if ( str )
		strcpy(new_str, str);
	else
		strcpy(new_str, "");
	return new_str;
}

/*
 * Generic tree scanning routine, which looks for a matching entry for
 * the given file name.  If one is found, it returns the data pointer;
 * otherwise it returns NULL.  If "place_for_new_entry" is not NULL, it
 * is filled in with a pointer to the pointer which should get a value
 * associated with "filename".
 */
t_file_tree *search_in_tree(filename, place_for_new_entry)
char *filename;
t_file_tree ***place_for_new_entry;
{
	t_file_tree	*tp, **next_tp;
	int		cmp_val;

	next_tp = &known_files;
	for (tp = known_files; *next_tp != NULL; tp = *next_tp) {
		cmp_val = strcmp(filename, tp->filename);
		if (cmp_val == 0)
			return tp;
		else if (cmp_val < 0)
			next_tp = &tp->left;
		else
			next_tp = &tp->right;
	}
	if (place_for_new_entry != NULL)
		*place_for_new_entry = next_tp;
	return NULL;
}


/*
 * Return TRUE if we've read the file already and don't need to read
 * it again.
 */
char *condition_buffer;
boolean already_read(filename)
char *filename;
{
	t_file_tree *file;

	if (NoInclusionAvoidance)
		return FALSE;

	file = search_in_tree(filename, NULL);
	if (file == NULL || !file->avoidable)
		return FALSE;

	switch (file->if_type) {
	  case ENCLOSING_IF:
		/* get_if_value modifies the buffer so we operate on a copy */
		strcpy(condition_buffer, file->condition);
		return !get_if_value(condition_buffer);
	  case ENCLOSING_IFDEF:
		return get_define(file->condition, strlen(file->condition)) == NULL;
	  case ENCLOSING_IFNDEF:
		return get_define(file->condition, strlen(file->condition)) != NULL;
	}
	return FALSE;
}

/*
 * Save the data associated with the current file for future use
 * (assuming it isn't already there, of course).
 */
save_file_data()
{
	t_file_tree	*tp, **next_tp;
	t_file_tree	*new_tp;
	int		cmp_val;

	if (search_in_tree(current_file->filename, &next_tp) != NULL)
		return;

	new_tp = (t_file_tree *) perm_alloc(sizeof *tp);
	new_tp->filename = copy_string(current_file->filename);
	new_tp->avoidable = current_file->avoidable;
	new_tp->if_type = current_file->if_type;
	if (new_tp->avoidable)
		new_tp->condition = copy_string(current_file->condition);
	else
		new_tp->condition = NULL;
	new_tp->left = NULL;
	new_tp->right = NULL;

	*next_tp = new_tp;
}
#endif

/* This is the start of a linked list of files which are the
 * current nested include files starting with the file input
 * currently comes from and ending with the original source. */
t_include_file *current_file;

#ifdef HP_OSF
/* To provide the OSF 'incl_private:path' and 'incl_sys:path' options, we create
 * two lists of directory paths to search for include files. The lists
 * 'pri_search_path_list' and 'sys_search_path_list' are used by the options
 * 'incl_private:path' and 'incl_sys:path' respectively. The last directory on 
 * the two lists is the standard include directory. Directories before it in
 * 'sys_search_path_list' are those added by '-I' and/or '-y' command line 
 * options. '-y' is similar to the -I option, but only use the supplied
 * path to search for header files specified with brackets. Directories before 
 * it in 'pri_search_path_list' are those added by '-I' and/or '-p' command 
 * line options. '-p' is similar to the -I option, but only use the supplied
 * path to search for header files specified with double quotes.
 */
static t_search_path *sys_search_path_list;
static t_search_path *sys_pre_search_path_list;
static t_search_path *pri_search_path_list;
static t_search_path *pri_pre_search_path_list;
#else
/* This is the list of directory paths to search for include files.
 * The last directory on the list is the standard include directory.
 * Directories before it are those added by '-I' command line options. */
#ifdef hpe
t_search_path *search_path_list;
t_search_path *pre_search_path_list;
#else
static t_search_path *search_path_list;
static t_search_path *pre_search_path_list;
#endif
#endif /* HP_OSF */

/*
 * This routine starts the given include file by opening it for input and
 * adding it to the 'current_file' linked list.
 */
boolean start_file(filename_is_stdin,filename)
boolean filename_is_stdin;
char *filename;
{
	t_include_file *file;
	FILE *fp;
	boolean is_include_file;
	char *get_file_date();

	/* If there's already a file open, then this must be an include file
	   (used below) */
	is_include_file = (current_file != NULL);

#ifdef OPT_INCLUDE
	/* If file would expand to nothing, don't bother opening it. */
	if (already_read(filename))
		goto found_file;
#endif
	/* If we're embedded in some other application which already has data
	   for this file, no need to open it either. */
	if (notify_file && (*notify_file)(filename))
		goto show_dependents;
#ifdef hpe
        if (num_file_params > 0)
        {
           fp = fopen(filename, "r T");
           if (fp == NULL)
              return FALSE;
        }
#else
	if (filename_is_stdin) 
	{
	   fp = stdin;
	   filename = "";
	}
	else
	{
	   fp = fopen(filename, "r");
	   if(fp == NULL)
	      return FALSE;
	}
#endif
	FileNestLevel++;
	if (ShowFiles)
		fprintf(stderr, "%*s%s\n", FileNestLevel*4, "", filename);

	file = NEW(t_include_file);
	file->fp = fp;
	file->filename = copy_string(filename);
	if(DebugOption)
		file->filedate = get_file_date(filename);
	else
		file->filedate = NULL;
	file->lineno = 0;
	num_lines_to_finish_file = current_lineno-output_lineno+1;
	current_lineno = file->lineno+1;
#ifdef OPT_INCLUDE
	file->avoidable = TRUE;	/* every file is avoidable until proven otherwise */
	file->inside_if = FALSE;
	file->seen_if   = FALSE;
	file->condition = NULL;
#endif
	file->parent = current_file;
	current_file = file;

	/* 920131 vasta: added the labels and moved this code down so that it is
	   executed even if the file I/O gets optimized out; presumably one would
	   still want correct dependency and xref info generated.
	   NOTE: use filename instead of current_file->filename, since current_file
	   will not refer to the right file if I/O has been optimized away. */

	/* If this is an included file and someone wants to know about it,
	 * then let them know (only if not already notified by notify_file). */

found_file:
	if (notify_include && is_include_file)
		(*notify_include) (filename);

show_dependents:

#if defined(HP_OSF) || defined(__sun)
	if (ShowDependents)
		{
		extern int linesize;
		int len = strlen(filename);
		/* the last 4 bytes reserved for a backslash and a newline */
		if (linesize - 1 - 4 >= len) /* enough room left */
			{
			fprintf(stderr, " %s", filename);
			linesize = linesize - 1 - len;
			}
		else
			{
			fprintf(stderr, " \\\n  %s", filename);
			linesize = LINESIZE - 2 - len; /* reset the linesize */
			}
		}
#endif /* HP_OSF */

#if defined(CXREF) && !defined(REF_ONLY)
	if(doCXREFdata)
		xfname(filename);
#endif

	return TRUE;
}


/*
 * This routine ends the current include file by closing it and
 * removing it from the 'current_file' linked list.
 */
boolean end_file()
{
	FileNestLevel--;
	if(current_file && current_file->fp)
		fclose(current_file->fp);
	if(current_file == NULL || current_file->parent == NULL)
	{
		check_if_ifs_are_ended();
		return FALSE;
	}
#ifdef OPT_INCLUDE
	save_file_data();
#endif
	current_file = current_file->parent;
	num_lines_to_finish_file = current_lineno-output_lineno;
	current_lineno = current_file->lineno+1;
#if defined(CXREF) && !defined(REF_ONLY)
	if(doCXREFdata)
		xfname(current_file->filename);
#endif
	return TRUE;
}

/*
 * This routine returns number of levels of nested include files.
 */
int levels_of_nested_include(p)
t_include_file *p;
{
	int i;
	for (i=0; p; i++) p = p->parent;
	return (i);
}

#ifdef HP_OSF
/*
 * This routine handles a '#include' directive.  The given 'line' points
 * to the first character following the '#include'.  The string pointed
 * to by 'line' is expected to represent an include file.  If the file is
 * in quotes, then it is searched for starting with the directory
 * that the source file is in and continuing with the dircetories
 * in 'pri_search_path_list'.  If the file is in angle brackets, then the search
 * starts with the directories in 'sys_search_path_list' without searching the
 * directory that the source file is in.  If neither of these forms is matched
 * then the string is macro substituted and the two forms are tried for again.
 */
#else
/*
 * This routine handles a '#include' directive.  The given 'line' points
 * to the first character following the '#include'.  The string pointed
 * to by 'line' is expected to represent an include file.  If the file is
 * in quotes, then it is searched for starting with the directory
 * that the source file is in and continuing with the dircetories
 * in 'search_path_list'.  If the file is in angle brackets, then the search
 * starts with the directories in 'search_path_list' without searching the
 * directory that the source file is in.  If neither of these forms is matched
 * then the string is macro substituted and the two forms are tried for again.
 */
#endif /* HP_OSF */
#ifdef hpe
#define INC_FILE_QUOTED     TRUE             /* Indicates that include file is
                                                in quotes.  */
#define START_SEARCH_PATH   TRUE             /* Indicates that it has exhausted
                                                places to search except for
                                                the ones in search path list.
                                                Now, start using search path
                                                list.  */
#endif

handle_include(line)
register char *line;
{
	register char *ch_ptr;
	register char *filename;
#ifdef hpe
        static char inc_base_name[27];
        static char inc_group[10];
        static char inc_account[10];
        static char src_account[10];
        static char src_group[10];
        static char temp_group[10];
        static char temp_account[10];
        static void get_src_group_account();
        static void get_inc_name_group_account();
        extern char *src_group_account_ptr;
#endif
	register char *pathname;
	register char *current_filename;
	register t_search_path *search_path;
	char *parse_filename();
	char *get_sub_line();


	filename = NULL;
	skip_space(line);
	if(*line == '"')
		filename = parse_filename(line+1, '"');
	else if(*line == '<')
		filename = parse_filename(line+1, '>');
	/* If filename is NULL then 'line' didn't match either acceptable form,
	 * so macro substitute and try again. */
	if(filename == NULL)
	{
		/* Create directive_macro_info in case any macros are in the include. */
		if(StaticAnalysisOption)
		{
			ch_ptr = line;
			while(*ch_ptr != '\n')
				ch_ptr++;
			directive_macro_info = temp_alloc(10+2*(ch_ptr-line)+2);
			*directive_macro_info = DIRECTIVE_START;
			strcpy(directive_macro_info+1, "#include ");
			mark_macros(directive_macro_info+10, line, ch_ptr-line);
		}
		line = get_sub_line(line);
		skip_space(line);
		if(*line == '"')
			filename = parse_filename(line+1, '"');
		else if(*line == '<')
			filename = parse_filename(line+1, '>');
	}
	/* Was the include file found? */
	if(filename == NULL)
	{
		error("Bad syntax for #include directive");
		return;
	}
#ifndef hpe
/* If the filename is absolute then look for it at its absolute location. */
#if defined(APOLLO_EXT)
	/* A filename starting with "$(" is taken to be absolute; Domain/OS will
	   expand the environment variable */
	if(*filename == '/' || (filename[0] == '$' && filename[1] == '('))
#else
	if(*filename == '/')
#endif /* APOLLO_EXT */
	{
		if(start_file(FALSE,filename))
			return;
	}
	else
	{
		/* Look for the include relative to the directory of the current file
		 * if the include file is in quotes. */
		if(*line == '"')
		{
			current_filename = current_file->filename;
			ch_ptr = current_filename+strlen(current_filename);
			while(ch_ptr > current_filename && *(ch_ptr-1) != '/')
				ch_ptr--;
			pathname = perm_alloc((ch_ptr-current_filename)+strlen(filename)+1);
			strncpy(pathname, current_filename, ch_ptr-current_filename);
			strcpy(pathname+(ch_ptr-current_filename), filename);
			if(start_file(FALSE,pathname))
				return;
		}
		/* Look in the directories in the search path. */
#ifdef HP_OSF
		if(*line == '"')
			search_path = pri_search_path_list;
		else
			search_path = sys_search_path_list;
#else
		search_path = search_path_list;
#endif /* HP_OSF */
		while(search_path != NULL)
		{
			pathname = perm_alloc(strlen(filename)+strlen(search_path->pathname)+1);
			strcpy(pathname, search_path->pathname);
			strcpy(pathname+strlen(search_path->pathname), filename);
			if(start_file(FALSE,pathname))
				return;
			search_path = search_path->next;
		}
	}
	/* DTS # CLLbs00124. klee 920310
	 * Check the number of levels of nested include files. If more than
	 * the maximum then issues an error message.
	 */
	if ( levels_of_nested_include(current_file) >= MAX_NESTED_LEVELS )
		error("Unreasonable include nesting");
	else
	/* end DTS # CLLbs00124 */
		error("Unable to find include file '%s'", filename);
#else /* hpe */
   if( posix_mode )
   { 
   /* This is a POSIX ( hirarchical ) filename on MPE/iX. Apply the
    * HP-UX version of searching algorithm.
    */

/* If the filename is absolute then look for it at its absolute location. */
	if(*filename == '/')
	{
		if(start_file(FALSE,filename))
			return;
	}
	else
	{
		/* Look for the include relative to the directory of the current file
		 * if the include file is in quotes. */
		if(*line == '"')
		{
			current_filename = current_file->filename;
			ch_ptr = current_filename+strlen(current_filename);
			while(ch_ptr > current_filename && *(ch_ptr-1) != '/')
				ch_ptr--;
			if ( *current_filename != '.' && *current_filename != '/' )
				/* the file name must be MPE-escaped syntax */
				{
				pathname = perm_alloc((ch_ptr-current_filename)+strlen(filename)+1+2);
				sprintf(pathname,"./");
				strncpy(pathname+2, current_filename, ch_ptr-current_filename);
				strcpy(pathname+2+(ch_ptr-current_filename), filename);
				}
			else
				{
				pathname = perm_alloc((ch_ptr-current_filename)+strlen(filename)+1);
				strncpy(pathname, current_filename, ch_ptr-current_filename);
				strcpy(pathname+(ch_ptr-current_filename), filename);
				}
			if(start_file(FALSE,pathname))
				return;
		}
		/* Look in the directories in the search path. */
#ifdef HP_OSF
		if(*line == '"')
			search_path = pri_search_path_list;
		else
			search_path = sys_search_path_list;
#else
		search_path = search_path_list;
#endif /* HP_OSF */
		while(search_path != NULL)
		{
			pathname = perm_alloc(strlen(filename)+strlen(search_path->pathname)+1);
			strcpy(pathname, search_path->pathname);
			strcpy(pathname+strlen(search_path->pathname), filename);
			if(start_file(FALSE,pathname))
				return;
			search_path = search_path->next;
		}
	}

        if ( levels_of_nested_include(current_file) >= MAX_NESTED_LEVELS )
           error("Unreasonable include nesting");
        else
           error("Unable to find include file '%s'", filename);

   } /* POSIX filename on MPE/iX */
   else 
   /* else filename is in classic MPE syntax - i.e. filename.group.account.
    * Use the following algorithm to search for include file.
    */
   {

/* This is the algorithm cpp uses to search for include file on classic MPE/XL:
 * 
 * cpp strips all prefixes, and
 *
 * 1. If the include file is in angle brackets: just use the base name of the
 *    of the include file and search for it in the H group of the SYS account.
 *    If the file is not found there, an error is issued.
 *
 * 2. If the include file is in quotation marks and
 * CLLbs00096   open file with the name given as is.  This will allow MPE
 *              file equations to take effect.                 pkwan 920220
 *    A. If the file name HAS NO group/account:
 *       a. Search for the file in the group and account of the source file.
 *          If there are NO source group and account, logon group and account
 *          will be used by default.
 *       b. If (a) fails, search for the file in H group of the source account.
 *          Again, if there is no source account, logon account will be used by
 *          default.
 *       c. If (b) fails, search for the file in the H group of the SYS
 *          account.
 *       d. If (c) failes, issue error message.
 *
 *    B. If the file name HAS group and possibly account:
 *       a. Search for the file using file name as is, if it's fully qualified.
 *          If the file name ONLY HAS group, use source account.
 *       b. If (a) fails, search for the file in the source group of the source
 *          account.
 *       c. If (b) fails, search for the file in the H group of the source
 *          account.
 *       d. If (c) fails, search for the file in the H group of the SYS
 *          account.
 *       e. If (d) failes, issue error message.
 *
 */

        get_inc_name_group_account(filename, inc_base_name, inc_group, inc_account);
        if ((strcmp(src_group_account_ptr, "")) != 0)     /* CLLca01481 */
           get_src_group_account(src_group, src_account);


        if (*line == '<')
        {
        /* Search for file in group "h" of account "sys" by using the default
           search path list.
         */

           if (search_inc_file(inc_base_name, "", "", !INC_FILE_QUOTED,
                               START_SEARCH_PATH))
              return;
           if ( levels_of_nested_include(current_file) >= MAX_NESTED_LEVELS )
              error("Unreasonable include nesting");
           else
              error("Unable to find include file '%s'", inc_base_name);
        }
        else if (*line == '"')
        {
           /* CLLbs00096 begin: search as if file name was within quotes */
           if (search_inc_file(inc_base_name, inc_group, inc_account,
               INC_FILE_QUOTED, !START_SEARCH_PATH))
              return;
           /* CLLbs00096 end */

           if (inc_group[0] == '\0')
           {

           /* Include file name HAS NO group/account.  Search file using
              name as read + group & account of source file if the source
              file name has group/account otherwise just use the file name.
              The first postition of the src_group or src_account contains
              a null character if there is NO source group or source account.
            */

              if (search_inc_file(inc_base_name, src_group, src_account,
                                  INC_FILE_QUOTED, !START_SEARCH_PATH))
                 return;

              /* When start_search_path is set to TRUE, search_inc_file
                will do another search using group ".h" and src_account.  Then
                if that fails, it will start using search path list.
              */
              if (search_inc_file(inc_base_name, ".h", src_account,
                                  INC_FILE_QUOTED, START_SEARCH_PATH))
                 return;
              if ( levels_of_nested_include(current_file) >= MAX_NESTED_LEVELS )
                 error("Unreasonable include nesting");
              else
                 error("Unable to find include file '%s'", inc_base_name);
           } /* end of handling include file name with NO group/account */
           else
           {
              /* include file name HAS group and possibly HAS account */

              if ( inc_group[0] != '\0' && inc_account[0] == '\0' )
              {
                 /* include file name HAS group but NO account */
                 strcpy(temp_group, inc_group);
                 strcpy(temp_account, src_account);
              }
              else if ( inc_group[0] != '\0' && inc_account[0] != '\0' )
              {
                 /* include file name HAS group and account  */
                 strcpy(temp_group, inc_group);
                 strcpy(temp_account, inc_account);
              }

              /* Search include file using file name as read if it fully     */
              /* qualified.  If include file only HAS group, use source      */
              /* source account.                                             */

              if (search_inc_file(inc_base_name, temp_group, temp_account,
                                  INC_FILE_QUOTED, !START_SEARCH_PATH))
                 return;

              /* Couldn't find it.  Now, if the source file name has group   */
              /* and account then append them to the include base file name. */
              /* Otherwise, just use the include base file name to search    */
              /* (using logon group/account).                                */

              if (search_inc_file(inc_base_name, src_group, src_account,
                                  INC_FILE_QUOTED, !START_SEARCH_PATH))
                 return;

              /* Could'nt find it again.  Now, search file using filename */
              /* + .h + .source account                                   */

              if (search_inc_file(inc_base_name, ".h", src_account,
                                  INC_FILE_QUOTED, START_SEARCH_PATH))
                 return;
              if ( levels_of_nested_include(current_file) >= MAX_NESTED_LEVELS )
                 error("Unreasonable include nesting");
              else
                 error("Unable to find include file '%s'", inc_base_name);
           } /* end of handling file name with group and possible account */
         } /* else if (*line == '"') */
   }
#endif /* hpe */
}

#ifdef hpe
/* This routine parses and returns the include file's base name, group,
 * and account.  If the include file does not have group/account, the group/
 * account strings will be returned with the first characters being null.
 */
static void get_inc_name_group_account(filename, inc_base_name,
                                       inc_group, inc_account)
char *filename;
char inc_base_name[];
char inc_group[];
char inc_account[];
{

   char *filename_ptr_1;
   char *filename_ptr_2;
   int i;



   /* First save the address of the file name.  Then search in file name
    * for the slash.  If the slash is found, adjust the saved address to
    * point to the character after the slash.  If the slash is not found,
    * just use the saved address (as is) to extract include file name,
    * group, and account.
    */

   filename_ptr_1 = filename;
   filename_ptr_2 = strrchr(filename, '/');
   i = 0;

   if (filename_ptr_2 != NULL)
   {
      filename_ptr_1 = filename_ptr_2;
      filename_ptr_1++;
   }

   inc_base_name[i++] = *filename_ptr_1++;
   while ( (*filename_ptr_1 != '.') && (*filename_ptr_1 != '\0') )
      inc_base_name[i++] = *filename_ptr_1++; /* extract base name */
   inc_base_name[i] = '\0';

   if (*filename_ptr_1 != '\0')
   {
      i = 0;
      inc_group[i++] = *filename_ptr_1++;
      while ( (*filename_ptr_1 != '.') && (*filename_ptr_1 != '\0') )
         inc_group[i++] = *filename_ptr_1++; /* extract group */
      inc_group[i] = '\0';

      if (*filename_ptr_1 != '\0')
      {
         i = 0;
         inc_account[i++] = *filename_ptr_1++;
         while ( (*filename_ptr_1 != '.') && (*filename_ptr_1 != '\0') )
            inc_account[i++] = *filename_ptr_1++; /* extract account */
         inc_account[i] = '\0';
      }
      else
         inc_account[0] = '\0';
   }
   else
      inc_group[0] = '\0';
}


/* This routine parses and returns the group and account of the source file.
 * If the source file does not have group/account, the group/account strings
 * will be returned with the null characters in the first positions.
 */
static void get_src_group_account(src_group, src_account)
char src_group[];
char src_account[];
{
   extern char *src_group_account_ptr;
   int i;

   if (src_group_account_ptr != NULL)
   {
      i = 0;
      src_group[i++] = *src_group_account_ptr++;
      while ((*src_group_account_ptr != '.') && (*src_group_account_ptr != '\0'))
         src_group[i++] = *src_group_account_ptr++;
      src_group[i] = '\0';

      if (*src_group_account_ptr != '\0')
      {
         i = 0;
         src_account[i++] = *src_group_account_ptr++;
         while ((*src_group_account_ptr != '.') && (*src_group_account_ptr != '\0'))
            src_account[i++] = *src_group_account_ptr++;
         src_account[i] = '\0';
      }
      else
         src_account[0] = '\0';
   }
   else
   {
      src_group[0] = '\0';
      src_account[0] = '\0';
   }
}

/* This routine receives from the calling procedure the base name of the
 * include file, a group name, and a account name.  It then puts the name,
 * group, and account together and use that to search for the file.  When
 * it failed to look for the file in places sent to it, it will start looking
 * in the search path list ( places from -I options ).
 */
static int search_inc_file(inc_base_name, group, account, inc_file_quoted,
                           start_search_path)
char inc_base_name[];
char group[];
char account[];
boolean inc_file_quoted;
boolean start_search_path;
{
   static char pathname[27];
   t_search_path *search_path;

   /* If the include file is in quotes, first search for it in the group and
      account received from the calling program.  The code following this "if"
      is executed one more time when start_search_path becomes TRUE the first
      time.
    */
   if (inc_file_quoted)
   {
      strcpy(pathname, inc_base_name);
      if (group[0] != '\0')
      {
         /* group/account includes a dot + group/account name. So, there's no
            need to append the dot
          */
         strcat(pathname, group);
         if (account[0] != '\0')
            strcat(pathname, account);
      }
      if (start_file(FALSE,pathname))
         return(1);
   }

   /* If failed to find the include file in the group and account of the
      source file and other places other than the search path list, then start
      using the search path list which is built from -I options.  This is ind-
      cated by the flag start_search_path = TRUE.  The h.sys group/account is
      ALWAYS on this list and is the last place to be searched.  If the include
      file is in angle brackets, ONLY the search path list (code below this
      comment) be used.
    */

   if (start_search_path)
   {
#ifdef HP_OSF
      if (inc_file_quoted)
          search_path = pri_search_path_list;
      else
          search_path = sys_search_path_list;
#else
      search_path = search_path_list;
#endif /* HP_OSF */
      while(search_path != NULL)
      {
         strcpy(pathname, inc_base_name);
         strcat(pathname, ".");
         strcat(pathname, search_path->pathname);
         if(start_file(FALSE,pathname))
            return(1);
         search_path = search_path->next;
      }

      /* If it ever gets here, it fails.  */
      return(0);
   }
}
#endif /* #ifdef hpe */

#ifdef HP_OSF
/*
 * To support DSEE builders on OSF machines we have to allow
 * for environment variables of the form "$(X)" at the beginning
 * of a filename string. Unlike Domain, the OS doesn't do the
 * expansion, so we do it ourselves.
 *
 * If "$(X)" appears at the beginning of the string, then X is assumed
 * to be an environment variable and is recursively expanded. If X is not
 * in the environment, NULL is returned. If no environment variable occurs
 * in the string, a null-terminated copy of the string is returned.
 */
static char *expand_envars(line, length)
char *line;
int length;
{
	char *filename;
	char *ev_name;
	char *end_ev_name;
	int  ev_name_length;
	char *ev_value;

	while (line[0] == '$' && line[1] == '(')
	{
		line   += 2; /* point past "$(" */
		length -= 2;

		/* figure out the length of the variable name */
		for (ev_name_length = 0; ev_name_length < length; ++ev_name_length)
			if (line[ev_name_length] == ')')
				break;

		if (ev_name_length == 0 || ev_name_length >= length)
			return NULL;

		/* make a null-terminated copy of the name and look it up */
		ev_name = temp_alloc (ev_name_length + 1);
		strncpy (ev_name, line, ev_name_length);
		ev_name[ev_name_length] = '\0';

		if (!(ev_value = getenv (ev_name)))
			return NULL;

		/* build a new filename with the expanded variable text */
		line   += ev_name_length + 1; /* point past "...)" */
		length -= ev_name_length + 1;
		filename = temp_alloc (strlen (ev_value) + length + 1);
		strcpy (filename, ev_value);
		strncat (filename, line, length);
		line   = filename;
		length = strlen (line);
	}

	/* make a permanent copy of the final string */
	filename = perm_alloc (length + 1);
	strncpy (filename, line, length);
	filename[length] = '\0';
	return filename;
}
#endif /* HP_OSF */

/*
 * This routine returns a filename parsed out of the given 'line' and
 * terminated by the 'delimiter'.  When the routine is called, 'line'
 * points to the character immediately following the first delimiter.
 * If the filename doesn't parse properly then NULL is returned.
 */
char *parse_filename(line, delimiter)
char *line;
char delimiter;
{
	register char *line_ptr;
	register int length;
	char *filename;

	line_ptr = line; 
	while(*line_ptr != delimiter && *line_ptr != '\n')
	{
		/* If the character is an NLS character pair then skip over the pair to
		 * avoid the second character being taken as the delimiter. */
#ifdef NLS
		if(NLSOption && FIRSTof2((unsigned char)*line_ptr) && SECof2((unsigned char)*(line_ptr+1)))
			line_ptr++;
#endif
		line_ptr++;
	}
	length = line_ptr-line;
	/* Was filename terminated by 'delimiter' rather than end of line. */
	if(*line_ptr == delimiter)
	{
		line_ptr++;
		/* Make sure there are no other non-whitspace characters on line
		 * after filename. */
		skip_space(line_ptr);
		if(*line_ptr == '\n')
		{
#ifdef HP_OSF
			filename = expand_envars (line, length);
#else
			filename = perm_alloc(length+1);
			strncpy(filename, line, length);
			filename[length] = '\0';
#endif
			return filename;
		}
	}
	return NULL;
}


/*
 * This routine handles the '#line' directive.  The 'line' parameter points
 * to the first character following the '#line'.  The following string must
 * contain a line number and an optional filename.  The preprocessors assumed
 * current line number and filename are set to those values specified by the
 * '#line' directive.
 */
handle_line(line)
register char *line;
{
	register int lineno;
	register char *ch_ptr;
	register char *filename;
	
	register char *pathname;
	char *parse_filename();
	char *get_sub_line();

	skip_space(line);
	/* Create directive_macro_info in case any macros are in the line directive. */
	if(StaticAnalysisOption)
	{
		ch_ptr = line;
		while(*ch_ptr != '\n')
			ch_ptr++;
		directive_macro_info = temp_alloc(7+2*(ch_ptr-line)+2);
		*directive_macro_info = DIRECTIVE_START;
		strcpy(directive_macro_info+1, "#line ");
		mark_macros(directive_macro_info+7, line, ch_ptr-line);
	}
	line = get_sub_line(line);
	skip_space(line);
	if(isdigit(*line))
	{
		lineno = 0;
		while(isdigit(*line))
			lineno = lineno*10+*line++-'0';
		skip_space(line);
		/* If there was only a line number in the directive then return. */
		if(*line == '\n')
		{
			current_file->lineno = lineno-1;
			current_lineno = current_file->lineno+1;
			return;
		}
		/* Otherwise, look for a following filename. */
		if(*line == '"')
		{
			filename = parse_filename(line+1, '"');
			if(filename != NULL)
			{
				current_file->filename = filename;
				num_lines_to_finish_file = current_lineno-output_lineno;
				current_file->lineno = lineno-1;
				current_lineno = current_file->lineno+1;
				return;
			}
		}
	}
	error("Bad syntax for #line directive");
}


/*
 * This routine initializes the value of output_line to be the
 * header information that needs to appear as the first output.  This header
 * information is not generated if SuppressSynchronization is set.
 */
init_output_line()
{
	register char *line_ptr, *date;
	char *get_current_date();

	output_lineno = 1;
	output_filename = current_file->filename;
	if(SuppressSynchronization)
		return;
	line_ptr = output_line = temp_alloc(buffersize);
	if(DebugOption)
	{
		strcpy(line_ptr, "#pragma CURRENT_DATE ");
		line_ptr += 21;
		date = get_current_date();
		*line_ptr++ = ' ';
		strcpy(line_ptr, date);
		line_ptr += strlen(date);
		*line_ptr++ = '\n';
	}
	strcpy(line_ptr, "# 1 \"");
	line_ptr += 5;
	strcpy(line_ptr, current_file->filename);
	line_ptr += strlen(current_file->filename);
	*line_ptr++ = '"';
	if(current_file->filedate != NULL)
	{
		strcpy(line_ptr, current_file->filedate);
		line_ptr += strlen(current_file->filedate);
	}
	*line_ptr++ = '\n';
	*line_ptr++ = '\0';
}


/*
 * This is routine returns one line of preprocessed input (which may contain newlines).
 * It is responsible for returning '#' synchronization lines to keep the line count
 * right.
 */
char *get_output_line()
{
	register char *synchronization_line;
	register char *sync_ptr;
	register char *line_ptr;
	register int num_lines;
	register int length;
	char *get_sub_line();

	extern jmp_buf cpp_error_jmp_buf;

	/* If NoExitOnError is set by caller, then use cpp_error_jmp_buf
	 * to return here if a fatal error is encountered. */
	if (NoExitOnError)
		if (setjmp (cpp_error_jmp_buf))
			return NULL;

	/* If there is a line left over due to a line synchronization directive
	 * being output last time, then output that line. */
	if(output_line != NULL)
	{
		line_ptr = output_line;
		output_line = NULL;
		/* Since the returned line may have had multiple newlines, then increment
		 * output_line by the actual number of newlines in the output. */
		output_lineno += num_newlines_in_output;
		return line_ptr;
	}
	/* Get a new non-blank or file transition line from get_sub_line. */
	do
	{
		/* Deallocate all line memory since there are no current lines. */
		temp_dealloc();
		num_newlines_in_output = 0;
		/* Since the first line of a potentially multi-line 'get_line' input is
		 * about to be read, set 'current_lineno' to the first line number. */
		current_lineno = current_file->lineno+1;
		output_line = get_sub_line(NULL);
		/* If a ^B or ^C sequence needs to be returned then insert the sequence
		 * in front of the current output line.  The test for output_line not
		 * being NULL is just to be safe; all # directives which cause such
		 * sequences should cause a newline in output_line. */
		if(StaticAnalysisOption && directive_macro_info != NULL && output_line != NULL)
		{
			length = strlen(directive_macro_info);
			line_ptr = temp_alloc(length+strlen(output_line)+1);
			strcpy(line_ptr, directive_macro_info);
			strcpy(line_ptr+length, output_line);
			output_line = line_ptr;
			directive_macro_info = NULL;
		}
	} while(output_line != NULL && *output_line == '\n' && output_filename == current_file->filename);
	/* If the line is NULL, then the end of the current file was hit. */
	if(output_line == NULL)
	{
		/* If the original source file was ended then return NULL. */
		if(!end_file())
			return NULL;
	}
	/* If the output line number or filename are different than they should be
	 * then generate a line synchronization directive. (The double check for the
	 * filename is to make the normal case of the files being the same short
	 * circuit after a cheap test.) */
	if(output_lineno != current_lineno ||
	   (output_filename != current_file->filename
	    && strcmp(output_filename, current_file->filename) != 0))
	{
		/* See if synchronization can be accomplished by outputting blank lines. */
		num_lines = current_lineno-output_lineno;
		if((output_filename == current_file->filename)
	           && ((StaticAnalysisOption && 
			/* DTS CLLbs00359 klee 09/08/92 */ 
			(output_line) &&
			/* DTS CLLbs00359 */ 
			*output_line == DEFINE_START)
			|| SuppressSynchronization
			|| num_lines <= MAX_SYNC_LINES)
		   && (num_lines > 0))
		{
			int num_NLs;
			char *lp;

			if (StaticAnalysisOption&&(*output_line==DEFINE_START))
			{
				num_NLs = 0;
				lp = output_line;
				while(*lp++)
					if(*lp == NL_MARK) num_NLs++;
				num_lines -= num_NLs;
				if (num_NLs > 0)
					output_line = temp_realloc(output_line,
							strlen(output_line)
							+ num_NLs + 1);
				while(num_NLs-- > 0)
				{
					strcat(output_line, "\n");
					++num_newlines_in_output;
				}
			}
			synchronization_line = temp_alloc(num_lines+1);
			sync_ptr = synchronization_line;
			output_lineno += num_lines;
			while(num_lines-- > 0)
				*sync_ptr++ = '\n';
			*sync_ptr = '\0';
			return synchronization_line;
		}
		else if(!SuppressSynchronization)
		{
			if(num_lines_to_finish_file > 0)
			{
				synchronization_line = temp_alloc(13+strlen(current_file->filename)+20+18);
				sync_ptr = synchronization_line;
				if(num_lines_to_finish_file <= MAX_SYNC_LINES)
				{
					while(num_lines_to_finish_file-- > 0)
						*sync_ptr++ = '\n';
				}
				else
				{
					*sync_ptr++ = '#';
					*sync_ptr++ = ' ';
					number_to_string(output_lineno+num_lines_to_finish_file-1, sync_ptr);
					*sync_ptr++ = '\n';
					*sync_ptr++ = '\n';
				}
			}
			else
			{
				synchronization_line = temp_alloc(strlen(current_file->filename)+20+18);
				sync_ptr = synchronization_line;
			}
			*sync_ptr++ = '#';
			*sync_ptr++ = ' ';
			number_to_string(current_lineno, sync_ptr);
			if(output_filename != current_file->filename)
			{
				*sync_ptr++ = ' ';
				*sync_ptr++ = '"';
				strcpy(sync_ptr, current_file->filename);
				sync_ptr += strlen(current_file->filename);
				*sync_ptr++ = '"';
				if(current_file->filedate != NULL)
				{
					strcpy(sync_ptr, current_file->filedate);
					sync_ptr += strlen(current_file->filedate);
				}
				output_filename = current_file->filename;
			}
			*sync_ptr++ = '\n';
			*sync_ptr = '\0';
			output_lineno = current_lineno;
			/* if this is a marked #include directive, emit the */
			/* sync line after the output line                  */ 
	                if (StaticAnalysisOption
			    && (output_line != NULL)
			    && (*output_line == DIRECTIVE_START)
			    && (id_cmp(output_line+2,"include")) )
				{
				line_ptr = output_line;
				output_line = synchronization_line;
				return line_ptr;
				}
			else
				return synchronization_line;
		}
		else
		{
			output_filename = current_file->filename;
			output_lineno = current_lineno;
		}
	}
	/* The 'output_line' can be NULL if an include file has just terminated. */
	if(output_line == NULL)
	{
		return get_output_line();
	}
	line_ptr = output_line;
	output_line = NULL;
	/* Since the returned line may have had multiple newlines, then increment
	 * output_line by the actual number of newlines in the output. */
	output_lineno += num_newlines_in_output;
	return line_ptr;
}

#ifdef HP_OSF
/*
 * This routine adds the given search path to the 'sys_search_path_list' 
 * and 'pri_search_path_list'.  It is called to set up the standard include 
 * directory, directories given via a '-I' command line option, and the 
 * directory containing the source file.
 */
add_search_path(path)
char *path;
{
	t_search_path *search_path;
	register char *ch_ptr;

	/* Adds the given search path to the 'sys_search_path_list' */
	search_path = NEW(t_search_path);
	ch_ptr = search_path->pathname = perm_alloc(strlen(path)+2);
	while(*path != '\0')
		*ch_ptr++ = *path++;
	/* If the path was non-null and did not end with '/' then add a '/'. */
	if(ch_ptr > search_path->pathname && *(ch_ptr-1) != '/')
		*ch_ptr++ = '/';
	*ch_ptr++ = '\0';
	search_path->next = sys_search_path_list;
	sys_search_path_list = search_path;

	/* Adds the given search path to the 'pri_search_path_list' */
	search_path = NEW(t_search_path);
	ch_ptr = search_path->pathname = perm_alloc(strlen(path)+2);
	while(*path != '\0')
		*ch_ptr++ = *path++;
	/* If the path was non-null and did not end with '/' then add a '/'. */
	if(ch_ptr > search_path->pathname && *(ch_ptr-1) != '/')
		*ch_ptr++ = '/';
	*ch_ptr++ = '\0';
	search_path->next = pri_search_path_list;
	pri_search_path_list = search_path;
}


/*
 * This routine adds the given search path to the 'sys_pre_search_path_list'
 * and 'pri_pre_search_path_list'.  It is called for directories given via a 
 * '-I' command line option. It holds the list of -I paths which will later 
 * be attached to the sys_search_path_list and pri_search_path_list by 
 * add_pre_search_paths which will put them in the proper order of priority.
 */
handle_pre_search_path(path)
char *path;
{
	t_search_path *search_path;
	register char *ch_ptr;
	register char *save_path = path;

	/* Adds the given search path to the 'sys_pre_search_path_list' */
	search_path = NEW(t_search_path);
	ch_ptr = search_path->pathname = perm_alloc(strlen(path)+2);
	while(*path != '\0')
		*ch_ptr++ = *path++;
#ifndef hpe
	/* If the path was non-null and did not end with '/' then add a '/'. */
	if(ch_ptr > search_path->pathname && *(ch_ptr-1) != '/')
		*ch_ptr++ = '/';
#endif
	*ch_ptr++ = '\0';
	*ch_ptr++ = '\0';
	search_path->next = sys_pre_search_path_list;
	sys_pre_search_path_list = search_path;

	/* Adds the given search path to the 'pri_pre_search_path_list' */
	path = save_path;
	search_path = NEW(t_search_path);
	ch_ptr = search_path->pathname = perm_alloc(strlen(path)+2);
	while(*path != '\0')
		*ch_ptr++ = *path++;
#ifndef hpe
	/* If the path was non-null and did not end with '/' then add a '/'. */
	if(ch_ptr > search_path->pathname && *(ch_ptr-1) != '/')
		*ch_ptr++ = '/';
#endif
	*ch_ptr++ = '\0';
	*ch_ptr++ = '\0';
	search_path->next = pri_pre_search_path_list;
	pri_pre_search_path_list = search_path;
}

/*
 * This routine adds the given search path to the 'sys_pre_search_path_list'.
 * It is called for directories given via a '-y' command line option.
 * It holds the list of -y paths which will later be attached to the
 * sys_search_path_list by add_pre_search_paths which will put them in the
 * proper order of priority.
 */
handle_sys_pre_search_path(path)
char *path;
{
	t_search_path *search_path;
	register char *ch_ptr;

	search_path = NEW(t_search_path);
	ch_ptr = search_path->pathname = perm_alloc(strlen(path)+2);
	while(*path != '\0')
		*ch_ptr++ = *path++;
#ifndef hpe
	/* If the path was non-null and did not end with '/' then add a '/'. */
	if(ch_ptr > search_path->pathname && *(ch_ptr-1) != '/')
		*ch_ptr++ = '/';
#endif
	*ch_ptr++ = '\0';
	*ch_ptr++ = '\0';
	search_path->next = sys_pre_search_path_list;
	sys_pre_search_path_list = search_path;
}

/*
 * This routine adds the given search path to the 'pri_pre_search_path_list'.
 * It is called for directories given via a '-p' command line option.
 * It holds the list of -p paths which will later be attached to the
 * pri_search_path_list by add_pre_search_paths which will put them in the
 * proper order of priority.
 */
handle_pri_pre_search_path(path)
char *path;
{
	t_search_path *search_path;
	register char *ch_ptr;

	search_path = NEW(t_search_path);
	ch_ptr = search_path->pathname = perm_alloc(strlen(path)+2);
	while(*path != '\0')
		*ch_ptr++ = *path++;
#ifndef hpe
	/* If the path was non-null and did not end with '/' then add a '/'. */
	if(ch_ptr > search_path->pathname && *(ch_ptr-1) != '/')
		*ch_ptr++ = '/';
#endif
	*ch_ptr++ = '\0';
	*ch_ptr++ = '\0';
	search_path->next = pri_pre_search_path_list;
	pri_pre_search_path_list = search_path;
}

/*
 * This routine adds those paths in the 'sys_pre_search_path_list' and the
 * 'pri_pre_search_path_list' to the 'sys_search_path_list' and the
 * 'pri_search_path_list' respectively in the proper order by reversing the 
 * order in the 'sys_pre_search_path_list' and the 'pri_pre_search_path_list'.
 */
add_pre_search_paths()
{
	t_search_path *result, *temp;
                                          
	while(sys_pre_search_path_list != NULL)
	{
		temp = sys_search_path_list;
		sys_search_path_list = sys_pre_search_path_list;
		sys_pre_search_path_list = sys_pre_search_path_list->next;
		sys_search_path_list->next = temp;
	}

	while(pri_pre_search_path_list != NULL)
	{
		temp = pri_search_path_list;
		pri_search_path_list = pri_pre_search_path_list;
		pri_pre_search_path_list = pri_pre_search_path_list->next;
		pri_search_path_list->next = temp;
	}
}
#else
/*
 * This routine adds the given search path to the 'search_path_list'.
 * It is called to set up the standard include directory, directories given
 * via a '-I' command line option, and the directory containing the
 * source file.
 */
add_search_path(path)
char *path;
{
	t_search_path *search_path;
	register char *ch_ptr;

	search_path = NEW(t_search_path);
	ch_ptr = search_path->pathname = perm_alloc(strlen(path)+2);
	while(*path != '\0')
		*ch_ptr++ = *path++;
	/* If the path was non-null and did not end with '/' then add a '/'. */
	if(ch_ptr > search_path->pathname && *(ch_ptr-1) != '/')
		*ch_ptr++ = '/';
	*ch_ptr++ = '\0';
	search_path->next = search_path_list;
	search_path_list = search_path;
}


/*
 * This routine adds the given search path to the 'pre_search_path_list'.
 * It is called for directories given via a '-I' command line option.
 * It holds the list of -I paths which will later be attached to the
 * search_path_list by add_pre_search_paths which will put them in the
 * proper order of priority.
 */
handle_pre_search_path(path)
char *path;
{
	t_search_path *search_path;
	register char *ch_ptr;

	search_path = NEW(t_search_path);
	ch_ptr = search_path->pathname = perm_alloc(strlen(path)+2);
	while(*path != '\0')
		*ch_ptr++ = *path++;
#ifndef hpe
	/* If the path was non-null and did not end with '/' then add a '/'. */
	if(ch_ptr > search_path->pathname && *(ch_ptr-1) != '/')
		*ch_ptr++ = '/';
#endif
	*ch_ptr++ = '\0';
	*ch_ptr++ = '\0';
	search_path->next = pre_search_path_list;
	pre_search_path_list = search_path;
}


/*
 * This routine adds those paths in the pre_search_path_list to the
 * search_path_list in the proper order by reversing the order in the
 * pre_search_path_list.
 */
add_pre_search_paths()
{
	t_search_path *result, *temp;
                                          
#if defined(APOLLO_EXT) || defined(CPP_CSCAN)
        if (!default_dir_set)
                add_search_path("/usr/include");
        /* The default include directory may already have been set
           through the -Y option.  See startup.c */
#endif                 

	while(pre_search_path_list != NULL)
	{
		temp = search_path_list;
		search_path_list = pre_search_path_list;
		pre_search_path_list = pre_search_path_list->next;
		search_path_list->next = temp;
	}
}
#endif /* HP_OSF */

init_file_module()
{
	num_newlines_in_output = 0;
	num_lines_to_finish_file = 0;
	directive_macro_info = NULL;
	known_files = NULL;
	current_file = NULL;
#ifdef HP_OSF
	sys_search_path_list = NULL;
	sys_pre_search_path_list = NULL;
	pri_search_path_list = NULL;
	pri_pre_search_path_list = NULL;
#else
	search_path_list = NULL;
	pre_search_path_list = NULL;
#endif /* HP_OSF */
}
