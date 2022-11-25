/*
 *                     RCS revision number handling
 *
 *  $Header: rcsrev.c,v 66.1 90/11/14 14:46:47 ssa Exp $ Purdue CS
 */
 
/*********************************************************************************
 *********************************************************************************
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

/*
#define REVTEST
/* version REVTEST is for testing the routines that generate a sequence
 * of delta numbers needed to regenerate a given delta.
 */

#include <stdio.h>
#include "rcsbase.h"

extern char * strcpy();
extern FILE * finptr;   /* RCS input file */
extern char * getid();
extern struct hshentry * getnum();
extern int    getkey();
extern int    getlex();

extern char * getkeyval();
extern int delta(), deltatext();
struct hshentry * genbranch(); /* forward */



int countnumflds(s)
char * s;
/* Given a pointer s to a dotted number (date or revision number),
 * countnumflds returns the number of digitfields in s.
 */
{       register char * sp;
        register int    count;
        if ((sp=s)==nil) return(0);
        if (*sp == '\0') return(0);
        count = 1;
        while (*sp) {
                if (*sp++ == '.') count++;
        }
        if (*(--sp) == '.') count--; /*trailing periods don't count*/
        return(count);
}

getbranchno(revno,branchno)
char * revno, * branchno;
/* Given a non-nil revision number revno, getbranchno copies the number of the branch
 * on which revno is into branchnumber. If revno itself is a branch number,
 * it is copied unchanged.
 */
{
        register int i, numflds;
        register char * tp, * sp;

        numflds=countnumflds(revno);
        if (numflds%2 == 1)
                strcpy(branchno,revno);
        else {
                sp=revno; tp=branchno;
                for (i=1;i<numflds;i++) {
                        while(*sp!='.') *tp++ = *sp++;
                        *tp++ = *sp++;
                }
                *(tp-1)='\0';
        }
}



int cmpnum(num1, num2)
char * num1, * num2;
/* compares the two dotted numbers num1 and num2 lexicographically
 * by field. Individual fields are compared numerically.
 * returns <0, 0, >0 if num1<num2, num1==num2, and num1>num2, resp.
 * omitted fields are assumed to be higher than the existing ones.
*/
{
        register char * s1, *s2;
        register int n1, n2;

        s1=num1==nil?"":num1;
        s2=num2==nil?"":num2;

        do {
                n1 = 0;
                while (('0' <= *s1) && (*s1 <= '9')) {
                        n1 = n1*10 + (*s1 - '0');
                        s1++;
                }
                /* skip '.' */
                if (*s1=='.') s1++;

                n2 = 0;
                while (('0' <= *s2) && (*s2 <= '9')) {
                        n2 = n2*10 + (*s2 - '0');
                        s2++;
                }
                /* skip '.' */
                if (*s2=='.') s2++;

        } while ((n1==n2) && (*s1!='\0') && (*s2!='\0'));

        if (((*s1=='\0') && (*s2=='\0')) || (n1!=n2))
                return (n1 - n2);
        /*now n1==n2 and one of s1 or s2 is shorter*/
        /*give precedence to shorter one*/
        if (*s1=='\0') return 1;
        else           return -1;

}



int cmpnumfld(num1, num2, fld)
char * num1, * num2; int fld;
/* compares the two dotted numbers at field fld and returns
 * num1[fld]-num2[fld]. Assumes that num1 and num2 have at least fld fields.
*/
{
        register char * s1, *s2;
        register int n1, n2;

        s1=num1; n1=fld-1;
        /* skip fld-1 fields */
        while (n1) {
                while(*s1 != '.') s1++;
                n1--; s1++;
        }
        s2 = num2; n2=fld-1;
        while (n2) {
                while(*s2 != '.') s2++;
                n2--; s2++;
        }
        /* Don't put the above into a single loop! */
        /* Now s1 and s2 point to the beginning of the respective fields */
        /* compute numerical value and compare */
        n1 = 0;
        while (('0' <= *s1) && (*s1 <= '9')) {
                n1 = n1*10 + (*s1 - '0');
                s1++;
        }
        n2 = 0;
        while (('0' <= *s2) && (*s2 <= '9')) {
                n2 = n2*10 + (*s2 - '0');
                s2++;
        }
        return (n1 - n2);
}


int compartial(num1, num2, length)
char    * num1;
char    * num2;
int     length;

/*   compare the first "length" fields of two dot numbers;
     the omitted field is considered to be larger than any number  */
/*   restriction:  at least one number has length or more fields   */

