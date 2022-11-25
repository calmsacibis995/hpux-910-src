static char *HPUX_ID = "@(#) $Revision: 66.2 $";
/* HPUX_ID: @(#) $Revision: 66.2 $  */
/****************************************************************************
 *
 * fpagen [-uucode_addr -ttable_addr -stable_size] targetfile [files]
 *
 * ucode_addr = offset to microcode area
 * table_addr = offset to lookup table
 * table_size = size of table (number of 32-bit words)
 * targetfile = download file to be created
 * files      = source files (default is stdin)
 *
 * If any of the -u, -t, or -s options is given, they must all be given.
 * If given, source files will be of the format:
 *
 *     $<addr>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *     $<addr>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *        .
 *        .
 *        .
 * <codeword>'s from such source files will be loaded consecutively into
 * the microcode area and pointers to the microcode will be inserted into
 * the lookup table at the appropriate <addr>'s.  Each <addr> is an offset
 * from the beginning of the lookup table.  Note that a <codeword> section
 * may be preceded by multiple <addr>'s (i.e. several <addr>'s use the
 * same microcode.)  Any line beginning with a '#' in column 1 will be
 * treated as a comment.
 *
 * There is also a "generic" file format which may be used whether or not
 * the -u, -t, and -s options are specified. This allows code to be loaded
 * at any address. Its format is:
 *
 *     $OFFSET <addr>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *     <codeword> <codeword>  <optional comments>
 *        .
 *        .
 *        .
 *
 * The <codeword>'s will be loaded beginning at <addr>.  Here, the
 * <addr> is the offset from the beginning of the floating point
 * accelerator card.  The $OFFSET directive must be the first non-comment
 * line in the file.  (This file format also treats any line beginning
 * with a '#' in column 1 as a comment.)  There must be only one $OFFSET
 * directive per file.
 *
 * For both file formats, <addr>'s and <codeword>'s are assumed to be
 * decimal numbers unless preceded by a "0x" or "0X" to indicate hexadecimal
 * or by a "0" to indicate octal. Likewise, this is the format for numeric
 * values in the command line.
 *
 * Source files may be specified in any order with any mixture of the
 * two file formats. If stdin is used, it is treated as a single file.
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define EOL   10
#define MAGIC_NUMBER 0x1234CDEF

typedef short BOOLEAN;
#define FALSE 0
#define TRUE  1
#define AND   &&
#define OR    ||

static char *USAGE_MSG =
  "Usage: fpagen [-uucode_addr -ttable_addr -stable_size] targetfile [files]\n";
static char *OPEN_MSG = "%s: cannot open file\n";
static char *CLOSE_MSG = "%s: cannot close file\n";
static char *ALLOC_MSG = "insufficient memory\n";
static char *DIR_MSG = "%s: improper or missing directive\n%s\n";
static char *WRITE_MSG = "%s: cannot write file\n";
static char *SEEK_MSG = "%s: cannot seek\n";
static char *STAT_MSG = "%s: fail on file status\n";
static char *OFFSET_MSG = "%s: improper $OFFSET directive\n";
static char *CODEWORD_MSG = "%s: improper codeword\n";

main(argc, argv)
int argc;
char *argv[];
{
    BOOLEAN firstflag, headerflag, sflag, specialflag;
    BOOLEAN stdinflag, tableflag, tablesizeflag, ucodeflag;
    FILE *file, *target;
    char *dst, *ptr1, *ptr2, *src, buffer[132];
    int *table, arg, c, codeword[2], filenum, header[2], i, offset;
    int table_addr, table_index, table_size, top, ucode_addr;
    long endptr, fileptr, size;

    src = (char *) 0;
    dst = (char *) 0;
    table = (int *) 0;
    stdinflag = FALSE;
    specialflag = FALSE;
    ucodeflag = FALSE;
    tableflag = FALSE;
    tablesizeflag = FALSE;
    sflag = FALSE;

    /* Determine ucode and table addresses and table size (if given) */

    /* Parse options */
    for (arg = 1; arg < argc; arg++) {
        ptr1 = argv[arg];
        if (*ptr1++ != '-') break;
        c = *ptr1++;
        ptr2 = ptr1;
        switch (c) {
        case 's':
            if (tablesizeflag) break;
            table_size = (strtoul(ptr1, &ptr2, 0) + sizeof(int) - 1) / 
	                 sizeof(int);
            tablesizeflag = TRUE;
            break;
        case 't':
            if (tableflag) break;
            table_addr = strtoul(ptr1, &ptr2, 0);
            tableflag = TRUE;
            break;
        case 'u':
            if (ucodeflag) break;
            ucode_addr = strtoul(ptr1, &ptr2, 0);
            ucodeflag = TRUE;
            break;
        };
        if (ptr1 == ptr2) {
            fprintf(stderr, USAGE_MSG);
            exit(1);
        };
    };

    /* Either -s, -t, -u flags must all be given or none given */
    if(!(specialflag = ucodeflag AND tableflag AND tablesizeflag)) {
        if (ucodeflag OR tableflag OR tablesizeflag) {
            fprintf(stderr, USAGE_MSG);
            exit(1);
        };
    };

    /* Target file must be specified */
    if (arg == argc) {
        fprintf(stderr, USAGE_MSG);
        exit(1);
    };

    top = argc;

    /* Sort files: special files first, generic files last */
    if (specialflag AND (argc >= (arg + 3))) {
        filenum = arg + 1;
        while (filenum < top) {

            if (!(file = fopen(argv[filenum], "r"))) {
                (void) fprintf(stderr, OPEN_MSG, argv[filenum]);
                goto abort;
            };
            src = argv[filenum];

            /* Determine type of file */
            for (;;)
                if ((c = getc(file)) == '$') {
                    ptr1 = "OFFSET";
                    for (i = 0; i < 6; i++)
                        if (getc(file) != *ptr1++) break;
                    if (i == 6) {
                        ptr2 = argv[filenum];
                        argv[filenum] = argv[--top];
                        argv[top] = ptr2;
                    } else filenum++;
                    break;
                } else if (c == '#') {
                    while (getc(file) != EOL);
		} else {
		    while (isspace(c) && (c != EOL)) c = getc(file);
		    if (c == EOL) continue;
                    (void) fprintf(stderr, DIR_MSG, src, 0);
                    goto abort;
                };

            if (fclose(file)) {
                (void) fprintf(stderr, CLOSE_MSG, argv[filenum]);
                src = (char *) 0;
                goto abort;
            };
            src = (char *) 0;
        };
    } else stdinflag = argc == (arg + 1);

    /* Create table */
    if (specialflag) {
        if (!(table = (int *) calloc(table_size, sizeof(int)))) {
            (void) fprintf(stderr, ALLOC_MSG);
            goto abort;
        };
    };

    /* Open target file */
    if (!(target = fopen(argv[arg],"w+"))) {
        (void) fprintf(stderr, OPEN_MSG, argv[arg]);
        goto abort;
    };
    dst = argv[arg];

    /* Write target file header */
    header[0] = MAGIC_NUMBER;
    header[1] = 0;
    if (!fwrite(header, sizeof(header), 1, target)) {
        (void) fprintf(stderr, WRITE_MSG, dst);
        goto abort;
    };

    if (!stdinflag) arg++;

    /* For each source file... */
    while (arg < argc) {

        /* Open file */
        if (stdinflag) file = stdin;
        else if (!(file = fopen(argv[arg],"r"))) {
            (void) fprintf(stderr, OPEN_MSG, argv[arg]);
            goto abort;
        };
        src = stdinflag ? "stdin" : argv[arg];

        firstflag = TRUE;
	headerflag = FALSE;

        /* Process file */
        while ((c = getc(file)) != EOF) {

            /* Get next line */
            ptr1 = buffer;
            while (c != EOL) {
                *ptr1++ = c;
                c = getc(file);
            };
	    while (--ptr1 >= buffer) if (!isspace(*ptr1)) break;
            *++ptr1 = '\0';

            /* Ignore comments and blank lines */
            if ((ptr1 == buffer) || (buffer[0] == '#')) continue;

            /* Process directives */
            if (buffer[0] == '$') {

                /* $OFFSET */
                if (!strncmp(&buffer[1], "OFFSET", 6)) {
		    if (headerflag) {
                        /* Remember end of file */
                        if ((endptr = ftell(target)) == -1) {
                            (void) fprintf(stderr, STAT_MSG, dst);
                            goto abort;
                        };
                
                        /* Length of section */
                        size = endptr - fileptr;
                
                        /* Point to section size */
                        if (fseek(target, fileptr - sizeof(int), 0)) {
                            (void) fprintf(stderr, SEEK_MSG, dst);
                            goto abort;
                        };
                
                        /* Write section size */
                        if (!fwrite(&size, sizeof(size), 1, target)) {
                            (void) fprintf(stderr, WRITE_MSG, dst);
                            goto abort;
                        };
            
                        /* Back to end of file */
                        if (fseek(target, endptr, 0)) {
                            (void) fprintf(stderr, SEEK_MSG, dst);
                            goto abort;
                        };

                    } else if (!firstflag) {
                        (void) fprintf(stderr, DIR_MSG, src, buffer);
                        goto abort;
                    };

                    ptr1 = &buffer[7];
                    offset = strtoul(ptr1, &ptr2, 0);
                    if ((ptr1 == ptr2) OR *ptr2) {
                        (void) fprintf(stderr, OFFSET_MSG, src);
                        goto abort;
                    };
                    header[0] = offset;
                    header[1] = 0;
                    if (!fwrite(header, sizeof(header), 1, target)) {
                        (void) fprintf(stderr, WRITE_MSG, dst);
                        goto abort;
                    };
                    fileptr = ftell(target);
                    headerflag = TRUE;

                /* $<addr> */
                } else if (specialflag) {
		    if (!sflag) {
                        header[0] = ucode_addr;
                        header[1] = 0;
                        if (!fwrite(header, sizeof(header), 1, target)) {
                            (void) fprintf(stderr, WRITE_MSG, dst);
                            goto abort;
                        };
                        fileptr = ftell(target);
                        sflag = TRUE;
		    };
                    ptr1 = &buffer[1];
                    table_index = strtoul(ptr1, &ptr2, 0);
                    if ((table_index % sizeof(int)) OR (ptr1 == ptr2) OR 
		      *ptr2 OR ((table_index /= sizeof(int)) < 0) OR 
		      (table_index >= table_size)) {
                        (void) fprintf(stderr, DIR_MSG, src, buffer);
                        goto abort;
                    };
                    table[table_index] = ucode_addr;

                /* Unrecognized directive */
                } else {
                    (void) fprintf(stderr, DIR_MSG, src, buffer);
                    goto abort;
                };

		firstflag = FALSE;
                continue;

            } else {

                /* First non-comment line in a file must be a directive */
                if (firstflag) {
                    (void) fprintf(stderr, DIR_MSG, src, buffer);
                    goto abort;
                };

                /* Get codewords */
                ptr1 = buffer;
                while (isspace(*ptr1)) ptr1++;
                c = *ptr1;
                if ((c >= '0') AND (c <= '9'))
                    codeword[0] = strtoul(ptr1, &ptr2, 0);
                else {
                    (void) fprintf(stderr, CODEWORD_MSG, src);
                    goto abort;
                };
                while (isspace(*ptr2)) ptr2++;
                c = *ptr2;
                if ((c >= '0') AND (c <= '9'))
                    codeword[1] = strtoul(ptr2, &ptr1, 0);
                else {
                    (void) fprintf(stderr, CODEWORD_MSG, src);
                    goto abort;
                };
            };

            /* Dump codewords */
            if (!fwrite(codeword, sizeof(codeword), 1, target)) {
                (void) fprintf(stderr, WRITE_MSG, dst);
                goto abort;
            };

            ucode_addr++;
        };

        /* Close the file */
        if (!stdinflag AND fclose(file)) {
            (void) fprintf(stderr, CLOSE_MSG, src);
            src = (char *) 0;
            goto abort;
        };
        src = (char *) 0;

        /* End of section? */
        if ((++arg == top) OR headerflag) {

            /* Remember end of file */
            if ((endptr = ftell(target)) == -1) {
                (void) fprintf(stderr, STAT_MSG, dst);
                goto abort;
            };
    
            /* Length of section */
            size = endptr - fileptr;
    
            /* Point to section size */
            if (fseek(target, fileptr - sizeof(int), 0)) {
                (void) fprintf(stderr, SEEK_MSG, dst);
                goto abort;
            };
    
            /* Write section size */
            if (!fwrite(&size, sizeof(size), 1, target)) {
                (void) fprintf(stderr, WRITE_MSG, dst);
                goto abort;
            };

            /* Back to end of file */
            if (fseek(target, endptr, 0)) {
                (void) fprintf(stderr, SEEK_MSG, dst);
                goto abort;
            };
        };
    };

    /* Dump table */
    if (specialflag AND sflag) {
        header[0] = table_addr;
        header[1] = (table_size * sizeof(int)) | 0x80000000;
        if (!fwrite(header, sizeof(header), 1, target)) {
            (void) fprintf(stderr, WRITE_MSG, dst);
            goto abort;
        };
        if (!fwrite(table, sizeof(int), table_size, target)) {
            (void) fprintf(stderr, WRITE_MSG, dst);
            goto abort;
        };
    };

    /* Length of file */
    if ((size = ftell(target)) == -1) {
        (void) fprintf(stderr, STAT_MSG, dst);
        goto abort;
    };
    
    /* Seek length-of-file word */
    if (fseek(target, sizeof(int), 0)) {
        (void) fprintf(stderr, SEEK_MSG, dst);
        goto abort;
    };

    /* Write length-of-file word */
    if (!fwrite(&size, sizeof(size), 1, target)) {
        (void) fprintf(stderr, WRITE_MSG, dst);
        goto abort;
    };

    /* Close target */
    if (fclose(target)) {
        (void) fprintf(stderr, CLOSE_MSG, dst);
        dst = (char *) 0;
        goto abort;
    };

    exit(0);

    /* An error happened... */
abort:
    if (table) free(table);
    if (src AND !stdinflag AND fclose(file))
        (void) fprintf(stderr, CLOSE_MSG, src);
    if (dst AND fclose(target))
        (void) fprintf(stderr, CLOSE_MSG, dst);
    exit(1);

}
