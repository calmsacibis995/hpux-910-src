#ifdef ED1000

/* #include <stdio.h> */
#include "ex.h"
#include "ex_argv.h"
#include "ex_tty.h"
#include "ex_temp.h"
#include "ex_vis.h"
#include "ex_sm.h"

/* Initial all the screen modes global variables. 
*/
line	*sm_start;
line	*sm_end;
int	sm_tmp;
int	ttyfd;
int	ss_size;
int	ss_max;
var	diff;
bool	sm_clean;
bool	sm_q_cmd;
bool	insm;
bool	oinsm;
jmp_buf	sm_reslab;
int	down;
int	q_sm_line;
int	state_first = 0;

#ifndef NONLS8	/* User messages */
# define NL_SETN	103	/* set number */
# include <msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

sm_main(m_q_cmd)
char *m_q_cmd;
{
	/* main routine. Needed to fix a bug in dealing with entering screen
	 * mode recursively
	 */
	if (insm) return;

	if (*m_q_cmd == 'q')	/* is q instead of s command sm_q_cmd */
		sm_q_cmd = 1;	/* is true (non zero) */
	else
		sm_q_cmd = 0;

	sm_init();
	if ( setjmp(sm_reslab) == 0 ){
		/* All errors return here */
		sm_cmdloop(sm_q_cmd);

	}
	sm_bye();
}

sm_init()
{

	if ( insm ) return;
	value(LIST) = value(NUMBER) = 0;
	/* ddl: rel 1 */
	setnumb(value(NUMBER));
	setlist(value(LIST));

	insm = 1;

	/* Initialize tty name */
	if (sm_clean)
		sm_rmfile();
	ttyfileInit();
	/* Initialize temp file */
	sm_tmpinit();
	/* screen pointers initialization */
	if (state_first == 0){
		ttystate();
	}
	ss_size = SM_SS;
	sm_setsc();
	/* Initialize terminal */
	ttyinit();
}

char ttybuf[20];
ttyfileInit()
/* get tty file descriptor */
{
	char *ttystr="\0";
	if ( ttybuf[0] == '\0'){
		ttystr = ttyname(0);
		strcpy(ttybuf,ttystr);
	}
	ttyfd = open(ttybuf,2);
}

char tmpfileName[100];
sm_tmpinit()
{
	tmpnam(tmpfileName);
	sm_tmp = creat(tmpfileName,0600);
	close(sm_tmp);
	sm_tmp = open(tmpfileName,2);
	sm_clean = 1;
}

ttystate()
{
	char status[10];
	unsigned saveflag,mm;
	unsigned f264x;

	/* Code to initializes the terminal and display first screen */
	/* The escape sequences should probably be replaced by capabilities
	 * from termcap or terminfo. We'll see
	 */

	/* turn off echo the read in status byte */
	saveflag = tty.c_lflag;
	tty.c_lflag &= ~ECHO;
	sTTY(ttyfd);
	escstr = "\033^\021";
	write(ttyfd,escstr,strlen(escstr));
	lseek(ttyfd,0L,0);
	read(ttyfd,status,10);
	tty.c_lflag = saveflag;
	sTTY(ttyfd);

	mm = status[2] & 0x0F;                   /* calculate max screen size */
	ss_max = mm * 10;
	f264x = (status[5] & 0x08 ) >> 3;
	if (f264x) ss_max = ss_max - 15 ;
	if ( ss_max  > 100 ) ss_max = 100;
	state_first = 9;
}

