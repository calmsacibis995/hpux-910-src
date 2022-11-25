/* @(#) $Revision: 70.10 $       */
/* parser.y */

/* See file main.c for documentation on the implementation of %x states.
 *
 * Here as a side note, the grammar rules that handle start conditions
 * are right recursive, which is inefficient for a LALR parser.  If
 * I was doing major rework to lex (or starting a design), I would
 * change this to left recursion.  At this point, however, to change
 * the recursion would require defining a different shape to the
 * trees for start conditions, and would require changes throughout
 * the tree handling routines.  So in the grammar changes, I left it
 * so that it was still right recursive, and would build the same
 * shape pattern trees as before.
 */

%token CHAR CCLNLS CCL NCCL STR DELIM SCON ITER NEWE NULLS
%left SCON '/' NEWE
%left '|'
%left '$' '^'
%left CHAR CCLNLS CCL NCCL '(' '.' STR NULLS
%left ITER
%left CAT
%left '*' '+' '?'

%{
# include "ldefs.c"
# include "msgs.h"

#define BADPR	(-1)	/* Return code to signal an error in a production */

#define YYSTYPE union _yystype_
union _yystype_
{
	int	i;
	uchar	*cp;
};

%}
%%
%{
int i;
int j,k;
int g;
uchar *p;
%}
acc	:	lexinput
	={	
# ifdef DEBUG
		if(debug) sect2dump();
# endif
	}
	;
lexinput:	defns delim prods end
	|	defns delim end
	={
		if(!funcflag)phead2();
		funcflag = TRUE;
	}
	| error
	={
# ifdef DEBUG
		if(debug) {
			sect1dump();
			sect2dump();
			}
# endif
		}
	;
end:		delim | ;
defns:	defns STR STR
	={	int l2, l3;
		l2 = slength($2.cp) + 1;
		l3 = slength($3.cp) + 1;
		if(dp+l2+l3 >= dchar+defchar)
			error(LONGDEFS);

		scopy($2.cp,dp);
		def[dptr] = dp;
		dp += l2;
		scopy($3.cp,dp);
		subs[dptr++] = dp;
		if(dptr >= defsize)
			error(MANYDEFS);
		dp += l3;
		subs[dptr]=def[dptr]=0;	/* for lookup - require ending null */
	}
	|
	;
delim:	DELIM
	={
# ifdef DEBUG
		if(sect == DEFSECTION && debug) sect1dump();
# endif
		sect++;
		}
	;

	/* Productions get formed into a tree.  But if an error was
	 * found, we don't want to keep the (probably) badly formed
	 * production tree, because it will quite likely cause bad
	 * memory accesses later in the processing.
	 */
prods:	prods pr
	={	if ($1.i==BADPR)	/* in case the first production was 
					 * an error, and no list started yet  */
		   $$.i = $2.i;
		else {
		   if ($2.i==BADPR)	/* just skip it, don't insert in tree */
		   	$$.i = $1.i;
		   else
			$$.i = mn2(RNEWE,$1.i,$2.i);	/* normal case */
		   }
		}
	|	pr
	={	$$.i = $1.i;}
	;
pr:	r1 NEWE
	={
		if(divflg == TRUE)
			i = mn1(S1FINAL,casecount);
		else i = mn1(FINAL,casecount);
		$$.i = mn2(RCAT,$1.i,i);
		divflg = FALSE;
		casecount++;
		}
	| error NEWE
	={
		 $$.i = BADPR ;		/* signal bad production to higher
					 * level */
# ifdef DEBUG
		if(debug) sect2dump();
# endif
		}
r1:	SCON  r2
	{	$$.i = mn2(RSCON,$2.i,$1.i); }
	| r
	{	/* here is where we can mark a pattern as having
		 * no start condition associated.
		 */
		 $$.i = mn1(RNOSCON,$1.i);
		 }
	;
