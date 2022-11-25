#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "walkfs.h"
#include "rcstoys.h"


#ifndef TRUE
#   define TRUE  1
#   define FALSE 0
#endif

#ifdef DEBUG
#   define CHUNKSZ  15
#else
#   define CHUNKSZ  512
#endif

extern void *malloc();
extern void *realloc();

/*
 * Used to store access list
 */
struct rcs_access
{
    char *user;
    struct rcs_access *next;
};

/*
 * Used to store symbolic revision.  Since locks look the same,
 * we use it for that too.
 */
struct rcs_tag
{
    char *name;
    char *rev;
    struct rcs_tag *next;
};

struct rcsinfo {
    char *head;                 /* head revision */
    struct rcs_access *access;  /* access list */
    struct rcs_tag *tags;       /* symbolic revision information */
    struct rcs_tag *locks;      /* lock information */
    int strict;                 /* TRUE if locking is "strict" */
};

/*
 * Lexical elements of an RCS header.  The part of the header that we
 * are interested in is:
 *
 *  "head"    <whitespace> <top_revision>       ";" "\n"
 *  "access"  <whitespace> <access_list>*       ";" "\n"
 *  "symbols" <whitespace> <symbolic_revision>* ";" "\n"
 *  "locks"   <whitespace> <lockers>*           ";" [strict] ";"
 *
 * For example:
 *    head     64.3;
 *    access   user1 user2 user3;
 *    symbols  tag1:64.3 tag2:64;
 *    locks    user1 user2; strict;
 *
 * Is a valid RCS header
 */
enum lex_types
{
    l_head,    /* "head" */
    l_access,  /* "access" */
    l_symbols, /* "symbols" */
    l_locks,   /* "locks" */
    l_ws,      /* <whitespace> */
    l_trev,    /* <top revision> */
    l_ausers,  /* <access_list>* */
    l_srev,    /* <symbolic_revision>* */
    l_lusers,  /* <lockers>* */
    l_sem,     /* ";" */
    l_nl,      /* "\n" */
    l_END      /* end of state machine */
};
		
static enum lex_types states[] =
{
    l_head,    l_ws, l_trev,    l_sem, l_nl,
    l_access,  l_ws, l_ausers,  l_sem, l_nl,
    l_symbols, l_ws, l_srev,    l_sem, l_nl,
    l_locks,   l_ws, l_lusers,  l_sem,
    l_END
};

static char *no_rev = "0.0";
static char *buf;    /* beginning of buffer */
static char *bufp;   /* current beginning of free space */
static char *buflim; /* last byte of buffer (minus some scratch) */
static int  bufsz;   /* size of buffer */
static struct rcsinfo rcs_info;

/*
 * grow_buf() -- We allocate all of our structs and strings from a
 *               buffer from a single malloc() call.  This is so that
 *               we can re-use the space for each file easily and
 *               efficiently.  However, we may need to grow the buffer
 *               from time to time, so this routine does that.
 *
 *               Since we have pointers into the buffer, we must adjust
 *               all of the pointers if realloc() moves our chunk of
 *               memory on us.
 *
 *               This routine exits if realloc() fails.
 */
void
grow_buf()
{
    char *newbuf;
    long diff;
    bufsz += CHUNKSZ;

    if ((newbuf = (char *)realloc(buf, bufsz)) == NULL)
    {
	fputs("find: out of memory\n", stderr);
	exit(1);
    }
    buflim = newbuf + bufsz - 10;

    /*
     * We might not need to adjust all of our pointers into buf if
     * realloc didn't move our buffer.
     */
    if (newbuf == buf)
	return;
    
    /*
     * Drat! We have to adjust the pointers.  Looks bad, but we 
     * probably only do it once or twice at most (usually not at
     * all).
     */
    rcs_info.head += (diff = newbuf-buf);
    buf  = newbuf;
    bufp += diff;

    if (rcs_info.access != NULL)
    {
	struct rcs_access *p = rcs_info.access =
	    (struct rcs_access *)((char *)rcs_info.access + diff);

	while (p != NULL)
	{
	    if (p->user != NULL)
		p->user += diff;
	    if (p->next != NULL)
		p->next = (struct rcs_access *)((char *)p->next + diff);
	    p = p->next;
	}
    }

    if (rcs_info.tags != NULL)
    {
	struct rcs_tag *p = rcs_info.tags =
	    (struct rcs_tag *)((char *)rcs_info.tags + diff);

	while (p != NULL)
	{
	    if (p->name  != NULL)
		p->name += diff;
	    if (p->rev   != NULL)
		p->rev  += diff;
	    if (p->next != NULL)
		p->next = (struct rcs_tag *)((char *)p->next + diff);
	    p = p->next;
	}
    }

    if (rcs_info.locks != NULL)
    {
	struct rcs_tag *p = rcs_info.locks =
	    (struct rcs_tag *)((char *)rcs_info.locks + diff);

	while (p != NULL)
	{
	    if (p->name  != NULL)
		p->name += diff;
	    if (p->rev   != NULL)
		p->rev  += diff;
	    if (p->next  != NULL)
		p->next = (struct rcs_tag *)((char *)p->next + diff);
	    p = p->next;
	}
    }
}

