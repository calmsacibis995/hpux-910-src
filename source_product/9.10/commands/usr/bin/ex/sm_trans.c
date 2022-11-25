/* Translate a command string from vi syntax to edit/1000 syntax
 *
 *    0.Check to see if it has the ed1000 command marker '%'.
 *    1.Skip leading spaces.
 *    2.If is an addr char, then turn pc flag on and skip to next non-addr char.
 *    3.If is a space, skip to next non-space.
 *    4.If is a addr, then put in comma and skip to next non-addr.
 *    5.If is a comma,then advance until next non-addr.
 *    6.Skip spaces.
 *    7.Decode each cmd char and advance to next possible command.
 *    8.goto 1 if there is still a command
 */

#ifdef ED1000

#include "ex.h"
#include "ex_sm.h"
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

var char sm_org_cmdline[151];
var char repeat_line[SM_MAXLLEN+1];
var int  repeat_count;
bool chdot;

#ifndef NONLS8	/* User messages */
# define NL_SETN	102	/* set number */
# include <msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8


sm_trans(a)
char a[SM_MAXLLEN+1];
{
	bool addrseen;
	bool addrseen2;
	bool err,oerr;              /* err flags */
	int i;
	int start;
	int pc;                     /* put comma ? */
	int commapos;               /* comma location */
	int addrstart;
	char *p;
	char b[SM_MAXLLEN+1];

	chdot = 0;

	i=1;

	if ( a[0] != '%' ){
		input = a;
		return(0);
	}
	a[0] = ' ';
	strcpy(b,a);
	chdot = 0;
	oerr = 0;
	err = 0;
	while( i < strlen(a)){
		pc = 0;
		commapos = 0;
		addrseen = 0;
		addrseen2 = 0;
		oerr = err || oerr;
		err = 0;
		start = addrstart = i;

		/* span first address: (num|sign|[.:*])(num|sign)* */

		while (sm_isaddr(a[i]) && i < strlen(a) ){
			if (isweird(a[i]) && (i>addrstart)) break;
			if (a[i] == ':')i++;
			addrseen = 1;
			i++;
			pc = 1;
		}

		/* chew up trailing blanks. Be careful with space command */
		if (addrseen)
			while (sm_iswhite(a[i]) && i < strlen(a) )
				i++;

		/* if we crashed into address2 without a comma, put one in */
		if ((sm_isaddr(a[i]) || a[i] == '*') && pc ){
			strinsert(a,",",i);
		}
		if (a[i] == ',' ) {
			pc = 0;
			commapos = i;
			i++;
			while(sm_iswhite(a[i]) && i <strlen(a)) i++;
		}
		addrstart = i;

		/*  turn EDIT/1000 * (linespec 1) into ;. that vi likes better  */
		if (a[i]=='*' && commapos > 0) {
			a[i] = '.';
			a[commapos] = ';';
		}

		/* span address 2 like before */
		while (sm_isaddr(a[i]) && i < strlen(a) ){
			if (isweird(a[i]) && (i>addrstart)) break;
			if (a[i] == ':')i++;
			i++;
			addrseen2 = 1;
		}
		if (addrseen) while (sm_iswhite(a[i]) && i < strlen(a) ) i++;

		/* Turn into valid vi command */
		i= trans(a,i,start,addrseen,commapos,addrseen2,&err);

		/* Process possible multiple commands on a line */
		p = strchr(&a[i],'|');
		if (p){
			i = (p - a) + 1 ;
		}
		else i = strlen(a);

	}
	err = err || oerr;
	if(!err) input = a;
	else {
		error((nl_msg(1, " Error in command: %s  ")), b);
	}

}

