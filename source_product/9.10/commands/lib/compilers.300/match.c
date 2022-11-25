/* file match.c */
/*    SCCS    REV(64.1);       DATE(92/04/03        14:22:12) */
/* KLEENIX_ID @(#)match.c	64.1 91/08/28 */

# include "mfile2"

#ifdef DEBUGGING
flag sdebug;
flag tdebug;
#endif

int fldsz, fldshf;


static long mamask[] = { /* masks for matching dope with shapes */
	SIMPFLG,		/* OPSIMP */
	SIMPFLG|ASGFLG,		/* ASG OPSIMP */
	COMMFLG,	/* OPCOMM */
	COMMFLG|ASGFLG,	/* ASG OPCOMM */
	MULFLG,		/* OPMUL */
	MULFLG|ASGFLG,	/* ASG OPMUL */
	DIVFLG,		/* OPDIV */
	DIVFLG|ASGFLG,	/* ASG OPDIV */
	UTYPE,		/* OPUNARY */
	TYFLG,		/* ASG OPUNARY is senseless */
	LTYPE,		/* OPLEAF */
	TYFLG,		/* ASG OPLEAF is senseless */
	0,		/* OPANY */
	ASGOPFLG|ASGFLG,	/* ASG OPANY */
	LOGFLG,		/* OPLOG */
	TYFLG,		/* ASG OPLOG is senseless */
	FLOFLG,		/* OPFLOAT */
	FLOFLG|ASGFLG,	/* ASG OPFLOAT */
	SHFFLG,		/* OPSHFT */
	SHFFLG|ASGFLG,	/* ASG OPSHIFT */
	SPFLG,		/* OPLTYPE */
	TYFLG,		/* ASG OPLTYPE is senseless */
	};

# define SPECIALLIST (SCCON|SICON|S8CON|SZERO|SONE|SMONE)

#ifdef FASTMATCH

tshape( p) register NODE *p; {
	/* return true if shape is appropriate for the node p
	   side effect for SFLD is to set up fldsz,etc */

	register short o = p->in.op;
	register  mask = SANY;

#ifdef DEBUGGING
	if( sdebug ){
		prntf( "tshape( 0x%x), op = 0x%x\n", p, o );
		}
#endif /* DEBUGGING */

	if( o == ICON && (p->in.name == NULL)) {
		if (p->tn.lval >= -32768 && p->tn.lval <= 32767) {
			mask |= SICON;
			if (p->tn.lval >= -128 && p->tn.lval <= 127) {
				mask |= SCCON;
				if (p->tn.lval == -1)
					mask |= SMONE;
				else if (p->tn.lval == 0)
					mask |= SZERO;
				else {
					if (p->tn.lval >= 1 && p->tn.lval <= 8) {
						mask |= S8CON;
						if (p->tn.lval == 1) mask |= SONE;
					}
				}
			}
		}
	}

	if(shtemp(p)) mask |= INTEMP;


	switch( o ){

	case NAME:
		mask |= SNAME;
		break;
	case ICON:
		mask |= SCON;
		break;
	case FLD:
		if( flshape( p->in.left ) ) {
			/* it is a FIELD shape; make side-effects */
			o = p->tn.rval;
			fldsz = UPKFSZ(o);
			fldshf = tsize2(p->in.type)*SZCHAR - fldsz - UPKFOFF(o);
			mask |= SFLD;
			}
		break;
	case CCODES:
		mask |= SCC;
		break;
	case REG:
		/* distinctions:
		SAREG	any scalar register
		STAREG	any temporary scalar register
		SBREG	any lvalue (index) register
		STBREG	any temporary lvalue register
		*/

		mask |= ISBREG( p->tn.rval ) ? SBREG : 
			ISFREG(p->tn.rval)? SFREG :
			ISDREG(p->tn.rval)? SDREG : SAREG;
		if( ISTREG( p->tn.rval ) && busy[p->tn.rval]<=1 )
			mask |= (mask&SAREG)? STAREG :
				(mask&SFREG)? STFREG :
				(mask&SDREG)? STDREG : STBREG;
		break;
	case OREG:
		mask |= SOREG;
		break;
	case UNARY MUL:
		/* return STARNM or STARREG or 0 */
		mask |= shumul(p->in.left);
		if (p->in.left->in.op == OREG) mask |= STARNM;
		break;

	case PLUS:
	case MINUS:
		if (ISPTR(p->in.type)) mask |= adrreg(p);
		break;
		}
	return(mask);
	}