ttyinit()
{
	char status[10];

	/* Turn off display function,lock keyboard,formatt off,memory lock off,
	 * insert off
	 */

	escstr = "\033Z\033c\033X\033m\033R";
	write(ttyfd,escstr,strlen(escstr));

	if (sm_q_cmd != 0){		/* issue for q mode */
		/* set right margin,set left margin */
		/* escstr ="\033&a80C\0335\033&a0C\0334"; */
		escstr ="";

	} else{	/* issue for s mode */
		/* 1st Row & Col,set left margin,home up,clear display,
		 * cursor down, cursor left,set right margin,home up, memory
		 * lock
		 */
		/* escstr ="\033&a0r0C\0334\033H\033J\n\033D\0335\033H\033M\033l"; */

		escstr ="\033Z\033c\033X\033m\033R\033&a0r00C\0334\033H\033J\033B\033D\0335\033H\033M\033l";
	}
	write(ttyfd,escstr,strlen(escstr));
	outline = outcol = 0;                     /* make vi happy */
	destline = destcol = 0;

	/* Dump screen */

	/* if ( clear_screen  && span() > lines ){
	 * 	flush1();
	 * 	vclear();
	 * }
	 */

	/* Write out dummy 1st line */
	wrtsl();
	if ( dol > zero ){
		plines(sm_start,sm_end,1);
		flush();
	}
	wrtel();

	/* Unlock keyboard */
	if ( down <= 0){
		if (sm_q_cmd != 0){
			escstr = "\033b";
		} else{
			escstr = "\033h\012\033b";
		}
		write(ttyfd,escstr,strlen(escstr));
	} else{
		itoa(down,status);	/* Caution : status used again for space
					 * saving
					 */
		escstr = "\033h\033&a";
		write(ttyfd,escstr,strlen(escstr));
		write(ttyfd,status,strlen(status));
		escstr = "r0C\033b";
		write(ttyfd,escstr,strlen(escstr));
	}

	destcol = outcol = 0;
	destline = outline = 1;
}

wrtsl()
{                     /*  write special start line */
	int a,b,c,d,e,f,i,g;
	char *buf,star,numstr[10],*ptr;
	line *addr;

	if (sm_q_cmd != 0){
		addr = sm_start;
		getline(*addr);

#ifdef NLS16
		ptr = linebuf + STRLEN(linebuf) - 1;
#else
		ptr = linebuf + strlen(linebuf) - 1;
#endif

		q_sm_line = 0;
		if ( column(ptr) > 79){
			q_sm_line = 1;
			buf = "Put cursor on first line when edit is complete.\n";
			write(ttyfd,nl_msg(1, buf),strlen(nl_msg(1, buf)));
		}
	} else{
		buf = ">>****** line";
		a = write(ttyfd,buf,strlen(buf));
		b = lineno(sm_start);
		itoa(b,numstr);
		g = 7 - strlen(numstr);
		buf = " ";
		for ( i= 0; i < g;i++) write(ttyfd,buf,1);
		f = write(ttyfd,numstr,strlen(numstr));
		buf = " ********* cntl U reads *** cntl U cntl U aborts ";
		c = write(ttyfd,buf,strlen(buf));
		d = columns - c - a - f-g;
		star = '*';
		buf ="<<\012";
		e = strlen(buf);
		for ( i = 0; i < d-e-1; i++)
			write(ttyfd,&star,1);
		write(ttyfd,buf,strlen(buf));
	}
	destline = outline = 1;
	destcol = outcol = 0;

}

sm_setsc()
{
	int temp1,temp2;

	temp1 = addr1;
	temp2 = addr2;
	fixzero();
	addr1 = temp1;
	addr2 = temp2;
	/* if ( dot == dol ) dot = one;
	 * setdot1();
	 */
	down = 0;

	if (sm_q_cmd != 0){   /* screen mode of one line */
		/* sm_start = sm_end = dot; */
		if (addr1 != 0 && (addr1 == addr2))
			sm_start = sm_end = addr1;
		else	sm_start = sm_end = dot;
		addr1 = addr2 = 0;
		return;
	}

	/* No addr specs given.Starting using default, ie dot -10 and
	 * dot + 10
	 */

	if ( addr1 == 0 && addr2 == 0 ){
		sm_start = dot - 10;
		if ( sm_start < one )	sm_start = one;
		sm_end = sm_start + SM_SS;
		if ( sm_end > dol )	sm_end = dol;
		down = dot - sm_start + 1;
	} else if ( addr1 == addr2 && !diff){      /* only one addr is given */
		sm_start = addr1;
		if ( sm_start < one )	sm_start = one;
		sm_end = sm_start + SM_SS;
		if ( sm_end > dol )	sm_end = dol;
		down = 1;
	} else{                                   /* both addr is given */
		if ( dol == zero )	fixzero();
		sm_start = addr1;
		if ( sm_start < one )	sm_start = one;
		sm_end = (sm_start + ss_max  > addr2 ? addr2 : sm_start + ss_max);
		if ( sm_end > dol )	sm_end = dol;
		down = 1;
	}
	addr1 = addr2 = 0;
}

