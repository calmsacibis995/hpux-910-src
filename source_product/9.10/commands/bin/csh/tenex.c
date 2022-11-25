/* @(#) $Revision: 72.2 $ */
/*
 * Revision 1.12  83/10/14  15:27:22  shel
 * save original tty state when first entered
 * also correctly recognize user set erase and kill characters
 * and only erase with BS SP BS if echoe is set by user
 *
 * Revision 1.11  83/10/12  17:50:50  shel
 * added ^w processing to back up a single word
 *
 * Revision 1.10  83/10/07  18:07:09  shel
 * use raw mode to read characters to support ^u and ^f processing
 * corectly along with ESC
 *
 * Revision 1.9  83/10/07  15:31:20  shel
 * first revision using alternative EOL character
 *
 *
 * Tenex style file name recognition, .. and more.
 * History:
 *   Author: Ken Greer, Sept. 1975, CMU.
 *   Finally got around to adding to the Cshell., Ken Greer, Dec. 1981.
 *
 *   Search and recognition of command names (in addition to file names)
 *   by Mike Ellis, Fairchild A.I. Labs, Sept 1983.
 *
 */
#include <sys/param.h>
#include "sh.h"
#include <termio.h>
#include <pwd.h>
#include <fcntl.h>

/*  PCHAR is undefined here so that it can be re-defined later on in this file.
*/

#ifndef NLS
#   define catgets(i,sn,mn, s) (s)
#else /* NLS */
#   define NL_SETN 16	/* set number */
#   include <nl_types.h>
#   include <nl_ctype.h>
#   undef PCHAR		/* we use PCHAR in this file, but it is also in nl_ctype.h */
    nl_catd nlmsg_fd;
#endif /* NLS */

/* Don't include stdio.h!  Csh doesn't like it!! */
#ifdef TEST
#include <stdio.h>
#undef putchar
#define flush()         fflush(stdout)
#endif

#define TRUE            1
#define FALSE           0
#define ON              1
#define OFF             0
#define FILSIZ          MAXPATHLEN	/* Max reasonable file name length */
#define ESC             '\033'
#define equal(a, b)     (strcmp(a, b) == 0)
#define is_set(var)     ((int) adrof(var) != 0)
#define BUILTINS        "/usr/lib/builtins/" /* builtins directory.-hn */
#define index strchr
#define rindex strrchr

extern short SHIN, SHOUT;
extern char *getenv ();
extern putchar ();
extern int (*sigignore())();

#ifdef NLS
extern	int	_nl_space_alt;
#endif

typedef enum {LIST, RECOGNIZE} COMMAND;

static char
    *BELL = "\07";
struct termio SAVEtty,CURtty;

/*  Called by:

	auto_logout ()
	pintr1 ()
	tenex ()
*/
/*  Purpose:  Toggle between no canonical processing and echoing and whatever
	      was set originally.
*/
/**********************************************************************/
setup_tty (on)
/**********************************************************************/
{
    static int werehere = 0;

	/* DSDe413471: If setup_tty is already ON, we should not
	 * re-save the current settings, else the original saved tty
	 * gets clobbered.
	 */
	 if(on && werehere) return;

    	sigignore (SIGINT);
    if (on)
    {
        ioctl (SHIN, TCGETA, &CURtty);

/*  Apparently structures can be set just by setting them equal.
*/
	SAVEtty=CURtty;
	werehere = 1;

#ifdef DEBUG_TTYECHO
  printf ("setup_tty (1): %d, Turning OFF echo: flag: %ho (ECHO=010)\n", 
	  getpid(), CURtty.c_lflag);
#endif

/*  Turn off canonical processing and echoing.
*/
	CURtty.c_lflag &= ~ICANON;
	CURtty.c_lflag &= ~ECHO;
	CURtty.c_cc[VMIN]=1;
	CURtty.c_cc[VTIME]=1;
        ioctl (SHIN, TCSETAW, &CURtty);

    }
    else if (werehere) /* only reset if we had already set */
    {


#ifdef DEBUG_TTYECHO
  printf ("setup_tty (1): %d, Turning ON echo: flag: %ho (ECHO=010)\n", 
	  getpid(), SAVEtty.c_lflag);
#endif

/*  Turn on canonical processing and echoing.
	SAVEtty.c_lflag |= ICANON;
	SAVEtty.c_lflag |= ECHO;
*/
        ioctl (SHIN, TCSETA, &SAVEtty);
        werehere = 0;		/* DSDe413471: Added this to ensure mutual
							 * exclusion */
    }
    	sigrelse (SIGINT);
}

/*  Called by:

	tenex ()
*/
/*  Purpose:  Get termio information about TERM and store it in a buffer
	      which isn't static.  The visual bell information for the
	      terminal is stored in a static area.
*/
/**********************************************************************/
static
termchars ()
/**********************************************************************/
{
    extern char *tgetstr ();
    char bp[1024];
    static char area[256];
    static int been_here = 0;
    char *ap = area;
    register char *s;

    if (been_here)
        return;
    been_here = TRUE;

    s=getenv("TERM");
    if (s == 0)
        return;

/*  This is a library call that extracts the compiled entry for "s" from
    terminfo and stores it in user provided space.
*/
    if (tgetent (bp, s) != 1)
        return;

/*  This is a library call that returns a pointer to the string for the
    requested capability.  It also stores the string in user provided space.
*/
    if (s = tgetstr ("vb", &ap))                /* Visible Bell */
        BELL = s;
    return;
}

/*
 * Move back to beginning of current line
 */

/*  Called by:

	tenex ()
	backspace ()
*/
/*  Purpose:  Move back to column 1 after turning off post processing, then
	      restoring it if it was on previously.
*/
/**********************************************************************/
static
back_to_col_1 ()
/**********************************************************************/
{
    struct termio tty, tty_normal;
    sigignore (SIGINT);
    ioctl (SHIN, TCGETA, &tty);

/*  Apparently structures can be set just by setting them equal.
*/
    tty_normal = tty;

/*  Turn off post processing.
*/
    tty.c_oflag &= ~OPOST;
    ioctl (SHIN, TCSETAW, &tty);

/*  Send out a carriage return to move back to column 1.
*/
    (void) write (SHOUT, "\r", 1);

/*  Restore post processing if it was turned on previously.
*/
    ioctl (SHIN, TCSETAW, &tty_normal);
    sigrelse (SIGINT);
}

/*
 * Push string contents back into tty queue
 */
static char pline[BUFSIZ]; /* where we will store the pushbacked line */
static int  pbacked=0;

