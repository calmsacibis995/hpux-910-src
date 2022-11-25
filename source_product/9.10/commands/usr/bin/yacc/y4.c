/* @(#) $Revision: 70.2 $ */   
# include "dextern"

#if defined( NLS) || defined( NLS16)
  extern nl_catd nlmsg_fd;	/* catalog descriptor */
#endif

# define a amem
# define mem mem0
# define pa indgo
# define yypact temp1
# define greed tystate

int * ggreed;
int * pgo;
int *yypgo;

int maxspr = 0;  /* maximum spread of any entry */
int maxoff = 0;  /* maximum offset into a array */
int *pmem;
int *maxa;
# define NOMORE FLAGVAL

int nxdb = 0;
int adb = 0;

callopt(){

	register i, *p, j, k, *q;

	/* read the arrays from tempfile and set parameters */

	finput = ftemp;		/* kludge because the code expects to read
				 * from finput. */
	rewind(finput);

	pgo[0] = 0;
	yypact[0] = 0;
	nstate = 0;
	nnonter = 0;
	for(;;){
		switch( gtnm() ){

		case '\n':
			yypact[++nstate] = (--pmem) - mem;
		case ',':
			continue;

		case '$':
			break;

		default:
			error( BADTMPFILE);
			}
		break;
		}

	yypact[nstate] = yypgo[0] = (--pmem) - mem;

	for(;;){
		switch( gtnm() ){

		case '\n':
			yypgo[++nnonter]= pmem-mem;
		case ',':
			continue;

		case EOF:
			break;

		default:
			error( BADTMPFILE);
			}
		break;
		}

	yypgo[nnonter--] = (--pmem) - mem;



	for( i=0; i<nstate; ++i ){

		k = MAXINT;
		j = 0;
		q = mem + yypact[i+1];
		for( p = mem + yypact[i]; p<q ; p += 2 ){
			if( *p > j ) j = *p;
			if( *p < k ) k = *p;
			}
		if( k <= j ){ /* nontrivial situation */
			/* temporarily, kill this for compatibility
			j -= k;  /* j is now the range */
			if( k > maxoff ) maxoff = k;
			}
		greed[i] = (yypact[i+1]-yypact[i]) + 2*j;
		if( j > maxspr ) maxspr = j;
		}

	/* initialize ggreed table */

	for( i=1; i<=nnonter; ++i ){
		ggreed[i] = 1;
		j = 0;
		/* minimum entry index is always 0 */
		q = mem + yypgo[i+1] -1;
		for( p = mem+yypgo[i]; p<q ; p += 2 ) {
			ggreed[i] += 2;
			if( *p > j ) j = *p;
			}
		ggreed[i] = ggreed[i] + 2*j;
		if( j > maxoff ) maxoff = j;
		}


	/* now, prepare to put the shift actions into the a array */

	for( i=0; i<actsize; ++i ) a[i] = 0;
	maxa = a;

	for( i=0; i<nstate; ++i ) {
		if( greed[i]==0 && adb>1 ) message( ftable, NULLSTATE, dtos(i));
		pa[i] = YYFLAG1;
		}

	while( (i = nxti()) != NOMORE ) {
		if( i >= 0 ) stin(i);
		else gin(-i);

		}

	if( adb>2 ){ /* print a array */
		for( p=a; p <= maxa; p += 10){
			fprintf( ftable, "%4d  ", p-a );
			for( i=0; i<10; ++i ) fprintf( ftable, "%4d  ", p[i] );
			fprintf( ftable, "\n" );
			}
		}
	/* write out the output appropriate to the language */

	aoutput();

	osummary();
	fclose( ftemp );
	finput = NULL;
	/* ZAPFILE( tempname ); */
	}

gin(i){

	register *p, *r, *s, *q1, *q2;

	/* enter gotos on nonterminal i into array a */

	ggreed[i] = 0;

	q2 = mem+ yypgo[i+1] - 1;
	q1 = mem + yypgo[i];

	/* now, find a place for it */

	for( p=a; p < &a[actsize]; ++p ){
		if( *p ) continue;
		for( r=q1; r<q2; r+=2 ){
			s = p + *r +1;
			if( *s ) goto nextgp;
			if( s > maxa ){
				if( (maxa=s) > &a[actsize] ) error( ARRYOVRFLW);
				}
			}
		/* we have found a spot */

		*p = *q2;
		if( p > maxa ){
			if( (maxa=p) > &a[actsize] ) error( ARRYOVRFLW);
			}
		for( r=q1; r<q2; r+=2 ){
			s = p + *r + 1;
			*s = r[1];
			}

		pgo[i] = p-a;
		if( adb>1 ) fprintf( ftable, "Nonterminal %d, entry at %d\n", i, pgo[i] );
		goto nextgi;

		nextgp:  ;
		}

	/*error( "cannot place goto %d\n", i );*/
	error(ARRYOVRFLW);

	nextgi:  ;
	}

