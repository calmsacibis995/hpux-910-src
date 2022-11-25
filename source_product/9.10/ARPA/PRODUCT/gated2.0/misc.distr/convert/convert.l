%p 4500
%e 2000

%{ 

#include "convert.h"
#include "y.tab.h"

#define screate(x) (x *) malloc(sizeof(x))

extern int ln;

%}

digit                   [0-9]
dot			\.
number                  {digit}+
separator		[ \t\n]
ippart			({digit}|{digit}{digit}|((1|2){digit}{digit}))
ipaddr			{ippart}{dot}{ippart}{dot}{ippart}{dot}{ippart}
nws			[^ \t\n]

%%
rip		{yylval.strings=screate(str_list);
	         yylval.strings->str =(char *) malloc(strlen(yytext));
	         yylval.strings->next = NULL;
		 strcpy(yylval.strings->str,yytext);
	 	 return(T_RIP);
		}

hello		{yylval.strings=screate(str_list);
	         yylval.strings->str =(char *) malloc(strlen(yytext));
	         yylval.strings->next = NULL;
		 strcpy(yylval.strings->str,yytext);
	 	 return(T_HELLO);
		}

egp		{yylval.strings=screate(str_list);
	         yylval.strings->str =(char *) malloc(strlen(yytext));
	         yylval.strings->next = NULL;
		 strcpy(yylval.strings->str,yytext);
		 return(T_EGP);
		}

yes		{  return(T_YES); }
no		{  return(T_NO); }

quiet		{ return(T_QUIET); }
supplier	{ return(T_SUPPLIER); }
pointopoint	{ return(T_P2P); }
gateway		{ return(T_GW); }


autonomoussystem {return(T_ASYS);}

egpmaxacquire	{return(T_EGPMAX);}

egpneighbor	{return(T_EGPNEIGH);}

metricin	{return(T_METRICIN);}
egpmetricout    {return(T_EGPMETRICOUT);}

asin	{return(T_ASIN);}
asout	{return(T_ASOUT);}
as	{return(T_AS);}

all		{ yylval.strings = screate(str_list);
		  yylval.strings->str = (char *) malloc(strlen(yytext));
	          yylval.strings->next = NULL;
		  strcpy(yylval.strings->str,yytext);
		  return(T_ALL);
		}
nogendefault	{return(T_NOGENDEFAULT);}
acceptdefault	{return(T_ACCEPTDEFAULT);}
defaultout	{return(T_DEFAULTOUT);}
validate	{return(T_VALIDATE);}
intf		{return(T_INTF);}
sourcenet	{return(T_SOURCENET);}


traceflags	{return(T_TRACE);}
internal	{return(T_INTERNAL);}
external	{return(T_EXTERNAL);}
route		{return(T_ROUTE);}
update		{return(T_UPDATE);}
icmp		{return(T_ICMP);}
stamp		{return(T_STAMP);}
general		{return(T_GENERAL);}

trustedrip{nws}+ 	{return(T_TRSTDRIPGW);}
trustedhello{nws}+ 	{return(T_TRSTDHELLOGW);}
sourcerip{nws}+ 	{return(T_SRCRIPGW);}
sourcehello{nws}+ 	{return(T_SRCHELLOGW);} 
noripout{nws}+	{return(T_NORIPOUT);}
nohelloout{nws}+	{return(T_NOHELLOOUT);}	
noripfrom{nws}+	{return(T_NORIPFROM);}
nohellofrom{nws}+    {return(T_NOHELLOFROM);}

passive			{return(T_PASSIVE);}
active			{return(T_ACTIVE);}
restrict		{return(T_RESTRICT);}
norestrict		{return(T_NORESTRICT);}
aslist			{return(T_ASLIST);}
passivei{nws}+		{return(T_PASSIVEINTF);}
interfacemetric		{return(T_INTFMETRIC);}
reconstmetric		{return(T_RECONSTMETRIC);}
fixedmetric		{return(T_FIXEDMETRIC);}
proto			{return(T_PROTO);}
donotlisten		{return(T_DONTLISTEN);}
donotlistenhost 	{return(T_DONTLISTENHOST);}
listenhost		{return(T_LISTENHOST);}
listen			{return(T_LISTEN);}
announcetoas		{return(T_ANNOUNCEAS);}
noannouncetoas		{return(T_NOANNOUNCEAS);}
announcehost		{return(T_ANNOUNCEHOST);}
announce		{return(T_ANNOUNCE);}
noannouncehost          {return(T_NOANNOUNCEHOST);}
noannounce              {return(T_NOANNOUNCE);}
egpmetric		{return(T_EGPMETRIC);}
defaultegp{nws}+	{return(T_DEFAULTEGPMETRIC);}	
defaultga{nws}+		{return(T_DEFAULTGW);}
egpnet{nws}+		{return(T_EGPNET);}
martia{nws}+		{return(T_MARTIAN);}
net			{return(T_NET);}
host			{return(T_HOST);}
metric			{return(T_METRIC);}



{ipaddr}   {   yylval.strings=screate(str_list);
	       yylval.strings->str =(char *) malloc(strlen(yytext));
	       yylval.strings->next = NULL;
	       strcpy(yylval.strings->str,yytext);
	       return(T_IPADDR); 
	   }

#[^\n]*			{ return(T_COMMENT); }

{number}    {
		yylval.strings = screate(str_list);
		yylval.strings->str = (char *) malloc(strlen(yytext));
	        yylval.strings->next = NULL;
		strcpy(yylval.strings->str, yytext);
		return(T_INT);
	    }

[a-z0-9\.-]+	{yylval.strings=screate(str_list);
	         yylval.strings->str =(char *) malloc(strlen(yytext));
	         yylval.strings->next = NULL;
	         strcpy(yylval.strings->str,yytext);
	         return(T_GOODNAME); 
		}

[ \t]*			{ }
[\n]			{ ln++; return(T_EOL); }

.		{ printf("??? %s\n",yytext); return(T_UNKNOWN);}
%%


