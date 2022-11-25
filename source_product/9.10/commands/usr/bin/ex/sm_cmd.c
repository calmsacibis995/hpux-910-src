#ifdef ED1000

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"
#include "ex_sm.h"
#include "ex_re.h"
#include "ex_vars.h"
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
extern int errno;

CHAR BUF[SM_MAXLLEN + 1];
char tmpbuf[SM_MAXLLEN + 1];	/* to read in bytes before converting to
				 * 16-bit string format
				 */
char sm_ch;			/* Tell whether it's a ctrl x or ctrl s */
char cmdline[SM_MAXLLEN + 1];
CHAR sm_linebuf[SM_MAXLLEN + 1];
CHAR sm_linebuf2[SM_MAXLLEN + 1];
char *escstr;
short sm_names['z'-'a'+2];	/* info to restore marks */
int offset;			/* what line cursor is on */
bool insmcmd;

char sm_ctrltab[32] = {  /* How to print control characters */
	' ',  '\1',  '\2',  '\3',  '\4',  ' ',    ' ',  '\7',  /* no null,enq,ack*/
	'\010','\011',   ' ','\013','\014',  ' ', '\016','\017',  /* no lf, cr */
	'\020',   ' ',   ' ','\023','\024','\025','\026','\027',  /* no dc1', dc2 */
	'\030','\031','\032','\033','\034','\035','\036','\037' };

#ifndef NONLS8	/* User messages */
# define NL_SETN	101	/* set number */
# include <msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

sm_cmdloop(sm_q_cmd)
bool sm_q_cmd;
{
	short noread;                /* Read into file ? */
	line *sm_savedot;
	int n,i,count;
	char templn[150];
	char ps[5];
	char *buf;
	char ich,ch,ch2;
	char *ptr;
	unsigned short savelflag;
	unsigned short saveiflag;
	unsigned char killchar;
	unsigned char ruptchar;
	unsigned char ctrlchar[8];
	struct stat tbuf;
	bool readytoread = 0;

	/* Turn off write permission to terminal */
	stat(ttybuf,&tbuf);
	ttymesg = tbuf.st_mode & 0777;
	n = chmod(ttybuf,0600);

	/* save the special characters (interrupt, etc).  */

	for ( i=0; i < 4;i++){
		ctrlchar[i] = tty.c_cc[i];
		tty.c_cc[i] = '\377';
	}
#ifdef TIOCLGET
	ioctl(ttyfd,TIOCGLTC,&olttyc);
#endif TIOCLGET
#ifdef TIOCLGET
	nlttyc.t_suspc  = 0377;
	nlttyc.t_dsuspc = 0377;
	nlttyc.t_rprntc = olttyc.t_rprntc;
	nlttyc.t_flushc = olttyc.t_flushc;
	nlttyc.t_werasc = olttyc.t_werasc;
	nlttyc.t_lnextc = olttyc.t_lnextc;
	ioctl(ttyfd,TIOCSLTC,&nlttyc);
#endif TIOCLGET

	insmcmd=1;

	/* get a line,look at last few chars,and decide what to do.
	 * The user cannot type more than the system input buffer size
	 * without hitting a CR.The current limit is ?1024? chars
	 */

	for(;;){
		sm_ch = '\0';
		if (!readytoread) {
			saveiflag = tty.c_iflag;
				/* Turn off xon xoff and interrupt so
				 * that we will get ^C,^S & ^Q
				 */
			savelflag = tty.c_lflag;
			tty.c_iflag &= ~IXON;
			tty.c_lflag &= ~ECHOE;
				/* prevents funny insert BS-SP-BS */
			sTTY(ttyfd);
			readytoread++;
		}
		lseek(ttyfd,0L,0);

		/* Enable soft keys in screen mode */

		write(ttyfd,"\021",1);
		lseek(ttyfd,0L,0);

		count = read(ttyfd,templn,150);
		templn[count] = '\0';

		if (count == 1){
			ch = templn[count-1];   /* last char is \n,so -1,start at 0,so -1 */
		} else{
			ch = templn[count-2];   /* last char is \n,so -1,start at 0,so -1 */
		}
		noread = 0;
		if ((count >= 3 )&& (templn[count-3] == ch ))
			noread = 1;

		/* if ((ch < ' ') && (templn[0] != '\012')) {
		 */
		switch (ch){
		case CTRL(j):
			if (sm_q_cmd != 1)
				break;
		case CTRL(c):
		case CTRL(f):
		case CTRL(p):
		case CTRL(u):
		case CTRL(q):
		case CTRL(x):
		case CTRL(s):
		case CTRL(t):
		case CTRL(z):
		case CTRL(a):
		case CTRL(o):
		case CTRL(k):

			/* tty back to the way it was, to get xon/xoff
			 * for output
			 */

			tty.c_iflag = saveiflag;
			tty.c_lflag = savelflag;
			sTTY(ttyfd);
			readytoread = 0;

			/* } */
		default:
			break;
		}
		if (sm_q_cmd == 1){
			sm_j();
			sm_insert();
			chmod(ttybuf,ttymesg);
			peekc = '\0';
			for ( i =0;i< 4;i++)
				tty.c_cc[i] = ctrlchar[i];
			sTTY(ttyfd);
			insmcmd=0;
			/* iop(); */
			outcol = destcol = 0;
			outline = destline = lines;
			return;
		}

		switch ( ch ){
		case CTRL(c):
			offset = 0;          /* get cursor pos */
			curpos();

			/* check boundaries */

			if (offset > SM_SS)	offset = SM_SS + 2;
			if (offset > 1)
				sm_savedot = offset + sm_start - 2;
			if (sm_savedot <= zero)	sm_savedot = one;
			if (sm_savedot > dol)	sm_savedot = dol;

			/* print the apporpriate prompt */

			if ( noread )	ptr = "\033L\\\021";
			else		ptr = "\033L\057\021";

			write(ttyfd,ptr,strlen(ptr));
			lseek(ttyfd,0L,0);

			cmdline[0] = '%';       /* Insert marker for trans */

			/* get a line */
			resetterm();
			count = read(ttyfd,&cmdline[1],150);
			cmdline[count] = '\n';
			cmdline[count +1] ='\0';

			/* delete it from screen */

			ptr = "\033A\033M";
			write(ttyfd,ptr,strlen(ptr));
			lseek(ttyfd,0L,0);

			/* read screen */

			if ( !noread ){
				sm_read();
				sm_insert();
			}

			/* safety.Throw away unused garbage */

			peekc = '\0';
			globp = "\0";
			input = "\0";

			sm_tbye();        /* say bye bye to sm */
			for ( i =0;i< 4;i++)
				tty.c_cc[i] = ctrlchar[i];
			sTTY(ttyfd);
#ifdef TIOCLGET
			ioctl(ttyfd,TIOCSLTC,&olttyc);
#endif TIOCLGET

			sm_trans(cmdline);  /* translate the command */
			flush();

			globp = input;      /* put it in globp */
			input = "\0";

			TSYNC();

			dot = sm_savedot;
			getline(*dot);

			oinsm = insm;
			insm = 0;
			commands(1,1);
			insm = oinsm;
			if ( dot <= zero )	dot = one ;
			TSYNC();
			CP(file,savedfile);
				/* prevents the replacement of the file
				 * variable after commands like fi
				 */
			sm_cont();	/* Back to the way it was */
			/* save the special characters (interrupt, etc).  */

			for ( i=0; i < 4;i++){
				ctrlchar[i] = tty.c_cc[i];
				tty.c_cc[i] = '\377';
			}
			sTTY(ttyfd);
#ifdef TIOCLGET
			ioctl(ttyfd,TIOCSLTC,&nlttyc);
#endif TIOCLGET

			break;
		case CTRL(f):
			if ( !noread ){       /* read */
				sm_read();
				sm_insert();
			}
			sm_nextsc();
			ptr ="\033h\033J";
			write(ttyfd,ptr,strlen(ptr));
			outline = destline = 0;
			outcol = destcol = 0;

			/* print */
			wrtsl();
			plines(sm_start,sm_end,1);
			flush();
			wrtel();

			/* center */
			down = (sm_end - sm_start)/2 + 1;
			if ( down == 0 ){
				ptr ="\033h\012";
				write(ttyfd,ptr,strlen(ptr));
			} else{
				ptr = "\033h\033&a";
				write(ttyfd,ptr,strlen(ptr));
				itoa(down,ps);
				write(ttyfd,ps,strlen(ps));
				ptr = "r0C";
				write(ttyfd,ptr,strlen(ptr));
			}
			outline = destline = 1;
			outcol = destcol = 0;
			break;
		case CTRL(p):
			if (!noread){
				sm_read();
				sm_insert();
			}
			sm_prevsc();
			ptr = "\033h\033J";
			write(ttyfd,ptr,strlen(ptr));
			outline = destline = 0;
			outcol = destcol = 0;
			wrtsl();
			plines(sm_start,sm_end,1);
			flush();
			wrtel();
			down = (sm_end - sm_start)/2 + 1;
			if ( down == 0 ){
				ptr ="\033h\012";
				write(ttyfd,ptr,strlen(ptr));
			} else{
				ptr = "\033h\033&a";
				write(ttyfd,ptr,strlen(ptr));
				itoa(down,ps);
				write(ttyfd,ps,strlen(ps));
				ptr = "r0C";
				write(ttyfd,ptr,strlen(ptr));
			}
			outcol = destcol = 0;
			outline = destline = 1;
			break;
		case CTRL(U):
		case CTRL(q):
			if (!noread){
				sm_read();
				sm_insert();
			}
			chmod(ttybuf,ttymesg);
			peekc = '\0';
			for ( i =0;i< 4;i++)
				tty.c_cc[i] = ctrlchar[i];
			sTTY(ttyfd);
			insmcmd=0;
#ifdef TIOCLGET
			ioctl(ttyfd,TIOCSLTC,&olttyc);
#endif TIOCLGET
			/*iop();*/
			outcol = destcol = 0;
			outline = destline = lines;
			return;
		case CTRL(x):
			sm_ch = 'x';
		case CTRL(s):
		case CTRL(t):
			curpos();
			if (!noread){
				sm_read();
				sm_insert();
			}
			setnewsc();
			ptr = "\033h\033J";
			write(ttyfd,ptr,strlen(ptr));
			outline = destline = 0;
			outcol = destcol = 0;
			wrtsl();
			plines(sm_start,sm_end,1);
			flush();
			wrtel();
			ptr = "\033h\012";
			write(ttyfd,ptr,strlen(ptr));
			destline = outline = 1;
			outcol = destcol = 0;
			break;
		case CTRL(z):
			sm_z();
			break;
		case CTRL(a):
			sm_a();
			break;
		case CTRL(o):
			sm_o();
			break;
		case CTRL(k):
			sm_k();
			break;
		default:
			break;
		}
	}
}


