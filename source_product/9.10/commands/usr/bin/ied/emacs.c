/* @(#) $Revision: 66.1 $ */

/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
/* Adapted for ksh by David Korn */
/* EMACS_MODES: c tabstop=4 

One line screen editor for any program


Questions and comments should be
directed to 

	Michael T. Veach
	IX 1C-341 X1614
	ihuxl!veach

*/


/*	The following is provided by:
 *
 *			Matthijs N. Melchior
 *			AT&T Network Systems International
 *			APT Nederland
 *			HV BZ335 x2962
 *			hvlpb!mmelchio
 *
 *
 *	If symbol ESHPLUS is defined, the following features is present:
 *
 *  ESH_NFIRST
 *	-  A ^N as first history related command after the prompt will move
 *	   to the next command relative to the last known history position.
 *	   It will not start at the position where the last command was entered
 *	   as is done by the ^P command.  Every history related command will
 *	   set both the current and last position.  Executing a command will
 *	   only set the current position.
 *
 *  ESH_KAPPEND
 *	-  Successive kill and delete commands will accumulate their data
 *	   in the kill buffer, by appending or prepending as appropriate.
 *	   This mode will be reset by any command not adding something to the
 *	   kill buffer.
 *
 *  ESH_BETTER
 *	-  Some enhancements:
 *		- argument for a macro is passed to its replacement
 *		- ^X^H command to find out about history position (debugging)
 *		- ^X^D command to show any debugging info
 *
 *  I do not pretend these for changes are completely independent,
 *  but you can use them to seperate features.
 */

#ifdef	DMERT	/* 3bcc #undefs RT */
#   define	RT
#endif

#   include	"defs.h"

#include	"history.h"
#include	"edit.h"


#undef blank
#undef putchar
#define putchar(fd,c)	ed_putchar(fd,c)
#define beep(fd)	ed_ringbell(fd)


#   define gencpy(a,b)	strcpy((char*)(a),(char*)(b))
#   define genncpy(a,b,n)	strncpy((char*)(a),(char*)(b),n)
#   define genlen(str)	strlen(str)
#   define print(c)	isprint(c)
#   define isword(c)	isalnum(out[c])

#define eol		editb.e_eol
#define cur		editb.e_cur
#define mark		editb.e_fchar
#define hline		editb.e_hline
#define hloff		editb.e_hloff
#define hismin		editb.e_hismin
#define usrkill		editb.e_kill
#define usreof		editb.e_eof
#define usrerase	editb.e_erase
#define crallowed	editb.e_crlf
#define llimit		editb.e_llimit
#define Prompt		editb.e_prompt
#define plen		editb.e_plen
#define kstack		editb.e_killbuf
#define lstring		editb.e_search
#define lookahead	editb.e_index
#define env		editb.e_env
#define raw		editb.e_raw
#define histlines	editb.e_hismax
#define w_size		editb.e_wsize
#define drawbuff	editb.e_inbuf
#define NO	0
#define YES	1
#define LBUF	100
#define KILLCHAR	UKILL
#define ERASECHAR	UERASE
#define EOFCHAR		UEOF

/**********************
A large lookahead helps when the user is inserting
characters in the middle of the line.
************************/


static genchar *screen;		/* pointer to window buffer */
static genchar *cursor;		/* Cursor in real screen */
static enum
{
	CRT=0,	/* Crt terminal */
	PAPER	/* Paper terminal */
} terminal ;

typedef enum
{
	FIRST,		/* First time thru for logical line, prompt on screen */
	REFRESH,	/* Redraw entire screen */
	APPEND,		/* Append char before cursor to screen */
	UPDATE,		/* Update the screen as need be */
	FINAL		/* Update screen even if pending look ahead */
} DRAWTYPE;

static void draw();
static int escape();
static void putstring();
static int search();
static void setcursor();
static void show_info();
static void xcommands();

static int cr_ok;
static	histloc location = { -5, 0 };

