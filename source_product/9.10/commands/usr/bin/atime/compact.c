/*

    compact_instruction()

    Copy up to two tokens of gBuffer to the target buffer, ignoring
    any label. This routine is used primarily to condense an assembly
    instruction down to the opcode and its operands (if any) for
    printing out a message to the user.

    This routine creates a buffer containing the instruction. The caller
    may then later free up this buffer by calling free().

*/

#include "fizz.h"

char *compact_instruction()
{
    char *ptr, *start1, *end1, *start2, *end2;
    char *buffer;
    unsigned long length;

    ptr = gBuffer;	/* complete instruction */
    start2 = (char *) 0;	/* assume no argument string */
    get_next_token(ptr, &start1, &end1); /* find first argument string */
    if (start1 != (char *) 0) {	/* first argument string exist? */
	if (*start1 == '#') start1 = (char *) 0;  /* check for comment */
	else {
	    if (*(end1 - 1) == ':') {	/* check for label */
		get_next_token(ptr, &start1, &end1);  /* scan over label */
		if (start1 != (char *) 0) {	/* is there a first argument? */
		    if (*start1 == '#') start1 = (char *) 0; /* comment? */
		    else {	/* find second argument string */
			get_next_token(ptr, &start2, &end2);
			if (*start2 == '#') start2 = (char *) 0; /* comment? */
		    };
		};
	    } else {	/* get second argument string */
		get_next_token(end1, &start2, &end2);
		if (*start2 == '#') start2 = (char *) 0;  /* comment? */
	    };
	};
    };
    length = 2; /* for blank between arguments and terminating null */
    if (start1 != (char *) 0) length += end1 - start1; /* 1st argument length */
    if (start2 != (char *) 0) length += end2 - start2; /* 2nd argument length */
    CREATE_STRING(buffer, length);  /* create buffer */
    length = 0;
    if (start1 != (char *) 0) {  /* copy arguments to buffer */
	length = end1 - start1;
	(void) strncpy(buffer, start1, length);
	if (start2 != (char *) 0) {
	    buffer[length++] = ' '; /* separate arguments with blank */
	    (void) strncpy(&buffer[length], start2, end2 - start2);
	    length += end2 - start2;
	};
    };
    buffer[length] = '\0';	/* null terminator */
    return buffer;
}