/*
 * get_nrev() -- Read in a numeric revision string, returning a
 *               pointer to the string (allocated from buf).
 *               Return NULL if error.
 */
char *
get_nrev(fp)
FILE *fp;
{
    int c;
    char *p = bufp;

    while (((c = fgetc(fp)) >= '0' && c <= '9') || c == '.')
    {
	if (bufp >= buflim)
	    grow_buf();
	*bufp++ = c;
    }
    *bufp++ = '\0';
    (void)ungetc(c, fp);
    return p;
}

/*
 * get_srev() -- Read the symbolic part of a revision string,
 *               returning a pointer to the string (allocated from
 *               buf).  The trailing ":" is discarded.
 *
 *               Since locks look just like symbolic strings, we
 *               also use this routine to parse th "locks" stuff.
 * 
 *               Return NULL if error.
 */
char *
get_srev(fp)
FILE *fp;
{
    int c;
    char *p = bufp;

    while ((c = fgetc(fp)) != ':' &&
	   c != ' ' && c != '\t' && c != ';' && c != EOF)
    {
	if (bufp >= buflim)
	    grow_buf();
	*bufp++ = c;
    }
    *bufp++ = '\0';

    /*
     * Must be delimited by a ':'
     */
    return c == ':' ? p : NULL;
}

/*
 * read_str() -- Read a string (whitespace, ';' or '\n' terminated)
 *               returning a pointer to the string (allocated from
 *               buf).
 *               Return NULL if error.
 */
char *
read_str(fp)
FILE *fp;
{
    int c;
    char *p = bufp;

    while ((c = fgetc(fp)) != ' ' && c != '\t' && c != ';' && c != EOF)
    {
	if (bufp >= buflim)
	    grow_buf();
	*bufp++ = c;
    }
    *bufp++ = '\0';
    (void)ungetc(c, fp);

    return p;
}

/*
 * Open the rev file, return TRUE if opened (or already open), FALSE
 * if didn't open.  An error message is printed to stderr if the file
 * can't be opened.
 */