/*  Called by:

	tenex ()
*/
/*  Purpose:  Copy the input string back to a static area so that it can
	      be processed again.
*/
/**********************************************************************/
static
pushback (string)
char  *string;
/**********************************************************************/
{
    strcpy(pline,string);
    pbacked=1;
}

/*
 * Concatonate src onto tail of des.
 * Des is a string whose maximum length is count.
 * Always null terminate.
 */

/*  Called by:

	filetype ()
	tnx_search ()
*/
/*  Purpose:  Concatenate strings and NULL terminate the result.
*/
/**********************************************************************/
catn (des, src, count)
register char *des, *src;
register count;
/**********************************************************************/
{
/*  If count goes negative before the end of des, then des gets truncated.
    If count goes negative before the end of src then the des string doesn't
    get all of src appended onto it.  Otherwise, all of src gets copied onto
    des, including the terminating NULL on src.  Then another NULL is added
    onto des.
*/
    while (--count >= 0 && *des)
        des++;
    while (--count >= 0)
        if ((*des++ = *src++) == 0)
            return;
    *des = '\0';
}

/*  Called by:

	print_by_column ()
*/
/**********************************************************************/
static
max (a, b)
/**********************************************************************/
{
    if (a > b)
        return (a);
    return (b);
}

/*
 * like strncpy but always leave room for trailing \0
 * and always null terminate.
 */

/*  Called by:

	extract_dir_and_name ()
	tnx_search ()
	recognize ()
*/
/*  Purpose:  Copy a string and NULL terminate it.
*/
/**********************************************************************/
copyn (des, src, count)
register char *des, *src;
register count;
/**********************************************************************/
{
    while (--count >= 0)
        if ((*des++ = *src++) == 0)
            return;
    *des = '\0';
}

/*
 * For qsort()
 */

/*  Called indirectly by:

	qsort library routine
*/
/**********************************************************************/
static
fcompare (file1, file2)
char  **file1, **file2;
/**********************************************************************/
{
    return (strcoll (*file1, *file2));
}

/*  Called by:

	tnx_search ()
*/
/*  Purpose:  Determine if a file is a directory, symbolic link, executable,
	      CDF, or regular file and return a code:

	      directory:     /
	      symbolic link: > if final file statable
			     @ if not
              executable:    *
	      CDF:           +
	      regular file:  space
*/
/**********************************************************************/
#if defined(DISKLESS) || defined(DUX)
static char
filetype (dir, file, check_hidden)
char *dir, *file;
int check_hidden;
#else
static char
filetype (dir, file)
char *dir, *file;
#endif /* not defined(DISKLESS) || defined(DUX) */
/**********************************************************************/
{
    if (dir)
    {
        char path[FILSIZ];
        struct stat statb;
#ifdef ACLS
	int rootgid = 0;
#endif /* ACLS */

        strcpy (path, dir);
        catn (path, file, sizeof path);

#ifdef SYMLINKS
	if (lstat(path, &statb) == 0) {
	    switch (statb.st_mode & S_IFMT) {
		case S_IFDIR:
		    return ('/');
		case S_IFLNK:
		    if (stat(path, &statb) == 0 &&	/* follow it out */
		       (statb.st_mode & S_IFMT) == S_IFDIR)
			return ('>');
		    else
			return ('@');
		default:
#ifdef ACLS
		    /*
		     * If execute is not on in base mode bits and file has
		     * optional entries (these checks are done for performance):
		     * To find out if execute is on for anyone, do a getaccess
		     * as root since root has execute if anyone does.  This
		     * works for files that only have execute in optional
		     * ACL entries that don't match the uid or gid of caller.
		     * Note: gid is required, but ignored for root.
		     */
		     if ((statb.st_mode & 0111) || ((statb.st_acl) &&
		    	(1 & (getaccess(path, 0, 1,&rootgid,(void *)0,
			    (void *)0)))))
#else
		     if (statb.st_mode & 0111)
#endif /* ACLS */
			return ('*');
	    }
	}
#else /* not SYMLINKS */
	if (stat (path, &statb) >= 0)
	{
	    if ((statb.st_mode & S_IFMT) == S_IFDIR)
		return ('/');
#ifdef ACLS
	    /*
	     * If execute is not on in base mode bits and file has
	     * optional entries (these checks are done for performance):
	     * To find out if execute is on for anyone, do a getaccess
	     * as root since root has execute if anyone does.  This
	     * works for files that only have execute in optional
	     * ACL entries that don't match the uid or gid of caller.
	     * Note: gid is required, but ignored for root.
	     */
	     if ((statb.st_mode & 0111) || ((statb.st_acl) &&
	    	(1 & (getaccess(path, 0, 1,&rootgid,(void *)0,
		    (void *)0)))))
#else
	     if (statb.st_mode & 0111)
#endif /* ACLS */
		return ('*');
	}
#endif /* SYMLINKS */
#if defined(DISKLESS) || defined(DUX)
	else if (check_hidden)
	{
	    int len = strlen(path);

	    path[len] = '+';
	    path[len+1] = '\0';

	    if (stat (path, &statb) >= 0)
	    {
		if (((statb.st_mode & S_IFMT) == S_IFDIR) &&
			(statb.st_mode & S_ISUID))
		    return ('+');
	    }
	    else
		return (' ');
	}
#endif /* defined(DISKLESS) || defined(DUX) */
    }
    return (' ');
}

/*
 * Print sorted down columns
 */

/*  Called by:

	tnx_search ()
*/
/**********************************************************************/
static
print_by_column (dir, items, count, looking_for_command)
register char *dir, *items[];
/**********************************************************************/
{
    register int i, rows, r, c, maxwidth = 0, columns;
    for (i = 0; i < count; i++)
        maxwidth = max (maxwidth, strlen (items[i]));

    maxwidth++;			/* Allow at least one space between names. */
    columns = 80 / maxwidth;
    columns = columns ? columns : 1;	/* make sure columns > 0 */
    rows = (count + (columns - 1)) / columns;
    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < columns; c++)
        {
            i = c * rows + r;
            if (i < count)
            {
                register int w;
                printf("%s", items[i]);
                w = strlen (items[i]);

                if (c < (columns - 1))                  /* Not last column? */
                    for (; w < maxwidth; w++)
                        putchar (' ');
            }
        }
        printf ("\n");
    }
}

/*
 * expand "old" file name with possible tilde usage
 *              ~person/mumble
 * expands to
 *              home_directory_of_person/mumble
 * into string "new".
 */

