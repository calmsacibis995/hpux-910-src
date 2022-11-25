/* @(#) $Revision: 66.1 $ */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <values.h>
#include <ctype.h>

#ifdef STANDALONE_TEST
#   define UDEBUG
#   define console printf
#   define timer   sleep
#endif

#include "init.h"

/* Script file for "init" */
#ifdef TRUX
/*
 * INITTAB must be a character pointer for SecureWare stuff to work
 */
#ifdef UDEBUG
    char *INITTAB = "inittab";
#else
    char *INITTAB = "/etc/inittab";
#endif
#else /* not TRUX */
#ifdef UDEBUG
    char INITTAB[] = "inittab";
#else
    char INITTAB[] = "/etc/inittab";
#endif
#endif /* TRUX */

#define HASHSIZE  101

/*
 * The contents of the inittab file are kept in memory.  They are
 * available both in an ordered linked list, and in a hash table
 * (key is c_id).
 */
cmdline_t *cmd_table = NULL;
static cmdline_t *free_cmds = NULL;
static cmdline_t *hash_tbl[HASHSIZE];

#ifdef MEMORY_CHECKS
static int num_entries = 0;
/*
 * check_cmd_alloc()
 *    Check the length of the cmd table and free list to make
 *    sure that they match with num_entries.
 */
void
check_cmd_alloc()
{
    static time_t error_time = 0;
    register int list_entries = 0;
    register int free_entries = 0;
    register cmdline_t *cmd;
    time_t now;

    for (cmd = cmd_table; cmd; cmd = cmd->next)
	list_entries++;
    for (cmd = free_cmds; cmd; cmd = cmd->next)
	free_entries++;
    
    if (list_entries+free_entries != num_entries &&
	(now = time(0)) > error_time + 60)
    {
	FILE *fp = fopen("init.memory", "a");
	error_time = now;

	if (fp != NULL)
	{
	    fprintf(fp, "\nNumber of inittab table records: %d\n",
		list_entries);
	    fprintf(fp, "Number of free table records:    %d\n",
		free_entries);
	    fprintf(fp, "Allocated table records:         %d\n",
		num_entries);
	    fclose(fp);
	}
    }
}
#endif

/*
 * cmd_alloc() --
 *    allocate a cmdline_t structure.
 *
 *    Does not exit if can't malloc()!
 */
static cmdline_t *
cmd_alloc()
{
    register cmdline_t *new_c;

    if (free_cmds != (cmdline_t *)NULL)
    {
        new_c = free_cmds;
        free_cmds = free_cmds->next;
    }
    else
    {
        new_c = (cmdline_t *)malloc(sizeof(cmdline_t));
#ifdef MEMORY_CHECKS
	num_entries++;
#endif
    }

    return new_c;
}

/*
 * cmd_free() --
 *     return a process table entry to our free list.
 */
static void
cmd_free(cmd)
register cmdline_t *cmd;
{
    if (cmd->c_command)
	free(cmd->c_command);
    cmd->c_command = (char *)NULL;

    cmd->next = free_cmds;
    free_cmds = cmd;

}

/*
 * Free any previous image of /etc/inittab
 */
static void
free_inittab()
{
    cmdline_t *cmd;
    cmdline_t *next;

    for (cmd = cmd_table; cmd; cmd = next)
    {
	next = cmd->next;
	cmd_free(cmd);
    }
    cmd_table = (cmdline_t *)NULL;

    /*
     * A hash table is used to speed lookup based on key.
     * We must initialize all of the buckets in the hash table to NULL.
     */
    memset(hash_tbl, 0, sizeof hash_tbl);
}

static int inittab_entry;  /* logical line of inittab file, used
			      for error messages only */

/*
 * parseline() --
 *    read and parse one line of /etc/inittab into up to four fields
 *    that comprise it.
 *
 *    Returns the number of fields actually read or EOF.
 */
