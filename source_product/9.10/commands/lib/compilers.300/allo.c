/* file allo.c */
/*    SCCS    REV(64.1);       DATE(92/04/03        14:21:49) */
/* KLEENIX_ID @(#)allo.c	64.1 91/08/28 */
# include "mfile2"

# define RMOVEM (NAREG|NBREG)	/* special case register need */

#ifdef DEBUGGING
flag rdebug = 0;
#endif	/* DEBUGGING */

NODE resc[5];

short busy[REGSZ];

short maxa, mina, maxb, minb, maxf, minf;
short maxd,mind;

#ifdef BRIDGE
int cg_max_dreg = D1;   /* D0 and D1 always available for codegen? */
int cg_max_areg = A1;   /* A0 and A1 always available for codegen? */
int cg_max_freg = F1;   /* F0 and F1 always available for codegen? */
int cg_max_fpreg = FP1; /* FP0 and FP1 always available for codegen? */

extern int usedregs;
extern int usedfdregs;

void cg_set_reg_info(reg)
   int               reg;
{
   /* reg vars allocated from high to low.  
    * cg temps allocated from low to high.
    */
   if (reg < F0) {
      usedregs |= (1<<reg);
      if (reg < A0) {
         /* data reg */

#ifdef   DEBUGGING
         if (reg < MINRVAR)  cerror("too many D reg vars");
#endif /*DEBUGGING*/

	 /*  fregs is the number of regs available.  reg is being allocated
	     using a 0 - 7 for d0-d7.  We want to set it at the value of
	     reg rather than reg - 1.  A better interface would be to use
	     setregs like the c compiler  	pgf  11/6/90
         if (reg <= fregs) fregs = reg-1;  /* fewer regs avail for temps 
	 */
         if (reg <= fregs) fregs = reg;  /* fewer regs avail for temps*/ 
         rstatus[reg] = SAREG;  
      } else {
         /* addr reg */

#ifdef   DEBUGGING
         if (reg-A0 < minrvarb) cerror("too many A reg vars");
#endif /*DEBUGGING*/

         if (reg-A0 <= afregs) afregs = reg-A0-1;
         rstatus[reg] = SBREG;  
      }
   } else {
      usedfdregs |= (1<<(reg-F0));
      if (reg < FP0) {

#ifdef   DEBUGGING
         if (reg-F0 < MINRFVAR)  cerror("too many F reg vars");
#endif /*DEBUGGING*/

         if (reg-F0 < ffregs) ffregs = reg-F0-1;
         rstatus[reg] = SFREG;  
      }
      else {

#  ifdef   DEBUGGING
         if (reg-FP0 < MINRDVAR)  cerror("too many FP reg vars");
#  endif /*DEBUGGING*/

         if (reg-FP0 < dfregs) dfregs = reg-FP0-1;
         rstatus[reg] = SDREG;
      }
   }
}
#endif /* BRIDGE */


allo0(){ /* free everything */

	register i;

	maxa = -1;  maxb = -1; maxf = -1;
	mina = 0;   minb = 0; minf = 0;
	maxd = -1; mind = 0;

	REGLOOP(i){
		busy[i] = 0;
		if( rstatus[i] & STAREG ){
			if( maxa<0 ) mina = i;
			maxa = i;
			}
		else if( rstatus[i] & STBREG ){
			if( maxb<0 ) minb = i;
			maxb = i;
			}
		else if( rstatus[i] & STFREG ){
			if( maxf<0 ) minf = i;
			maxf = i;
			}
		else if( rstatus[i] & STDREG ){
			if( maxd<0 ) mind = i;
			maxd = i;
			}
		}
	}


