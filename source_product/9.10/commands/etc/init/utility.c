/*
 * $Header: utility.c,v 70.1 91/09/11 07:48:40 hmgr Exp $
 *
 * utlity.c -- utility routines used by "init"
 */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <utmp.h>
#include <ctype.h>
#include <magic.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include "init.h"

/*
 * mask() --
 *    convert an init level to a level mask
 */
int
mask(level)
int level;
{
    switch (level)
    {
    case LVLQ:
	return 0;
    case LVL0:
	return MASK0;
    case LVL1:
	return MASK1;
    case LVL2:
	return MASK2;
    case LVL3:
	return MASK3;
    case LVL4:
	return MASK4;
    case LVL5:
	return MASK5;
    case LVL6:
	return MASK6;
    case SINGLE_USER:
	return MASKSU;
    case LVLa:
	return MASKa;
    case LVLb:
	return MASKb;
    case LVLc:
	return MASKc;
    default:
	return -1;
    }
}

/*
 * level() -- convert an init level to an init state character
 */
char
level(state)
int state;
{
    switch (state)
    {
    case LVL0:
	return '0';
    case LVL1:
	return '1';
    case LVL2:
	return '2';
    case LVL3:
	return '3';
    case LVL4:
	return '4';
    case LVL5:
	return '5';
    case LVL6:
	return '6';
    case SINGLE_USER:
	return 'S';
    case LVLa:
	return 'a';
    case LVLb:
	return 'b';
    case LVLc:
	return 'c';
    default:
	return '?';
    }
}

static char init_levels[] = "0123456sS";
static int  init_states[]  = { LVL0, LVL1, LVL2, LVL3, LVL4, LVL5, LVL6,
			       SINGLE_USER, SINGLE_USER };

/*
 * ltos() --
 *    convert an init level to an init state
 */
int
ltos(l)
{
    register int i;

    for (i = 0; i < sizeof init_levels; i++)
	if (l == init_levels[i])
	    return init_states[i];
    return BOGUS_STATE;
}

/*
 * stol() --
 *    convert an init state to an init level
 */
int
stol(s)
{
    register int i;

    for (i = 0; i < sizeof init_states; i++)
	if (s == init_states[i])
	    return init_levels[i];
    return BOGUS_LEVEL;
}

/*
 * fdup() --
 *    Dup the file descriptor for the specified stream and then
 *    convert it to a stream pointer with the modes of the original
 *    stream pointer.
 */
FILE *
fdup(fp)
register FILE *fp;
{
    register int newfd;
    register char *mode;

    if ((newfd = dup(fileno(fp))) != -1)
    {

	/*
	 * Determine the proper mode.  If the old file was _IORW, then
	 * use the "r+" option, if _IOREAD, the "r" option, or if _IOWRT
	 * the "w" option.  Note that since none of these force an lseek
	 * by "fdopen", the dupped file pointer will be at the same spot
	 * as the original.
	 */
	if (fp->_flag & _IORW)
	    mode = "r+";
	else if (fp->_flag & _IOREAD)
	    mode = "r";
	else if (fp->_flag & _IOWRT)
	    mode = "w";
	else
	{
	    /*
	     * Something is wrong, close dupped descriptor and
	     * return NULL.
	     */
	    close(newfd);
	    return NULL;
	}

	/*
	 * Now have fdopen finish the job of establishing a new file
	 * pointer.
	 */
	return fdopen(newfd, mode);
    }
    return NULL;
}

/*
 * "prog_name" searches for the word or unix path name and
 * returns a pointer to the last element of the pathname.
 */
char *
prog_name(string)
register char *string;
{
    register char *ptr, *ptr2;
    struct utmp *dummy;		/* Used only to get size of ut_user */
    static char word[sizeof (dummy->ut_user) + 1];

    /*
     * Search for the first word skipping leading spaces and tabs.
     */
    while (*string == ' ' || *string == '\t')
	string++;

    /*
     * If the first non-space non-tab character is not one allowed in
     * a word, return a pointer to a null string, otherwise parse the
     * pathname.
     */
    if (*string != '.' && *string != '/' && *string != '_' &&
	(*string < 'a' || *string > 'z') &&
	(*string < 'A' || *string > 'Z') &&
	(*string < '0' || *string > '9'))
	return "";

    /*
     * Parse the pathname looking forward for '/', ' ', '\t', '\n' or
     * '\0'.  Each time a '/' is found, move "ptr" to one past the
     * '/', thus when a ' ', '\t', '\n', or '\0' is found, "ptr" will
     * point to the last element of the pathname.
     */
    for (ptr = string; *string != ' ' && *string != '\t' &&
		       *string != '\n' && *string != '\0'; string++)
    {
	if (*string == '/')
	    ptr = string + 1;
    }

    /*
     * Copy out up to the size of the "ut_user" array into "word",
     * null terminate it and return a pointer to it.
     */
    for (ptr2 = word; ptr2 < word + sizeof (dummy->ut_user) &&
		      ptr < string;)
	*ptr2++ = *ptr++;

    *ptr2 = '\0'; /* Add null to end of string. */
    return word;
}

#define SHELL_EXEC 0x2321