trans(a,pos,start,addrseen,commapos,addrseen2,err)
char a[SM_MAXLLEN+1];
int pos,start;
bool addrseen;
int commapos;
bool addrseen2;
bool *err;
{
	/* Translate the command word starting at pos, moving pos if necessary */
	int i,j,k;
	bool wasb;
	bool exclam;
	bool confirm;
	bool all;
	char *ptr;
	char temp[20];
	int index1;
	char buf[5];

	i=j=pos;
	exclam = 0;

	for (k= 0; k<20;k++)temp[k] = '\0';
	if (a[i] != ' ') while (notcmd(a[i]) && i< strlen(a) ) i++;
	confirm = 0;
	all = 0;
	switch(a[i]){

	case ' ':
		a[i] = 'a';
		*err = 0;
		strinsert(a,"\n",i+1);
		i++;
		i++;
		if (insm) chdot = 1;
		while (!sm_eol(a[i])) i++;
		if (a[i]!='\n') {
			strinsert(a,"\n",i);
		}
		strinsert(a,".\n",i+1);
		break;

	case 'a':
	case 'A':
		a[i] = 'q';
		if (a[i+1] == '/') {
			a[i+1] = ' ';
			confirm = 1;
		}
		*err = !check1(a,i+1);
		strinsert(a,"!",i+1);
		if ( tchng && !confirm){
			strinsert(a," as | ",start);
			i = i + 7 ;
		} else i = i + 1;
		break;

	case '\'':
		if (i==start) {
			if (addrseen == 0){
				strinsert(a," ",i);
				strinsert(a,".+1",i);
				i=i+3;
			}
		}
		*err = !check444(a,i+1,&all);
		goto f1;

	case '`':
		if (i==start) {
			if (addrseen == 0){
				strinsert(a," ",i);
			/* strinsert(a,".+1",i);
			 * i=i+3;
			 */
			}
		}
		*err = !check44(a,i+1,&all);
		goto f1;

	case 'b':
	case 'B':
		if (( a[i+1] == 'k') || (a[i+1] == 'K')){
			a[i]   = 's';
			a[i+1] = 's';
			strinsert(a,"|1,$s/ *$//g|sr",i+2);
			i = i+16;
			break;
		}
		*err = !check4(a,i+1,&all);
		if (i==start) strinsert(a,"1",i++);
f1:
		if (all) {
			a[i]=0;
			if (!strchr(&a[start],',') && !strchr(&a[start],';')) {
				a[i]='g';
				strinsert(a,",$",i);
				i+=2;
			}
			a[i]='g';
			j=i;
			while(!check1(a,j)) j++;
			strinsert(a,"nu",j);
		}
		else {
			if (i!=start) {
				a[i] = '|';
			}
			else {
				a[i] = ' ';
			}

			/*  Allow F [or B] by itself to mean continue searching */

			if (a[i+1] == '\n' || a[i+1] == '\0'){
				strinsert(a,"/",i+1);
				i++;
			}
		}
		if (insm) chdot = 1;
		i+=2;
		break;

	case 'c':
	case 'C':
		a[i] = (char) tolower(a[i]);
		a[i+1] = (char) tolower(a[i+1]);
		if (a[i+2] == 'q' || a[i+2] == 'Q') {
		/* so the older folks used to typing 'q' for the quiet
		 * option (this is implicit anyway) won't get errors.
		 */
			*err = (!check1(a,i+3)) || (a[i+1] != 'o');
			a[i+2] = '.';
		}
		else {
			*err = (!check1(a,i+2)) || (a[i+1] != 'o');
			strinsert(a,".",i+2);
		}
		i = i + 3;
		break;

	case 'e':
	case 'E':
		if (a[i+1] == 'r' || a[i+1] == 'R') exclam = 1;
		a[i] = 'w';
		a[i+1] = ' ';
		*err = !check2(a,i+2);
		if (exclam) /*strinsert(a,"!",i+2);*/
			a[i+1] = '!';
		i = i + 2;
		j = i;
		while(!check1(a,j))j++;
		strinsert(a,"|q",j);
		i = j+2;
		break;

	case 'f':
	case 'F':
		if (a[i+1] == '#') {
			if (chng) {
				write(1, "save changes? ", strlen("save changes? "));
				buf[read(1, buf, 5)] = '\0';
				if (*buf == 'y' || *buf == 'Y'){
					a[i] = 'w';
					a[i+1] = '!';
					strinsert(a,"|e#|1",i+2);
					i += 7;
				}
				else {
					a[i] = 'e';
					a[i+1] = '!';
					strinsert(a,"#|1",i+2);
					i += 5;
				}
			} else {
				a[i] = 'e';
				a[i+1] = '#';
				strinsert(a, "|1", i+2);
				i += 4;
			}
			break;
		} else if (a[i+1] == 'i' || a[i+1] == 'I') {

		/* 'fi file'	takes file as the next input stream
		 * 'fi'		goes to the next file in the list
		 * 'f#'		goes to the previous file
		 *
		 * if changes were made, the user is queried if the
		 * changes are to be saved.
		 */
			if (chng) {
				write(1, "save changes? ", strlen("save changes? "));
				buf[read(1, buf, 5)] = '\0';
				if (*buf == 'y' || *buf == 'Y') {
					a[i] = 'w';
					a[i+1] = '!';
					strinsert(a, "|ne ", i+2);
					i += 6;
				}
				else {
					a[i] = 'n';
					a[i+1] = 'e';
					strinsert(a, "! ", i+2);
					i += 4;
				}
			}
			else {
				a[i] = 'n';
				a[i+1] = 'e';
				strinsert(a, " ", i+2);
				i += 3;
			}
			j = i;
			while(!check1(a,j)) j++;
			if (a[j-1] == '/') {
				a[j-1] = ' ';
				confirm = 1;
			}
			*err = !check2(a,i);
			/* if (!confirm) {
			 *	strinsert(a,"as|",start);
			 * 	i = i +3;
			 * }
			 */
			if (insm) chdot = 1;
			j = i;
			while ( a[j] != '\n' && a[j] != '|')j++;
			strinsert(a,"|1",j);
			i = j + 2;
			break;
		} else {	/* command is 'f': string search */
			if (i==start) {
				if (addrseen == 0) {
					strinsert(a,".+1",i);
					i=i+3;
				}
			}
			*err = !check4(a,i+1,&all);
			goto f1;
		}
		break;

	case 'g':
	case 'G':
	case 'x':
	case 'X':
xchg:
		a[i] = 's';
		i++;
		if (check1(a,i)){
			a[i-1] = '&';
			break;
		}
		*err = !check5(a,i,&confirm);
		j = i;
		while (!check1(a,j)) j++;
		if ( notaslash(a,j-1) ||
		    (j-3 >= 0 && !notaslash(a,j-2) && !notaslash(a,j-1) &&
		    a[j-3] == 's'))
			strinsert(a,"/g",j);
		else strinsert(a,"g",j);
		if (!confirm) {
			strinsert(a,"as|",start);
			i = i + 3;
		}
		if (insm) chdot = 1;
		break;

	case '?':
	case 'h':
	case 'H':
		a[i] = (char) tolower(a[i]);
		a[i+1] = (char) tolower(a[i+1]);
		if (a[i+1] == 'e') a[i+1] = ' ';
		/* *err = !check1(a,i+1);
		 */
		break;

	case 'i':
	case 'I':
		a[i] = (char) tolower(a[i]);
		*err = 0;
		strinsert(a,"\n",i+1);
		i++;
		i++;
		if (insm) chdot = 1;
		while (!sm_eol(a[i])) i++;
		if (a[i]!='\n') {
			strinsert(a,"\n",i);
		}
		strinsert(a,".\n",i+1);
		break;

	case 'j':
	case 'J':
		a[i] = (char) tolower(a[i]);
		*err = !check1(a,i+1);
		i++;
		if (insm) chdot = 1;
		break;

	case 'k':
	case 'K':
		a[i] = (char) tolower(a[i]);
		if ((a[i+1] >= IS_MACRO_LOW_BOUND) && isalpha(a[i+1])) {
			i++;
			*err = !check1(a,i+1);
		}
		else{
			a[i] = 'd';
			*err = !check3(a,i+1,&confirm);
			i++;
			if ((!confirm) && (i != 2)) {
				strinsert(a,"as|",start);
				i = i + 3;
			}
		}
		if (insm) chdot = 1;
		break;

	case 'l':
	case 'L':
		switch(a[i+1]){
		case ' ':
		case '\t':
			a[i] = 'p';
			*err = !check3(a,i+1,&confirm);
			j = start;
			i++;
			break;

		case '\n':
		case '|':
		case EOF:
			a[i] = 'p';
			i++;
			j = start;
			break;

		case 'N':
		case 'n':
			a[i] = 'p';
			a[i+1] = ' ';
			*err = !check3(a,i+1,&confirm);
			/* if ( i - 1 < 0 ) j = 0;
			 * else j = i-1;
			 */
			strinsert(a," set nu | ",start);
			i = i + 12;
			j = start + 10;
			break;

		case 'u':
		case 'U':
			a[i] = 'p';
			a[i+1] = ' ';
			*err = !check3(a,i+1,&confirm);
			/* if ( i - 1 < 0 ) j = 0;
			 * else j = i-1;
			 */
			strinsert(a," set nonu | ",start);
			i = i + 14;
			j = start + 12;
			break;

		default:
			if ((a[i+1] < IS_MACRO_LOW_BOUND) || !isdigit(a[i+1])) 
			        *err = 1;
			else{
				a[i] = 'p';
				i++;
				j = start;
			}
			break;
		}
		if (!addrseen2 && !iscnt(a,i)){
			if (commapos){
				a[commapos] = ' ';
				strinsert(a,"20",i);
			} else{
				if (!addrseen){
					if (dot + 20 > dol){
						strinsert(a,".,$",j);
					} else{
						strinsert(a,".,.+20",j);
					}
				} else{
					strinsert(a,"20",i);
				}
			}
		}
		break;

	case 'm':
	case 'M':
		if ( a[i+1] != 'o' && a[i+1] != 'O'){
			a[i] = 'r';
			*err = !check2(a,i+1);
		} else{
			a[i] = (char) tolower(a[i]);
			a[i+1] = ( char) tolower(a[i+1]);
			if (a[i+2] == 'q' || a[i+2] == 'Q') {
				*err = !check1(a, i+3);
				a[i+2] = '.';
			} else {
				*err = !check1(a,i+2);
				strinsert(a,".",i+2);
			}
			i = i + 3;
		}
		break;

	case 'n':
	case 'N':
		a[i] = ' ';
		*err = !check1(a,i+1);
		strinsert(a,".=",i);     /* ddl : rev1 */
		i++;
		break;

	case 'q':
	case 'Q':
		a[i] = (char) tolower(a[i]);
		a[i+1] = (char) tolower(a[i+1]);
		if (check1(a,i+1)){
			strinsert(a,"s",i);
			i= i+2;
		} else{
			*err = 1;
		}
		break;

	case 'r':
	case 'R':
		a[i] = (char) tolower(a[i]);
		a[i+1] = (char) tolower(a[i+1]);
		if (a[i+1] == 'u' ){
			a[i] = '!';
			a[i+1] = ' ';
			index1 = i + 2;
			while (a[index1]){
				if (a[index1] == ' '){
					index1++;
				} else{
					if (a[index1] == ','){
						a[index1] = ' ';
					}
					break;
				}
			}
		}
		else if (a[i+1] == 'e') {
		/*
		 * 're' in vi may cause duplication of lines, hence
		 * 're' is translated to 'rew'.
		 */
			if (chng) {
				write(1, "save changes? ", strlen("save changes? "));
				buf[read(1, buf, 5)] = '\0';
				if (*buf == 'y' || *buf == 'Y') {
					/* want a="w!|rew" */
					if (a[i+2] == 'w') {
						/* a="rew" already */
						strinsert(a, "w!|", i);
						i += 6;
					} else {
						a[i] = 'w';
						a[i+1] = '!';
						strinsert(a, "|rew", i+2);
						i += 6;
					}
				} else {
					/* want a="rew!" */
					if (a[i+2] == 'w') {
						/* a="rew" already */
						strinsert(a, "!", i+3);
						i += 4;
					}
					else {
						strinsert(a, "w!", i+2);
						i += 4;
					}
				}
			}
			else {
				/* we want just a="rew" */
				if (a[i+2] == 'w')
					i += 3;
				else {
					strinsert(a, "w", i+2);
					i += 3;
				}
			}
		}
		else
			*err = 1;
		break;

	case 's':
	case 'S':
		a[i] = (char) tolower(a[i]);
		a[i+1] = (char) tolower(a[i+1]);
		if (check1(a,i+1)){
			strinsert(a,"m",i+1);
			i= i+2;
		} else if (a[i+1] == 'e' ){
			char	option[5];

			i=i+2;
			j = 0;
			if (a[i] != ' ') strinsert(a," ",i);
			while (sm_iswhite(a[i])&& i < strlen(a))i++;
			k = 0;
			while ((a[i] >= IS_MACRO_LOW_BOUND) && isalpha(a[i]) && i < strlen(a)) {
				if (!j) j = i;
				temp[k] =a[i] = (char) tolower(a[i]);
				i++;
				k++;
				if (k==2) break;
			}
			temp[k] = '\0';
			if (strcmp(temp,"re") && strcmp(temp,"as") &&
			    strcmp(temp,"ws") && strcmp(temp,"cf") &&
			    strcmp(temp,"df")){
				*err = 1;
			}
			(void) strncpy(option, temp, 2);
			if (a[i] != ' ') strinsert(a," ",i);
			while ( sm_iswhite(a[i]) && i < strlen(a))i++;
			k = 0;
			for ( k=0;k<20;k++) temp[k] = '\0';
			k = 0;
			while ((a[i] >= IS_MACRO_LOW_BOUND) && isalpha(a[i]) && i < strlen(a)){
				temp[k] = (char)tolower(a[i]);
				a[i] = ' ';
				i++;
				k++;
			}
			temp[k] = '\0';
			if (!strncmp(temp,"of",2)) strinsert(a,"no",j);
		/*	if (!strcmp(temp,"off"))strinsert(a,"no",j);
		 *	if (!strcmp(temp,"of"))strinsert(a,"no",j);
		 */
		/*
		 *	if (strncmp(temp,"of",2) && strcmp(temp,"on"))
		 *		*err =1;
		 */
			if (strncmp(temp,"of",2) && strcmp(temp,"on")) {
				if (!strncmp(option, "as", 2)) {
					if (value(ASK) == 1)
						strinsert(a,"no",j);
				}
				if (!strncmp(option, "cf", 2)) {
					if (value(IGNORECASE) == 1)
						strinsert(a,"no",j);
				}
				if (!strncmp(option, "df", 2)) {
					if (value(DISPLAY_FNTS) == 1)
						strinsert(a,"no",j);
				}
				if (!strncmp(option, "re", 2)) {
					if (value(MAGIC) == 1)
						strinsert(a,"no",j);
				}
			}
			i = i + 2;
		} else if (a[i+1] == 'h')
			i = i + 2;	/* *err = !check1(a,i); */
		else
			*err = 1;
		break;                           /* ddl: rel1 */

	case 't':
	case 'T':
		a[i+1] = (char) tolower(a[i+1]);
		if (a[i+1] == 'r'){
			a[i]   = 's';
			a[i+1] = 'o';
		} else{
			*err = 1;
		}
		break;

	case 'w':
	case 'W':
		a[i+1] = (char) tolower(a[i+1]);
		if (check1(a, i+1) || (a[i+1] != 'c' && a[i+1] != 'r'))
		/* if dot-1 < 10 then list 1,+10 otherwise list -10,+10
		 * if dol-dot < 10 then list -10,dol else list -10,dol
		 */
		{	int	top, bottom;
			char	buf[10];

			top = dot - one + 1;
			bottom = dol - dot + 1;
			a[i] = ' ';
			sprintf(buf, "%s,%s",
				((top > 10) ? "-10" : "1"),
				((bottom > 10) ? "+10": "$"));
			strinsert(a, buf, i+1);
			break;
		}
		*err = (!check2(a,i+2) ||
			(a[i+1] != 'c' && a[i+1] != 'r'));
		if (a[i+1] == 'r')	exclam=1;
		a[i] = 'w';
		a[i+1]=' ';
		if (exclam)	strinsert(a,"!",i+1);
		i++;
		ptr = strchr(&a[start],'|');
		if (ptr) {
			i = (ptr - a) + 1;
		}
		else	i = strlen(a)-1;
		strinsert( a," | f",i);
		i = i + 4;
		break;

	case 'u':
		a[i] =(char)tolower(a[i]);
		a[i+1] = (char)tolower(a[i+1]);
		*err = !check1(a,i+2) || a[i+1] != 'n';
		break;

	case '^':
		a[i] = '-';
		break;

	case '*':
		break;

	case '_':
		index1 = i - 1;
		while (index1 >= 0){
			if (sm_eol(a[index1])){
				a[index1]   = '\n';
				a[index1+1] = '\0';
				break;
			}
			index1--;
		}
		if (index1 < 0){
			a[0] = '*';
			a[1] = '\n';
			a[2] = '\0';
		}
		repeat_line[0] = '\0';
		strcpy(repeat_line,a);
		repeat_count = 0;
		i++;
		while (!sm_eol(a[i])){
			if ((a[i] >= IS_MACRO_LOW_BOUND) && isdigit(a[i])){
				repeat_count = (a[i] - 060) + (repeat_count * 10);
			} else{
				repeat_count = 0;
				*err = 1;
				break;
			}
			i++;
		}
		break;

	case '\n':
		/*if(insm) chdot = 1;*/
		break;

	default:
		*err = 1;
		break;

	}
	return(i);
}

