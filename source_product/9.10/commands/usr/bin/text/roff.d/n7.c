/* @(#) $Revision: 51.1 $ */      
#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#endif NLS
#include "tdef.hd"
#ifdef NROFF
#include "tw.hd"
extern struct ttable t;
#endif
#include "strs.hd"
#ifdef NROFF
#define GETCH gettch
#endif
#ifndef NROFF
#define GETCH getch
#endif

/*
troff7.c

text
*/

extern struct s *frame, *stk;
extern struct s *ejl;
extern struct d d[NDI], *dip;
#ifndef INCORE
extern struct envblock eblock;
#else
extern struct envblock eblock[NEV];
extern int ev;
extern int maclev;
#endif
extern struct datablock dblock;
extern struct tmpfaddr ip;

extern int Hipb;		/* cu test in gettch */
extern int ch_CMASK;
extern int dilev;
extern int pl;
extern int trap;
extern int flss;
extern int fic;		/* .mc char saved during diversions */
extern int npnflg;
extern int npn;
extern int stop;
extern int nflush;
extern int ejf;
extern int ascii;
extern int donef;
extern int dpn;
extern int ndone;
extern int pto;
extern int pfrom;
extern int print;
extern int *pnp;
extern int totout;
extern int ch;
extern int nlflg;
extern int over;
extern int nhyp;
extern int cwidth;
extern int widthp;
extern int xbitf;
extern int vflag;
extern int sfont;
extern int **hyp;
extern int nform;
extern int po;
extern int ulbit;
extern int nrbits;
int brflg;
tbreak(){
	register *i, j, pad;
	int res;
	int saveic;

	trap = 0;
	if(nb)return;
	if((dilev == 0) && (v_nl == -1) && (donef != -1)){
		newline(1);
		return;
	}
	if(!nc){
		setnel();
		if(!wch)return;
		if(pendw)getword(1);
		movword();
	}else if(pendw && !brflg){
		getword(1);
		movword();
	}
	*linep = dip->nls = 0;
#ifdef NROFF
	if(dilev == 0)horiz(po);
#endif
	if(lnmod)donum();
	lastl = ne;
	if(brflg != 1){
		totout = 0;
	}else if(ad){
		if((lastl = (ll - un)) < ne)lastl = ne;
	}
	if(admod && ad && (brflg != 2)){
		lastl = ne;
		adsp = adrem = 0;
#ifdef NROFF
		if(admod == 1)un +=  quant(nel/2,t.Adj);
#endif
#ifndef NROFF
		if(admod == 1)un += nel/2;
#endif
		else if(admod ==2)un += nel;
	}
	totout++;
	brflg = 0;
	if((lastl+un) > dip->maxl)dip->maxl = (lastl+un);
	horiz(un);
#ifdef NROFF
	res = t.Adj;
#endif
#ifdef NLS16
	/* adrem%t.Adj must be 0. But this assumption is broken  */
	/* when 16-bit code of having 3/2 width of alpha-numeric */
	/* is supported. Therefore next sentence is required.	 */
	adrem=quant(adrem,t.Adj);
#endif
	for(i = line;nc > 0;){
		if(((j = *i++) & CMASK) == ' '){
			pad = 0;
			do{
				pad += width(j);
				nc--;
			  }while(((j = *i++) & CMASK) == ' ');
			i--;
			pad += adsp;
			--nwd;
			if(adrem){
				if(adrem < 0){
#ifdef NROFF
					pad -= res;
					adrem += res;
				}else if((totout&01) ||
					((adrem/res)>=(nwd))){
					pad += res;
					adrem -= res;
#endif
#ifndef NROFF
					pad--;
					adrem++;
				}else{
					pad++;
					adrem--;
#endif
				}
			}
			horiz(pad);
		}else{
			pchar(j);
			nc--;
		}
	}
	/* If we are getting text from a diversion, the FIC code
	 * indicates that a margin character was in effect when
	 * the text went into the diversion, and we should establish
	 * the saved character as the margin character temporarily.
	 * getch() sets fic when it sees the FIC code.
	 * If we're putting text into a diversion, we just pchar1()
	 * the FIC and the margin character itself; at the outermost
	 * level, we generate the ics space and print the margin
	 * character.
	 */
	saveic = ic;
	if (fic) ic = fic;
	if(ic){
		if (dilev > 0) {
			pchar1(FIC);
			pchar1(ic);
		} else {
			if((j = ll - un - lastl + ics) > 0)horiz(j);
			pchar(ic);
		}
	}
	ic = saveic;
	fic = 0;
	if(icf)icf++;
		else ic = 0;
	ne = nwd = 0;
	un = in;
	setnel();
	newline(0);
	if(dilev > 0){if(dip->dnl > dip->hnl)dip->hnl = dip->dnl;}
	else{if(v_nl > dip->hnl)dip->hnl = v_nl;}
	for(j=ls-1; (j >0) && !trap; j--)newline(0);
	spread = 0;
}
donum(){
	register i, nw;
	extern pchar();

	nrbits = nmbits;
	nw = width('1' | nrbits);
	if(nn){
		nn--;
		goto d1;
	}
	if(v_ln%ndf){
		v_ln++;
	d1:
		un += nw*(3+nms+ni);
		return;
	}
	i = 0;
	if(v_ln<100)i++;
	if(v_ln<10)i++;
	horiz(nw*(ni+i));
	nform = 0;
	fnumb(v_ln,pchar);
	un += nw*nms;
	v_ln++;
}
text(){
	register i;
	static int spcnt;

	nflush++;
	if((dilev == 0) && (v_nl == -1) && (donef != -1)){newline(1); return;}
	setnel();
	if(ce || !fi){
		nofill();
		return;
	}
	if(pendw)goto t4;
	if(pendt)if(spcnt)goto t2; else goto t3;
	pendt++;
	if(spcnt)goto t2;
	while ((i = GETCH()) && (ch_CMASK == ' ')) spcnt++;
	if(nlflg){
	t1:
		nflush = pendt = ch = spcnt = 0;
		callsp();
		return;
	}
	ch = i;
	if(spcnt){
	t2:
		tbreak();
		if(nc || wch)goto rtn;
		un += spcnt*sps;
		spcnt = 0;
		setnel();
		if(trap)goto rtn;
		if(nlflg)goto t1;
	}
t3:
	if(spread)goto t5;
	if(pendw || !wch)
	t4:
		if(getword(0))goto t6;
	if(!movword())goto t3;
t5:
	if(nlflg)pendt = 0;
	adsp = adrem = 0;
	if(ad){
/* jfr */	if (nwd==1) adsp=nel; else adsp=nel/(nwd-1);
#ifdef NROFF
		adsp = (adsp/t.Adj)*t.Adj;
#endif
		adrem = nel - adsp*(nwd-1);
	}
	brflg = 1;
	tbreak();
	spread = 0;
	if(!trap)goto t3;
	if(!nlflg)goto rtn;
t6:
	pendt = 0;
	ckul();
rtn:
	nflush = 0;
}
nofill(){
	register i;

	if(!pendnf){
		over = 0;
		tbreak();
		if(trap)goto rtn;
		if(nlflg){
			ch = nflush = 0;
			callsp();
			return;
		}
		adsp = adrem = 0;
		nwd = 10000;
	}
	while ((i = GETCH()) && (ch_CMASK != '\n')) {
		if (ch_CMASK == ohc) continue;
		if (ch_CMASK == CONT) {
			pendnf++;
			nflush = 0;
			flushi();
			ckul();
			return;
		}
		storeline(i,-1);
	}
	if(ce){
		ce--;
		if((i=quant(nel/2,HOR)) > 0)un += i;
	}
	if(!nc)storeline(FILLER,0);
	brflg = 2;
	tbreak();
	ckul();
rtn:
	pendnf = nflush = 0;
}
callsp(){
	register i;

	if (flss) {	i = flss;
			flss = 0;	}
	    else i = lss;
	casesp(i);
}
ckul(){
	if(ul && (--ul == 0)){
			cu = 0;
			font = sfont;
			mchbits();
	}
	if(it && (--it == 0) && itmac)	{
			nflush = 0;	/* flush input */
			control(itmac,0);	}
}
storeline(c,w){
	register i;

	if((c & CMASK) == JREG){
		if((i=findr(c>>BYTE)) != -1)vlist[i] = ne;
		return;
	}
	if(linep >= (line + lnsize - 1)){
		if(over++) return;
		prstrfl((nl_msg(49, "Line overflow.\n")));
		c = 0343;
		w = -1; 	}
	if(w == -1)w = width(c);
	ne += w;
	nel -= w;
	*linep++ = c;
	nc++;
}
newline(a)
int a;
{
	register i, j, nlss;
	int opn;

	if(a)goto nl1;
	if (dilev > 0) {
		j = lss;
		pchar1(FLSS);
		if(flss)lss = flss;
		i = lss + dip->blss;
		dip->dnl += i;
		pchar1(i);
		pchar1('\n');
		lss = j;
		dip->blss = flss = 0;
		if(dip->alss){
			pchar1(FLSS);
			pchar1(dip->alss);
			pchar1('\n');
			dip->dnl += dip->alss;
			dip->alss = 0;
		}
		if(dip->ditrap && !dip->ditf &&
			(dip->dnl >= dip->ditrap) && dip->dimac)
			if(control(dip->dimac,0)){trap++; dip->ditf++;}
		return;
	}
	j = lss;
	if(flss)lss = flss;
	nlss = dip->alss + dip->blss + lss;
	v_nl += nlss;
#ifndef NROFF
	if(ascii){dip->alss = dip->blss = 0;}
#endif
	pchar1('\n');
	flss = 0;
	lss = j;
	if(v_nl < pl)goto nl2;
nl1:
	ejf = dip->hnl = v_nl = 0;
#ifndef INCORE
	ejl = frame;
#else
	ejl = (struct s *)maclev;
#endif
	if(donef){
		if((!nc && !wch) || ndone)done1(0);
		ndone++;
		donef = 0;
		if(frame == stk)nflush++;
	}
	opn = v_pn;
	v_pn++;
	if(npnflg){
		v_pn = npn;
		npn = npnflg = 0;
	}
nlpn:
	if(v_pn == pfrom){
		print++;
		pfrom = -1;
	}else if(opn == pto){
		print = 0;
		opn = -1;
		chkpn();
		goto nlpn;
		}
	if(stop && print){
		dpn++;
		if(dpn >= stop){
			dpn = 0;
			dostop();
		}
	}
nl2:
	trap = 0;
	if(v_nl == 0){
		if((j = findn(0)) != NTRAP)
			trap = control(mlist[j],0);
	} else if((i = findt(v_nl-nlss)) <= nlss){
		if((j = findn1(v_nl-nlss+i)) == NTRAP){
			prstrfl((nl_msg(50, "Trap botch.\n")));
			done2(-5);
		}
		trap = control(mlist[j],0);
	}
}
findn1(a)
int a;
{
	register i, j;

	for(i=0; i<NTRAP; i++){
		if(mlist[i]){
			if((j = nlist[i]) < 0)j += pl;
			if(j == a)break;
		}
	}
	return(i);
}
chkpn(){
	pto = *(pnp++);
	pfrom = pto & ~MOT;
	if(pto == -1){
		flusho();
		done1(0);
	}
	if(pto & MOT){
		pto &= ~MOT;
		print++;
		pfrom = 0;
	}
}
findt(a)
int a;
{
	register i, j, k;

	k = 32767;
	if (dilev > 0) {
		if(dip->dimac && ((i = dip->ditrap -a) > 0))k = i;
		return(k);
	}
	for(i=0; i<NTRAP; i++){
		if(mlist[i]){
			if((j = nlist[i]) < 0)j += pl;
			if((j -= a)  <=  0)continue;
			if(j < k)k = j;
		}
	}
	i = pl - a;
	if(k > i)k = i;
	return(k);
}
findt1(){
	register i;

	if (dilev > 0) i = dip->dnl;
		else i = v_nl;
	return(findt(i));
}
eject(a)
struct s *a;
{
	register savlss;