{
        register        char    *s1, *s2;
        register        int     n1, n2;


        s1 = num1;      s2 = num2;
        if ( s1==nil || *s1 == '\0' ) return 1;
        if ( s2==nil || *s2 == '\0' ) return -1;

        do {
            n1 = 0;
            while( ('0' <= *s1) && (*s1 <= '9') ) {
                n1 = n1 * 10 + (*s1 - '0') ;
                s1++;
            }
            if ( *s1 == '.' ) s1++;    /*  skip .   */

            n2 = 0;
            while( ( '0' <= *s2) && ( *s2 <= '9' ) ) {
                   n2 = n2 * 10 + ( *s2 - '0' ) ;
                s2++;
            }
            if (*s2 == '.') s2++;
        }   while(  ( n1 == n2) && ((--length) != 0) &&
                    ( *s1 != '\0') && (*s2 != '\0')  );

        if ( (n1 != n2) || (length == 0) ){
                return(n1-n2);   }

        if ( *s1 == '\0' ) return 1;
        if ( *s2 == '\0' ) return -1;
	return 0;
}



incnum(onum,nnum)
char * onum, *nnum;
/* increments the last field of revision number onum by one and
 * places the result into nnum
 */
{
        register char * sp, *tp;
        register int i;

        sp = onum; tp = nnum;
        for (i=countnumflds(onum)-1; i>0; i--) {
                while (*sp != '.') *tp++ = *sp++;
                *tp++ = *sp++;  /* copy dot also */
        }
        sprintf(tp,"%d",atoi(sp)+1);
}


char * partialno(rev1,rev2,length)
char * rev1, * rev2; register int length;
/* Function: Copies length fields of revision number rev2 into rev1.
 * returns rev1.
 */
{       register char * r1,* r2;

        r1=rev1; r2=rev2;
        while (length) {
                while(*r2 != '.' && *r2!='\0') *r1++ = *r2++;
                *r1++ = *r2++;
                length--;
        }
        /* eliminate last '.'*/
        *(r1-1)='\0';
        return rev1;
}



char * getancestor(r1, r2, r3)
char * r1, *r2, *r3;
/* function: finds the common ancestor of r1 and r2 and
 * places it into r3.
 * returns r3 if successful, false otherwise.
 * works reliably only if r1 and r2 are not branch numbers.
 */
{       int l1, l2, l3;
        char t1[revlength], t2[revlength];

        l1=countnumflds(r1); l2=countnumflds(r2);
        if ((l1<=2 && l2<=2)||(cmpnum(r1,r2)==0)) {
                /* on main trunk or identical */
                error2s("Common ancestor of %s and %s undefined.", r1, r2);
                return false;
        }

        l3=0;
        while ((cmpnumfld(r1, r2, l3+1)==0) && (cmpnumfld(r1, r2, l3+2)==0)){
                l3=l3+2;
        }
        /* This will terminate since r1 and r2 are not the same; see above*/
        if (l3==0) {
                /* no common prefix. Common ancestor on main trunk. */
                partialno(t1,r1,l1>2?2:l1);partialno(t2,r2,l2>2?2:l2);
                if (cmpnum(t1,t2)<0)
                        strcpy(r3,t1);
                else    strcpy(r3,t2);
                if ((cmpnum(r3,r1)==0)||(cmpnum(r3,r2)==0)) {
                        error2s("Ancestor for %s and %s undefined.",r1,r2);
                        return false;
                }
                return r3;
        } else {
               if (cmpnumfld(r1,r2,l3+1)==0) {
                        error2s("Ancestor for %s and %s undefined.",r1,r2);
                        return false;
                }
                return(partialno(r3,r1,l3));
        }
}