sm_nextsc()                         /* get next screen,move dot */
{
	line  *cnt;

	sm_start = ((sm_end -1 < one ) ? one : sm_end  - 1);
	cnt = sm_start + SM_SS;
	sm_end = ((cnt > dol) ? dol : cnt );
	dot = sm_start;
}

sm_prevsc()                         /* prev screen */
{
	line *cnt;

	sm_end = sm_start + 1;
	cnt = sm_end - SM_SS;
	sm_start = ((cnt < one ) ? one : cnt );
	if ( sm_start == one )
		sm_end = ((one + SM_SS > dol ) ? dol:one + SM_SS);
	dot = sm_start;
}

int linesinbuf = 0;
bool ask;

sm_read()
{
	char *buf,*p;
	CHAR sl[15], el[15];
	char markch;
	short more = 1;
	short watchout = 0;        /* begin to watch out for zero len lines */
	int zerolenlines,n;
	int i,j;
	int nsaved;
	unsigned short savelflag;

	for (j=0;j<'z'-'a'+1;j++)sm_names[j] = -1;

	savelflag = tty.c_lflag;     /* Turn echo off */
	tty.c_lflag &= ~ECHO;
	sTTY(2);

	/* Prepare tty */

	buf = "\033Z\033c\033R\033m\033X\033&a00r0C\0334\033B\033D\0335\033H";
	write(ttyfd,buf,strlen(buf));
	outcol = outline = 0;

	lseek(sm_tmp,0L,0);
	linesinbuf = 0;
	more = 1;

	/* Do stupid first line stuff */
	buf = "\033d\021";                     /* give me a line ! */
	write(ttyfd,buf,strlen(buf));
	lseek(ttyfd,0L,0);

	/* n = read(ttyfd,sm_linebuf,SM_MAXLLEN);
	 * sm_linebuf[n] = '\0';
	 */
	n = read(ttyfd, tmpbuf, SM_MAXLLEN);
	tmpbuf[n] = '\0';
	char_TO_CHAR(tmpbuf, sm_linebuf);
	/* n = STRLEN(sm_linebuf); */

	if (n < 8 )
		ask = 1;
	else{
		/* sl =">>******"; */
		buf =">>******";
		char_TO_CHAR(buf, sl);
		ask = (STRNCMP(sm_linebuf,sl,8));
	}
	if (ask) {
		buf = "\033h";
		write(ttyfd,buf,strlen(buf));
		outcol = outline = 0;
	} else{
		buf = "\n";
		write(ttyfd,buf,strlen(buf));
	};
	lseek(ttyfd,0L,2);

	/* Read the screen and put it into buffer */
	/* el = ">>------"; */
	buf = ">>------";
	char_TO_CHAR(buf, el);

	zerolenlines = 0;
	while (more){
		buf = "\033d\021";
		write(ttyfd,buf,strlen(buf));
		lseek(ttyfd,0L,0);

		/* nsaved= n = read(ttyfd,sm_linebuf,SM_MAXLLEN);
		 * sm_linebuf[n] = '\0';
		 */
		nsaved = n = read(ttyfd, tmpbuf, SM_MAXLLEN);
		tmpbuf[n] = '\0';

		char_TO_CHAR(tmpbuf, sm_linebuf);
		/* nsaved = n = STRLEN(sm_linebuf); */
		
		if ( n >=79 &&
		    sm_linebuf[n-3] == '.' &&
		    sm_linebuf[n-2] == '.')
		{
			lseek(ttyfd,0L,2);
			buf = "\n\033d\021";
			write(ttyfd,buf,strlen(buf));
			lseek(ttyfd,0L,0);

			/* n = read(ttyfd,sm_linebuf2,SM_MAXLLEN);
			 */
			n = read(ttyfd, tmpbuf, SM_MAXLLEN);
			tmpbuf[n] = '\0';
			char_TO_CHAR(tmpbuf, sm_linebuf2);

			if (n >= 79 && sm_linebuf2[n-3] == ':' ) {
				markch = sm_linebuf2[n-2];
				sm_names[markch - 'a'] = linesinbuf;
			}
			if ( n > 70 )	n = 70;
			sm_linebuf2[n] = '\0';
			nsaved = nsaved - 4;
			/* while (sm_linebuf[nsaved]  == ' ' ||
			 * 	  sm_linebuf[nsaved] == '\t')
			 * 	nsaved--;
			 */
			nsaved++;
			STRCPY(&sm_linebuf[nsaved],sm_linebuf2);

		}
		else if ( n >=79 && sm_linebuf[n-3] == ':' )
		{
			sm_linebuf[n-3] = '\0';
			markch = sm_linebuf[n-2];
			sm_linebuf[n-2] = '\0';
			sm_names[markch - 'a'] = linesinbuf;
			j = n-5;
			while ( j >= 0 && sm_linebuf[j] == ' ')
				sm_linebuf[j--] = '\0';

		}
		else{
			j = n-2;
			while ( j >= 0 && sm_linebuf[j] == ' ')
				sm_linebuf[j--] = '\0';

		}
		if ( sm_linebuf[0] == '\012' )	watchout = 1;
		else watchout = 0 ;
		if ( watchout )
			zerolenlines ++;
		else 
			zerolenlines = 0;
		linesinbuf++;

		CHAR_TO_char(sm_linebuf, tmpbuf);

		write(sm_tmp,tmpbuf,SM_MAXLLEN);
		lseek(ttyfd,0L,2);

		if ((n == -1) || watchout &&
		    (zerolenlines >= MAX(24,sm_span())) || 
		    (!STRNCMP(sm_linebuf,el,8)) )
			more = 0;
		if ( more ) {
			buf = "\n";
			write(ttyfd,buf,strlen(buf));
		}
	}

	if (!STRNCMP(sm_linebuf,el,8))
		linesinbuf --;
	else
		ask = 1;
	linesinbuf = linesinbuf - zerolenlines;
	buf ="\033b";
	write(ttyfd,buf,strlen(buf));
	tty.c_lflag = savelflag;
	sTTY(2);
}