wrtel()                           /* write special end line */
{
	char star,*buf,numstr[10];
	int a,b,c,d,e,f,g,h,i;
	int maxlen;	/* max file name chars allowed to be printed */

	if (sm_q_cmd != 0){
		buf = "\033A";
		write(ttyfd,buf,strlen(buf));
		destline = outline = q_sm_line;
		destcol = outcol = 0;
		if (q_sm_line != 0){
			write(ttyfd,buf,strlen(buf));
		}
	} else{
		buf = ">>------ line";
		a = write(ttyfd,buf,strlen(buf));
		if ( sm_end >= dol ){
			buf = "    EOF";
			g = write(ttyfd,nl_msg(2, buf),strlen(nl_msg(2, buf)));
			h = 0;
		} else{
			b = lineno(sm_end);
			itoa(b,numstr);
			g = strlen(numstr);
			h = 7 - g;
			buf = " ";
			for ( i = 0; i < h;i++) write(ttyfd,buf,1);
			g = write(ttyfd,numstr,strlen(numstr));
		}
		maxlen = ((strlen(file) < 52 ) ? strlen(file):52);
		c = 15;
		if (maxlen > 24)	c = c - (maxlen-24) / 2;
		write(ttyfd," ",1);
		for (i=0; i<c; i++)	write(ttyfd,"-",1);
		write(ttyfd," ",1);
		c = c + 2;
		d = write(ttyfd,file,maxlen);
		buf = " ";
		write(ttyfd,buf,1);
		e = columns - c - a - d -g-1-h;
		star = '-';
		buf ="<<\n";
		f = strlen(buf);
		for ( i = 0; i < e-f-1; i++)
			write(ttyfd,&star,1);
		write(ttyfd,buf,strlen(buf));
		destline = outline = 1;
		destcol = outcol = 0;
	}

}

sm_bye()                  /* Send esc sequences to exit sm */
{
	char *escstr;
	if (sm_q_cmd == 1) /* is q instead of s command */
		escstr = "\033Z\033m\033R\033&a0C\033B\033D\0335\033b";
	else
		escstr = "\033Z\033m\033R\033&a0C\0334\033B\033D\0335\033F\033J\033S\033b";
	write(ttyfd,escstr,strlen(escstr));
	destcol = outcol = 0;
	close(sm_tmp);
	unlink(tmpfileName);
	close(ttyfd);
	sm_clean = 0;
	insm = 0;
}

sm_rmfile()
{
	close(sm_tmp);
	unlink(tmpfileName);
	close(ttyfd);
	sm_clean = 0;
}

/* Some useful routines directly from the C book . Wonder why they aren't
 * in the standard lib
 */

itoa(n,s)
char s[];
int n;
{
	int i,sign;
	if ( (sign = n ) < 0 )
		n = -n;
	i = 0;
	do{ 
		s[i++] = n%10 + '0';
	} while( (n/=10) > 0 );
	if ( sign < 0 )
		s[i++] = '-';
	s[i] = '\0';
	str_reverse(s);
}

str_reverse(s)
char s[];
{
	int c,i,j;
	for ( i = 0, j = strlen(s) - 1; i<j;i++,j--){
		c =s[i];
		s[i] = s[j];
		s[j] = c; 
	}
}
#endif ED1000
