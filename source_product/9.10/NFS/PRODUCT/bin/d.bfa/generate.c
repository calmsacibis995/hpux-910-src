#include <stdio.h>

#include "defines.h"
#include "global.h"

#define INC_STACK(a)	{ if((++a) >= stack_elements) {	\
			     fprintf(stderr,"BFA Curl table overflow "); \
			     fprintf(stderr,"(Current size %d)  ", \
					stack_elements); \
			     fprintf(stderr,"Try -C<number> to increase.\n"); \
			     Exit(1); \
			  } /* if */ \
			}

#define DEC_STACK(a) 	{ if((--a) < -1) { \
			    fprintf(stderr,"BFA Internal Error "); \
			    fprintf(stderr,"Curl table underflow\n"); \
			  } /* if */ \
			}

#define INC_ELSE(a)	{ if((++a) >= else_len) {	\
			     fprintf(stderr,"BFA Else table overflow "); \
			     fprintf(stderr,"(Current size %d)  ", \
					else_len); \
			     fprintf(stderr,"Try -E<number> to increase.\n"); \
			     Exit(1); \
			  } /* if */ \
			}


static void ReadThruParenExpr() {
	char	 c[2];
	register int	paren_count = 1;

	int	in_string = 0;
	int	in_char   = 0;
	int	escape    = 0;

	int	id_pos    = 0;
	char	id_string[BUFSIZ];

	c[1] = '\0';

	while((c[0] = input()) != '(' && c[0] != 0) {
	   ConvertString(c);
	   fprintf(outfile,"%s",c);
	} /* while */

	if(c[0] == '0') {
	   fprintf(stderr,"BFA Internal Error - Can't find expression\n");
	   exit(1);
	} /* if */

	fprintf(outfile,"(");

	while(paren_count && (c[0] = input())) {

	   if( ! in_string && ! in_char && ! escape) {
	      id_pos = 0;
	      while(((toupper(c[0]) >= 'A' && toupper(c[0]) <= 'Z') ||
	           c[0] == '_' || (c[0] >= '0' && c[0] <= '9'))) {
	           id_string[id_pos++] = c[0];
		   c[0] = input();
	      } /* while */

	      if(id_pos) {
	         id_string[id_pos] = '\0';
	         unput(c[0]);
	         ProcessId(id_string);
	         input();
	      } /* if */
	   } /* if */

	   ConvertString(c);
	   fprintf(outfile,"%s",c);

	   if(c[0] == '"') {
	      if(! in_string && ! in_char && ! escape) 
		 in_string = 1;
	      else if( ! escape)
		 in_string = 0;
	      escape = 0;
	   } else if(c[0] == '\'') {
	      if(! in_string && ! in_char && ! escape) 
		 in_char = 1;
	      else if( ! escape)
		 in_char = 0;
	      escape = 0;
	   } else if(c[0] == '\\') {
	      if(! escape)
		 escape = 1;
              else
		 escape = 0;
	   } else {
	      if(! in_string && ! in_char && ! escape) {
	         if(c[0] == '(')
	            paren_count++;
	         else if(c[0] == ')')
	            paren_count--;
	      } /* if */
	      escape = 0;
	   } /* else */
	} /* while */

	if(c[0] == '0') {
	   fprintf(stderr,"BFA Internal Error - Can't find expression\n");
	   exit(1);
	} /* if */

} /* ReadThruParenExpr */


static void ReadFunctionArgument() {
	char	c[2];
	int	paren_count = 0;
	int	done	    = 0;

	int	in_string = 0;
	int	in_char   = 0;
	int	escape    = 0;

	c[1] = '\0';

	while(!done && (c[0] = input())) {
	   if(c[0] == '"') {
	      if(! in_string && ! in_char && ! escape) 
		 in_string = 1;
	      else if( ! escape)
		 in_string = 0;
	      escape = 0;
	   } else if(c[0] == '\'') {
	      if(! in_string && ! in_char && ! escape) 
		 in_char = 1;
	      else if( ! escape)
		 in_char = 0;
	      escape = 0;
	   } else if(c[0] == '\\') {
	      if(! escape)
		 escape = 1;
              else
		 escape = 0;
	   } else {
	      if(! in_string && ! in_char && ! escape) {
		 if(paren_count == 0 && (c[0] == ',' || c[0] == ')')) {
		    unput(c[0]);
		    done = 1;
	         } else if(c[0] == '(')
	            paren_count++;
	         else if(c[0] == ')')
	            paren_count--;
	      } /* if */
	      escape = 0;
	   } /* else */

	   if(! done) {
	      ConvertString(c);
	      fprintf(outfile,"%s",c);
	   }  /* if */
	} /* while */

	if(c[0] == '0') {
	   fprintf(stderr,"BFA Internal Error - Can't find argument\n");
	   exit(1);
	} /* if */

} /* ReadFunctionArgument */