strinsert(a,b,pos)
char a[];
char *b;
int pos;
/* Insert  string b into string a starting at pos */
{
	char temp[SM_MAXLLEN+1];
	int i;

	if ( strlen(a) + strlen(b) > SM_MAXLLEN )	return(0);
	strcpy(temp,&a[pos]);
	strcpy(&a[pos],b);
	strcat(a,temp);
}

STRINSERT(a,b,pos)
CHAR a[];
CHAR *b;
int pos;
/* Insert  string b into string a starting at pos */
{
	CHAR temp[SM_MAXLLEN+1];
	int i;

	if ( STRLEN(a) + STRLEN(b) > SM_MAXLLEN )	return(0);
	STRCPY(temp,&a[pos]);
	STRCPY(&a[pos],b);
	STRCAT(a,temp);
}

strinsert2(a,b,pos)
char a[];
char *b;
int pos;
/* Insert  string b into string a starting at pos */
{
	char temp[SM_MAXLLEN+1];
	int i;

	if ( strlen(a) + strlen(b) > LBSIZE )	return(0);
	strcpy(temp,&a[pos]);
	strcpy(&a[pos],b);
	strcat(a,temp);
}

STRINSERT2(a,b,pos)
CHAR a[];
CHAR *b;
int pos;
/* Insert  string b into string a starting at pos */
{
	CHAR temp[SM_MAXLLEN+1];
	int i;

	if ( STRLEN(a) + STRLEN(b) > LBSIZE )	return(0);
	STRCPY(temp,&a[pos]);
	STRCPY(&a[pos],b);
	STRCAT(a,temp);
}

