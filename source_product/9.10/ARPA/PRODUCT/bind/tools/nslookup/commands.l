%{

/*-
 * Copyright (c) 1985 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifdef hpux
static char rcsid[] = "$Header: commands.l,v 1.25.109.5 94/02/28 16:21:44 randyc Exp $";
#else
#ifndef lint
static char sccsid[] = "@(#)commands.l	5.13 (Berkeley) 7/24/90";
#endif /* not lint */
#endif /* hpux */

/*
 *******************************************************************************
 *
 *  commands.l
 *
 *	Andrew Cherenson	CS298-26	Fall 1985
 *	Govind Tatachari	HP-CNS		Oct 1993 (Modified)
 *
 *	Lex input file for the nslookup program command interpreter. When
 *	a sequence is recognized, the associated action routine is called.
 *	The action routine may need to parse the string for additional
 *	information.
 *
 *  Recognized commands: (identifiers are shown in uppercase)
 *
 *	exit		- exit the program
 *	finger [NAME]	- finger the optional NAME
 *	help		- print help information
 *	?		- print help information
 *	ls NAME		- list the domain NAME
 *	policy		- print switch policy info
 *	root		- set default server to the root
 *	server NAME	- set default server to NAME, using default server
 *	lserver NAME	- set default server to NAME, using initial server
 *	set OPTION	- set an option
 *	view FILE	- sorts and view the file with more
 *	NAME		- print info about the host/domain NAME
 *			  using default server.
 *	NAME1 NAME2	- as above, but use NAME2 as server
 *
 *
 *   yylex Results:
 *	0		upon end-of-file.
 *	1		after each command.
 *
 *******************************************************************************
 */

#include <switch.h>
#include "res.h"
extern char rootServerName[];

#ifdef hpux
extern int  lookup_type;

struct  commands  {
	void	(*SetDefaultServer)();
	void	(*ViewList)();
	void	(*ListHosts)();
	void	(*SetOption)();
	void	(*PrintHelp)();
	void	(*LookupHost)();
	void	(*LookupHostWithServer)();
	void    (*ListHostsByType)();
	void    (*Finger)();
	void	(*PrintPolicy)();
	void	(*Reset)();
} ;

void	SetDefaultServer(), ViewList(), ListHosts(), SetOption(),
	PrintHelp(), PolicyLookupHost(), LookupHostWithServer(), 
	ListHostsByType(), Finger(), Reset();
void	PrintPolicy(), Setswt();
void	PrintHostTableHelp(), print_error();

struct  commands commands[HOSTTABLE+1] =  {
	{
		SetDefaultServer, ViewList, ListHosts, SetOption, PrintHelp,
		PolicyLookupHost, LookupHostWithServer, ListHostsByType, Finger, PrintPolicy, Reset
	},
	{
		SetDefaultServer, print_error, print_error, Setswt, PrintHostTableHelp,
		PolicyLookupHost, print_error, print_error, print_error, PrintPolicy, print_error
	},
	{
		SetDefaultServer, print_error, print_error, Setswt, PrintHostTableHelp,
		PolicyLookupHost, print_error, print_error, print_error, PrintPolicy, print_error
	},
} ;

void
print_error()
{
	fprintf(stderr, "*** Command not supported for ");
	switch(lookup_type){
		case NAMESERVER: fprintf(stderr, 
					"name server lookups\n"); break;
		case YP        : fprintf(stderr, 
					"NIS lookups\n"); break;
		case HOSTTABLE : fprintf(stderr, 
					"/etc/hosts lookups\n"); break;
	}
}

#endif

%}
WS	[ \t]
FLET	[A-Za-z0-9.*\\]
LET	[A-Za-z0-9.*]
NAME	[A-Za-z0-9.*=_/-]
%%
^{WS}*exit{WS}*$			{
					    return(0);
					}
^{WS}*finger({WS}+{LET}{NAME}*)?{WS}+>>?{WS}*{NAME}+{WS}*$	{
					    /*
					     * 2nd arg.
					     *  0 == output to stdout
					     *  1 == output to file
					     */
#ifdef hpux
				(*commands[lookup_type].Finger)(yytext, 1);
#else
					    Finger(yytext, 1);
#endif
					    return(1);
					}
^{WS}*finger({WS}+{LET}{NAME}*)?{WS}*$	{
#ifdef hpux
				(*commands[lookup_type].Finger)(yytext, 0);
#else
					    Finger(yytext, 0);
#endif
					    return(1);
					}
^{WS}*help{WS}*$			{
					    extern void PrintHelp();

#ifdef hpux
				   	   (*commands[lookup_type].PrintHelp)();
#else
					    PrintHelp();
#endif
					    return(1);
					}
^{WS}*"?"{WS}*$				{
#ifdef hpux
				   	   (*commands[lookup_type].PrintHelp)();
#else
					    PrintHelp();
#endif
					    return(1);
					}