static void ReadAndWriteUntilChar(stop_char) 
char	stop_char;
{
	char	 c[2];

	int	no_stop	  = 1;
	int	in_string = 0;
	int	in_char   = 0;
	int	escape    = 0;

	c[1] = '\0';

	while(no_stop && (c[0] = input())) {
	   ConvertString(c);
	   fprintf(outfile,"%s",c);

	   if(c[0] == '"') {
	      if(! in_string && ! in_char && ! escape) 
		 in_string = 1;
	      else if( ! escape)
		 in_string = 0;
	      escape = 0;
	   } else if(c[0] == '\'') {
	      if(! in_string && ! in_char && ! escape) 
		 in_char = 1;
	      else if( ! escape)
		 in_char = 0;
	      escape = 0;
	   } else if(c[0] == '\\') {
	      if(! escape)
		 escape = 1;
              else
		 escape = 0;
	   } else {
	      if(! in_string && ! in_char && ! escape) {
	         if(c[0] == stop_char)
	            no_stop = 0;
	      } /* if */
	      escape = 0;
	   } /* else */
	} /* while */

} /* ReadAndWriteUntilChar */



static int GetNextNonWhiteChar() {
	char	c[2];

	c[1] = '\0';

	while((c[0] = (char)input()) == ' ' || c[0] == '\t' ||
	       c[0] == '\f' || c[0] == SPECIAL_CHAR || c[0] == '\n') {
	   ConvertString(c);
	   fprintf(outfile,"%s",c); 
	} /* while */


	return((int)c[0]);

} /* GetNextNonWhiteChar */


static int GetNextNonWhiteCharFromString(string) 
char	*string;
{
	int	i = 0;
	char	c;


	while((c = string[i++]) == ' ' || c == '\t' ||
	       c == '\f' || c == SPECIAL_CHAR || c == '\n');

	return((int)c);

} /* GetNextNonWhiteCharFromString */


static int LookForNextNonWhiteChar() {
	int	ret_val = 1;

	else_pos = 0;

	while(else_pos< else_len-3 && ((else_string[else_pos]=input()) == ' ' ||
	       		      else_string[else_pos] == '\t' ||
	       		      else_string[else_pos] == '\f' ||
	       		      else_string[else_pos] == '\n' ||
	       		      else_string[else_pos] == SPECIAL_CHAR ))
	   INC_ELSE(else_pos);

	if(else_pos >= else_len-3) {
	   fprintf(stderr,"BFA Else table overflow (Current size %d)",else_len);
	   fprintf(stderr,"  Try -E<number> to increase.\n"); 
	   Exit(1); 
	} /* if */

	ret_val = else_string[else_pos];

	for(;else_pos >= 0;else_pos--)
	   unput(else_string[else_pos]);

	return(ret_val);
} /* LookForNextNonWhiteChar */


static int NoElseFollowing() {
	int	ret_val = 1;

	else_pos = 0;

	while(else_pos< else_len-3 && ((else_string[else_pos]=input()) == ' ' ||
	       		      else_string[else_pos] == '\t' ||
	       		      else_string[else_pos] == '\f' ||
	       		      else_string[else_pos] == '\n' ||
	       		      else_string[else_pos] == SPECIAL_CHAR ))
	   INC_ELSE(else_pos);

	if(else_pos >= else_len-3) {
	   fprintf(stderr,"BFA Else table overflow (Current size %d)",else_len);
	   fprintf(stderr,"  Try -E<number> to increase.\n"); 
	   Exit(1); 
	} /* if */

	if(else_string[else_pos] == 'e') {
	   INC_ELSE(else_pos);
	   else_string[else_pos] = input();
	   INC_ELSE(else_pos);
	   else_string[else_pos] = input();
	   INC_ELSE(else_pos);
	   else_string[else_pos] = input();

	   if(else_string[else_pos - 2] == 'l' &&
	      else_string[else_pos - 1] == 's' &&
	      else_string[else_pos] == 'e' )
	      ret_val = 0;
	      
	} /* if */

	for(;else_pos >= 0;else_pos--)
	   unput(else_string[else_pos]);


	return(ret_val);
} /* NoElseFollowing */


