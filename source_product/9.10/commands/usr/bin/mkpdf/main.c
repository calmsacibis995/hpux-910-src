/* $Revision: 66.6 $ */
/* Driver program for pdf functions */

#include <stdio.h>
#include <macros.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <malloc.h>
#include <sys/sysmacros.h>
#include "pdf.h"

int errors = 0;	    /* 0 - none */
		    /* 1 - differences */
		    /* 2 - problems */
char *progname;
int numeric_ids = 0;	/* instead of strings from passwd */

main(argc,argv)
int argc;
char *argv[];
{
    if (progname = strrchr(argv[0], '/')) {
	progname++;
    } else {
	progname = argv[0];
    }
    if (equal(progname, "mkpdf")) {
	return mkpdf(argc,argv);
    } else if (equal(progname, "pdfdiff")) {
	return pdfdiff(argc,argv);
    } else if (equal(progname, "pdfck")) {
	return pdfck(argc,argv);
    } else if (equal(progname, "pdfpr")) {
	return pdfpr(argc,argv);
    } else if (equal(progname, "revck")) {
	return revck(argc,argv);
    } else {
	(void)fprintf(stderr,
	    "%s: Not a valid name for this program!\n",
	    progname);
	return 2;
    }
}


/*****************************************************************************

   mkpdf - the mkpdf program.  All the default values are set up first,
   this includes setting the default alternate root, and initializing
   the system size to 0.  Then any command line options are parsed out
   by the getopt procedure.  There should be two arguements left in the
   command line once the options are parsed out.  These should be the
   prototype PDF file name and the final file name.  Then each line from
   the structure is read, if it is a comment line it is just written out
   as is to the final PDF file.  Otherwise it is parsed by the
   parse_line procedure and the structure is processed by the
   process_line procedure.  If the entry is not a device or a directory
   the size of the entry is added to the system size.  If there is at
   least one file missing from the system that was not an optional file
   in the prototype PDF, a exit code of 1 is given.

   CALLED PROCEDURES - getopt, fprintf, fopen, fclose, fgets,
		       parse_line, process_line, atoi

******************************************************************************/

mkpdf(argc, argv)

char **argv;
int  argc;

