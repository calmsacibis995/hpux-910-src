/*


	print_byte_info(ptr)

	Print a character string as one or more byte instructions with
	each character converted to a decimal value. For example, if
	the input string is "hello", the output will be:

		byte	104,101,108,108,111

	For long strings, there will be multiple "byte" instructions
	printed.

*/

#include "fizz.h"

#define MAXCHAR  54

void print_byte_info(ptr)
register unsigned char *ptr;
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register unsigned int c, length;
    register unsigned char *bufptr;
    unsigned char buffer[MAXCHAR + 1];
    char *format = "\tbyte\t%s\n";

    bufptr = buffer;
    length = 0;

    while (c = (unsigned int) *ptr++) {	/* get next character */

	/* determine if this will fit on end of current byte instruction */
	if ((length >= MAXCHAR - 3) && ((c >= 100) || (length >= MAXCHAR - 1)
	  || ((length >= MAXCHAR - 2) && (c >= 10)))) {
	    *bufptr = '\0';
	    print(output, format, buffer);
            bufptr = buffer;
        } else if (length) *bufptr++ = ',';

	/* convert character to ASCII decimal format */
	if (c < 10) *bufptr++ = c + '0';
	else {
	    if (c >= 200) {
	        *bufptr++ = '2';
		c -= 200;
	    } else if (c >= 100) {
	        *bufptr++ = '1';
		c -= 100;
	    };
	    *bufptr++ = c / 10 + '0';
	    *bufptr++ = c % 10 + '0';
	};
	length = bufptr - buffer;
    };

    /* print any leftovers */
    if (length) {
	*bufptr = '\0';
	print(output, format, buffer);
    };

    return;
}
