/*

    open_input_file()

    Open either the main input file or an include file for input.

*/

#include "fizz.h"

static FILE *input_stream = (FILE *) 0;
static FILE *include_stream = (FILE *) 0;
static unsigned long input_line; /* remember where we left off when 
				    switching to an include file */

void open_input_file(filename, include)
char *filename;
BOOLEAN include;  /* this is an include file */
{
    if (include) { /* include file? */
	if (include_stream != (FILE *) 0) { /* already in include file? */
	    error_locate(1);
	    return;
        };
	if ((include_stream = fopen(filename, "r")) == NULL) /* open file */
	    error(0, filename);
	input_line = gLine;
    } else { /* input file */
	if ((input_stream = fopen(filename, "r")) == NULL) /* open file */
	    error(0, filename);
    };

    /* store file name */
    CREATE_STRING(gFile, strlen(filename) + 1);
    (void) strcpy(gFile, filename);

    gLine = 0;

    return;
}

/*

    close_input_files()

    Close any input and include files that are open.

*/

void close_input_files()
{
    void close_input_file();
    void close_include_file();

    close_include_file();
    close_input_file();

    return;
}

/*

    close_input_file()

    Close input file if it is open.

*/

static void close_input_file()
{
    if (input_stream != (FILE *) 0) {
	if (fclose(input_stream) == EOF)
	    error(26, gInputFile);
	input_stream = (FILE *) 0;
	gFile = (char *) 0;
	gLine = 0;
    };

    return;
}

/*

    close_include_file()

    Close include file if it is open.

*/

static void close_include_file()
{
    if (include_stream != (FILE *) 0) {
	if (fclose(include_stream) == EOF)
	    error(26, gFile);
	include_stream = (FILE *) 0;
	free(gFile);
	gLine = input_line;
	gFile = gInputFile;
    };

    return;
}

/*

    read_line()

    Read a line from the current file (either the input file or an
    include file).  Input goes into the global gBuffer.  The return
    value is FALSE if the end-of-file has been reached on the main input
    file.

*/

BOOLEAN read_line()
{
    register int c, eol, count;
    register char *ptr;
    void close_input_file();
    void close_include_file();

    if (include_stream != (FILE *) 0) {	/* try include file first */
	ptr = gBuffer;
	count = BUFFER_SIZE - 1;
	eol = '\n';
	while (((c = getc(include_stream)) != EOF) && (c != eol)) /* get char */
	    if (count) { /* don't overflow buffer */
	        *ptr++ = (char) c;
	        count--;
	    };
	if (!((c == EOF) && (ptr == gBuffer))) { /* check for end-of-file */
	    *ptr = '\0';
	    gLine++;
	    return TRUE;
	};
	close_include_file();
    };

    /* otherwise, it's the main input file */
    ptr = gBuffer;
    count = BUFFER_SIZE - 1;
    eol = '\n';
    while (((c = getc(input_stream)) != EOF) && (c != eol)) /* get char */
        if (count) { /* don't overflow buffer */
	    *ptr++ = (char) c;
	    count--;
        };
    if (!((c == EOF) && (ptr == gBuffer))) {
        *ptr = '\0';
	gLine++;
        return TRUE;
    };
    close_input_file();

    return FALSE;

}
