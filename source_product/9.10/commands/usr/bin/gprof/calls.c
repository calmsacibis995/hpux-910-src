/* HPUX_ID: @(#) $Revision: 66.1 $  */
#include	"gprof.h"

    /*
     *	a namelist entry to be the child of indirect calls
     */
nltype	indirectchild = {
	"(*)" ,				/* the name */
	(unsigned long) 0 ,		/* the pc entry point */
	(unsigned long) 0 ,		/* entry point aligned to histogram */
	(double) 0.0 ,			/* ticks in this routine */
	(double) 0.0 ,			/* cumulative ticks in children */
	(long) 0 ,			/* how many times called */
	(long) 0 ,			/* how many calls to self */
	(double) 1.0 ,			/* propagation fraction */
	(double) 0.0 ,			/* self propagation time */
	(double) 0.0 ,			/* child propagation time */
	(bool) 0 ,			/* print flag */
	(int) 0 ,			/* index in the graph list */
	(int) 0 , 			/* graph call chain top-sort order */
	(int) 0 ,			/* internal number of cycle on */
	(struct nl *) &indirectchild ,	/* pointer to head of cycle */
	(struct nl *) 0 ,		/* pointer to next member of cycle */
	(arctype *) 0 ,			/* list of caller arcs */
	(arctype *) 0 			/* list of callee arcs */
    };

void
findcalls( parentp , p_lowpc , p_highpc )
    nltype		*parentp;
    unsigned long	p_lowpc;
    unsigned long	p_highpc;
{
    nltype		*childp;
    register unsigned short	*instructp;
    long		reg;
    long		mode;
    register unsigned long	destpc;
    register long		length;

    if ( textspace == 0 ) {
	return;
    }
    if ( p_lowpc < s_lowpc ) {
	p_lowpc = s_lowpc;
    }
    if ( p_highpc > s_highpc ) {
	p_highpc = s_highpc;
    }
#   ifdef DEBUG
	if ( debug & CALLSDEBUG ) {
	    printf( "[findcalls] %s: 0x%x to 0x%x\n" ,
		    parentp -> name , p_lowpc , p_highpc );
	}
#   endif DEBUG
    for (   instructp = (unsigned short *)(textspace + p_lowpc) ;
	    instructp < (unsigned short *)(textspace + p_highpc) ;
	    instructp += length ) {
	length = 1;  /* 2 bytes */
	if ( (*instructp & JSR_MASK) == JSR ) {
		/*
		 *	maybe a jsr, better check it out.
		 *	skip the count of the number of arguments.
		 */
#	    ifdef DEBUG
		if ( debug & CALLSDEBUG ) {
		    printf( "[findcalls]\t0x%x:calls" , instructp - textspace );
		}
#	    endif DEBUG
	    /* get mode */
	    mode = (*instructp & 0x38) >> 3;
	    switch (mode) {

	    default:   /* botched */
		    goto botched;

	    case 2:
	    case 5:
	    case 6:	/* indirect */
indirect:
		    addarc( parentp, &indirectchild, (long) 0);
		    continue;

	    case 7:  /* get the register */
		    reg = *instructp & 7;
		    switch (reg) {
		    default:
			    goto botched;
		    case 0:
			    destpc = *(instructp + 1);
			    length++;
			    goto checkaddr;

		    case 1:
			    destpc = *((long *)(instructp + 1));
			    length += 2;
			    goto checkaddr;

		    case 2:  /* PC relative */
			    destpc = *(((short *)instructp) + 1)
				     + ((long)instructp - (long)textspace)
				     + 2;
			    length++;
checkaddr:
			    if ((destpc >= s_lowpc) && (destpc <= s_highpc))
				{
				childp = nllookup (destpc);
				if (childp->value == destpc) {  /* a hit */
#ifdef DEBUG
			    	    if ( debug & CALLSDEBUG ) {
					printf( "[findcalls]\tdestpc 0x%x" , destpc );
					printf( " childp->name %s" , childp -> name );
					printf( " childp->value 0x%x\n" ,
						childp -> value );
			    	    }
#endif DEBUG
				    addarc( parentp, childp, (long) 0);
				    continue;
				    }
				}
			    goto botched;

		   case 3:  /* indirect */
		        goto indirect;
		   }
	    }
	}
	else if ((*instructp & BSR_MASK) == BSR)
	    {
	    destpc = (short)(*instructp & 0xff);
	    if (destpc == 0)
		{
		length++;
		destpc = (short)(*(instructp + 1))
			 + (long)instructp - (long)textspace + 2;
		goto checkaddr;
		}
	    else if (destpc == 0xff)
		{
		length += 2;
		destpc = *((long *)(instructp + 1))
			 + (long)instructp - (long)textspace + 2;
		goto checkaddr;
		}
	    else
		{
		destpc += (long)instructp - (long)textspace + 2;
		goto checkaddr;
		}
	    }
botched:
    /*
     *	something funny going on.
     */
#ifdef DEBUG
    if ( debug & CALLSDEBUG ) {
	printf( "[findcalls]\tbut it's a botch\n" );
    }
#endif DEBUG
    length = 1;
    continue;
    }
}
