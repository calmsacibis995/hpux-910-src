static char *HPUX_ID = "@(#) $Revision: 64.3 $";

#include <stdio.h>
#include <ctype.h>

/*
 * CLIENT_CODE is the code used for remoteroot systems in the 4th
 * field of /etc/clusterconf.  
 * MAXCSP is the maximum allowable number of CSP's
 */
#define CLIENT_CODE	'c'
#define MAXCSP		16

extern char *malloc();
extern char *ltoa();

char *conffile = "/etc/clusterconf";

#define MAXLINE  256  /* Maximum length of line in clusterconf file */

struct listnode
{
    int line;		    /* line number of entry */
    char *data[6];	    /* 6 data fields from clusterconf line */
    struct listnode *next;  /* next item in list */
} ;

main(argc, argv)
int argc;
char **argv;
{
    char line[MAXLINE];
    char *data[100];
    int i, nfield;
    int firsttime = 1;
    int lineno = 0;
    int len;
    struct listnode *list_head = NULL;
    FILE *fp;

    if (argc > 2)
	usage();
    if (argc == 2)
	conffile = argv[1];
    if ((fp = fopen(conffile, "r")) == NULL)
    {
	perror(conffile);
	exit(1);
    }

    while (fgets(line, MAXLINE, fp) != NULL)
    {
	lineno++;
	if (*line == '#')	/* skip comment lines */
	    continue;

	if (firsttime)
	{			/* check broadcast address */
	    firsttime = 0;
	    if ((nfield = parseline(line, data)) != 1 && nfield != 2 ||
		strlen(data[0]) != 12)
	    {
		bad_msg("Bad clustercast entry", lineno);
		continue;
	    }

	    for (i = 0; i < 12; i++)
		if (!isxdigit(data[0][i]))
		{
		    bad_msg("Bad clustercast address", lineno);
		    break;
		}
	}
	else
	{
	    /* 
	     * check cnode entry line
	     */
	    if (parseline(line, data) != 6)
	    {
		bad_msg("Wrong number of fields", lineno);
		continue;
	    }

	    /* add to list for duplicate checking */
	    add_line_data(lineno, data, &list_head);
	    if (strlen(data[0]) != 12)
		bad_msg("Bad machine ID", lineno);
	    else
		for (i = 0; i < 12; i++)
		    if (!isxdigit(data[0][i]))
		    {
			bad_msg("Bad machine ID", lineno);
			break;
		    }

	    /* 
	     * check cnode ID field
	     */
	    len = strlen(data[1]);
	    for (i = 0; i < len; i++)
		if (!isdigit(data[1][i]))
		{
		    bad_msg("Bad cnode ID", lineno);
		    break;
		}

	    i = atoi(data[1]);
	    if (i < 1 || i > 255)
		bad_msg("Bad cnode ID", lineno);

	    /* 
	     * check cnode name
	     */
	    if ((len = strlen(data[2])) < 1 || len > 8)
		bad_msg("Bad cnode name", lineno);

	    /* 
	     * check system type field
	     */
	    if (strlen(data[3]) != 1 ||
		(*data[3] != 'r' && *data[3] != CLIENT_CODE))
		bad_msg("Bad cnode type", lineno);

	    /* 
	     * check for numeric entry of swap server.
	     * verification of rootserver ID will be done
	     * later.
	     */
	    len = strlen(data[4]);
	    for (i = 0; i < len; i++)
		if (!isdigit(data[4][i]))
		{
		    bad_msg("Bad swap server ID", lineno);
		    goto check_csp;
		}
	    if ((i = atoi(data[4])) < 1 || i > 255)
		bad_msg("Bad swap server ID", lineno);

	check_csp: 
	    /* 
	     * check the default number of CSP's
	     */
	    len = strlen(data[5]);
	    for (i = 0; i < len; i++)
		if (!isdigit(data[5][i]))
		{
		    bad_msg("Bad default number of CSPs", lineno);
		    continue;
		}
	    i = atoi(data[5]);
	    if (i < 0 || i > MAXCSP)
		bad_msg("Bad default number of CSPs", lineno);
	}
    }
    do_other_checks(list_head);
}

usage()
{
    fputs("ccck: Usage: ccck [file]\n", stderr);
    exit(2);
}


/*
 * add_line_data stores the parsed line from the clusterconf file
 * for later checking.
 */