static int
parseline(fp, argv, buf)
FILE *fp;
char *argv[];
char *buf;
{
    int num_fields;
    char *sb;
    char *sb2;
    int bytes_left = MAXCMDL * 2;
    char *bufp = buf;
    int bytes_read;
    int line_len = 0;

    inittab_entry++;
    argv[0] = argv[1] = argv[2] = argv[3] = NULL;

    /*
     * Read one logical line into buf
     */
    for (;;)
    {
	if (bytes_left <= 0)
	{
	    int c;
	    int lastc = 0;

	    console("Entry %d of %s too long, ignored\n",
		inittab_entry, INITTAB);

	    /*
	     * Skip to the beginning of the next logical line
	     */
	    while ((c = fgetc(fp)) != EOF &&
		    (c != '\n' || lastc == '\\'))
		lastc = c;
	    
	    if (c == EOF)
		return EOF;

	    return parseline(fp, argv, buf);
	}

	if (fgets(bufp, bytes_left, fp) == NULL)
	{
	    if (line_len == 0)
		return EOF;
	    break;
	}
	
	line_len += strlen(bufp);

	/*
	 * If the line ends in a '\\' followed by a '\n', treat it
	 * as a continuation to the next line.
	 * We must delete both the '\\' and the '\n'.
	 */
	if (line_len >= 2 &&
	    (buf[line_len-1] == '\n' && buf[line_len-2] == '\\'))
	{
	    line_len -= 2;
	    buf[line_len] = '\0';
	    bufp = buf + line_len;
	    continue;
	}

	/*
	 * Otherwise, break out of the loop
	 */
	break;
    }

    if (buf[line_len-1] == '\n')
	buf[--line_len] = '\0';

    for (sb = buf; *sb == ' ' || *sb == '\t'; sb++)
	continue; /* Skip white space */
    
    /* skip lines that are all comments or blanks */
    if (*sb == '\0' || *sb == '#')
    {
	inittab_entry--;
	return parseline(fp, argv, buf);
    }

    /*
     * Parse the first three fields.  Any white space in these fields
     * is is ignored.  Note this allows things like "o f f" to be
     * treated like "off".  Strange, but shouldn't hurt anything.
     */
    for (num_fields = 0; num_fields < 3; num_fields++)
    {
	argv[num_fields] = sb2 = sb;
	while (*sb && *sb != ':' && *sb != '#')
	    if (*sb != ' ' && *sb != '\t')
		*sb2++ = *sb++;
	    else
		sb++;

	if (*sb != ':')
	{
	    *sb2 = '\0';
	    return num_fields + 1;
	}
	*sb2 = '\0';
	sb++;
    }
    
    /*
     * Parse the last field.  Leading or trailing white space in
     * this field is ignored.
     */
    while (*sb == ' ' || *sb == '\t')
	sb++;
    
    sb2 = buf + line_len-1;
    while (sb2 >= sb && (*sb2 == ' ' || *sb2 == '\t' || *sb2 == '\n'))
	sb2--;
    sb2++;
    *sb2 = '\0';
    
    if (*sb == '\0')
	return 3;
    
    argv[3] = sb;
    return 4;
}

/*
 * get_levels() --
 *    Build a mask for all the levels in a given level string.
 *
 *    Returns 0 if an invalid level character is found.
 */
static short
get_levels(s)
char *s;
{
    short mask;

    if (s == NULL || *s == '\0')
	return MASK0|MASK1|MASK2|MASK3|MASK4|MASK5|MASK6;

    for (mask = 0; *s; s++)
    {
	if (*s >= '0' && *s <= '6')
	    mask |= (MASK0 << (*s - '0'));
	else if (*s >= 'a' && *s <= 'c')
	    mask |= (MASKa << (*s - 'a'));
	else if (*s == 's' || *s == 'S')
	    mask |= MASKSU;
	else
	    return 0;
    }
    return mask;
}

/*
 * get_action() --
 *    convert an action string to the correct action mask
 *
 *    Returns 0 if not a valid action string.
 */
static short
get_action(s)
char *s;
{
    static char *actions[] =
    {
	"off", "respawn", "ondemand", "once", "wait", "boot",
	"bootwait", "powerfail", "powerwait", "initdefault",
	"sysinit", NULL
    };
    static short act_masks[] =
    {
	M_OFF, M_RESPAWN, M_ONDEMAND, M_ONCE, M_WAIT, M_BOOT,
	M_BOOTWAIT, M_PF, M_PWAIT, M_INITDEFAULT,
	M_SYSINIT
    };
    int i;

    for (i = 0; actions[i]; i++)
	if (strcmp(s, actions[i]) == 0)
	    return act_masks[i];
    return 0;
}