#else /* FASTMATCH */

tshape( p, shape ) register NODE *p; register shape; {
	/* return true if shape is appropriate for the node p
	   side effect for SFLD is to set up fldsz,etc */

	register short o = p->in.op;
	register  mask;

#ifdef DEBUGGING
	if( sdebug ){
		prntf( "tshape( 0x%x, 0x%x), op = 0x%x\n", p, shape, o );
		}
#endif

	if( shape & SPECIAL )
		{
		if( o != ICON || p->in.name ) ;
		else
		switch( mask = shape & SPECIALLIST ){

		case SZERO:
		case SONE:
		case SMONE:
			if( p->tn.lval == 0 && shape == SZERO ) return(1);
			else if( p->tn.lval == 1 && shape == SONE ) return(1);
			else if( p->tn.lval == -1 && shape == SMONE ) return(1);
			else  break;

		default:

		case SCCON :
			if (p->tn.lval >= -128 && p->tn.lval <= 127)
				return(1);
			break;
		case SICON :	/* signed integer const. (16 bits) */
			if (p->tn.lval >= -32768 && p->tn.lval <= 32767)
				return (1);
			break;
		case S8CON :
			if (p->tn.lval >= 1 && p->tn.lval <= 8)
				return (1);
			break;
		}
		if (mask == shape) return (0);

		}

	if( shape & SANY ) return(1);

	if( (shape&INTEMP) && shtemp(p) ) return(1);


	switch( o ){

	case NAME:
		return( shape&SNAME );
	case ICON:
		mask = SCON;
		return( shape & mask );

	case FLD:
		if( shape & SFLD ){
			if( !flshape( p->in.left ) ) return(0);
			/* it is a FIELD shape; make side-effects */
			o = p->tn.rval;
			fldsz = UPKFSZ(o);
			fldshf = tsize2(p->in.type)*SZCHAR - fldsz - UPKFOFF(o);
			return(1);
			}
		return(0);

	case CCODES:
		return( shape&SCC );

	case REG:
		/* distinctions:
		SAREG	any scalar register
		STAREG	any temporary scalar register
		SBREG	any lvalue (index) register
		STBREG	any temporary lvalue register
		*/
		mask = ISBREG( p->tn.rval ) ? SBREG : 
			ISFREG(p->tn.rval)? SFREG :
			ISDREG(p->tn.rval)? SDREG : SAREG;
		if( ISTREG( p->tn.rval ) && busy[p->tn.rval]<=1 )
			mask |= (mask==SAREG)? STAREG :
				(mask==SFREG)? STFREG:
				(mask==SDREG)? STDREG:STBREG;
		return( shape & mask );

	case OREG:
		return( shape & SOREG );

	case UNARY MUL:
		/* return STARNM or STARREG or 0 */
		return( (shumul(p->in.left) & shape) || 
			(p->in.left->in.op == OREG && (shape & STARNM)) );

	case PLUS:
	case MINUS:
		if (ISPTR(p->in.type))
			return(adrreg(p) & shape);
		else
			return(0);
		}

	return(0);
	}
#endif /* FASTMATCH */

#ifdef FASTMATCH 

static int type_conv[]= { /* Index by frontend type, return backend type */
	/* UNDEF */	0,
	/* FARG */	0,
	/* CHAR */	TCHAR,
	/* SHORT */	TSHORT,
	/* INT */	TINT,
	/* LONG */	TLONG,
	/* FLOAT */	TFLOAT,
	/* DOUBLE */	TDOUBLE,
	/* STRTY */	TSTRUCT,
	/* UNIONTY */	TSTRUCT,
	/* ENUMTY */	0,
	/* MOETY */	0,
	/* UCHAR */	TUCHAR,
	/* USHORT */	TUSHORT,
	/* UNSIGNED */	TUNSIGNED,
	/* ULONG */	TULONG,
	/* VOID */	TVOID,
	/* LONGDOUBLE*/	TLONGDOUBLE,
	/* SCHAR */	0,
	/* LABTY */	0,
	/* SIGNED */	0,
	/* CONST */	0,
	/* VOLATILE */	0,
	/* TNULL */	0,
	};

#define TTYPE(t) (t == BTYPE(t) ? (type_conv[t] | TANY) : (TPOINT|TANY))