int textptr;
sm_insert()
{
	char c,*buf,cc;
	short saveio;
	int i,dummy;
	line *adr,*initial,nsm_end;

	c = 's';
	/* Insert text into file . */
	while ( ask ) {
		buf = "\n";
		write(ttyfd,buf,strlen(buf));
		buf = "O saves original text written to screen\r\n";
		write(ttyfd,nl_msg(1, buf),strlen(nl_msg(1, buf)));
		buf = "S saves text just read from screen\r\n";
		write(ttyfd,nl_msg(2, buf),strlen(nl_msg(2, buf)));
		buf = "B saves both (Inserts screen text before original text)\r\n";
		write(ttyfd,nl_msg(3, buf),strlen(nl_msg(3, buf)));
		buf = "What should be saved ?";
		write(ttyfd,nl_msg(4, buf),strlen(nl_msg(4, buf)));

		/* flush(); */

		peekc = '\0';
		dummy = 0;
		while ((cc = getchar()) != '\n'){
			if (!dummy){
				dummy = 1;
				c = cc;
			}
		}
		if ((c == 'O') || (c == 'S') || (c == 'B') ||
		    (c == 'o') || (c == 's') || (c == 'b'))
			ask = 0;
	}

	switch (c){
	case  'O' :
	case  'o' :
		/* do nothing */
		break;
	case  'S' :
	case  's':
		lseek(sm_tmp,0L,0);
		textptr = 0;
		/* setdot();
		 * nonzero();
		 */
		inglobal = 1;	/* make these changes global so 
				 * that they are un-undoable
				 */
		/* sorry for the double negative */
		/* first add current screen after the original,
		 * then delete the original
		 */
		vmacchng(0);
		deletenone();
		addr2 = addr1 = sm_end;
		inappend =1;
		ignore(append(sm_get,addr2));
		nsm_end = dot - sm_end -1 + sm_start;
		initial = sm_start;
		inappend =0;
		if ( dot == zero && dol > zero )
			dot = one;
		addr1 = sm_start;    /* delete old text */
		addr2 = sm_end;
		if (!(addr1 == addr2 && addr2 == zero)){
			vmacchng(0);
			delete(0);
			appendnone();
		}
		/* readjust marks */
		for ( i= 0; i < 'z' -'a' +1; i++)
			if (sm_names[i] != -1 ){
				adr = sm_names[i] + initial;
				names[i] = *adr &~ 01;
			};
		sm_end = nsm_end;
		inglobal = 0;
		linebp = '\0';
		break;
	case 'B':
	case 'b':
		inglobal = 1;
		lseek(sm_tmp,0L,0);
		textptr = 0;
		addr2 = addr1 = sm_start-1;
		if ( addr1 < one )	addr2 = addr1 = zero;
		inappend =1;
		ignore(append(sm_get,addr2));
		inappend =0;
		if ( dot == zero && dol > zero )
			dot = one;
		linebp ='\0';
		inglobal = 0;
		break;
	default:
		break;
	}

}