int sm_eol(c)
char c;
{
	return ( c == '\n' || c == EOF || c == '|' );
}

int notcmd(c)
char c;
{
	return(((c >= IS_MACRO_LOW_BOUND) && isdigit(c))||c==','||c=='.'||
	    c=='$'||c==' '||c=='\t'|| c=='+'||c=='-'||c==':');
}

int sm_isaddr(c)
char c;
{
	return(((c >= IS_MACRO_LOW_BOUND) && isdigit(c)) ||c=='.'||c=='$'||
	    c=='+'||c=='-'||c==':');
}

int sm_iswhite(c)
{
	return( c==' '||c== '\t');
}

int sm_iswhite_c(c)
char *c;
{
	if (*c == ',')
		*c = ' ';
	return( *c==' '||*c== '\t');
}

int isweird(c)
char c;
{
	return(c == ':' || c == '.' || c == '$' );
}

int check1(a,pos)
/* next word must be the end */
int pos;
char a[];
{
	int j;

	j = pos;
	while (sm_iswhite(a[j]) &&  j < strlen(a) )	j++;
	return(a[j] == '\n' || a[j] == EOF || a[j] == '\0' ||
	       a[j] == '|');
}

int check2(a,pos)
/* next word optional space, then possible a character string, then end */
int pos;
char a[];
{
	int j;

	j = pos;
	while (sm_iswhite_c(&a[j]) &&  j < strlen(a) )	j++;
	if ( j == pos && !check1(a,j)) {
		strinsert(a," ",j);
		j++;
	}
	while (!sm_iswhite(a[j]) &&  j < strlen(a) )	j++;
	return(check1(a,j));
}