int
revopen()
{
    extern int Rcs_Newfile;
    extern struct walkfs_info *Info;
    FILE *fp = NULL;
    int len;
    int state;
    struct rcs_access *last_access = NULL;  /* so order is maintained */
    struct rcs_tag    *last_tag    = NULL;
    struct rcs_tag    *last_lock   = NULL;

    /*
     * If we have already processed this file once, return what we
     * returned last time for this file.
     */
    if (!Rcs_Newfile)
	return rcs_info.head != NULL;

    rcs_info.head = NULL;
    rcs_info.access = NULL;
    rcs_info.tags = NULL;
    rcs_info.locks = NULL;
    Rcs_Newfile = FALSE;

    /*
     * Make sure this a regular file ending in ",v".  Since RCS files
     * are at least 60 bytes long, we also check this.
     */
    if ((Info->st.st_mode & S_IFMT) != S_IFREG ||
	Info->st.st_size < 60 ||
	(len = strlen(Info->shortpath)) < 3 ||
	Info->shortpath[len-1] != 'v' || Info->shortpath[len-2] != ',')
	return FALSE;
    
    /*
     * Now open the file.
     */
    if ((fp = fopen(Info->shortpath, "r")) == NULL)
    {
	fputs("find: can't open ", stderr);
	fputs(Info->relpath, stderr);
	fputc('\n', stderr);
	return FALSE;
    }

    if (buf == NULL && (buf = (char *)malloc(bufsz = CHUNKSZ)) == NULL)
    {
	fputs("find: out of memory!\n", stderr);
	exit(1);
    }

    /*
     * Start bufp at the beginning of the buffer, set buflim to the
     * end of the buffer, but leave 10 bytes for scratch space (so
     * we don't have to always test for the corner case and can read
     * in our fixed length strings using fread() instead of a byte at
     * a time).
     */
    bufp = buf;
    buflim = bufp + bufsz - 10;

    for (state = 0; /* nothing */; state++)
    {
	int c;     /* general temp variables */
	char *s;
	char *t;

	if (bufp >= buflim)
	    grow_buf();

	switch (states[state])
	{
	case l_head:    /* "head" */
	    if (fgets(bufp, 5, fp) == NULL ||
		strncmp(bufp, "head", 4) != 0)
		goto Fail;
	    break;
	case l_access:  /* "access" */
	    if (fgets(bufp, 7, fp) == NULL ||
		strncmp(bufp, "access", 6) != 0)
		goto Fail;
	    break;
	case l_symbols: /* "symbols" */
	    if (fgets(bufp, 8, fp) == NULL ||
		strncmp(bufp, "symbols", 7) != 0)
		goto Fail;
	    break;
	case l_locks:   /* "locks" */
	    if (fgets(bufp, 6, fp) == NULL ||
		strncmp(bufp, "locks", 5) != 0)
		goto Fail;
	    break;
	case l_ws:      /* <whitespace> */
	    if (fgetc(fp) != ' ')
		goto Fail;
	    while ((c = fgetc(fp)) == ' ' || c == '\t')
		continue;
	    if (c == EOF)
		goto Fail;
	    (void)ungetc(c, fp);
	    break;
	case l_trev:    /* <top revision> */
	    if ((rcs_info.head = get_nrev(fp)) == NULL ||
		fgetc(fp) != ';')
		goto Fail;
	    (void)ungetc(';', fp);
	    break;
	case l_ausers:  /* <access_list>* */
	    if ((c = fgetc(fp)) == ';')
	    {
		(void)ungetc(';', fp);
		break;
	    }
	    else
		(void)ungetc(c, fp);

	    if ((s = read_str(fp)) == NULL || 
		((c = fgetc(fp)) != ' ' && c != '\t' && c != ';'))
		goto Fail;
	    /*
	     * Now allocate an access record
	     */
	    if (buflim - bufp <= (long)sizeof(struct rcs_access))
		grow_buf();
	    if (last_access == NULL)
		rcs_info.access = (struct rcs_access *)bufp;
	    else
		last_access->next = (struct rcs_access *)bufp;
	    last_access = (struct rcs_access *)bufp;
	    bufp += sizeof(struct rcs_access);
	    last_access->user = s;
	    last_access->next = NULL;

	    if (c != ';')  /* If not at a ';', stay in this state */
		--state;
	    else
		(void)ungetc(';', fp);
	    break;
	case l_srev:    /* <symbolic_revision>* */
	case l_lusers:  /* <lockers>* */
	    if ((c = fgetc(fp)) == ';')
	    {
		(void)ungetc(';', fp);
		break;
	    }
	    else
		(void)ungetc(c, fp);

	    if ((s = get_srev(fp)) == NULL ||
		(t = get_nrev(fp)) == NULL ||
		((c = fgetc(fp)) != ' ' && c != '\t' && c != ';'))
		goto Fail;
	    /*
	     * Now allocate a tag record
	     */
	    if (buflim - bufp <= (long)sizeof(struct rcs_tag))
		grow_buf();

	    if (states[state] == l_srev)
	    {
		if (last_tag == NULL)
		    rcs_info.tags = (struct rcs_tag *)bufp;
		else
		    last_tag->next = (struct rcs_tag *)bufp;
		last_tag = (struct rcs_tag *)bufp;
		bufp += sizeof(struct rcs_tag);
		last_tag->name = s;
		last_tag->rev  = t;
		last_tag->next = NULL;
	    }
	    else
	    {
		if (last_lock == NULL)
		    rcs_info.locks = (struct rcs_tag *)bufp;
		else
		    last_lock->next = (struct rcs_tag *)bufp;
		last_lock = (struct rcs_tag *)bufp;
		bufp += sizeof(struct rcs_tag);
		last_lock->name = s;
		last_lock->rev  = t;
		last_lock->next = NULL;
	    }

	    if (c != ';')  /* If not at a ';', stay in this state */
		--state;
	    else
		(void)ungetc(';', fp);
	    break;
	case l_sem:     /* ";" */
	    if (fgetc(fp) != ';')
		goto Fail;
	    break;
	case l_nl:      /* "\n" */
	    if (fgetc(fp) != '\n')
		goto Fail;
	    break;
	case l_END:     /* end of state machine */
	    /*
	     * See if the next token is a "\n", if it is, we have
	     * non-strict locking, otherwise assume "strict".
	     */
	    while ((c = fgetc(fp)) == ' ' || c == '\t')
		continue;
	    rcs_info.strict = (c != '\n');
	    fclose(fp);
	    return TRUE;
	}
    }

Fail:
    rcs_info.head = NULL;
    fclose(fp);
    return 0;
}