	if (dilev > 0) return;
	ejf++;
	if(a)ejl = a;
#ifndef INCORE
		else ejl = frame;
#else
		else ejl = (struct s *)maclev;
#endif
	if(trap)return;
	do {
	    savlss = lss;
	    lss = findt(v_nl);
	    newline(0);
	    lss = savlss;	}
	while (v_nl && !trap);
}
movword(){
	register i, w, *wp;
	int savwch, hys;

	over = 0;
	wp = wordp;
	if(!nwd){
		while(((i = *wp++) & CMASK) == ' '){
			wch--;
			wne -= width(i);
		}
		wp--;
	}
	if((wne > nel) &&
	   !hyoff && hyf &&
	   (!nwd || (nel > 3*sps)) &&
	   (!(hyf & 02) || (findt1() > lss))
	  )hyphen(wp);
	savwch = wch;
	hyp = hyptr;
	nhyp = 0;
	while(*hyp && (*hyp <= wp))hyp++;
	while(wch){
		if((hyoff != 1) && (*hyp == wp)){
			hyp++;
			if(!wdstart ||
			   ((wp > (wdstart+1)) &&
			    (wp < wdend) &&
			    (!(hyf & 04) || (wp < (wdend-1))) &&
			    (!(hyf & 010) || (wp > (wdstart+2)))
			   )
			  ){
				nhyp++;
				storeline(IMP,0);
			}
		}
		i = *wp++;
		w = width(i);
		wne -= w;
		wch--;
		storeline(i,w);
	}
	if(nel >= 0){
		nwd++;
		return(0);
	}
	xbitf = 1;
	hys = width(0200); /*hyphen*/
m1:
	if(!nhyp){
		if(!nwd) {nwd++; wordp = wp; return (1);}
		if(wch == savwch) {wordp = wp; return (1);}
	}
	if(*--linep != IMP) {
		nc--;
		w = width(*linep);
		ne -= w;
		nel += w;
		wne += w;
		wch++;
		wp--;
		goto m1;	}
	if (((--nhyp) || nwd) && (nel < hys)) {
	    nc--;
	    goto m1;	}
	if(((i = *(linep-1) & CMASK) != '-') && (i != 0203)) {
		*linep = (*(linep-1) & ~CMASK) | 0200;
		w = width(*linep);
		nel -= w;
		ne += w;
		linep++;	}
	nwd++;
	wordp = wp;
	return(1);
}
setnel(){
	if(!nc){
		linep = line;
		if(un1 >= 0){
			un = un1;
			un1 = -1;
		}
		nel = ll - un;
		ne = adsp = adrem = 0;
	}
}
getword(x)
int x;
{
	register i, j, swp;
	int noword;
#ifdef NLS16
	int prevj, *p, *q;
#endif
	noword = 0;
	if(x)if(pendw){
		*pendw = 0;
		goto rtn;
	}
	if(wordp = pendw)goto g1;
	hyp = hyptr;
	wordp = word;
	over = wne = wch = 0;
	hyoff = 0;
	while(1){
		i = GETCH();  j = ch_CMASK;
		if(j == '\n'){
			wne = wch = 0;
			noword = 1;
			goto rtn;
		}
		if(j == ohc){
			hyoff = 1;
			continue;
		}
		if(j == ' '){
			storeword(i,cwidth);
			continue;
		}
		break;
	}
	swp = widthp;
#ifdef NLS16
	if ( spflg&002 ) { /* Previous char is space */
		storeword( ' ' | chbits, -1);
	} else if(spflg&004) {	/* Previous char is '\n' */
		if ( spflg&010 && j&BMASK2ND ) { 
			/* Previous '\n' follows 16-bit code */
			/* And next code is also 16-bit one. */
			storeword( ' ' | chbits | ZBIT , -1);
		} else {
			storeword( ' ' | chbits, -1);
		}
		if(spflg&001){  /* Previous '\n' follows terminated char */
			storeword( ' ' | chbits, -1);
			if(spflg&010 && j&BMASK2ND) {
				storeword( ' ' | chbits, -1);
			}
		}
	} else {
		storeword( ' ' | chbits | ZBIT , -1);
	}
	spflg = 0;
#else
	storeword(' ' | chbits, -1);
	if(spflg){
		storeword(' ' | chbits, -1);
		spflg = 0;
	}
#endif
	widthp = swp;
g0:
	if(j == CONT){
		pendw = wordp;
		nflush = 0;
		flushi();
		return(1);
	}
	if(hyoff != 1){
		if(j == ohc){
			hyoff = 2;
			*hyp++ = wordp;
			if(hyp > (hyptr+NHYP-1))hyp = hyptr+NHYP-1;
			goto g1;
		}
		if((j == '-') ||
		   (j == 0203) /*3/4 Em dash*/
		  )if(wordp > word+1){
			hyoff = 2;
			*hyp++ = wordp + 1;
			if(hyp > (hyptr+NHYP-1))hyp = hyptr+NHYP-1;
		}
	}
	storeword(i,cwidth);
g1:
#ifdef NLS16
	prevj=j;
	i = GETCH();  j = ch_CMASK;
	if ( j==' ' ) {
		spflg |= 002;	/* Set when blank is detected. */
	} else if ( j=='\n' ) {
		spflg |= 004;	/* Set when '\n' is detected. */
		if ( prevj&BMASK2ND ) {
			spflg |= 010;	/* Set when 16-bit code is detected. */
		}
		p=wordp;
		do	{ 	/* Skip extended terminal char */
			p--;
			for(q=exbuf;!(*q=='\0'||*q==((*p)&CMASK));q++);
		} while ( *q );
		/* Search terminal char */
		for(q=tebuf;!(*q=='\0'||*q==((*p)&CMASK));q++);
		if ( *q ) {
			spflg |= 001;
		}
	} else if ( prevj==UNPAD || prevj==FILLER || prevj==OHC ) {
		goto g0;
	} else if ( j==UNPAD || j==FILLER || j==OHC ) {
		goto g0;
	} else if ( prevj&BMASK2ND || j&BMASK2ND ) {
		/* Search prefix formatting restriction char */
		for(p=prbuf;!(*p=='\0'||*p==prevj);p++);
		if (*p) {
			goto g0;
		}
		/* Search suffix restriction */
		for(q=subuf;!(*q=='\0'||*q==j);q++);
		if (*q) {
			goto g0;
		}
		ch=i;
	} else {
		goto g0;
	}
	*wordp = 0;
#else
	i = GETCH();  j = ch_CMASK;
	if(j != ' '){
		if(j != '\n')goto g0;
		j = *(wordp-1) & CMASK;
		if((j == '.') ||
		   (j == '!') ||
		   (j == '?'))spflg++;
	}
	*wordp = 0;
#endif
rtn:
	wdstart = 0;
	wordp = word;
	pendw = 0;
	*hyp++ = 0;
	setnel();
	return(noword);
}
storeword(c,w)
int c, w;
{

	if(wordp >= &word[WDSIZE - 1]){
		if(over++) return;
		prstrfl((nl_msg(51, "Word overflow.\n")));
		c = 0343;
		w = -1; 	}
	if(w == -1)w = width(c);
	wne += w;
	*wordp++ = c;
	wch++;
}
#ifdef NROFF
gettch(){
	register int i, j;

	if(!((i = getch()) & MOT) && (i & ulbit)){
		j = i & CMASK;
#ifdef NLS16
		if ( j&BMASK2ND ) { /* 16-bit code	*/
		} else if ( j&FLAG8 ) {	/* 8-bit code	*/
			j &= 0177;
			if(cu && (trtab[j+256] == ' '))
				i = ((i & ~ulbit)& ~CMASK) | '_';
		} else {
			if(cu && (trtab[j] == ' '))
				i = ((i & ~ulbit)& ~CMASK) | '_';
			if(!cu && (j>32) && (j<0370) &&
				!(*t.codetab[j-32] & 0200) &&
				((ip.b == Hipb) || (Hipb == -2)))
					i &= ~ulbit;	
		}
#else
		if(cu && (trtab[j] == ' '))
			i = ((i & ~ulbit)& ~CMASK) | '_';
		if(!cu && (j>32) && (j<0370) &&
			!(*t.codetab[j-32] & 0200) &&
			((ip.b == Hipb) || (Hipb == -2)))
				i &= ~ulbit;	
#endif
	}
	ch_CMASK = i & CMASK;
	return(i);
}
#endif