int check3(a,pos,confirm)
/* commands which allows an max range */
int pos;
char a[];
bool *confirm;
{
	int j;

	j = pos;
	while (sm_iswhite(a[j]) &&  j < strlen(a) )	j++;
	while ((a[j] >= IS_MACRO_LOW_BOUND) && isdigit(a[j]) && j < strlen(a))
		j++;
	while (sm_iswhite(a[j]) &&  j < strlen(a) )	j++;
	if (a[j] == '/') {
		a[j] = ' ';
		*confirm = 1;
	}
	return(check1(a,j));
}

int check4(a,pos,all)
/* handles types for commands like b & f */
int pos;
char a[];
bool *all;
{
	int j;

	j = pos;
	while (sm_iswhite(a[j]) &&  j < strlen(a) )	j++;
	if (!notaslash(a,j)){
		j++;
		while( notaslash(a,j) && !check1(a,j) && j <strlen(a) )
			j++;
	}
	if (!notaslash(a,j)) {
		j++;
		if (a[j] == 'a' || a[j] == 'A') {
			*all = 1;
			a[j] = ' ';
			if (a[j+1] == 'q' || a[j+1] == 'Q')
				a[j+1] = ' ';
		}
		else if (a[j] == 'q' || a[j] == 'Q') {
			a[j] = ' ';
			if (a[j+1] == 'a' || a[j+1] == 'A') {
				*all = 1;
				a[j+1] = ' ';
			}
		}
	}
	return(check1(a,j));
}