{
    char *alt_root = NULL;		/* pointer to the alternate path */
    char line[MAXLINELEN];		/* string to hold line from PDF */
    char *comment = NULL;		/* pointer to comment */
    char *std_name = "-";		/* default string for stdin/stdout */
    char *usage_string =
"Usage: mkpdf [ -c version ] [ -r alt_root ] prototype_PDF new_PDF\n";
    FILE *In_fp;			/* file pointer to prototype PDF */
    FILE *Out_fp;			/* file pointer to final PDF */
    int  system_size;		/* total byte size of entries in PDF */
    int  system_blocks;
    struct pdfitem pdf_holder;	/* struct for 1 element of a PDF */
    int  argchoice;			/* variables used by getopt */
    extern char *optarg;
    extern int optind;
    extern int opterr;
    char *title_string = "% Product Description File";
    char *size_string  = "% total size is";

    opterr = 0;
    system_size = 0;
    system_blocks = 0;
    while ((argchoice = getopt(argc,argv,"r:c:n")) != EOF)
	switch (argchoice)
	{
	case 'r' :
	    alt_root = optarg;
	    break;
	case 'c' :
	    comment = optarg;
	    break;
	case 'n':
	    numeric_ids = 1;
	    break;
	default :
	    (void)fprintf(stderr,usage_string);
	    return 2;
	}
    if ((argc - optind) == 2)
    {
	In_file = argv[optind++];
	Out_file = argv[optind];
    }
    else
    {
	(void)fprintf(stderr,usage_string);
	return 2;
    }
    if (equal(In_file,std_name))
	In_fp = stdin;
    else if ((In_fp = fopen(In_file,"r")) == NULL)
    {
	(void)fprintf(stderr,usage_string);
	sprintf(line,"mkpdf: %s",In_file);
	perror(line);
	return 2;
    }
    if (equal(Out_file,std_name))
	Out_fp = stdout;
    else if ((Out_fp = fopen(Out_file,"w")) == NULL)
    {
	(void)fprintf(stderr,usage_string);
	sprintf(line,"mkpdf: %s",Out_file);
	perror(line);
	fclose(In_fp);
	return 2;
    }
    (void)fprintf(Out_fp,"%s\n", title_string);
    if (comment)
	(void)fprintf(Out_fp,"%% %s\n", comment);
    while ((fgets(line,MAXLINELEN,In_fp)) != NULL) {
	switch (line[0]) {
	case '%': /* comment to be passed through */
	    /* remove internally generated comments, pass all others */
	    if (strncmp(line, title_string, strlen(title_string)) &&
		    strncmp(line, size_string, strlen(size_string)))
		fputs(line, Out_fp);
	    break;
	case '\n':
	case '#': /* comment to be deleted */
	    break;
	default:
	    parse_line(line,&pdf_holder);
	    switch (process_line(alt_root,&pdf_holder)) {
	    case 0:
		if ((pdf_holder.mode[0] != '\0')  &&
		    (pdf_holder.mode[0] != 'd') &&
		    (pdf_holder.mode[0] != 'H') &&
		    (pdf_holder.mode[0] != 'c') &&
		    (pdf_holder.mode[0] != 'b') &&
		    (pdf_holder.mode[0] != '?')) {
			system_size = system_size + atoi(pdf_holder.fsize);
			system_blocks = system_blocks + 
			    (atoi(pdf_holder.fsize) + fragment_size -1) / 
			    fragment_size;
		}
		break;
	    case 1:
		if (!errors)
		    errors = 1;
		continue;
	    default:
		errors = 2;
	    }
	    (void)pdf_pr(Out_fp, "%s:%s:%s:%s:%s:%s:%s:%s:%s\n", pdf_holder, 0);
	}
    }
    (void)fprintf(Out_fp,"%s %d bytes.\n",size_string,system_size);
    (void)fprintf(Out_fp,"%s %d blocks.\n",size_string,system_blocks);
/* if there has been at least one file missing from the system that
has been converted to an optional file by process_line, then exit
with a 1. */
/*
    exit 0 - no errors
    exit 1 - missing files
    exit 2 - serious problems
*/
    return errors;
}


/*****************************************************************************

   pdfdiff - the pdfdiff program.  All the default values are set up
   first, the percent growth is set to 0.  Then any options entered in
   on the command line are parsed out by the getopt procedure.  There
   should be two arguments left in the command line once the options
   are parsed out.  These arguements are the new pdf file and the old
   pdf file.  Then the new pdf file is opened and read line by line.
   Each line is parsed and the fields from that line are stored in a
   pdfitem structure with a pointer from the array newpdf pointing to
   it.  The same is done for the old pdf file.  The procedure add_delete
   then takes the new pdf and old pdf pointer arrays and removes the
   structures that do not have matching entries in the other array.

   CALLED PROCEDURES - getopt, atoi, fprintf, sprintf, fopen, fclose,
                       fgets, realloc, malloc, parse_line, add_delete,
                       verbose_check_report

******************************************************************************/

char *old_pdf_filename, *new_pdf_filename;
void bubblesort();

