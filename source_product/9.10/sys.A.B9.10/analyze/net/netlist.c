/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/netlist.c,v $
 * $Revision: 1.11.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:28:21 $
 *
 */
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

#include <string.h>
#include <stdio.h>

#include "../standard/inc.h" 

#ifdef BUILDFROMH 
#include <h/clist.h>
#else
#include <sys/clist.h>
#endif BUILDFROMH

/*Included for the list token definitions*/
#include "../objects/y.tab.h"

#include "net.h"
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"

#define MAXRES 80

/*Global variables - for this file only*/
int pausef,countf;

/*********************************************************
 *Command : listdisplay                                  *
 *********************************************************/ 

/*Display a dump of the nodes in a list structure*/
listdisplay(type,space,addr,length,ctrinfo,offset,redir,path)
int type,space,*addr,length,redir;
char *path,ctrinfo,offset;
{
   int  *nextp;
   int  more,count;

   /*init flags*/
   pausef = 0;
   countf = 0;
   more = 1; 
   
   if (redir) {
      if ((outf = fopen(path,((redir == 2)?"a+":"w+"))) == NULL) {
         fprintf(stderr,"Can't open file %s,errno = %d\n",path,errno);
         fclose(outf);
         return;
      }
   }

   checkoffset(&offset);
   setoptf(type,ctrinfo);
   
   count = 0;
   nextp = addr;
   while ((nextp != 0) && more) { 
       if (ltor(space,nextp) != 0) {
          if (!countf) {
             fprintf(outf,"\n"); 
             xdumpmem(space,nextp,length,redir,path);
             checkmore(&more,pausef); 
          }
          netvread(nextp + offset, &nextp, 4);
          ++count;
       }
       else {
          fprintf(outf,"bad pointer \n");
          return;
       }
   }
   if (countf) {
      fprintf(outf,"\n");
      fprintf(outf,"There are %d nodes starting at 0x%08x\n",count,addr);
   }
}

/*********************************************************
 *Supporting procedures for routine : listdisplay         *
 *********************************************************/ 

/*Does user want more data?*/
checkmore(more,pausef)
int *more,pausef;
{
    if (pausef) {
       fprintf(outf,"\n");
       fprintf(outf,"more(y/n) ");

       if (moredata() == 0)
           *more = 0;
    }
}

/*********************************************************/

/*Get user response for more data prompt*/
moredata()
{
     int character,index;
     char response[MAXRES];
    
     /*get user input*/ 
     for (index=0;index < MAXRES && (character = getchar()) != EOF                                               && character != '\n';index++)
         response[index] = character;
     response[index] = '\0';

     if (strcmp(response,"n") == 0)
        return(0);
     else /*more data*/
        return(1);
}

/*********************************************************/

/*set flags for listdisplay*/
setoptf(type,ctrinfo)
char ctrinfo;
int type;
{
   if (type == TLISTDUMP) {
      /*Is the pause option set?*/ 
      switch (ctrinfo) {
           case '\0':
           case 'p' :
              pausef = 1;
              break;
           case 'a' :
              pausef = 0;
              break;
           default:
              fprintf(outf,"bad option\n");
            return;
      }
   }
   if (type == TLISTCOUNT) {
      ctrinfo = "\0";
      countf = 1;
   }
}

/*********************************************************/

checkoffset(offset)
char *offset;
{
    if (*offset < 0) 
       *offset = 0;
}