allo( p, q ) register NODE *p; struct optab *q; {

	register n = q->needs;				
	register i = 0;		

	while( n & NACOUNT ){
		resc[i].in.op = REG;
		resc[i].tn.rval = freereg( p, n&NAMASK );
		resc[i].tn.lval = 0;
		resc[i].in.name = 0;
		n -= NAREG;
		++i;
		}

	while( n & NBCOUNT ){
		resc[i].in.op = REG;
		resc[i].tn.rval = freereg( p, n&NBMASK );
		resc[i].tn.lval = 0;
		resc[i].in.name = 0;
		n -= NBREG;
		++i;
		}


	while( n & NFCOUNT ){
		resc[i].in.op = REG;
		resc[i].tn.rval = freereg( p, n&NFMASK );
		resc[i].tn.lval = 0;
		resc[i].in.name = 0;
		n -= NFREG;
		++i;
		}

	while( n & NDCOUNT ){
		resc[i].in.op = REG;
		resc[i].tn.rval = freereg( p, n&NDMASK );
		resc[i].tn.lval = 0;
		resc[i].in.name = 0;
		n -= NDREG;
		++i;
		}

	if( n & NTMASK ){
		/* temps are always assumed to be available */
		int llval;
		resc[i].in.op = OREG;
		if( p->in.op == STCALL || p->in.op == STARG ||
			p->in.op == UNARY STCALL || p->in.op == STASG )
			{
			llval = freetemp( (SZCHAR*p->stn.stsize +
						(SZINT-1))/SZINT );
			}
		else {
			llval = freetemp( (n&NTMASK)/NTEMP );
			}
		llval = BITOOR(llval);
		resc[i].tn.rval = TMPREG;
		resc[i].tn.lval = llval;
		resc[i].in.name = 0;
		++i;
		}

	/* turn off "temporarily busy" bit */

	REGLOOP(n){
		busy[n] &= ~TBUSY;
		}

	for( n=0; n<i; ++n ) if( resc[n].tn.rval < 0 ) return(0);
	return(1);

	}

#ifndef FORT1PASS
freetemp( k ){ /* allocate k integers worth of temp space */
	/* we also make the convention that, if the number of words is more than 1,
	/* it must be aligned for storing doubles... */

	tmpoff += k*SZINT;
	if( k>1 ) {
		SETOFF( tmpoff, ALDOUBLE );
		}
	if( tmpoff > maxoff ) maxoff = tmpoff;
	if( tmpoff-baseoff > maxtemp ) maxtemp = tmpoff-baseoff;
	return( -tmpoff );
	}
#endif /* ! FORT1PASS */

freereg( p, n ) register NODE *p; {
	/* allocate a register of type n */
	/* p gives the type, if floating */

	register j;

	/* not general; means that only one register (the result) OK for call */
	if( callop(p->in.op) ){
		j = CALLREG(p);
		if( usable( p, n, j ) ) return( j );
		/* have allocated callreg first */
		}
	j = p->in.rall & ~MUSTDO;
	if( j!=NOPREF && usable(p,n,j) ){ /* needed and not allocated */
		return( j );
		}
	if( n&NAMASK ){
		for( j=mina; j<=maxa; ++j ) if( rstatus[j]&STAREG ){
			if( usable(p,n,j) ){
#ifdef BRIDGE
                                if (j > cg_max_dreg) cg_max_dreg = j;
#endif /* BRIDGE */
				return( j );
				}
			}
		}
	else if( n &NBMASK ){
		for( j=minb; j<=maxb; ++j ) if( rstatus[j]&STBREG ){
			if( usable(p,n,j) ){
#ifdef BRIDGE
                                if (j > cg_max_areg) cg_max_areg = j;
#endif /* BRIDGE */
				return(j);
				}
			}
		}
	else if( (!p->in.fpaside) && (n &NFMASK) ){
		for( j=minf; j<=maxf; ++j ) if( rstatus[j]&STFREG ){
			if( usable(p,n,j) ){
#ifdef BRIDGE
                                if (j > cg_max_freg) cg_max_freg = j;
#endif /* BRIDGE */
				return(j);
				}
			}
		}
	else if( p->in.fpaside && (n &NDMASK) ){
		for( j=mind; j<=maxd; ++j ) if( rstatus[j]&STDREG ){
			if( usable(p,n,j) ){
#ifdef BRIDGE
                                if (j > cg_max_fpreg) cg_max_fpreg = j;
#endif /* BRIDGE */
				return(j);
				}
			}
		}

	return( -1 );
	}

