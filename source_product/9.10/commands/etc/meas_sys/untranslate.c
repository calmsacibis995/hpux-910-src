/* 
 * Undo translate format back into iscan format
 * (mostly for debugging other programs)
 */

#include <stdio.h>
#include <nlist.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/meas_sys.h>
#include <string.h>

int debug = 0;
char *progname = "untranslate";
int chatty = 1;

/* Add stuff to make common.c happy.  Most is irrelevant to us. */
/* Make the values bogus, so that it will die if anyone tries to use them. */
int kmem = -17;
struct nlist nlst[1] = { { 0 } };
char *nlist_filename = "";
int ms_id = -17;
int my_rev = -17;
int ms_bin_id = -17;
int wizard = 0;

int kmemf_flg = 0;	/* to keep common.c happy */

#define MAX_REC_LEN 8000

main(argc,argv)
int argc;
char **argv;
{
    long    lseek ();
    int     len, id, type;
    int	    recbuf[MAX_REC_LEN +1];
    int     i;
    char    line[MAX_REC_LEN * 4]; /* ought to be big enough */
    char    comment[MAX_REC_LEN * 4];
    char *token, *strtok();
    int scanf_cnt;

/*
 * MS_INCLUDE_REV should be defined.  The meas_sys.h shipped with
 * 9.0 is old, so it doesn't; hence this workaround.
 */
#ifdef MS_INCLUDE_REV
    (void) MS_INCLUDE_REV;	/* so we can ident the .o */
#endif
    
    handle_runstring(argc, argv);

    chat("$Header: untranslate.c,v 64.1 89/05/01 21:57:09 marcs Exp $\n");

    while (gets(line) != NULL) {	/* read a line */
	if (sscanf(line, "%d %d %d", &len, &id, &type) != 3) {
	    fprintf(stderr, "bad start of line\n");
	    exit(2);
	}

	switch (type) {
	  case MS_GRAB_ID:
	  case MS_COMMENT:
	  case MS_ERROR:
	  case MS_GRAB_BIN_ID:
	    (void) strtok(line, " ");
	    (void) strtok(NULL, " ");
	    (void) strtok(NULL, " ");
	    token = strtok(NULL, "'"); /* will have problems with included ' */
	    strcpy(comment, token);
	    recbuf[0] = len;
	    recbuf[1] = id;
	    recbuf[2] = type;
	    strcpy(&recbuf[3], comment);
	    fwrite(recbuf, sizeof(int), len, stdout);
	    break;
	  case MS_PUT_TIME:
	  case MS_DATA:
	  case MS_POST_BIN:
	  case MS_DATA_V:
	  case MS_OVERRUN_ERR:
	  case MS_LOST_DATA:
	  case MS_LOST_BUFFER:
	  case MS_ADMIN_INFO:
	    recbuf[0] = len;
	    (void) strtok(line, " ");
	    for (i = 1; i < len; i++) {
		token = strtok(NULL, " ");
		recbuf[i] = atoi(token);
	    }
	    fwrite(recbuf, sizeof(int), len, stdout);
	    break;
	  default:
	    fprintf(stderr, "unknown record type: %d\n", type);
	    exit(3);
	}
    }
    clean_up(0);
    return(0);
}

handle_runstring(argc, argv)
    int argc;
    char **argv;
{
    int c;
    extern char *optarg;
    extern int optind;

    while ((c = getopt(argc, argv, "q")) != EOF) {
    	switch (c) {
	    case 'q':
	    	chatty = 0;
		break;
	    case '?':
	    	bail_out("Usage: untranslate -q");
    	    	break;
	    default:
	    	bail_out("getopt returned %c", c);
	}
    }
    if (argc > optind) {
	bail_out("extra arg: %s", argv[optind]);
    }
}

/*VARARGS1*/
/*
 * Abnormal termination, but errno isn't set.
 * 
 * bail_out writes a newline before and after the given string.
 */
bail_out(fmt, a0, a1, a2, a3, a4, a5, a6, a7)
    char *fmt;
{
    fprintf(stderr,"\n%s: ", progname);
    fprintf(stderr, fmt, a0, a1, a2, a3, a4, a5, a6, a7);
    fprintf(stderr,"\n");
    clean_up(1);
}

/*
 * Used in die(), perror_bail_out(), and bail_out() .
 * Can handle normal and abnormal termination.
 */
clean_up(exit_code)
    int exit_code;
{
    chat("%s done\n", progname);
    exit(exit_code);
}

/*
 * Used for friendly, but potentially annoying messages.
 * Controlled by the global variable 'chatty'.
 */
/*VARARGS1*/
chat(s, a, b, c, d)
    char *s;
{
    if (chatty) {
	fprintf(stderr, s, a, b, c, d);
    }
}

