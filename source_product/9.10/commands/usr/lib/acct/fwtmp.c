/* @(#) $Revision: 66.3 $ */     
/*
 *	fwtmp [-ic]
 *
 *	Fwtmp reads from the standard input and writes to the standard
 *	output, converting from records found in /etc/wtmp to formatted
 *	ASCII records. The ASCII version is useful to enable editing,
 *	via ed(1), bad records or general purpose maintenance of the
 *	file.
 *
 *	The argument -ic is used to denote that input is in ASCII form,
 *	and output is to be written in binary form. The arguments i and
 *	c are independent, respectively specifying ASCII input and
 *	binary output, thus -i is an ASCII to ASCII copy and -c is a
 *	binary to binary copy.
 *
 */

#define MAX_INET_SIZE	15

#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "acctdef.h"

main (argc, argv)
int argc;
char *argv[];
{

    struct utmp Ut;
    int iflg = 0;
    int cflg = 0;

    while (--argc > 0)
    {
	if (**++argv == '-')
	    while (*++*argv)
		switch (**argv)
		{
		case 'c': 
		    cflg++;
		    continue;
		case 'i': 
		    iflg++;
		    continue;
		}
	break;
    }

    for (;;)
    {
	if (iflg)
	{
	    if (inp(stdin, &Ut) == -1)
		break;
	}
	else
	{
	    if (fread(&Ut, sizeof Ut, 1, stdin) != 1)
		break;
	}

	if (cflg)
	    fwrite(&Ut, sizeof Ut, 1, stdout);
	else
	{
            char time_buf[30];
            char inet_addr[16];
            int cnt;

	    strcpy(time_buf, ctime(&Ut.ut_time) + 4);
	    time_buf[20] = '\0';

	    if (Ut.ut_addr)
            {
	        /*
	         * Get the internet address in '.' notation
	         */
	        if (Ut.ut_addr)
		    strcpy(inet_addr, inet_ntoa(Ut.ut_addr));
	        else
	            inet_addr[0] = '\0';

		if (*Ut.ut_host)
		{
	        printf(
	     "%-8.8s %-4.4s %-12.12s %5ld %2hd %4.4ho %4.4ho %lu %s %s %.*s\n",
		    Ut.ut_name, Ut.ut_id, Ut.ut_line,
		    Ut.ut_pid, Ut.ut_type,
		    Ut.ut_exit.e_termination, Ut.ut_exit.e_exit,
		    Ut.ut_time, time_buf , inet_addr, 
		    sizeof(Ut.ut_host), Ut.ut_host);
		}
		else
		{
	        printf(
	     "%-8.8s %-4.4s %-12.12s %5ld %2hd %4.4ho %4.4ho %lu %s %s\n",
		    Ut.ut_name, Ut.ut_id, Ut.ut_line,
		    Ut.ut_pid, Ut.ut_type,
		    Ut.ut_exit.e_termination, Ut.ut_exit.e_exit,
		    Ut.ut_time, time_buf , inet_addr);
		}
	    }
            else
	        printf(
	     "%-8.8s %-4.4s %-12.12s %5ld %2hd %4.4ho %4.4ho %lu %s\n",
		    Ut.ut_name, Ut.ut_id, Ut.ut_line,
		    Ut.ut_pid, Ut.ut_type,
		    Ut.ut_exit.e_termination, Ut.ut_exit.e_exit,
		    Ut.ut_time, time_buf);
	}
    }
    exit(0);
}

#define LINE_MAX   1024

int
inp(file, u)
FILE *file;
register struct utmp *u;
{
    char buf[LINE_MAX];
    char tmp_buf[MAX_INET_SIZE];
    register char *p;
    register int i;

    if (fgets((p = buf), LINE_MAX, file) == NULL)
	return -1;
    
    /*
     * Zero out *u.
     */
    memset((char *)u, 0, sizeof(struct utmp));

    /*
     * Process ut_name
     */
    for (i = 0; i < NSZ; i++)
	u->ut_name[i] = *p++;

    for (i = NSZ - 1; i >= 0; i--)
    {
	if (u->ut_name[i] == ' ')
	    u->ut_name[i] = '\0';
	else
	    break;
    }
    p++;

    /*
     * Process ut_id
     */
    for (i = 0; i < 4; i++)
	if ((u->ut_id[i] = *p++) == ' ')
	    u->ut_id[i] = '\0';
    p++;

    /*
     * Process ut_line
     */
    for (i = 0; i < LSZ; i++)	/* Allow a space in line field */
	u->ut_line[i] = *p++;

    for (i = LSZ - 1; i >= 0; i--)
    {
	if (u->ut_line[i] == ' ')
	    u->ut_line[i] = '\0';
	else
	    break;
    }

    /*
     * Now get the ut_pid, ut_type, ut_exit and ut_time fields.  Ignore 
     * the string representation of the date/time.
     */
    (void) sscanf(p, "%ld %hd %ho %ho %ld",
	    &u->ut_pid,
	    &u->ut_type,
	    &u->ut_exit.e_termination,
	    &u->ut_exit.e_exit,
	    &u->ut_time);

    /*
     * How many ASCII characters did it take to represent the ut_time value?
     */
    sprintf(tmp_buf, "%ld", u->ut_time);
    p = p + 20 + strlen(tmp_buf);

    /*
     * Skip past string representation of date/time.
     */
    p += 21;

    /*
     * Check to see if the addr field exists.
     */
    if (*p == '\n')
        return 0;

    /*
     * Skip past space following date/time.
     */
    p++; 

    /*
     * Process get the ut_addr field.
     */
    for (i = 0; (*p != ' ') && (*p != '\n') && i < MAX_INET_SIZE; i ++)
    {
	tmp_buf[i] = *p++;
    }

    tmp_buf[i] = '\0';
    u->ut_addr = inet_addr(tmp_buf);

    if (*p == '\n')
    {
	u->ut_host[0] = '\0';
	return 0;
    }

    /*
     * Skip past space following internet address.
     */
    while (*p != ' ')
    {
	if (*p == '\n')
	{
	    u->ut_host[0] = '\0';
	    return 0;
	}
        p++;
    }
    p++;

    /*
     * Process ut_host
     */
    for (i = 0; (*p != '\n') && (i < sizeof(u->ut_host)); i++)
	u->ut_host[i] = *p++;

    for (i = (sizeof(u->ut_host) - 1); i >= 0; i--)
    {
	if (u->ut_host[i] == ' ')
	    u->ut_host[i] = '\0';
	else
	    break;
    }

    return 0;
}
