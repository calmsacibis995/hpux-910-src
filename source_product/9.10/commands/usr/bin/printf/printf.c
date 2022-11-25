static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
 * AW: POSIX.2/D11.2 Conformance: Added %b conversion character processing.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#if defined(NLS) || defined(NLS16)
#include <locale.h>
#include <setlocale.h>
#include <nl_ctype.h>
#include <nl_types.h>
#endif

#ifndef NLS		/* NLS must be defined */
#define catgets(i,sn,mn,s) (s)
#else /* NLS */
#define NL_SETN 1	/* set number */
nl_catd catd;
#endif /* NLS */

int errs;
int cflag = 0;		/* '\c' character flag in %b option */

main(argc, argv)
int argc;
char **argv;
{
	char fmt[BUFSIZ], **argptr;
	char *f, *fptr, *failptr = NULL, *next; 
	float ftmp;
	double dtmp;
	int    itmp, val, not_num, i;
	int 	converted = 0;
	long   ltmp;
	unsigned long utmp;

#if defined(NLS) || defined(NLS16)
	if (!setlocale(LC_ALL,"")) {
	    fprintf(stderr, _errlocale("printf"));
	    putenv("LANG=");
	    catd = (nl_catd)-1;
	} 
	else
	    catd = catopen("printf", 0);
#endif

	fptr = argv[1];
	argptr = &argv[2];
	f = fmt - 1;    	/* Let's us start w/ an increment */

#ifdef DEBUG
	for(i = 0; argv[1][i] != 0; i++)
	    printf(" %o",argv[1][i]);
	putc('\n',stdout);
#endif

	do{		/* while *argptr */
 	    while((*++f = *fptr++) != NULL) {

	        if(*f == '%') {
	advance:    *++f = *fptr++;
			switch (*f) {
			    case 's':
				*++f = 0;
				if(printf(fmt,*argptr) < 0)
				    error('p',*argptr);
				break;
			    case 'c':
				*++f = 0;
				if(printf(fmt,*argptr[0]) < 0)
				    error('p',*argptr);  
			       	break;
			    case 'd':
			    case 'i':
	            		not_num = check_num(*argptr,&val);
				*++f = 0;
				if(not_num) {
				    printf(fmt,val);
				}
				else {
				    itmp = (int)strtol(*argptr,&failptr,0);
				    if (*failptr != NULL || errno == ERANGE) {
					 error('c',*argptr);
					*(f-1) = 's';
					printf(fmt,*argptr);
				    }
				    else 
					printf(fmt,itmp);
				}
				break;
			    case 'o':
			    case 'u':
			    case 'x':
			    case 'X':
	            		not_num = check_num(*argptr,&val);
				*++f = 0;
				if(not_num) {
				    printf(fmt,val);
				}
				else {
				    utmp = (unsigned)strtoul(*argptr,&failptr,0);
				    if (*failptr != NULL || errno == ERANGE) {
					 error('c',*argptr);
					*(f-1) = 's';
					printf(fmt,*argptr);
				    }
				    else 
					printf(fmt,utmp);
				}
				break;
			    case 'f':
	            		not_num = check_num(*argptr,&val);
				*++f = 0;
				if(not_num) {
				    printf(fmt,val);
				}
				else {
				    ftmp = (float)strtod(*argptr,&failptr);
				    if (*failptr != NULL || errno == ERANGE) {
				 	error('c',*argptr);
					*(f-1) = 's';
					printf(fmt,*argptr);
				    }
				    else 
					printf(fmt,ftmp);
				}
				break;
			    case 'e':
			    case 'E':
			    case 'g':
			    case 'G':
	            		not_num = check_num(*argptr,&val);
				*++f = 0;
				if(not_num) {
				    printf(fmt,val);
				}
				else {
				    dtmp = strtod(*argptr,&failptr);
				    if (*failptr != NULL || errno == ERANGE) {
					error('c',*argptr);
					*(f-1) = 's';
					printf(fmt,*argptr);
				    }
				    else 
					printf(fmt,dtmp);
				}
				break;
			    /* POSIX.2/D11.2 addition */
			    case 'b':	/* process '\'escape sequences */
				/* convert to 's' */
				*f = 's';	/* change 'b' to 's' */
				*++f = 0;
				process_arg(*argptr);
				if(printf(fmt, *argptr) < 0)
					error('p', *argptr);
				if(cflag) return(0);	/* terminate */
				break;
			    case '\0':
			        break;
			    default: 
				goto advance;
	    	        }          /*  switch *f */
			
			if(*argptr)  /* don't bump if already NULL */
	    		    argptr++;
			f = fmt - 1;      /* reset fmt, failptr, errno */  
			failptr = NULL;
			errno = 0;
			++converted;
	    		
	        }	/*  if '%' */
		else {
		    if(*f == '\\') {
			switch(*fptr) {
			case 'a':  *f = '\a';
				   *fptr++;
				   break;
			case 'b':  *f = '\b';
				   *fptr++;
				   break;
			case 'f':  *f = '\f';
				   *fptr++;
				   break;
			case 'n':  *f = '\n';
				   *fptr++;
				   break;
			case 'r':  *f = '\r';
				   *fptr++;
				   break;
			case 't':  *f = '\t';
				   *fptr++;
				   break;
			case 'v':  *f = '\v';
			   	   *fptr++;
			   	   break;
			case '\\':  *f = *++fptr;
			   	    break;
			default:   val = 0;
			   /* if next 3 are digits, treat as octal num */
			   	   for(i = 0; i <= 2 && isdigit(fptr[i]); i++)
			   		 val = (val * 8) + fptr[i] - '0';
			   	   if(i > 0 && val >= 0 && val < 256) {
				          *f = val;
					  fptr = &fptr[i];
				   }	  
				   else {
				        *f = *fptr++;
					continue;
				   }
			}  /* end switch(*f) */
		    }      /* if '\' */
		}          /* else (not '%') */

	    }   	   /*  while fptr */

	    *++f = 0;
#ifdef DEBUG
	printf("At end of inner loop\n");
	for(i = 0; fmt[i] != 0; i++)
	    printf(" %o",fmt[i]);
#endif
	    printf(fmt);
	    if(*argptr != NULL){
	        fptr = argv[1];
	        f = fmt - 1;
	    }
	}while(*argptr && converted);
	/*  Will exit after first pass if no conversions
	    took place  */
	
	exit(errs);
}