#else /* FASTMATCH */

ttype( t, tword ) register TWORD t; register int tword; {
	/* does the type t match tword */

#ifdef DEBUGGING
	if( tdebug ){
		prntf( "ttype( 0x%x, 0x%x )\n", t, tword );
		}
#endif
	if( tword & TANY ) return(1);

	if( ISPTR(t) && (tword&TPTRTO) ) {
		do {
			t = DECREF(t);
		} while ( ISARY(t) );
			/* arrays that are left are usually only
			   in structure references... */
		return( ttype( t, tword&(~TPTRTO) ) );
		}
	if( t != BTYPE(t) ) return( tword & TPOINT ); /* TPOINT means not simple! */
	if( tword & TPTRTO ) return(0);

	switch( t ){

	case CHAR:
		return( tword & TCHAR );
	case SHORT:
		return( tword & TSHORT );
	case STRTY:
	case UNIONTY:
		return( tword & TSTRUCT );
	case INT:
		return( tword & TINT );
	case UNSIGNED:
		return( tword & TUNSIGNED );
	case USHORT:
		return( tword & TUSHORT );
	case UCHAR:
		return( tword & TUCHAR );
	case ULONG:
		return( tword & TULONG );
	case LONG:
		return( tword & TLONG );
	case FLOAT:
		return( tword & TFLOAT );
	case DOUBLE:
		return( tword & TDOUBLE );
	case LONGDOUBLE:
		return( tword & TLONGDOUBLE );
	case VOID:
		return( tword & TVOID );
		}

	return(0);
	}

#endif /* FASTMATCH */

struct optab *rwtable;

struct optab *opptr[DSIZE];

setrew(){
	/* set rwtable to first value which allows rewrite */
	register struct optab *q;
	register short i;

#ifdef DEBUGGING
#ifdef FASTMATCH
	for (q=table; q->op != FREE; ++q){
		if (q->op < DSIZE) {
		    if (((optype(q->op) == BITYPE) &&
				((q->rshape & SANY) || (q->lshape & SANY))) ||
                       ((optype(q->op) == LTYPE) && (q->rshape & SANY)) ||
		       ((optype(q->op) == UTYPE) && (q->lshape & SANY))) {
			if ((q->needs != REWRITE) &&
                            (q->op != CCODES) &&
			    (q->op != FMONADIC)) {
				prntf("Error in template ");
				prindex(q);
				if (optype(q->op) == LTYPE) prntf("\tLTYPE");
				if (optype(q->op) == UTYPE) prntf("\tUTYPE");
				if (optype(q->op) == BITYPE) prntf("\tBITYPE");
				prntf("\t");
				prshape(q->rshape);
				prntf("\t");
				prshape(q->lshape);
				prntf("\n");
				}
			}
		    }
		}
#endif /* FASTMATCH */
#endif /* DEBUGGING */

	for( q = table; q->op != FREE; ++q ){
		if( q->needs == REWRITE ){
			rwtable = q;
			goto more;
			}
		}
	cerror( "bad setrew" );


more:
	for( i=0; i<DSIZE; ++i ){
		if( dope[i] ){ /* there is an op... */
			for( q=table; q->op != FREE; ++q ){
				/*  beware; things like LTYPE that match
				    multiple things in the tree must
				    not try to look at the NIL at this
				    stage of things!  Put something else
				    first in table.c  */
				/* at one point, the operator matching was 15% of the
				    total comile time; thus, the function
				    call that was here was removed...
				*/

				if( q->op < OPSIMP ){
					if( q->op==i ) break;
					}
				else 
					{
					register long opmtemp;
					if((opmtemp=mamask[q->op - OPSIMP])&SPFLG)
						{
						if ( daleafop(i) ) break;
						}
					else if( (dope[i]&(opmtemp|ASGFLG)) == opmtemp ) break;
					}
				}
			opptr[i] = q;
			}
		}
	}

#ifdef FASTMATCH
#ifdef DEBUGGING
prindex(q)
register struct optab *q;
{
	char *charptr, *stopptr, templatename[100];
	int  templateindex;

	charptr = q->cstring;
	stopptr = charptr + strlen(charptr);
	templateindex = 0;
	for (charptr = q->cstring;
	     (charptr < stopptr) &&
		(strncmp(charptr,"template",strlen("template")) != 0);
	     charptr++);
	while ((charptr < stopptr) && (*charptr != '\n')) {
		templatename[templateindex++] = *charptr++;
		}
	templatename[templateindex] = '\0';
	if (templateindex == 0) {
		sprntf(templatename,"template index %d",q-table);
		}
	prntf("%s",templatename);
}

