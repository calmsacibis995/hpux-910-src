/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: strings.c,v 66.4 90/09/20 12:59:55 kb Exp $ */
/*
 * strings.c
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved.
 */


/*---------------------------------------------------------------------*\
| tokenize                                                              |
|                                                                       |
| Tokenizes a line and returns the tokens in an argv style array of     |
| strings. Also returns an array of pointers to the beginning of each	|
| token in the original line buffer. The return value of the function	|
| is the number of tokens found.					|
\*---------------------------------------------------------------------*/

int tokenize( line, delimiters, tokbuff, tokens, toksinline, maxtoks )
register char *line;
char *delimiters;
register char *tokbuff;
char *tokens[];
char *toksinline[];
int maxtoks;
{
	int nbrtoks;
	register char *delimptr;

	nbrtoks = 0;
	while ( *line )
	{
		for ( ; *line; line++ )
		{
			for( delimptr = delimiters; *delimptr; delimptr++ )
				if ( *line == *delimptr )
					break;
			if ( *delimptr == 0 )
				break;
		}

		if (!(*line))
			break;

		tokens[nbrtoks] = tokbuff;
		toksinline[nbrtoks] = line;
		for (; *line; line++)
		{
			for( delimptr = delimiters; *delimptr; delimptr++ )
				if ( *line == *delimptr )
					break;
			if ( *delimptr )
				break;
			*(tokbuff++) = *line;
		}
		*(tokbuff++) = '\0';
		nbrtoks++;
		if ( nbrtoks >= maxtoks )
			break;
	}
	return (nbrtoks);
}


/*---------------------------------------------------------------------*\
| stoi                                                                  |
|                                                                       |
| String to integer. Returns 0 if ok, -1 if non-numeric encountered	|
| before end of string.							|
|                                                                       |
\*---------------------------------------------------------------------*/

stoi (string, intval)
char *string;
int *intval;
{
	for (*intval = 0; *string; string++)
	{
		if (*string < '0' || *string > '9')
			return (-1);
		*intval = *intval * 10 + *string - '0';
	}
	return (0);
}


/*
 * itos
 *
 * The integer value to be converted is sent in "intval" and a pointer
 * to the buffer to hold the string is sent in "string". The radix for the
 * conversion is provided in radix. Itos returns the length of the
 * resultant string. 
 */

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
        } while ((intval /= radix) > 0);
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
	int i,j;
 
	for (i = 0, j = len-1; i < j; i++, j--)
	{
		c = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = c;
	}
}
