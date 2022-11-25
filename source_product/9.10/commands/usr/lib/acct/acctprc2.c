/* @(#) $Revision: 70.1 $ */     
/*
 *	acctprc2
 *
 *	Acctprc2 reads records in the form written by acctprc1,
 *	summarizes them by user ID and name, then writes the sorted
 *	summaries to the standard output as total accounting records.
 *
 * Modifications:
 *		11/6/91 : AW
 *		Removing users and sessions limitations for 9.0
 *		(5000 users enhancements)
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include "ptmp.h"
#include "tacct.h"
#ifdef NLS || NLS16
#include <locale.h>
#endif NLS || NLS16

struct	ptmp	pb;
struct	tacct	tb;

struct	utab	{
	uid_t	ut_uid;
	char	ut_name[NSZ];
	float	ut_cpu[2];	/* cpu time (mins) */
	float	ut_kcore[2];	/* kcore-mins */
	long	ut_pc;		/* # processes */
	struct	utab *ut_ptr;	/* forward pointer to the list */
};
struct utab *ub[A_USIZE];

static	usize;
static struct utab *usave;
int	ucmp();

main(argc, argv)
char **argv;
{
int i;
	usize = 0;
	for(i = 0; i < A_USIZE; i++)
		ub[i] = NULL;
	
#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("acctprc2"), stderr);
		putenv("LANG=");
	}
#endif NLS || NLS16

/*	while (scanf("%hu\t%s\t%lu\t%lu\t%u", */
	while (scanf("%ld\t%s\t%lu\t%lu\t%u",
		&pb.pt_uid,
		pb.pt_name,
		&pb.pt_cpu[0], &pb.pt_cpu[1],
		&pb.pt_mem) != EOF)
			enter(&pb);
	squeeze();
	qsort(usave, usize, sizeof(struct utab), ucmp);
	output();
}


enter(p)
register struct ptmp *p;
{
	register unsigned i;
	register struct utab *up;
	struct utab **q;
	int j;
	double memk;

	/* clear end of short users' names */
	for(i = strlen(p->pt_name) + 1; i < NSZ; p->pt_name[i++] = '\0') ;
	/* now hash the uid and login name */
	for(i = j = 0; p->pt_name[j] != '\0'; ++j)
		i = i*7 + p->pt_name[j];
	i = i*7 + p->pt_uid;

	q = &ub[i % A_USIZE];		/* get hash index */
	while(*q != NULL) {		/* search linked list */
		if ((*q)->ut_name[0])
			break;		/* found entry */
		q = &((*q)->ut_ptr);
	}
	if(*q == NULL) {		/* allocate buffer */
		up = (struct utab *) calloc(1, sizeof(struct utab));
		if(up == NULL) {
			printf("acctprc2: out of memory\n");
			exit(1);
		}
		*q = up;		/* update entry */
		up->ut_ptr = NULL;
		up->ut_uid = p->pt_uid;
		CPYN(up->ut_name, p->pt_name);
		++usize;
	}

	(*q)->ut_cpu[0] += MINT(p->pt_cpu[0]);
	(*q)->ut_cpu[1] += MINT(p->pt_cpu[1]);
	memk = KCORE(pb.pt_mem);
	(*q)->ut_kcore[0] += memk * MINT(p->pt_cpu[0]);
	(*q)->ut_kcore[1] += memk * MINT(p->pt_cpu[1]);
	(*q)->ut_pc++;
}

squeeze()		/*eliminate holes in hash table*/
{
	int i;
	register struct utab *up, *ut;
	struct utab *ux;
	
	up = (struct utab *) calloc(usize, sizeof(struct utab));
	if(up == NULL) {
		printf("acctprc2: out of memory\n");
		exit(1);
	}
	usave = up;
	for (i = 0; i < A_USIZE; i++) {
		ut = ub[i];
		while(ut != NULL) {
			up->ut_uid = ut->ut_uid;
			CPYN(up->ut_name, ut->ut_name);
			up->ut_cpu[0] = ut->ut_cpu[0];
			up->ut_cpu[1] = ut->ut_cpu[1];
			up->ut_kcore[0] = ut->ut_kcore[0];
			up->ut_kcore[1] = ut->ut_kcore[1];
			up->ut_pc = ut->ut_pc;
			ux = ut->ut_ptr;
			free(ut);
			ut = ux;
			up++;
		}
	}
}

ucmp(p1, p2)
register struct utab *p1, *p2;
{
	if (p1->ut_uid != p2->ut_uid)
		/* the following (short) typecasts are a kludge fix
		 * for a bug in the 5.0 C compiler.  The bug returns a
		 * result that is always positive from the subtraction
		 * because of the unsigned short type of ut_uid.
		 */
		return((short)p1->ut_uid - (short)p2->ut_uid);
	return(strcoll(p1->ut_name, p2->ut_name));
}

output()
{
	register i;
	register struct utab *up;

	up = usave;
	for (i = 0; i < usize; i++) {
		tb.ta_uid = up->ut_uid;
		CPYN(tb.ta_name, up->ut_name);
		tb.ta_cpu[0] = up->ut_cpu[0];
		tb.ta_cpu[1] = up->ut_cpu[1];
		tb.ta_kcore[0] = up->ut_kcore[0];
		tb.ta_kcore[1] = up->ut_kcore[1];
		tb.ta_pc = up->ut_pc;
		fwrite(&tb, sizeof(tb), 1, stdout);
		up++;
	}
}

