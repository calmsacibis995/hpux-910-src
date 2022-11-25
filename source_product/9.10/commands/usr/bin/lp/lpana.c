#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "lp.h"

#ifndef NLS
#define nl_msg(i,s) (s)
#else NLS
#define NL_SETN 22
#include <msgbuf.h>
#endif NLS

#define MEMBLK	10			/* to get memory as struct logging */

struct logging {	/*  logging structure  */
	char name[DESTMAX];	/*  printer/class name  */
	long wait;		/*  sum of waiting time  */
	long wait2;		/*  sum of squared waiting time  */
	long print;		/*  sum of printing time  */
	long print2;		/*  sum of squared printing time  */
	long size;		/*  sum of bytes  */
	long size2;		/*  sum of squared bytes  */
	long requests;		/*  sum of number of requests  */
};

char numbuf[30];		/*  buffer for time value  */
char prname[80];		/*  buffer for request-id  */

long bytes;			/*  number of request  */
long accept_time;		/*  time when request is accepted */
long start;			/*  time when printing is started */
long pend;			/*  time when printing is finished */
int cur_pr;			/*  number of printers/classes  */

struct logging *log_str;
long start_date=0L;		/*  time when first request is accepted  */
long end_date=0L;		/*  time when last printing is finished  */

long sd;			/* standard deviation */

FILE *fp;
char *cmd;			/*  name of this command  */
char *destname;			/*  destination which is specified in -d opt */

main(argc, argv)
	int argc;
	char **argv;
{
	int i;
	static char fname[256];		/*  logging file name  */
	int c;
	extern char *optarg;
	extern int optind;

#ifdef NLS
	nl_init(getenv("LANG"));
	nl_catopen("lp");
#endif NLS

	cmd = argv[0];
	fname[0] = '\0';

	while ((c=getopt(argc,argv,"d:")) != EOF) {
		switch (c) {
		case 'd' :	destname = optarg;
				break;
		case '?' :	fprintf(stderr,
			  nl_msg(1,"usage: %s [-ddestination] [filename]\n"),
									  cmd);
				exit(1);
		}
	}

	if ( argv[optind] != NULL ) {
		strcpy( fname, argv[optind] );
	} else {
		strcpy( fname, SPOOL );
		strcat( fname, "/");
		strcat( fname, LPANA );
	}

	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,nl_msg(2,"%s: cannot open %s\n"),cmd,fname);
		exit(1);
	}
	lockf(fileno(fp), F_LOCK, 0);

	floating_exception();

	cur_pr = 0;
	get_logdata();

	rewind(fp);
	lockf(fileno(fp), F_ULOCK, 0);
	close(fp);

	if ( cur_pr )  print_logdata();

	exit(0);
}

/*  read a item separated by a space character  */
int get_item(p)
	char *p;	/*  pointer to buffer  */
{
	char ch;

	while((ch = getc(fp)) != ' ' && ch != '\n' && ch != EOF) {
		*p++ = ch;
	}
	*p = '\0';

	switch( ch ) {
		case ' '  : return( 0 );
		case '\n' : return( 1 );
		case EOF  : return( -1 );
	}
}

