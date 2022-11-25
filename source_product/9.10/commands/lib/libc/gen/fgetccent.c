/* @(#) $Revision: 66.2 $ */
/*
 * fgetccent() -- return one entry from a clusterconf-like file
 *                see getccent(3C).
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define fgetccent _fgetccent
#define fgets _fgets
#define fclose _fclose
#define fopen _fopen
#define ftell _ftell
#define memchr _memchr
#define rewind _rewind
#define strcpy _strcpy
#define strlen _strlen
#define strtol _strtol
#endif

/*
 * To get the machine and iotype fields, define EXTRA_FIELDS when
 * compiling.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <cluster.h>
#ifdef EXTRA_FIELDS
#   include <sys/utsname.h>
#endif

#define MAX_LINE  512  /* maximum length of line in /etc/clusterconf */

static char *
next_field(p)
char *p;
{
    while (*p && *p != ':' && *p != '\n')
	p++;
    if (*p == '\n')
	*p = '\0';
    else if (*p)
	*p++ = '\0';
    return p;
}

static
ascii_to_hex(hex, ascii, len)
u_char *hex;
char *ascii;
int len;
{
    extern long strtol();
    int i;
    int j = 0;
    u_char temp;
    char c;
    char *p;

    for (i = 0; i < len; i++)
    {
	c = ascii[i * 2 + 2];
	ascii[i * 2 + 2] = '\0';
	temp = strtol(&ascii[i * 2], &p, 16);
	if (*p)
	    return 1;
	hex[i] = temp;
	ascii[i * 2 + 2] = c;
    }
    return 0;
}

#ifdef _NAMESPACE_CLEAN
#undef fgetccent
#pragma _HP_SECONDARY_DEF _fgetccent fgetccent
#define fgetccent _fgetccent
#endif /* _NAMESPACE_CLEAN */

struct cct_entry *
fgetccent(f)
FILE *f;
{
    extern char *memchr();
    extern long strtol();

    static struct cct_entry cct_entry;	/* results returned here */
    char line[MAX_LINE];
    char *p;
    char *end;
    char *s;
    long x;
    int first_time = (ftell(f) == 0);
#ifdef EXTRA_FIELDS
    static char machine[UTSLEN];/* cnode_machine returned here */
    static char iotype[15];	/* cnode_iotype returned here */
#endif

    if ((p = fgets(line, MAX_LINE, f)) == NULL)
	return NULL;

    /* 
     * skip empty and comment lines
     * i.e. get to the start of the next entry.
     */
again: 
    while (*p == '#' || *p == ' ' || *p == '\t' || *p == '\n')
	if (*p == '#' || *p == '\n')
	{
	    if ((p = fgets(line, MAX_LINE, f)) == NULL)
		return NULL;
	}
	else
	    p++;

    if (first_time)
    {
	/* 
	 * skip the broadcast address
	 */
	if ((p = fgets(line, MAX_LINE, f)) == NULL)
	    return NULL;
	first_time = 0;
	goto again;
    }

    /* 
     * we have the line... now parse it.
     */
    s = p;
    p = next_field(p);
    if (strlen(s) != (M_IDLEN * 2) ||
	    ascii_to_hex(cct_entry.machine_id, s, M_IDLEN))
	return NULL;

    if (p == NULL || *p == ':')	/* check for non-null cnode_id */
	return NULL;
    x = strtol(p, &end, 10);
    if (end != memchr(p, ':', strlen(p)))
	/* make sure cnode_id is numeric */
	return NULL;
    cct_entry.cnode_id = (x < 0 || x > MAXCNODE) ? 0 : x;

    p = next_field(p);
    if (p == NULL || *p == ':')	/* check for non-null cnode_name */
	return NULL;
    s = p;
    p = next_field(p);
    if (strlen(s) > 14)
	return NULL;
    strcpy(cct_entry.cnode_name, s);

    if (p == NULL || *p == ':' || p[1] != ':')
	/* make sure cnode_type is one character */
	return NULL;
    cct_entry.cnode_type = *p;

    p = next_field(p);
    if (p == NULL || *p == ':')	/* check for non-null swap_cnode */
	return NULL;
    x = strtol(p, &end, 10);
    if (end != memchr(p, ':', strlen(p)))
	/* make sure swap_cnode is numeric */
	return NULL;
    cct_entry.swap_serving_cnode = (x < 0 || x > MAXCNODE) ? 0 : x;

    p = next_field(p);
    if (p == NULL || *p == ':')	/* check for non-null kcsp */
	return NULL;
#ifdef EXTRA_FIELDS
    x = strtol(p, &end, 10);
    if (*end != ':')
#else
	x = strtol(p, &p, 10);
    if (*p != ' ' && *p != '\t' && *p != '#' && *p != '\n')
#endif
	return NULL;
    cct_entry.kcsp = (x < 0) ? 0 : x;

#ifdef EXTRA_FIELDS
    /* 
     * make sure cnode_machine is non-null and not too long
     */
    p = next_field(p);
    if (p == NULL || *p == ':')	/* check for non-null machine type */
	return NULL;
    s = p;
    p = next_field(p);
    if (strlen(s) >= UTSLEN)
	return NULL;
    strcpy(machine, s);
    cct_entry.cnode_machine = machine;

    /* 
     * make sure cnode_iotype is non-null and not too long
     */
    if (p == NULL)
	return NULL;
    s = iotype;
    while (*p && s < iotype + 15 &&
	    *p != ' ' && *p != '\t' && *p != '#' && *p != '\n')
	*s++ = *p++;
    if (s == iotype || s >= iotype + 15)
	return NULL;
    *s = '\0';
    cct_entry.cnode_iotype = iotype;
#endif

    /* 
     * Make sure that there is no trailing garbage.  Allow "# comment"
     * stuff and white space at the end of the line
     */
    for (; *p != '#' && *p != '\n'; p++)
	if (*p != ' ' && *p != '\t')
	    return NULL;

    return &cct_entry;
}
