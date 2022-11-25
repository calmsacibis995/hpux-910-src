/* RCS revision: @(#) $Revision: 70.2 $ 
*/
/****************************************************************************/
/* PROTOGEN                                                                 */
/*                                                                          */
/* Mark Rowland           School:Room 1228-O                                */
/* 7366 Marywood Dr.             Hawkins Graduate House                     */
/* Newburgh, IN 47630            Purdue University                          */
/* 812-853-2249                  West Lafayette, IN 47906                   */
/*                               317-495-7619                               */
/* Date      Description of modifications                                   */
/* ------    -------------------------------------------------------------  */
/* 920420    DTS # CLLbs00179 protogen(1) dies with "internal sync error"   */
/* A.Datla   on compilable file                                             */
/*           protogen(1) calls cpp (the C language preprocessor) to process */
/*           include files. If the c files has too many macro definitions   */ 
/*           then the default macro table size will overflow.  To aviod this*/
/*           situation cpp accepts the option "-H", see cpp man page for    */
/*           details. Protogen is enhanced to accpet "-H" option.           */
/*                                                                          */
/****************************************************************************/

#include        <stdio.h>
#include        <string.h>
#include        <setjmp.h>
#include	<unistd.h>
#include	<sys/param.h>
#undef	FLOAT
#include        "tab.h"  
#define         TokensMax          500
#define         UnGetMax             5
#define         IncludeFileMax      20
#define         LineBufferMax      256

#define         TRUE                 1
#define         FALSE                0

#define         PARAMVAR          (-5)
#define         FUNCNAME          (-6)
#define         CONVERTEDPARAM    (-7)
#define         WhiteSpace(c)     (((c==' ')||(c=='\t')||  \
				  (c=='\n'))?TRUE:FALSE)
#define         TypeSpecifier(c)  (c>=EXTERN)&&(c<=ENUM)

int		BufMax = 2000;
int             PPCommandFlag=TRUE,
                PPCommandInFuncDeclaration=FALSE,
                ErrorInDeclaration=FALSE,
                DefineFlag=FALSE,
                IncludeLevel=0,
                IfDefLevel=0,
                NoOutput,
                Token,
                Count=0,
                TokenList[TokensMax],
                TokenPos[TokensMax],
                StackPtr=0,
                UnGottenTokens=0,
                TokenBuf[UnGetMax],
                CPPLineNo,
                CPPLineNoBuffer;
jmp_buf         IncludeJumpStack[IncludeFileMax],
                NextFileJmp;
#ifdef OSF
#ifdef PAXDEV
char		CppCommand[LineBufferMax]="/paXdev/lbin/cpp.trad -Q ",
#else
char		CppCommand[LineBufferMax]="/usr/ccs/lib/cpp.trad -Q ",
#endif /* PAXDEV */
#else
char            CppCommand[LineBufferMax]="/lib/cpp -Q ",
#endif /* OSF */
                *Buffer,
                SynchByte[3],
                *yytextBuf[UnGetMax],
                FileName[LineBufferMax]="",
                OutputFileName[LineBufferMax]="";
int             AddImpliedInts = FALSE,
                WidenParams    = FALSE;
FILE            *ReportFile=NULL,
                *OutFile,
                *ProtoTypeFile,
                *IfDefFile;

#ifdef LINTMAKER
int 		lintflag = FALSE;
char		FuncType[LineBufferMax];
#endif /* LINTMAKER */

