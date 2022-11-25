/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/namelist.c,v $
 *
 * $Revision: 66.2 $
 *
 * namelist.c - track filenames given as arguments to tar/cpio/pax
 *
 * DESCRIPTION
 *
 *	Arguments may be regular expressions, therefore all agurments will
 *	be treated as if they were regular expressions, even if they are
 *	not.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
 *
 * COPYRIGHT
 *
 *	Copyright (c) 1989 Mark H. Colburn.  All rights reserved.
 *
 *	Redistribution and use in source and binary forms are permitted
 *	provided that the above copyright notice and this paragraph are
 *	duplicated in all such forms and that any documentation,
 *	advertising materials, and other materials related to such
 *	distribution and use acknowledge that the software was developed
 *	by Mark H. Colburn.
 *
 *	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	namelist.c,v $
 * Revision 66.2  90/07/10  08:32:45  08:32:45  kawo
 * Changes for CDF-file handling in name_next().
 * Generate warnings for files with ACLs and for RFA-files in name_next().
 * 
 * Revision 66.1  90/05/11  09:00:29  09:00:29  michas (#Michael Sieber)
 * inital checkin
 * 
 * Revision 2.0.0.4  89/12/16  10:35:37  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.3  89/10/13  02:35:21  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: namelist.c,v 2.0.0.4 89/12/16 10:35:37 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Type Definitions */

/*
 * Structure for keeping track of filenames and lists thereof. 
 */
struct nm_list {
    struct nm_list     *next;
    short               length;	/* cached strlen(name) */
    char                found;	/* A matching file has been found */
    char                firstch;/* First char is literally matched */
    char                regexp;	/* regexp pattern for item */
    char                name[1];/* name of file or rexexp */
};

struct dirinfo {
    char                dirname[PATH_MAX + 1];	/* name of directory */
    OFFSET              where;	/* current location in directory */
    struct dirinfo     *next;
};


/* Static Variables */

static struct dirinfo *stack_head = (struct dirinfo *) NULL;


/* Function Prototypes */

#ifdef __STDC__
#define P(x)    x
#else
#define P(x)	()
#endif
    
static void		pushdir P((struct dirinfo * info));
static struct dirinfo  *popdir P((void));

extern	int		modify_last_name_in_path P((char *));
extern	void		expand_cdf_path P((char *));

#undef P

    
/* Internal Identifiers */

static struct nm_list *namelast;/* Points to last name in list */
static struct nm_list *namelist;/* Points to first name in list */


/* addname -  add a name to the namelist. 
 *
 * DESCRIPTION
 *
 *	Addname adds the name given to the name list.  Memory for the
 *	namelist structure is dynamically allocated.  If the space for 
 *	the structure cannot be allocated, then the program will exit
 *	the an out of memory error message and a non-zero return code
 *	will be returned to the caller.
 *
 * PARAMETERS
 *
 *	char *name	- A pointer to the name to add to the list
 */

void
add_name(name)
    char               *name;	/* pointer to name */
{
    int                 i;	/* Length of string */
    struct nm_list     *p;	/* Current struct pointer */

    DBUG_ENTER("add_name");
    
#ifdef MSDOS
    dio_str(name);
#endif /* MSDOS */
    
    i = strlen(name);
    p = (struct nm_list *) malloc((unsigned) (i + sizeof(struct nm_list)));
    if (!p) {
	fatal("cannot allocate memory for namelist entry\n");
    }
    p->next = (struct nm_list *) NULL;
    p->length = i;
    strncpy(p->name, name, i);
    p->name[i] = '\0';		/* Null term */
    p->found = 0;
    p->firstch = isalpha(name[0]);
    if (index(name, '*') || index(name, '[') || index(name, '?')) {
	p->regexp = 1;
    }
    if (namelast) {
	namelast->next = p;
    }
    namelast = p;
    if (!namelist) {
	namelist = p;
    }
    DBUG_VOID_RETURN;
}


/* name_match - match a name from an archive with a name from the namelist 
 *
 * DESCRIPTION
 *
 *	Name_match attempts to find a name pointed at by p in the namelist.
 *	If no namelist is available, then all filenames passed in are
 *	assumed to match the filename criteria.  Name_match knows how to
 *	match names with regular expressions, etc.
 *
 * PARAMETERS
 *
 *	char	*p	- the name to match
 *
 * RETURNS
 *
 *	Returns 1 if the name is in the namelist, or no name list is
 *	available, otherwise returns 0
 *
 */

int
name_match(p)
    char               *p;
{
    struct nm_list     *nlp;
    int                 len;

    DBUG_ENTER("name_match");
    if ((nlp = namelist) == 0) {/* Empty namelist is easy */
	DBUG_RETURN(1);
    }
    len = strlen(p);
    for (; nlp != 0; nlp = nlp->next) {
	/* If first chars don't match, quick skip */
	if (nlp->firstch && nlp->name[0] != p[0]) {
	    continue;
	}
	/* Regular expressions */
	if (nlp->regexp) {
	    if (wildmat(nlp->name, p)) {
		nlp->found = 1;	/* Remember it matched */
		DBUG_RETURN(1);	/* We got a match */
	    }
	    continue;
	}
	/* Plain Old Strings */
	if (nlp->length <= len	/* Archive len >= specified */
	    && (p[nlp->length] == '\0' || p[nlp->length] == '/')
	    && strncmp(p, nlp->name, nlp->length) == 0) {
	    /* Name compare */
	    nlp->found = 1;	/* Remember it matched */
	    DBUG_RETURN(1);	/* We got a match */
	}
    }
    DBUG_RETURN(0);
}


/* notfound - print names of files in namelist that were not found 
 *
 * DESCRIPTION
 *
 *	Notfound scans through the namelist for any files which were
 *	named, but for which a matching file was not processed by the
 *	archive.  Each of the files is listed on the standard error.
 *
 */

void
notfound()
{
    struct nm_list     *nlp;

    DBUG_ENTER("notfound");
    for (nlp = namelist; nlp != 0; nlp = nlp->next) {
	if (!nlp->found) {
	    fprintf(stderr, "%s: %s not found in archive\n",
		    myname, nlp->name);
	}
	free(nlp);
    }
    namelist = (struct nm_list *) NULL;
    namelast = (struct nm_list *) NULL;
    DBUG_VOID_RETURN;
}


/* name_init - set up to gather file names 
 *
 * DESCRIPTION
 *
 *	Name_init sets up the namelist pointers so that we may access the
 *	command line arguments.  At least the first item of the command
 *	line (argv[0]) is assumed to be stripped off, prior to the
 *	name_init call.
 *
 * PARAMETERS
 *
 *	int	argc	- number of items in argc
 *	char	**argv	- pointer to the command line arguments
 */

void
name_init(argc, argv)
    int                 argc;
    char              **argv;
{
    DBUG_ENTER("name_init");
    /* Get file names from argv, after options. */
    n_argc = argc;
    n_argv = argv;
    DBUG_VOID_RETURN;
}


/* name_next - get the next name from argv or the name file. 
 *
 * DESCRIPTION
 *
 *	Name next finds the next name which is to be processed in the
 *	archive.  If the named file is a directory, then the directory
 *	is recursively traversed for additional file names.  Directory
 *	names and locations within the directory are kept track of by
 *	using a directory stack.  See the pushdir/popdir function for
 *	more details.
 *
 * 	The names come from argv, after options or from the standard input.  
 *
 * PARAMETERS
 *
 *	name - a pointer to a buffer of at least MAX_PATH + 1 bytes long;
 *	statbuf - a pointer to a stat structure
 *
 * RETURNS
 *
 *	Returns -1 if there are no names left, (e.g. EOF), otherwise returns 
 *	0 
 */

int
name_next(name, statbuf)
    char               *name;	/* File name (returned to caller) */
    Stat               *statbuf;/* status block for file (returned) */
{
    int                 err = -1;
    int                 len;
    int                 cdfflag;	/* only used if cdf handling */
    static int          in_subdir = 0;
    static DIR         *dirp;
    struct dirent      *d;
    static struct dirinfo *curr_dir;

    DBUG_ENTER("name_next");
    do {
	if (names_from_stdin) {
	    if (lineget(stdin, name) < 0) {
		DBUG_RETURN(-1);
	    }
	    if (nameopt(name) < 0) {
		continue;
	    }
	} else {
	    if (in_subdir) {
		if ((d = readdir(dirp)) != (struct dirent *) NULL) {
		    /* Skip . and .. */
		    if (strcmp(d->d_name, ".") == 0 ||
			strcmp(d->d_name, "..") == 0) {
			continue;
		    }
		    if (strlen(d->d_name) +
			strlen(curr_dir->dirname) >= PATH_MAX) {
			warn("name too long", d->d_name);
			continue;
		    }
		    strcpy(name, curr_dir->dirname);
		    strcat(name, d->d_name);
		} else {
		    closedir(dirp);
		    in_subdir--;
		    curr_dir = popdir();
		    if (in_subdir) {
			errno = 0;
			if ((dirp = opendir(curr_dir->dirname)) == (DIR *) NULL) {
			    warn(curr_dir->dirname,
				 "error opening directory (1)");
			    in_subdir--;
			}
			seekdir(dirp, curr_dir->where);
		    }
		    continue;
		}
	    } else if (optind >= n_argc) {
		DBUG_RETURN(-1);
	    } else {
		strcpy(name, n_argv[optind++]);
	    }
	}
	expand_cdf_path(name);
	cdfflag = modify_last_name_in_path(name);
	if ((err = LSTAT(name, statbuf)) < 0) {
	    warn(name, strerror(errno));
	    continue;
	}
	if (cdfflag) {
	    statbuf->sb_mode |= S_CDF;
	}

	if (statbuf->sb_acl)
	    fprintf(stderr, "Optional acl entries for <%s> are not backed up.\n", name);

#ifdef S_ISNWK
	if (S_ISNWK(statbuf->sb_mode)) {
	    fprintf(stderr, "<%s> is a network special file which is not backed up.\n", name		);
	    err = -1;
	    continue;
	}
#endif /* S_ISNWK */
	if (!names_from_stdin && S_ISDIR(statbuf->sb_mode)) {
	    if (in_subdir) {
		curr_dir->where = telldir(dirp);
		pushdir(curr_dir);
		closedir(dirp);
	    }
	    in_subdir++;

	    /* Build new prototype name */
	    if ((curr_dir = (struct dirinfo *) mem_get(sizeof(struct dirinfo)))
		== (struct dirinfo *) NULL) {
		exit(2);
	    }
	    strcpy(curr_dir->dirname, name);
	    len = strlen(curr_dir->dirname);

	    /* delete trailing slashes */
	    while (len >= 1 && curr_dir->dirname[len - 1] == '/') {
		len--;
	    }
	    curr_dir->dirname[len++] = '/';	/* Now add exactly one back */
	    curr_dir->dirname[len] = '\0';	/* Make sure null-terminated */
	    curr_dir->where = 0;

	    errno = 0;
	    do {
		if ((dirp = opendir(curr_dir->dirname)) == (DIR *) NULL) {
		    warn(curr_dir->dirname, "error opening directory (2)");
		    if (in_subdir > 1) {
			curr_dir = popdir();
		    }
		    in_subdir--;
		    err = -1;
		    continue;
		} else {
		    seekdir(dirp, curr_dir->where);
		}
	    } while (in_subdir && (!dirp));
	}
    } while (err < 0);
    
#ifdef MSDOS
    dio_str(name);
#endif /* MSDOS */
    
    DBUG_RETURN(0);
}


/* name_gather - gather names in a list for scanning. 
 *
 * DESCRIPTION
 *
 *	Name_gather takes names from the command line and adds them to
 *	the name list.
 *
 * FIXME
 *
 * 	We could hash the names if we really care about speed here.
 */

void
name_gather()
{
    DBUG_ENTER("name_gather");
    while (optind < n_argc) {
	add_name(n_argv[optind++]);
    }
    DBUG_VOID_RETURN;
}


/* pushdir - pushes a directory name on the directory stack
 *
 * DESCRIPTION
 *
 *	The pushdir function puses the directory structure which is pointed
 *	to by "info" onto a stack for later processing.  The information
 *	may be retrieved later with a call to popdir().
 *
 * PARAMETERS
 *
 *	dirinfo	*info	- pointer to directory structure to save
 */

static void
pushdir(info)
    struct dirinfo     *info;
{
    DBUG_ENTER("pushdir");
    if (stack_head == (struct dirinfo *) NULL) {
	stack_head = info;
	stack_head->next = (struct dirinfo *) NULL;
    } else {
	info->next = stack_head;
	stack_head = info;
    }
    DBUG_VOID_RETURN;
}


/* popdir - pop a directory structure off the directory stack.
 *
 * DESCRIPTION
 *
 *	The popdir function pops the most recently pushed directory
 *	structure off of the directory stack and returns it to the calling
 *	function.
 *
 * RETURNS
 *
 *	Returns a pointer to the most recently pushed directory structure
 *	or NULL if the stack is empty.
 */

static struct dirinfo *
popdir()
{
    struct dirinfo     *tmp;

    DBUG_ENTER("popdir");
    if (stack_head == (struct dirinfo *) NULL) {
	DBUG_RETURN((struct dirinfo *) NULL);
    } else {
	tmp = stack_head;
	stack_head = stack_head->next;
    }
    DBUG_RETURN(tmp);
}