pdfdiff(argc, argv)
char **argv;
int  argc;
{
    char line[MAXLINELEN];              /* string to hold line from pdf */
    char *usage_string = "Usage: pdfdiff [ -p n ] pdf1 pdf2\n";
    FILE *new_pdf_fp;                   /* new pdf and old pdf file     */
    FILE *old_pdf_fp;                   /* pointers                     */
    unsigned int  offset;               /* offset into pdf files        */
    unsigned int  reallocsize;          /* size of pointer allocations  */
    int  percent;                       /* percentage growth threshold  */
    struct pdfitem pdf_holder;          /* struct for 1 element of pdf  */
    struct pdfitem **new_pdf;           /* pointers to array of structs */
    struct pdfitem **old_pdf;           /* pointers to array of structs */

    int  argchoice;                     /* variables used by getopt     */
    extern char *optarg;                /* a library routine            */
    extern int  optind;
    extern int  opterr;
    int gr_percent;
    int old_size = 0, old_blocks = 0, new_size = 0, new_blocks = 0;
    int fsize;

    percent = 0;
    opterr = 0;                         /* getopt won't report errors */

    while ((argchoice = getopt(argc,argv,"np:")) != EOF) {
	switch(argchoice) {
	case 'n':
	    numeric_ids = 1;
	    break;
	case 'p' :
	    percent = atoi(optarg);
	    break;
	default:
	    (void)fprintf(stderr,usage_string);
	    return 2;
	}
    }
    if ((argc-optind) == 2)
    {
	old_pdf_filename = argv[optind++];
	new_pdf_filename = argv[optind];
    }
    else
    {
	(void)fprintf(stderr,usage_string);
	return 2;
    }
    if ((new_pdf_fp = fopen(new_pdf_filename,"r")) == NULL)
    {
	(void)fprintf(stderr,usage_string);
	sprintf(line,"%s: %s", progname, new_pdf_filename);
	perror(line);
	return 2;
    }
    if ((old_pdf_fp = fopen(old_pdf_filename,"r")) == NULL)
    {
	(void)fprintf(stderr,usage_string);
	sprintf(line,"%s: %s", progname, old_pdf_filename);
	perror(line);
	return 2;
    }

    /* start by allocating 100 pointers for the pdf entries */
    new_pdf = (struct pdfitem **) malloc(100 * (sizeof(struct pdfitem *)));
    old_pdf = (struct pdfitem **) malloc(100 * (sizeof(struct pdfitem *)));

    /* do the new pdf */
    offset = 0;
    reallocsize = 100;
    while (line == fgets(line,MAXLINELEN,new_pdf_fp)) {
	if (line[0] != '%') {
	    if (offset == reallocsize - 1) {
		reallocsize = reallocsize + 100;
		if ((new_pdf = (struct pdfitem **)
			realloc((char *)new_pdf, reallocsize * 
			sizeof(struct pdfitem *))) == 
			(struct pdfitem **)0) {
		    (void)fprintf(stderr,"%s: no more memory left", progname);
		    return 2;
		}
	    }
	    new_pdf[offset] = (struct pdfitem *) malloc(sizeof (pdf_holder));
	    if (new_pdf[offset] == (struct pdfitem *)0)
	    {
		(void)fprintf(stderr,"%s: no more memory left", progname);
		return 2;
	    }
	    fsize = parse_line(line,new_pdf[offset]);
	    new_size += fsize;
	    new_blocks += ((fsize + fragment_size -1) / fragment_size);
	    offset++;
	}
    }
    bubblesort(new_pdf,offset);
    new_pdf[offset] = (struct pdfitem *)0;

    /* now do the old pdf */
    offset = 0;
    reallocsize = 100;
    while (line == fgets(line,MAXLINELEN,old_pdf_fp)) {
	if (line[0] != '%') {
	    if (offset == reallocsize - 1) {
		reallocsize = reallocsize + 100;
		if ((old_pdf = (struct pdfitem **) realloc((char *)old_pdf,
			reallocsize * sizeof(struct pdfitem *))) == 
			(struct pdfitem **)0) {
		    (void)fprintf(stderr,"%s: no more memory left", progname);
		    return 2;
		}
	    }
	    old_pdf[offset] = (struct pdfitem *) malloc(sizeof (pdf_holder));
	    if (old_pdf[offset] == (struct pdfitem *)0) {
		(void)fprintf(stderr,"%s: no more memory left", progname);
		return 2;
	    }
	    fsize = parse_line(line,old_pdf[offset]);
	    old_size += fsize;
	    old_blocks += ((fsize + fragment_size -1) / fragment_size);
	    offset++;
	}
    }
    bubblesort(old_pdf,offset);
    old_pdf[offset] = (struct pdfitem *)0;

    /* now do the real work */
    errors = add_delete(new_pdf, old_pdf);
    for (offset=0; (struct pdfitem *)new_pdf[offset] != (struct pdfitem *)0;
	    offset++) {
	switch (verbose_check_report(new_pdf[offset], old_pdf[offset], percent)) {
	case 0:
	    break;
	case 1:
	    if (!errors)
		errors = 1;
	    break;
	default:
	    errors = 2;
	    break;
	}
    }

    /* compute growth */
    if (old_blocks == 0) {
	if (new_blocks == 0) {
	    gr_percent = 0;
	} else {
	    gr_percent = INT_MAX;
	}
    } else {
	gr_percent = ((new_blocks - old_blocks) * 100)/ old_blocks;
    }
    printf("Growth: %d bytes, %d blocks (%d%%)\n",
	new_size - old_size, new_blocks - old_blocks, gr_percent);

    return errors;
}


