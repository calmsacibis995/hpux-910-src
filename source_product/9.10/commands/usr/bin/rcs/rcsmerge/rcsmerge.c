/*
 *  $Revision: 66.1 $
 *
 *                       rcsmerge operation
 */
/*****************************************************************************
 *                       join 2 revisions with respect to a third
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

extern int execstdout();	    /* execute a command & redirect stdout  */
extern char * bindex();
extern char * rindex();
extern int  cleanup();              /* cleanup after signals                */
extern void rcssigs();
extern char * mktempfile();         /*temporary file name generator         */
extern struct hshentry * genrevs(); /*generate delta numbers                */
extern int check_in;		       /*check-in operation flag (true/false)*/
extern int  nerror;                 /*counter for errors                    */

char *RCSfilename;
char *workfilename;
char * temp1file, * temp2file;

main (argc, argv)
int argc; char **argv;
{
        char *RCSfilename1, *RCSfilename2;
        char *workfilename1, *workfilename2;
        char * cmdusage;
        char command[NCPPN+revlength+40];
        char p_parm[NCPPN];
        char *co = CO;
        char *cmd_args[8];	 /* holds co & merge command arguments	    */
        int  revnums; /* counter for revision numbers given */
        int tostdout;
        int multRCSfile = false;
        int neutralize;
        char * rev1, * rev2; /*revision numbers*/
        char numericrev[revlength];   /* holds expanded revision number     */
        struct hshentry * gendeltas[hshsize];/*stores deltas to be generated*/
        struct hshentry * target;
        char *argv1[2];
        char *argv2[2];
        char *state;

        rcssigs();
        cmdid = "rcsmerge";
        check_in = false;
        cmdusage = "command format:\n    rcsmerge -p -rrev1 -rrev2 file\n    rcsmerge -p -rrev1 file";
        revnums=0;tostdout=false;neutralize=false;

        while (--argc,++argv, argc>=1 && ((*argv)[0] == '-')) {
                switch ((*argv)[1]) {
                default:
                        faterror2s("unknown option: %s\n%s", *argv,cmdusage);
                        break;

                case 'm':
                        multRCSfile = true;
                        break;
                case 'p':
                        tostdout=true;
                        goto revno;
                case 'z':
                        neutralize=true;
                        /* falls into -r */
                case 'r':
                revno:  if ((*argv)[2]!='\0') {
                                if (revnums==0) {
                                        rev1 = (char *) malloc(revlength);
                                        rev2 = (char *) malloc(revlength);
                                        strcpy(rev1, *argv+2); revnums=1;
                                } elif (revnums==1) {
                                        strcpy(rev2, *argv+2); revnums=2;
                                } else {
                                        faterror("too many revision numbers");
                                }
                        } /* do nothing for empty -r, -p or -z */
                        break;
                };
        } /* end of option processing */

        if (argc<1) faterror1s("No input file\n%s",cmdusage);
        if (revnums<1) faterror("no base revision number given");

        /* now handle all filenames */
        if (multRCSfile) {
                state = "b";
                while (argc>=1) {
                        switch (*state) {
                        case 'b':
                                if (isRCSfile(argv[0])) {
                                        RCSfilename1 = (char *) malloc(NCPPN);
                                        strcpy(RCSfilename1, argv[0]);
                                        state = "d";
                                } else {
                                        workfilename1 = (char *) malloc(NCPPN);
                                        strcpy(workfilename1, argv[0]);
                                        state = "c";
                                }
                                break;
                        case 'c':
                                if (isRCSfile(argv[0])) {
                                        RCSfilename1 = (char *) malloc(NCPPN);
                                        strcpy(RCSfilename1, argv[0]);
                                        state = "f";
                                } else
                                        faterror("Workfile already specified\n");
                                break;
                        case 'd':
                                if (isRCSfile(argv[0])) {
                                        RCSfilename2 = (char *) malloc(NCPPN);
                                        strcpy(RCSfilename2, argv[0]);
                                        state = "g";
                                } else {
                                        workfilename1 = (char *) malloc(NCPPN);
                                        strcpy(workfilename1, argv[0]);
                                        state = "e";
                                }
                                break;
                        case 'e':
                                if (isRCSfile(argv[0])) {
                                        RCSfilename2 = (char *) malloc(NCPPN);
                                        strcpy(RCSfilename2, argv[0]);
                                        state = "h";
                                } else
                                        faterror("Workfile already specified\n");
                                break;
                        case 'f':
                                if (isRCSfile(argv[0])) {
                                        RCSfilename2 = (char *) malloc(NCPPN);
                                        strcpy(RCSfilename2, argv[0]);
                                        state = "h";
                                } else
                                        faterror("Workfile already specified\n");
                                break;
                        case 'g':
                                if (isRCSfile(argv[0]))
                                        faterror("Two RCSfiles already specified\n");
                                else {
                                        workfilename1 = (char *) malloc(NCPPN);
                                        strcpy(workfilename1, argv[0]);
                                        state = "h";
                                }
                                break;
                        case 'h':
                                if (argc > 0)
                                        faterror("Too many arguments\n");
                                break;
                        default:
                                faterror("unknown state");
                        }
                        argc--; argv++;
                }
                argv1[0] = (char *) malloc(NCPPN);
                argv1[1] = (char *) malloc(NCPPN);
                strcpy(argv1[0], RCSfilename1);
                argc=1;
                if (workfilename1) {
                        strcpy(argv1[1], workfilename1);
                        argc=2;
                }
                if (pairfilenames(argc,argv1,true,false)!=1) goto end;
                else {
                        RCSfilename1 = RCSfilename;
                        if (workfilename1 && strcmp(workfilename1, workfilename) != 0)
                                workfilename = workfilename1;
                        diagnose1s("RCS file: %s",RCSfilename1);
                        if (!trysema(RCSfilename1,false)) goto end; /* give up */
                        gettree();  /* reads in the delta tree */
                        if (Head==nil) faterror("no revisions present");
                        if (!expandsym(rev1,numericrev)) goto end;
                        if (!(target=genrevs(numericrev,nil,nil,nil,gendeltas))) goto end;
                        strcpy(rev1, target->num);
                        argv2[0] = (char *) malloc(NCPPN);
                        argv2[1] = (char *) malloc(NCPPN);
                        strcpy(argv2[0], RCSfilename2);
                        argc=1;
                        if (workfilename1) {
                                strcpy(argv2[1], workfilename);
                                argc=2;
                        }
                        cleanup();
                        if (pairfilenames(argc,argv2,true,false)!=1) goto end;
                        else {
                                RCSfilename2= RCSfilename;
                                if (workfilename1 && strcmp(workfilename1, workfilename) != 0)
                                        workfilename = workfilename1;
                                diagnose1s("RCS file: %s",RCSfilename2);
                                if (!(access(workfilename,4)==0))
                                        faterror1s("Can't open %s",workfilename);
                                if (!trysema(RCSfilename2,false)) goto end; /* give up */
                                gettree();  /* reads in the delta tree */
                                if (Head==nil) faterror("no revisions present");
                        }
                        if (revnums==1) strcpy(rev2, Head->num); /* default for rev2 */
                        if (!expandsym(rev2,numericrev)) goto end;
                        if (!(target=genrevs(numericrev,nil,nil,nil,gendeltas))) goto end;
                        strcpy(rev2, target->num);
                }
        } else {
                /* now handle all filenames */

                if (pairfilenames(argc,argv,true,false)!=1) goto end;
                else {

                        if (argc>2 || (argc==2&&argv[1]!=nil))
                                warn("too many arguments");
                        RCSfilename1 = RCSfilename;
                        RCSfilename2 = RCSfilename;
                        diagnose1s("RCS file: %s",RCSfilename);
                        if (!(access(workfilename,4)==0))
                                faterror1s("Can't open %s",workfilename);

                        if (!trysema(RCSfilename,false)) goto end; /* give up */

                        gettree();  /* reads in the delta tree */

                        if (Head==nil) faterror("no revisions present");


                        if (!expandsym(rev1,numericrev)) goto end;
                        if (!(target=genrevs(numericrev,nil,nil,nil,gendeltas))) goto end;
                        strcpy(rev1, target->num);
                        if (revnums==1) strcpy(rev2, Head->num); /* default for rev2 */
                        if (!expandsym(rev2,numericrev)) goto end;
                        if (!(target=genrevs(numericrev,nil,nil,nil,gendeltas))) goto end;
                        strcpy(rev2, target->num);
                }
        }

        temp1file=mktempfile("/tmp/",TMPFILE1);
        temp2file=mktempfile("/tmp/",TMPFILE2);

        diagnose1s("retrieving revision %s",rev1);
        if ((cmd_args[0] = rindex(co, '/')) != NULL) cmd_args[0]++;
        else cmd_args[0] = co;
        if (neutralize) {
                cmd_args[1] = "-z";
                cmd_args[2] = "-q";
                cmd_args[3] = strcat(strcpy(p_parm, "-p"), rev1);
                cmd_args[4] = RCSfilename1;
                cmd_args[5] = NULL;
        } else {
                cmd_args[1] = "-q";
                cmd_args[2] = strcat(strcpy(p_parm, "-p"), rev1);
                cmd_args[3] = RCSfilename1;
                cmd_args[4] = NULL;
        }
        if (execstdout(co, cmd_args, temp1file)) {
                faterror("co failed");
        }
        diagnose1s("retrieving revision %s",rev2);
        if ((cmd_args[0] = rindex(co, '/')) != NULL)
                cmd_args[0]++;
        else cmd_args[0] = co;
        if (neutralize) {
                cmd_args[1] = "-z";
                cmd_args[2] = "-q";
                cmd_args[3] = strcat(strcpy(p_parm, "-p"), rev2);
                cmd_args[4] = RCSfilename2;
                cmd_args[5] = NULL;
        } else {
                cmd_args[1] = "-q";
                cmd_args[2] = strcat(strcpy(p_parm, "-p"), rev2);
                cmd_args[3] = RCSfilename2;
                cmd_args[4] = NULL;
        }
        if (execstdout(co, cmd_args, temp2file)) {
                faterror("co failed");
        }
        diagnose4s("Merging differences between %s and %s into %s%s",
            rev1, rev2, workfilename,
            tostdout?"; result to stdout":"");

        sprintf(command,"%s %s%s %s %s %s %s\n",MERGE,tostdout?"-p ":"",
                workfilename,temp1file,temp2file,workfilename,rev2);
        setruser();
        if (system(command)) {
                faterror("merge failed");
        }
        seteuser();

end:
        cleanup();
        exit(nerror!=0);

}

isRCSfile(fname)
char *fname;
/* Function: Given a filename fname, isRCSfile decides if the file is
 * an RCS file.
 */
{
        register char * lastsep, * match;

        if (*fname != nil) {
                /* bindex finds the beginning of the file name stem */
                match = bindex(fname,'/');
                lastsep = rindex( match, RCSSEP);
                if ( lastsep != 0 && *(lastsep+1) == RCSSUF &&
                        *(lastsep+2) == '\0' )
                                return(true);
        }
        return(false);
}
