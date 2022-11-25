%{
#undef input
#undef unput
#undef output
#undef YYLMAX

#include "defines.h"
#include "global.h"


%}
SPACE	[ \t\f\001]
ID	([_A-Za-z][_0-9A-Za-z]*)
NUMBER	([0-9]*)
PARENS	"("([^&{};])*")"

%{
/************************************************************************/
/*   Different Lexical Analyzer States					*/
/*									*/
/*	L - Looking for function definition.				*/
/*	I - Inside function body looking for branches.			*/
/*	D - Inside an include file that is not to have branches BFAed.	*/
/*	P - Processing the formal arguments of function declaration.	*/
/*									*/
/************************************************************************/
%}

%Start L,I,D,P

%%

<D,L>"#"(({SPACE}*"line"{SPACE}*)|({SPACE}*)){NUMBER}{SPACE}*\"(.)*\"  {
		CheckFileAndSetLineNumber(yytext);
	}

<D,L>"#"(({SPACE}*"line"{SPACE}*)|({SPACE}*)){NUMBER}	{
		SetLineNumber(yytext);
	}

<L>("*"|{ID}|{SPACE})*{PARENS}+{SPACE}*(([^0-9,;=\+\-\/\*\%\<\>\!\?\~\^\&\|\.()\[\]\n \f\t](.)*"{")|("{")) {
		ProcessNewFunction(yytext);
	}

<I,L>\"(\\.|[^\\"\n])*\"	{
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<I,L>"'"(\\.|[^\\'\n])*"'"	{
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<I>"if"	{
		ProcessIf();
	}

<I>"else"{SPACE}+"if"[^A-Za-z0-9_] 	{
		unput(yytext[strlen(yytext)-1]);
		ProcessElse(_ELSEIF);
	}

<I>"else"	{
		ProcessElse(_ELSE);
	}

<I>"do"	{
		ProcessDo();
	}

<I>"for"	{
		ProcessFor();
	}

<I>"while"	{
		ProcessWhile();
	}

<I>"switch"	{
		ProcessSwitch();
	}

<I>"case"	{
		ProcessCase();
	}

<I>"return"	{
		CheckReturn();
	}

<I>{ID}{SPACE}*":"	{
		ProcessLabel(yytext);
	}

<I>"{"	{
		ProcessLeftCurl();
	}

<I>"}"	{
		ProcessRightCurl();
	}

<I>";"	{
		ProcessSemiColon();
	}

<I>"?"	{
		ProcessQuestion();
	}

<I>":"	{
		ProcessColon();
	}

<I>{ID}	{
		ProcessId(yytext);
	}

<L>{ID}	{
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<P>"struct" {
	   	specifier++;
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<P>"union" {
	   	specifier++;
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<P>"enum" {
	   	specifier++;
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<P>";" {
		if(specifier)
		   specifier--;

		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<P>"{" {
		if(specifier) {
		   specifier--;

		   ConvertString(yytext);
		   fprintf(outfile,"%s",yytext);
		} else  {
		   ProcessFunctionAfterParams(yytext);
		} 
	}

<P>{ID}	{
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<P>[\n]	{
		/* Do Nothing */
	}

<P>.	{
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}

<L,I>[\n]	{
		/* Do Nothing */
	}

<L,I,D>.	{
		ConvertString(yytext);
		fprintf(outfile,"%s",yytext);
	}
%%



static int old_state = STATE_L;

void InitScanner()
{
	BEGIN	L;

	old_state   = STATE_L;
	semi_cr     = 0;
	line_number = 1;
	else_pos    = 0;
	stream_pos  = 0;
	quest_count = 0;

} /* InitScanner */


void BeginScannerState(state)
int	state;
{
	switch(state) {
	   case STATE_I	:
			   old_state = STATE_I;
			   BEGIN I;
			   break;
	   case STATE_L	:
			   old_state = STATE_L;
			   BEGIN L;
			   break;
	   case STATE_D	:
			   BEGIN D;
			   break;
	   case STATE_P	:
			   BEGIN P;
			   break;
	   default	:
			   fprintf(stderr,"BFA Internal Error ");
			   fprintf(stderr,"Unkown scanner state\n");
			   exit(1);
	} /* switch */
} /* BeginScannerState */


void ReturnToOldState() {
	BeginScannerState(old_state);	
} /* RetrunToOldState() */
