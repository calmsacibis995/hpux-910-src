/* @(#) $Revision: 66.4 $ */
D                       [0-9]
L                       [a-zA-Z_]
H                       [a-fA-F0-9]
E                       [Ee][+-]?{D}+
FS                      (f|F|l|L)
IS                      (u|U|l|L)*

%{
#include "tab.h"
# define TRUE    1
# define output(c) putc(c,yyout)
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
#undef ECHO
# define ECHO  {} 
extern int BufMax;
int            column=0;
void           count(),count2();
extern char    *Buffer;
extern int     PPCommandFlag,
               Count;
extern         ClearBuffer();
extern FILE    *ReportFile,
               *OutFile,
               *ProtoTypeFile;
extern int     GetToken();
extern void    Debug(),Error();

%}

%%
"/*"                    { comment(); }

"auto"                  { count(); return(AUTO); }
"break"                 { count(); return(BREAK); }
"case"                  { count(); return(CASE); }
"char"                  { count(); return(CHAR); }
"const"                 { count(); return(CONST); }
"continue"              { count(); return(CONTINUE); }
"default"               { count(); return(DEFAULT); }
"do"                    { count(); return(DO); }
"double"                { count(); return(DOUBLE); }
"else"                  { count(); return(ELSE); }
"enum"                  { count(); return(ENUM); }
"extern"                { count(); return(EXTERN); }
"float"                 { count(); return(FLOAT); }
"for"                   { count(); return(FOR); }
"goto"                  { count(); return(GOTO); }
"if"                    { count(); return(IF); }
"int"                   { count(); return(INT); }
"long"                  { count(); return(LONG); }
"register"              { count(); return(REGISTER); }
"return"                { count(); return(RETURN); }
"short"                 { count(); return(SHORT); }
"signed"                { count(); return(SIGNED); }
"sizeof"                { count(); return(SIZEOF); }
"static"                { count(); return(STATIC); }
"struct"                { count(); return(STRUCT); }
"switch"                { count(); return(SWITCH); }
"typedef"               { count(); return(TYPEDEF); }
"union"                 { count(); return(UNION); }
"unsigned"              { count(); return(UNSIGNED); }
"void"                  { count(); return(VOID); }
"volatile"              { count(); return(VOLATILE); }
"while"                 { count(); return(WHILE); }
"include"               { count(); return(INCLUDE); }
"ifdef"                 { count(); return(IFDEF); }
"ifndef"                { count(); return(IFNDEF); }
"endif"                 { count(); return(ENDIF); }
"asm"                   { count(); return(ASM); }
"..."                   { count(); return(ELIPSIS); }
{L}({L}|{D})*           { count(); return(IDENTIFIER); }

0[xX]{H}+{IS}?          { count(); return(CONSTANT); }
0[xX]{H}+{IS}?          { count(); return(CONSTANT); }
0{D}+{IS}?              { count(); return(CONSTANT); }
0{D}+{IS}?              { count(); return(CONSTANT); }
{D}+{IS}?               { count(); return(CONSTANT); }
{D}+{IS}?               { count(); return(CONSTANT); }
'(\\.|[^\\'])+'         { count2(); return(CONSTANT);}

{D}+{E}{FS}?            { count(); return(CONSTANT); }
{D}*"."{D}+({E})?{FS}?  { count(); return(CONSTANT); }
{D}+"."{D}*({E})?{FS}?  { count(); return(CONSTANT); }

