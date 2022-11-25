static char *HPUX_ID = "@(#) $Revision: 66.1 $";

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
int nlmsg_fd;
char *catgets();
#endif NLS
#include <stdio.h>
#include <sys/types.h>
#include <cluster.h>
#include <varargs.h>
#include <string.h>
#include <curses.h>
#include <term.h>

int aflag;
int Aflag;
int nflag;
int lflag;
int mflag;
int multi_col;
int sflag;
int xflag;
int rflag;
int errors;
extern int optind, opterr;
char *output_array[MAXCNODE+1];
int output_ctr = 0;
int num_cols = 0;
int filewidth;
int is_tty;
char *getenv();

struct cct_list {
	struct cct_entry data;
	struct cct_list *next;
}  *cct_head;

cnode_t clustered[MAXCNODE+1];

read_clusterconf()
{
    struct cct_list *tail;
    struct cct_entry *e;

    setccent();
    if (e = getccent())
    {
	cct_head = (struct cct_list *)malloc(sizeof (struct cct_list));
	if (cct_head == NULL)
	{
	    fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 1, "cnodes: malloc error\n")));
	    exit(2);
	}
	cct_head->data = *e;
	tail = cct_head;
	while (e = getccent())
	{
	    tail->next = (struct cct_list *)
		malloc(sizeof (struct cct_list));
	    if (tail->next == NULL)
	    {
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 2, "cnodes: malloc error\n")));
		exit(2);
	    }
	    tail = tail->next;
	    tail->data = *e;
	}
	tail->next = NULL;
	return 0;
    }
    return 1;
}

main(argc, argv)
int argc;
char **argv;
{
    int c;

#ifdef NLS
    nlmsg_fd = catopen("cnodes", 0);
#endif

    if (isatty(fileno(stdout)))
	is_tty = multi_col = 1;
    else
	is_tty = multi_col = 0;

    while ((c = getopt(argc, argv, "aAnlmxC1sr")) != EOF)
	switch (c)
	{
	case 'a': 
	    aflag++;
	    break;
	case 'A': 
	    Aflag++;
	    break;
	case 'n': 
	    nflag++;
	    break;
	case 'l': 
	    lflag++;
	    break;
	case 'm': 
	    mflag++;
	    break;
	case 'C': 
	    multi_col = 1;
	    break;
	case '1': 
	    multi_col = 0;
	    break;
	case 's': 
	    sflag++;
	    break;
	case 'x': 
	    xflag++;
	    break;
	case 'r': 
	    rflag++;
	    break;
	case '?': 
	    errors++;
	    break;
	}
    if (errors)
    {
	fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 3, "Usage: %s -aAnlmxC1sr [names]\n")), argv[0]);
	exit(2);
    }

    if (sflag)
	exit(cnodes(NULL) ? 0 : 1);

    if (read_clusterconf())
    {
	fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 4, "cnodes: can't read /etc/clusterconf\n")));
	exit(2);
    }

    if (cnodes(clustered) == 0)
	clustered[0] = 0;	/* for unclustered case */

    /*
     * Now we are at the point that we might need num_cols set. We only
     * do this if multi_col is TRUE and lflag is not set since num_cols
     * isn't needed if lflag was set.
     */
    if (multi_col && !lflag)
    {
	char *p;

	if ((p = getenv("COLUMNS")) != NULL)
	    num_cols = atoi(p);
	else
	    if (is_tty)
	    {
		int rc = 0;

		setupterm(0, 1, &rc);
		if (rc == 1)
		{
		    resetterm();
		    num_cols = columns;
		}
	    }

	if (num_cols < 20 || num_cols > 160)
	    num_cols = 80;
    }

    if (mflag)
    {
	/* 
	 * only display info about myself
	 */
	struct cct_list *p = cct_head;
	cnode_t id = cnodeid();

	while (p && p->data.cnode_id != id)
	    p = p->next;
	if (p)
	    format_data(&p->data);
	else
	    fprintf(stderr,
		    (catgets(nlmsg_fd, NL_SETN, 5, "cnodes: cnode_id %d: Machine does not exist\n")), id);
	print_and_exit(cnodes(NULL) ? 0 : 1);
    }

    if (rflag)
    {
	struct cct_list *p = cct_head;
	while (p)
	{
	    if (p->data.cnode_type == 'r')
		format_data(&p->data);
	    p = p->next;
	}
	print_and_exit(cnodes(NULL) ? 0 : 1);
    }

    if (optind == argc)
    {
	/* 
	 * no name arguments
	 */
	struct cct_list *p = cct_head;
	cnode_t id = cnodeid();

	while (p)
	{
	    if (!xflag || p->data.cnode_id != id)
		format_data(&p->data);
	    p = p->next;
	}
	print_and_exit(cnodes(NULL) ? 0 : 1);
    }
    else
    {
	/* 
	 * user has entered cnode names
	 */
	struct cct_list *p;
	cnode_t id = cnodeid();

	while (optind < argc)
	{
	    p = cct_head;
	    while (p)
	    {
		if (!strcmp(p->data.cnode_name, argv[optind]))
		    if (!aflag && search_cluster(p->data.cnode_id))
		    {
			fprintf(stderr,
				(catgets(nlmsg_fd, NL_SETN, 6, "cnodes: %s: Machine is not clustered\n")),
				argv[optind]);
			goto next_arg;
		    }
		    else
		    {
			if (!xflag || p->data.cnode_id != id)
			    format_data(&p->data);
			goto next_arg;
		    }
		p = p->next;
	    }
	    if (p == NULL)
	    {
		fprintf(stderr,
			(catgets(nlmsg_fd, NL_SETN, 7, "cnodes: %s: Machine does not exist\n")),
			argv[optind]);
	    }
	next_arg: optind++;
	}
	print_and_exit(cnodes(NULL) ? 0 : 1);
    }
}