/*****************************************************************************

   pdfck - the pdfck program.  Then any options entered in on the
   command line are parsed out by the getopt procedure.  There should be
   one argument left in the command line once the options are parsed
   out.  This argument is the pdf.  Then the pdf file is opened and read
   line by line.  Each line is parsed and the fields from that line are
   stored in a pdfitem structure.  Then the file refered to by the pdf
   entry is inspected and a pdfitem structure is created for it.  If the
   file does not exist and the pathname in the pdf does not start with
   '?', then the file is reported as deleted.  If the file exists
   differences are reported exactly as in pdfdiff.

   CALLED PROCEDURES - getopt, atoi, fprintf, sprintf, fopen, fclose,
                       fgets, parse_line, process_line
                       verbose_check_report

******************************************************************************/

pdfck(argc, argv)

char **argv;
int  argc;

{
    char *alt_root = NULL;		/* pointer to the alternate path */
    char line[MAXLINELEN];              /* string to hold line from pdf  */
    char *usage_string = "Usage: pdfck [ -r alt_root ] pdf\n";
    FILE *old_pdf_fp;                   /* pointers                      */
    struct pdfitem file_pdf;          /* real file pdf struct   */
    struct pdfitem old_pdf;		/* reference pdf entry  */
    char *pathname;			/* points to pdf pathname for testing */

    int  argchoice;                     /* variables used by getopt      */
    extern char *optarg;                /* a library routine             */
    extern int  optind;
    extern int  opterr;

    opterr = 0;                         /* getopt will not report errors */

    while ((argchoice = getopt(argc,argv,"nr:")) != EOF) {
	switch(argchoice)
	{
	case 'n':
	    numeric_ids = 1;
	    break;
	case 'r' :
	    alt_root = optarg;
	    break;
	default:
	    (void)fprintf(stderr,usage_string);
	    return 2;
	}
    }
    if ((argc-optind) == 1) {
	old_pdf_filename = argv[optind++];
    } else {
	(void)fprintf(stderr,usage_string);
	return 2;
    }
    if ((old_pdf_fp = fopen(old_pdf_filename,"r")) == NULL) {
	(void)fprintf(stderr,usage_string);
	sprintf(line,"pdfck: %s",old_pdf_filename);
	perror(line);
	return 2;
    }

    /* do the pdf */
    while (line == fgets(line,MAXLINELEN,old_pdf_fp)) {
	if (line[0] != '%') {
	    int optional = 0;
	    (void) parse_line(line,&old_pdf);
	    file_pdf = old_pdf;
	    pathname = file_pdf.pathname;
	    if (*pathname == '?') {
		pathname++;
		optional++;
	    }
	    sprintf(line, "%s%s", alt_root, pathname);
	    if (access(line, 0) == 0) {
		if (process_line(alt_root,&file_pdf) == 0) {
		    switch (verbose_check_report(&file_pdf, &old_pdf, 0)) {
		    case 0:
			break;
		    case 1:
			if (!errors)
			    errors = 1;
			break;
		    default:
			errors = 2;
			break;
		    }
		} else {
		    errors = 2;
		}
	    } else if (!optional) {
		printf("%s: deleted\n", pathname);
		errors = 1;
	    }
	}
    }

    return errors;
}


/*************************************************************************

   PDFPR - print out a pdf using user specified format

   CALLS - pdf_pr

*************************************************************************/