emacs_read(fd,buff,scend)
char *buff;
int fd;
unsigned scend;
{
	register int c;
	register int i;
	register genchar *out;
	register int count;
	int adjust,oadjust;
	char backslash;
	genchar *kptr;
static int CntrlO;
	char prompt[PRSIZE];
	char string[LBUF*CHARSIZE];
	genchar Screen[MAXWINDOW];
#if (2 * CHARSIZE * MAXLINE) < IOBSIZE
	kstack = buff + MAXLINE*sizeof(genchar);
#else
	if(kstack==0)
	{
		kstack = (genchar*)malloc(sizeof(genchar)*(MAXLINE));
		kstack[0] = '\0';
	}
#endif
	Prompt = prompt;
	lstring = string;
	screen = Screen;
	drawbuff = out = (genchar*)buff;
	if(tty_raw(fd) < 0)
	{
#ifdef NOTDEF
		 p_flush();
		 return(ee_read(fd,buff,scend));
#endif /* NOTDEF */
	}
	raw = 1;
	/* This mess in case the read system call fails */
	
	ed_setup(fd);
	if ((i=setjmp(env))!=0)
	{
		tty_cooked(fd);
		if (i == UEOF)
		{
			return(0); /* EOF */
		}
		return(-1); /* some other error */
	}
	*out = 0;
	if(scend > (MAXLINE-2))
		scend = (MAXLINE-2);
	llimit = scend;
	mark = eol = cur = 0;
	draw(fd,FIRST);
	adjust = -1;
	backslash = 0;
	if (CntrlO)
	{
		location = hist_locate(location.his_command,location.his_line,1);
		if (location.his_command < histlines)
		{
			hline = location.his_command;
			hloff = location.his_line;
			hist_copy((char*)kstack,hline,hloff);
			ed_ungetchar(cntl('Y'));
		}
	}
	CntrlO = 0;
	while ((c = ed_getchar(fd)) != (-1))
	{
		if (backslash)
		{
			backslash = 0;
			if (c==usrerase||c==usrkill||(!print(c) &&
				(c!='\r'&&c!='\n')))
			{
				/* accept a backslashed character */
				cur--;
				out[cur++] = c;
				out[eol] = '\0';
				draw(fd,APPEND);
				continue;
			}
		}
		if (c == usrkill)
		{
			c = KILLCHAR ;
		}
		else if (c == usrerase)
		{
			c = ERASECHAR ;
		} 
		else if ((c == usreof)&&(eol == 0))
		{
			c = EOFCHAR;
		}
		oadjust = count = adjust;
		if(count<0)
			count = 1;
		adjust = -1;
		i = cur;
		switch(c)
		{
		case cntl('V'):
			show_info(fd,&e_version[5]);
			continue;
		case '\0':
			mark = i;
			continue;
		case cntl('X'):
			xcommands(fd,count);
			continue;
		case EOFCHAR:
			ed_flush(fd);
			tty_cooked(fd);
			return(0);
		default:
			i = ++eol;
			if (i >= (scend)) /*  will not fit on line */
			{
				eol--;
				ed_ungetchar(c); /* save character for next line */
				goto process;
			}
			for(i=eol;i>=cur;i--)
			{
				out[i] = out[i-1];
			}
			backslash =  (c == '\\');
			out[cur++] = c;
			draw(fd,APPEND);
			continue;
		case cntl('Y') :
			{
				c = genlen(kstack);
				if ((c + eol) > scend)
				{
					beep(fd);
					continue;
				}
				mark = i;
				for(i=eol;i>=cur;i--)
					out[c+i] = out[i];
				kptr=kstack;
				while (i = *kptr++)
					out[cur++] = i;
				draw(fd,UPDATE);
				eol = genlen(out);
				continue;
			}
		case '\n':
		case '\r':
			c = '\n';
			goto process;
		case ERASECHAR :
			if (count > i)
				count = i;
			while ((count--)&&(i>0))
			{
				i--;
				eol--;
			}
			genncpy(kstack,out+i,cur-i);
			kstack[cur-i] = 0;
			gencpy(out+i,out+cur);
			mark = i;
			goto update;
		case cntl('W') :
			if (mark > eol )
				mark = eol;
			if (mark == i)
				continue;
			if (mark > i)
			{
				adjust = mark - i;
				ed_ungetchar(cntl('D'));
				continue;
			}
			adjust = i - mark;
			ed_ungetchar(ERASECHAR);
			continue;
		case cntl('D') :
			mark = i;
			kptr = kstack;
			while ((count--)&&(eol>0)&&(i<eol))
			{
				*kptr++ = out[i];
				eol--;
				while(1)
				{
					if ((out[i] = out[(i+1)])==0)
						break;
					i++;
				}
				i = cur;
			}
			*kptr = '\0';
			goto update;
		case cntl('C') :
		case cntl('F') :
		{
			int cntlC = (c==cntl('C'));
			while (count-- && eol>i)
			{
				if (cntlC)
				{
					c = out[i];
					if(islower(c))
					{
						c += 'A' - 'a';
						out[i] = c;
					}
				}
				i++;
			}
			goto update;
		}
		case cntl(']') :
			c = ed_getchar(fd);
			if ((count == 0) || (count > eol))
                        {
                                beep(fd);
                                continue;
                        }
			if (out[i])
				i++;
			while (i < eol)
			{
				if (out[i] == c && --count==0)
					goto update;
				i++;
			}
			i = 0;
			while (i < cur)
			{
				if (out[i] == c && --count==0)
					break;
				i++;
			};

update:
			cur = i;
			draw(fd,UPDATE);
			continue;

		case cntl('B') :
			if (count > i)
				count = i;
			i -= count;
			goto update;
		case cntl('T') :
			if ((is_option(GMACS))||(eol==i))
			{
				if (i >= 2)
				{
					c = out[i - 1];
					out[i-1] = out[i-2];
					out[i-2] = c;
				}
				else
				{
					beep(fd);
					continue;
				}
			}
			else
			{
				if (eol>(i+1))
				{
					c = out[i];
					out[i] = out[i+1];
					out[i+1] = c;
					i++;
				}
				else
				{
					beep(fd);
					continue;
				}
			}
			goto update;
		case cntl('A') :
			i = 0;
			goto update;
		case cntl('E') :
			i = eol;
			goto update;
		case cntl('U') :
			adjust = 4*count;
			continue;
		case KILLCHAR :
			cur = 0;
			oadjust = -1;
		case cntl('K') :
			if(oadjust >= 0)
			{
				mark = count;
				ed_ungetchar(cntl('W'));
				continue;
			}
			i = cur;
			eol = i;
			mark = i;
			gencpy(kstack,&out[i]);
			out[i] = 0;
			draw(fd,UPDATE);
			if (c == KILLCHAR)
			{
				if (terminal == PAPER)
					putstring(fd,"\r\n");
				c = ed_getchar(fd);
				if (c != usrkill)
				{
					ed_ungetchar(c);
					continue;
				}
				if (terminal == PAPER)
					terminal = CRT;
				else
				{
					terminal = PAPER;
					putstring(fd,"\r\n");
				}
			}
			continue;
		case cntl('L'):
			ed_crlf(fd);
			draw(fd,REFRESH);
			continue;
		case cntl('[') :
			adjust = escape(fd,out,oadjust);
			continue;
		case cntl('R') :
			search(fd,out,count);
			goto drawline;
		case cntl('P') :
                        if (count <= hloff)
                                hloff -= count;
                        else
                        {
                                hline -= count - hloff;
                                hloff = 0;
                        }
			if (hline <= hismin)
			{
				hline = hismin+1;
				beep(fd);
				continue;
			}
			goto common;

		case cntl('O') :
			location.his_command = hline;
			location.his_line = hloff;
			CntrlO = 1;
			c = '\n';
			goto process;
		case cntl('N') :
			location = hist_locate(hline,hloff,count);
			if (location.his_command > histlines)
			{
				beep(fd);
				continue;
			}
			hline = location.his_command;
			hloff = location.his_line;
		common:
			hist_copy(out,hline,hloff);
		drawline:
			eol = genlen(out);
			cur = eol;
			draw(fd,UPDATE);
			continue;
		}
		
	}
	
process:

	if (c == (-1))
	{
		lookahead = 0;
		beep(fd);
		*out = '\0';
	}
	draw(fd,FINAL);
	tty_cooked(fd);
	if(c == '\n')
	{
		out[eol++] = '\n';
		out[eol] = '\0';
		ed_crlf(fd);
	}
	else
		p_flush();
	i = strlen(buff);
	if (i)
		return(i);
	return(-1);
}

