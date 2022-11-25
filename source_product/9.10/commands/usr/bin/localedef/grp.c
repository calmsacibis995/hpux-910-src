/* @(#) $Revision: 70.3 $ */     
/* LINTLIBRARY */

#include	<stdio.h>
#include        <string.h> /* for strcat */
#include	<limits.h>
#include	"global.h"

int	gotstr;
int value=0; /* defined in global.c */
unsigned char grp_str[LEN_INFO_MSGS]; /* revisit for size */
unsigned char buf[LEN_INFO_MSGS];
/* grp_init: 
** 
*/
void
grp_init(token)
int	token;				/* keyword token */
{
	extern void mntry_str();
	extern void mntry_finish();
	extern void grp_num();
	extern void grp_finish();

if (op)
  {
    if (finish == NULL) error(STATE);
      (*finish)();
  }
	strcpy(grp_str,NULL);
	op = token;			/* save off the token */
        value = num_value;
	number = grp_num;
	finish = grp_finish;
/*	strcat(grp_str, "\\");  */ /* To comply to Posix */
	gotstr = FALSE;
}

void 
grp_num()
{
  char tmp[10];

  if (gotstr) error(STATE);

  (void) getstr(buf);

  if( atoi(buf) == -1) 
    tmp[0]=CHAR_MAX;
  else
    tmp[0]=atoi(buf);
  tmp[1]='\0';
  strcpy(buf,tmp);
  strcat(grp_str, buf);
  semi_flag=FALSE;
  
}

/*
** Make sure that there are no left-over metachars or item number.
*/
void
grp_finish()
{
if (gotstr) error(STATE);		/* only one string allowed */
if ((cp = (unsigned char *)malloc((unsigned) strlen((char *)grp_str)))
		== NULL)
		error(NOMEM);

if(op == MONEY)
  {
    mntry_tab[value] = cp;
    (void) strcpy((char*)cp, (char *)grp_str);
  }
if (op == NUMERIC)
  {
    nmrc_tab[value] = cp;
    (void) strcpy((char*)cp, (char *)grp_str);
  }
}