/*
 * hash_key() --
 *    given a string id value, compute a hash key.
 */
static unsigned int
hash_key(id)
unsigned char *id;
{
    register unsigned int h = 0;
    register int g;

    while (*id != '\0')
    {
	h = (h << 4) + *id++;
	if (g = h & (0xF << BITS(unsigned)-4))
	{
	    h ^= g >> BITS(unsigned)-4;
	    h ^= g;
	}
    }
    return h;
}

/*
 * find_init_entry() --
 *    Look for an entry of "id" in inittab.
 */
cmdline_t *
find_init_entry(id)
char *id;
{
    unsigned int key = hash_key(id) % HASHSIZE;
    cmdline_t *cmd;

    for (cmd = hash_tbl[key]; cmd; cmd = cmd->hash_next)
	if (id_eq(id, cmd->c_id))
	    return cmd;
    return (cmdline_t *)NULL;
}

/*
 * read_inittab() --
 *    read /etc/inittab and build a memory image of the pertinent data.
 *
 *    Only reads /etc/inittab if it has changed since it was last read.
 *    Returns TRUE if /etc/inittab was re-read.
 */
int
read_inittab()
{
    static time_t inittab_time = 0;    /* time inittab was last read */
    struct stat statb;
    FILE *fp_inittab;
    int i;
    int errnum;
    int num_fields;
    cmdline_t *cmd;
    cmdline_t *cmd_last;
    char *argv[4];
    char linbuf[MAXCMDL*2];

#ifdef DEBUG1
    debug("In read_inittab()\n");
#endif
    if (stat(INITTAB, &statb) == -1)
    {
	free_inittab();
	inittab_time = 0;
#ifdef DEBUG1
	debug("In read_inittab(), no inittab file\n");
#endif
	console("Cannot stat %s. errno: %d\n", INITTAB, errno);
	return TRUE;
    }

    /*
     * If /etc/inittab hasn't changed, just return.
     */
    if (statb.st_mtime <= inittab_time)
    {
#ifdef DEBUG1
	debug("In read_inittab(), inittab didn't change\n");
#endif
	return FALSE;
    }

#ifdef DEBUG1
    debug("In read_inittab(), inittab changed\n");
#endif
    inittab_time = statb.st_mtime;
    free_inittab();

    /*
     * Be very persistent in trying to open /etc/inittab.
     */
    for (i = 0; i < 3; i++)
    {
	if ((fp_inittab = fopen(INITTAB, "r")) != NULL)
	    break;
	else
	{
	    /*
	     * Remember the errno value and wait 5 seconds to see
	     * if the file appears.
	     */
	    errnum = errno;
	    timer(5);
	}
    }

    /*
     * If unable to open /etc/inittab, print error message and
     * return.
     */
    if (fp_inittab == NULL)
    {
        inittab_time = 0;
#ifdef DEBUG1
	debug("In read_inittab(), couldn't open inittab\n");
#endif
	console("Cannot open %s. errno: %d\n", INITTAB, errnum);
	return TRUE;
    }

#ifdef DEBUG1
    debug("In read_inittab(), reading inittab\n");
#endif
    /*
     * Now parse the lines from /etc/inittab
     */
    inittab_entry = 0;
    cmd_table = cmd_last = (cmdline_t *)NULL;
    while ((num_fields = parseline(fp_inittab, argv, linbuf)) != EOF)
    {
	int len;
	short c_levels = 0;
	short c_action = 0;

	if (num_fields < 3)
	{
	    if (num_fields >= 1)
		console("Bad entry in %s, id = %s\n", INITTAB, argv[0]);
	    else
		console("Bad entry in %s, entry %d\n",
		    INITTAB, inittab_entry);
	    continue;
	}

	/*
	 * Validate the id field, it must be from 1 to 4 characters
	 * long
	 */
	len = strlen(argv[0]);
	if (len < 1 || len > 4)
	{
	    console("Bad id field in %s, entry %d\n",
		INITTAB, inittab_entry);
	    continue;
	}

	/*
	 * Now see if this is a duplicate of an id that we already
	 * have.
	 */
	if (find_init_entry(argv[0]) != (cmdline_t *)NULL)
	{
	    console("Duplicate id field in %s, id: %s, entry %d; ignored\n",
		    INITTAB, argv[0], inittab_entry);
	    continue;
	}

	/*
	 * Validate the levels field for correctness.
	 */
	if ((c_levels = get_levels(argv[1])) == 0)
	{
	    console("Bad levels field in %s, id: %s\n",
		INITTAB, argv[0]);
	    continue;
	}

	/*
	 * If the action field is empty, assume "respawn" only if the
	 * id field is numeric, otherwise we assume "off".
	 *
	 * This is very obscure, not-documented, and I have no idea
	 * why it is like this.  -- Scott Holbrook 05/03/89
	 */
	if (argv[2] == NULL || argv[2][0] == '\0')
	{
	    if (isdigit(argv[0][0]) &&
		(argv[0][1] == '\0' ||
		 isdigit(argv[0][1])) &&
		 (argv[0][2] == '\0' ||
		  isdigit(argv[0][2])) &&
		  (argv[0][3] == '\0' ||
		   isdigit(argv[0][3])))
		c_action = M_RESPAWN;
	    else
		c_action = M_OFF;
	}
	else
	    c_action = get_action(argv[2]);

	if (c_action == 0 ||
	    ((c_levels & MASKSU) && c_action != M_INITDEFAULT))
	{
	    console("Bad action field in %s, id: %s\n",
		INITTAB, argv[0]);
	    continue;
	}

	if (c_action != M_INITDEFAULT &&
	    (num_fields != 4 || argv[3] == NULL || argv[3][0] == '\0'))
	{
	    console("Bad or missing process field in %s, id: %s\n",
		INITTAB, argv[0]);
	    continue;
	}

	/*
	 * Allocate space for this entry.  Add room for "exec " to
	 * the command string.
	 * "initdefault" entries don't have a command string, so we
	 * ignore it.
	 *
	 * We don't need the command string for "off" entries, so
	 * don't waste space on it.
	 */
	cmd = cmd_alloc();
	if (cmd && c_action != M_INITDEFAULT && c_action != M_OFF)
	    cmd->c_command = (char *)malloc(strlen(argv[3])+EXEC+1);
	
	if (cmd == (cmdline_t *)NULL ||
	    (c_action != M_INITDEFAULT &&
	     c_action != M_OFF && cmd->c_command == NULL))
	{
	    console("Running out of memory\n\
Entries of %s after number %d ignored\n",
		INITTAB, inittab_entry);
	    cmd_free(cmd);
	    fclose(fp_inittab);
	    return TRUE;
	}

	/*
	 * Fill in the values of the new cmd structure
	 */
	strncpy(cmd->c_id, argv[0], sizeof cmd->c_id);
	cmd->c_levels = c_levels;
	cmd->c_action = c_action;
	cmd->next = (cmdline_t *)NULL;

	if (c_action != M_INITDEFAULT && c_action != M_OFF)
	{
	    strcpy(cmd->c_command, "exec ");
	    strcpy(cmd->c_command + EXEC, argv[3]);
	}
	else
	    cmd->c_command = NULL;

	/*
	 * Add this entry to the hash table
	 */
	{
	    unsigned int key = hash_key(argv[0]) % HASHSIZE;

	    cmd->hash_next = hash_tbl[key];
	    hash_tbl[key] = cmd;
	}

	/*
	 * And add it to the END of the list of inittab entries.
	 */
	if (cmd_last == (cmdline_t *)NULL)
	    cmd_table = cmd;
	else
	    cmd_last->next = cmd;
	cmd_last = cmd;
    }
    fclose(fp_inittab);
#ifdef DEBUG1
    debug("Leave read_inittab()\n");
#endif
    return TRUE;
}

#ifdef STANDALONE_TEST
main()
{
    read_inittab();
}
#endif