stin(i){
	register *r, *s, n, flag, j, *q1, *q2;

	greed[i] = 0;

	/* enter state i into the a array */

	q2 = mem+yypact[i+1];
	q1 = mem+yypact[i];
	/* find an acceptable place */

	for( n= -maxoff; n<actsize; ++n ){

		flag = 0;
		for( r = q1; r < q2; r += 2 ){
			if( (s = *r + n + a ) < a ) goto nextn;
			if( *s == 0 ) ++flag;
			else if( *s != r[1] ) goto nextn;
			}

		/* check that the position equals another only if the states are identical */

		for( j=0; j<nstate; ++j ){
			if( pa[j] == n ) {
				if( flag ) goto nextn;  /* we have some disagreement */
				if( yypact[j+1] + yypact[i] == yypact[j] + yypact[i+1] ){
					/* states are equal */
					pa[i] = n;
					if( adb>1 ) fprintf( ftable, "State %d: entry at %d equals state %d\n",
						i, n, j );
					return;
					}
				goto nextn;  /* we have some disagreement */
				}
			}

		for( r = q1; r < q2; r += 2 ){
			if( (s = *r + n + a ) >= &a[actsize] ) error(OPTOVRFLW);
			if( s > maxa ) maxa = s;
			if( *s != 0 && *s != r[1] ) error( ARRYCLOBER, dtos(s-a), dtos(r[1]), dtos(*r));
			*s = r[1];
			}
		pa[i] = n;
		if( adb>1 ) fprintf( ftable, "State %d: entry at %d\n", i, pa[i] );
		return;

		nextn:  ;
		}

	/*error( "Error; failure to place state %d\n", i );*/
	error(ARRYOVRFLW);

	}

nxti(){ /* finds the next i */
	register i, max, maxi;

	max = 0;

	for( i=1; i<= nnonter; ++i ) if( ggreed[i] >= max ){
		max = ggreed[i];
		maxi = -i;
		}

	for( i=0; i<nstate; ++i ) if( greed[i] >= max ){
		max = greed[i];
		maxi = i;
		}

	if( nxdb ) fprintf( ftable, "nxti = %d, max = %d\n", maxi, max );
	if( max==0 ) return( NOMORE );
	else return( maxi );
	}

osummary(){
	/* write summary */

	register i, *p;

	if( foutput == NULL ) return;
	i=0;
	for( p=maxa; p>=a; --p ) {
		if( *p == 0 ) ++i;
		}

	message( foutput, OPTSPACE, dtos1(pmem-mem+1), dtos2(memsize), 
                                    dtos3(maxa-a+1), dtos4(actsize) );
	message( foutput, TBLENTRIES, dtos1((maxa-a)+1), dtos2(i) );
	message( foutput, MAXSPREAD, dtos1(maxspr), dtos2(maxoff) );

	}

aoutput(){ /* this version is for C */


	/* write out the optimized parser */

	fprintf( ftable, "# define YYLAST %d\n", maxa-a+1 );

	arout( "yyact", a, (maxa-a)+1 );
	arout( "yypact", pa, nstate );
	arout( "yypgo", pgo, nnonter+1 );

	}

arout( s, v, n ) uchar *s; int *v, n; {

	register i;

	fprintf( ftable, "%syytabelem %s[]={\n", yystorage_class, s );
	for( i=0; i<n; ){
		if( i%10 == 0 ) fprintf( ftable, "\n" );
		fprintf( ftable, "%6d", v[i] );
		if( ++i == n ) fprintf( ftable, " };\n" );
		else fprintf( ftable, "," );
		}
	}


gtnm(){

	register s, val, c;

	/* read and convert an integer from the standard input */
	/* return the terminating character */
	/* blanks, tabs, and newlines are ignored */

	s = 1;
	val = 0;

	while( (c=getc(finput)) != EOF ){
		if( isdigit(c) ){
			val = val * 10 + c - '0';
			}
		else if ( c == '-' ) s = -1;
		else break;
		}

	*pmem++ = s*val;
	if( pmem > &mem[memsize] ) error( NOSPACENM);
	return( c );

	}