/*  Called by:

	tnx_search ()
*/
/*  Purpose:  Copy home directory for a login and concatenate the rest of the
	      path onto it.  The new path is copied into a supplied buffer
	      and a pointer to the buffer is returned.
*/
/**********************************************************************/
char *
tilde (new, old)
char *new, *old;
/**********************************************************************/
{
    extern struct passwd *getpwuid (), *getpwnam ();

    register char *o, *p;
    register struct passwd *pw;
    static char person[40] = {0};
    if (old[0] != '~')
    {
        strcpy (new, old);
        return (new);
    }

/*  Copy the login name to the person buffer.
*/
    for (p = person, o = &old[1]; *o && *o != '/'; *p++ = *o++);
    *p = '\0';

    if (person[0] == '\0')                      /* then use current uid */
        /* pw = getpwuid (getuid ()); */
	strcpy (new, to_char(value(CH_home)));	/* changed to this so when  */
						/* "su other", ~ will still */
						/* expand to you.     -judy */
    else {
        pw = getpwnam (person);
    	if (pw == NULL)
        	return (NULL);

/*  Copy the login directory into the new buffer.
*/
    	strcpy (new, pw -> pw_dir);
    }

/*  Copy the rest of the path onto the login directory.
*/
    (void) strcat (new, o);
    return (new);
}

/*
 * Cause pending line to be printed
 */

/*  Called by:

	tenex ()
*/
/*  Purpose:  Just calls redo () to write pline to SHOUT.
*/
/**********************************************************************/
static
retype ()
/**********************************************************************/
{
    redo(pline);
}

/*  Called indirectly by:

	tputs library routine in termcap (3)
*/
/*  Purpose:   Write a single character to SHOUT.
*/
/**********************************************************************/
static
writec(c)
/**********************************************************************/
{
    (void) write (SHOUT, &c, 1);
}

/*  Called by:

	tenex ()
	getline ()
*/
/*  Purpose:  Send a bell to SHOUT; directly or via termcap.
*/
/**********************************************************************/
static
beep ()
/**********************************************************************/
{
    if (strcmp(BELL, "\07"))			/* Visible Bell */
        (void) tputs(BELL, 1, writec);
    else
        (void) write (SHOUT, BELL, strlen(BELL));
}


/*
 * parse full path in file into 2 parts: directory and file names
 * Should leave final slash (/) at end of dir.
 */

/*  Called by:

	tnx_search ()
*/
/*  Purpose:  Put the directory prefix and file name (as in dir and basename)
              into separate supplied buffers, dir and name.
*/
/**********************************************************************/
static
extract_dir_and_name (path, dir, name)
char   *path, *dir, *name;
/**********************************************************************/
{
/*  Note:  rindex == srrchr
*/
    extern char *rindex ();
    register char  *p;

/*  This finds the last occurance of a slash in the path.
*/
    p = rindex (path, '/');
    if (p == NULL)
    {
        copyn (name, path, MAXNAMLEN);
        dir[0] = '\0';
    }
    else
    {

/*  Move past the last slash.
*/
        p++;
        copyn (name, p, MAXNAMLEN);
        copyn (dir, path, p - path);
    }
}


/*  Called by:

	tnx_search ()
*/
/*  Purpose:  Return the next user login directory in the password file,
	      or the next file in the current open directory.
*/
/**********************************************************************/
char *
getentry (dir_fd, looking_for_lognames)
DIR *dir_fd;
/**********************************************************************/
{
    if (looking_for_lognames)                   /* Is it login names we want? */
    {
        extern struct passwd *getpwent ();
        register struct passwd *pw;

/*  getpwent returns successive entries in the passord file.
*/
        if ((pw = getpwent ()) == NULL)
            return (NULL);
        return (pw -> pw_name);
    }
    else                                        /* It's a dir entry we want */
    {
        register struct direct *dirp;
        if (dirp = readdir(dir_fd))
            return (dirp -> d_name);
        return (NULL);
    }
}

/*  Called by:

	tnx_search ()
*/
/*  Purpose:  Free an array of strings and the array of pointers that point
	      to the strings.
*/
/**********************************************************************/
static
free_items (items)
register char **items;
/**********************************************************************/
{
    register int i;
    for (i = 0; items[i]; i++)
        free ((CHAR *)items[i]);
    free ((CHAR *) items);
}

/*  Don't allow SIGINT while freeing items or directory pointers.
*/

#define FREE_ITEMS(items)\
{\
    sighold (SIGINT);\
    free_items (items);\
    items = NULL;\
    sigrelse (SIGINT);\
}

#define FREE_DIR(fd)\
{\
    sighold (SIGINT);\
    closedir (fd);\
    fd = NULL;\
    sigrelse (SIGINT);\
}

static int  dirctr;             /* -1 0 1 2 ... */
static char dirflag[5];         /*  ' nn\0' - dir #s -  . 1 2 ... */

/*
 * Strip next directory from path; return ptr to next unstripped directory.
 */

/*  Called by:

	tnx_search ()
*/
/*  Purpose:  Copy a path component pointed to by the path pointer into a
	      supplied buffer (dir) and advance the path pointer to the
	      next component.  Return the path pointer.
*/
/**********************************************************************/
char *extract_dir_from_path (path, dir)
char *path, dir[];
/**********************************************************************/
{
    register char *d = dir;

/*  Get to the beginning of the next component.
*/
    while (*path && (*path == ' ' || *path == ':')) path++;

/*  Copy the path component up to the separator, while advancing the path
    pointer.
*/
    while (*path && (*path != ' ' && *path != ':')) *(d++) = *(path++);

/*  Get to the beginning of the next component.
*/
    while (*path && (*path == ' ' || *path == ':')) path++;

    ++dirctr;
    if (*dir == '.')
        strcpy (dirflag, " .");
    else
    {
        dirflag[0] = ' ';
        if (dirctr <= 9)
        {

/*  Puts a [space][digit][\0] into dirflag.
*/
                dirflag[1] = '0' + dirctr;
                dirflag[2] = '\0';
        }
        else
        {

/*  Puts a [space][digit][digit][\0] into dirflag.
*/
                dirflag[1] = '0' + dirctr / 10;
                dirflag[2] = '0' + dirctr % 10;
                dirflag[3] = '\0';
        }
    }

/*  Append a slash to the path component and NULL terminate it.
*/
    *(d++) = '/';
    *d = 0;

/*  The path now points to the next component in the PATH, so return it.
*/
    return path;
}

/*
 * Perform a RECOGNIZE or LIST command on string "word".
 */

/*  Called by:

	tenematch ()
*/
/*  Purpose:  Generate a list or a single item of login directories or files 
	      that begin with the word passed in.  If a single item is 
	      wanted, but multiple items are found, return the longest
	      prefix match among them.  (i.e. initial word = foo, matches
	      are foofile10 and foofile100, this generates foofile10.)
*/
/**********************************************************************/
static
tnx_search (word, wp, command, routine, max_word_length, looking_for_command)
char   *word,
       *wp;                     /* original end-of-word */