static void AddFallThru(kind,line)
int	kind;
int	line;
{
	switch(kind) {
	   case _IF	:
	   case _ELSEIF	:
			   if(NoElseFollowing()) {
			      fprintf(outfile," _bfa_array[%d]++;\n",
					NextRecord(_FALLTHRU));
			   }
			   break;
	   case _ELSE	:
	   case _DO	:
	   case _FOR	:
	   case _WHILE	:
	   case _SWITCH	:
			   fprintf(outfile," _bfa_array[%d]++;\n",
					NextRecord(_FALLTHRU));
			   break;
	   default	:
			   fprintf(stderr,"BFA Internal Error - ");
			   fprintf(stderr,"Unkown branch type\n");
			   Exit(1);
			   break;
	} /* switch */
} /* AddFallThru */



static int	start_function_line = 0;

void ProcessNewFunction(string)
char	*string;
{
	char	function[MAX_FUNCTION_ID];

	int	paren_count;
	int	end;
	int	paren_place;

	register int	i;
	register int	j;



	for(i=strlen(string)-1;i >= 0 && !(string[i] == ')' &&
		((j = GetNextNonWhiteCharFromString(&string[i+1])) != ';' &&
		  j != ',' && j != '(' && j != '*')) ;i--); 

	paren_place = i;

	for(paren_count=1,i--;i >= 0 && paren_count;i--)
	   if(string[i] == ')')
	      paren_count++;
	   else if(string[i] == '(')
	      paren_count--;

	for(;i>=0 && (toupper(string[i])<'A' || toupper(string[i])>'Z') &&
	     string[i] != '_' && (string[i] <'0' || string[i] >'9');i--);

	if(i < 0) {
	   ConvertString(string);
	   fprintf(outfile,"%s",string);
	   return;
	} /* if */

	end = i;

	for(;i>=0 &&((toupper(string[i])>='A' && toupper(string[i])<='Z') ||
	     string[i] == '_' || (string[i]>='0' && string[i]<='9'));i--);

	for(i++,j=0; i <= end;i++,j++)
	   function[j] = string[i];

	function[j] = '\0';

	strcpy(cur_func,function);

	for(i=strlen(string)-1;i > paren_place;i--)
	   unput(string[i]);

	string[i+1] = '\0';

	ConvertString(string);
	fprintf(outfile,"%s",string);

	BeginScannerState(STATE_P);

	specifier = 0;
	semi_cr   = 1;
	start_function_line = line_number;

} /* ProcessNewFunction */


void ProcessFunctionAfterParams(string)
char	*string;
{
	int	branch_num;
	int	line_tmp;

	line_tmp    = line_number;
	line_number = start_function_line;
	branch_num  = NextRecord(_ENTRY);
	line_number = line_tmp;


	ConvertString(string);
	fprintf(outfile,"%s",string);

	fprintf(outfile,"if(! _bfa_init)\n   _InitializeBFA();\n");
	fprintf(outfile,"_bfa_array[%d]++;\n{\n",branch_num);

	curl_count = 1;
	no_update  = 0;

	BeginScannerState(STATE_I);

} /* ProcessFunctionAfterParams */