#define PRINT(name) (i ? (prntf(name),i=0) : (prntf("|"),prntf(name)));

prtype(ptype) int ptype;
	{
		int i = 1;
		if (ptype & TANY) PRINT("TANY");
		if (ptype & TCHAR) PRINT("TCHAR");
		if (ptype & TSHORT) PRINT("TSHORT");
		if (ptype & TSTRUCT) PRINT("TSTRUCT");
		if (ptype & TINT) PRINT("TINT");
		if (ptype & TUNSIGNED) PRINT("TUNSIGNED");
		if (ptype & TUSHORT) PRINT("TUSHORT");
		if (ptype & TUCHAR) PRINT("TUCHAR");
		if (ptype & TULONG) PRINT("TULONG");
		if (ptype & TLONG) PRINT("TLONG");
		if (ptype & TFLOAT) PRINT("TFLOAT");
		if (ptype & TDOUBLE) PRINT("TDOUBLE");
		if (ptype & TLONGDOUBLE) PRINT("TLONGDOUBLE");
		if (ptype & TVOID) PRINT("TVOID");
	}
#endif /* DEBUGGING */
#endif /* FASTMATCH */

short posrel[] = {EQ, NE, GT, LE, GE, LT, LT, GE, LE, GT};

match( p, cookie ) register NODE *p; {
	/* called by: order, gencall
	   look for match in table and generate code if found unless
	   entry specified REWRITE.
	   returns MDONE, MNOPE, or rewrite specification from table */

	register struct optab *q;
	register NODE *r, *l;
	register short op = p->in.op;

# ifdef DEBUGGING
	char *charptr, *stopptr, templatename[100];
	int  templateindex;
# endif /* DEBUGGING */

#ifdef FASTMATCH
	int rshape, lshape, rtype, ltype;


	/* Set left shape and type */
	l = ((optype( p->in.op ) == LTYPE) ? p : p->in.left);
	lshape = tshape(l);
	ltype = TTYPE(l->in.type);
#ifdef DEBUGGING
	if ((odebug > 1) && (l == p)) prntf(", left=top");
#endif
	/* Set right shape and type */
	r = ((optype( p->in.op ) != BITYPE) || (ISPTR(p->in.type) && adrreg(p))) ? p : p->in.right;
	rshape = tshape(r);
	rtype = TTYPE(r->in.type);
#ifdef DEBUGGING
	if ((odebug > 1) && (r == p)) prntf(", right=top");
#endif
#ifdef DEBUGGING
	if (odebug > 1) {
		prntf("\nsubtree right shape is ");
		prshape(rshape);
		prntf(", subtree left shape is ");
		prshape(lshape);
		prntf("\nsubtree right type is ");
		prtype(rtype);
		prntf(", subtree left type is ");
		prtype(ltype);
		prntf("\n");
	}
#endif /* DEBUGGING */
#endif /* FASTMATCH */

#ifdef FASTMATCH
#define NOSHAPE(s)	((s & ~(SANY|INTEMP)) == 0)
	rcount();
	if (cookie == FORREW)
		q = rwtable;
	else if ((((r == p->in.right) && (l == p->in.left)) &&
			(p->in.op != FMONADIC) &&
			(NOSHAPE(rshape) || NOSHAPE(lshape))) ||
                 ((optype(p->in.op) & LTYPE) && (p->in.op != CCODES) &&
					NOSHAPE(rshape)) ||
		 ((optype(p->in.op) & UTYPE) && (p->in.op != UNARY MUL) &&
					NOSHAPE(lshape)))

		q = rwtable;
	else
		q = opptr[op];

#ifdef DEBUGGING
	if ((q == rwtable) && (odebug > 1)) prntf("Shortcut table\n");
#endif /* DEBUGGING */
#else
	rcount();			/* macro */
	if( cookie == FORREW ) q = rwtable;
	else q = opptr[op];

#endif /* FASTMATCH */

	for( ; q->op != FREE; ++q ){
#ifdef DEBUGGING
                if (odebug > 1) {
                        charptr = q->cstring;
                        stopptr = charptr + strlen(charptr);
                        templateindex = 0;
                        for (charptr = q->cstring; (charptr < stopptr) && (strncmp(charptr,"template",strlen("template")) != 0); charptr++);
                        while ((charptr < stopptr) && (*charptr != '\n')) {
                                templatename[templateindex++] = *charptr++;
                                }
                        templatename[templateindex] = '\0';
			if (templateindex == 0) {
				sprntf(templatename,"template index %d",q-table);
				}
                        prntf("MATCH: trying %s on node 0x%x, ",templatename,p);
                        }
#endif /* DEBUGGING */

#ifdef FASTMATCH
		if (((rshape & q->rshape) == 0) ||
		    ((lshape & q->lshape) == 0) ||
		    ((rtype & q->rtype) == 0) ||
		    ((ltype & q->ltype) == 0)) {
#if DEBUGGING
			if (odebug > 1) {
				prntf("FASTMATCH failed\n");
			}
#endif /* DEBUGGING */
			continue;
			}
#endif /* FASTMATCH */

		/* at one point the call that was here was over 15% of the total time;
		    thus the function call was expanded inline */
		if( q->op < OPSIMP ){
			if( q->op != op ) {
# ifdef DEBUGGING
				if (odebug > 1) prntf("wrong op.\n");
# endif /* DEBUGGING */
				continue;
				}
			}
		else {
			register long opmtemp;
			if((opmtemp=mamask[q->op - OPSIMP])&SPFLG)
				{
				if (!daleafop(op) &&
			 	    !(op==UNARY MUL && shumul(p->in.left)) )
					{
# ifdef DEBUGGING
					if (odebug > 1) prntf("failed.\n");
# endif /* DEBUGGING */
					if ((!ISPTR(p->in.type)) || (!adrreg(p)))
						continue;
					}
				/*	Originally ...
				if( op!=NAME && op!=ICON && op!= OREG &&
					! shltype( op, p ) ) continue;
				*/
				}
			else if( (dope[op]&(opmtemp|ASGFLG)) != opmtemp ) {
# ifdef DEBUGGING
				if (odebug > 1) prntf("failed.\n");
# endif /* DEBUGGING */
				continue;
				}
			}

		if( !(q->visit & cookie ) ) {
# ifdef DEBUGGING
			if (odebug > 1) prntf("failed.\n");
# endif /* DEBUGGING */
			continue;
			}
		
		/* If no intent to use float hardware, don't even consider
		   templates using INTFREG.
		*/
		/* r = getlr( p, 'L' );			/* see if left child matches */
#ifndef FASTMATCH
		r = optype( p->in.op ) == LTYPE ? p : p->in.left;

		if( !tshape( r, q->lshape ) ) {
# ifdef DEBUGGING
			if (odebug > 1) prntf("left shape failed.\n");
# endif /* DEBUGGING */
			continue;
			}
		if( !ttype( r->in.type, q->ltype ) ) {
# ifdef DEBUGGING
			if (odebug > 1) prntf("left type failed.\n");
# endif /* DEBUGGING */
			continue;
			}
		/* r = getlr( p, 'R' );			/* see if right child matches */
		r = ((optype( p->in.op ) != BITYPE) || (ISPTR(p->in.type) && adrreg(p))) ? p : p->in.right;
		if( !tshape( r, q->rshape ) ) {
# ifdef DEBUGGING
			if (odebug > 1) prntf("right shape failed.\n");
# endif /* DEBUGGING */
			continue;
			}
		if( !ttype( r->in.type, q->rtype ) ) {
# ifdef DEBUGGING
			if (odebug > 1) prntf("right type failed.\n");
# endif /* DEBUGGING */
			continue;
			}
#endif /* FASTMATCH */

			/* REWRITE means no code from this match but go ahead
			   and rewrite node to help future match */
		if( q->needs & REWRITE ) return( q->rewrite );
		/* if can't generate code, skip entry */
		if( !allo( p, q ) ) {
# ifdef DEBUGGING
			if (odebug > 1) prntf("allo failed.\n");
# endif /* DEBUGGING */
			continue;
			}
		expand( p, cookie, q->cstring );	/* generate code */
		reclaim( p, q->rewrite, cookie );

# ifdef DEBUGGING
		if (odebug > 1) prntf("expanded template.\n");
# endif /* DEBUGGING */
		return(MDONE);

		}

	return(MNOPE);
	}


