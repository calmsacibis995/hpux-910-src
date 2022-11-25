/*
 *  $Revision: 66.1 $
 *
 *                     Logging of RCS commands co and ci
 */
/*******************************************************************
 * This program appends argv[1] to the file SNOOPFILE.
 * To avoid overlaps, it creates a lockfile with name lock in the same
 * directory as SNOOPFILE. SNOOPFILE must be defined in the cc command. 
 * Prints an error message if lockfile doesn't get deleted after
 * MAXTRIES tries.
 *******************************************************************
 *
 * Copyright (C) 1982 by Walter F. Tichy
 *                       Purdue University
 *                       Computer Science Department
 *                       West Lafayette, IN 47907
 *
 * All rights reserved. No part of this software may be sold or distributed
 * in any form or by any means without the prior written permission of the 
 * author.
 * Report problems and direct all inquiries to Tichy@purdue (ARPA net).
 */

#include <stdio.h>
#include "system.h"
#include "bin.h"
#include "rcsbase.h"

#define fflsbuf _flsbuf
/* undo redefinition of putc in rcsbase.h */

extern unsigned sleep();
extern char * strcpy();

char  lockfname[NCPPN];
FILE * logfile;
int lockfile;

#define MAXTRIES 20

main(argc,argv)
int argc; char * argv[];
/* writes argv[1] to SNOOPFILE and appends a newline. Invoked as follows:
 * rcslog logmessage
 */
{       int tries;
        register char * lastslash, *sp;

        strcpy(lockfname,SNOOPFILE);
        lastslash = sp = lockfname;
        while (*sp) if (*sp++ =='/') lastslash=sp; /* points beyond / */
        strcpy(lastslash,",lockfile");
        tries=0;
        while (((lockfile=creat(lockfname, 000)) == -1) && (tries<=MAXTRIES)) {
                tries++;
                sleep(5);
        }
        if (tries<=MAXTRIES) {
                close(lockfile);
                if ((logfile=fopen(SNOOPFILE,"a")) ==NULL) {
                        fprintf(stderr,"Can't open logfile %s\n",SNOOPFILE);
                } else {
                        fputs(argv[1],logfile);
                        putc('\n',logfile);
                        fclose(logfile);
                }
                unlink(lockfname);
        } else {
                fprintf(stderr,"RCS logfile %s seems permanently locked.\n",SNOOPFILE);
                fprintf(stderr,"Please alert system administrator\n");
        }
}