COMMAND command;
int (*routine) ();
/**********************************************************************/
{
#   define MAXITEMS 2048
    register numitems,
            name_length,                /* Length of prefix (file name) */
            looking_for_lognames;       /* True if looking for login names */
    int     showpathn;                  /* True if we want path number */
    struct stat
            dot_statb,                  /* Stat buffer for "." */
            curdir_statb;               /* Stat buffer for current directory */
    int     dot_scan,                   /* True if scanning "." */
            dot_got;                    /* True if have scanned dot already */
    char    tilded_dir[FILSIZ + 1],     /* dir after ~ expansion */
            dir[FILSIZ + 1],            /* /x/y/z/ part in /x/y/z/f */
            name[MAXNAMLEN + 1],        /* f part in /d/d/d/f */
            extended_name[MAXNAMLEN+1], /* the recognized (extended) name */
            *entry,                     /* single directory entry or logname */
            *path,                      /* hacked PATH environment variable */
	    ftype = ' ';		/* Returned by filetype() */
#if defined(DISKLESS) || defined(DUX)
    int     showhidden;
#endif
    static DIR
            *dir_fd = NULL;
    static char
           **items = NULL;              /* file names when doing a LIST */


    if (items != NULL)
        FREE_ITEMS (items);
    if (dir_fd != NULL)
        FREE_DIR (dir_fd);

/*  Is there a ~ as the first character and no slash in the word?
*/
    looking_for_lognames = (*word == '~') && (index (word, '/') == NULL);

/*  There should be no ~ in the word, and no slash.  This can change the
    value passed in, which indicated whether or not the word was a command.
*/
    looking_for_command &= (*word != '~') && (index (word, '/') == NULL);

    if (looking_for_command)
    {
        copyn (name, word, MAXNAMLEN);
        if ((path = getenv ("PATH")) == NULL)
            path = "";
        /* setup builtins as 1st to search before PATH */

/*  This will cause the builtin directory to be opened first.  Then, if the
    PATH is not NULL, all the other directories in the path will be searched
    in turn.
*/
        copyn (dir, BUILTINS, sizeof dir);

        dirctr = -1;            /* BUILTINS -1 */
        dirflag[0] = 0;
    }
    numitems = 0;

    dot_got = FALSE;
    stat (".", &dot_statb);

cmdloop:        /* One loop per directory in PATH, if looking_for_command */

    if (looking_for_lognames)                   /* Looking for login names? */
    {
        setpwent ();                            /* Open passwd file */
        copyn (name, &word[1], MAXNAMLEN);      /* name sans ~ */
    }
    else
    {                                           /* Open directory */

/*  If we aren't looking for a command, parse the whole path into a directory
    part and a file name part.
*/
        if (!looking_for_command)
            extract_dir_and_name (word, dir, name);

/*  The dir variable has been filled in with the builtin directory name if 
    the word was a command, or with the directory part if it wasn't.  The
    tilde routine fills in a new version if the word contained a ~.  If it
    didn't, then dir just gets copied into tilded_dir.  In all cases, tilde
    returns tilded_dir.  It doesn't seem like this could ever return a 0.

    If there was no directory part to the word, then tilded_dir is NULL,
    and this causes the current directory to be opened.
*/
        if ((tilde (tilded_dir, dir) == 0) ||   /* expand ~user/... stuff */

           ((dir_fd = opendir (*tilded_dir ? tilded_dir : ".")) == NULL))
        {
            if (looking_for_command)
                goto try_next_path;
            else
	    {
                return (0);
	    }
        }
        dot_scan = FALSE;
        if (looking_for_command)
        {
            /*
             * Are we searching "."?
             */
            fstat (dir_fd->dd_fd, &curdir_statb);
            if (curdir_statb.st_dev == dot_statb.st_dev &&
                curdir_statb.st_ino == dot_statb.st_ino)
            {
                if (dot_got)                    /* Second time in PATH? */
                        goto try_next_path;
                dot_scan = TRUE;
                dot_got = TRUE;
            }
        }
    }

/*  The name was set either to the full word if it was a command, or to the
    file name part if it wasn't.
*/
    name_length = strlen (name);
    showpathn = looking_for_command && is_set(CH_listpathnum);
#if defined(DISKLESS) || defined(DUX)
    showhidden = is_set(CH_hidden);
#endif

/*  This gets a new entry in the open directory or a new login directory
    from the password file.
*/
    while (entry = getentry (dir_fd, looking_for_lognames))
    {

/*  If the name is not a prefix to the entry, try the next entry.
*/
        if (!is_prefix (name, entry))
            continue;

        /*
         * Don't match . files on null prefix match
         */

/*  Seems like this is stopping the match on something like .profile when
    the name is NULL.
*/
        if (name_length == 0 && entry[0] == '.' && !looking_for_lognames)
            continue;

        /*
         * Skip non-executables if looking for commands:
         * Only done for directory "." for speed.
         * (Benchmarked with and without:
         * With filetype check, a full search took 10 seconds.
         * Without filetype check, a full search took 1 second.)
         *                                   -Ken Greer
         */
#if defined(DISKLESS) || defined(DUX)
        if (looking_for_command && dot_scan && filetype (dir, entry, 0) != '*')
#else
        if (looking_for_command && dot_scan && filetype (dir, entry) != '*')
#endif /* not defined(DISKLESS) || defined(DUX) */
            continue;

        if (command == LIST)            /* LIST command */
        {
	    extern char *malloc ();
	    extern CHAR *calloc ();
            register int length;
            if (numitems >= MAXITEMS)
            {
		if (looking_for_lognames)
                	printf ((catgets(nlmsg_fd,NL_SETN,1, "\nYikes!! Too many names in passwd file!!\n")));
		else
                	printf ((catgets(nlmsg_fd,NL_SETN,2, "\nYikes!! Too many files!!\n")));
                break;
            }

/*  This allocates an array of 2048 pointers to strings.  However, the first
    time through, items is NULL as well, so what does items[1] mean?
*/
            if (items == NULL)
            {
                items = (char **) calloc (sizeof (items[1]), MAXITEMS + 1);
                if (items == NULL)
                    break;
            }
            length = strlen(entry) + 1;

	    if (!looking_for_command)
	    {
		length++;
		if (!looking_for_lognames)
		{
		    /* Get filetype, one of: '/', '*', '+' or ' ' */
		    ftype = filetype(tilded_dir, entry, 1);
#if defined(DISKLESS) || defined(DUX)
		    if ((!showhidden) && (ftype == '+'))
			continue;
#endif /* defined(DISKLESS) || defined(DUX) */
		}
	    }

/*  This is set if word was a command, and listpathnum is set.  The dirflag
    variable is a static array of 5 characters.  It gets set in 
    extract_dir_from_path, but was set to NULL earlier in this routine.
*/
            if (showpathn)
                length += strlen(dirflag);
	    if ((items[numitems] = (char *) malloc ((unsigned) length)) == NULL)
            {
                printf ((catgets(nlmsg_fd,NL_SETN,3, "out of mem\n")));
                break;
            }
            copyn (items[numitems], entry, MAXNAMLEN);

	    if (!looking_for_command)
	    {

/*  If we were looking for a login directory, this just appends a blank 
    since ftype was initialized to a space.
*/
                /* append filetype to end of filename. */
		items[numitems][length-2] = ftype;
		items[numitems][length-1] = '\0';
	    }

            if (showpathn)
                catn (items[numitems], dirflag, MAXNAMLEN);
            numitems++;
        }
        else                                    /* RECOGNIZE command */
	{
#if defined(DISKLESS) || defined(DUX)
		/* Get filetype, one of: '/', '*', '+' or ' ' */
	    ftype = filetype(tilded_dir, entry, 1);

/*  If the file type is a CDF but showhidden isn't set then skip this
    one.
*/
	    if ((!showhidden) && (ftype == '+'))
		continue;

#endif /* defined(DISKLESS) || defined(DUX) */

/*  Since numitems is incremented before being used, it starts as a 1.
    If multiple matches occurred, the recognize routine saves the longest
    prefix among them in extended_name.  This happens since extended_name
    is filled in each time, and recognize is called for each match with the
    current longest prefix and the next match.
*/
            if (recognize (extended_name, entry, name_length, ++numitems))
                break;
	}
    }

/*  Close the password file.
*/
    if (looking_for_lognames)
        endpwent ();

/*  Close the current open directory.
*/
    else
        FREE_DIR (dir_fd);

try_next_path:

/*  The extract_dir_from_path routine picks up the next component of the path
    and copies it into dir.  It resets path to start at the next component
    while doing the copy and returns it as well.  This construct is interesting.
    It evaluates the expression on the left of the comma, throws the value
    away, and looks at the expression on the right of the comma.  Since this
    routine sets dir, this is effectively looking at the new value of dir
    in order to decide whether or not to continue.

    Note that this continuation of the loop only occurs if word was a 
    command.
*/
    if (looking_for_command && *path &&
        (path = extract_dir_from_path (path, dir), dir))
        goto cmdloop;

    if (command == RECOGNIZE && numitems > 0)
    {

/*  It seems like this will produce: ~/users/person/stuff...
*/
        if (looking_for_lognames)
            copyn (word, "~", 1);

        else if (looking_for_command)
            word[0] = 0;

/*  This copies the dir name over word.
*/
        else
            copyn (word, dir, max_word_length);         /* put back dir part */

/*  In all cases the name matched is then appended to word.
*/
        catn (word, extended_name, max_word_length);    /* add extended name */

/*  If the command was RECOGNIZE (which was how we got into this construct)
    then CharAppend is called on each character in the string to add the
    characters in the expanded name onto the end of the input line.  (It also
    seems to echo them out.)
*/
        while (*wp) (*routine) (*wp++);
        return (numitems);
    }

    if (command == LIST)
    {

/*  This calls the sort library routine on the array of items, and uses the 
    fcompare routine to compare each item.  The items are sorted in place.
*/
        qsort (items, numitems, sizeof (items[1]), fcompare);

        print_by_column (looking_for_lognames ? NULL:tilded_dir, items,
                         numitems, looking_for_command);
        if (items != NULL)
            FREE_ITEMS (items);
    }

/*  It looks like the LIST command always returns 0, and RECOGNIZE always
    returns a number greater than 0 if it found any matches.
*/
    return (0);
}