/*  Checks whether arg starts off looking like a number
 *  Returns 1 if not a number, and sets val to encoded value of first
 *  character; returns 0 if arg looks like a number.
 */
int check_num(arg,val)
char *arg;
int  *val;
{
	    if( arg[0] == '-' || arg[0] == '+')
		return(0);
  	    if (arg[0] == '"' || arg[0] == '\'') {
		*val = (int)arg[1];
		return(1);
	    }
	    
	    if (!isdigit(arg[0])) {
		*val = (int)arg[0];
		return(1);
	    }

	    return(0);
}

int
error(type, arg)
char type;
char *arg;
{
   	char *msg;
	int  num;
	
	switch(type) {
	   case 'p':	msg = "printf:  Error printing %s\n";	/* catgets 1 */
			num = 1;
	      		break;
	   case 'c':	msg = "printf:  Error converting %s\n";	/* catgets 2 */
			num = 2;
	      		break;
	}
	
	fprintf(stderr,catgets(catd, NL_SETN, num, msg), arg);
	++errs;
}

/* POSIX.2/D11.2 compliance */
process_arg(s)
char *s;
{
register char *p, c;
int val, i;

	/* process backslash-escapes in the string */
	while(*s) {
		/* check backslash escapes */
		if(*s++ == '\\') {
			p = s-1;
			switch(*s++) {
				case 'a':  *p++ = '\a';
					   break;
				case 'b':  *p++ = '\b';
					   break;
				case 'f':  *p++ = '\f';
					   break;
				case 'n':  *p++ = '\n';
					   break;
				case 'r':  *p++ = '\r';
					   break;
				case 't':  *p++ = '\t';
					   break;
				case 'v':  *p++ = '\v';
					   break;
				case 'c':  cflag = 1; /* set flag */
					   *s = '\0'; /* terminate processing */
					   break;
				case '\\':
					   break;
				default:   val = 0;
					   s--;
				/* if next 3 are digits, treat as octal num */
					   for(i = 0; i <= 2 && isdigit(s[i])
					       && s[i] < '8'; i++)
						val = (val * 8) + s[i] - '0';
					   if(i > 0 && val >= 0 && val < 256)
						*p++ = val;
					   s = &s[i];
			} /* switch *s */
			compress(s, p);		/* compress string */
			s = p;
		} /* if backslash */
	} /* while */
}

compress(s, p)
char *s, *p;
{
	while (*s) *p++ = *s++;
	*p = '\0';
}