/*
 * get_tag() -- search a given tag list (revisions or locks) and search
 *              for a matching entry.  The special value ":" matches
 *              the head revision (if any).  Returns NULL if not found.
 *
 */
static char *
get_tag(tags, str)
struct rcs_tag *tags;
char *str;
{
    if (str[0] == ':' && str[1] == '\0')
	return rcs_info.head;

    /*
     * If str is a revision, search revisions and return the "name"
     * for the first match that we find.
     */
    if (*str >= '0' && *str <= '9')
    {
	while (tags != NULL && strcmp(tags->rev, str) != 0)
	    tags = tags->next;
	return tags ? tags->name : NULL;
    }

    /*
     * str is a symbolic name (revisions) or locking user (locks).  Find
     * the corresponding revision number
     */
    while (tags != NULL && strcmp(tags->name, str) != 0)
	tags = tags->next;
    return tags ? tags->rev : NULL;
}

/*
 * revcmp() -- compare two numeric revision strings, indicating if they
 *             are equal (0), rev1 < rev2 (-1) or rev1 > rev2 (+1).
 *
 *             Revision strings are of the form "x", "x.y", "x.y.z",
 *             etc.
 * 
 * Some examples:
 *
 *    64.5  > 64.3
 *    64.5 == 64.5
 *    64.5  < 64.6
 *    64.5  < 64.5.7  (because 64.5 refers to the leaf of the 64.5
 *                     branch and 64.5.7 might not be the leaf.
 *                     This is really a kludge until I get the branch
 *                     and leaf stuff working properly)
 */
static int
revcmp(rev1, rev2)
char *rev1;
char *rev2;
{
    if (strcmp(rev1, rev2) == 0)
	return 0;

    for (;;)
    {
	int n1 = atoi(rev1);
	int n2 = atoi(rev2);

	if (n1 < n2)
	    return -1;
	if (n1 > n2)
	    return 1;
	/*
	 * These components are equal, go to the next component.
	 */
	while (*rev1 && *rev1 != '.')
	    rev1++;
	if (*rev1)
	    rev1++;
	while (*rev2 && *rev2 != '.')
	    rev2++;
	if (*rev2)
	    rev2++;

	/*
         * If we ran out of rev1, return 0 if we also ran out of
         * rev2, otherwise '<'.
         *
         * If we ran out of rev2 then return '>'.
         */
        if (*rev1 == '\0')
            return *rev2 == '\0' ? 0 : -1;
        if (*rev2 == '\0')
            return 1;
    }
}

int
rcsstrict()
{
    if (!revopen())
	return FALSE;
    return rcs_info.strict;
}

int
rcsrev(p)
register struct { int f; struct rcs_arg *syms; int op; } *p;
{
    char *tag1 = p->syms->tag1;
    char *tag2 = p->syms->tag2;
    int cmp;

    if (!revopen())
	return FALSE;

    if (*tag1 < '0' || *tag1 > '9')
    {
	if ((tag1 = get_tag(rcs_info.tags, tag1)) == NULL)
	    tag1 = no_rev;
    }
    
    if (*tag2 < '0' || *tag2 > '9')
    {
	if ((tag2 = get_tag(rcs_info.tags, tag2)) == NULL)
	    tag2 = no_rev;
    }
    
    cmp = revcmp(tag1, tag2);

    switch (p->op)
    {
    case RCS_LT:
	return cmp < 0;
    case RCS_LE:
	return cmp <= 0;
    case RCS_EQ:
	return cmp == 0;
    case RCS_GE:
	return cmp >= 0;
    case RCS_GT:
	return cmp > 0;
    case RCS_NE:
	return cmp != 0;
    }
}