r2:	SCON r2
	{	$$.i = mn2(RSCON,$2.i,$1.i); }
	| r
	{	$$.i = $1.i; }
	;
r:	CHAR
	={	$$.i = mn0($1.i); }
	| STR
	={
		p = $1.cp;
		i = mn0(*p++);
		
		while(*p)
			i = mn2(RSTR,i,*p++);
		$$.i = i;
		}
	| '.'
	={	symbol['\n'] = 0;
		if(psave == FALSE){
# ifdef NLS16
                        if (nls16) {
                             /* The only single byte characters that will ever*/
                             /* occur when in a multibyte locale will be in   */
                             /* ASCII (except under the japanese locale)      */
                             /* therefore, we only want 1-127 to be recognized*/
			     if(ccptr + NCH + 1 >= ccl+cclsize)
                                     error(MANYCLASSES);
			     p = ccptr;
			     psave = ccptr;
			     for(i=1; i < '\n'; i++){
				     symbol[i] = 1;
				     *ccptr++ = i;
				     }
			     for(i='\n'+1; i <= 127; i++){
				     symbol[i] = 1;
				     *ccptr++ = i;
				     }
                             /* Partial hack added to support the KANA-8      */
                             /* characters (0xa1 - 0xdf) found in the japanese*/
                             /* character set (HP15).  We must also recognize */
                             /* the single byte characters that are not first */
                             /* of 2 characters.  I.e., 0xa1 - 0xdf.          */
                             for (i=128; i < NCH; i++)
                                     if (! FIRSTof2(i)) {
                                           symbol[i] = 1;
                                           *ccptr++ = i;
                                           }
			     *ccptr++ = 0;
                        }
			else
                        {
#endif
			     if(ccptr + NCH + 1 >= ccl+cclsize)
                                     error(MANYCLASSES);
			     p = ccptr;
			     psave = ccptr;
			     for(i=1;i<'\n';i++){
				     symbol[i] = 1;
				     *ccptr++ = i;
				     }
			     for(i='\n'+1;i<NCH;i++){
				     symbol[i] = 1;
				     *ccptr++ = i;
				     }
			     *ccptr++ = 0;
# ifdef NLS16
                        }  /* end else nls16 */
#endif
		     } 
		else
			p = psave;
		$$.i = mn1(RCCL,p);
		cclinter(1);
# ifdef NLS16
                if (nls16)
                   /* Combine the single byte part with the range of all */
                   /* multibyte characters */
                   $$.i = mn2(BAR, $$.i, do_multibyte_dot());
# endif                
		}
	| CCL
	={	$$.i = mn1(RCCL,$1.i); 
                if (col_syms != None) {
                   $$.i = mn2(BAR, col_syms, $$.i); 
                   col_syms = None;
                   }
                }
	| NCCL
	={	$$.i = mn1(RNCCL,$1.i); }
	| CCLNLS
	={	$$.i = $1.i; /* Treen node has already been created. */ }
	| r '*'
	={	$$.i = mn1(STAR,$1.i); }
	| r '+'
	={	$$.i = mn1(PLUS,$1.i); }
	| r '?'
	={	$$.i = mn1(QUEST,$1.i); }
	| r '|' r
	={	$$.i = mn2(BAR,$1.i,$3.i); }
	| r r %prec CAT
	={	$$.i = mn2(RCAT,$1.i,$2.i); }
	| r '/' r
	={	if(!divflg){
			j = mn1(S2FINAL,-casecount);
			i = mn2(RCAT,$1.i,j);
			$$.i = mn2(DIV,i,$3.i);
			}
		else {
			$$.i = mn2(RCAT,$1.i,$3.i);
			warning(XTRASLASH);
			}
		divflg = TRUE;
		}
	| r ITER ',' ITER '}'
	={	if($2.i > $4.i){
			i = $2.i;
			$2.i = $4.i;
			$4.i = i;
			}
		if($4.i <= 0)
			warning(NEGINTR);
		else {
			j = $1.i;
			for(k = 2; k<=$2.i;k++)
				j = mn2(RCAT,j,dupl($1.i));
			for(i = $2.i+1; i<=$4.i; i++){
				g = dupl($1.i);
				for(k=2;k<=i;k++)
					g = mn2(RCAT,g,dupl($1.i));
				j = mn2(BAR,j,g);
				}
			$$.i = j;
			}
	}
	| r ITER '}'
	={
		if($2.i < 0)warning(NEGINTR);
		else if($2.i == 0) $$.i = mn0(RNULLS);
		else {
			j = $1.i;
			for(k=2;k<=$2.i;k++)
				j = mn2(RCAT,j,dupl($1.i));
			$$.i = j;
			}
		}
	| r ITER ',' '}'
	={
				/* from n to infinity */
		if($2.i < 0)warning(NEGINTR);
		else if($2.i == 0) $$.i = mn1(STAR,$1.i);
		else if($2.i == 1)$$.i = mn1(PLUS,$1.i);
		else {		/* >= 2 iterations minimum */
			j = $1.i;
			for(k=2;k<$2.i;k++)
				j = mn2(RCAT,j,dupl($1.i));
			k = mn1(PLUS,dupl($1.i));
			$$.i = mn2(RCAT,j,k);
			}
		}
	| '^' r
	={	$$.i = mn1(CARAT,$2.i); }
	| r '$'
	={	i = mn0('\n');
		if(!divflg){
			j = mn1(S2FINAL,-casecount);
			k = mn2(RCAT,$1.i,j);
			$$.i = mn2(DIV,k,i);
			}
		else $$.i = mn2(RCAT,$1.i,i);
		divflg = TRUE;
		}
	| '(' r ')'
	={	$$.i = $2.i; }
	|	NULLS
	={	$$.i = mn0(RNULLS); }
	;