struct hshentry * genrevs(revno,date,author,state,store)
char * revno, * date, * author, * state;
struct hshentry * * store;
/* Function: finds the deltas needed for reconstructing the
 * revision given by revno, date, author, and state, and stores pointers
 * to these deltas into an array whose starting address is given by store.
 * The last pointer stored is nil. The last delta (target delta) is returned.
 * If the proper delta could not be found, nil is returned.
 */
{
        int length, Hnumlen;
        register struct hshentry * next;
        int result;
        char * branchnum;
        char t[revlength];

        if (Head == nil) {
                error("RCSfile empty.");
                return nil;
        }

		Hnumlen = countnumflds(Head->num);
        length = countnumflds(revno);
        next=Head;

	if (length>0 && length<Hnumlen-1) {
	  error1s("Branch number %s is not applicable", revno);
	  return nil;
	}
	if (length >= Hnumlen-1) {
                /* at least one field; find branch exactly */
                while ((next!=nil) &&
                       ((result=compartial(revno,next->num,Hnumlen-1))<0)) {
                        /*puts(next->num);*/
                        *store++ = next;
                        next = next->next;
                }

		  if (next==nil) {
			error1s("Branch number %s too low.",partialno(t,revno,Hnumlen-1));
			return nil;
		  }
		  if ( result>0 && length>Hnumlen-1) {
			error1s("Branch number %s not present.", partialno( t, revno, Hnumlen-1));
			return nil;
		  }
        }
        if (length<=Hnumlen-1){
                /* pick latest one on given branch */
                branchnum = next->num; /* works even for empty revno*/
                while ((next!=nil) &&
                       (compartial(branchnum,next->num,Hnumlen-1)==0) &&
                       !(
                        (date==nil?1:(cmpnum(date,next->date)>=0)) &&
                        (author==nil?1:(strcmp(author,next->author)==0)) &&
                        (state ==nil?1:(strcmp(state, next->state) ==0))
                        )
                       )
                {       /*puts(next->num);*/
                        *store ++ = next;
                        next=next->next;
                }
                if ((next==nil) ||
                    (compartial(branchnum,next->num,Hnumlen-1)!=0))/*overshot*/ {
                        error4s("Cannot find revision on branch %s with a date before %s, author %s, and state %s.",
                                length==0?partialno(t,branchnum,Hnumlen-1):revno,date==nil?"<now>":date,
                                author==nil?"<any>":author, state==nil?"<any>":state);
                        return nil;
                } else {
                        /*puts(next->num);*/
                        *store++ = next;
                }
                *store = nil;
                return next;
        }

        /* length >=Hnumlen */
        /* find revision; may go low if length==2*/
        while ((next!=nil) &&
               ((result =cmpnumfld(revno,next->num,Hnumlen)) <0) &&
               (compartial(revno,next->num,1)==0) ) {
                /*puts(next->num);*/
                *store++ = next;
                next = next->next;
        }

        if ((next==nil) || (compartial(revno,next->num,Hnumlen-1)!=0)) {
                error1s("Revision number %s too low.",partialno(t,revno,Hnumlen));
                return nil;
        }
        if ((length>Hnumlen) && (result!=0)) {
                error1s("Revision %s not present.",partialno(t,revno,Hnumlen));
                return nil;
        }

        /* print last one */
        /*puts(next->num);*/
        *store++ = next;

        if (length > Hnumlen)
                return genbranch(next,revno,length,date,author,state,store);
        else { /* length == trunk field lenght*/
                if ((date!=nil) && (cmpnum(date,next->date)<0)){
                        error2s("Revision %s has date %s.",next->num, next->date);
                        return nil;
                }
                if ((author!=nil)&&(strcmp(author,next->author)!=0)) {
                        error2s("Revision %s has author %s.",next->num,next->author);
                        return nil;
                }
                if ((state!=nil)&&(strcmp(state,next->state)!=0)) {
                        error2s("Revision %s has state %s.",next->num,
                               next->state==nil?"<empty>":next->state);
                        return nil;
                }
                *store=nil;
                return next;
        }
}