int
pdfpr(argc,argv)
int argc;
char **argv;
{
    char *format = "pathname:\t%s\nowner:\t\t%s\ngroup:\t\t%s\nmode:\t\t%s\nfsize:\t\t%s\nlink_count:\t%s\nversion:\t%s\nchecksum:\t%s\nlink_target:\t%s\n\n";
    int user_format = 0;            /* did the user provide the format? */
    char line[MAXLINELEN];          /* string to hold line from pdf  */
    char *usage_string = "Usage: pdfpr [ -f format ] pdf\n";
    FILE *old_pdf_fp;               /* pointers                      */
    struct pdfitem old_pdf;		/* reference pdf entry  */

    int  argchoice;                 /* variables used by getopt      */
    extern char *optarg;            /* a library routine             */
    extern int  optind;
    extern int  opterr;

    opterr = 0;                     /* getopt will not report errors */

    while ((argchoice = getopt(argc,argv,"f:")) != EOF) {
	switch(argchoice) {
	case 'f' :
	    format = optarg;
	    user_format = 1;
	    break;
	default:
	    (void)fprintf(stderr,usage_string);
	    return 2;
	}
    }
    if ((argc-optind) == 1)
    {
	old_pdf_filename = argv[optind++];
    }
    else
    {
	(void)fprintf(stderr,usage_string);
	return 2;
    }
    if ((old_pdf_fp = fopen(old_pdf_filename,"r")) == NULL)
    {
	(void)fprintf(stderr,usage_string);
	sprintf(line,"pdfck: %s",old_pdf_filename);
	perror(line);
	return 2;
    }

    /* do the pdf */
    while (line == fgets(line,MAXLINELEN,old_pdf_fp)) {
	if ((line[0] != '%') && (line[0] != '?')) {
	    (void) parse_line(line,&old_pdf);
	    if (pdf_pr(stdout,format,old_pdf, user_format)) {
		return 2;
	    }
	}
    }
    return errors;
}

#include <sys/wait.h>
int
pdf_pr(outfile, format, pdf_entry, use_printf_cmd)
FILE* outfile;
char *format;
struct pdfitem pdf_entry;
int use_printf_cmd;
{
    int retval;
    if (use_printf_cmd) {
	/* form an argv */
	char *pfarg[13];
	int  pid, statval;

	pfarg[0] = "printf";
	pfarg[1] = format;
	pfarg[2] = pdf_entry.pathname;
	pfarg[3] = pdf_entry.owner;
	pfarg[4] = pdf_entry.group;
	pfarg[5] = pdf_entry.mode;
	pfarg[6] = pdf_entry.fsize;
	pfarg[7] = pdf_entry.link_count;
	pfarg[8] = pdf_entry.version;
	pfarg[9] = pdf_entry.checksum;
	pfarg[10] = pdf_entry.link_target;
	pfarg[11] = NULL;
	switch (pid=vfork()) {
	case 0:
	    retval = execv("/bin/printf", pfarg);
	    retval = execv("/usr/bin/printf", pfarg);
	    fputs(progname, stderr);
	    perror(": attempt to use printf command failed");
	    exit(1);
	case -1:
	    fputs(progname, stderr);
	    perror(": fork failed");
	    return 1;
	default:
	    if (waitpid(pid,&statval,0)) {
		(void)fprintf(stderr, "%s: ", progname);
		perror("printf");
		return 1;
	    }
	    if (WIFEXITED(statval) && WEXITSTATUS(statval)) {
		/* printf failed */
		return 1;
	    }
	}
    } else {
	retval = fprintf(outfile, format,
	    pdf_entry.pathname,
	    pdf_entry.owner,
	    pdf_entry.group,
	    pdf_entry.mode,
	    pdf_entry.fsize,
	    pdf_entry.link_count,
	    pdf_entry.version,
	    pdf_entry.checksum,
	    pdf_entry.link_target);
    }
    if (retval < 0) {
	(void)fprintf(stderr,
	    "Can't print PDF entry with format specified (%s).\n", format);
	return 1;
    } else {
	return 0;
    }
}


/**************************************************************************

   BUBBLESORT - bubblesort an array of pdfitems so that they are in order
   alphabetically. 
 
   CALLED PROCEDURES: strcmp

*****************************************************************************/

void bubblesort(pdf_array,array_size)

struct pdfitem *pdf_array[];
unsigned int array_size;

{
    int index;
    struct pdfitem *temp_pdf_pointer;
    int exchange_index;

    do {
	exchange_index = 0;
	for (index = 1; index < array_size; index++) {
	    if ((strcmp(pdf_array[index - 1]->pathname,
		    pdf_array[index]->pathname)) > 0) {
		{
		    temp_pdf_pointer = pdf_array[index];
		    pdf_array[index] = pdf_array[index - 1];
		    pdf_array[index - 1] = temp_pdf_pointer;
		    exchange_index = index;
		}
	    }
	}
    } while ( (array_size = exchange_index) > 1);  
    return;
}