%%
yylex(){
	register uchar *p;
	register int c, i;
	uchar  *t, *xp;
	int n, j, k, x;
	static int sectbegin;
	static uchar token[TOKENSIZE];
	static int iter;
# ifdef NLS16
	static uchar range[TOKENSIZE];
	uchar prevprev;
        int has_ere, multibyte;
# endif

# ifdef DEBUG
	yylval.i = 0;
# endif

	if(sect == DEFSECTION) {		/* definitions section */
		while(!eof) {
			if(prev == '\n'){		/* next char is at beginning of line */
				getl(p=buf);
				switch(*p){
				case '%':
					switch(c= *(p+1)){
					case '%':
						lgate();
						if(!ratfor)fprintf(fout,"# ");
						fprintf(fout,"define YYNEWLINE %d\n",ctable['\n']);
						if(!ratfor)fprintf(fout,"yylex(){\n   int nstr; extern int yyprevious;\n");
						sectbegin = TRUE;
						i = treesize*(sizeof(*name)+sizeof(*left)+
							sizeof(*right)+sizeof(*nullstr)+sizeof(*parent))+ALITTLEEXTRA;
						c = (int)myalloc(i,1);
						if(c == 0)
							error(NOCORE4);
						p = (uchar *)c;
						cfree((char *)p,i,1);
						name = (int *)myalloc(treesize,sizeof(*name));
						left = (int *)myalloc(treesize,sizeof(*left));
						right = (int *)myalloc(treesize,sizeof(*right));
						nullstr = myalloc(treesize,sizeof(*nullstr));
						parent = (int *)myalloc(treesize,sizeof(*parent));
						if(name == 0 || left == 0 || right == 0 || parent == 0 || nullstr == 0)
							error(NOCORE4); 
						return(freturn(DELIM));

					case 'p': case 'P':	/* has overridden number of positions */
                                                                /* Or specified POSIX %pointer */
                                                if (!strncmp("%pointer",p,8)) {
                                                /* %pointer support 8/27/91 */
                                                     yytextarr=0;
                                                }
                                                else {
                                                /* position # override */
						     while(*p && !digit(*p))p++;
						     maxpos = siconv(p);
# ifdef DEBUG
						     if (debug) printf("positions (%%p) now %d\n",maxpos);
# endif
						     if(report == 2)report = 1;
                                                }
						continue;
					case 'q': case 'Q':	/* has overridden number of positions  per state*/
						while(*p && !digit(*p))p++;
						maxpos_state = siconv(p);
# ifdef DEBUG
						if (debug) printf("positions per state (%%q) now %d\n",maxpos_state);
# endif
						if(report == 2)report = 1;
						continue;
					case 'n': case 'N':	/* has overridden number of states */
						while(*p && !digit(*p))p++;
						nstates = siconv(p);
# ifdef DEBUG
						if(debug)printf( " no. states (%%n) now %d\n",nstates);
# endif
						if(report == 2)report = 1;
						continue;
					case 'e': case 'E':		/* has overridden number of tree nodes */
						while(*p && !digit(*p))p++;
						treesize = siconv(p);
# ifdef DEBUG
						if (debug) printf("treesize (%%e) now %d\n",treesize);
# endif
						if(report == 2)report = 1;
						continue;
					case 'o': case 'O':
						while (*p && !digit(*p))p++;
						outsize = siconv(p);
						if (report ==2) report=1;
						continue;
					case 'a': case 'A':		/* has overridden number of transitions */
                                                                        /* Or specified POSIX %array   */
                                                if (!strncmp("%array",p,6)) {
                                                /* %array support 8/27/91 */
                                                     yytextarr=1;
						}
                                                else {
                                                /* Transition # override */
                                                     while(*p && !digit(*p))p++;
						     if(report == 2)report = 1;
						     ntrans = siconv(p);
# ifdef DEBUG
						     if (debug)printf("N. trans (%%a) now %d\n",ntrans);
# endif
                                                }
						continue;
					case 'k': case 'K': /* overriden packed char classes */
						while (*p && !digit(*p))p++;
						if (report==2) report=1;
						cfree((char *)pchar, pchlen, sizeof(*pchar));
						pchlen = siconv(p);
# ifdef DEBUG
						if (debug) printf( "Size classes (%%k) now %d\n",pchlen);
# endif
						pchar=pcptr=myalloc(pchlen, sizeof(*pchar));
						continue;
					case 't': case 'T': 	/* character set specifier */
						ZCH = atoi(p+2);
						if(ZCH < NCH) ZCH = NCH;
						if(ZCH > 2*NCH) error(CHTAB1);
						chset = TRUE;
						for(i = 0; i<ZCH; i++)
							ctable[i] = 0;
						while(getl(p) && scomp(p,"%T") != 0 && scomp(p,"%t") != 0){
							if((n = siconv(p)) <= 0 || n > ZCH){
								warning(CHARRANGE, itos(b_, n));
								continue;
								}
							while(!space(*p) && *p) p++;
							while(space(*p)) p++;
							t = p;
							while(*t){
								c = ctrans(&t);
								if(ctable[c]){
									if (printable(c))
										warning(DCHARC, itos(b_,c));
									else
										warning(DCHAR0, itos(b_,c));
									}
								else ctable[c] = n;
								t++;
								}
							p = buf;
							}
						{
						uchar chused[2*NCH]; int kr;
						for(i=0; i<ZCH; i++)
							chused[i]=0;
						for(i=0; i<NCH; i++)
							chused[ctable[i]]=1;
						for(kr=i=1; i<NCH; i++)
							if (ctable[i]==0)
								{
								while (chused[kr] != 0)
									kr++;
								ctable[i]=kr;
								chused[kr]=1;
								}
						}
						lgate();
						continue;
					case 'r': case 'R':
						c = 'r';
					case 'c': case 'C':
						if(lgatflg)
							error(LANG2LATE);
						ratfor = (c == 'r');
						continue;
					case '{':
						lgate();
						while(getl(p) && scomp(p,"%}") != 0)
							fprintf(fout, "%s\n",p);
						if(p[0] == '%') continue;
						error(PEOF);
					case 's': case 'S':		/* start conditions */
					case 'x': case 'X':
						lgate();
						while(*p && index(*p," \t,") < 0) p++;
						n = TRUE;
						while(n){
							int sl;
							while(*p && index(*p," \t,") >= 0) p++;
							t = p;
							while(*p && index(*p," \t,") < 0)p++;
							if(!*p) n = FALSE;
							*p++ = 0;
							if (*t == 0) continue;
							i = sptr*2;
							if(!ratfor)fprintf(fout,"# ");
							fprintf(fout,"define %s %d\n",t,i);
							sl = slength(t) + 1;
							if(sp +sl >= schar+startchar)
								error(LONGSTART);
							scopy(t,sp);
#ifdef DEBUG
				printf("sname[%d] = %s stype[%d] = %c\n",
					sptr, sp, sptr, c);
# endif
							stype[sptr] = c;
							sname[sptr++] = sp;
							sname[sptr] = 0;	/* required by lookup */
							if(sptr >= startsize)
								error(TSTART1);
							sp += sl;
							}
						continue;
# ifdef NLS16
                                        case 'L':/* Specify run-time language */
                                        case 'l':
                                          if(lgatflg)
                                                        error(LOCALE2LATE);
                                          for(i=2;buf[i] == ' ' && buf[i]; i++);
                                          for(j=i;buf[j] != ' ' && buf[j]; j++);
                                          buf[j] = 0;
                                          /* strip quotes if they exist */
                                          if (buf[i] == '"' && buf[j-1] == '"'){
                                              i++;
                                              buf[j-1] = 0;
                                              }
                                          if (!setlocale(LC_COLLATE,&buf[i]))
                                             warning(BADLOCALE, &buf[i]);
                                          else if (!setlocale(LC_CTYPE,&buf[i]))
                                             warning(BADLOCALE, &buf[i]);
                                          if (!(strcmp(nl_langinfo(BYTES_CHAR),
                                                       "2"))) {
					     if (opt_w_set)
   						nls_wchar = 1;
                                             nls16=1;
                                             }
                                          else
                                             nls16=0;
                                          continue;
# endif

					default:
						warning(INVALREQ, p);
						continue;
						}	/* end of switch after seeing '%' */
				case ' ': case '\t':		/* must be code */
					lgate();
					fprintf(fout, "%s\n",p);
					continue;
				default:		/* definition */
					while(*p && !space(*p)) p++;
					if(*p == 0)
						continue;
					prev = *p;
					*p = 0;
					bptr = p+1;
					yylval.cp = buf;
					if(digit(buf[0]))
						warning(NOLDIGITS);
					return(freturn(STR));
					}
				}
			/* still sect 1, but prev != '\n' */
			else {
				p = bptr;
				
                                while(*p && space(*p)) p++;
				if(*p == 0)
					warning(NOTRANS);
				scopy(p,token);
				yylval.cp = token;
				prev = '\n';
				return(freturn(STR));
				}
			}
		/* end of section one processing */
		}
	else if(sect == RULESECTION){		/* rules and actions */
		while(!eof){
#ifdef NLS16
			if (nls16)
				prevprev = prev;  /*used with start conditions*/
#endif
			c=gch();
			switch(c)
			{
			case '\0':
				return(freturn(0));
			case '\n':
				if(prev == '\n') continue;
				x = NEWE;
				break;
			case ' ':
			case '\t':
				if(sectbegin == TRUE){
					cpyact();
					/* dts CLLca01935 not all C code was */
					/* being copied into the lex.yy.c file*/
					/* jbc 10/30/91 */
					while((c=gch()) && c != '\n')
						fprintf(fout,"%c",c);
					fprintf(fout,"\n");
					continue;
					}
				if(!funcflag)phead2();
				funcflag = TRUE;
				if(ratfor)fprintf(fout,"%d\n",30000+casecount);
				else fprintf(fout,"case %d:\n",casecount);
				if(cpyact()){
					if(ratfor)fprintf(fout,"goto 30997\n");
					else fprintf(fout,"break;\n");
					}
#ifdef NLS16
				if (nls16) {
					while((c=gch()) && c != '\n')
						if (TwoByte(c,peek))
							c=gch();
					}
				else
#endif
				while((c=gch()) && c != '\n');
				if(peek == ' ' || peek == '\t' || sectbegin == TRUE){
					warning(EXSTATES);
					continue;
					}
				x = NEWE;
				break;
			case '%':
				if(prev != '\n') goto character;
				if(peek == '{'){	/* included code */
					getl(buf);
					while(!eof && getl(buf) && scomp("%}",buf) != 0)
						fprintf(fout,"%s\n",buf);
					continue;
					}
				if(peek == '%'){
					c = gch();
					c = gch();
					x = DELIM;
					break;
					}
				goto character;
			case '|':
				if(peek == ' ' || peek == '\t' || peek == '\n'){
					if(ratfor)fprintf(fout,"%d\n",30000+casecount++);
					else fprintf(fout,"case %d:\n",casecount++);
					continue;
					}
				x = '|';
				break;
			case '$':
				if(peek == '\n' || peek == ' ' || peek == '\t' || peek == '|' || peek == '/'){
					x = c;
					break;
					}
				goto character;
			case '^':
				if(prev != '\n' && scon != TRUE) goto character;	/* valid only at line begin */
				x = c;
				break;
			case '?':
			case '+':
			case '.':
			case '*':
			case '(':
			case ')':
			case ',':
			case '/':
				x = c;
				break;
			case '}':
				iter = FALSE;
				x = c;
				break;
			case '{':	/* either iteration or definition */
				if(digit(c=gch())){	/* iteration */
					iter = TRUE;
				ieval:
					i = 0;
					while(digit(c)){
						token[i++] = c;
						c = gch();
						}
					token[i] = 0;
					yylval.i = siconv(token);
					munput('c',c);
					x = ITER;
					break;
					}
				else {		/* definition */
					i = 0;
#ifdef NLS16
					if (nls16)
						while(c && c!='}') {
							token[i++] = c;
                                                	if (TwoByte(c,peek))
                                                		token[i++]=gch();
                                                	c = gch();
							}
					else
#endif
					while(c && c!='}') {
						token[i++] = c;
						c = gch();
						}
					token[i] = 0;
					i = lookup(token,def);
					if(i < 0)
						warning(NODEF, token);
					else
						munput('s',subs[i]);
					continue;
					}
			case '<':		/* start condition ? */
#ifdef NLS16
				if (nls16) {
					if(prev != '\n' && !TwoByte(prevprev,prev))
						goto character;
					}
				else
#endif
				if(prev != '\n')	/* not at line begin, not start */
					goto character;
				t = slptr;
				do {
					i = 0;
					c = gch();
#ifdef NLS16				
 					if (nls16)
					   while(TwoByte(prev,c) || (c != ',' && c && c != '>'))
                                                {
                                                token[i++] = c;
                                                c = gch();
                                                }
					else 
					   while(c != ',' && c && c != '>')
						{
						token[i++] = c;
						c = gch();
						}
#else
					while(c != ',' && c && c != '>')
					   {
					   token[i++] = c;
					   c = gch();
					   }
#endif
					token[i] = 0;
					if(i == 0)
						goto character;
					i = lookup(token,sname);
					if(i < 0) {
						warning(NOSTART, token);
						continue;
						}
					if(slptr >= slist+startsize)		/* note not packed ! */
					   error(TSTART);
					*slptr++ = i+1;
					} while(c && c != '>');
				if(slptr >= slist+startsize)		/* note not packed ! */
					error(TSTART);
				*slptr++ = 0;
				/* check if previous value re-usable */
				for (xp=slist; xp<t; )
					{
					if (strcmp(xp, t)==0)
						break;
					while (*xp++);
					}
				if (xp<t)
					{
					/* re-use previous pointer to string */
					slptr=t;
					t=xp;
					}
				yylval.cp = t;
				x = SCON;
				break;
			case '"':
				i = 0;
				while((c=gch()) && c != '"' && c != '\n')
					{
					if(c == '\\') c = usescape(c=gch());
					token[i++] = c;
					if(i > TOKENSIZE){
						warning(STRLONG);
						i = TOKENSIZE-1;
						break;
						}
#ifdef NLS16
					if (nls16) {
						if (TwoByte(c,peek)){
							token[i++] = c = gch();
							if(i > TOKENSIZE){
                                                		warning(STRLONG);
                                                		i = TOKENSIZE-1;
                                                		break;
                                                		}
							}
						}
#endif
					}
				if(c == '\n') {
					yyline--;
					warning(ENDLESSTR);
					yyline++;
					}
				token[i] = 0;
				if(i == 0)x = NULLS;
				else if(i == 1){
					yylval.i = token[0];
					x = CHAR;
					}
				else {
					yylval.cp = token;
					x = STR;
					}
				break;
			case '[':
				x = CCL;
				if((c = gch()) == '^'){
					x = NCCL;
					c = gch();
					}
#ifdef NLS16
				if (nls16) {
		                   /* build new leading '[' stripped CCL and  */
		                   /* ship off the work to do_multibyte_class */
				   i=0;
                                   has_ere = multibyte = 0;
      				   while (c != ']')  {
                                      range[i++] = c;
                                      if (i >= TOKENSIZE)
                                         error(CCLOVRFLW, itos(b_,TOKENSIZE));
				      if (TwoByte(c,peek)) {
					 range[i++] = gch();
                                         multibyte = 1; /* has multibyte chars*/
                                         }
                                      else if (c=='[' &&(peek=='.'|| peek=='='||peek==':')) {
                                         has_ere  =1; /* has POSIX equiv class*/
                                         }            /* char class, etc.(ERE)*/
				      c = gch();
 				      } 
                                   range[i] = ']';
                                   if (multibyte && (x == NCCL)) {
				      /* ^ not supported- pass to non 16-bit */
                                      warning(NOCARROT);
                                      }
                                   if (multibyte && has_ere)
                                      error(NOMIXING);
                                   if ((multibyte && (x == NCCL)) ||
                                            !multibyte) { 
                                      /*non 16-bit code is faster.  Refill    */
                                      /*input buffer for use in non-NLS16 code*/
                                      while (i>0) munput('c', range[i--]);
                                      c = range[0];
                                      peek = range[1];
                                   }
                                }
                                if (nls16 && multibyte && (x == CCL)) {
                                   /* Use special NLS16 routines to build CCL */
                                   x = CCLNLS;
          			   yylval.cp = (uchar *) do_multibyte_class(range,i);
                  		   break;
				}
				else {
#endif
				for(i=1;i<NCH;i++) symbol[i] = 0;
				while(c != ']' && c){
#ifdef POSIX_ERE
                                        if ((c == '[') && (peek=='.' ||
                                                   peek=='=' || peek==':')) {
                                           /* Posix Extended R.E. element. */
                                           switch (c = gch()){
                                               case '.': 
                                                  /* POSIX Collating Symbol */
                                                  do_posix_collat_sym(symbol,x);
                                                  break;
                                               case '=': 
                                                  /* POSIX Equivalence Class */
                                                  do_posix_equiv_class(symbol);
                                                  break;
                                               case ':': 
                                                  /* POSIX Character Class */
                                                  do_posix_ccl(symbol);
                                                  break;
                                              }
                                           }/* end Posix Extended R.E. ele. */
                                        else {
#endif
					   if(c == '\\') c = usescape(c=gch());
					   symbol[c] = 1;
					   j = c;
					   if((c=gch()) == '-' && peek != ']'){		/* range specified */
						   c = gch();
						   if(c == '\\') c = usescape(c=gch());
						   k = c;
						   if(j > k) {
							   n = j;
							   j = k;
							   k = n;
							   }
						   if(!(('A' <= j && k <= 'Z') ||
						        ('a' <= j && k <= 'z') ||
						        ('0' <= j && k <= '9')))
							   warning(NOPORTCHAR);
						   for(n=j+1;n<=k;n++)
							   symbol[n] = 1;		/* implementation dependent */
						   c = gch();
						   }
#ifdef POSIX_ERE
					   }
#endif
                                        }
				/* try to pack ccl's */
				i = 0;
				for(j=0;j<NCH;j++)
					if(symbol[j])token[i++] = j;
				token[i] = 0;
				p = ccptr;
				if(optim){
					p = ccl;
					while(p <ccptr && scomp(token,p) != 0)p++;
					}
				if(p < ccptr)	/* found it */
					yylval.cp = p;
				else {
					int sl;
					yylval.cp = ccptr;
					sl = slength(token) + 1;
					if(ccptr +sl >= ccl+cclsize)
						error(MANYCLASSES);
					scopy(token,ccptr);
					ccptr += sl;
					}
				cclinter(x==CCL);
				break;
#ifdef NLS16
				} /* end of non multi-byte else clause */
#endif
			case '\\':
				c = usescape(c=gch());
			default:
			character:
				if(iter){	/* second part of an iteration */
					iter = FALSE;
					if('0' <= c && c <= '9')
						goto ieval;
					}
#ifdef NLS16
				if(alpha(peek) || TwoByte(c,peek))
#else
				if(alpha(peek))
#endif
					{ /* This is a string. */
					i = 0;
					yylval.cp = token;
					token[i++] = c;
#ifdef NLS16
					if (nls16) {
						while(alpha(peek) || TwoByte(c,peek)){
							c = token[i++] = gch();
							if (TwoByte(c,peek))
								c = token[i++] = gch();
							}
						if (i==2 && TwoByte(prev,c)) 
						 	/*special case for a single 2 byte "char" */
							goto endstr;
						if(peek == '?' || peek == '*' || peek == '+') {
						 	/*push back entire 2 byte "char" */
                                                	munput('c',token[--i]);
							if (TwoByte(token[i],token[i+1])) 
                                                		munput('c',token[--i]);
							goto endstr;
 							}
						}
					else
#endif
					while(alpha(peek)) 
						token[i++] = gch();
					if(peek == '?' || peek == '*' || peek == '+')
						munput('c',token[--i]);
#ifdef NLS16
				endstr:
#endif
					token[i] = 0;
					if(i == 1){
						yylval.i = token[0];
						x = CHAR;
						}
					else x = STR;
					}
				else {
					yylval.i = c;
					x = CHAR;
					}
				}
			scon = FALSE;
			if(x == SCON)scon = TRUE;
			sectbegin = FALSE;
			return(freturn(x));
			}
		}
	/* section three */
	ptail();
# ifdef DEBUG
	if(debug)
		fprintf(fout,"\n/*this comes from section three - debug */\n");
# endif
	while(getl(buf) && !eof)
		fprintf(fout,"%s\n",buf);
	return(freturn(0));
	}
/* end of yylex */
# ifdef DEBUG
freturn(i)
  int i; {
	if(yydebug) {
		printf("now return ");
		if(i < NCH) allprint(i);
		else printf("%d",i);
		printf("   yylval = ");
		switch(i){
			case STR: case CCL: case NCCL:
				strpt(yylval.cp);
				break;
			case CHAR:
				allprint(yylval.i);
				break;
			default:
				printf("%d",yylval.i);
				break;
			}
		putchar('\n');
		}
	return(i);
	}
# endif

