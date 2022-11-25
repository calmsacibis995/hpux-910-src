/*
 *  $Revision: 66.2 $
 *
 *                     RCS rcsdiff operation
 */
/*****************************************************************************
 *                       generate difference between RCS revisions
 *****************************************************************************
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
#include <string.h>
#include "system.h"
#include "bin.h"
#include "rcsbase.h"
#include "copyright.h"

extern int    cleanup();            /* cleanup after signals                */
extern char * mktempfile();         /*temporary file name generator         */
extern struct hshentry * genrevs(); /*generate delta numbers                */
extern int execstdout();	    /* execute a command & redirect stdout  */
extern int execonly();		    /* execute a command (no I/O redirect)  */
extern char * rindex();
extern void rcssigs();
extern int check_in;		       /*check-in operation flag (true/false)*/
extern int    nerror;               /*counter for errors                    */

char *RCSfilename;
char *workfilename;
char * temp1file, * temp2file;
char *co = CO;
char *rdiff = RDIFF;
char *cmd_args[8];	 /* holds co & merge command arguments	    */
char p_parm[NCPPN];

main (argc, argv)
int argc; char **argv;
{
        char * cmdusage;
        int  revnums;                 /* counter for revision numbers given */
        int neutralize;
        char * rev1, * rev2;          /* revision numbers from command line */
        char * xrev1, * xrev2;        /* expanded revision numbers */
        char numericrev[revlength];   /* holds expanded revision number     */
        struct hshentry * gendeltas[hshsize];/*stores deltas to be generated*/
        struct hshentry * target;
        char * boption, * otheroption;
        int multRCSfile = false;
        int  exit_stats;
	char **ca;		      /* cmd_args pointer for rdiff options */
        int  filecounter;
        char *RCSfilename1, *RCSfilename2;
        int argc1, argc2;
        char *workfilename1;
        char *argv1[2];
        char *argv2[2];

        rcssigs();
        boption=otheroption="";
        cmdid = "rcsdiff";
	check_in = false;
        cmdusage = "command format:\n    rcsdiff [-b] [-cefhn] [-rrev1] [-rrev2] file";
        filecounter=revnums=0; neutralize=false;
        rev1 = (char *) malloc(revlength);
        rev2 = (char *) malloc(revlength);
        xrev1 = (char *) malloc(revlength);
        xrev2 = (char *) malloc(revlength);
        while (--argc,++argv, argc>=1 && ((*argv)[0] == '-')) {
                switch ((*argv)[1]) {
                case 'z':
                        neutralize=true;
                        /* falls into -r */
                case 'r':
                        if ((*argv)[2]!='\0') {
                                if (revnums==0) {
                                        strcpy(rev1, *argv+2); revnums=1;
                                } elif (revnums==1) {
                                        strcpy(rev2, *argv+2); revnums=2;
                                } else {
                                        faterror("too many revision numbers");
                                }
                        } /* do nothing for empty -r */
                        break;
                case 'b':
                        boption="-b";
                        break;
                case 'c':
                case 'e':
                case 'f':
                case 'h':
                case 'n':
                        if (*otheroption=='\0') {
                                otheroption=(*argv);
                        } else {
                                faterror("Options c,e,f,h,n are mutually exclusive");
                        }
                        break;
                case 'm':
                        multRCSfile = true;
                        break;
                default:
                        faterror2s("unknown option: %s\n%s", *argv,cmdusage);
                };
        } /* end of option processing */

        if (argc<1) faterror1s("No input file\n%s",cmdusage);
        if (multRCSfile && argc<2) faterror1s("Not enough input files for multiple trunk option\n%s",cmdusage);

        /* now handle all filenames */
        do {
                if (multRCSfile) {
                        argv1[0] = (char *) malloc(NCPPN);
                        strcpy(argv1[0], argv[0]);
                        argc1=1;
                        if (pairfilenames(argc1,argv1,true,false)!=1) {
				argc--; argv++; continue; /* give up */
			}
                        RCSfilename1 = RCSfilename;
                        workfilename1 = workfilename;
                        if (++filecounter>1)
                                diagnose("===================================================================");
                        diagnose1s("RCS file: %s",RCSfilename);
                        if (!trysema(RCSfilename,false)) {
				argc--; argv++; continue; /* give up */
                        }
                        gettree(); /* reads in the delta tree */
                        if (Head==nil) {
                                error("no revisions present");
				argc--; argv++; continue; /* give up */
                        }
                        if (revnums==0)
                                strcpy(rev1, Head->num); /* default rev1 */
                        if (!expandsym(rev1,numericrev)) {
				argc--; argv++; continue; /* give up */
                        }
                        if (!(target=genrevs(numericrev,nil,nil,nil,gendeltas))){
				argc--; argv++; continue; /* give up */
                        }
                        strcpy(xrev1, target->num);
                        argc--;
                        argv++;
                        argv2[0] = (char *) malloc(NCPPN);
                        strcpy(argv2[0], argv[0]);
                        argc2=1;
                        cleanup();
                        if (pairfilenames(argc2,argv2,true,false)!=1) continue;
                        RCSfilename2 = RCSfilename;
                        diagnose1s("RCS file: %s",RCSfilename2);
                        if (!trysema(RCSfilename,false)) continue; /* give up */
                        gettree(); /* reads in the delta tree */
                        if (Head==nil) {
                                error("no revisions present");
                                continue;
                        }
                        if (revnums==1)
                                strcpy(rev2, Head->num); /* default rev2 */
                        if (!expandsym(rev2,numericrev)) continue;
                        if (!(target=genrevs(numericrev,nil,nil,nil,gendeltas))) continue;
                        strcpy(xrev2, target->num);
                }

                else {
                        if (pairfilenames(argc,argv,true,false)!=1) continue;
                        RCSfilename1 = RCSfilename2 = RCSfilename;
                        if (++filecounter>1)
                                diagnose("===================================================================");
                        diagnose1s("RCS file: %s",RCSfilename);
		        if (revnums<2 && !(access(workfilename,4)==0)) {
                                error1s("Can't open %s",workfilename);
                                continue;
                        }
                        if (!trysema(RCSfilename,false)) continue; /* give up */
                        gettree(); /* reads in the delta tree */
                        if (Head==nil) {
                                error("no revisions present");
                                continue;
                        }
                        if (revnums==0)
                                strcpy(rev1, Head->num); /* default rev1 */
                        if (!expandsym(rev1,numericrev)) continue;
                        if (!(target=genrevs(numericrev,nil,nil,nil,gendeltas))) continue;
                        strcpy(xrev1, target->num);
        
                        if (revnums==2) {
                                if (!expandsym(rev2,numericrev)) continue;
                                if (!(target=genrevs(numericrev,nil,nil,nil,gendeltas))) continue;
                                strcpy(xrev2, target->num);
                        }
                }



                temp1file=mktempfile("/tmp/",TMPFILE1);
                diagnose1s("retrieving revision %s",xrev1);
		if ((cmd_args[0] = rindex(co, '/')) != NULL) cmd_args[0]++;
		else cmd_args[0] = co;
		if (neutralize) {
		    cmd_args[1] = "-z";
		    cmd_args[2] = "-q";
		    cmd_args[3] = strcat(strcpy(p_parm, "-p"), xrev1);
		    cmd_args[4] = RCSfilename1;
		    cmd_args[5] = NULL;
		}
		else {
		    cmd_args[1] = "-q";
		    cmd_args[2] = strcat(strcpy(p_parm, "-p"), xrev1);
		    cmd_args[3] = RCSfilename1;
		    cmd_args[4] = NULL;
		}
		if (execstdout(co, cmd_args, temp1file)) {
                        error("co failed");
                        continue;
                }
                if (revnums<=1) {
                        temp2file=workfilename;
                        diagnose4s("rdiff %s%s -r%s %s",boption,otheroption,xrev1,workfilename);
                } else {
                        temp2file=mktempfile("/tmp/",TMPFILE2);
                        diagnose1s("retrieving revision %s",xrev2);
			if ((cmd_args[0] = rindex(co, '/')) != NULL)
				cmd_args[0]++;
			else cmd_args[0] = co;
			if (neutralize) {
			    cmd_args[1] = "-z";
			    cmd_args[2] = "-q";
			    cmd_args[3] = strcat(strcpy(p_parm, "-p"), xrev2);
			    cmd_args[4] = RCSfilename2;
			    cmd_args[5] = NULL;
			}
			else {
			    cmd_args[1] = "-q";
			    cmd_args[2] = strcat(strcpy(p_parm, "-p"), xrev2);
			    cmd_args[3] = RCSfilename2;
			    cmd_args[4] = NULL;
			}
			if (execstdout(co, cmd_args, temp2file)) {
                                error("co failed");
                                continue;
                        }
                        diagnose4s("rdiff %s%s -r%s -r%s",boption,otheroption,xrev1,xrev2);
                }
		ca = cmd_args;
		if ((*ca = rindex(rdiff, '/')) != NULL) (*ca)++;
		else *ca = rdiff;
		ca++;
		if (*boption != '\0') *(ca++) = boption;
		if (*otheroption != '\0') *(ca++) = otheroption;
		*(ca++) = temp1file;
		*(ca++) = temp2file;
		*(ca++) = NULL;
		setruser();
                exit_stats = execonly(rdiff, cmd_args);
		seteuser();
                if (exit_stats < 0 || exit_stats > 1) {
                        error ("rdiff failed");
                        continue;
                }
        } while (cleanup(),
                 ++argv, --argc >=1);


        exit(nerror!=0);

}