/*******************************************************************

   VERBOSE_CHECK_REPORT - The old and new pdf entries are compared for
   exceeding the growth percentage threshold, change in ownership,
   group, or protection, change in checksum, and if the entries are SUID
   files.  If a field is NULL/empty in one of or both of the pdfs, the
   check will not be preformed.

   CALLED PROCEDURES - fopen, fclose, fprintf,
                       pdf_check_suid,pdf_check_sgid 
		       cat_diff, id_check

**********************************************************************/
int cat_diff();
int id_check();
uid_t id_lookup();
#define MSGMAX MAXPATHLEN+256
char message[MSGMAX + 1];	/* for the error report */
int msg_len;

verbose_check_report(newpdf,oldpdf,percent)

struct pdfitem *newpdf;		/* pointer to pdfitem struct */
struct pdfitem *oldpdf;		/* for new and old pdf */
int  percent;			/* growth percentage threshold */

{
    int  newsize, oldsize;	/* int of fsize */
    int  ndiff = 0;		/* number of differences found */

    /* initialize the message */
    sprintf(message, "%s:", newpdf->pathname);
    msg_len = strlen(newpdf->pathname);

    ndiff += id_check("owner", oldpdf->owner, newpdf->owner);
    ndiff += id_check("group", oldpdf->group, newpdf->group);

    if (cat_diff("mode", oldpdf->mode, newpdf->mode)) {
	ndiff++;
	switch(pdf_check_suid(newpdf->mode, oldpdf->mode)) {
	case 0:
	    break;
	case 1:
	    strcat(message, "(became suid)");
	    break;
	case 2:
	    strcat(message, "(no longer a suid)");
	    break;
	case 3:
	    (void)cat_diff("suid owner", oldpdf->owner, newpdf->owner);
	    break;
	}

	switch(pdf_check_sgid(newpdf->mode, oldpdf->mode)) {
	case 0:
	    break;
	case 1:
	    strcat(message, "(became sgid)");
	    break;
	case 2:
	    strcat(message, "(no longer a sgid)");
	    break;
	case 3:
	    (void)cat_diff("sgid group", oldpdf->group, newpdf->group);
	    break;
	}
    }

    /* check size */
    if ((newpdf->mode[0] == 'c') || (newpdf->mode[0] == 'b')) {
	ndiff += cat_diff("major/minor", oldpdf->fsize, newpdf->fsize);
    } else {
	newsize = atoi(newpdf->fsize);
	oldsize = atoi(oldpdf->fsize);
	if ((!oldsize && newsize) || (oldsize && !newsize) ||
/* prevent divide by 0 */
		((oldsize && newsize) &&
		(abs(((newsize - oldsize)*100)/oldsize) >= percent))) {
	    ndiff += cat_diff("size", oldpdf->fsize, newpdf->fsize);
	}
    }

    ndiff += cat_diff("links", oldpdf->link_count, newpdf->link_count);
    ndiff += cat_diff("version", oldpdf->version, newpdf->version);
    ndiff += cat_diff("checksum", oldpdf->checksum, newpdf->checksum);
    ndiff += cat_diff("linked_to", oldpdf->link_target, newpdf->link_target);

    if (ndiff) {
	printf("%s\n", message);
	if (msg_len > MSGMAX)
	    (void)fprintf(stderr,
		"%s: WARNING: %s comparison message truncated.\n",
		progname, newpdf->pathname);
    }
    ndiff += cat_diff(NULL,NULL,NULL);
    return (ndiff != 0);
}


/********************************************************************
CAT_DIFF - compare field and append to message if not equal
********************************************************************/

int
cat_diff(diff_field, old, new)
char *diff_field, *old, *new;
{
    char stmp[MSGMAX+1];		/* tmp string */
    static int diff_found = 0;	/* used to prepend ", " */

    if (diff_field==NULL) {
	/* clean up and report activites */
	if (diff_found) {
	    diff_found = 0;
	    return 1;
	} else {
	    return 0;
	}
    }

    if (!equal(old, new) && !equal(old,"") && !equal(new,"")) {
	/* only if different and both are non-NULL */
	sprintf(stmp, "%s %s(%s -> %s)",
	    diff_found?",":"",
	    diff_field, old, new);
	if ((msg_len += strlen(stmp)) <= MSGMAX)
	    strcat(message, stmp);
	diff_found++;
	return 1;
    } else {
	return 0;
    }
}