int sm_get()
{
	int i;
	int n;

	/* From text buffer to vi's linebuf */
	if ( linesinbuf > 0 ){

		/* n = read(sm_tmp,sm_linebuf,SM_MAXLLEN);
		 * sm_linebuf[n] = '\0';
		 */
		n = read(sm_tmp, tmpbuf, SM_MAXLLEN);
		tmpbuf[n] = '\0';
		char_TO_CHAR(tmpbuf, sm_linebuf);
		/* n = STRLEN(sm_linebuf); */

#ifdef NLS16
		STRCPY(linebuf, sm_linebuf);
#else
		CP(linebuf,sm_linebuf);
#endif

		textptr++;
		linesinbuf --;
		return (0);
	} else
		return(EOF);
}

curpos()
{                             /* get cursor position by esc a */
	char result[15];
	char c;
	char *p;
	int i;
	unsigned short savelflag;

	/* Turn off echo so that we won't get the cursor pos echoed to terminal */

	savelflag = tty.c_lflag;
	tty.c_lflag &= ~ECHO;
	sTTY(ttyfd);

	escstr = "\033a\021";
	write(ttyfd,escstr,strlen(escstr));
	lseek(ttyfd,0L,0);
	read(ttyfd,result,15);
	tty.c_lflag = savelflag;
	sTTY(ttyfd);

	/* Convert to int */
	i = 0;
	while ( result[i] != 'c')	i++;
	offset = atoi(&result[i+1]);
	/* printf("%d",offset); */
}

setnewsc()
{
	int ss;

	/* update screen stuff */
	/* If cmd is ctrl x, use maxium screen size */
	/* Don't forget that the first text line is at 2 */

	if ( (ss_max > SM_SS) && (sm_ch == 'x') )
		ss = ss_max -1;
	else	ss = SM_SS;

	if ( offset > ss ) offset = ss + 2 ;
	if ( offset > 1 ) sm_start = offset + sm_start - 2;
	if ( sm_start < one ) sm_start = one;
	if ( sm_start > dol ) sm_start = dol;
	sm_end = sm_start + ss;
	if (sm_end > dol )	sm_end = dol;
	dot = sm_start;
}

sm_z()
{
	unsigned saveflag;
	int x;
	int n;
	int i;
	/* move to end of current line */

	/* turn off echo */
	saveflag = tty.c_lflag;
	tty.c_lflag &= ~ECHO;
	sTTY(ttyfd);

	/* read line in */
	/* go back to previous line. Remember that we type
	 * ctrl z <cr>, not just ctrl z
	 */

	escstr = "\033A\033&a0C\033d\021";
	write(ttyfd,escstr,strlen(escstr));
	lseek(ttyfd,0L,0);
	/* n = read(ttyfd,sm_linebuf,SM_MAXLLEN);
	 */
	n = read(ttyfd, tmpbuf, SM_MAXLLEN);
	tmpbuf[n] = '\0';
	char_TO_CHAR(tmpbuf, sm_linebuf);

	/* Move to col n */
	escstr= "\033&a0C";
	write(ttyfd,escstr,strlen(escstr));
	escstr = "\033C";
	for ( i= 0; i< n-1 ;i++)
		write(ttyfd,escstr,strlen(escstr));
	tty.c_lflag = saveflag;
	sTTY(ttyfd);
}

sm_a()
{
	/* ctrl a goto  start of line */

	unsigned saveflag;
	int x;
	int n;
	int i;
	/* move to start of current line */

	/* turn off echo */
	saveflag = tty.c_lflag;
	tty.c_lflag &= ~ECHO;
	sTTY(ttyfd);


	/* read line in */
	/* go back to pervious line. Remember that we type 
	 * ctrl a <cr>,not just ctrl a
	 */

	escstr = "\033A\033&a0C\033d\021";
	write(ttyfd,escstr,strlen(escstr));
	lseek(ttyfd,0L,0);
	/* n = read(ttyfd,sm_linebuf,SM_MAXLLEN);
	 */
	n = read(ttyfd, tmpbuf, SM_MAXLLEN);
	tmpbuf[n] = '\0';
	char_TO_CHAR(tmpbuf, sm_linebuf);

	/* Move to col n */
	escstr= "\033&a0C";
	write(ttyfd,escstr,strlen(escstr));
	escstr = "\033C";
	i = 0;
	while ( sm_linebuf[i++] == ' ' )
		write(ttyfd,escstr,strlen(escstr));
	tty.c_lflag = saveflag;
	sTTY(ttyfd);
}

sm_o()
{
	/* Copies PL */
	unsigned saveflag;
	int x;
	int n;
	bool join;
	int i;

	/* read line in */
	/* go back to pervious line. Remember that we type
	 * ctrl o <cr>,not just ctrl o
	 */
	join = 0;

	/* turn off echo */
	saveflag = tty.c_lflag;
	tty.c_lflag &= ~ECHO;
	sTTY(ttyfd);

	escstr = "\033A\033&a0C\033d\021";
	write(ttyfd,escstr,strlen(escstr));
	lseek(ttyfd,0L,0);

	/* n = read(ttyfd,sm_linebuf,SM_MAXLLEN);
	 * sm_linebuf[n] = '\0';
	 */
	n = read(ttyfd, tmpbuf, SM_MAXLLEN);
	tmpbuf[n] = '\0';
	char_TO_CHAR(tmpbuf, sm_linebuf);

	/* escstr = "\033L";
	 * write(ttyfd,escstr,strlen(escstr));
	 * write(ttyfd,sm_linebuf,n);
	 */
	if (sm_linebuf[78] == '.' && sm_linebuf[79] == '.' && n >= 79) {
		join = 1;
		lseek(ttyfd,0L,2);
		escstr = "\n\033d\021";
		write(ttyfd,escstr,strlen(escstr));
		lseek(ttyfd,0L,0);
		/*
		 * n = read(ttyfd,sm_linebuf2,SM_MAXLLEN);
		 * sm_linebuf2[n] = '\0';
		 */
		n = read(ttyfd, tmpbuf, SM_MAXLLEN);
		tmpbuf[n] = '\0';
		char_TO_CHAR(tmpbuf, sm_linebuf2);
	}
	lseek(ttyfd,0L,0);

	if (join)	escstr = "\n\033L" ;
	else		escstr = "\033L";

	write(ttyfd,escstr,strlen(escstr));
	sendtoterm(sm_linebuf);
	if ( join ){
		escstr = "\033A\033L";
		write(ttyfd,escstr,strlen(escstr));
		sendtoterm(sm_linebuf2);
	}

	tty.c_lflag = saveflag;
	sTTY(ttyfd);
}