usable( p, n, r ) register NODE *p; register int r; {
	/* decide if register r is usable in tree p to satisfy need n */

/* the following code is removed to allow unused regs to be used as temps */
/*
	if( !ISTREG(r) )
		cerror( "usable asked about nontemp register" );
*/

/* the following code was added to trap the propagation of bad index */
/* values for registers.                                             */
#ifdef DEBUGGING
	if(r > (REGSZ-1))
		cerror( "register index is out of bounds" );
#endif
	if( busy[r] > 1 ) return(0);
	if(((n&NAMASK) && !(rstatus[r]&SAREG)) ||
		((n&NBMASK) && !(rstatus[r]&SBREG)) ||
		((n&NDMASK) && !(rstatus[r]&SDREG)) ||
		((n&NFMASK) && !(rstatus[r]&SFREG)))
		return(0);
	if( (p->in.type==DOUBLE) && !((n&NFMASK) | (n&NDMASK)) )
		{ /* only do the pairing for real regs (not float regs on 68881). */
		if( r&01 ) return(0);
		if( !ISTREG(r+1) ) return( 0 );
		if( busy[r+1] > 1 ) return( 0 );
		if( busy[r] == 0 && busy[r+1] == 0  ||
		    busy[r+1] == 0 && shareit( p, r, n ) ||
		    busy[r] == 0 && shareit( p, r+1, n ) ){
			busy[r] |= TBUSY;
			busy[r+1] |= TBUSY;
			return(1);
			}
		else return(0);
		}
	if( busy[r] == 0 ) {
		busy[r] |= TBUSY;
		return(1);
		}

	/* busy[r] is 1: is there chance for sharing */
	return( shareit( p, r, n ) );

	}





shareit( p, r, n ) NODE *p; {
	/* can we make register r available by sharing from p
	   given that the need is n */
	if( (n&(NASL|NBSL|NFSL|NDSL)) && ushare( p, 'L', r ) ) return(1);
	if( (n&(NASR|NBSR|NFSR|NDSR)) && ushare( p, 'R', r ) ) return(1);
	return(0);
	}




ushare( p, f, r ) register NODE *p; {
	/* can we find a register r to share on the left or right
		(as f=='L' or 'R', respectively) of p */

	p = getlr( p, f );
	if( p->in.op == UNARY MUL ) p = p->in.left;
	if( p->in.op == OREG ){
		if( p->tn.rval < 0 )
			{
			/* It's really an indexed mode address (packed). */
			union indexpacker x;
			x.rval = p->tn.rval;
			return( r==x.i.addressreg || r==x.i.xreg );
			}
		else 
			return( r == p->tn.rval );
		}
	if( p->in.op == REG ){
		return( r == p->tn.rval || ( szty(p->in.type) == 2 &&
					!ISFDREG(r) && r==p->tn.rval+1 ) );
		}
	return(0);
	}




recl2( p ) register NODE *p; {
	register r = p->tn.rval;

	if( p->in.op == REG ) rfree( r, p->in.type );
	else if( p->in.op == OREG ) {
		if (p->tn.rval < 0)
			{
			/* It's really an indexed mode address (packed) */
			union indexpacker x;
			x.rval = p->tn.rval;
			if (x.i.addressreg) 
				rfree ( x.i.addressreg, (TWORD)(PTR+INT));
			rfree ( x.i.xreg, 
				ISBREG(x.i.xreg)? (TWORD)(PTR+INT):(TWORD)INT);
			}
		else 
			{
			rfree( r, PTR+INT );
			}
		}
	}





