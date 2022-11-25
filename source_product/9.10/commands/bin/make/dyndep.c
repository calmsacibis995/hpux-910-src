/* @(#) $Revision: 70.1 $ */   
#include "defs"

#ifdef NLS16
#include <nl_ctype.h>
#endif

#define is_dyn(a)        (any( (a), DOLLAR) )


dynamicdep(p)
register NAMEBLOCK p;
/*******************************************************************/
/*    Dynamicdep() checks each dependency by calling runtime().    */
/*    Runtime() determines if a dependent line contains "$@"       */
/*    or "$(@F)" or "$(@D)". If so, it makes a new dependent line  */
/*    and inserts it into the dependency chain of the input name,  */
/*    p. Here, "$@" gets translated to p->namep. That is           */
/*    the current name on the left of the colon in the             */
/*    makefile.  Thus,                                             */
/*        xyz:    s.$@.c                                           */
/*    translates into                                              */
/*        xyz:    s.xyz.c                                          */
/*                                                                 */
/*    Also, "$(@F)" translates to the same thing without a         */
/*    preceding directory path (if one exists).                    */
/*    Note, to enter "$@" on a dependency line in a makefile       */
/*    "$$@" must be typed. This is because `make' expands          */
/*    macros upon reading them.                                    */
/*******************************************************************/
{
    register LINEBLOCK lp, nlp;
    LINEBLOCK backlp=0;

    p->rundep = 1;

    for(lp = p->linep; lp != 0; lp = lp->nextline)
    {
        if( (nlp=runtime(p, lp)) != 0)
            if(backlp)
                backlp->nextline = nlp;
            else
                p->linep = nlp;

        backlp = (nlp == 0) ? lp : nlp;
    }
}

LINEBLOCK runtime(p, lp)
NAMEBLOCK p;
register LINEBLOCK lp;
/************************************/
/* see above comments               */
/* For each line in the nameblock,  */
/* each line has its macros expanded*/
/* and the new line is added to the */
/* lineblock.  The new lineblock is */
/* returned from the function       */
/************************************/
{
    union
    {
        int u_i;
        NAMEBLOCK u_nam;
    } temp;                     /* variant record - temporary var */
    register DEPBLOCK q, nq;
    LINEBLOCK nlp;
    NAMEBLOCK pc;
    CHARSTAR pc1;
    unsigned char c;
    CHARSTAR pbuf;
    unsigned char buf[INMAX];

    temp.u_i = NO;
  /* look for macro definitions in the deplist associated with this line */
    for(q = lp->depp; q != 0; q = q->nextdep)
    {
        if((pc=q->depname) != 0)
        {
            if(is_dyn(pc->namep))
            {
                temp.u_i = YES;
                break;
            }
        }
    }

    if(temp.u_i == NO)
    {
        return(0);           /* no macros were found in this line */
    }

    nlp = ALLOC(lineblock);  /* if here, at least one macro was found */
    nq  = ALLOC(depblock);   /* allocate space to add the expanded version */

    nlp->nextline = lp->nextline;   /* add more dependency info? */
    nlp->shp   = lp->shp;
    nlp->depp  = nq;

    for(q = lp->depp; q != 0; q = q->nextdep)  /* for each dependency */
    {
        pc1 = q->depname->namep;              /* get the name */
        if(is_dyn(pc1))                       /* if it contains a macro */
        {
            subst(pc1, buf, &buf[INMAX]);     /* expand the macro into buf */
            temp.u_nam = srchname(buf);       /* checks for "buf" in hashtable*/
            if(temp.u_nam == 0)               /* if not in hashtable */
                temp.u_nam = makename(copys(buf)); /* force into temp var */
            nq->depname = temp.u_nam;         /* insert new macro into deplist*/
        }
        else
        {
            nq->depname = q->depname;
        }

        if(q->nextdep == 0)
            nq->nextdep = 0;
        else
            nq->nextdep = ALLOC(depblock);

        nq = nq->nextdep;
    }
    return(nlp);
}
