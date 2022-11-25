static char *HPUX_ID = "@(#) $Revision: 64.1 $";
#include <stdio.h>
/************************
	hyphen.c
	find hyphenated words in input stream
**************************/
char b[242];			/* holds running input stream */
char c[60];			/* holds legal alpha chars and -'s while
				   decision made as to whether or not this
				   qualifies as a hyphenated word    */
int nread = 1;			/* get-another-bufferful flag */
char buf[512];			/* contains stdin stream */

main(argc,argv)
int argc;
char *argv[];
{
	int l,isw,k,ifile,i,j;	/* l: argv index, isw: is a qaulifying word
				   flag, k: print loop variable, ifile: input 
				   file descriptor, i and j loop/index 
				   variables  */

	if(--argc <= 0)		/* determine from where input is comming */
		{ifile = 0;
		argc = 0;
		goto newl;	/* input from standard-in so jump around
				   while loop */
		}

	l = 1;
	while(argc--)	  /* for each input file */
		{fputc('\n', stdout);
		fputs(argv[l], stdout);
		fputs(":\n", stdout);
		ifile = open(argv[l++],0);
		if(ifile < 0)
			{fputs("cannot open input file\n", stdout);
			exit(0);
			}

newl:	isw = j =  0;	/* clear ishyphenatedword flag and set c index */
	i = -1;		/* initialize b index */

cont:	while((b[++i] = get(ifile,i)) != 0)	/* get char in running
						   stream b */
		{if((b[i] >= 'a' && b[i] <= 'z') ||
		(b[i] >= 'A' && b[i] <= 'Z') ||
		(b[i] == '\''))	 		/* if legal copy to  */
			{c[j++] = b[i];		/* c  and go get */
			goto cont;		/* another char */
			}
		if(b[i] == '-')			/* if char is potential 
						   hyphen then */
		{c[j++] = b[i];			
			if(j == 1)goto newl;	/* if first char is '-' with
						   no preceeding chars then
						   start over */
			if((b[++i] = get(ifile,i)) != '\n')  /* next char is 
							     not newline */
			  {if((b[i] >= 'a' && b[i] <= 'z') ||
			  (b[i] >= 'A' && b[i] <= 'Z') ||
			  (b[i] == '\''))	 	/* if legal copy to  */
			    {c[j++] = b[i];		/* c  and go get */
			    goto cont;		/* another char */
			    }
			  else
			    { goto newl; }	/* else start next word/line */
			  }

			if(j == 1)goto newl;	/* if first char is '-' followed
						   by new line start
						   over  */			
			isw = 1;		/* valid? word found set flag*/
			i = -1;			/* reset for new line */
			while(((b[++i] = get(ifile,i)) == ' ')	/* read past */
			|| (b[i] == '\t') || (b[i] == '\n'));	/* white space*/

			if((b[i] >= 'a' && b[i] <= 'z') ||
			(b[i] >= 'A' && b[i] <= 'Z') ||
			(b[i] == '\''))	 		/* if legal copy to  */
			    {c[j++] = b[i];		/* c  and go get */
			    goto cont;		/* another char */
			    }
			else
			    {goto newl;	}	/* else start again */	
		}
		if(b[i] == '\n'){if(isw != 1)goto newl;  /* if char is newline*/ 
						/* and no hyphen was seen 
						   then start over with fresh
						   line of input */
			i = -1; }		/* otherwise reset b index */
		if(isw == 1)		/* if it is a hyphenated word put it */
			{k = 0;		/* to stdout */
			c[j++] = '\n';
			while(k < j)putchar(c[k++]);
			}
		isw = j = 0;		/* reset isvalidword flag and c index
					   for next words */
		}	/* end while get a char from input loop */
	}		/* end while still files left to be scaned loop */
}		/* end main */
/* ------------------------------------------------ */
get(ifile, byte_count)
int ifile, byte_count;
{
	static char *ibuf;

	if (byte_count > 242) {
		fputs("line is longer than 242 bytes, exiting\n", stdout);
		return (0);
		}
	if(--nread)return(*ibuf++);	/* return chars from buffer as long as
					   any left */

	if(nread = read(ifile,buf,512)) /* get some more chars into buf from */
		{if(nread < 0)goto err; /* input stream */
		ibuf = buf;
		return(*ibuf++);
		}

	nread = 1;			/* reset number read flag/counter */
	return(0);

err:	nread = 1;
	fputs("read error\n", stdout);
	return(0);
}
