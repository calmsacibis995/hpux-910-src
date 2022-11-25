/* @(#) $Revision: 64.1 $ */     
/*
 *	acctcon2
 *
 *	Acctcon2 expects as input a sequence of login session records
 *	and converts them into total accounting records (see tacct
 *	format in acct(5)).
 *
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include "ctmp.h"
#include "tacct.h"
char	*ctime();

struct	ctmp	cb;
struct	tacct	tb;

main(argc, argv)
char **argv;
{
	tb.ta_sc = 1;
/*	while (scanf("%hd\t%hu\t%s\t%lu\t%lu\t%lu\t%*[^\n]", */
	while (scanf("%hd\t%ld\t%s\t%lu\t%lu\t%lu\t%*[^\n]",
		&cb.ct_tty,
		&cb.ct_uid,
		cb.ct_name,
		&cb.ct_con[0],
		&cb.ct_con[1],
		&cb.ct_start) != EOF) {

		tb.ta_uid = cb.ct_uid;
		CPYN(tb.ta_name, cb.ct_name);
		tb.ta_con[0] = MINS(cb.ct_con[0]);
		tb.ta_con[1] = MINS(cb.ct_con[1]);
		fwrite(&tb, sizeof(tb), 1, stdout);
	}
}
