static char *HPUX_ID = "@(#) $Revision: 70.2 $";
/* tail command 
**
**	tail where [file]
**	where is [+|-]n[type]
**	- means n lines before end
**	+ means nth line from beginning
**	type 'b' means tail n blocks, not lines
**	type 'c' means tail n characters
**	option 'f' means loop endlessly trying to read more
**		characters after the end of file, on the  assumption
**		that the file is growing
*/


#include	<stdio.h>
#include	<ctype.h>
#include	<sys/param.h>
#include	<sys/signal.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
#include 	<limits.h>	/* for LINE_MAX */
#define LINE_MAX 2048

#define	LBIN	((10*LINE_MAX)+1)   /* POSIX.2 defined minimum */

struct	stat	statb;
char	bin[LBIN];
extern	int	errno;
extern	char	*sys_errlist[];
int	follow;
int	piped;
extern int optind;
extern char *optarg;


main(argc,argv)
int argc;
char **argv;
{
	register i,j,k;
	long	n = -1,di;
	int	fromend = 1;
	int	partial,bylines = -1, byblocks = 0;
	char	*p,c;
	char	*arg;

	lseek(0,(long)0,1);
	piped = errno==ESPIPE;
	arg = argv[1];
	if(*arg == '+' || (*arg == '-' && isdigit(arg[1]))) {
	    fromend = *arg == '-';
	    if(isdigit(arg[1]))
		n = 0;
	    while(isdigit(*++arg))
		n = n*10 + *arg - '0';
	    if(*arg) {
		*--arg = '-';         /* put in form for getopt to finish */
		strcpy(argv[1],arg);
	    }
	    else     /* if arg was only +/-number, skip getopt */ 
		++optind;
	}
	    

	while((c = getopt(argc,argv,"flb:c:n:")) != EOF) {
	    switch(c) {
	    case 'b':
		byblocks = 1;
		if(bylines!=-1) goto errcom;
		if (n == -1) {  
			fromend = optarg[0] != '+';
			n = abs(atoi(optarg));
		}
		else
			optind--; 
		bylines=0;
		break;
	    case 'c':
		if(bylines!=-1 || byblocks==1) goto errcom;
		bylines=0;
		if (n == -1) {  
		    if ( optarg[0]=='f' )
			follow = 1;
		    else {
			fromend = optarg[0] != '+';
			n = abs(atoi(optarg));
		    }
		}
		else
		   if ( optarg[0]=='f' )  
			follow=1;
		   else
			optind--;    
		break;
	    case 'f':
		follow = 1;
		break;
	    case 'l':
		if(bylines!=-1) goto errcom;
		bylines=1;
		break;
	    case 'n':
		if(bylines!=-1) goto errcom;
		bylines=1;
		fromend = optarg[0] != '+';
		n = abs(atoi(optarg));
		break;
	    default:
		goto errcom;
	    }
	}

	if(bylines==-1) bylines = 1;
	if(n < 0) n = 10;
	if(!fromend&&n>0)
		n--;
	if(byblocks)
	    n *= 512;        /* blocksize of 512 for reporting purposes */

	if(argc - optind >= 1) {
		close(0);
		piped = 0;
		if(open(argv[optind],0)!=0) {
			if (errno == EOPNOTSUPP) {
				fputs("tail: ", stderr);
				fputs(argv[optind], stderr);
				fputs(": Operation not supported on socket\n", stderr);
			}
			else {
				fputs("tail: cannot open input\n", stderr);
				printf("error: %s on file %s\n",sys_errlist[errno],argv[optind]);
			}
			exit(2);
		}
	}

	if(fromend)
		goto keep;

			/*seek from beginning */

	if(bylines) {
		j = 0;
		while(n-->0) {
			do {
				if(j--<=0) {
					p = bin;
					j = read(0,p,512);
					if(j--<=0)
						fexit();
				}
			} while(*p++ != '\n');
		}
		write(1,p,j);
	} else  if(n>0) {
		if(!piped)
			fstat(0,&statb);
		if(piped||(statb.st_mode&S_IFMT)==S_IFCHR)
			while(n>0) {
				i = n>512?512:n;
				i = read(0,bin,i);
				if(i<=0)
					fexit();
				n -= i;
			}
		else
			lseek(0,n,0);
	}
	while((i=read(0,bin,512))>0)
		write(1,bin,i);
	fexit();

			/*seek from end*/

keep:
	if(n <= 0)
		fexit();
	if(!piped) {
		fstat(0,&statb);
		di = !bylines&&n<LBIN?n:LBIN-1;
		if(statb.st_size > di)
			lseek(0,-di,2);
	}
	partial = 1;
	for(;;) {
		i = 0;
		do {
			j = read(0,&bin[i],LBIN-i);
			if(j<=0)
				goto brka;
			i += j;
		} while(i<LBIN);
		partial = 0;
	}
brka:
	if(!bylines) {
		k =
		    n<=i ? i-n:
		    partial ? 0:
		    n>=LBIN ? i+1:
		    i-n+LBIN;
		k--;
	} else {
		k = i;
		j = 0;
		do {
			do {
				if(--k<0) {
					if(partial)
						goto brkb;
					k = LBIN -1;
				}
			} while(bin[k]!='\n'&&k!=i);
		} while(j++<n&&k!=i);
brkb:
		if(k==i) do {
			if(++k>=LBIN)
				k = 0;
		} while(bin[k]!='\n'&&k!=i);
	}
	if(k<i)
		write(1,&bin[k+1],i-k-1);
	else {
		write(1,&bin[k+1],LBIN-k-1);
		write(1,bin,i);
	}
	fexit();
errcom:
	fputs("usage: tail [+/-[n][lbc][f]] [file]\n", stderr);
	exit(2);
}
fexit()
{	register int n;
	if (!follow || piped) exit(0);
	for (;;)
	{	sleep(1);
		while ((n = read (0, bin, 512)) > 0)
			write (1, bin, n);
	}
}