add_line_data(lineno, data, head)
int lineno;
char *data[];
struct listnode **head;
{
    struct listnode *p;
    int i;

    p = (struct listnode *)malloc(sizeof (struct listnode));
    if (p == NULL)
    {
	fputs("ccck: malloc error, aborting...\n", stderr);
	exit(1);
    }

    p->line = lineno;
    for (i = 0; i < 6; i++)
	p->data[i] = data[i];
    p->next = *head;
    *head = p;
}


/*
 * do_other_checks --
 *   performs the following checks on the clusterconf data:
 *      no duplication of machine ID, cnode name, and cnode ID.
 *      verification of swap server.
 */
do_other_checks(list)
struct listnode *list;
{
    struct listnode *p, *q;
    int i, swapper;
    int rootcnode;

    /* 
     * get the cnode ID of the root server and make sure there is
     * only one root server.
     */
    if (rootcnode = getrootcnode(list))
    {				/* only check swap server if valid root
				   entry */
	for (p = list; p; p = p->next)
	    if (atoi(p->data[1]) != (swapper = atoi(p->data[4]))
		    && swapper != rootcnode)
		bad_msg("Bad swap server", p->line);
    }

    /* 
     * check uniqueness of machine ID, cnode name and cnode ID
     */
    for (p = list; p && p->next; p = p->next)
	for (q = p->next; q; q = q->next)
	{
	    if (strcmp(p->data[0], q->data[0]) == 0)
		bad_msg2("Duplicate machine IDs", p->line, q->line);

	    if (strcmp(p->data[1], q->data[1]) == 0)
		bad_msg2("Duplicate cnode IDs", p->line, q->line);

	    if (strcmp(p->data[2], q->data[2]) == 0)
		bad_msg2("Duplicate cnode names", p->line, q->line);
	}
}

/*
 * getrootcnode --
 *    searches the list of data to find the root server's cnode ID.
 *    It also ckecks for multiple root server entries.
 */
getrootcnode(list)
struct listnode *list;
{
    struct listnode *p;
    int index = 0, rootcnode, lines[256];
    int i;

    for (p = list; p; p = p->next)
    {
	if (p->data[3][0] != 'r')
	    continue;
	if (index == 0)
	    rootcnode = atoi(p->data[1]);
	lines[index++] = p->line;
    }

    switch (index)
    {
    case 0: 
	fputs("No root server found in clusterconf file\n", stderr);
	return 0;
    case 1: 
	return rootcnode;
    default: 
	fputs("Multiple root server entries found at lines", stderr);
	for (i = 0; i < index; i++)
	{
	    fputc(' ', stderr);
	    fputs(ltoa(lines[i]), stderr);
	}
	fputc('\n', stderr);
	return 0;
    }
}

/*
 * parseline --
 *    takes a line of colon separated fields and separates it into
 *    fields.  The return value is the number of fields in the line.
 */
int
parseline(line, fields)
char *line;
char *fields[];
{
    int i;
    char *s, *prev;
    char ch;
    int len;

    prev = line;
    i = 0;
    for (s = line; *s && *s != '\n'; s++)
    {
	if (*s != ':')
	    continue;
	ch = *s;
	*s = 0;
	fields[i] = malloc(strlen(prev) + 1);
	strcpy(fields[i], prev);
	i++;
	prev = s + 1;
	*s = ch;
    }

    len = strlen(prev);
    if (prev[len - 1] == '\n')
    {
	prev[len - 1] = 0;	/* remove trailing newline */
	len--;
    }
    fields[i] = malloc(len + 1);
    strcpy(fields[i], prev);
    return ++i;
}

/*
 * bad_msg -- printf("%s at line %d\n", msg, lineno)
 */
bad_msg(msg, lineno)
char *msg;
int lineno;
{
    fputs(msg, stderr);
    fputs(" at line ", stderr);
    fputs(ltoa(lineno), stderr);
    fputc('\n', stderr);
}

/*
 * bad_msg -- printf("%s at lines %d and %d\n", msg, lineno1, lineno2)
 */
bad_msg2(msg, lineno1, lineno2)
char *msg;
int lineno1;
int lineno2;
{
    fputs(msg, stderr);
    fputs(" at lines ", stderr);
    fputs(ltoa(lineno1), stderr);
    fputs(" and ", stderr);
    fputs(ltoa(lineno2), stderr);
    fputc('\n', stderr);
}