static void show_info(fd,str)
int fd;
char *str;
{
	register char *out = (char *)drawbuff;
	register int c;
	genchar string[LBUF];
	int sav_cur = cur;
	/* save current line */
	genncpy(string,out,sizeof(string)/CHARSIZE-1);
	*out = 0;
	cur = 0;
	gencpy(out,str);
	draw(fd,UPDATE);
	c = ed_getchar(fd);
	if(c!=' ')
		ed_ungetchar(c);
	/* restore line */
	cur = sav_cur;
	genncpy(out,string,sizeof(string)/CHARSIZE-1);
	draw(fd,UPDATE);
}

static void 
putstring(fd,s)
register char *s;
{
	register int c;
	while (c= *s++)
		 putchar(fd,c);
}


static int 
escape(fd,out,count)
int fd;
register genchar *out;
{
	register int i,value;
	int digit,ch;
	digit = 0;
	value = 0;
	while ((i=ed_getchar(fd)),isdigit(i))
	{
		value *= 10;
		value += (i - '0');
		digit = 1;
	}
	if (digit)
	{
		ed_ungetchar(i) ;
		return(value);
	}
	value = count;
	if(value<0)
		value = 1;
	switch(ch=i)
	{
		case ' ':
			mark = cur;
			return(-1);


		case 'p':	/* M-p == ^W^Y (copy stack == kill & yank) */
			ed_ungetchar(cntl('Y'));
			ed_ungetchar(cntl('W'));
			return(-1);

		case 'l':	/* M-l == lower-case */
		case 'd':
		case 'c':
		case 'f':
		{
			i = cur;
			while(value-- && i<eol)
			{
				while ((out[i])&&(!isword(i)))
					i++;
				while ((out[i])&&(isword(i)))
					i++;
			}
			if(ch=='l')
			{
				value = i-cur;
				while (value-- > 0)
				{
					i = out[cur];
					if(isupper(i))
					{
						i += 'a' - 'A';
						out[cur] = i;
					}
					cur++;
				}
				draw(fd,UPDATE);
				return(-1);
			}

			else if(ch=='f')
				goto update;
			else if(ch=='c')
			{
				ed_ungetchar(cntl('C'));
				return(i-cur);
			}
			else
			{
				if (i-cur)
				{
					ed_ungetchar(cntl('D'));
					return(i-cur);
				}
				beep(fd);
				return(-1);
			}
		}
		
		
		case 'b':
		case DELETE :
		case '\b':
		case 'h':
		{
			i = cur;
			while(value-- && i>0)
			{
				i--;
				while ((i>0)&&(!isword(i)))
					i--;
				while ((i>0)&&(isword(i-1)))
					i--;
			}
			if(ch=='b')
				goto update;
			else
			{
				ed_ungetchar(ERASECHAR);
				return(cur-i);
			}
		}
		
		case '>':
			ed_ungetchar(cntl('N'));
			hline = histlines-1;
			hloff = 0;
			return(0);
		
		case '<':
			ed_ungetchar(cntl('P'));
			hloff = 0;
			return(hline-hismin-1);


		case '#':
			ed_ungetchar('\n');
			ed_ungetchar('#');
			ed_ungetchar(cntl('A'));
			return(-1);
		case '_' :
		case '.' :
		{
			genchar name[MAXLINE];
			char buf[MAXLINE];
			char *ptr;
			ptr = hist_word(buf,(count?count:-1));
			if ((eol - cur) >= sizeof(name))
			{
				beep(fd);
				return(-1);
			}
			mark = cur;
			gencpy(name,&out[cur]);
			while(*ptr)
			{
				out[cur++] = *ptr++;
				eol++;
			}
			gencpy(&out[cur],name);
			draw(fd,UPDATE);
			return(-1);
		}

#ifdef NOTDEF
		/* file name expansion */
		case cntl('[') :	/* filename completion */
			i = '\\';
		case '*':		/* filename expansion */
		case '=':	/* escape = - list all matching file names */
			mark = cur;
			if(ed_expand(out,&cur,&eol,i) < 0)
				beep(fd);
			else if(i=='=')
				draw(fd,REFRESH);
			else
				draw(fd,UPDATE);
			return(-1);
#endif /* NOTDEF */

		/* search back for character */
		case cntl(']'):	/* feature not in book */
		{
			int c = ed_getchar(fd);
			if ((value == 0) || (value > eol))
			{
				beep(fd);
				return(-1);
			}
			i = cur;
			if (i > 0)
				i--;
			while (i >= 0)
			{
				if (out[i] == c && --value==0)
					goto update;
				i--;
			}
			i = eol;
			while (i > cur)
			{
				if (out[i] == c && --value==0)
					break;
				i--;
			};

		update:
			cur = i;
			draw(fd,UPDATE);
			return(-1);

		case '[':	/* feature not in book */
			i = '_';

		}
		default:
			/* look for user defined macro definitions */
			if(ed_macro(fd,i))
				return(-1);
		beep(fd);
		return(-1);
	}
}


