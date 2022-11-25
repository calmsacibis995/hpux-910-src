/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: errmsg.c,v 66.3 90/09/20 12:01:13 kb Exp $ */
/*---------------------------------------------------------------------*\
| errmsg                                                                |
|                                                                       |
| a special printf for reporting facet process errors.                  |
\*---------------------------------------------------------------------*/

/* VARARGS */
errmsg (fmt, args)
char *fmt, *args;
{
	char **s_argptr, *fmtptr;
	int *i_argptr;
	char buff[16];
	char *strptr;
	int len;

	s_argptr = &args;
	i_argptr = (int *)(&args);
	for (fmtptr = fmt; *fmtptr; fmtptr++)
	{
		if (*fmtptr == '%')
		{
			fmtptr++;
			if (*fmtptr == 's')
			{
				strptr = *s_argptr;
				len = strlen (strptr);
				s_argptr++;
				i_argptr = (int *)(s_argptr);
			}
			else if (*fmtptr == 'x')
			{
				strptr = buff;
				len = itos ((unsigned) *i_argptr, buff, 16);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else if (*fmtptr == 'd')
			{
				strptr = buff;
				len = itos ((unsigned) *i_argptr, buff, 10);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else if (*fmtptr == 'o')
			{
				strptr = buff;
				len = itos ((unsigned) *i_argptr, buff, 8);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else if (*fmtptr == '%')
			{
				strptr = fmtptr;
				len = 1;
			}
			else
			{
				strptr = fmtptr - 1;
				len = 2;
			}
		}
		else
		{
			strptr = fmtptr;
			len = 1;
		}
		while (len--)
		{
			if (*strptr == '\n')
				write( 2, "\r", 1 );
			write( 2, strptr, 1 );
			strptr++;
		}
	}
}




/*---------------------------------------------------------------------*\
| itos                                                                  |
|                                                                       |
| The integer value to be converted is sent in "intval" and a pointer   |
| to the buffer to hold the string is sent in "string". Itos returns    |
| the length of the resultant string. The integer is an unsigned short. |
\*---------------------------------------------------------------------*/

int itos (intval, string, radix)
unsigned intval;
char *string;
int radix;
{
	register int i, digit;
 
	i = 0;
	do
	{
		digit = intval % radix;
		if (digit >= 10)
			*(string + (i++)) = digit - 10 + 'A';
		else
			*(string + (i++)) = digit + '0';
	} while ((intval /= radix) != 0);
	*(string+i) = '\0';
	revstr (string, i);
	return (i);
}




/*---------------------------------------------------------------------*\
| revstr                                                                |
|                                                                       |
| Reverses the string pointed to by "buffer".                           |
\*---------------------------------------------------------------------*/
 
revstr (buffer, len)
char buffer[];
int len;
{
	char c;
	register int i,j;
 
	for (i = 0, j = len-1; i < j; i++, j--)
	{
		c = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = c;
	}
}