sm_j()
{
	char *buf,*p,*sl,*el;
	char markch;
	short more = 1;
	short watchout = 0;        /* begin to watch out for zero len lines */
	int zerolenlines,n;
	int i,j;
	int nsaved;
	unsigned short savelflag;

	for (j=0;j<'z'-'a'+1;j++)	sm_names[j] = -1;

	savelflag = tty.c_lflag;     /* Turn echo off */
	tty.c_lflag &= ~ECHO;
	sTTY(2);

	/* Prepare tty */

	outcol = outline = 0;

	lseek(sm_tmp,0L,0);
	linesinbuf = 0;
	more = 1;

	lseek(ttyfd,0L,2);

	/* Read the screen and put it into buffer */

	zerolenlines = 0;
	while (more){
		buf = "\033A\033d\021";
		/* buf = "\033A\033&a0C\033d\021";*/
		write(ttyfd,buf,strlen(buf));
		lseek(ttyfd,0L,0);

		/* nsaved= n = read(ttyfd,sm_linebuf,SM_MAXLLEN);
		 * sm_linebuf[n] = '\0';
		 */
		nsaved = n = read(ttyfd, tmpbuf, SM_MAXLLEN);
		tmpbuf[n] = '\0';
		char_TO_CHAR(tmpbuf, sm_linebuf);

		if (n >=79 && sm_linebuf[n-3] == '.' && sm_linebuf[n-2] == '.') {
			lseek(ttyfd,0L,2);
			buf = "\n\033d\021";
			write(ttyfd,buf,strlen(buf));
			lseek(ttyfd,0L,0);

			/* n = read(ttyfd,sm_linebuf2,SM_MAXLLEN);
			 */
			n = read(ttyfd, tmpbuf, SM_MAXLLEN);
			tmpbuf[n] = '\0';
			char_TO_CHAR(tmpbuf, sm_linebuf2);

			if (n >= 79 && sm_linebuf2[n-3] == ':' ){
				markch = sm_linebuf2[n-2];
				sm_names[markch - 'a'] = linesinbuf;
			}
			if ( n > 70 ) n = 70;
			sm_linebuf2[n] = '\0';
			nsaved = nsaved - 4;
			/* while (sm_linebuf[nsaved]  == ' ' ||
			 * 	  sm_linebuf[nsaved] == '\t') nsaved --;
			 */
			nsaved ++;
			STRCPY(&sm_linebuf[nsaved],sm_linebuf2);
			
		}
		else if ( n >=79 && sm_linebuf[n-3] == ':' ) {
			sm_linebuf[n-3] = '\0';
			markch = sm_linebuf[n-2];
			sm_linebuf[n-2] = '\0';
			sm_names[markch - 'a'] = linesinbuf;
			j = n-5;
			while ( j >= 0 && sm_linebuf[j] == ' ')
				sm_linebuf[j--] = '\0';
		}
		else{
			j = n-2;
			while ( j >= 0 && sm_linebuf[j] == ' ')
				sm_linebuf[j--] = '\0';
		}

		if ( sm_linebuf[0] == '\012' ) watchout = 1;
		else watchout = 0 ;
		if ( watchout )
			zerolenlines ++;
		else 
			zerolenlines = 0;
		linesinbuf ++;

		CHAR_TO_char(sm_linebuf, tmpbuf);
		write(sm_tmp,tmpbuf,SM_MAXLLEN);

		lseek(ttyfd,0L,2);
		more = 0;
		/*dhb
		 * if ( (n == -1) || watchout &&
		 * 	(zerolenlines >= MAX(24,sm_span())) || 
		 * 	(!strncmp(sm_linebuf,el,8)) )
		 * 	more = 0;   
		 * if ( more ) {
		 * 	buf = "\n";
		 * 	write(ttyfd,buf,strlen(buf));
		 * }
		 */
	}
	/*dhb
	 * if (!strncmp(sm_linebuf,el,8)) linesinbuf --; 
	 * else ask = 1;
	 */
	linesinbuf = linesinbuf - zerolenlines;
	buf ="\033b";
	write(ttyfd,buf,strlen(buf));
	tty.c_lflag = savelflag;
	sTTY(2);
}

sendtoterm(linebuf)
CHAR linebuf[];
{
	int i;
	int n;

	/*	*** NOTE ***
	 **** possibly change to STRLEN for NLS16
	 **** but note that linebuf is a local variable here
	 */

 	n = STRLEN(linebuf);
	/* CHAR_TO_char(linebuf, tmpbuf);
	 */
	for(i=n-1;i>=0;i--){
		if (value(DISPLAY_FNTS)){
			if (linebuf[i] < ' ') {
				/* (orig):
				 * strinsert2(linebuf,"\033Y",0);
				 * strinsert2(linebuf,"\033Z\033D\033P\033D\033P",n+1);
				 */
				/* strinsert2(tmpbuf, "\033Y", 0);
				 * strinsert2(tmpbuf, "\033Z\033D\033P\033D\033P", n+1);
				 */
				char_TO_CHAR("\033Y", BUF);
				STRINSERT2(linebuf, BUF, 0);

				char_TO_CHAR("\033Z\033D\033P\033D\033P", BUF);
				STRINSERT2(linebuf, BUF, n+1);

				/* char_TO_CHAR(tmpbuf, linebuf);
				 */
				break;
			}
		}
	}
	/* (orig):
	 * write(ttyfd,linebuf,strlen(linebuf));
	 */
	/* write(ttyfd, tmpbuf, strlen(tmpbuf));
	 * char_TO_CHAR(tmpbuf, linebuf);
	 */
	CHAR_TO_char(linebuf, tmpbuf);
	write(ttyfd, tmpbuf, strlen(tmpbuf));
}