rfree( r, t ) register r; TWORD t; {
	/* mark register r free, if it is legal to do so */
	/* t is the type */

#ifdef DEBUGGING
	if( rdebug ){
		prntf( "rfree( %s ), size %x\n", rnames[r], szty(t) );
		}
#endif	/* DEBUGGING */

	if( ISTREG(r) ){
		if( --busy[r] < 0 ) cerror( "register overfreed");
		if( szty(t) == 2 )
			{
			if( (r&01) && !ISFDREG(r) ) 
				cerror( "illegal free" );
			if( !ISFDREG(r) && --busy[r+1] < 0 )
				cerror( "register overfreed" );
			}
		}
	}





rbusy(r,t) register r; TWORD t; {
	/* mark register r busy */
	/* t is the type */

#ifdef DEBUGGING
	if( rdebug ){
		prntf( "rbusy( %s ), size %x\n", rnames[r], szty(t) );
		}
#endif	/* DEBUGGING */

	if( ISTREG(r) ) ++busy[r];
	if( szty(t) == 2 ){
		if( !ISFDREG(r) && ISTREG(r+1) )
			/* 68881 fregs are big enough to store doubles alone. */
			++busy[r+1];
		if( !ISFDREG(r) && (r&01) )
			cerror( "illegal register pair freed" );
		}
	}





# ifdef DEBUGGING
rwprint( rw ){ /* print rewriting rule */
	register short pflag;
	register i;
	static char * rwnames[] = {

		"RLEFT",
		"RRIGHT",
		"RESC1",
		"RESC2",
		"RESC3",
		"RESC4",
		"RESC5",
		0,
		};

	if( rw == RNULL ){
		prntf( "RNULL" );
		return;
		}

	if( rw == RNOP ){
		prntf( "RNOP" );
		return;
		}

	pflag = 0;
	for( i=0; rwnames[i]; ++i ){
		if( rw & (1<<i) ){
			if( pflag ) prntf( "|" );
			++pflag;
			prntf( rwnames[i] );
			}
		}
	}
# endif	/* DEBUGGING */