expand( p, cookie, cp ) register NODE *p;  register char *cp; {
	/* generate code by interpreting table entry */
#ifndef FLINT
	for( ; *cp; ++cp ){
		switch( *cp ){

		default:
			(void)PUTCHAR( *cp );
			continue;  /* this is the usual case... */

		case 'A': /* address of */
			adrput( getlr( p, *++cp ) );
			continue;


		case 'C': /* for constant value only */
			conput( getlr( p, *++cp ) );
			continue;

		case 'F':  /* this line deleted if FOREFF is active */
			if( cookie & FOREFF ) while( *++cp != '\n' ) ; /* VOID */
			continue;

		case 'H':  /* field shift */
			prntf( "%d", fldshf );
			continue;

		case 'I': /* in instruction */
			INSPUT( getlr( p, *++cp ) );
			continue;


		case 'M':  /* field mask */
		case 'N':  /* complement of field mask */
			{
			register CONSZ val;
			CONSZ mask2;

			val = 1;
			val <<= fldsz;
			--val;
			val <<= fldshf;
			if (*cp == 'N')
				{
				val = ~val;
				switch (p->in.type)
					{
					case CHAR:
					case UCHAR:
						mask2 = 0x000000ff;
						break;
					case SHORT:
					case USHORT:
						mask2 = 0x0000ffff;
						break;
					default:
						mask2 = 0xffffffff;
						break;
					}
				val &= mask2;
				}
			adrcon( val );
			continue;
			}

		case 'O':  /* opcode string */
			hopcode( ++cp, p->in.op );
			continue;

		case 'U': /* for upper half of address, only */
			upput( getlr( p, *++cp ), 1 );
			continue;

		case 'V': /* for upper half of address, only */
			upput( getlr( p, *++cp ), 2 );
			continue;

		case 'W': /* for upper half of address, only */
			upput( getlr( p, *++cp ), 3 );
			continue;

		case 'Z':  /* special machine dependent operations */
			++cp;
			cp += zzzcode( p, cookie, cp );
			/* zzzcode() returns # of places IT increments cp */
			continue;

			/* from here on down are macros previously in zzzcode */
		case 'J':	/* signed or unsigned shift prefix */
			prntf("%c", ISUNSIGNED(p->in.left->in.type)? 'l':'a');
			continue;

		case 'K':
			cbgen ( (p->in.op <= UGT) ?
				(short)p->in.op : (short)posrel[p->in.op - FEQ],
					p->bn.label, *cp );
#if 0
			cbgen ( (flibflag || !(ISFTP(p->in.left->in.type) ||
				ISFTP(p->in.right->in.type)) 
				|| p->in.op > UGT ) ?
				(short)p->in.op : (short)fposrel[p->in.op - EQ],
					p->bn.label, *cp );
#endif /* 0 */
			continue;

		case 'G':
			cbgen ( (p->in.op > UGT) ?
				(short)p->in.op : (short)fposrel[p->in.op - EQ],
					p->bn.label, *cp );
			continue;

		case 'P':  /* like 'J' above, except FORCC is not valid */
			if (p->in.op == ASG LS)
			  prntf("%c", 'l'); /* use faster logical shifts */
			else
			  prntf("%c",ISUNSIGNED(p->in.left->in.type)? 'l':'a');
			continue;

		case 'Q':	/* divu/divs or mulu/muls suffix generator */
			if ( !ISFTP(p->in.type) )
				prntf("%c", ISUNSIGNED(p->in.left->in.type) ||
					ISUNSIGNED(p->in.right->in.type) ? 'u' : 's');
			continue;
			}

		}
#endif /* FLINT */
	}

NODE *
getlr( p, c ) register NODE *p; {

	/* return the pointer to the left or right side of p, or p itself,
	   depending on the optype of p */

	switch( c ) {

	case '1':
 		return(resc );
	case '2':
 		return(resc +  1);
	case '3':
 		return(resc +  2);

	case 'L':
		return( optype( p->in.op ) == LTYPE ? p : p->in.left );

	case 'R':
		return(((optype( p->in.op ) != BITYPE) ||
			(ISPTR(p->in.type) && adrreg(p))) ? p : p->in.right );
		}
	cerror( "bad getlr: %c", c );
	/* NOTREACHED */
	}