rcsprint()
{
    struct rcs_access *acc;
    struct rcs_tag *tag;

    if (!revopen())
	return FALSE;

    fputs("head     ", stdout);
    fputs(rcs_info.head, stdout);
    fputs(";\naccess  ", stdout);
    if ((acc = rcs_info.access) != NULL)
	for (; acc != NULL; acc = acc->next)
	{
	    fputc(' ', stdout);
	    fputs(acc->user, stdout);
	}
    else
	fputc(' ', stdout);
    fputs(";\nsymbols ", stdout);
    if ((tag = rcs_info.tags) != NULL)
	for (; tag != NULL; tag = tag->next)
	{
	    fputc(' ', stdout);
	    fputs(tag->name, stdout);
	    fputc(':', stdout);
	    fputs(tag->rev, stdout);
	}
    else
	fputc(' ', stdout);
    fputs(";\nlocks   ", stdout);
    if ((tag = rcs_info.locks) != NULL)
	for (; tag != NULL; tag = tag->next)
	{
	    fputc(' ', stdout);
	    fputs(tag->name, stdout);
	    fputc(':', stdout);
	    fputs(tag->rev, stdout);
	}
    else
	fputc(' ', stdout);

    if (rcs_info.strict)
	fputs(";  strict", stdout);

    fputs(";\n", stdout);
}

/*
 * rcslocked() -- return true if the pattern "patt" matches any
 *                lockers or locked revisions.
 */
int
rcslocked(p)
register struct { int f; char *pattern; void *foo; } *p;
{
    struct rcs_tag *tag;

    if (!revopen())
	return FALSE;

    for (tag = rcs_info.locks; tag != NULL; tag = tag->next)
	if (fnmatch(p->pattern, tag->name, 0) == 0 ||
	    fnmatch(p->pattern, tag->rev, 0) == 0)
	    return TRUE;
    return FALSE;
}

/*
 * rcsaccess() -- return true if the pattern "patt" matches any user
 *                in the access list
 */
int
rcsaccess(p)
register struct { int f; char *pattern; void *foo; } *p;
{
    struct rcs_access *acc;

    if (!revopen())
	return FALSE;

    for (acc = rcs_info.access; acc != NULL; acc = acc->next)
	if (fnmatch(p->pattern, acc->user, 0) == 0)
	    return TRUE;
    return FALSE;
}

/*
 * escape() -- return the character represented by a '\' escape sequence
 *             pointed to by *psb (no leading '\'), updating *psb to
 *             one before the next character.
 */
static int
escape(psb)
char **psb;
{
    int c = **psb;

    switch (c)
    {
    case 'n': 
	return '\n';
    case 't': 
	return '\t';
    case 'b': 
	return '\b';
    case 'r': 
	return '\r';
    case 'f': 
	return '\f';
    case '\\': 
	return '\\';
    case '\'': 
	return '\'';
    default: 
	if (c < '0' || c >= '7')
	    return c;
	else
	{
	    int i = 3;
	    int c = 0;

	    do
	    {
		c = c * 8 + **psb - '0';
		(*psb)++;
		i--;
	    } while (i && **psb >= 0 && **psb <= '7');
	    (*psb)--;
	    return c;
	}
    }
}

/*
 * rcsprintf() -- print formatted information about an rcs file
 *
 *  %%            -- the character '%'
 *  %f            -- current path name
 *  %b            -- current base name
 *  %h            -- head revision
 *  %s            -- "strict" or "non-strict" based on rcs_info.strict
 *  %A            -- the access list (blank seperated)
 *  %R            -- all symbolic revision information
 *  %L            -- all lock information (except strict/non-strict)
 *  %:<string>:r  -- revision for a given symbolic name
 *  %:<string>:l  -- revision locked by a given person
 *  %#<string>#r  -- all symbolic names attached to a given revision
 *                   (blank seperated)
 *  %#<string>#l  -- person locking a given revision
 *
 * Also recognizes standard backslash escape sequences.
 *
 */