^{WS}*ls{WS}+(("-a"|"-d"|"-h"|"-m"|"-s"){WS}+)?{LET}{NAME}*{WS}+>>?{WS}*{NAME}+{WS}*$	{

/**** BSD method insists on a space after the redirection '>' symbol 
^{WS}*ls{WS}+(("-a"|"-d"|"-h"|"-m"|"-s"){WS}+)?{LET}{NAME}*{WS}+>>?{WS}+{NAME}+{WS}*$	{
****/
					    /*
					     * 2nd arg.
					     *  0 == output to stdout
					     *  1 == output to file
					     */
#ifdef hpux
				  (*commands[lookup_type].ListHosts)(yytext, 1);
#else
					    ListHosts(yytext, 1);
#endif
					    return(1);
					}
^{WS}*ls{WS}+(("-a"|"-d"|"-h"|"-m"|"-s"){WS}+)?{LET}{NAME}*{WS}*$	{
#ifdef hpux
				  (*commands[lookup_type].ListHosts)(yytext, 0);
#else
					    ListHosts(yytext, 0);
#endif
					    return(1);
					}
^{WS}*ls{WS}+-t{WS}+({LET}{NAME}*{WS}+)?{LET}{NAME}*{WS}+>>?{WS}*{NAME}+{WS}*$ {

/**** BSD method insists on a space after the redirection '>' symbol 
^{WS}*ls{WS}+-t{WS}+({LET}{NAME}*{WS}+)?{LET}{NAME}*{WS}+>>?{WS}+{NAME}+{WS}*$ {
****/
					    /*
					     * 2nd arg.
					     *  0 == output to stdout
					     *  1 == output to file
					     */
#ifdef hpux
			(*commands[lookup_type].ListHostsByType)(yytext,1);
#else
					    ListHostsByType(yytext, 1);
#endif
					    return(1);
					}
^{WS}*ls{WS}+-t{WS}+({LET}{NAME}*{WS}+)?{LET}{NAME}*{WS}*$ {
#ifdef hpux
			(*commands[lookup_type].ListHostsByType)(yytext,0);
#else
					    ListHostsByType(yytext, 0);
#endif
					    return(1);
					}
^{WS}*policy{WS}*$			{
#ifdef hpux
					  (*commands[lookup_type].PrintPolicy)();
#endif
					  return(1);
					}
^{WS}*root{WS}*$			{
#ifdef hpux
		   (*commands[lookup_type].SetDefaultServer)(rootServerName, 1);
#else
					    SetDefaultServer(rootServerName, 1);
#endif
					    return(1);
					}
^{WS}*server{WS}+{LET}{NAME}*{WS}*$	{
					    /*
					     * 0 == use current server to find
					     *	    the new one.
					     * 1 == use original server to find
					     *	    the new one.
					     */
#ifdef hpux
		 	(*commands[lookup_type].SetDefaultServer)(yytext, 0);
#else
					    SetDefaultServer(yytext, 0);
#endif
					    return(1);
					}
^{WS}*lserver{WS}+{LET}{NAME}*{WS}*$	{
#ifdef hpux
			(*commands[lookup_type].SetDefaultServer)(yytext, 1);
#else
					    SetDefaultServer(yytext, 1);
#endif
					    return(1);
					}
^{WS}*reset{WS}*$			{
					(*commands[lookup_type].Reset)();
					return(1);
					}
^{WS}*set{WS}+{NAME}+{WS}*$		{
#ifdef hpux
				   (*commands[lookup_type].SetOption)(yytext);
#else
					    SetOption(yytext);
#endif
					    return(1);
					}
^{WS}*view{WS}+{NAME}+{WS}*$	{
#ifdef hpux
				   (*commands[lookup_type].ViewList)(yytext);
#else
					    ViewList(yytext);
#endif
					    return(1);
					}
^{WS}*{FLET}{NAME}*{WS}+>>?{WS}*{NAME}+{WS}*$	{
					    /*
					     * 0 == output to stdout
					     * 1 == output to file
					     */
#ifdef hpux
			   (*commands[lookup_type].LookupHost)(yytext, 1);
#else
					    LookupHost(yytext, 1);
#endif
					    return(1);
					}
^{WS}*{FLET}{NAME}*{WS}*$		{
#ifdef hpux
			   (*commands[lookup_type].LookupHost)(yytext, 0);
#else
					    LookupHost(yytext, 0);
#endif
					    return(1);
					}
^{WS}*{FLET}{NAME}*{WS}+{LET}{NAME}*{WS}+>>?{WS}*{NAME}+{WS}*$	{
					    /*
					     * 0 == output to stdout
					     * 1 == output to file
					     */
#ifdef hpux
		   (*commands[lookup_type].LookupHostWithServer)(yytext, 1);
#else
					    LookupHostWithServer(yytext, 1);
#endif
					    return(1);
					}
^{WS}*{FLET}{NAME}*{WS}+{LET}{NAME}*{WS}*$	{
#ifdef hpux
		   (*commands[lookup_type].LookupHostWithServer)(yytext, 0);
#else
					    LookupHostWithServer(yytext, 0);
#endif
					    return(1);
					}
^{WS}*\n				{
					    return(1);
					}
^.*\n					{
					    printf("Unrecognized command: %s",
							yytext);
					    return(1);
					}
\n					{ ; }
%%