iop()
{

	ioctl(2,TCGETA,&tty);
	printf("%o\n",tty.c_iflag);
	printf("%o\n",tty.c_oflag);
	printf("%o\n",tty.c_cflag);
	printf("%o\n",tty.c_lflag);
}

int sm_span()
{
	return(sm_end - sm_start + 1);
}

sm_tbye()
{
	/* Tempoary say goodbye to screen mode */

	char *escstr;
	escstr = "\033Z\033m\033R\033&a0C\0334\033B\033D\0335\033F\033J\033S\033b";
	write(ttyfd,escstr,strlen(escstr));
	destcol = outcol = 0;
	/*printf("BEFORE BYE : %d\n",ttyfd);*/
}

sm_cont()
{
	int n;
	char ps[5];

	/* Start where we left off */
	/* Restart tmp's if they have already been clean up. Normally
	 * this should not be the case
	 */

	if (!sm_clean){
		addr1 = addr2 = 0;
		sm_init();
		return;
	}
	if ( chdot ) {
		addr1 = 0;
		addr2 = 0;      /* should start at new place */
		sm_setsc();
		chdot = 0;
	} else {
		down = dot - sm_start;
		/* if (down > sm_span()) down = 0; */

		if (dot != sm_start) {
		/* if command changes the current line, make that
		 * line the top of screen. This is especially necessary
		 * for line commands after ctrl-c in screen mode.
		 */
			down = 0;
			sm_start = dot;
			sm_end = dot + SM_SS;
		}
	}

	/* No numbering and list mode in screen mode */
	value(LIST) = value(NUMBER) = 0;
	setnumb(value(NUMBER));
	setlist(value(LIST));

	/* screen pointers initialization */
	ss_size = SM_SS;
	/* Initialize terminal */
	if ( sm_end >= dol )	sm_end = dol;
	if ( sm_start < zero )	sm_start = one;


	/* Turn off display function, lock keyboard, formatt off,
	 * memory lock off,insert off
	 */

	escstr = "\033Z\033c\033X\033m\033R";
	n = write(ttyfd,escstr,strlen(escstr));
	/*
	 * if (n <= 0 ) {
	 * 	printf("write error %d,%d\n",ttyfd,n);
	 * 	printf("%d",errno);
	 * 	flush();
	 * }
	 */

	/* 1st Row & Col, set left margin, home up, clear display,
	 * cursor down, cursor left,set right margin,home up, memory
	 * lock
	 */

	escstr ="\033&a0r0C\0334\033H\033J\n\033D\0335\033H\033M\033l";
	write(ttyfd,escstr,strlen(escstr));
	outline = outcol = 0;                     /* make vi happy */
	destline = destcol = 0;

	/* Dump screen */

	/* Write out dummy 1st line */
	wrtsl();
	if ( dol > zero ){
		plines(sm_start,sm_end,1);
		flush();
	}
	wrtel();

	/* Unlock keyboard */
	if ( down <= 0 ){
		escstr = "\033h\012\033b";
		write(ttyfd,escstr,strlen(escstr));
	} else{
		itoa(down,ps);
		escstr = "\033h\033&a";
		write(ttyfd,escstr,strlen(escstr));
		write(ttyfd,ps,strlen(ps));
		escstr = "r0C\033b";
		write(ttyfd,escstr,strlen(escstr));
	}


	destcol = outcol = 0;
	destline = outline = 1;

}

sm_help()
{

/*  This is a sample help screen that is being emulated.
 EDIT/1000  REV.2440 850122
 ------------------commands--------------------------  ----line specs----
 A  abort          Kx set mark        T  tabs            n line number
 B  top & find     L  list            TI time            + forward n
 BK blank kill     LE line length     TR transfer        - backward n
 C  P then +1      LI lines in file   U  uncond. X       ^ backward n
 CO copy lines     M  merge           UN undo            . current line
 D  del to match   MO move            UY undo list yank  * line spec 1
 EC create, exit   N  line number     W  list window     $ last line
 ER replace, exit  O  copy pl         WC write, create   > last line
 F  find           P  edit pl         WR write, replace  ' forward find
 FC file close     Q  line edit       X  exchange        ` backward find
 FI file input     R  replace line    Y  X then F        :x marked line
 G  no list X      RU run program     #  sequence
 H  help           S  screen edit     _  repeat         --special chars--
 HL header line    SC screen copy     ?  help            @ indefinite
 I  insert line    SE set options     /  command stack   ^ anchor
 J  join lines     SH show options  <space> append line  \ escape
 K  kill lines     SZ file size                          | cmd separator
   ?,command  for help on a command      ?,PA  find pattern information
   ?,EX  abbreviation information        ?,PL  pending line edits
   ?,RE  regular expression information  ?,LS  line specifications
   ?,RM  recover mode explanation        ?,AB  abort messages
*/

	int index0,index1,index2,index3,index4,maxlines;
	int column1_3, column1_3l, column4;
	static char *column_last[]={
		"n line number",
		"+ forward n",
		"- backward n",
		"^ backward n",
		". current line",
		"* line spec 1",
		"$ last line",
		"' forward find",
		"` backward find",
		":x marked line",
		"",
		"--special chars--",
		"* indefinite",
		"^ anchor",
		"\\ escape",
		"| cmd separator",
		"               "
	};
	static char *msg_index[]={
		"A  abort           ",
		"B  top & find      ",
		"BK blank kill      ",
		"CO copy lines      ",
		"EC create, exit    ",
		"ER replace, exit   ",
		"F  find            ",
		"FI file input      ",
	/*      "G  no list X       ", */
		"G  same as X       ",
		"H  help            ",
		"I  insert line     ",
		"J  join lines      ",
		"K  kill lines      ",
		"Kx set mark        ",
		"L  list            ",
		"M  merge           ",
		"MO move            ",
		"N  line number     ",
		"Q  line edit       ",
		"RU run program     ",
		"S  screen edit     ",
		"SE set options     ",
		"SH show options    ",
		"TR transfer        ",
		"UN undo            ",
		"WC write, create   ",
		"WR write, replace  ",
		"X  exchange        ",
		"_  repeat          ",
		"?  help            ",
		"/  command stack   ",
		"<space> append line",
		"                   ",
		"                   ",
		"                   "
	};
/* These commands are the ones removed from the help list.
 * Add them back in (remove the comments) as needed.
 */
	/*      "T  tabs            ",*/
	/*      "TI time            ",*/
	/*      "LE line length     ",*/
	/*      "C  P then +1       ",*/
	/*      "LI lines in file   ",*/
	/*      "U  uncond. X       ",*/
	/*      "D  del to match    ",*/
	/*      "UY undo list yank  ",*/
	/*      "W  list window     ",*/
	/*      "O  copy pl         ",*/
	/*      "P  edit pl         ",*/
	/*      "FC file close      ",*/
	/*      "R  replace line    ",*/
	/*      "Y  X then F        ",*/
	/*      "#  sequence        ",*/
	/*      "HL header line     ",*/
	/*      "SC screen copy     ",*/
	/*      "SZ file size       ",*/
	/*      "> last line",*/
	/*      "@ indefinite",*/

	if (help_check() == 1)
		return;

	column4    = (sizeof(column_last)/4) - 1;
	index3     = sizeof(msg_index)/4;
	column1_3  = (index3 / 3);
	column1_3l = index3 - 1;
	if ((index3 % 3) == 0){
		column1_3--;
	}
	if (column1_3 > column4)
		maxlines = column1_3;
	else
		maxlines = column4;
	if ((index3 % 3) == 0){
		maxlines--;
	}
	printf(" ed1000");
	printf("\n");
	printf((nl_msg(1, " --------------------commands-------------------------------  ------------------")));
	printf("\n");
	index0 = 0;
	while (index0 < maxlines){
		if (index0 < column1_3){
			index1 = index0;
			index2 = index1 + column1_3;
			index3 = index2 + column1_3;
		} else{
			index1 = column1_3l;
			index2 = column1_3l;
			index3 = column1_3l;
		}
		if (index0 > column4){
			index4 = column4 + 1;
		} else{
			index4 = index0;
		}
		printf((nl_msg(2, " %s %s %s  %s\n")),
			msg_index[index1], msg_index[index2],
			msg_index[index3], column_last[index4]);
		index0++;
	}
	flush();
/* These commands were also removed from the table

   ?,command  for help on a command      ?,PA  find pattern information
   ?,EX  abbreviation information        ?,PL  pending line edits
   ?,RE  regular expression information  ?,LS  line specifications
   ?,RM  recover mode explanation        ?,AB  abort messages

*/
}