void ProcessIf() {
	int	next;
	int	branch_num;


	branch_num = NextRecord(_IF);

	fprintf(outfile,"if");

	INC_STACK(tos);
	stack[tos].branch_type = _IF;
	stack[tos].line        = line_number;

	ReadThruParenExpr();
	next = GetNextNonWhiteChar();

	if(next == '{') {
	   curl_count++;
	   stack[tos].kind = CURL;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n{\n",branch_num);
	} else {
	   unput(next);
	   stack[tos].kind = SEMI;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n",branch_num);
	} /* else */

	
} /* ProcessIf */


void ProcessFor() {
	int	next;
	int	branch_num;


	branch_num = NextRecord(_FOR);

	fprintf(outfile,"for");

	INC_STACK(tos);
	stack[tos].branch_type = _FOR;
	stack[tos].line        = line_number;

	ReadThruParenExpr();
	next = GetNextNonWhiteChar();

	if(next == '{') {
	   curl_count++;
	   stack[tos].kind = CURL;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n{\n",branch_num);
	} else {
	   unput(next);
	   stack[tos].kind = SEMI;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n",branch_num);
	} /* else */

	
} /* ProcessFor */


void ProcessWhile() {
	int	next;
	int	branch_num;


	branch_num = NextRecord(_WHILE);

	fprintf(outfile,"while");

	INC_STACK(tos);
	stack[tos].branch_type = _WHILE;
	stack[tos].line        = line_number;

	ReadThruParenExpr();
	next = GetNextNonWhiteChar();

	if(next == '{') {
	   curl_count++;
	   stack[tos].kind = CURL;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n{\n",branch_num);
	} else {
	   unput(next);
	   stack[tos].kind = SEMI;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n",branch_num);
	} /* else */

	
} /* ProcessWhile */


void ProcessSwitch() {
	int	next;


	fprintf(outfile,"{ switch");

	INC_STACK(tos);
	stack[tos].branch_type = _SWITCH;
	stack[tos].line        = 0;

	ReadThruParenExpr();
	next = GetNextNonWhiteChar();

	if(next == '{') {
	   curl_count++;
	   fprintf(outfile,"{");
	   stack[tos].kind = CURL;
	} else {
	   unput(next);
	   stack[tos].kind = SEMI;
	} /* else */
	
} /* ProcessSwitch */


void ProcessCase() {
	int	next;
	int	branch_num;


	branch_num = NextRecord(_CASE);
	fprintf(outfile,"case");

	if(stack[tos].branch_type == _SWITCH &&
	   stack[tos].line == 0)
	   stack[tos].line = line_number;

	ReadAndWriteUntilChar(':');
	fprintf(outfile," _bfa_array[%d]++;\n",branch_num);

} /* ProcessCase */


 void ProcessDefault() {
	int	next;
	int	branch_num;


	branch_num = NextRecord(_DEFAULT);
	fprintf(outfile,"default :");

	if(stack[tos].branch_type == _SWITCH &&
	   stack[tos].line == 0)
	   stack[tos].line = line_number;

	fprintf(outfile," _bfa_array[%d]++;\n",branch_num);
	
} /* ProcessDefault */



void ProcessLabel(string) 
char	*string;
{
	int	next;
	int	branch_num;

	if(quest_count) {
	   quest_count--;
	   ConvertString(string);
	   fprintf(outfile,"%s",string);
	   return;
	}

	if(strlen(string) >= 8 &&
	   string[0] == 'd' && string[1] == 'e' &&
	   string[2] == 'f' && string[3] == 'a' &&
	   string[4] == 'u' && string[5] == 'l' &&
	   string[6] == 't' && string[7] != '_' &&
	   (toupper(string[7]) < 'A' || toupper(string[7]) > 'Z') &&
	   (string[7] < '0' || string[7] > '9')) {
	   ProcessDefault();
	   return;
	} /* if */

	branch_num = NextRecord(_LABEL);
	ConvertString(string);
	fprintf(outfile,"%s",string);

	fprintf(outfile," _bfa_array[%d]++;\n",branch_num);
	
} /* ProcessLabel */



void ProcessLeftCurl() {
	INC_STACK(tos);
	stack[tos].kind = IGNORE;
	curl_count++;
	fprintf(outfile,"{");
} /* ProcessLeftCurl */


void ProcessRightCurl() {
	int	i;


	curl_count--;

	fprintf(outfile,"}");

	if(curl_count == 0) {
	   semi_cr = 0;

	   for(i=0;i<main_cnt && ! no_update ;i++)
	      if(! strcmp(main_ids[i],cur_func))
		 fprintf(outfile,"_UpdateBFA();\n");

	   fprintf(outfile,"}");
	   BeginScannerState(STATE_L);
	} else {
	   if(tos >= 0 && stack[tos].kind != IGNORE) {
	      DEC_STACK(tos);
	      fprintf(outfile,"}");

	      if(stack[tos+1].kind == DO_CURL) {
	         ReadAndWriteUntilChar(';');
	      } /* if */

	      AddFallThru(stack[tos+1].branch_type,stack[tos+1].line);

	      for(;tos>=0 && stack[tos].kind==SEMI && NoElseFollowing();) {
		 fprintf(outfile,"}");
	         AddFallThru(stack[tos].branch_type,stack[tos].line);
		 DEC_STACK(tos);
	      } /* for */
	   } else
	      DEC_STACK(tos);

	} /* else */


} /* ProcessRightCurl */


void ProcessSemiColon() {
	fprintf(outfile,";");

	if(stack[tos].kind == DO_SEMI) {
	   fprintf(outfile,"}");
	   ReadAndWriteUntilChar(';');
	   AddFallThru(stack[tos].branch_type,stack[tos].line);
	   DEC_STACK(tos);
	} /* if */

	if(tos >= 0 && stack[tos].kind == SEMI) {
	   fprintf(outfile,"}");
	   AddFallThru(stack[tos].branch_type,stack[tos].line);
	   DEC_STACK(tos);
	} /* for */
	
	for(;tos >= 0 && stack[tos].kind == SEMI && NoElseFollowing();) {
	   fprintf(outfile,"}");
	   AddFallThru(stack[tos].branch_type,stack[tos].line);
	   DEC_STACK(tos);
	} /* for */
	
} /* ProcessSemiColon */



void ProcessQuestion() {
	quest_count++;
	fprintf(outfile,"?");
} /* ProcessQuestion */



void ProcessColon() {
	quest_count--;
	fprintf(outfile,":");
} /* ProcessColon */


void ProcessElse(kind) 
int	kind;
{
	int	next;
	int	branch_num;

	branch_num = NextRecord(kind);

	if(kind == _ELSE) {
	   fprintf(outfile," else ");
	} else if (kind == _ELSEIF) {
	   fprintf(outfile," else if ");
	} else {
	   fprintf(stderr,"BFA Internal Error - Uknown else type\n");
	   exit(1);
	} /* else */


	INC_STACK(tos);
	stack[tos].branch_type = kind;

	if(kind == _ELSEIF)
	   ReadThruParenExpr();

	next = GetNextNonWhiteChar();

	if(next == '{') {
	   curl_count++;
	   stack[tos].kind = CURL;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n{\n",branch_num);
	} else {
	   unput(next);
	   stack[tos].kind = SEMI;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n",branch_num);
	} /* else */

	
} /* ProcessElse */


void ProcessDo() {
	int	next;
	int	branch_num;


	branch_num = NextRecord(_DO);
	fprintf(outfile,"do");

	INC_STACK(tos);
	stack[tos].branch_type = _DO;
	stack[tos].line        = line_number;

	next = GetNextNonWhiteChar();

	if(next == '{') {
	   curl_count++;
	   stack[tos].kind = DO_CURL;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n{\n",branch_num);
	} else {
	   unput(next);
	   stack[tos].kind = DO_SEMI;
	   fprintf(outfile,"{ _bfa_array[%d]++;\n",branch_num);
	} /* else */

	
} /* ProcessDo */


void CheckReturn() {
	int	i;
	int	done;

	for(i=0,done=0;i<main_cnt && ! done;i++)
	   if(! strcmp(main_ids[i],cur_func)) {
	      fprintf(outfile,"{ _UpdateBFA(); \nreturn");
	      ReadAndWriteUntilChar(';');
	      fprintf(outfile,"}");
	      done = 1;

	      for(;tos >= 0 && stack[tos].kind == SEMI;) {
	         fprintf(outfile,"}");
	         AddFallThru(stack[tos].branch_type,stack[tos].line);
		 DEC_STACK(tos);
	      } /* for */

	      if(curl_count == 1)
		 no_update = 1;
	   } /* if */

	if(! done)
	   fprintf(outfile,"return");

} /* CheckReturn */


void ProcessId(string)
char	*string;
{
	int	next;

	next = LookForNextNonWhiteChar();


	if(next != '(') {
	   ConvertString(string);
	   fprintf(outfile,"%s",string);
	   return;
	} /* if */


	if(e_flag && ! strcmp(string,"exit")) {
	   fprintf(outfile,"ExitBFA");
	   return;
	} /* if */

	if(e_flag && ! strcmp(string,"_exit")) {
	   fprintf(outfile,"_ExitBFA");
	   return;
	} /* if */

#if defined(FORK)
	if(f_flag && ! strcmp(string,"fork")) {
	   fprintf(outfile,"_ForkBFA");
	   return;
	} /* if */
#endif

#if defined(FORK)
	if(f_flag && ! strcmp(string,"vfork")) {
	   fprintf(outfile,"_VForkBFA");
	   return;
	} /* if */
#endif

#if defined(EXEC)
	if(e_flag && (! strcmp(string,"execl")  ||
	              ! strcmp(string,"execv")  ||
	              ! strcmp(string,"execle") ||
	              ! strcmp(string,"execve") ||
	              ! strcmp(string,"execlp") ||
	              ! strcmp(string,"execvp"))) { 

	   fprintf(outfile,"%s",string);
	   if((next = GetNextNonWhiteChar()) != '(') {
	      fprintf(stderr,"BFA Internal Error - Can't find '('\n");
	      exit(1);
	   } /* if */
	   fprintf(outfile,"(_StringUpdateBFA(");
	   ReadFunctionArgument();
	   fprintf(outfile,")");
	   return;
	} /* if */
#endif
	ConvertString(string);
	fprintf(outfile,"%s",string);
} /* ProcessId */