int check44(a,pos,all)
/* handles types for commands like b & f */
int pos;
char a[];
bool *all;
{
	int j;

	j = pos;
	while (sm_iswhite(a[j]) &&  j < strlen(a) )	j++;
	if (!notabackquote(a,j)){
		j++;
		while(notabackquote(a,j) && !check1(a,j) &&
		      j <strlen(a))
			j++;
	}
	if (!notabackquote(a,j)) {
		j++;
		if (a[j] == 'a' || a[j] == 'A') {
			*all = 1;
			a[j] = ' ';
		}
	}
	return(check1(a,j));
}

int check444(a,pos,all)
/* handles types for commands like b & f */
int pos;
char a[];
bool *all;
{
	int j;

	j = pos;
	while (sm_iswhite(a[j]) &&  j < strlen(a) )	j++;
	if (!notaforwardquote(a,j)){
		j++;
		while(notaforwardquote(a,j) && !check1(a,j) &&
		      j <strlen(a))
			j++;
	}
	if (!notaforwardquote(a,j)) {
		j++;
		if (a[j] == 'a' || a[j] == 'A') {
			*all = 1;
			a[j] = ' ';
		}
	}
	return(check1(a,j));
}

int check5(a,pos,confirm)
/* all exchanges */
int pos;
char a[];
bool *confirm;
{
	int j;
	int error;

	j = pos;
	error = 0;
	while (sm_iswhite(a[j]) &&  j < strlen(a) ) j++;
	if ( !notaslash(a,j)){
		j++;
		while(notaslash(a,j)  && !check1(a,j) &&
		      j <strlen(a) )
			j++;
	}
	if (notaslash(a,j)) error =1;
	if ( !notaslash(a,j)){
		j++;
		while(notaslash(a,j)  && !check1(a,j) && j <strlen(a) )
			j++;
	}
	if (!notaslash(a,j)) {
		j++;
		if (!notaslash(a,j)) {
			a[j] = ' ';
			*confirm = 1;
		}
	}
	return(check1(a,j) && !error);
}

int iscnt(a,pos)
char a[];
int pos;
/* check to see if there is number following pos */
{
	int j;

	j = pos;
	if (check1(a,pos))return(0);
	while( sm_iswhite(a[j]) && j < strlen(a) )	j++;
	return((a[j] >= IS_MACRO_LOW_BOUND) && isdigit(a[j]));
}