/*  read a logging file into logging structure  */
get_logdata()
{
	struct logging *log_p;
	int pr_num;		/*  current number of printers  */
	int blocks;		/*  current number of logging structures  */
	char *p;
	int n;

	pr_num = 0;
	blocks = MEMBLK;	/*  get a logging structure in initial size  */
	if ( (log_str = (struct logging *)malloc( sizeof(struct logging)
							* blocks)) == NULL) {
		fprintf(stderr,nl_msg(4,"%s: memory allocation error\n"),cmd);
		exit(1);
	}

	for(;;) {
	/*  printer name  */
	    if ( (n = get_item(prname)) < 0)	return;
	    else if (n != 0)		continue;

	/*  extract printer name from request-id string  */
	    if ((p = strchr(prname,'-')) == NULL ||
			p == prname || p - prname > DESTMAX) {
		fprintf(stderr,nl_msg(3,"%s: logfile format error\n"),cmd);
		return;
	    }
	    *(prname + (p - prname)) = '\0';

	/*  number of bytes  */
	    if ( (n = get_item(numbuf)) < 0)	return;
	    else if (n != 0)	continue;
	    bytes = atol(numbuf);

	/*  accept time  */
	    if ( (n = get_item(numbuf)) < 0)	return;
	    else if (n != 0)	continue;
	    accept_time = atol(numbuf);

	/*  printing start time  */
	    if ( (n = get_item(numbuf)) < 0)	return;
	    else if (n != 0)	continue;
	    start = atol(numbuf);

	/*  printing finish time  */
	    get_item(numbuf);
	    pend = atol(numbuf);

	    if ( start > pend )		continue;

	    if (start_date == 0L) start_date = accept_time;
	    if (end_date < pend)  end_date = pend;

	/*  if -d option is specified, printer name is checked  */
	    if (destname != NULL && strcmp(destname,prname))   continue;

	/*  save logging data into logging structure  */
	    log_p = log_str;
	    pr_num = 0;
	    for(;;) {
		if (pr_num == cur_pr) {		/*  new printer appears */
			if (++cur_pr > blocks) {
				/*  extend a logging structure  */
				if ((log_str=(struct logging *)realloc(log_str,
				    sizeof(struct logging)*(blocks += MEMBLK)))
							== NULL) {
				  fprintf(stderr,
				nl_msg(4,"%s: memory allocation error\n"),cmd);
				  exit(1);
				}
				log_p = &log_str[cur_pr - 1];
			}
			/*  initialize new printer data  */
			strcpy(log_p->name,prname);
			log_p->wait = 0L;
			log_p->wait2 = 0L;
			log_p->print = 0L;
			log_p->print2 = 0L;
			log_p->size = 0L;
			log_p->size2 = 0L;
			log_p->requests = 0L;
		}
		else if (strcmp(log_p->name,prname)) {
			pr_num++;
			log_p++;
			continue;
		}
		/*  compute sum of item and save them  */
		log_p->wait += (start - accept_time);
		log_p->wait2 += (start - accept_time) *
				(start - accept_time);
		log_p->print += (pend - start);
		log_p->print2 += (pend - start) * (pend - start);
		log_p->size += bytes;
		log_p->size2 += bytes * bytes;
		log_p->requests++;
		break;
	    }
	}
}

int compare(p1,p2)
	struct logging *p1,*p2;
{
	return( strcmp(p1->name,p2->name) );
}

/*  sqrt(3M) error handling  */
int matherr(x)
	struct exception *x;
{
	x->retval = 0.0;
	return(1);
}

/*  calculation of standard deviation  */
double sdval(x1,x2,mean,num)
	double x1,x2,mean,num;
{
	if ( num <= 1.0 ) {
		return(0.0);
	} else {
		return( sqrt( (x2 - x1 * mean) / (num - 1.0) ) );
	}
}

/*  print a second in "min'sec" format  */
#define print_sec(x) printf(" %l3d'%l.2d",x/60,x%60)

/*  print results  */
print_logdata()
{
	long av, kbyte;		/*  average */
	double x1,x2,mean,num;	/*  temporary variables  */
	struct logging *p;
	int i;

	/*  header  */
	printf(nl_cxtime(&start_date,
		nl_msg(5,"performance analysis is done from %h.%d '%y %H:%M")));

	printf(nl_cxtime(&end_date,nl_msg(6," through %h.%d '%y %H:%M\n")));

	printf(nl_msg(7, "---printers     ----wait----   ---print---    ---bytes---   -sum- num_of\n"));

	printf(nl_msg(8, "    /classes--     AV     SD     AV     SD      AV     SD      KB requests\n"));

	/*  sort by printer name  */
	qsort(log_str,cur_pr,sizeof (struct logging),compare);

	p = log_str;
	for (i=0; i<cur_pr; i++) {
	/*  pinter/class name  */
		printf("%-14s",p->name);
	/*  waiting time  */
		x1 = p->wait;
		x2 = p->wait2;
		num = p->requests;
		mean = x1 / num;
		av = mean + 0.5;
		print_sec(av);
		sd = sdval(x1,x2,mean,num) + 0.5;
		printf(" %l6d",sd);
	/*  printing time  */
		x1 = p->print;
		x2 = p->print2;
		num = p->requests;
		mean = x1 / num;
		av = mean + 0.5;
		print_sec(av);
		sd = sdval(x1,x2,mean,num) + 0.5;
		printf(" %l6d",sd);
	/*  bytes  */
		x1 = p->size;
		x2 = p->size2;
		num = p->requests;
		mean = x1 / num;
		av = mean + 0.5;
		sd = sdval(x1,x2,mean,num) + 0.5;
		printf(" %l7d %l6d",av,sd);
	/*  sum of bytes  */
		kbyte = x1 / 1024.0 + 0.5;
		printf(" %l7d",kbyte);
	/*  num of requests  */
		printf(" %l8d\n",p->requests);
		p++;
	}
}

floating_exception()
{
	signal(SIGFPE,floating_exception);
	sd = 0L;
	return;
}