reclaim( p, rw, cookie ) register NODE *p; register int rw, cookie; 
{
	register NODE **qq;
	register NODE *q;
	register struct respref *r;
	register i;
	NODE *recres[5];

	/* get back stuff */

#ifdef DEBUGGING
	if( rdebug ){
		prntf( "reclaim( %x, ", p );
		rwprint( rw );
		prntf( ", " );
		prcook( cookie );
		prntf( " )\n" );
		}
#endif	/* DEBUGGING */

	if( rw == RNOP || ( p->in.op==FREE && rw==RNULL ) ) return;  /* do nothing */

	walkf( p, recl2 );

	if( callop(p->in.op) ){
		/* check that all scratch regs are free */
		callchk(p);  /* ordinarily, this is the same as allchk() */
		}

	if( rw == RNULL || (cookie&FOREFF) ){ /* totally clobber, leaving nothing */
		tfree(p);
		return;
		}

	/* handle condition codes specially */

	if( (cookie & FORCC) && (rw&RESCC)) {
		/* result is CC register */
		tfree(p);
		p->in.op = CCODES;
		p->tn.lval = 0;
		p->tn.rval = 0;
		return;
		}

	/* locate results */

	qq = recres;

	if( rw&RLEFT) *qq++ = getlr( p, 'L' );;
	if( rw&RRIGHT ) *qq++ = getlr( p, 'R' );
	if( rw&RESC1 ) *qq++ = &resc[0];
	if( rw&RESC2 ) *qq++ = &resc[1];
	if( rw&RESC3 ) *qq++ = &resc[2];
	if( rw&RESC4 ) *qq++ = &resc[3];
	if( rw&RESC5 ) *qq++ = &resc[4];

	if( qq == recres ){
		cerror( "illegal reclaim");
		}

	*qq = NIL;

	/* now, select the best result, based on the cookie */

	for( r=respref; r->cform; ++r ){
		if( cookie & r->cform ){
			for( qq=recres; (q= *qq) != NIL; ++qq ){
#ifdef FASTMATCH
				if (tshape(q) & r->mform) goto gotit;
#else
				if( tshape( q, r->mform ) ) goto gotit;
#endif /* FASTMATCH */
				}
			}
		}

	/* we can't do it; die */
	cerror( "cannot reclaim");

gotit:

	if( p->in.op == STARG ) p = p->in.left;  /* STARGs are still STARGS */

	q->in.type = p->in.type;  /* to make multi-register allocations work */
		/* maybe there is a better way! */
	q = tcopy(q);

	tfree(p);

	p->in.op = q->in.op;
	p->tn.lval = q->tn.lval;
	p->tn.rval = q->tn.rval;
	p->in.name = q->in.name;	/* just copy the pointer */

	q->in.op = FREE;

	/* if the thing is in a register, adjust the type */

	switch( p->in.op ){

	case REG:
		{
		int sz = szty(p->in.type);
		if( ! (p->in.rall & MUSTDO ) ) return;  /* unless necessary, ignore it */
		i = p->in.rall & ~MUSTDO;
		if( i & NOPREF ) return;
		if( i != p->tn.rval )
			{
			if( busy[i] || ( sz==2 && busy[i+1] && !ISFDREG(i)) )
				cerror( "faulty register move" );
			rbusy( i, p->in.type );
			rfree( p->tn.rval, p->in.type );
			rmove( i, p->tn.rval, p->in.type, NULL );
			p->tn.rval = i;
			}
		}

	case OREG:
		if( p->tn.rval < 0 )
			{
			int r2;
			union indexpacker x;
			x.rval = p->tn.rval;
			i = x.i.addressreg;
		    	if( i && busy[i]>1 && ISTREG(i) )
			    	cerror( "potential register overwrite" );
			r2 = x.i.xreg;
			if (busy[r2]>1 && ISTREG(r2))
			    	cerror( "potential register overwrite" );
			}
		else
			if( (busy[p->tn.rval]>1) && ISTREG(p->tn.rval) )
			cerror( "potential register overwrite");
	}

}



# if defined(DEBUGGING)
/* ncopy2() is a node copier. */
ncopy2( q, p ) 
	/* copy the contents of p into q, without any feeling for
	   the contents */
	/* this code assume that copying rval and lval does the job;
	   in general, it might be necessary to special case the
	   operator types */

NODE *p, *q;
	{
	*q = *p;
	}
# endif /* DEBUGGING, MULTASSIGN */



/* tcopy() copies an entire tree (pass2 type) */
NODE *
tcopy( p ) register NODE *p; {
	/* make a fresh copy of p */

	register NODE *q;
	register r;

	q = talloc();
	ncopy( q, p );

	r = p->tn.rval;
	if( p->in.op == REG ) rbusy( r, p->in.type );
	else if( p->in.op == OREG ) {
		if(r < 0 )
			{
			/* It's really an indexed mode address (packed) */
			union indexpacker x;
			x.rval = r;
			if (x.i.addressreg) rbusy( x.i.addressreg, PTR+INT);
			rbusy( x.i.xreg, ISBREG(x.i.xreg)? 
					(TWORD)(PTR+INT) : (TWORD)INT);
			}
		else 
			{
			rbusy( r, (TWORD)(PTR+INT) );
			}
		}

	switch( optype(q->in.op) ){

	case BITYPE:
		q->in.right = tcopy(p->in.right);
	case UTYPE:
		q->in.left = tcopy(p->in.left);
		}

	return(q);
	}





allchk(){
	/* check to ensure that all register are free */

	register i;

	REGLOOP(i){
		if( busy[i] && ISTREG(i) )
			cerror( "register allocation error");
		}

	}
