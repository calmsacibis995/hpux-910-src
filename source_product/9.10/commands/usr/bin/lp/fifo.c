/* @(#) $Revision: 66.1 $ */    
/* enqueue(cmd, args) -- writes command and args for scheduler on FIFO */

#include	"lp.h"

static FILE *fifo = NULL;

static int
sig14()
{
	signal(SIGALRM, sig14);
}

/* enqueue -- returns  0 for success.                     */
/*            returns -1 to indicate a pipe write failure */
/*            returns -2 to indicate some other failure   */

enqueue(cmd, args)
char cmd;
char *args;
{
	int i = 0;
	unsigned alarm();

	if(GET_ACCESS(FIFO, ACC_W) == 0) {
		if(fifo == NULL) {
			sig14();
			alarm(ALTIME);
			fifo = fopen(FIFO, "w");
			alarm(0);
		}
		if(fifo != NULL) {
			alarm(ALTIME);
			i = fprintf(fifo, "%c %s\n", cmd, args);
			alarm(0);
			if(i != 0)
				fflush(fifo);
			else
				return(-1);
		}
	}
	else
		fifo = NULL;

	return(fifo == NULL ? -2 : 0);
}