/*
 * valid_exec() --
 *    return TRUE if the absolute path name "path" is a valid
 *    executable file.
 *    If no_script is TRUE, '#!' scripts are not considered legal.
 */
int
valid_exec(path, no_script)
char *path;
int no_script;
{
    struct stat st;
    MAGIC magic;
    int fd;

    /*
     * We make sure that the file exists, is a regular file, is at
     * least large enough to have a magic number and is executable.
     * We don't worry about owner or group, since we always run as
     * root and root is allowed to exec any file as long as one of
     * its execute bits is on.
     */
    if (stat(path, &st) == -1 || 
	(st.st_mode & S_IFMT) != S_IFREG ||
	st.st_size < MAGIC_OFFSET + sizeof (MAGIC) ||
	(st.st_mode & 0111) == 0 ||
        (fd = open(path, O_RDONLY)) == -1)
	return FALSE;

#if MAGIC_OFFSET == 0
    /*
     * Read in the magic number.
     */
    if (read(fd, &magic, sizeof (MAGIC)) != sizeof (MAGIC))
#else
    /*
     * Read in the first 2 bytes, determine if it is '#!'
     */
    if (read(fd, &magic, 2) != 2)
#endif
    {
	close(fd);
	return FALSE;
    }
    
    /*
     * If it is a '#!' script, figure out what the interpreter is and
     * see if it is a valid executable.
     */
    if (magic.system_id == SHELL_EXEC)
    {
	char buf[MAXPATHLEN];
	int i = 1;
	int n;
	char c;

	/*
	 * If no_scrpit is TRUE, we aren't allowing interpreted scripts.
	 */
        if (no_script)
	{
	    close(fd);
	    return FALSE;
	}

#if MAGIC_OFFSET == 0
	/*
	 * Get back to the correct place in the file, since earlier we
	 * read more bytes than we needed to.
	 */
	lseek(fd, 2, SEEK_SET);
#endif

	/* Skip the white space */
	while ((n = read(fd, &c, 1)) == 1 && (c == ' ' || c == '\t'))
	    continue;

	/* If there is no string, it won't work */
	if (n != 1 || c == '\n')
	{
	    close(fd);
	    return FALSE;
	}

	/*
	 * Read in the path of the interpreter, up to the first white
	 * space or end of line.
	 */
	buf[0] = c;
	while (i < MAXPATHLEN &&
	       (n = read(fd, &c, 1)) == 1 &&
	       c != ' ' && c != '\t' && c != '\n' && c != '\0')
	    buf[i++] = c;

	close(fd);
	
	/*
	 * If the path is too long, we can't exec it.
	 */
	if (i >= MAXPATHLEN || n != 1)
	    return FALSE;

	/*
	 * Null terminate the interpreter path, and see if the
	 * interpreter is a valid executable.
	 */
	buf[i] = '\0';
	return valid_exec(buf, TRUE);
    }
    
#if MAGIC_OFFSET != 0
    /*
     * Since the magic number isn't in the same place as the '#!', we
     * need to seek to where the magic number is and read it.
     */
    if (lseek(fd, MAGIC_OFFSET, SEEK_SET) == -1 ||
        read(fd, &magic, sizeof (MAGIC)) != sizeof (MAGIC))
    {
	close(fd);
	return FALSE;
    }
#endif

    close(fd);

    if (magic.file_type != EXEC_MAGIC &&
	magic.file_type != SHARE_MAGIC &&
	magic.file_type != DEMAND_MAGIC)
	return FALSE;

#ifdef __hp9000s800
    if (magic.system_id == _PA_RISC1_0_ID)
	return TRUE;
    else
    {
	unsigned long my_cpu = (unsigned long)sysconf(_SC_CPU_VERSION);

        if (my_cpu >= CPU_PA_RISC1_1)
            return (magic.system_id >= _PA_RISC1_1_ID &&
		    magic.system_id <= my_cpu);
    }
#else
    return (magic.system_id == HP9000S200_ID ||
	    magic.system_id == HP98x6_ID);
#endif
}

#ifdef UDEBUG
#   define DBG_FILE "debug"
#endif
#ifdef DEBUGGER
#   ifndef DBG_FILE
#      define DBG_FILE "/etc/debug"
#   endif

/*
 * debug() --
 *    print a message to the debug file, DBG_FILE
 */
void
debug(format, arg1, arg2, arg3, arg4, arg5, arg6)
char *format;
int arg1, arg2, arg3, arg4, arg5, arg6;
{
    extern int errno;
    register FILE *fp;

    if ((fp = fopen(DBG_FILE, "a+")) == NULL)
    {
	console("Can't open \"%s\".  errno: %d\n", DBG_FILE, errno);
	return;
    }
    fprintf(fp, format, arg1, arg2, arg3, arg4, arg5, arg6);
    fclose(fp);
}

/*
 * C() --
 *    convert a character to a printable representation
 */
char *
C(id)
register char *id;
{
    static char answer[12];
    register char *ptr;
    register int i;

    for (i = 4, ptr = &answer[0]; --i >= 0; id++)
    {
	if (isprint(*id) == 0)
	{
	    *ptr++ = '^';
	    *ptr++ = *id + 0100;
	}
	else
	    *ptr++ = *id;
    }
    *ptr++ = '\0';
    return answer;
}
#endif