int notaslash(a,pos)
int pos;
char a[];
/* check if a[pos] is not a slash */
{
	return( (a[pos] == '/' &&  pos-1 >= 0 && a[pos-1] == '\\') ||
	    (a[pos] != '/') );
}
int notabackquote(a,pos)
int pos;
char a[];
/* check if a[pos] is not a "`" */
{
	return( (a[pos] == '`' &&  pos-1 >= 0 && a[pos-1] == '\\') ||
	    (a[pos] != '`') );
}

int notaforwardquote(a,pos)
int pos;
char a[];
/* check if a[pos] is not a "'" */
{
	return( (a[pos] == '\'' &&  pos-1 >= 0 && a[pos-1] == '\\') ||
	    (a[pos] != '\'') );
}

int  cmd_stk_sizes[20];/* space for 20 sizes */
char *cmd_stk_ptr[20]; /* space for 20 pointers */
char  cmd_stk[1640];   /* space for 20 records (80 char, null, status */
int  initialized = 0;
int  lines_number_of;  /* The number of lines in the stack. */
int  lines_last;       /* This number is one less than 'lines_number_of'
			  because c arrays start at 0 not 1. */
extern char sm_cmdline[151];

putaline()
{
	int index, length;

	index = 1;
	length = 0;
	while ((index < 151) && (sm_cmdline[index++] != '\n')) {
		length++;
	}
	cmd_stk_add(&sm_cmdline[1],length);
}

getaline()
{
	int  length,index;
	int  bufferlength,fd1,fd2,state;

	fd1 = 0;
	fd2 = 1;
	state = 0;
	bufferlength = 151;
	index = 1;
	length = 0;
	while (sm_cmdline[index++] != '\n'){
		length++;
	}
	cmd_stk_get(&sm_cmdline[1],bufferlength,fd1,fd2,state,&length);
	index = length + 1;
	while (index < 151){
		sm_cmdline[index++] = '\0';
	}
}

cmd_stk_init()
{
	int count;

	count = 1;
	cmd_stk_ptr[0] = cmd_stk;
	cmd_stk_sizes[0] = 0;

	while (count < 20){
		cmd_stk_ptr[count] = cmd_stk_ptr[count-1] + 82;
		cmd_stk_sizes[count] = 0;
		count++;
	}
	initialized = 1;
	lines_number_of = 20;
	lines_last = lines_number_of - 1;
}

/*
 *	Add a line to the command stack.
 */
cmd_stk_add(buffer,length)
char *buffer;
int  length;
{
	int count,temp;
	int *temp_ptr;

	/* Initialize the command stack system if necessary */
	if (!initialized)
		cmd_stk_init();

	/* if the line is zero length, just forget it. */
	if (length <= 0)
		return;

	/* maximum line length is not allowed to be greater than 80 */
	if (length > 80)
		length = 80;

	/* Is there is an exact duplicate of an existing line? */
	count = 0;
	while (count < lines_number_of){
		if (cmd_stk_sizes[count] == length){
			if (!strncmp(buffer,cmd_stk_ptr[count],length)){
				if (count != lines_last){
					temp_ptr = cmd_stk_ptr[count];
					moveints(&cmd_stk_ptr[count+1],&cmd_stk_ptr[count],lines_last-count);
					cmd_stk_ptr[lines_last] = temp_ptr;
					temp = cmd_stk_sizes[count];
					moveints(&cmd_stk_sizes[count+1],&cmd_stk_sizes[count],lines_last-count);
					cmd_stk_sizes[lines_last] = temp;
				}
				return;
			}
		}
		count++;
	}

	/* Push all stacks. */
	/* Push all record lengths, making the last entry available. */
	moveints(&cmd_stk_sizes[1],&cmd_stk_sizes[0],lines_number_of - 1);

	/* Push all record pointers, making the last entry available. */
	/* Rotate entry 0 to the last entry position. */
	temp_ptr = cmd_stk_ptr[0];
	moveints(&cmd_stk_ptr[1], &cmd_stk_ptr[0], lines_number_of - 1);
	cmd_stk_ptr[lines_last] = temp_ptr;

	/* Put the buffer in the last entry of the command stack. */
	/* Put the buffer length in the last entry of the command stack sizes.
	 */
	temp = movechars(buffer,cmd_stk_ptr[lines_last],length);
	cmd_stk_ptr[lines_last] [81] = (char) temp;
	cmd_stk_sizes[lines_last] = length;
	count = 0;

}

/*
 *	subroutine to fetch a line (maintains command stack)
 *	returns byte count as value and data by reference
 */