extern char     yytext[];
extern int      yylex(),yylineno;
extern char     *optarg;
extern int      optind,errno;
extern void     GetPreProcessorCommand(),GetAsmCommand();
extern FILE     *yyin;
/*--------------------------------------------------------------------------*/
/* DEBUG                                                                    */
/*       Error:  title for debug report                                     */
/*                                                                          */
/* This procedure prints out the contents of the buffer, token values,      */
/* and token positions it titles the printout with the given string Error.  */
/*--------------------------------------------------------------------------*/
Debug(Error)
char *Error;
{
 int Temp;
 printf("________________%s____________\n",Error);
 printf("|%s| <%s> ",Buffer,yytext);
 printf("\nCount %d \n",Count);
 for (Temp=1; Temp<=TokenPos[0];Temp++)
     printf("|%d",TokenPos[Temp]);
 printf("\n");
 for (Temp=1; Temp<=TokenList[0];Temp++)
     printf("|%d",TokenList[Temp]);
 printf("\n");
 fflush(OutFile);
}
/*--------------------------------------------------------------------------*/
/* CLEAR BUFFER                                                             */
/*                                                                          */
/* This procedure clears the buffer.                                        */
/*--------------------------------------------------------------------------*/
ClearBuffer()
{
 TokenList[0]=0;
 TokenPos[0]=0;
 Count=0;
 Buffer[0]=NULL;
}
/*--------------------------------------------------------------------------*/
/* DUMP BUFFER                                                              */
/*                                                                          */
/* This procedure dumps the buffer's contents to Outfile and then clears it.*/
/*--------------------------------------------------------------------------*/
DumpBuffer()
{
#ifdef SEEBUFFER
 printf("-------------------------\n%s\n",Buffer);
#endif
 fputs(Buffer,OutFile);
 ClearBuffer();
}
/*--------------------------------------------------------------------------*/
/* END OF FILE                                                              */
/*            ERROR  TRUE/FALSE was an error the cause of the termination   */
/* This procedure is called whenever the end of an input file is reached.   */
/* If it was an include file the previous file is retrieved and traversered.*/
/*--------------------------------------------------------------------------*/
EndOfFile(Error)
int       Error;
{
#ifdef DEBUG
 printf("EndOfFile\n");
#endif
 DumpBuffer();
 fclose(OutFile);
 if ((!Error)&&(NoOutput))
   {
    unlink(OutputFileName);
    printf("no change    %s\n",FileName);
   }
 else
    if (!Error)
       printf("generating   %s\n",OutputFileName);
    else
       unlink(OutputFileName);
       
 if ((IncludeLevel)&&(!Error)) 
   longjmp(IncludeJumpStack[--IncludeLevel],TRUE);
 fscanf(IfDefFile,"%s %d %s ",SynchByte,&CPPLineNo,FileName);
 if ((!Error)&&(fscanf(IfDefFile,"%s",CppCommand)>0))
    { 
     printf("Error: protogen synch error\n");
     printf("%s %d %s \n",SynchByte,CPPLineNo,FileName);
    }
 if (!Error)
    longjmp(NextFileJmp,1);
#ifdef LINTMAKER
 if (!lintflag)  fclose(ProtoTypeFile);
#else
 fclose(ProtoTypeFile);
#endif /* LINTMAKER */
 fclose(ReportFile);
 exit(1);
}
/*--------------------------------------------------------------------------*/
/* ERROR                                                                    */
/*       Severity  TRUE/FALSE should termination occur                      */
/*       Message   Error message sent to stderr                             */
/* This procedure is the standard error routine which prints the given error*/
/* message with the filename and current line number. If Severity is TRUE   */
/* the program terminates, otherwise control is returned to the caller.     */
/*--------------------------------------------------------------------------*/
Error(Severity,Message)
int    Severity;
char   *Message;
{
#ifdef DEBUG
 printf("ERROR\n");
#endif
 fprintf(ReportFile,"ERROR: %s\n",Message);
 fprintf(ReportFile,"File:  %s  Line : %d\n",FileName,yylineno);
 if (Severity)
   EndOfFile(TRUE);
}
/*--------------------------------------------------------------------------*/
/* BSTRCMP                                                                  */
/*        STRING1                                                           */
/*        STRING2                                                           */
/* This function returns true if the tail portion of STRING1 is identical to*/
/* STRING2                                                                  */
/*--------------------------------------------------------------------------*/
int bstrcmp(s1,s2)
char  *s1,*s2;
{
 int Pos =strlen(s1)-1,
     Halt=Pos-strlen(s2);
 while ((Pos>Halt)&&(s1[Pos]==s2[Pos-Halt-1]))
     --Pos;
 if (Pos==Halt)
    return(TRUE);
 return(FALSE);
}
/*--------------------------------------------------------------------------*/
/* GET TOKEN 1                                                              */
/* This function calls yylex and copies yytext into the buffer. It also     */
/* updates the list of tokens and positions in the buffer. It then returns  */
/* the value of the token recieved. All whitespace found is joined to the   */
/* end of the previous token by Count() in lex.c.                           */
/*--------------------------------------------------------------------------*/
int GetToken1()
{
 int  Temp,i,NewCount;

 if (UnGottenTokens)
  {
   Token=TokenBuf[0];
   strcpy(yytext,yytextBuf[0]);
   free(yytextBuf[0]);
   --UnGottenTokens;
   for (i=UnGottenTokens; i>0 ; i--)
    {
     TokenBuf[i-1]=TokenBuf[i];
     yytextBuf[i-1]=yytextBuf[i];
    };
  }
 else
   Token=yylex();
 if ((PPCommandFlag)&&(Token=='#'))
    Token=PP_COMMAND;
 PPCommandFlag=FALSE;
 TokenList[++TokenList[0]]=Token;
 TokenPos[++TokenPos[0]]=Count;
 if ((NewCount = Count + strlen(yytext)) >= BufMax)
     {
	 if ((Buffer = (char *) realloc(Buffer,BufMax *= 1.5)) == NULL)
	     Error(TRUE,"cannot malloc internal buffer");
     }
 strcpy(&Buffer[Count],yytext);
 Count = NewCount;
 if (!Token)
    EndOfFile(FALSE);
 return (Token);
}
/*--------------------------------------------------------------------------*/
/* GET TOKEN                                                                */
/* This function simply calls GetToken1() and filters out any preprocessor  */
/* commands returned.                                                       */
/*--------------------------------------------------------------------------*/
int GetToken()
{
 while ((GetToken1()==PP_COMMAND)||(Token==ASM))
   if (Token==ASM)
      GetAsmCommand();
   else
     ProcessPPCommand();
 return(Token);
}
/*--------------------------------------------------------------------------*/
/* UnGET TOKEN                                                              */
/* This function removes a gotten token from the buffer and stores it for   */
/* regetting later. It removes all pointers and token values from the       */
/* proper lists.                                                            */
/*--------------------------------------------------------------------------*/
UnGetToken()
{
 int   Temp,i;

 Temp=strlen(yytext);

 for (i=UnGottenTokens; i>0 ; --i)
  {
   TokenBuf[i]=TokenBuf[i-1];
   yytextBuf[i]=yytextBuf[i-1];
  };
  Temp=Count-TokenPos[TokenPos[0]];
  TokenBuf[0]=Token;
  yytextBuf[0]=(char *)malloc(Temp+1);
  strcpy(yytextBuf[0],yytext);
  Count -=Temp;
  Buffer[Count]=NULL;
  ++UnGottenTokens;
  --TokenList[0];
  --TokenPos[0];
}
/*--------------------------------------------------------------------------*/
/* DEL BUFFER                                                               */
/*           Position actual character position in the buffer               */
/*           Length  number of characters to remove                         */
/* This function removes characters from the buffer                         */
/*--------------------------------------------------------------------------*/
DelBuffer(Position,Length)
int Position,Length;
{
 int i;
 for (i=Position; i+Length<=Count; i++)
   Buffer[i]=Buffer[i+Length];
 Count-=Length;
}
/*--------------------------------------------------------------------------*/
/* DEL TOKEN IN BUFFER                                                      */
/*           Position actual character position in the buffer               */
/*           Length  number of characters to remove                         */
/* This function removes a token from the buffer.                           */
/*--------------------------------------------------------------------------*/
DelTokenInBuffer(TokenNum)
{
 int i=TokenNum,
     Length;
 if (TokenNum>=TokenList[0])
    Length=Count-TokenPos[TokenNum];
 else
    Length=TokenPos[TokenNum+1]-TokenPos[TokenNum];
 DelBuffer(TokenPos[TokenNum],Length);
 for(i=TokenNum+1; i<=TokenList[0]; i++)
    {
     TokenPos[i-1]=(TokenPos[i]-=Length);
     TokenList[i-1]=TokenList[i];
    }
 TokenPos[0]--;
 TokenList[0]--;
}
/*--------------------------------------------------------------------------*/
/* DEL UNTIL RIGHT BRACE                                                    */
/*     Start actual character position of the first left brace              */
/*     Flag  TRUE/FALSE if True all tokens between the braces are removed   */
/* IF this function finds the matching brace to the given one in the buffer.*/
/* the flag is TRUE all all characters between the braces are deleted. This */
/* function is used to create prototypes.                                   */
/*--------------------------------------------------------------------------*/
int DelUntilRightBrace(Start,Flag)
int Start,Flag;
{
 int Position=0;
 while (Buffer[Start+Position]!='}')
  {
   ++Position;
   if (Buffer[Start+Position-1]=='{')
      Position+=DelUntilRightBrace(Start+Position,TRUE);
  }
 if (!Flag)
    DelBuffer(Start-1,Position+2);
 return(Position+1);
}
/*--------------------------------------------------------------------------*/
/* DEL BETWEEN BRACES                                                       */
/* This function deletes all charaters between sets of braces in the buffer.*/
/*--------------------------------------------------------------------------*/
DelBetweenBraces()
{
 int i= -1;
 while (++i<Count-1)
   if (Buffer[i]=='{')
      DelUntilRightBrace(i+1,0);
   else
     if ((Buffer[i-1]=='/')&&(Buffer[i]=='*'))
        while (((Buffer[i-1]!='*')||(Buffer[i]!='/'))&&(++i<Count-1));
}
/*--------------------------------------------------------------------------*/
/* COPY TO BUFFER                                                           */
/*         Position actual character position in the buffer to where the    */
/*                  string is to be copied                                  */
/*         String the string to be copied into the buffer                   */
/* This function copies a given string to the buffer without insertion.     */
/*--------------------------------------------------------------------------*/
CopyToBuffer(Position,String)
int    Position;
char   String[];
{
 int i= -1,
     Length=strlen(String);
 while (++i<Length)
    Buffer[Position+i]=String[i];
}
/*--------------------------------------------------------------------------*/
/* CHANGE TOKEN                                                             */
/*     TokenNum index of token to be changed                                */
/*     Length   the amount of characters to be changed                      */
/*     TokenType Type of the Token being changed                            */
/* This function is used by widen parameters to change short, char and      */
/* float tokens to int and double respectivly.                              */
/*--------------------------------------------------------------------------*/
ChangeToken(TokenNum,Length,TokenType)
{
 int i;
 DelBuffer(TokenPos[TokenNum],Length);
 for (i=TokenNum+1; i<=TokenList[0]; i++)
     TokenPos[i]-=Length;
 switch(TokenType) {
    case(INT)     :InsertBuf(TokenNum,3);
                   CopyToBuffer(TokenPos[TokenNum],"int");
                   TokenList[TokenNum]=INT;
                   break;
    case(DOUBLE)  :InsertBuf(TokenNum,6);
                   CopyToBuffer(TokenPos[TokenNum],"double");
                   TokenList[TokenNum]=DOUBLE;
                   break;
    }
}
/*--------------------------------------------------------------------------*/
/* WIDEN PARAMETERS                                                         */
/*     ParamID  token position of parameter to be widened                   */
/*     Pos      token postion of the first type specifier of the parameter  */
/* This function given ParamID, checks the parameter to see if it should be */
/* widened, if so then it changes the appropiate tokens respectivly.        */
/*--------------------------------------------------------------------------*/
WidenParameters(ParamID,Pos)
{
 int Flag=TRUE, Pos2=Pos;
#ifdef DEBUG
 printf("Widen Parameters\n");
 Debug("WIDENPARAM");
 printf("%d %d %d %d \n",Pos,ParamID,
	 TokenList[ParamID+1],TokenList[ParamID-1]);
#endif
 
 if ((TokenList[ParamID-1]=='*')||
     ((TokenList[ParamID+1]!=',')&&(TokenList[ParamID+1]!=';')))
    return;
 while ((Pos<TokenList[0])&&(TokenList[Pos]!='{'))
   {
    switch(TokenList[Pos]) {
       case(CHAR)  :ChangeToken(Pos,4,INT);
                    break;
       case(SHORT) :for (;(Pos2<TokenList[0])&&(TokenList[Pos2]!='{');Pos2++)
	              if (TokenList[Pos2]==INT)
		         {
			  DelTokenInBuffer(Pos);
			  return;
		         }
	            ChangeToken(Pos,5,INT);
                    break;
       case(FLOAT) :ChangeToken(Pos,5,DOUBLE);
                    break;
       default     :break;
       }
    Pos++;
   }
}
/*--------------------------------------------------------------------------*/
/* ADD TO PROTOTYPE FILE                                                    */
/* This function assumes that a valid prototype is in the buffer and then   */
/* it adds a semicolon and formats it, and sends it to the prototype file.  */
/*--------------------------------------------------------------------------*/
 AddToProtoTypeFile()
{
 int i=1;
 NoOutput=FALSE;
 if (PPCommandInFuncDeclaration)
    {
     for (; (Buffer[i])&&((Buffer[i]!='\n')||(Buffer[i+1]!='#')); i++);
     if (Buffer[i+1]=='#')
        {
         Buffer[i+1]=NULL;
         Count=strlen(Buffer);
         PPCommandInFuncDeclaration=FALSE;
        }
    }
 DelBuffer(Count-1,1);
 while (WhiteSpace(Buffer[Count-1]))
  DelBuffer(Count-1,1); 
 DelBetweenBraces();
 strcat(Buffer,";\n");
#ifdef LINTMAKER
 if (!lintflag) fputs(Buffer,ProtoTypeFile);
#else
 fputs(Buffer,ProtoTypeFile);
#endif /* LINTMAKER */
 ClearBuffer();
}
/*--------------------------------------------------------------------------*/
/* READ UNTIL RIGHT BRACE                                                   */
/*     Dump  TRUE/FALSE Determines if buffer should be iteratively dumped   */
/* This function gets tokens from the input file until it finds the matching*/
/* right brace to one that had been read before the call to this function.  */
/*--------------------------------------------------------------------------*/
ReadUntilRightBrace(Dump)
int Dump;
{
 do {
  if (Dump)
     DumpBuffer();
  if (GetToken()=='{')
     {
      ReadUntilRightBrace(Dump);
      Token=' ';
     }
 }while (Token!='}');
}
/*--------------------------------------------------------------------------*/
/* READ UNTIL SEMICOLON                                                     */
/* This function reads input until a semicolon is obtained.                 */
/*--------------------------------------------------------------------------*/
ReadUntilSemiColon()
{
 while(GetToken()!=';')
    DumpBuffer();
}
/*--------------------------------------------------------------------------*/
/* GET IDENTIFIER                                                           */
/* This function tries to read an identifier only.                          */
/*--------------------------------------------------------------------------*/
GetIdentifier()
{
 if (GetToken()!=IDENTIFIER)
    UnGetToken();
}
/*--------------------------------------------------------------------------*/
/* MUST GET IDENTIFIER                                                      */
/* This function issues an error if next token read is not an identifier.   */
/*--------------------------------------------------------------------------*/
int MustGetIdentifier()
{
 if (GetToken()!=IDENTIFIER)
    {
     Error(FALSE,"tag must be given to structure/union/enum types");
     UnGetToken();
    }
}
/*--------------------------------------------------------------------------*/
/* GET BRACES                                                               */
/* This function will optionally read in a pair of matching braces if they  */
/* exist.                                                                   */
/*--------------------------------------------------------------------------*/
GetBraces()
{
 if (GetToken()=='{')
   ReadUntilRightBrace(FALSE);
 else
   UnGetToken();
}
/*--------------------------------------------------------------------------*/
/* INSERT DEF                                                               */
/*        TOKEN NUM     is the token index of the parameter inside the old  */
/*                      style C declaration                                 */
/*        PARAM DEF POS is the token index of the start of the definition   */
/*                      of the parameter                                    */
/*        PARAM ID      is the token index of the position of the id in the */
/*                      definition                                          */
/*      SAVE TOKEN TYPE the type of the new token to be produed from the    */
/*                      definition                                          */
/* This function actually moves the definition of an old style C declaration*/
/* into the new position of the prototype.                                  */
/*--------------------------------------------------------------------------*/
InsertDef(TokenNum,ParamDef,ParamID,SaveTokenType)
{
 int    i,
        InsertAmount=Count-TokenPos[ParamDef],
        ParamDefPos=TokenPos[ParamDef],
        VariableClass=TokenList[ParamDef];

 Buffer[TokenPos[TokenPos[0]]]=' ';
 for (i=Count; i>=TokenPos[TokenNum]; --i)
   Buffer[i+InsertAmount]=Buffer[i];
 for (i=0; i<InsertAmount; i++)
    Buffer[i+TokenPos[TokenNum]]=Buffer[i+ParamDefPos+InsertAmount];
 Count+=InsertAmount;
 DelBuffer(TokenPos[TokenNum]+InsertAmount,
           TokenPos[TokenNum+1]-TokenPos[TokenNum]);
 InsertAmount-=TokenPos[TokenNum+1]-TokenPos[TokenNum];
 for(i=TokenNum+1; i<=TokenList[0]; i++)
    TokenPos[i]+=InsertAmount;
 if ((SaveTokenType)&&(TokenList[TokenList[0]-1]!=']')&&
     (TokenList[ParamID-1]!='*'))
    TokenList[TokenNum]=VariableClass;
 else
    TokenList[TokenNum]=CONVERTEDPARAM;
}
/*--------------------------------------------------------------------------*/
/* EQUAL TOKENS                                                             */
/* This function compares two tokens (usually ids) and determines if they   */
/* are identical.                                                           */
/*--------------------------------------------------------------------------*/
int EqualTokens(Token1,Token2)
int    Token1,Token2;
{
 int i=0,
     Length=TokenPos[Token1+1]-TokenPos[Token1];
 while ((i<Length)
        &&(Buffer[TokenPos[Token1]+i]!='/')
        &&(Buffer[TokenPos[Token1]+i]!=' ')
        &&(Buffer[TokenPos[Token1]+i]!='\t')
        &&(Buffer[TokenPos[Token1]+i]!='\n'))
   if (Buffer[TokenPos[Token1]+i]!=Buffer[TokenPos[Token2]+i])
      return(FALSE);
   else
      ++i;
 i=0;
 Length=TokenPos[Token2+1]-TokenPos[Token2];
 while ((i<Length)
        &&(Buffer[TokenPos[Token2]+i]!='/')
        &&(Buffer[TokenPos[Token2]+i]!=' ')
        &&(Buffer[TokenPos[Token2]+i]!='\t')
        &&(Buffer[TokenPos[Token2]+i]!='\n'))
   if (Buffer[TokenPos[Token1]+i]!=Buffer[TokenPos[Token2]+i])
      return(FALSE);
   else
      ++i;
 return(TRUE);
}
/*--------------------------------------------------------------------------*/
/* INSERT BUF                                                               */
/*      Start Token  the token index of the token to recievce the insertion */
/*                   spaces                                                 */
/*      Length       number of blank spaces to insert to the buffer         */
/* This function takes a given token index and adds spaces in the buffer    */
/* location where the token resides.                                        */
/*--------------------------------------------------------------------------*/
InsertBuf(StartToken,Length)
int StartToken,Length;
{
 int i;
#ifdef DEBUG
 printf("InsertBuf\n");
#endif
 for(i= -2; i<=Count-TokenPos[StartToken]; i++)
    Buffer[Count+Length-i]=Buffer[Count-i];
 for(i=StartToken+1; i<=TokenPos[0]; i++)
    TokenPos[i]+=Length;
 Count+=Length;
}
/*--------------------------------------------------------------------------*/
/* FIND RIGHT BRACE                                                         */
/*      Position the character location in the buffer of the left brace to  */
/*               be matched.                                                */
/* This function finds the matching brace of the given left brace in the    */
/* buffer.                                                                  */
/*--------------------------------------------------------------------------*/
int FindRightBrace(Position)
{
#ifdef DEBUG
 printf("FindRightBrace\n");
#endif
 while ((Position<Count)&&(Buffer[Position]!='}'))
    if (Buffer[Position++]=='{')
        Position=FindRightBrace(Position);
 return(Position+1);
}
/*--------------------------------------------------------------------------*/
/* FORMAT BUFFER                                                            */
/* This function assumes that a valid prototype is in the buffer and        */
/* reformats it for style.                                                  */
/*--------------------------------------------------------------------------*/
FormatBuffer()
{
 int i,j,Length;
 char InsertString[LineBufferMax];
#ifdef DEBUG
 printf("FormatBuffer\n");
#endif
 strcpy(InsertString,"\n");
 for (i=TokenPos[1]; (i<=Count) && Buffer[i] && (Buffer[i+1]!='#');i++)
     if ((Buffer[i]=='\n')&&(((i<Count)&&WhiteSpace(Buffer[i+1]))||
	 ((i>0)&&WhiteSpace(Buffer[i-1])&&(Buffer[i+1]!='#'))))
        { 
         DelBuffer(i,1);
         i--;
        }
 for(i=TokenPos[1]; Buffer[i]&&(Buffer[i]!='#') ;i++)
    if ((Buffer[i]=='/')&&(Buffer[i+1]=='*'))
       while(((Buffer[i]!='*')||(Buffer[i+1]!='/'))&&(i++<=Count));
    else
       if (((Buffer[i]==' ')&&(Buffer[i+1]==' '))||
           ((Buffer[i]==' ')&&(Buffer[i+1]=='\t'))||
           ((Buffer[i]=='\t')&&(Buffer[i+1]=='\t'))||
           ((Buffer[i]=='\t')&&(Buffer[i+1]==' ')))
           DelBuffer(i,1);
 for (i=1; (i<TokenList[0])&&(TokenList[i]!='('); i++);
 for (j=TokenPos[i]; (j>0)&&(Buffer[j]!='\n'); j--)
      if (Buffer[j]=='\t')
	 strcat(InsertString,"\t");
      else
	 strcat(InsertString," ");
 Length=strlen(InsertString);
 for (i=2; Buffer[i]&&(Buffer[i]!='#') ; i++)
     if (Buffer[i]==',')
        {
         Count=strlen(Buffer);
         for(j=Count+1; j>=i; j--)
            Buffer[Length+j]=Buffer[j];
         CopyToBuffer(i+1,InsertString);
         i+=Length;
        }
    else
    if ((Buffer[i-1]=='/')&&(Buffer[i]=='*'))
       while(((Buffer[i-1]!='*')||(Buffer[i]!='/'))&&(i++<=Count));
    else
       if (Buffer[i]=='{')
         i=FindRightBrace(i+1);
 Count=strlen(Buffer);  
 DelBuffer(Count-1,1);
 strcat(Buffer,"\n{");
 Count+=2;
#ifdef LINTMAKER
 if (lintflag)
	{
	for (i=TokenPos[1]; (Buffer[TokenPos[i]] != '(');i++);
	i = TokenPos[i] - 1;
	for (j=0;(WhiteSpace(Buffer[j]));j++);
	while(WhiteSpace(Buffer[i])) i--;
	while(!WhiteSpace(Buffer[i])) i--;
	while(WhiteSpace(Buffer[i])) i--;
	strncpy(FuncType,&Buffer[j],(i-j+1));
	FuncType[i-j+1] = NULL;
	}
#endif /* LINTMAKER */
}
/*--------------------------------------------------------------------------*/
/* FIX IMPLIED PARAMS                                                       */
/* This function looks for non converted param tokens in the bufffer and    */
/* adds an int specifier to them.                                           */
/*--------------------------------------------------------------------------*/
FixImpliedParams()
{
 int i= -1;
#ifdef DEBUG
 printf("FixImpliedParams\n");
#endif
 while (++i<TokenList[0])
    if (TokenList[i]==PARAMVAR)
       {
        InsertBuf(i,4);
        CopyToBuffer(TokenPos[i],"int ");
       }
}
/*--------------------------------------------------------------------------*/
/* IMPLICIT INT FUNC                                                        */
/* This function adds an int declaration to functions that had no type      */
/* specifier.                                                               */
/*--------------------------------------------------------------------------*/
ImplicitIntFunc()
{
#ifdef DEBUG
 printf("ImplicitIntFunc\n");
#endif
 if (TokenList[1]==FUNCNAME)
  {
   InsertBuf(1,4);
   CopyToBuffer(TokenPos[1],"int ");
  }
}
/*--------------------------------------------------------------------------*/
/* FIX IMPLIED INTS                                                         */
/* This function adds an int declaration to intergers declared in global    */
/* space with out a type specifier.                                         */
/*--------------------------------------------------------------------------*/
FixImpliedInts()
{
#ifdef DEBUG
 printf("FixImpliedInts\n");
#endif
 Debug("Implied Int1");
 InsertBuf(1,3);
 Debug("Implied Int2");
 strcpy(Buffer,"int");
 Debug("Implied Int3");
 Buffer[3]=' ';
 Debug("Implied Int4");
}
/*--------------------------------------------------------------------------*/
/* FIX PROTOTYPE                                                            */
/*   Param Def is the token index for the position of the parameter         */
/*             definition in the old style C declaration                    */
/*   Param ID  is the token index for the positiion of the parameter        */
/*             variable in the definition declaration                       */
/* This function creates the prototype declaration by moving the old C      */
/* parameter definition into the inside of the parenthises.                 */
/*--------------------------------------------------------------------------*/
int FixProtoType(ParamDef,ParamID)
int ParamDef,ParamID;
{
 int i=1,
     SaveToken;
#ifdef DEBUG
 printf("FixProtoType\n");
#endif
 while (i<TokenList[0]) {
     while ((i<=TokenList[0])&&(TokenList[i]!=PARAMVAR))
        i++;
     if (EqualTokens(i,ParamID))
       {
	if (WidenParams)
	   WidenParameters(ParamID,ParamDef);
        InsertDef(i,ParamDef,ParamID,TRUE);
        return(TRUE);
       }
      i++;
     }
 return(FALSE);
}
/*--------------------------------------------------------------------------*/
/* GET TOKEN ERROR ON PP COMMANDS                                           */
/* This function calls GetToken1() and will issue a warning if any          */
/* preprocessor commands are found. It is used whenever parsing a function  */
/* declaration.                                                             */
/*--------------------------------------------------------------------------*/
int GetTokenErrorOnPPCommands()
{
#ifdef DEBUG
 printf("GetTokenErrorOnPPCommands\n");
#endif
 while (GetToken1()==PP_COMMAND)
  {
   Error(FALSE,"preprocessor command in function declaration");
   ProcessPPCommand();
   ErrorInDeclaration=TRUE;
   }
 return(Token);
}
/*--------------------------------------------------------------------------*/
/* GET PARAMETER DEFINITION                                                 */
/* This function parses the definition of an old C declaration.             */
/*--------------------------------------------------------------------------*/
int GetParameterDef()
{
 int Flag=TRUE,
     ParamID,
     ParamDef,
     SaveToken;
#ifdef DEBUG
 printf("GetParameterDef\n");
#endif
 ErrorInDeclaration=FALSE;
 while(TRUE)
  {
     if (Flag)
        {
         ParamDef=TokenList[0];
         --Flag;
        }
     switch(Token)
      {
       case '*'        :
       case IDENTIFIER :do {
                        while((GetTokenErrorOnPPCommands()=='*')||(Token=='('));
                        if (Token!=IDENTIFIER)
                           UnGetToken();
                        ParamID=TokenList[0];
                        while((GetTokenErrorOnPPCommands()=='[')||(Token==')')||
                              ((Token=='(')&&(GetTokenErrorOnPPCommands()==')')))
                           if (Token=='[')
                              while (GetTokenErrorOnPPCommands()!=']');
                        SaveToken=Token;
                        GetToken1();
                        UnGetToken();
                        FixProtoType(ParamDef,ParamID);
                        if (SaveToken==',')
                         {
                          Count=TokenPos[ParamID];
                          TokenPos[0]-=1+TokenList[0]-ParamID;
                          TokenList[0]-=1+TokenList[0]-ParamID;
                          Buffer[Count]=NULL;
                          while ((TokenList[TokenList[0]]=='*')||(TokenList[TokenList[0]]=='('))
                             {
                              Count=TokenPos[TokenPos[0]];
                              --TokenList[0];
                              --TokenPos[0];
                             }  
                          Buffer[Count]=NULL;
			  GetTokenErrorOnPPCommands();
                         }
                        } while (SaveToken==',');
                        Count=TokenPos[ParamDef];
                        Buffer[Count]=NULL;
                        TokenPos[0]=ParamDef-1;
                        TokenList[0]=ParamDef-1;
                        Flag=TRUE;
                        break;
       case '{'        :return(TRUE);
       case '('        :break;
       case EXTERN     :break;
       case STATIC     :break;
       case AUTO       :break;
       case REGISTER   :break;
       case CHAR       :break;
       case SHORT      :break;
       case INT        :break;
       case LONG       :break;
       case SIGNED     :break;
       case UNSIGNED   :break;
       case FLOAT      :break;
       case DOUBLE     :break;
       case CONST      :break;
       case VOLATILE   :break;
       case VOID       :break;
       case STRUCT     :MustGetIdentifier();
                        GetBraces();
                        break;
       case ENUM       :MustGetIdentifier();
                        GetBraces();
                        break;
       case UNION      :MustGetIdentifier();
                        GetBraces();
                        break;
       case PP_COMMAND :if (GetToken1()==ELSE)
			   {
			    SaveToken=TokenList[0];
	                    ReadUntilElseEndIf(TRUE,FALSE);
			    TokenPos[0]=TokenList[0]=SaveToken;
			    PPCommandInFuncDeclaration=TRUE;
			   }
	                else
			   if (Token==ENDIF)
			     {
			      if (IfDefLevel)
			         --IfDefLevel;
	                      GetPreProcessorCommand(FALSE);
			      TokenPos[0]=TokenList[0]-=2;
			      PPCommandInFuncDeclaration=TRUE;
			     }
	                else
			  {
			   Error(FALSE,"preprocessor command in function declaration");
			   UnGetToken();
			   ProcessPPCommand();
			   return(FALSE);
		          }
	                break;
       default         :UnGetToken();
                        return(FALSE);
      }
     if (ErrorInDeclaration)
        {
	 ReadUntilRightBrace(TRUE);
	 return(FALSE);
        }
     GetToken1();
  }
}
/*--------------------------------------------------------------------------*/
/* GET PROTOTYPE PARAMETER                                                  */
/* This function parses the parameter list of a prototyped function.        */
/*--------------------------------------------------------------------------*/
GetProtoTypeParameter()
{
 int Parens=1,
     SaveToken;
#ifdef DEBUG
 printf("GetProtoTypeParameter\n");
#endif
 ErrorInDeclaration=FALSE;
 while(TRUE)
  {
     switch(Token)
      {
       case '*'        :break;
       case IDENTIFIER :
	                while((GetTokenErrorOnPPCommands()=='[')||
			      ((Token==')')&&(Parens>1))||
                              ((Token=='(')&&(Parens++)&&
			       (GetTokenErrorOnPPCommands()==')')))
                           if (Token=='[')
                              while (GetTokenErrorOnPPCommands()!=']');
	                   else
			       if (Token==')')
				  --Parens;
                        if (Token!=',')
			   UnGetToken();
	                break;
       case ')'        :
	                if (--Parens)
			   Error(TRUE,"grammar conflict (1)");
	                return(FALSE);
       case '('        :Parens++;
	                break;
       case EXTERN     :break;
       case STATIC     :break;
       case AUTO       :break;
       case REGISTER   :break;
       case CHAR       :break;
       case SHORT      :break;
       case INT        :break;
       case LONG       :break;
       case SIGNED     :break;
       case UNSIGNED   :break;
       case FLOAT      :break;
       case DOUBLE     :break;
       case CONST      :break;
       case VOLATILE   :break;
       case VOID       :break;
       case STRUCT     :MustGetIdentifier();
                        GetBraces();
                        break;
       case ENUM       :MustGetIdentifier();
                        GetBraces();
                        break;
       case UNION      :MustGetIdentifier();
                        GetBraces();
                        break;
       case ELIPSIS    :GetToken();
	                return(FALSE);
       case PP_COMMAND :if (GetToken1()==ELSE)
			   {
			    SaveToken=TokenList[0];
	                    ReadUntilElseEndIf(TRUE,FALSE);
			    TokenPos[0]=TokenList[0]=SaveToken;
			    PPCommandInFuncDeclaration=TRUE;
			   }
	                else
			   if (Token==ENDIF)
			     {
			      if (IfDefLevel)
			         --IfDefLevel;
	                      GetPreProcessorCommand(FALSE);
			      TokenPos[0]=TokenList[0]-=2;
			      PPCommandInFuncDeclaration=TRUE;
			     }
	                else
			  {
			   Error(FALSE,"preprocessor command in function declaration");
			   UnGetToken();
			   ProcessPPCommand();
			   return(FALSE);
		          }
	                break;
       default         :return(FALSE);
     }
     if (ErrorInDeclaration)
        {
	 printf("HERE\n");
	 ReadUntilSemiColon;
         return(FALSE);
        }
     GetToken1();
  }
}
/*--------------------------------------------------------------------------*/
/* GET PROTOTYPE                                                            */
/* This function parses a previously prototyped function and sends its      */
/* definition to the prototype file.                                        */
/*--------------------------------------------------------------------------*/
int GetProtoType()
{
 while (GetProtoTypeParameter());
 if (GetToken()=='{')
    {
     UnGetToken();
     fputs(Buffer,OutFile);
     Buffer[Count++]=NULL;
     AddToProtoTypeFile();
     GetToken();
     ReadUntilRightBrace(TRUE);
    }
 else
    UnGetToken();
 return(FALSE);
}
/*--------------------------------------------------------------------------*/
/* GET PARAMETER VARIABLE                                                   */
/* This function parses a parameter variable and determines if the function */
/* has already been prototyped.                                             */
/*--------------------------------------------------------------------------*/
int GetParameterVariable()
{
 int SaveToken;
#ifdef DEBUG
 printf("GetParameterVariable\n");
#endif
 if (GetTokenErrorOnPPCommands()==IDENTIFIER)
  { /*Check for typedef in function*/
   if ((GetTokenErrorOnPPCommands()!=',')&&(Token!=')')) 
    {
     UnGetToken();
     return(GetProtoType());
    }
   TokenList[TokenList[0]-1]=PARAMVAR;
  }
 else
   if ((Token!=')')&&(Token!='*')&&(Token!='(')&&(Token!=VOID))
       return(GetProtoType());
   else
      if (Token==VOID)
       {
	GetTokenErrorOnPPCommands();
        if (GetToken1()==PP_COMMAND)
	  {
	   if (GetToken1()==ELSE)
	      {
	       SaveToken=TokenList[0];
	       ReadUntilElseEndIf(TRUE,FALSE);
	       TokenPos[0]=TokenList[0]=SaveToken;
	       PPCommandInFuncDeclaration=TRUE;
	      }
	   else
	     if (Token==ENDIF)
	      {
	       if (IfDefLevel)
		  --IfDefLevel;
	       GetPreProcessorCommand(FALSE);
	       TokenPos[0]=TokenList[0]-=2;
	       PPCommandInFuncDeclaration=TRUE;
	      }
	    else
	      {
	       Error(FALSE,"preprocessor command in function declaration");
	       UnGetToken();
	       ProcessPPCommand();
	       return(FALSE);
              }
          }
	else
	   UnGetToken();
        if (GetTokenErrorOnPPCommands()=='{')
	     {
	      UnGetToken();
              fputs(Buffer,OutFile);
              Buffer[Count++]=NULL;
              AddToProtoTypeFile();
	      GetToken();
	      ReadUntilRightBrace(FALSE);
	      return(FALSE);
             }
       }
      else
       UnGetToken();
 return(TRUE);
}
/*--------------------------------------------------------------------------*/
/* INSERT VOID                                                              */
/* This function inserts the void specifier into empty declarations.        */
/*--------------------------------------------------------------------------*/
InsertVoid()
{
 int i,j;
 char Data[5];
#ifdef DEBUG
 printf("InsertVoid\n");
#endif
 strcpy(Data,"void");
 for (j=TokenList[0]-1; (j>0)&&(TokenList[j]!=')'); j--);
 if (j)
  {
   InsertBuf(j,4);
   for (i=0; i<4; i++)
        Buffer[TokenPos[j]+i]=Data[i];
  }
}
/*--------------------------------------------------------------------------*/
/* GET PARAMETERS AND BODY                                                  */
/* This function parses the parameters list and function body of the input. */
/*--------------------------------------------------------------------------*/
int GetParametersAndBody()
{
 int SaveToken;
#ifdef DEBUG
 printf("GetParametersAndBody\n");
#endif
 if (GetTokenErrorOnPPCommands()==')')
    {
     if (GetToken1()==PP_COMMAND)
      {
	if (GetToken1()==ELSE)
	   {
	    SaveToken=TokenList[0];
	    ReadUntilElseEndIf(TRUE,FALSE);
	    TokenPos[0]=TokenList[0]=SaveToken;
	    PPCommandInFuncDeclaration=TRUE;
	   }
        else
	  if (Token==ENDIF)
	   {
	    if (IfDefLevel)
	       --IfDefLevel;
	    GetPreProcessorCommand(FALSE);
	    TokenPos[0]=TokenList[0]-=2;
	    PPCommandInFuncDeclaration=TRUE;
	   }
	else
	   {
	    Error(FALSE,"preprocessor command in function declaration");
	    UnGetToken();
	    ProcessPPCommand();
	    return(FALSE);
           }
      GetTokenErrorOnPPCommands();
     }
     if (Token=='{')
      {
        ImplicitIntFunc();
        InsertVoid();
        FormatBuffer();
        fputs(Buffer,OutFile);
        AddToProtoTypeFile();
#ifdef LINTMAKER
	if (lintflag)
		ReplaceBody();
	else
		ReadUntilRightBrace(TRUE);
#else
        ReadUntilRightBrace(TRUE);
#endif /* LINTMAKER */
        return(TRUE);
       }
     else
       {
        UnGetToken();
	ReadUntilSemiColon();
	return(FALSE);
       }
    }
 else
    {
     UnGetToken();
     do {
         if (!GetParameterVariable())
            return(FALSE);
        } while (Token==',');
     if (Token!=')')
        return(FALSE);
    }
 GetToken();
 if (Token==',')
    while(GetToken()!=';');
 if (Token==';')
    return(FALSE);
 if (GetParameterDef())
    {
     ImplicitIntFunc();
     FixImpliedParams(); 
     FormatBuffer();
     fputs(Buffer,OutFile);
     AddToProtoTypeFile();
#ifdef LINTMAKER
     if (lintflag)
	ReplaceBody();
     else
	ReadUntilRightBrace(TRUE);
#else
     ReadUntilRightBrace(TRUE);
#endif /* LINTMAKER */
     return(TRUE);
    }
 return(FALSE);
}
#ifdef LINTMAKER
/*--------------------------------------------------------------------------*/
/* ReplaceBody                                                              */
/* This function reads the body of a function and prints a dummy version    */
/* for lint.								    */
/*--------------------------------------------------------------------------*/
ReplaceBody()
{
	int i;
	int Start;
	int ApexComment;
	char SaveChar;

	ReadUntilRightBrace(FALSE);

	i = 0;
	while (i <= Count) {
	    if ((Buffer[i] == '/') && (Buffer[i+1] == '*')) {
		Start = i;
		i = i + 3;
		while(WhiteSpace(Buffer[i])) i++;
		ApexComment = (strncmp("APEX",&Buffer[i],4) == 0);
		while(((Buffer[i-1]!='*')||(Buffer[i]!='/'))&&(i <= Count))
			i++;
		if (ApexComment) {
			fputs("\n",OutFile);
			SaveChar = Buffer[i+1];
			Buffer[i+1] = NULL;
			fputs(&Buffer[Start],OutFile);
			Buffer[i+1] = SaveChar;
			fputs("\n",OutFile);
		}
	    }
	    i++;
	}

	ClearBuffer();
	if (strcmp(FuncType,"void") == 0) {
		fputs("\nreturn;\n}\n",OutFile);
	}
	else {
		fputs("\nstatic ",OutFile);
		fputs(FuncType,OutFile);
		fputs(" __APEX_dummy_return_lint_val;\n",OutFile);
		fputs("return(__APEX_dummy_return_lint_val);\n}\n",OutFile);
	}
}
#endif /* LINTMAKER */
/*--------------------------------------------------------------------------*/
/* TYPE DEF OR IDENTIFIER                                                   */
/* This function continues to parse the declarations of a function.         */
/*--------------------------------------------------------------------------*/
int TypeDefOrIdentifier()
{
#ifdef DEBUG
 printf("TypeDefOrIdentifier\n");
#endif
 while ((GetToken()=='*')||(TypeSpecifier(Token)));
 if (Token==IDENTIFIER)
    GetToken();
 else
    if ((TokenList[0]==1)&&(Token!='('))
       {	
        if (AddImpliedInts)
            FixImpliedInts();
        ReadUntilSemiColon();
	return(FALSE);
       }
 TokenList[TokenList[0]-1]=FUNCNAME;
 if (Token=='(')
    return(GetParametersAndBody());
 UnGetToken();
 ReadUntilSemiColon();
 return(FALSE);
}
/*--------------------------------------------------------------------------*/
/* Validate File Name                                                       */
/*--------------------------------------------------------------------------*/
GetValidFileName(OriginalName,Extension,Result,RemoveChars)
char OriginalName[],
     Extension[],
     *Result;
{
 char  *BaseFileName=strrchr(OriginalName,'/'),
       FileName[LineBufferMax];
 long max_name_length = pathconf((BaseFileName?BaseFileName:"."),
				 _PC_NAME_MAX);
#ifdef DEBUG
  printf("GetValidFileName\n");
#endif
 if (BaseFileName)
   strcpy(FileName,BaseFileName+1);
 else
   strcpy(FileName,OriginalName);
 FileName[strlen(FileName)-RemoveChars]=NULL;
 if ((strlen(FileName)+strlen(Extension))>max_name_length)
    {
     strcpy(&FileName[max_name_length-strlen(Extension)],Extension);
     if (!access(FileName,2))
	 {
	  fprintf(stderr,
	  "%s already exists\n",
	  FileName);
	  exit(1);
         }
    }
 else
    strcat(FileName,Extension);
 strcpy(Result,FileName);
}
/*--------------------------------------------------------------------------*/
/* INITIALIZE                                                               */
/*      File Name the name of the file to be prototyped.                    */
/* This function determines the names of the output files and prepares them */
/* for scanning. It also has proto2 (altered cpp) scan the file and pipe    */
/* its results back.                                                        */
/*--------------------------------------------------------------------------*/
initialize(FileName)
char  FileName[LineBufferMax];
{
 int  EndPos=strlen(FileName);
 char FileNameCopy[LineBufferMax];
#ifdef DEBUG
 printf("initialize\n");
#endif
 while((EndPos>=0)&&(FileName[EndPos]!='/'))
      EndPos--;
 strcpy(FileNameCopy,&FileName[EndPos+1]);
 EndPos=strlen(FileNameCopy);
 if (bstrcmp(FileName,".h"))
    GetValidFileName(FileName,".a.h",FileNameCopy,2);
 else
    GetValidFileName(FileName,".a.c",FileNameCopy,bstrcmp(FileName,".c")*2);
 strcpy(OutputFileName,FileNameCopy);
 if ((OutFile=fopen(FileNameCopy,"w"))==NULL)
    Error(TRUE,"output file can not be created");
}
/*--------------------------------------------------------------------------*/
/* FIX INCLUDE FILE                                                         */
/* This function is called whenever an #include preprocessor command is     */
/* found. It stores the current variables and scans the include files. When */
/* the end of the include file is found by EndOfFile, a long jump is        */
/* performed to return to processing the original file.                     */
/*--------------------------------------------------------------------------*/
FixIncludeFile()
{
 int    TempLineNo,
        TempOutputFlag,
        ReadInclude;
 FILE   *TempInFile,
        *TempOutFile;
 char   Name[LineBufferMax],
        IncludeFile[LineBufferMax],
        FileNameCopy[LineBufferMax],
        FileNameCopy2[LineBufferMax];

#ifdef DEBUG
 printf("FixIncludeFile\n");
#endif
 GetToken();
 fscanf(IfDefFile,"%s %d %s ",SynchByte,&CPPLineNo,IncludeFile);
#ifdef PPCOMMAND
 printf("|%s| %d |%s| %d\n",SynchByte,CPPLineNo,IncludeFile,yylineno);
#endif
 if (strcmp(SynchByte,"$"))
    Error(TRUE,"internal sync error in the fix include subroutine");
 if (Token=='<')
    {
     while (GetToken()!='>')
        DumpBuffer();
     GetPreProcessorCommand(FALSE);
     while (fscanf(IfDefFile,"%s %d %s ",SynchByte,&CPPLineNo,Name))
       {
#ifdef PPCOMMAND
        printf("bumping |%s| %d |%s| |%s| %d\n",SynchByte,
	       CPPLineNo,IncludeFile,FileName,yylineno);
#endif	
        if ((bstrcmp(SynchByte,"#"))&&(!strcmp(IncludeFile,Name)))
            break;
       }
     return;
    }
 if (IncludeLevel>=IncludeFileMax)
    {
     Error(FALSE,"include files nested to deep");
     return;
    }
 strcpy(Name,yytext+1);
 strcpy(FileNameCopy,FileName);
 strcpy(FileNameCopy2,OutputFileName);
 DumpBuffer();
 Name[strlen(Name)-1]=NULL;
 strcpy(FileName,Name);
 TempLineNo=yylineno;
 yylineno=1;
 TempInFile=yyin;
 TempOutFile=OutFile;
 TempOutputFlag=NoOutput;
 NoOutput=TRUE;
 if ((yyin=fopen(IncludeFile,"r"))==NULL)
    Error(TRUE,"includefile can not be opened");
 initialize(Name);
 PPCommandFlag=TRUE;
 ReadInclude=!(setjmp(IncludeJumpStack[IncludeLevel++]));
 while (ReadInclude)
  {
   GetFunctionDeclaration();
   DumpBuffer();
  };
 fscanf(IfDefFile,"%s %d %s ",SynchByte,&CPPLineNo,Name);
#ifdef PPCOMMAND
 printf("|%s| %d |%s| |%s| %d \n",SynchByte,CPPLineNo,IncludeFile,Name,
                                  yylineno);
#endif
 strcpy(FileName,FileNameCopy);
 if ((strcmp(SynchByte,"#"))||(!bstrcmp(IncludeFile,Name)))
    Error(TRUE,"internal sync error in the fix include subroutine 2");
 DumpBuffer();
 fclose(yyin);
 yyin=TempInFile;
 OutFile=TempOutFile;
 yylineno=TempLineNo;
 NoOutput=TempOutputFlag;
 strcpy(OutputFileName,FileNameCopy2);
}
/*--------------------------------------------------------------------------*/
/* READ UNTIL ELSE END IF                                                   */
/*      End If Only TRUE/FALSE if TRUE thhis function read until a #endif   */
/*                  is found otherwise it will read until an #else or       */
/*                  #endif is found.                                        */
/*      Dump        TRUE/FALSE if True the buffer will be iteratively       */
/*                  dumped, otherwise the contents will be preserved.       */
/* This function reads in tokens until a #end or #else is found.            */
/*--------------------------------------------------------------------------*/
int ReadUntilElseEndIf(EndIfOnly,Dump)
{
 int   Last;
 char  DefFlag;
 
#ifdef DEBUG
 printf("ReadUntilElseEndIf\n");
#endif
 GetPreProcessorCommand(Dump);
 while (((Last=GetToken1())!=PP_COMMAND) ||
        ((GetToken1()!=ELSE)&&(Token!=ENDIF)) ||
	((Token==ELSE)&&(EndIfOnly)))
   {
   if (Dump)
      DumpBuffer();
   if ((Last==PP_COMMAND)&&
       ((Token==IFDEF)||(Token==IF)||(Token==IFNDEF)))
      {
       fscanf(IfDefFile,"%c %d %c ",SynchByte,&CPPLineNo,&DefFlag);

#ifdef PPCOMMAND
       printf("|%s| %d  |%c| %d %d * ignore *\n",SynchByte,CPPLineNo,DefFlag,
	                                         yylineno,IfDefLevel);
#endif

       if ((strcmp(SynchByte,"@"))||(DefFlag!='F')||
           (abs(yylineno-CPPLineNo)>1))
           Error(TRUE,"internal error IF Def sync problem 3");
       ReadUntilElseEndIf(TRUE,Dump);
      }
    else
      if (Token==ELSE)
	 GetPreProcessorCommand(Dump);
    if (Dump)
       DumpBuffer();
   }
 GetPreProcessorCommand(Dump);
 return(Token);
}
/*--------------------------------------------------------------------------*/
/* IF DEF PARSE                                                             */
/* This function parses #ifdef, #if, and #ifndef  commands and determines   */
/* which path to traverse by inspecting the results piped to it from        */
/* proto2.c.  It also uses the information to ensure proper parsing of the  */
/* of the file                                                              */
/*--------------------------------------------------------------------------*/
IfDefParse()
{
 char DefFlag;

#ifdef DEBUG
 printf("IfDefParse\n");
#endif

 GetPreProcessorCommand(TRUE);
 fscanf(IfDefFile,"%c %d %c ",SynchByte,&CPPLineNo,&DefFlag);

#ifdef PPCOMMAND
 printf("|%s| %d  |%c| %d %d\n",SynchByte,CPPLineNo,DefFlag,
	 yylineno,IfDefLevel);
#endif
 if ((strcmp(SynchByte,"@"))||((DefFlag!='T')&&(DefFlag!='F'))||
     (abs(yylineno-CPPLineNo)>1))
    Error(TRUE,"internal error IF Def sync problem");
 if (DefFlag=='T')
    {
     IfDefLevel++;
     GetPreProcessorCommand(TRUE);
    }
 else
     ReadUntilElseEndIf(FALSE,TRUE);
}
/*--------------------------------------------------------------------------*/
/* PROCESS PP COMMAND                                                       */
/* This function parses all preprocessor commands and branches to the       */
/* appropiate subroutines.                                                  */
/*--------------------------------------------------------------------------*/
ProcessPPCommand()
{
#ifdef DEBUG 
  printf("ProcessPPCommand |%s| \n",Buffer);
#endif
 if (GetToken()==INCLUDE)
    FixIncludeFile();
 else
    if ((Token==IFDEF)||(Token==IFNDEF)||
	(Token==IF))
       IfDefParse();
    else
       if ((Token==ELSE)&&(IfDefLevel))
	  {
	   DumpBuffer();
	   ReadUntilElseEndIf(TRUE,TRUE);
	   DumpBuffer();
	  }
       else
	  {	
           if ((Token==ENDIF)&&(IfDefLevel))
              --IfDefLevel;
           GetPreProcessorCommand(TRUE);
          }
}
/*--------------------------------------------------------------------------*/
/* GET FUNCTION DECLARATION                                                 */
/* This function parses function declaration. If at any point the source is */
/* determined not to be a function defintion, it will return false.         */
/*--------------------------------------------------------------------------*/
int GetFunctionDeclaration()
{
#ifdef DEBUG
 printf("GetFunctionDeclaration\n");
#endif
 switch(GetToken())
  {
   case IDENTIFIER :return(TypeDefOrIdentifier());
   case '*'        :return(GetFunctionDeclaration());
   case EXTERN     :return(GetFunctionDeclaration());
   case AUTO       :return(GetFunctionDeclaration());
   case REGISTER   :return(GetFunctionDeclaration());
   case CHAR       :return(GetFunctionDeclaration());
   case SHORT      :return(GetFunctionDeclaration());
   case INT        :return(GetFunctionDeclaration());
   case LONG       :return(GetFunctionDeclaration());
   case SIGNED     :return(GetFunctionDeclaration());
   case UNSIGNED   :return(GetFunctionDeclaration());
   case FLOAT      :return(GetFunctionDeclaration());
   case DOUBLE     :return(GetFunctionDeclaration());
   case CONST      :return(GetFunctionDeclaration());
   case VOLATILE   :return(GetFunctionDeclaration());
   case VOID       :return(GetFunctionDeclaration());
   case STATIC     :return(GetFunctionDeclaration());
   case STRUCT     :GetIdentifier();
                    GetBraces();
                    return(TypeDefOrIdentifier());
   case UNION      :GetIdentifier();
                    GetBraces();
                    return(TypeDefOrIdentifier());
   case ENUM       :GetIdentifier();
                    GetBraces();
                    return(TypeDefOrIdentifier());
   case '{'        :ReadUntilRightBrace(TRUE);
                    return(FALSE);
   default         :return(FALSE);
   }
}
/*--------------------------------------------------------------------------*/
/* GET OPTIONS                                                              */
/*     argc argv command line parameters                                    */
/* This function sets the appropiate flags according to the command line    */
/* option and opens the initial file prototype file, and report file which  */
/* is stderr.                                                               */
/*--------------------------------------------------------------------------*/
GetOptions(argc,argv)
int argc;
char *argv[];
{
 int   c;
 char  ProtoTypeFileName[LineBufferMax],
       DefinesCommand[5000],
       *DefineString=DefinesCommand;
#ifdef DEBUG
 printf("GetOptions\n");
#endif
 strcpy(ProtoTypeFileName,"prototypes.h");
 ReportFile=stderr;
#ifndef OSF
 if (access("/lib/cpp",4)==-1)
     {
          fprintf(ReportFile,"Error: can not access proto2 file\n");
          exit(1);
     }
#endif
#ifdef LINTMAKER
 while ((c=getopt(argc,argv,"U:I:H:D:h:wl"))!=EOF)  /*DTS#CLLbs00179*/
#else
 while ((c=getopt(argc,argv,"U:I:H:D:h:w"))!=EOF)   /*Pkwan 920420  */
#endif /* LINTMAKER */
  {
    switch(c)
     {
      case 'D':strcat(CppCommand,"-D");
	       strcat(CppCommand,optarg);
	       if (!DefineFlag)
		 {
		  DefineFlag=TRUE;
		  DefineString+=sprintf(DefineString,"\n#if defined(%s)",optarg);
		 }
	       else
	          DefineString+=sprintf(DefineString,"&&defined(%s)",optarg);
	       break;
      case 'H':strcat(CppCommand," -H ");            /*DTS#CLLbs00179 */
               strcat(CppCommand,optarg);          /*Pkwan 920420   */
               break;
      case 'U':strcat(CppCommand,"-U");
	       strcat(CppCommand,optarg);
	       if (!DefineFlag)
		 {
		  DefineFlag=TRUE;
		  DefineString+=sprintf(DefineString,"\n#if !defined(%s)",optarg);
		 }
	       else
	          DefineString+=sprintf(DefineString,"&&!defined(%s)",optarg);
	       break;
      case 'I':strcat(CppCommand,"-I");
	       strcat(CppCommand,optarg);
	       break;
      case 'w':WidenParams=!WidenParams;
	       break;
      case 'h':if (strlen(optarg)<LineBufferMax)
                  strcpy(ProtoTypeFileName,optarg);
	       break;
#ifdef LINTMAKER
      case 'l':lintflag = TRUE;
	       break;
#endif /* LINTMAKER */
      case '?':fprintf(ReportFile,"unknown command option %s\n",argv[optind]);
	       exit(1);
     };
    strcat(CppCommand," ");
  }
#ifdef LINTMAKER
 if (!lintflag)
	{
 	if ((ProtoTypeFile=fopen(ProtoTypeFileName,"a"))==NULL)
     		{
		Error(TRUE,"Prototype file can not be created");
		exit(1);
		}
 	fprintf(ProtoTypeFile,"%s\n/*\nOptions %s\n*/\n",DefinesCommand,
         	&CppCommand[25]);
	}
#else
 if ((ProtoTypeFile=fopen(ProtoTypeFileName,"a"))==NULL)
     { Error(TRUE,"Prototype file can not be created"); exit(1); }
 fprintf(ProtoTypeFile,"%s\n/*\nOptions %s\n*/\n",DefinesCommand,
         &CppCommand[25]);
#endif /* LINTMAKER */
}
/*--------------------------------------------------------------------------*/
/* MAIN                                                                     */
/*     argc,argv variables for accessing command line options               */
/* The main procedure sets up the initial variables and then iteratively    */
/* scans the files listed on the command line.                              */
/*--------------------------------------------------------------------------*/
main(argc,argv)
int  argc;
char *argv[];
{
 int   Flag,
       CommandLength;

 GetOptions(argc,argv);
 CommandLength=strlen(CppCommand);
 for (;optind<argc; optind++)
  {
   if (access(argv[optind],4))
     {
      printf("Error: can not access %s\n\n",argv[optind]);
      continue;
     }
   strcpy(FileName,argv[optind]);
   strcat(CppCommand,FileName);
   printf("prototyping  %s\n",FileName);
   IfDefFile=popen(CppCommand,"r");
   fscanf(IfDefFile,"%s %d %s ",SynchByte,&CPPLineNo,FileName);
   if ((strcmp(SynchByte,"#"))||(CPPLineNo!=1))
      printf("Error: internal sync problem\n");
   yylineno=1;
   yyin=fopen(FileName,"r");
   initialize(FileName);
   PPCommandFlag=TRUE;
   NoOutput=TRUE;
   Flag=!setjmp(NextFileJmp);
   Buffer = (char *) malloc(BufMax);
   while (Flag)
     {
      GetFunctionDeclaration();
      DumpBuffer();
     }
   pclose(IfDefFile);
   printf("\n");
   CppCommand[CommandLength]=NULL;
  }
 fclose(ReportFile);
#ifdef LINTMAKER
 if (!lintflag)
	{
 	if (DefineFlag)
    		fprintf(ProtoTypeFile,"\n#endif\n");
 	else
    		fprintf(ProtoTypeFile,"\n");
 	fclose(ProtoTypeFile);
	}
#else
 if (DefineFlag)
    fprintf(ProtoTypeFile,"\n#endif\n");
 else
    fprintf(ProtoTypeFile,"\n");
 fclose(ProtoTypeFile);
#endif /* LINTMAKER */
}
/*--------------------------------------------------------------------------*/