/*
 * Object: extend what user typed up to an ambiguity.
 * Algorithm:
 * On first match, copy full entry (assume it'll be the only match)
 * On subsequent matches, shorten extended_name to the first
 * character mismatch between extended_name and entry.
 * If we shorten it back to the prefix length, stop searching.
 */

/*  Called by:

	tnx_search ()
*/
/*  Purpose:  Copy the longest prefix between extended_name and entry
	      into extended_name.  If this is the first time, just copy
	      the entire entry into extended_name.
*/
/**********************************************************************/
recognize (extended_name, entry, name_length, numitems)
char *extended_name, *entry;
/**********************************************************************/
{
    if (numitems == 1)                          /* 1st match */
        copyn (extended_name, entry, MAXNAMLEN);
    else                                        /* 2nd and subsequent matches */
    {
        register char *x, *ent;
        register int len = 0;

/*  This loop copies characters from entry into extended_name as long as they
    are the same.
*/

#ifndef NLS16
        for (x = extended_name, ent = entry; *x && *x == *ent++; x++, len++);
#else
	/* We don't want to extend name to half a character, so shorten */
	/* to the last matching whole character.  For-loop re-written as */
	/* while-loop for clarity */
        x = extended_name, ent = entry;
	while (*x && *x == *ent)  {
#ifdef EUC
		if (_CHARAT(x) > 0377)
#else  EUC
		if (CHARAT(x) > 0377)
#endif EUC

/*  If the 2nd character isn't the same don't increment to the next character.
*/
			if (*(x+1) != *(ent+1))
				break;
			else ent++, x++, len++;

/*  Increment to the next character.
*/
		ent++, x++, len++ ;
	}
#endif /* NLS16 */
        *x = '\0';                              /* Shorten at 1st char diff */

/*  The length of the original word is in name_length.
*/
        if (len == name_length)                 /* Ambiguous to prefix? */
            return (-1);                        /* So stop now and save time */
    }
    return (0);
}

/*
 * return true if check items initial chars in template
 * This differs from PWB imatch in that if check is null
 * it items anything
 */

/*  Called by:

	tnx_search ()
*/
/*  Purpose:  Return TRUE if the check string is a prefix of the template
	      string.  Otherwise return FALSE.
*/
/**********************************************************************/
static
is_prefix (check, template)
char   *check,
       *template;
/**********************************************************************/
{
    register char  *check_char,
                   *template_char;

    check_char = check;
    template_char = template;

/*  Step through each character.  If they are equal go to the next character.
    If we get to the end then we found a prefix.  If 2 characters didn't match
    the loop was broken and FALSE is returned.
*/
    do
        if (*check_char == 0)
            return (TRUE);

    while (*check_char++ == *template_char++);
    return (FALSE);
}

