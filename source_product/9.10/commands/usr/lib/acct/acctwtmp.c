/* @(#) $Revision: 27.3 $ */      
/*
 *	acctwtmp "reason"
 *
 *	Acctwtmp writes a utmp(5) record to its standard output. The 
 *	record contains the current time and a string of characters that
 *	describe the reason. A record type of ACCOUNTING is assigned 
 *	(see utmp(5)). Reason must be a string of 11 or less characters,
 *	numbers, $, or spaces. For example, the following are
 *	suggestions for use in reboot and shutdown procedures,
 *	respectively:
 *
 *		acctwtmp `uname` >> /etc/wtmp
 *		acctwtmp "file save" >> /etc/wtmp
 *
 */

#include <stdio.h>
#include "acctdef.h"
#include <sys/types.h>
#include <utmp.h>

struct	utmp	wb;

main(argc, argv)
char **argv;
{
	if(argc < 2) {
		fprintf(stderr, "Usage: %s \"reason\" [ >> %s ]\n",
			argv[0], WTMP_FILE);
		exit(1);
	}

	strncpy(wb.ut_line, argv[1], LSZ);
	wb.ut_line[11] = NULL;
	wb.ut_type = ACCOUNTING;
	time(&wb.ut_time);
	fseek(stdout, 0L, 2);
	fwrite(&wb, sizeof(wb), 1, stdout);
}