sm_k()
{
	escstr = "\033A\033&a78C:";
	write(ttyfd,escstr,strlen(escstr));
}

char key[3];  /* parsed 2 character key. */
char *keyptr;  /* parsed 2 character key pointer. */
int  keyfg;  /* a key was found. */
short tabch;
int temp,temp1,c;
var int ss_max;

rte_show()
{
	int i;

	c = getcd(); /* get past the h */
	pastwh();
	keyptr = key;
	if ((c = getcd()) != '\n'){
		*keyptr++ = c;
		if ((c = getcd()) != '\n'){
			*keyptr++ = c;
			while ((c = getcd()) != '\n'){
				if (c == 0){
					break;
				}
			}
		} else{
			*keyptr++ = ' ';
		}
	} else{
		keyptr = key;
		*keyptr++ = 'a';
		*keyptr++ = 'l';
	}
	*keyptr = (char) 0;
	keyptr = key;

	keyfg = (0 == 1);

	if (keyt3("ma")){
		lstmk();
		return;
	}

	/*
	 * if (keyt3("un")){
	 * printf(" NI    ");
	 * shwun;
	 * return;
	 * }
	 */

	if (keycheck("al")){
		printf((nl_msg(3, "         Command Default Values:")));
		crlf();
	}

	if (keycheck("er") || keyt3("wr")){
		printf((nl_msg(4, "  ER or WR...................... =")));
		printf("%s",savedfile);
		crlf();
	}

/*	if (keycheck("l")){
 *                 printf("  List file .................... =");
 *                 printf("       ");
 *                 crlf();
 *         }
 * 
 *         if (keycheck("f ") || keyt3("b ") || keyt3("d")){
 */
/*              printf("  F, B, D or line spec pattern.. =");*/
/*
 *                 printf("  F, B or line spec pattern..... =");
 * 		printmsg(scanre.Expbuf);
 * 		crlf();
 *         }
 * 
 *         if (keycheck("x") || keyt3("u") || keyt3("y") || keyt3("g")){
 */
/*              printf("  G, U, X or Y match............ =");*/
/*
 *                 printf("  G or X match.................. =");
 * 		printmsg(subre.Expbuf);
 * 		crlf();
 */
/*              printf("  G, U, X or Y substitute....... =");*/
/*
 *                 printf("  G or X substitute............. =");
 * 		printf("%s\n",rhsbuf);
 *         }
 */
	if (keycheck("ma")){
		printf((nl_msg(5, "  Number of marks............ MA =")));
		temp1 = 0;
		temp = 'z'-'a';
		while (temp){
			if (names[temp] != 1)temp1++;
			temp--;
		}
		pnum(temp1);
		crlf();
	}

	if (keycheck("fm")){
		printf((nl_msg(6, "  File modified flag......... FM =")));
		puoff(chng);
		crlf();
	}

	if (keycheck("al")){
		printf((nl_msg(7, "      Global Option Settings:")));
		crlf();
	}

	if (keycheck("tc") || keyt3("t")){
		printf((nl_msg(8, "  Tab character.............. TC =")));
		tabch = 011;
		if(tabch == 011)
			printf((nl_msg(9, "tab (cntl I)")));
		else
			printf("%o",tabch);

		crlf();
		printf((nl_msg(11, "  Tab columns................... =")));
		/* temp = value(TABS);
		 */
		temp = tabch;
		temp1 = temp;
		for (temp1 = temp; temp1 <= 80; temp1 += temp)
			pnum(temp1);
		printf((nl_msg(12, "  ...")));
		crlf();
	}

	if (keycheck("wc")){
		printf((nl_msg(13, "  Search window columns...... WC =")));
		temp = 1;
		pnum(temp);
		temp = LBSIZE;
		pnum(temp);
		crlf();
	}

	if (keycheck("sd")){
		printf((nl_msg(14, "  Screen defaults............ SD =")));
		temp = 10;
		pnum(temp);
		pnum(temp);
		temp = 2;
		pnum(temp);
		crlf();
	}

	if (keycheck("sl")){
		printf((nl_msg(15, "  Maximum screen mode lines.. SL =")));
		if(ss_max == 0)
			printf((nl_msg(16, " uninitialized")));
		else
			pnum(ss_max);
		crlf();
	}

	if (keycheck("vw")){
		printf((nl_msg(17, "  Vertical window ........... VW =")));
		temp = 10;
		pnum(temp);
		pnum(temp);
		crlf();
	}

	if (keycheck("le")){
		printf((nl_msg(18, "  Line length ............... LE =")));
		temp = LBSIZE;
		pnum(temp);
		crlf();
	}
	if (keycheck("as")){
		printf((nl_msg(19, "  Asking..................... AS =")));
		puoff(value(ASK));
		crlfp();
	}
	if (keycheck("ac")){
		printf((nl_msg(20, "Anchor character..... AC =")));
		printf("^ ");
		crlf();
	}

	if (keycheck("cf")){
		printf((nl_msg(22, "  Case folding............... CF =")));
		puoff(value(IGNORECASE));
		crlfp();
	}

	if (keycheck("ec")){
		printf((nl_msg(23, "Escape character..... EC =")));
		printf("\\ ");
		crlf();
	}

	if (keycheck("re")){
		printf((nl_msg(25, "  Regular expressions........ RE =")));
		puoff(value(MAGIC));
		crlfp();
	}

	if (keycheck("ic")){
		printf((nl_msg(26, "Indefinite character. IC =")));
		printf("* ");
		crlf();
	}

	if (keycheck("rt")){
		printf((nl_msg(28, "  Return to dot if no match.. RT =")));
		printf((nl_msg(29, "on     ")));
		crlfp();
	}


	if (keycheck("pc")){
		printf((nl_msg(30, "Prompt character..... PC =")));
		printf("/ ");
		crlf();
	}

	if (keycheck("df")){
		printf((nl_msg(32, "  Screen mode display functs. DF =")));
		puoff(value(DISPLAY_FNTS));
		crlfp();
	}

	if (keycheck("cs")){
		printf((nl_msg(33, "Command separator.... CS =")));
		printf("| ");
		crlf();
	}

	if (keycheck("ts")){
		printf((nl_msg(35, "  Time stamp <YYMMDD.HHMM>... TS =")));
		printf((nl_msg(36, "off    ")));
		crlfp();
	}

	if (keycheck("al")){
		printf((nl_msg(37, "Only AS, CF, RE and DF may be user set")));
		/* printf("SH UN shows undo list.");
		 */
		crlf();
	}

	if (!keyfg){
		/* printf("  Not an option.  Type SH to show all options and their current setting.");*/
		printf((nl_msg(38, "  Not an option.")));
		crlf();
	}
	flush();
}

