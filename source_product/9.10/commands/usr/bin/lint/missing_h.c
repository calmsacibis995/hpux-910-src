/* @(#) $Revision: 70.2 $ */

/* This filter reads stderr from cpp, looking for missing file error
 * messages.  It outputs the "# filename" directives that cpp would
 * have emitted, had it been able to find the header file.  The output
 * from this program is appended to the rest of the cpp output and
 * sent to the hdck program so that it can print any appropriate messages
 * from the header file database.
 */

#ifndef __LINT__
static char *HPUX_ID = "@(#) Internal $Revision: 70.2 $";
#endif

#include <stdio.h>
#include <string.h>

#define LINE_SIZE 1024
#define NULL_CHAR 	'\0'

main()
{
char buffer[LINE_SIZE];
char *ptr;
char *header_name;
char *msg;
int line_no;
int len;


	while (fgets(buffer, LINE_SIZE, stdin) != NULL) {
	    len = strlen(buffer);
	    buffer[len-1] = NULL_CHAR;

	    /* The lines we're looking for:
		    file.c: 5: Can't find include file header.h
		    file.c: 4: Unable to find include file 'header.h'.
		from cpp or cpp.ansi, respectively
	     */

	    msg = strstr(buffer, "Can't find include file ");
	    if (msg) {
		header_name = msg + strlen("Can't find include file ");
	    } else {
		msg = strstr(buffer, "Unable to find include file '");
		if (msg) {
		    header_name = msg + strlen("Unable to find include file '");
		    ptr = strchr(header_name, '\'');
		    if (ptr)
			*ptr = NULL_CHAR;	/* remove trailing quote */
		}
	    }

	    if (msg) {

		/* get the line number */
		ptr = strchr(buffer, ':');
		line_no = 1;
		if (ptr) {
		    *ptr = NULL_CHAR;
		    ptr++;
		    if (strchr(ptr, ':')) {
			while (*ptr==' ')
			    ptr++;
			line_no = atoi(ptr);
			/* There appears to be a bug in cpp so that the
			 * line number reported is 1 greater than the line
			 * on which the missing header was included.
			 * Compensate here.
			 */
			line_no--;
		    }
		}

		/* establish the offending line number in the includer */
		printf("# %d \"%s\"\n\n", line_no, buffer);
		/* make hdck think header_name has been included */
		printf("# 1 \"%s\"\n", header_name);
		/* then pretend we have returned to the includer */
		printf("# %d \"%s\"\n", line_no, buffer);
	    }
	}
}