/*
 * This routine process all commands starting with ^X
 */

static void
xcommands(fd,count)
int fd;
int count;
{
        register int i = ed_getchar(fd);
        switch(i)
        {
                case cntl('X'):	/* exchange dot and mark */
                        if (mark > eol)
                                mark = eol;
                        i = mark;
                        mark = cur;
                        cur = i;
                        draw(fd,UPDATE);
                        return;


                default:
                        beep(fd);
                        return;
	}
}

static int 
search(fd,out,direction)
int fd;
genchar out[];
{
	static int prevdirection =  1 ;
	histloc location;
	register int i,sl;
	genchar str_buff[LBUF];
	register genchar *string = drawbuff;
	/* save current line */
	char sav_cur = cur;
	genncpy(str_buff,string,sizeof(str_buff)/CHARSIZE-1);
	string[0] = '^';
	string[1] = 'R';
	string[2] = '\0';
	sl = 2;
	cur = sl;
	draw(fd,UPDATE);
	while ((i = ed_getchar(fd))&&(i != '\r')&&(i != '\n'))
	{
		if (i==usrerase)
		{
			if (sl > 2)
			{
				string[--sl] = '\0';
				cur = sl;
				draw(fd,UPDATE);
			}
			else
				beep(fd);
			continue;
		}
		if (i==usrkill)
		{
			beep(fd);
			goto restore;
		}
		if (i == '\\')
		{
			string[sl++] = '\\';
			string[sl] = '\0';
			cur = sl;
			draw(fd,APPEND);
			i = ed_getchar(fd);
			string[--sl] = '\0';
		}
		string[sl++] = i;
		string[sl] = '\0';
		cur = sl;
		draw(fd,APPEND);
	}
	i = genlen(string);
	
	if (direction < 1)
	{
		prevdirection = -prevdirection;
		direction = 1;
	}
	else
		direction = -1;
	if (i != 2)
	{
		gencpy(lstring,&string[2]);
		prevdirection = direction;
	}
	else
		direction = prevdirection ;
	location = hist_find((char*)lstring,hline,1,direction);
	i = location.his_command;
	if(i>0)
	{
		hline = i;
		hloff = location.his_line;
		hist_copy((char*)out,hline,hloff);
		return;
	}
	if (i < 0)
	{
		beep(fd);
		hloff = 0;
		hline = histlines;
	}
restore:
	genncpy(string,str_buff,sizeof(str_buff)/CHARSIZE-1);
	cur = sav_cur;
	return;
}


