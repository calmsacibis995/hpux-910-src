
/* LINTLIBRARY */

#include <stdio.h>

extern int errno, debug;
extern char *pname;

/* VARARGS1 */
fatal(s, a,b,c,d,e,f,g,h,i)
char *s;
{
	int save_errno=errno;

	fflush(stdout);
	fprintf(stderr, "%s: ", pname);
	fprintf(stderr, s, a,b,c,d,e,f,g,h,i);
	if (save_errno) {
		fprintf(stderr, ": ");
		errno=save_errno;
		perror("");
	} else
		fprintf(stderr, "\n");
	exit(1);
}

/* VARARGS1 */
bugout(s, a,b,c,d,e,f,g,h,i)
char *s;
{
	if (debug) {
		fflush(stdout);
		fprintf(stderr, s, a,b,c,d,e,f,g,h,i);
		fprintf(stderr, "\n");
	}
}