/*  Called by:

	tenematch ()
*/
/*  Purpose:  Return TRUE is the word being expanded is part of a command,
	      otherwise return FALSE.
*/
/**********************************************************************/
starting_a_command (wordstart, inputline)
register char *wordstart, *inputline;
/**********************************************************************/
{
    static char

/*  For some reason, it was decided that these characters always mean that
    characters directly following them are commands.  However, there can
    be intervening characters, the cmdalive set.

    Most of these characters make sense; (cmd), cmd;cmd, cmd|cmd, `cmd`
    However, this will also catch 'foreach i in (f<escape>' and should also
    get any other csh commands that have open parentheses in them.  Note
    that the loop can back up to the beginning of the line, which implies
    <prompt> cmd.
*/
            cmdstart[] = ";&(|`",

/*  These characters can come between a command start character and the word.
*/
            cmdalive[] = " \t'\"";

/*  Back up through the input line.  This can go on till worstart points to
    the first character in the line.
*/
    while (--wordstart >= inputline)
    {

/*  Note:  index == strchr - Returns the first occurrance of the character in
                             the string: strchr (string, char)
*/
        if (index (cmdstart, *wordstart))
            break;
#ifndef NLS
        if (!index (cmdalive, *wordstart))
#else

#ifdef EUC
        if (!index (cmdalive, *wordstart) || _CHARAT(wordstart) != _nl_space_alt)
#else  EUC
        if (!index (cmdalive, *wordstart) || CHARAT(wordstart) != _nl_space_alt)
#endif EUC

#endif
            return (FALSE);
    }

/*  Now, the loop above can go till wordstart points to the first character
    on the input line.  In this case, it seems like *(wordstart -1) is truly
    bogus.  
    
    39 is a forward quote.

    It looks like ';' would be caught, but '(t' wouldn't.
*/
    /* check to see if an elemnt of cmdstart is being quoted; if so,
	wordstart isn't a command but a file */

    if (index(cmdstart, *wordstart) && (*(wordstart-1) == 39) 
						      && (*(wordstart+1)== 39))
	return(FALSE);

    if (wordstart > inputline && *wordstart == '&')     /* Look for >& */
    {

/*  This loop backs up over white space and then checks against the next
    character, looking for a >, so it would catch >& as well as > &.
*/
#ifndef NLS
        while (wordstart > inputline &&
                        (*--wordstart == ' ' || *wordstart == '\t'));
#else

#ifdef EUC
        while (wordstart > inputline 
	       && (*--wordstart == ' ' || *wordstart == '\t' 
				      || _CHARAT(wordstart) == _nl_space_alt));
#else  EUC
        while (wordstart > inputline 
	       && (*--wordstart == ' ' || *wordstart == '\t' 
					|| CHARAT(wordstart) == _nl_space_alt));
#endif EUC

#endif
        if (*wordstart == '>')
                return (FALSE);
    }
    return (TRUE);
}

/*  Called by:

	tenex ()
*/
/*  Purpose:  Copy just the word to be expanded into the word buffer and
	      call tnx_search on it to expand the word.
*/
/**********************************************************************/
tenematch (inputline, inputline_size, num_read, command, command_routine)
char   *inputline;              /* match string prefix */
int     inputline_size;         /* max size of string */
int     num_read;               /* # actually in inputline */
COMMAND command;                /* LIST or RECOGNIZE */
int     (*command_routine) ();  /* either append char or display char */