int
rcsprintf(p)
register struct { int f; char *fmt; void *foo; } *p;
{
    extern struct walkfs_info *Info;
    char *sb;

    if (!revopen())
	return FALSE;

    for (sb = p->fmt; sb && *sb != '\0'; sb++)
    {
	if (*sb == '\\')
	{
	    sb++;
	    if (*sb == '\0')
		return TRUE;
	    fputc(escape(&sb), stdout);
	    continue;
	}

	if (*sb != '%')
	{
	    fputc(*sb, stdout);
	    continue;
	}

	switch (*(++sb))
	{
	case '%':
	    fputc('%', stdout);
	    break;
	case 'f':
	    fputs(Info->relpath, stdout);
	    break;
	case 'b':
	    fputs(Info->basep, stdout);
	    break;
	case 'h':
	    fputs(rcs_info.head ? rcs_info.head : no_rev, stdout);
	    break;
	case 's':
	    if (!rcs_info.strict)
		fputs("non-", stdout);
	    fputs("strict" , stdout);
	    break;
	case 'A':
	    {
		struct rcs_access *acc;
		int put_space = FALSE;

		for (acc=rcs_info.access; acc != NULL; acc = acc->next)
		{
		    if (put_space)
			fputc(' ', stdout);
		    else
			put_space = TRUE;
		    fputs(acc->user, stdout);
		}
	    }
	    break;
	case 'R':
	case 'L':
	    {
		struct rcs_tag *tag;
		int put_space = FALSE;

		if (*sb == 'R')
		    tag = rcs_info.tags;
		else
		    tag = rcs_info.locks;

		for (; tag != NULL; tag = tag->next)
		{
		    if (put_space)
			fputc(' ', stdout);
		    else
			put_space = TRUE;
		    fputs(tag->name, stdout);
		    fputc(':', stdout);
		    fputs(tag->rev, stdout);
		}
	    }
	    break;
	case ':':
	case '#':
	    {
		extern char *strchr();
		struct rcs_tag *tags;
		char *str = sb+1;
		char *endp = strchr(str, *sb);

		if (endp == NULL ||
		    (*(endp+1) != 'r' && *(endp+1) != 'l'))
		{
		    fputs("find: bad -rcsprintf format string\n",
			stderr);
		    exit(1);
		}

		*endp = '\0'; /* temporary, restored later */
		tags = (*(endp+1) == 'r') ?
			      rcs_info.tags : rcs_info.locks;

		if (*sb == '#')
		{
		    /*
		     * Search for a label associated with a revision.
		     * If we are searching the symbols list, we may
		     * have multiple matches, so we must search
		     * everything.  If we are searching the lock list,
		     * there can only be one lock, so we quit as soon
		     * as we get a match.
		     */
		    int put_space = FALSE;
		    char *nrev = str;

		    /*
		     * If str is not numeric, it must be a symbolic
		     * name, look up the numeric equivelant.  We
		     * also exclude this symbolic name from printing.
		     * If there is no revision for the given symbolic
		     * name, just quit.
		     */
		    if ((*str < '0' || *str >= '9') &&
			((nrev=get_tag(rcs_info.tags, str)) == NULL))
			break;

		    for (; tags != NULL; tags = tags->next)
			if (strcmp(tags->rev, nrev) == 0 &&
			    (nrev==str || strcmp(tags->name, str) != 0))
			{
			    if (put_space)
				fputc(' ', stdout);
			    else
				put_space = TRUE;
			    fputs(tags->name, stdout);
			    if (*(endp+1) == 'l')
				break;
			}
		}
		else
		{
		    /*
		     * Search by name for a numeric revision, since
		     * you can only have one revision/name, we quit
		     * as soon as we find it.
		     */
		    for (; tags != NULL; tags = tags->next)
			if (strcmp(tags->name, str) == 0)
			{
			    fputs(tags->rev, stdout);
			    break;
			}
		}
		*endp = *sb;   /* restore fmt string we modified */
		sb = endp + 1; /* move current pos to the 'r' or 'l' */
	    }
	    break;
	case '\0':
	    return TRUE;
	default:
	    if (*sb)
		fputc(*sb, stdout);
	    break;
	}
    }
}