format_data(element)
struct cct_entry *element;
{
    static char ROOTSERVER[] = " ROOTSERVER";
    char *asterisk = aflag ? " " : "";
    char *idtonam();
    char temp[10];

    if (search_cluster(element->cnode_id))
	if (aflag)
	    asterisk = "*";
	else if (!Aflag)
	    return;

    if (nflag)
	if (lflag)
	{
	    sprintf(temp, "%s%s", element->cnode_name, asterisk);
	    add_record("%3d %-10.10s %-10.10s%s", element->cnode_id,
		    temp, idtonam(element->swap_serving_cnode),
		    element->cnode_type == 'r' ? ROOTSERVER : "");
	}
	else
	    add_record("%d%s", element->cnode_id, asterisk);
    else if (lflag)
    {
	sprintf(temp, "%s%s", element->cnode_name, asterisk);
	add_record("%-10.10s %3d %-10.10s%s", temp,
		element->cnode_id, idtonam(element->swap_serving_cnode),
		element->cnode_type == 'r' ? ROOTSERVER : "");
    }
    else
	add_record("%s%s", element->cnode_name, asterisk);
}

add_record(va_alist)
va_dcl
{
    va_list ap;
    char *format;
    char buf[BUFSIZ + 1];
    int width;

    va_start(ap);
    format = va_arg(ap, char *);
    vsprintf(buf, format, ap);
    va_end(ap);

    /*
     * Delete trailing white space and calculate width of final string.
     */
    width = strlen(buf)-1;
    while (width > 0 && buf[width] == ' ' || buf[width] == '\t')
	buf[width--] = '\0';
    width++;
    if (width > filewidth)
	filewidth = width;

    if ((format = (char *)malloc(width + 1)) == NULL)
    {
	fprintf(stderr,
	    catgets(nlmsg_fd, NL_SETN, 8, "cnodes: malloc failed\n"));
	exit(2);
    }

    strcpy(format, buf);
    output_array[output_ctr++] = format;
}

search_cluster(id)
cnode_t id;
{
    int i = 0;

    while (clustered[i])
    {
	if (clustered[i] == id)
	    return 0;
	i++;
    }
    return 1;
}

char *
idtonam(id)
cnode_t id;
{
    struct cct_list *p = cct_head;

    while (p)
    {
	if (p->data.cnode_id == id)
	    return p->data.cnode_name;
	p = p->next;
    }
    return catgets(nlmsg_fd, NL_SETN, 9, "<not found>");
}

int curcol;

print_and_exit(ecode)
int ecode;
{
    static char aHeader[] =
"CNODE       ID SWAP SITE\n========== === =========="; /* catgets 10 */
    static char nHeader[] =
" ID CNODE      SWAP SITE\n=== ========== =========="; /* catgets 11 */
    int i;
    int ncols, nrows, row, col;
    int compar(), ncompar();

    if (nflag)
	qsort(output_array, output_ctr, sizeof (output_array[0]), ncompar);
    else
	qsort(output_array, output_ctr, sizeof (output_array[0]), compar);

    /*
     * Print a header with -l, only when stdout is a tty
     */
    if (lflag && is_tty)
    {
	puts(nflag ? catgets(nlmsg_fd, NL_SETN, 11, nHeader)
		   : catgets(nlmsg_fd, NL_SETN, 10, aHeader));
    }

    if (!multi_col || lflag)
	for (i = 0; i < output_ctr; i++)
	    puts(output_array[i]);
    else
    {
	if (aflag)
	    filewidth++;	/* add one space for '*' */
	filewidth += 2;		/* leave some space between names */
	ncols = num_cols / filewidth;
	nrows = (output_ctr - 1) / ncols + 1;
	for (row = 0; row < nrows; row++)
	{
	    for (col = 0; col < ncols; col++)
	    {
		i = (nrows * col) + row;
		if (i < output_ctr)
		{
		    next_column();
		    curcol += printf("%s", output_array[i]);
		}
	    }
	    if (curcol)
	    {
		putchar('\n');
		curcol = 0;
	    }
	}
    }
    exit(ecode);
}

next_column()
{
    if (curcol == 0)
	return;
    if ((curcol / filewidth + 2) * filewidth > num_cols)
    {
	putchar('\n');
	curcol = 0;
	return;
    }
    do
    {
	putchar(' ');
	curcol++;
    } while (curcol % filewidth);
}

compar(s1,s2)
char **s1, **s2;
{
    return strcmp(*s1, *s2);
}

ncompar(s1,s2)
char **s1, **s2;
{
    return atoi(*s1) - atoi(*s2);
}