/**********************************************************************/
{
    static char

/*  The delimitors did not include backquote, so `z would end up as the word
    `z but (z would generate a word z.  Seems like these two should work the
    same, so a ` was added to the list.  However, things like { weren't added
    since they are used in globbing where you would want them to be all one
    word (i.e. a{123}b).

            delims[] = " '\"\t;&<>()|^%";
*/
            delims[] = " '\"`\t;&<>()|^%";

    char word[FILSIZ + 1];
    register char *str_end, *word_start, *cmd_start, *wp, *save_last, c;
    int space_left;
    int is_a_cmd;               /* UNIX command rather than filename */

    str_end = &inputline[num_read];

   /*
    * Find LAST occurence of a delimiter in the inputline.
    * The word start is one character past it.
    */
#ifndef NLS16
    for (word_start = str_end; word_start > inputline; --word_start)
    {
	c = word_start[-1];
#ifndef NLS
	if (index(delims, c))
#else
	if (index(delims, c) || (unsigned char) c == _nl_space_alt)
#endif /* NLS */
		break;
    }
#else
    /* For HP15 data, we have to scan forword instead of backwords
       because a delim character can be the 2nd byte of a Kanji.  If
       we use old algorithm, such a directory name will cause root
       to be displayed (incidentally, dir. name a> will cause same thing */

    for (word_start = save_last = inputline; *word_start ; word_start++)
    {
#ifdef EUC
	if (*word_start == '\\' && _CHARAT(word_start+1) <= 0377 ||
	    _CHARAT(word_start) > 0377 && _CHARAT(word_start) != _nl_space_alt)  {
#else  EUC
	if (*word_start == '\\' && CHARAT(word_start+1) <= 0377 ||
	    CHARAT(word_start) > 0377 && CHARAT(word_start) != _nl_space_alt)  {
#endif EUC
		word_start++;
		continue;
	}
	if (index(delims, *word_start))
		save_last = word_start + 1;
#ifdef EUC
	else if (_CHARAT(word_start) == _nl_space_alt) {
#else  EUC
	else if (CHARAT(word_start) == _nl_space_alt) {
#endif EUC
		if (_nl_space_alt > 0377)		/* 2-byte space */
			word_start++;
		save_last = word_start +1;
	}
    }
    word_start = save_last;
#endif /* NLS16 */

/*  How much space is left in the inputline buffer that tenex is using.
*/
    space_left = inputline_size - (word_start - inputline) - 1;

    is_a_cmd = starting_a_command (word_start, inputline);

/*  This copies the word to be expanded into the word buffer and terminates
    it with a NULL.  str_end is the end of the original line read into the
    inputline buffer.
*/
    for (cmd_start = word_start, wp = word; cmd_start < str_end;
         *wp++ = *cmd_start++);
    *wp = 0;
    return tnx_search (word, wp, command, command_routine, space_left, is_a_cmd);
}

/*  Called indirectly by:

	tnx_search ()
*/
/*  Purpose:  Copy a character to the buffer that gets output during a flush ()
	      call.  Also copy it to the input line buffer being created by
	      the tenex () routine, for further processing.
*/
/**********************************************************************/
char *CharPtr;
static
CharAppend (c)
/**********************************************************************/
{
    putchar (c);
    *CharPtr++ = c;
    *CharPtr   = 0;
}

/*  Called by:

	bgetc ()
*/
/*  Purpose:  Input a buffer from the terminal using getline ().  Do file
	      expansion on it if necessary.  Continue input until no 
	      characters are read, a newline is seen, or the input buffer
	      is filled.  In some cases the input line is modified and
	      re-echo'd to the screen.
*/
/**********************************************************************/
tenex (inputline, inputline_size)
char   *inputline;
int     inputline_size;
/**********************************************************************/
{
    register int num_read;

    {
	int dummy;  /* can't use num_read since it is "register" */

	/*
	 * See if SHIN is open. If not, return 0
	 */
	if(fcntl(SHIN,F_GETFL,&dummy)<0)
	    return(0);
    }

/*  Turn off ICANON and ECHO, and set VMIN and VTIME to 1.
*/
    /* DSDe410174: setting echo off here gives rise to problems on slow
     * machines. Have moved this to the routine printprompt() in sh.c
    setup_tty (ON); */

/*  Get information about the terminal; the visual bell in particular.
*/
    termchars ();

    while(1)
    {
        register char *str_end, last_char, should_retype;
	register int numitems;
        COMMAND command;

/*  Write characters into the input buffer and return the number of characters
    read.
*/
	num_read = getline(inputline);

	if(num_read <=0)
	    break;

        last_char = inputline[num_read - 1] & 0377;

        if (last_char == '\n' || num_read == inputline_size)
            break;

        if (last_char == ESC)           /* RECOGNIZE */
        {
                command = RECOGNIZE;
                num_read--;
	}

/*  A ^D will do this.
*/
        else                            /* LIST */
	{
            command = LIST,
            putchar ('\n');
	}

/*  Put a pointer into the input string at the last character.  CharAppend
    puts characters found by other routines onto the end of this string.
*/
        CharPtr = str_end = &inputline[num_read];
        *str_end = '\0';

/*  Both putchar and CharAppend (it calls putchar) put characters into the
    flush buffer.
*/
        numitems = tenematch (inputline, inputline_size, num_read, command,
                          command == LIST ? putchar : CharAppend);
        flush ();
        if (command == RECOGNIZE)
            if (numitems != 1)                  /* Beep = No match/ambiguous */
                beep ();

        /*
         * Tabs in the input line cause trouble after a pushback.
         * tty driver won't backspace over them because column positions
         * are now incorrect. This is solved by retyping over current line.
         */

        should_retype = FALSE;
        if (index (inputline, '\t'))            /* tab in input line? */
        {
            back_to_col_1 ();
            should_retype = TRUE;
        }

/*  Note that earlier we also put a newline in the flush buffer.
*/
        if (command == LIST)                    /* Always retype after LIST */
            should_retype = TRUE;

        if (should_retype)
	{
            printprompt ();
	    flush();
	}

/*  This copies the inputline into another global buffer, pline.
*/
        pushback (inputline);

        if (should_retype)
	{

/*  This just calls redo(pline) which writes it to SHOUT.
*/
            retype ();
	}
    }

/*  Turn ICANON, ECHO back on if they were on previously.
*/
    setup_tty (OFF);
    return (num_read);
}

#define PCHAR 0100  /* bit mask to make a printing char from a control char */

#define CTLD '\004'
#define CTLF '\006'
#define CTLR '\022'
#define CTLV '\026'
#define CTLW '\027'
#define KILL  401	/* Changed values so they won't conflict with */
#define ERASE 400	/* Kanji or Roman 8 characters 	              */

#ifdef NLS16
int kj_backspace;	/* flag to remember if character backed over */
			/* was a kanji */
#endif

/*  Called by:

	tenex
*/
/*  Purpose:  Read characters into an input buffer supplied by tenex 
	      (actually by its caller).  Echo characters back to the
	      screen.  The loop is broken if the read returns a -1 and
	      errno is not EINTR, a ^D is seen, an ESCAPE is seen, a
	      newline is seen, or a carriage return is seen.  Control
	      characters are echo'd as ^<character>.  Return the number
	      of characters read.
*/
/**********************************************************************/
getline(s)
    char *s;
/**********************************************************************/
{
    unsigned char c;
    int tmpc;		/* temporary integer to hold c */
    int num;
    char *p = s;

    if (pbacked) {		/* Pushed back? */

/*  This can be set in tenex if the line expanded with taabs or was a list.
    It then got echo'd back out and was stored in pline by pushback().
*/
	strcpy(p,pline);
	p += strlen(p);
	pbacked = 0;
    }

    for(;;) {
	 if ((num = read(SHIN,&c,1)) == -1 && (errno == EINTR))
		continue;

	if (num <= 0)  /* either have zero-length reads or an error */
		return(num);

	tmpc = c;
	if (c == CURtty.c_cc[VERASE])
	    tmpc = ERASE;
	else if (c == CURtty.c_cc[VKILL])
	    tmpc = KILL;

	switch (tmpc) {
	    case CTLD:		/* ^D -- give list of matches */
		*p = '\0';
		return ((int)(p - s));

	    case '\r':		/* Carriage Return		*/
	    case '\n':
		write(SHOUT,"\n\r",2);
		c = '\n';
		/* Fall into... */

	    case ESC:		/* Escape -- name completion	*/
		if ((int) (p - s) >= BUFSIZ-1) {
		    beep();
		    break;  /* found input line boundary */
		}
		else {
		    *p++ = c;
		    *p   = '\0';
		    return ((int)(p - s));
		}

	    case ERASE:
		if (p > s) {
		    backspace(s, --p,FALSE,0);
#ifdef NLS16
		    /* 1 more decrement for the extra byte */
		    if (kj_backspace)
			p--;
#endif
		}
		else
		    beep();
		break;

	    case KILL:
		while (p > s) {
		    backspace(s, --p,TRUE, 0);
#ifdef NLS16
		    /* 1 more decrement for the extra byte */
		    if (kj_backspace)
			p--;
#endif
		}
		break;

	    case CTLW:
#ifndef NLS
		while ((*(p-1) == ' ' || *(p-1) == '\t') && p > s) {
#else

#ifdef EUC
		while ((*(p-1) == ' ' || *(p-1) == '\t' ||
			_CHARAT(p-1) == _nl_space_alt ||
			(_nl_space_alt > 0377 && _CHARAT(p-2) == _nl_space_alt)) && p > s) {
#else  EUC
		while ((*(p-1) == ' ' || *(p-1) == '\t' ||
			CHARAT(p-1) == _nl_space_alt ||
			(_nl_space_alt > 0377 && CHARAT(p-2) == _nl_space_alt)) && p > s) {
#endif EUC

#endif
		    backspace(s, --p,FALSE, 0);
#ifdef NLS16
		    /* 1 more decrement for the extra byte */
		    if (kj_backspace)
			p--;
#endif
		}
#ifndef NLS
		while (*(p-1) != ' ' && *(p-1) != '\t' && p > s) {
#else

#ifdef EUC
		while (*(p-1) != ' ' && *(p-1) != '\t' &&
			_CHARAT(p-1) != _nl_space_alt &&
			(_nl_space_alt < 0377 || _CHARAT(p-2) != _nl_space_alt)
			&& p > s) {
#else  EUC
		while (*(p-1) != ' ' && *(p-1) != '\t' &&
			CHARAT(p-1) != _nl_space_alt &&
			(_nl_space_alt < 0377 || CHARAT(p-2) != _nl_space_alt)
			&& p > s) {
#endif EUC

#endif
		    backspace(s, --p,FALSE, 0);
#ifdef NLS16
		    /* 1 more decrement for the extra byte */
		    if (kj_backspace)
			p--;
#endif
		}
		break;

	    case CTLR:
	     	write(SHOUT, "^R\n\r", 4);
		*p = '\0';
		redo(s);
		break;

	    case CTLV:			/* Literal next */
		write(SHOUT, "^\b", 2);
		read(SHIN, &c, 1);
		/* Fall into... */

gotchar:    default:
		if ((int) (p - s) >= BUFSIZ-1) {
		    beep();
		    break;   /* found input line boundary */
		}

/*  Write the character into the input buffer.
*/
		*p++ = c;

/*  This seems to echo the character back out.  If the character is a DEL
    or a control character it is echo'd as ? or ^<character>.  PCHAR is
    defined as 0100 so ^D (0104) is echo'd when 004 is seen, etc.

    A similar loop is found in putchar() (sh.print.c), except it also 
    checked for \n and \r.  These are taken care of earlier in this routine,
    and cause it to break out of the read loop.
*/
    		if ((c == 0177 || (unsigned char) c < ' ') && c != '\t') 
		  {
		    write(SHOUT, "^", 1);
		    if (c == 0177)
			c = '?';
		    else
			c |= PCHAR;
		    write(SHOUT, &c, 1);
		  }

		else
		  {

#ifdef DEBUG_TTYECHO
  printf ("getline (1): %d, writing character: %c\n", getpid (), c);
#endif
		     write(SHOUT, p-1, 1);
		  }
		break;
	}
    }
}

/*  Called by:

	getline ()
*/
/*  Purpose:  Back up a single character.  If the character is a tab what really
	      happens is that the entire line gets reprinted, minus the tab.
	      As the tenex loop runs, characters are being put into the
	      flush buffer, so flush () might have something to print.
*/
/*  Note:  kjflag never seems to get set; it always has a value of 0.
           start is the whole string.
*/
/**********************************************************************/
backspace(start, cur,killflag, kjflag)
    char *start, *cur;
    int killflag, kjflag;
/**********************************************************************/
{
    char *bstring;
    int   len;

#ifdef NLS16
    /* This code is to determine whether the character to be backed over was a kanji */
    int i = 0;
    int nbytes = cur - start + 1;	/* no. of bytes in this string */
    unsigned short lastchr;		/* the last complete char. in string */
    char *save_start = start;		/* save starting position */

    while (i < nbytes) {		/* this loop gets the last char */
#ifdef EUC
	lastchr = *start++;
	lastchr = lastchr & 0377;
	if (FIRSTof2(lastchr) && SECof2((int)*start & 0377)) {
		start++;
		i += 2;
		kj_backspace = 1;
	}
	else {
		i++;
		kj_backspace = 0;
	}
#else  EUC

/*  This advances start to point to the next whole character.  It then jumps
    by 2 bytes if this is a 2 byte character, or by one if it isn't.
*/
	lastchr = CHARADV(start);
	if (lastchr > 0377)
	    i += 2;
	else i++;
#endif EUC

    }
    start = save_start;
#endif /* NLS16 */

/*  This section is for regular characters => no tabs.
*/
    if (*cur != '\t') {		/* Tabs are a pain!  You can't back over 'em! */
	if (CURtty.c_lflag & ECHOE || killflag == TRUE) {
	    bstring = "\b \b";
	    len = 3;
        }
        else {
	    bstring = "\b";
	    len = 1;
        }

/*  The character is a DEL or a control character.
*/
        if ((*cur == 0177 || *(unsigned char *)cur < ' '))
    	    write(SHOUT, bstring, len);
#ifdef NLS16
#ifdef EUC
	if (kj_backspace && (C_COLWIDTH(lastchr) - 1))
			/* lastchr is 2-byte char (except for CS2), have to */
	    write(SHOUT, bstring, len);	     /* back over 2 bytes */
#else /* EUC */
	if (kj_backspace = (lastchr > 0377)) /* lastchr is kanji, have to */
	    write(SHOUT, bstring, len);	     /* back over 2 bytes */
#endif /* EUC */
#endif
        write(SHOUT, bstring, len);
    }

/*  This section is for tabs.
*/
    else {

/*  Replace the character we want to back over with NULL.  Then move back
    to the first column, print the prompt, flush the output buffer in case
    there was something in it, and reprint the entire line, minus the tab.
*/
	*cur = '\0';
	back_to_col_1();
	printprompt();
	flush();
	redo(start);
    }
}

/*  Called by:

	retype ()
	getline ()
	backspace ()
*/
/*  Purpose:  Copy the string s to the buffer buff and then write the buffer
	      to SHOUT.  Along the way, DEL characters are translated to
	      ? and control characters other than tabs and newlines to
	      ^<character>.
*/
/**********************************************************************/
redo(s)
    char *s;
{
    char buff[BUFSIZ];
    char *q = buff;

/*  This echo's the character in the string.  If the character is a DEL
    or a control character it is echo'd as ? or ^<character>.  PCHAR is
    defined as 0100 so ^D (0104) is echo'd when 004 is seen, etc.

    A similar loop is found in putchar() (sh.print.c), except it also 
    checked for \r. 
*/
    while (*s != '\0') {
    	if ((*s == 0177 || *(unsigned char *)s < ' ') && (*s != '\t' 
								&& *s != '\n'))
          {
	    *q++ = '^';
	    if (*s == 0177)
		*q++ = '?';
	    else
		*q++ = *s | PCHAR;
	  }
	else
	    *q++ = *s;
	++s;
    }
    *q = '\0';
    write(SHOUT,buff,strlen(buff));
}