struct hshentry * genbranch(bpoint, revno, length,date,author,state,store)
struct hshentry * bpoint;
char * revno; int length;
char * date, * author, * state;
struct hshentry ** store;
/* Function: given a branchpoint, a revision number, date, author, and state,
 * genbranch finds the deltas necessary to reconstruct the given revision
 * from the branch point on.
 * Pointers to the found deltas are stored in an array beginning with store.
 * revno must be on a side branch.
 * return nil on error
 */
{
        int field;
        register struct hshentry * next, * trail;
        register struct branchhead * bhead;
        int result;
        char t[revlength];

        bhead = bpoint->branches;

        for (field=countnumflds(Head->num) + 1; field<=length; field=field+2) {

                if (bhead==nil) {error1s("No side branches present for %s.",partialno(t,revno,field-1));return nil;}

                /*find branch head*/
                /*branches are arranged in increasing order*/
                while ((bhead!=nil) &&
                       ((result=cmpnumfld(revno,bhead->hsh->num,field))>0)) {
                        bhead = bhead->nextbranch;
                }

                if (bhead==nil) {error1s("Branch number %s too high.",partialno(t,revno,field));return nil;}
                if (result<0)   {error1s("Branch number %s not present.",partialno(t,revno,field));return nil;}

                next = bhead->hsh;
                if (length==field) {
                        /* pick latest one on that branch */
                        trail=nil;
                        do { if ((date==nil?1:(cmpnum(date,next->date)>=0)) &&
                                 (author==nil?1:(strcmp(author,next->author)==0)) &&
                                 (state ==nil?1:(strcmp(state, next->state) ==0))
                             ) trail = next;
                             next=next->next;
                        } while (next!=nil);

                        if (trail==nil) {
                             error4s("Cannot find revision on branch %s with a date before %s, author %s, and state %s.",
                                        revno, date==nil?"<now>":date, 
                                        author==nil?"<any>":author, state==nil?"<any>":state);
                             return nil;
                        } else { /* print up to last one suitable */
                             next = bhead->hsh;
                             while (next!=trail) {
                                  /*puts(next->num);*/
                                  *store++ = next;
                                  next=next->next;
                             }
                             /*puts(next->num);*/
                             *store++ = next;
                        }
                        *store = nil;
                        return next;
                }

                /* length > field */
                /* find revision */
                /* check low */
                if (cmpnumfld(revno,next->num,field+1)<0) {
                        error1s("Revision number %s too low.",partialno(t,revno,field+1));
                        return(nil);
                }
                do {    /*puts(next->num);*/
                        *store++ = next;
                        trail = next;
                        next = next->next;
                } while ((next!=nil) &&
                       (cmpnumfld(revno,next->num,field+1) >=0));

                if ((length>field+1) &&  /*need exact hit */
                    (cmpnumfld(revno,trail->num,field+1) !=0)){
                        error1s("Revision %s not present.",partialno(t,revno,field+1));
                        return(nil);
                }
                if (length == field+1) {
                        if ((date!=nil) && (cmpnum(date,trail->date)<0)){
                                error2s("Revision %s has date %s.",trail->num, trail->date);
                                return nil;
                        }
                        if ((author!=nil)&&(strcmp(author,trail->author)!=0)) {
                                error2s("Revision %s has author %s.",trail->num,trail->author);
                                return nil;
                        }
                        if ((state!=nil)&&(strcmp(state,trail->state)!=0)) {
                                error2s("Revision %s has state %s.",trail->num,
                                       trail->state==nil?"<empty>":trail->state);
                                return nil;
                        }
                }
                bhead = trail->branches;

        }
        * store = nil;
        return trail;
}


char * lookupsym(id)
char * id;
/* Function: looks up id in the list of symbolic names starting
 * with pointer SYMBOLS, and returns a pointer to the corresponding
 * revision number. Returns nil if not present.
 */
{
        register struct assoc * next;
        next = Symbols;
        while (next!=nil) {
                if (strcmp(id, next->symbol)==0)
                        return(next->delta->num);
                else    next=next->nextassoc;
        }
        return nil;
}

int expandsym(source, target)
char * source, * target;
/* Function: Source points to a revision number. Expandsym copies
 * the number to target, but replaces all symbolic fields in the
 * source number with their numeric values.
 * A trailing '.' is omitted; leading zeroes are compressed.
 * returns false on error;
 */
{       register char * sp, * tp, *bp;
        char symbuf[30];
        register enum tokens d;

        sp = source; tp=target;
        if (sp == nil) { /*accept nil pointer as a legal value*/
                *tp='\0';
                return true;
        }

        while (*sp != '\0') {
                if (ctab[*sp] == DIGIT) {
                        if (*sp=='0') {
                                /* skip leading zeroes */
                                sp++;
                                while(*sp == '0') sp++;
                                if (*sp=='\0' || *sp=='.') *tp++ = '0'; /*single zero*/
                        }
                        while(ctab[*sp] == DIGIT) *tp++ = *sp++;
                        if ((*sp == '\0') || ((*sp=='.')&&(*(sp+1)=='\0'))) {
                                *tp='\0'; return true;
                        }
                        if (*sp == '.') *tp++ = *sp++;
                        else {
                            error1s("Improper revision number: %s",source);
                            *tp = '\0';
                            return false;
                        }
                } elsif (ctab[*sp] == LETTER) {
                        bp = symbuf;
                        do {    *bp++ = *sp++;
                        } while(((d=ctab[*sp])==LETTER) || (d==DIGIT) ||
                              (d==IDCHAR));
                        *bp= '\0';
                        bp=lookupsym(symbuf);
                        if (bp==nil) {
                                error1s("Symbolic number %s is undefined.",symbuf);
                                *tp='\0';
                                return false;
                        } else { /* copy number */
                                while (*tp++ = *bp++); /* copies the trailing \0*/
                        }
                        if ((*sp == '\0') || ((*sp=='.')&&(*(sp+1)=='\0')))
                                return true;
                        if (*sp == '.')  {
                                *(tp-1) = *sp++;
                        } else {
                                error1s("Improper revision number: %s",source);
                                return false;
                        }
                }else {
                        error1s("Improper revision number: %s", source);
                        *tp = '\0';
                        return false;
                }
        }
        *tp = '\0';
        return true;
}
