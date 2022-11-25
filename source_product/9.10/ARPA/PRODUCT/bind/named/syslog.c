#include <stdio.h>

extern char *sys_errlist[];
extern int errno;

syslog(a,b,c,d,e,f)
int  a;
char *b, *c, *d, *e, *f;
{
	FILE *sf;
	static int pid=0;
	static char *hostname=NULL;
	char *timestring=NULL;
	long t;
	int percent=0;
	int position=0;
	char *s;
	int tmperrno;
	char msg[256];

	tmperrno=errno;
	if(pid == 0)
		pid=getpid();
	if(hostname == NULL){
		hostname=(char *)malloc(255);
		gethostname(hostname,255);
	}
	t=(long)time((long *)0);
	timestring=(char *)ctime(&t);
	/* 
	 * Since ctime always puts the
	 * year in the 19th spot
         */
	if(timestring != NULL)
		timestring[19]='\0';

	if((sf=fopen("/usr/tmp/named.syslog","a")) == NULL)
		return(0);
	/*
	 * Copy the string in case
	 * it is modified by the next
	 * step.
	 */
	strcpy(msg,b);
	for(s=msg; *s!=NULL; s++)
		if(*s == '%'){
			percent++;
			if(*(s+1) == 'm'){
				*(s+1) = 's';
				position=percent;
			}
		}
	fprintf(sf,"%s %s named[%d]: ", timestring+4, hostname, pid);
	switch(position){
		case 0: fprintf(sf,msg,c,d,e,f); break;
		case 1: fprintf(sf,msg,sys_errlist[tmperrno],c,d,e,f); break;
		case 2: fprintf(sf,msg,c,sys_errlist[tmperrno],d,e,f); break;
		case 3: fprintf(sf,msg,c,d,sys_errlist[tmperrno],e,f); break;
		case 4: fprintf(sf,msg,c,d,e,sys_errlist[tmperrno],f); break;
		case 5: fprintf(sf,msg,c,d,e,f,sys_errlist[tmperrno]); break;
		default: fprintf(sf,"percent m in position %d\n",position); break;
	}
	fprintf(sf,"\n");
	fflush(sf);
	fclose(sf);
}

openlog()
{}
