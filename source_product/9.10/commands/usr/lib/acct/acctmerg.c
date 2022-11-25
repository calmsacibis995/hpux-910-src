/* @(#) $Revision: 66.3 $ */      
/*
 *	acctmerg [options] [file] ...
 *
 *	Acctmerg reads its standard input and up to nine additional
 *	files, all in the tacct format (see acct(5)) or an ASCII version
 *	thereof. It merges these inputs by adding records whose keys
 *	(normally user ID and name) are identical, and expects the
 *	inputs to be sorted on those keys. Options are:
 *
 *	-a	Produce output in ASCII version of tacct.
 *	-i	Input files are in ASCII version of tacct.
 *	-p	Print input with no processing.
 *	-t	Produce a singe record that totals all input.
 *	-u	Summarize by user ID, rather than user ID and name.
 *	-v	Produce output in verbose ASCII format, with more 
 *		precise notation for floating point numbers.
 *
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include <memory.h>
#include "tacct.h"

#define NFILE	40
int	nfile;			/* index of last used in fl */
FILE	*fl[NFILE]	= {stdin};

struct	tacct tb[NFILE];	/* current record from each file */
struct	tacct	tt = {
	0,
	"TOTAL"
};
int	asciiout;
int	asciiinp;
int	printonly;
int	totalonly;
int	uidsum;
int	verbose;
struct tacct	*getleast();

main(argc, argv)
char **argv;
{
	register i;
	register struct tacct *tp;

	while (--argc > 0) {
		if (**++argv == '-')
			switch (*++*argv) {
			case 'a':
				asciiout++;
				continue;
			case 'i':
				asciiinp++;
				continue;
			case 'p':
				printonly++;
				continue;
			case 't':
				totalonly++;
				continue;
			case 'u':
				uidsum++;
				continue;
			case 'v':
				verbose++;
				asciiout++;
				continue;
			}
		else {
			if (++nfile >= NFILE) {
				fprintf(stderr, "acctmerg: File limit exceeded\n");
				exit(1);
			}
			if ((fl[nfile] = fopen(*argv, "r")) == NULL) {
				fprintf(stderr, "acctmerg: Cannot open %s\n", *argv);
				exit(1);
			}
		}
	}

	if (printonly) {
		for (i = 0; i <= nfile; i++)
			while (getnext(0))
				prtacct(&tb[0]);
		exit(0);
	}

	for (i = 0; i <= nfile; i++)
		if(getnext(i) == 0)
			fprintf(stderr,"acctmerg: Read error file %d\n",i);

	while ((tp = getleast()) != NULL)	/* get least uid of all files, */
		sumcurr(tp);			/* sum all entries for that uid, */
	if (totalonly)				/* and write the 'summed' record */
		output(&tt);
}

/*
 *	getleast returns ptr to least (lowest uid)  element of current 
 *	avail, NULL if none left; always returns 1st of equals
 */
struct tacct *getleast()
{
	register struct tacct *tp, *least;

	least = NULL;
	for (tp = tb; tp <= &tb[nfile]; tp++) {
		if (tp->ta_name[0] == '\0')
			continue;
		if (least == NULL ||
			tp->ta_uid < least->ta_uid ||
			((tp->ta_uid == least->ta_uid) &&
			!uidsum &&
			(strncmp(tp->ta_name, least->ta_name, NSZ) < 0)))
			least = tp;
	}
	return(least);
}

/*
 *	sumcurr sums all entries with same uid/name (into tp->tacct record)
 *	writes it out, gets new entry
 */
sumcurr(tp)
register struct tacct *tp;
{
	struct tacct tc;

	memcpy(&tc, tp, sizeof(struct tacct));
	tacctadd(&tt, tp);	/* gets total of all uids */
	getnext(tp-&tb[0]);	/* get next one in same file */
	while (tp <= &tb[nfile])
		if (tp->ta_name[0] != '\0' &&
			tp->ta_uid == tc.ta_uid &&
			(uidsum || EQN(tp->ta_name, tc.ta_name))) {
			tacctadd(&tc, tp);
			tacctadd(&tt, tp);
			getnext(tp-&tb[0]);
		} else
			tp++;	/* look at next file */
	if (!totalonly)
		output(&tc);
}

tacctadd(t1, t2)
register struct tacct *t1, *t2;
{
	t1->ta_cpu[0] = t1->ta_cpu[0] + t2->ta_cpu[0];
	t1->ta_cpu[1] = t1->ta_cpu[1] + t2->ta_cpu[1];
	t1->ta_kcore[0] = t1->ta_kcore[0] + t2->ta_kcore[0];
	t1->ta_kcore[1] = t1->ta_kcore[1] + t2->ta_kcore[1];
	t1->ta_con[0] = t1->ta_con[0] + t2->ta_con[0];
	t1->ta_con[1] = t1->ta_con[1] + t2->ta_con[1];
	t1->ta_du = t1->ta_du + t2->ta_du;
	t1->ta_pc += t2->ta_pc;
	t1->ta_sc += t2->ta_sc;
	t1->ta_dc += t2->ta_dc;
	t1->ta_fee += t2->ta_fee;
}

output(tp)
register struct tacct *tp;
{
	if (asciiout)
		prtacct(tp);
	else
		fwrite(tp, sizeof(*tp), 1, stdout);
}
/*
 *	getnext reads next record from stream i, returns 1 if one existed
 */
getnext(i)
register i;
{
	register struct tacct *tp;

	tp = &tb[i];
	tp->ta_name[0] = '\0';
	if (fl[i] == NULL)
		return(0);
	if (asciiinp) {
		if (fscanf(fl[i],
/*			"%hu\t%s\t%e %e %e %e %e %e %e %lu\t%hu\t%hu\t%hd", */
			"%ld\t%s\t%e %e %e %e %e %e %e %lu\t%hu\t%hu\t%hd",
			&tp->ta_uid,
			tp->ta_name,
			&tp->ta_cpu[0], &tp->ta_cpu[1],
			&tp->ta_kcore[0], &tp->ta_kcore[1],
			&tp->ta_con[0], &tp->ta_con[1],
			&tp->ta_du,
			&tp->ta_pc,
			&tp->ta_sc,
			&tp->ta_dc,
			&tp->ta_fee) != EOF)
			return(1);
	} else {
		if (fread(tp, sizeof(*tp), 1, fl[i]) == 1)
			return(1);
	}
	fl[i] = NULL;
	return(0);
}

/*char fmt[] = "%hu\t%.9s\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%lu\t%hu\t%hu\t%hd\n";
 *char fmtv[] = "%hu\t%.9s\t%e %e %e %e %e %e %e %lu %hu\t%hu\t%hd\n";
 */
char fmt[] = "%ld\t%.8s\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%lu\t%hu\t%hu\t%hd\n";
char fmtv[] = "%ld\t%.8s\t%e %e %e %e %e %e %e %lu %hu\t%hu\t%hd\n";

prtacct(tp)
register struct tacct *tp;
{

	printf(verbose ? fmtv : fmt,
		tp->ta_uid,
		tp->ta_name,
		tp->ta_cpu[0], tp->ta_cpu[1],
		tp->ta_kcore[0], tp->ta_kcore[1],
		tp->ta_con[0], tp->ta_con[1],
		tp->ta_du,
		tp->ta_pc,
		tp->ta_sc,
		tp->ta_dc,
		tp->ta_fee);
}
