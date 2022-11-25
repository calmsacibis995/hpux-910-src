
/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

#ifdef BOBCAT


#include "/usr/include/stdio.h"
#include "/usr/include/errno.h"
#include "ddbCons.h"
#include "ddbStd.h"
/*
 * Remember the default location for the include files has been
 * redirected by the makefile.
 */
#include "/usr/include/unistd.h"

# define GPIO

IMPORT   char   *myname;
IMPORT   int   verboseErrors;

/*
 *****************************************************************************
 *
 *  Local Globals - the following variables and data structures
 *  MUST now appear in any other compilation unit.
 *
 *****************************************************************************
 */
 int commfd=0;        /* comm link fd */
 char *commFileSave;  /* save ptr to comm file name */
 char errBuf[100];
 int errnoSave;

int cur_address = 0;
int verboseErrors = 0;
char *myname = "noname";

/*
 *-------------------------------------------------------------
 *  get_comm_link - Establish communication link.
 *
 *  This process establishes communication with the target system.
 *  It opens the file passed in, lock the file to this copy of ddb,
 *  saves the file descriptor in the global commfd, configures the 
 *  I/O card, and verifies the target system is ready to receive 
 *  commands.
 *
 *  CALL SEQUENCE: get_comm_link(&cfd,commlink)
 *          where: commlink = name of file to open
 *
 *  RETURN VALUES: return(SUCCESS/FAIL)
 *                 cfd = file descriptor return by the open request
 *
 *  GLOBALS: commfd
 *
 *  get_comm_link is called from:  init_target (ddbHi.c),
 *                                 syncWithTarget (ddbHi.c)
 *
 *  get_comm_link calls:  init_gpio (gpiotalker.c)
 *                        check_ready (gpiotalker.c)
 *
 *  system routine(s) used: fprintf, lockf, open, perror
 *
 *-------------------------------------------------------------
 */

int get_comm_link(cfd,commlink)
int *cfd;
char *commlink;

{
    int arg[SPEC_IOCTL_ARGS];
    int results;

    /*
     * This kludge is because of a bug in the 300 kernel.
     * The kernel is counting the number and size of transfers
     * to the file.  This is ok, for real disc file and a bug
     * for device files.  We must close the file and the reopen
     * it to reset the counter.
     */
    if (commlink != '\0') 
      commFileSave = commlink;
    else
    {
      if (close(commfd)==-1)
      {
	  fprintf(stderr,"Unable to close file for reopening\n");
          return(FAIL);
      }
    }

    if ((results=open(commFileSave, O_RDWR)) == FAIL)
    {
       errnoSave=errno;
       sprintf(errBuf,"%s",myname);
       fprintf(stderr,"Unable to open communication link to target system\n");
       return(FAIL);
    }
    *cfd = results;
    commfd = *cfd;

    if (init_gpio (*cfd) == FAIL)
    {
        if (verboseErrors == YES)
           fprintf(stderr, "init_gpio failed\n");
        return(FAIL);
    }

    if (check_ready(*cfd) == FAIL)
    {
        if (verboseErrors == YES)
           fprintf(stderr, "check_ready(%d) failed\n");
        return(FAIL);
    }

    return(SUCCESS);
}

#endif