pnum(i)
int i;
{
	printf("%6d",i);
}

puoff(flag)
int flag;
{
	if (flag)
		printf((nl_msg(39, "on     ")));
	else
		printf((nl_msg(40, "off    ")));
}

crlfp()
{
	if ((key[0] != 'a') || (key[1] != 'l'))
		crlf();
}

crlf()
{
	printf("\n");
}

keycheck(test)
char *test;
{
	int temp;

	if (temp = (keyt3(test)) || (keyt3("al")))
		keyfg = temp;
	return(temp);
}

keyt3(test)
char *test;
{
	int temp;
	char *temp_ptr;

	temp_ptr = key;
	if (temp = (*test++ == *temp_ptr++)){
		if (temp = (*test == *temp_ptr)){
			keyfg = temp;
		}
	}
	return(temp);
}

lstmk()
{
	int temp,index;

	if (anymarks == 0){
		printf((nl_msg(41, " no marks\n")));
	} else{
		for (index = 'a';index < 'z'+1;index++){
			temp = getmark(index);
			if (temp != 0){
				printf((nl_msg(42, "  %c %d\n")),index,lineno(temp));
			}
		}
	}
}

printmsg(list)
char *list;
{
	int c;
	int temp;
	temp = ESIZE+2;
	while (temp--){
		if ((c = *list++) == CEOFC){
			break;
		}
		printf("%c",c);
	}
}

help_check()
{
	int i,len;
	char buf1[81];  /* holds the info records. */
	char key[3];    /* parsed 2 character key. */
	char *keyptr;   /* parsed 2 character key pointer. */
	int  keyfg;     /* a key was found. */
	int c;
	int phase;
	char *helpfile;
	FILE *helpfd;

	/* c = getcd(); 
	 */
	skipit();
	keyptr = key;
	if ((c = getcd()) != '\n'){
		*keyptr++ = c;
		if ((c = getcd()) != '\n'){
			*keyptr++ = c;
			while ((c = getcd()) != '\n'){
				if (c == 0){
					break;
				}
			}
		} else{
			*keyptr++ = ' ';
		}
	} else{
		return(0);
	}
	*keyptr = '\0';
	keyptr = key;
	helpfile = "/usr/lib/rte/ed1000/ed1000.hlp";
	if ((helpfd = fopen(helpfile,"r")) == NULL){
		printf((nl_msg(43, "Error # %d occured when loading the help file\n")),errno);
		return(1);
	}
	phase = 1;
	key[0] = (char) toupper(key[0]);
	key[1] = (char) toupper(key[1]);
	while ((len = getln(helpfd,buf1)) != 0){
		if (buf1[3] == '\n')
			buf1[3] = ' ';
		switch (phase){
		case 1:
			if ((buf1[0] == '~') && (buf1[1] == '~') && (buf1[2] == key[0]) && (buf1[3] == key[1])){
				phase = 2;
			}
			break;
		case 2:
			if ((buf1[0] == '~') && (buf1[1] == '~')){
				break;
			} else{
				phase = 3;
			}
		case 3:
			if ((buf1[0] == '~') && (buf1[1] == '~')){
				flush();
				fclose(helpfd);
				return(1);
			} else{
				buf1[len] = '\0';
				printf((nl_msg(44, "  %s")),buf1);
				break;
			}
		}
	}
	if (phase = 1){
		printf((nl_msg(45, "  No help for %s.  Use ? for general help.\n")),key);
	}
	flush();
	fclose(helpfd);
	return(1);
}

getln(helpfd,buf1)
FILE *helpfd;
char buf1[];
{
	int ch,index1,index2;
	index1 = 80;
	index2 = 0;

	while (--index1 > 0 && (ch = getc(helpfd)) != EOF && ch != '\n')
		buf1[index2++] = ch;
	if (ch == '\n')
		buf1[index2++] = ch;
	buf1[index2] = '\0';
	return(index2);
}

skipit()
{
	while (iswhite(peekchar())|| (peekchar() == ',')) {
		ignchar();
	}
}
#endif ED1000