int 
cmd_stk_get(buffer,bufferlength,fd1,fd2,state,length)
char *buffer;
int  bufferlength;
int  fd1;
int  fd2;
int  state;
int  *length;
{
	int tlength, index1, index2, display_count, lastc, c;
	int cmdsiz,temp;
	char *dfon, *finchars, cc;
	struct termio a1;
	ushort save_c_lflag;
	unsigned char cc_temp[6];

	/* Read from the terminal unless the caller supplied the command. */

	if (state){
		tlength = read(fd1,buffer,bufferlength);
	} else{
		tlength = *length;
	}

	if ((tlength <= 0) || (buffer[0] != '/')){
		*length = tlength;
		return(tlength);
	}

again1:
	display_count = 0;
	index1 = 1;
	if (buffer[1] == '/'){
		while (buffer[index1++] == '/'){
			display_count++;
		}
	} else{
		while ((buffer[index1] >= IS_MACRO_LOW_BOUND) && isdigit(buffer[index1])){
			display_count = (buffer[index1] - 060) + (display_count * 10);
			index1++;
		}
	}
	if (display_count > lines_number_of)display_count = lines_number_of;

	/* If out of range or unspecified, display all */

	index1 = lines_number_of - display_count;
	if (index1 >= lines_number_of) index1 = 0;

	write(fd2,nl_msg(2, "---commands---\n"),strlen(nl_msg(2, "---commands---\n")));

	/* Print the command stack. */

	while (index1 < lines_number_of){
		cmdsiz = cmd_stk_sizes[index1];
		index2 = 0;
		if (cmd_stk_ptr[index1][81]){
			write(fd2,"\033Y",2);
		}
		if (cmdsiz != 0){
			lastc = -1;
			index2 = 0;
			while (index2 < cmdsiz){
				c = cmd_stk_ptr[index1][index2];
				cc = (char) c;
				temp = write(fd2,&cc,1);
				if (c < 040){
					if ((lastc == 033) && (c == 'Z')){
						write(fd2,"\033Y",2);
					}
				}
				lastc = c;
				index2++;
			}
			if (cmd_stk_ptr[index1][81]){
				write(fd2,"\033Z\033D\033D\033K\015\012",10);
			} else{
				write(fd2,"\015\012",2);
			}
		}
		index1++;
	}

	if (display_count != 0){
		index1 = 0;
		while (index1++ < display_count){
			write(fd2,"\033A",2);
		}
	}

	/* lock the terminal lu (without wait; we're not that insistent) */

	/*
	 * call lurq(140001b,lu,1,*161)
	 */

	write(fd2,"\021",1);
	read(fd1,buffer,bufferlength);

	ioctl(fd2,TCGETA,&a1);
	save_c_lflag = a1.c_lflag;
	a1.c_lflag &= ~(ECHO|ECHOE);
	index1 = 0;
	while (index1 < 6){
		cc_temp[index1] = a1.c_cc[index1];
		a1.c_cc[index1] = 0377;
		index1++;
	}
	ioctl(fd2,TCSETA,&a1);
	write(fd2,"\033R\033J\033A\033d\021",9);
	tlength = read(fd1,buffer,bufferlength);
	a1.c_lflag = save_c_lflag;
	index1 = 0;
	while (index1 < 6){
		a1.c_cc[index1] = cc_temp[index1];
		index1++;
	}
	ioctl(fd2,TCSETA,&a1);

	write(fd2,"\015\012",2);
	if (buffer[0] == '/')goto again1;

/*
 * c   unlock terminal now.
 *   
 * 	call lurq(40000b,lu,1,*25)
 */
	*length = tlength;
	return(tlength);
/*
 * c
 * c  here on a terminal initialization error.
 * c
 * 9009  continue
 * c
 * c   get here when there is an reio/exec error
 * 799   continue
 *       call lurq(40000b,lu,1,*9999) ! make sure terminal is unlocked.
 * 9999  cmd_stk_get = -1
 *       length = 0
 *       return
 * c
 *       end
 */
}

int
moveints(buffer1,buffer2,length)
int *buffer1, *buffer2, length;
/*
 * 	This routine will move 'length' ints from
 * 	'buffer1' to 'buffer2'.  The return status is undefined.
 */
{
	while (length-- > 0){
		*buffer2++ = *buffer1++;
	}
}

int
movechars(buffer1,buffer2,length)
char *buffer1, *buffer2;
int  length;
/*
 * 	This routine will move 'length' characters from
 * 	'buffer1' to 'buffer2'.  It will also determine
 * 	if any control characters exist in the record
 * 	(a control character is any character less than 
 * 	40 octal).  The record will be zero terminated.
 * 	The return status will be:
 * 
 * 		0 if no  control characters were found.
 * 		1 if any control characters were found.
 */
{
	int ctrl_char;

	ctrl_char = 0;
	while (length-- > 0){
		*buffer2 = *buffer1;
		if (*buffer2 < ' '){
			ctrl_char = 1;
		}
		buffer2++;
		buffer1++;
	}
	*buffer2 = '\0';
	return(ctrl_char);
}
#endif ED1000