/*********************************************************************

ID_CHECK - compare two user or group id's.  Handles numeric, text or one
of each.  Caches passwd and group file info for improved performance.

CALLED PROCEDURES - cat_diff, lookup

*********************************************************************/
#define OWNER 1
#define GROUP 2

int
id_check(field, old, new)
char *field, *old, *new;
{
    static char details[50];
    uid_t oldid, newid;
    int type;
    int retval;

    if (equal(field, "owner"))
	type=OWNER;
    else
	type=GROUP;

    if (equal(old, new) || equal(old,"") || equal(new,""))
	return 0;

/* handle the numeric/string case */
    if (numeric_ids ||
	(!isdigit((int)*old) && isdigit((int)*new)) ||
	(isdigit((int)*old) && !isdigit((int)*new))) {
	oldid = id_lookup(type, old);
	newid = id_lookup(type, new);
/* compare the numeric values */
	if (oldid != newid){
	    retval = cat_diff(field, old, new);
	    /* print the numeric values */
	    sprintf(details, "(%cid: %d -> %d)",
		type == OWNER ? 'u' : 'g',
		oldid, newid);
	    if ((msg_len += strlen(details)) <= MSGMAX)
		strcat(message, details);
	    return retval;
	} else
	    return 0;
    }
    return cat_diff(field, old, new);
}

/******************************************************************

ID_LOOKUP - find the [ug]id associated with name passed in.
Caches the passwd and group info to speed things up.

******************************************************************/

uid_t
id_lookup(type, name)
int type;
char *name;
{
    static int cached = 0;
    static struct ulist {
	char name[32];
	uid_t id;
	struct ulist *next;
    } ulist, *ulistp;
    static struct glist {
	char name[32];
	gid_t id;
	struct glist *next;
    } glist, *glistp;
    struct passwd *uentry;
    struct group *gentry;

    if (isdigit(*name)) {
	return (uid_t)atoi(name);
    }

    if (!cached) {
	/* initialize the lists */
	ulistp = &ulist;
	glistp = &glist;
	/* read in the info in passwd and group */
	setpwent();
	while ((uentry = getpwent()) != NULL) {
	    strcpy(ulistp->name, uentry->pw_name);
	    ulistp->id = uentry->pw_uid;
	    if ((ulistp->next = malloc(sizeof(struct ulist))) == NULL) {
		(void)fprintf(stderr, "%s: malloc failed\n", progname);
		exit (1);
	    }
	    ulistp = ulistp->next;
	}
	ulistp->next = NULL;
	endpwent();
	setgrent();
	while ((gentry = getgrent()) != NULL) {
	    strcpy(glistp->name, gentry->gr_name);
	    glistp->id = gentry->gr_gid;
	    if ((glistp->next = malloc(sizeof(struct glist))) == NULL) {
		(void)fprintf(stderr, "%s: malloc failed\n", progname);
		exit (1);
	    }
	    glistp = glistp->next;
	}
	glistp->next = NULL;
	endgrent();
	cached = 1;
	ulistp = &ulist;
	glistp = &glist;
    }

/* in either case, do a linear search of the linked list */
/* admittedly slow, but most uses of PDFs will hit the top of the list */
    switch (type) {
    case OWNER:
	if (equal(name, ulistp->name)) {
	    /* same as last time */
	    return ulistp->id;
	} else {
	    /* start at the top of the list */
	    ulistp = &ulist;
	}
	do {
	    if (equal(name, ulistp->name))
		return ulistp->id;
	    ulistp = ulistp->next;
	} while ( ulistp != NULL);
	return -1;
    case GROUP:
	if (equal(name, glistp->name)) {
	    /* same as last time */
	    return glistp->id;
	} else {
	    /* start at the top of the list */
	    glistp = &glist;
	}
	do {
	    if (equal(name, glistp->name))
		return (uid_t)glistp->id;
	    glistp = glistp->next;
	} while ( glistp != NULL);
	return -1;
    }
    return -1;
}