/* Adjust screen to agree with inputs: logical line and cursor */
/* If 'first' assume screen is blank */
/* Prompt is always kept on the screen */

static void
draw(fd,option)
int fd;
DRAWTYPE option;
{
#define	NORMAL ' '
#define	LOWER  '<'
#define	BOTH   '*'
#define	UPPER  '>'
#define UNDEF	0

	static char overflow;		/* Screen overflow flag set */
	register genchar *sptr;		/* Pointer within screen */
	
	static int offset;		/* Screen offset */
	static char scvalid;		/* Screen is up to date */
	
	genchar nscreen[2*MAXLINE];	/* New entire screen */
	genchar *ncursor;		/* New cursor */
	register genchar *nptr;		/* Pointer to New screen */
	char  longline;			/* Line overflow */
	genchar *logcursor;
	genchar *nscend;		/* end of logical screen */
	register int i;
	
	nptr = nscreen;
	sptr = drawbuff;
	logcursor = sptr + cur;
	longline = NORMAL;
	
	if (option == FIRST || option == REFRESH)
	{
		overflow = NORMAL;
		cursor = screen;
		offset = 0;
		cr_ok = crallowed;
		if (option == FIRST)
		{
			scvalid = 1;
			return;
		}
		*cursor = '\0';
		putstring(fd,Prompt);	/* start with prompt */
	}
	
	/*********************
	 Do not update screen if pending characters
	**********************/
	
	if ((lookahead)&&(option != FINAL))
	{
		
		scvalid = 0; /* Screen is out of date, APPEND will not work */
		
		return;
	}
	
	/***************************************
	If in append mode, cursor at end of line, screen up to date,
	the previous character was a 'normal' character,
	and the window has room for another character.
	Then output the character and adjust the screen only.
	*****************************************/
	

	i = *(logcursor-1);	/* last character inserted */
	
	if ((option == APPEND)&&(scvalid)&&(*logcursor == '\0')&&
	    print(i)&&((cursor-screen)<(w_size-1)))
	{
		putchar(fd,i);
		*cursor++ = i;
		*cursor = '\0';
		return;
	}

	/* copy the line */
	ncursor = nptr + ed_virt_to_phys(sptr,nptr,cur,0,0);
	nptr += genlen(nptr);
	sptr += genlen(sptr);
	nscend = nptr - 1;
	if(sptr == logcursor)
		ncursor = nptr;
	
	/*********************
	 Does ncursor appear on the screen?
	 If not, adjust the screen offset so it does.
	**********************/
	
	i = ncursor - nscreen;
	
	if ((offset && i<=offset)||(i >= (offset+w_size)))
	{
		/* Center the cursor on the screen */
		offset = i - (w_size>>1);
		if (--offset < 0)
			offset = 0;
	}
			
	/*********************
	 Is the range of screen[0] thru screen[w_size] up-to-date
	 with nscreen[offset] thru nscreen[offset+w_size] ?
	 If not, update as need be.
	***********************/
	
	nptr = &nscreen[offset];
	sptr = screen;
	
	i = w_size;
	
	while (i-- > 0)
	{
		
		if (*nptr == '\0')
		{
			*(nptr + 1) = '\0';
			*nptr = ' ';
		}
		if (*sptr == '\0')
		{
			*(sptr + 1) = '\0';
			*sptr = ' ';
		}
		if (*nptr == *sptr)
		{
			nptr++;
			sptr++;
			continue;
		}
		setcursor(fd,sptr-screen,*nptr);
		*sptr++ = *nptr++;
	}
	
	/******************
	
	Screen overflow checks 
	
	********************/
	
	if (nscend >= &nscreen[offset+w_size])
	{
		if (offset > 0)
			longline = BOTH;
		else
			longline = UPPER;
	}
	else
	{
		if (offset > 0)
			longline = LOWER;
	}
	
	/* Update screen overflow indicator if need be */
	
	if (longline != overflow)
	{
		setcursor(fd,w_size,longline);
		overflow = longline;
	}
	i = (ncursor-nscreen) - offset;
	setcursor(fd,i,0);
	scvalid = 1;
	return;
}

/*
 * put the cursor to the <new> position within screen buffer
 * if <c> is non-zero then output this character
 * cursor is set to reflect the change
 */

static void
setcursor(fd,new,c)
int fd;
register int new,c;
{
	register int old = cursor - screen;
	if (old > new)
	{
		if ((cr_ok == NO) || (2*(new+plen)>(old+plen)))
		{
			while (old > new)
			{
				putchar(fd,'\b');
				old--;
			}
			goto skip;
		}
		putstring(fd,Prompt);
		old = 0;
	}
	while (new > old)
		putchar(fd,screen[old++]);
skip:
	if(c)
	{
		putchar(fd,c);
		new++;
	}
	cursor = screen+new;
	return;
}