\"(\\(.|\n)|[^\\"])*\"  { count2(); return(STRING_LITERAL); }
"\n#"                   { count(); return(PP_COMMAND); }
"#"                     { count(); return('#'); }
">>="                   { count(); return(RIGHT_ASSIGN); }
"<<="                   { count(); return(LEFT_ASSIGN); }
"+="                    { count(); return(ADD_ASSIGN); }
"-="                    { count(); return(SUB_ASSIGN); }
"*="                    { count(); return(MUL_ASSIGN); }
"/="                    { count(); return(DIV_ASSIGN); }
"%="                    { count(); return(MOD_ASSIGN); }
"&="                    { count(); return(AND_ASSIGN); }
"^="                    { count(); return(XOR_ASSIGN); }
"|="                    { count(); return(OR_ASSIGN); }
">>"                    { count(); return(RIGHT_OP); }
"<<"                    { count(); return(LEFT_OP); }
"++"                    { count(); return(INC_OP); }
"--"                    { count(); return(DEC_OP); }
"->"                    { count(); return(PTR_OP); }
"&&"                    { count(); return(AND_OP); }
"||"                    { count(); return(OR_OP); }
"<="                    { count(); return(LE_OP); }
">="                    { count(); return(GE_OP); }
"=="                    { count(); return(EQ_OP); }
"!="                    { count(); return(NE_OP); }
";"                     { count(); return(';'); }
"{"                     { count(); return('{'); }
"}"                     { count(); return('}'); }
","                     { count(); return(','); }
":"                     { count(); return(':'); }
"="                     { count(); return('='); }
"("                     { count(); return('('); }
")"                     { count(); return(')'); }
"["                     { count(); return('['); }
"]"                     { count(); return(']'); }
"."                     { count(); return('.'); }
"&"                     { count(); return('&'); }
"!"                     { count(); return('!'); }
"~"                     { count(); return('~'); }
"-"                     { count(); return('-'); }
"+"                     { count(); return('+'); }
"*"                     { count(); return('*'); }
"/"                     { count(); return('/'); }
"%"                     { count(); return('%'); }
"<"                     { count(); return('<'); }
">"                     { count(); return('>'); }
"^"                     { count(); return('^'); }
"|"                     { count(); return('|'); }
"?"                     { count(); return('?'); }
"\\"                    { count(); return('\\'); }
[ \t\v\n\f]             { count(); }
.                       { /* ignore bad characters */}

%%

yywrap()
{
        return(1);
}
/*-----------------------------------------------------------------------*/
comment()
{
        char c, c1;
        Buffer[Count++]='/';
        Buffer[Count++]='*';
loop:
        while ((c = input()) != '*' && c != 0){
          Buffer[Count++]=c;
	  if (Count>BufMax-5){
	    if ((Buffer = (char *) realloc(Buffer,BufMax *= 1.5)) == NULL)
	      Error(1,"cannot malloc internal buffer");
	  }
	}
        Buffer[Count++]='*';

        if ((c1 = input()) != '/' && c != 0)
        {
                unput(c1);
                goto loop;
        }

        if (c != 0)
           Buffer[Count++]='/';
}
/*-----------------------------------------------------------------------*/
void count()
{
        int i;

        for (i = 0; yytext[i] != '\0'; i++)
            {
             if (((yytext[i]=='\n')&&(yytext[i+1]!='#'))||
		 (yytext[i]=='\t')||(yytext[i]==' ')||
		 (yytext[i]==12))
                Buffer[Count++]=yytext[i];

             if (yytext[i] == '\n')
                     column = 0;
             else if (yytext[i] == '\t')
                     column += 8 - (column % 8);
             else
                     column++;
            }
        ECHO;
}
/*-----------------------------------------------------------------------*/
void count2()
{
 int i;
 
        for (i = 0; yytext[i] != '\0'; i++)
            {
             if (yytext[i] == '\n')
                     column = 0;
             else if (yytext[i] == '\t')
                     column += 8 - (column % 8);
             else
                     column++;
            }
        ECHO;
}
/*------------------------------------------------------------------------*/
GetPreProcessorCommand(Dump)
{
 char Last=' ',c;
 if (Dump)
    DumpBuffer();
 while ((((c=input())!='\n')||(Last=='\\'))&&(c!=EOF))
  {
   if (Dump)
      fputc(c,OutFile);
   else
      Buffer[Count++]=c;
   if ((Last=='/')&&(c=='*'))
      {
       while(((c=input())!='/')||(Last!='*'))
         {
          if (Dump)
             fputc(c,OutFile);
          else
             Buffer[Count++]=c;
          Last=c;
         }
       if (Dump)
          fputc(c,OutFile);
       else
          Buffer[Count++]=c;
      } 
   Last=c;
  }
 if (c=='\n')
   unput(c) 
 else
   if (c!=EOF)
      if (Dump)
         fputc(c,OutFile);
      else
         Buffer[Count++]=c;
}
/*-----------------------------------------------------------------------*/
GetAsmCommand()
{
 char c;
 if (GetToken()!='(')
    Error(TRUE,"incorrect syntax for asm command");
 DumpBuffer();
 while ((c=input())!='\"') fputc(c,OutFile);
 fputc(c,OutFile);
 while ((c=input())!='\"') fputc(c,OutFile);
 fputc(c,OutFile);
 if (GetToken()!=')')
    Error(TRUE,"incorrect syntax for asm command");
 DumpBuffer();
}




