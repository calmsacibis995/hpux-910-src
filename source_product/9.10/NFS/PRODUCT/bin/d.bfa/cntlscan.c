#include <stdio.h>

#include "defines.h"
#include "global.h"


int input() {
	static int send_cr = 0;
	static int hold    = 0;

	extern int yytchar;

	if(yyin == NULL)
	   return(0);

	if(hold) {
	   yytchar = hold;
	   hold    = 0;
	   return(yytchar);
	} /* if */

	if(stream_pos > 0) {
	   yytchar = stream_string[--stream_pos];
	   return(yytchar);
	} else if(yyin == NULL)
	   yytchar = 0;
	else
	   yytchar = getc(yyin);

	switch(yytchar) {
	   case '#'	:
			  send_cr = 1;
			  break;
	   case ';'	:
			  if(semi_cr)
			     send_cr = 1; 
			  break;
	   case '{'	:
			  send_cr = 1; 
			  break;
	   case '\n'	:
			  if(send_cr) {
			     send_cr = 0;
			     hold = SPECIAL_CHAR;
			  }  else {
			     yytchar = SPECIAL_CHAR;
			  } /* else */
			  break;
	   case EOF	:
			  yytchar = 0;
			  break;
	   default	:
			  break;
	} /* switch */

	return(yytchar);
} /* input */


void unput(c)
unsigned char	c;
{
	extern int yytchar;

	yytchar = c;

	if(yytchar == '\n')
	   return;


	if(stream_pos == stream_len) {
	   fprintf(stderr,"BFA Stream Table Overflow ");
	   fprintf(stderr,"(Current size %d) ",stream_len);
	   fprintf(stderr,"Try -S<number> to increase\n");
	   Exit(1);
	}  /* if */

	stream_string[stream_pos++] = yytchar;
	stream_string[stream_pos]   = '\0';

} /* unput */


void output(c)
char	c;
{
	char	string[2];

	string[0] = c;
	string[1] = '\0';

	ConvertString(string);

	fprintf(outfile,"%s",string);
} /* output */



void ConvertString(string)
char	*string;
{
	register int	i;

	for(i=0;string[i] != '\0';i++)
	   if(string[i] == SPECIAL_CHAR) {
	      line_number++;
	      string[i] = '\n';
	   } /* if */

} /* ConvertString */


int yywrap() {
	return(1);
} /* yywrap */



void SetLineNumber(string)
char	*string;
{
	int	i;

	ConvertString(string);
	fprintf(outfile,"%s",string);

	for(i=strlen(string)-1;string[i] >= '0' && string[i] <= '9';i--);

	line_number = atoi(&string[i+1]) - 1;
} /* SetLineNumber */



void CheckFileAndSetLineNumber(string)
char	*string;
{
	int	i;

	ConvertString(string);
	fprintf(outfile,"%s",string);

	string[strlen(string)-1] = '\0';

	for(i=strlen(string)-1;string[i] != '"';i--);

	if(strcmp(&string[i+1],input_files[cur_input-1]) && !i_flag) {
	   BeginScannerState(STATE_D);
	} else {
	   ReturnToOldState();
	   for(;string[i] < '0' || string[i] > '9';i--);
	   for(;string[i] >= '0' && string[i] <= '9';i--);

	   line_number = atoi(&string[i+1]) - 1;
	
	} /* else */

} /* CheckFileAndSetLineNumber */

