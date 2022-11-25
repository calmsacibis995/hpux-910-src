%n 600
%e 1600
%o 6000
%p 6000
%a 1600
%k 600
%start cmd, scanop, showop , args, redir, netop, driverop
%%
<cmd>[ \t]				{;}
<cmd>\n					{return('\n');}
<cmd>"#".*\n				{return('\n');}
<cmd>"!".*\n				{dosystem(yytext,yyleng); return('\n');}
<cmd>sho(w?)?			 	{BEGIN showop; return(TSHOW);}
<cmd>sn(a(p?)?)?			{BEGIN cmd; return(TSNAP);}
<cmd>q(u(i(t?)?)?)?			{BEGIN cmd; return(TQUIT);}
<cmd>e(x(i(t?)?)?)?			{BEGIN cmd; return(TQUIT);}
<cmd>io                 		{BEGIN driverop; return(TDRIVER);}
<cmd>rea(l(t(i(m(e?)?)?)?)?)?		{BEGIN cmd; return(TREALTIME);}
<cmd>sc(a(n?)?)? 			{BEGIN scanop; return(TSCAN);}
<cmd>seto(p(t?)?)?			{BEGIN scanop; return(TSETOPT);}
<cmd>listc(o(u(n(t?)?)?)?)?		{BEGIN args; return(TLISTCOUNT);}
<cmd>listd(u(m(p?)?)?)?			{BEGIN args; return(TLISTDUMP);}
<cmd>muxd(a(t(a?)?)?)? 			{BEGIN args; return(TMUXDATA);}
<cmd>muxc(a(m(d(a(t(a?)?)?)?)?)?)? 	{BEGIN args; return(TMUXCAMDATA);}
<cmd>muxh(w(d(a(t(a?)?)?)?)?)?		{BEGIN args; return(TMUXHWDATA);}
<cmd>save(s(t(a(t(e?)?)?)?)?)?		{BEGIN args; return(TSAVESTATE);}
<cmd>pdirh(a(s(h?)?)?)?			{BEGIN args; return(TPDIRHASH);}
<cmd>bu(f?)?				{BEGIN args; return(TBUF);}
<cmd>sw(b(u(f?)?)?)?			{BEGIN args; return(TSWBUF);}
<cmd>cm(a(p?)?)?			{BEGIN args; return(TCMAP);}
<cmd>pf(d(a(t?)?)?)?			{BEGIN args; return(TPFDAT);}
<cmd>db(d?)?				{BEGIN args; return(TDBD);}
<cmd>look				{BEGIN args; return(TLOOK);}
<cmd>wizard				{BEGIN args; return(TWIZ);}
<cmd>findbug				{BEGIN args; return(TFIXBUG);}
<cmd>sw(a(p(t(a(b?)?)?)?)?)?		{BEGIN args; return(TSWAPTAB);}
<cmd>pr(e(g(i(o(n?)?)?)?)?)?		{BEGIN args; return(TPREGION);}
<cmd>sys(m(a(p?)?)?)?		 	{BEGIN args; return(TSYSMAP);}
<cmd>shm(m(a(p?)?)?)?		 	{BEGIN args; return(TSHMMAP);}
<cmd>re(g(i(o(n?)?)?)?)?		{BEGIN args; return(TREGION);}
<cmd>fr(a(m(e?)?)?)?			{BEGIN args; return(TFRAME);}
<cmd>po(r(t?)?)				{BEGIN args; return(TPORT);}
<cmd>in(o(d(e?)?)?)?			{BEGIN args; return(TINODE);}
<cmd>vn(o(d(e?)?)?)?			{BEGIN args; return(TVNODE);}
<cmd>rn(o(d(e?)?)?)?			{BEGIN args; return(TRNODE);}
<cmd>cr(e(d?)?)?			{BEGIN args; return(TCRED);}
<cmd>pd(i(r?)?)?			{BEGIN args; return(TPDIR);}
<cmd>pr(o(c?)?)?			{BEGIN args; return(TPROC);}
<cmd>p(ri(n(t?)?)?)?			{BEGIN netop; return(TPRINT);}
%{ 
   /*  Commented out as a temp hack for networking
<cmd>n(e(t?)?)?				{BEGIN netop; return(TNET);}
   */
%}
<cmd>sh(m(e(m?)?)?)?			{BEGIN args; return(TSHMEM);}
<cmd>te(x(t?)?)?			{BEGIN args; return(TTEXT);}
<cmd>pty(i(n(f(o?)?)?)?)?		{BEGIN args; return(TPTYINFO);}
<cmd>ptty				{BEGIN args; return(TPTTY);}
<cmd>tty				{BEGIN args; return(TTTY);}
<cmd>du(m(p?)?)?			{BEGIN args; return(TDUMP);}
<cmd>xd(u(m(p?)?)?)?			{BEGIN args; return(TXDUMP);}
<cmd>bi(n(a(r(y)?)?)?)?			{BEGIN args; return(TBINARY);}
<cmd>vf(i(l(e?)?)?)?			{BEGIN args; return(TVFILE);}
<cmd>fi(l(e?)?)?			{BEGIN args; return(TFILE);}
<cmd>sea(r(c(h?)?)?)?			{BEGIN args; return(TSEARCH);}
<cmd>sem(a?)?				{BEGIN args; return(TSEMA);}
<cmd>setm(a(s(k?)?)?)?			{BEGIN args; return(TSETMASK);}
<cmd>lt(o(r?)?)?			{BEGIN args; return(TLTOR);}
<cmd>sym(b(o(l?)?)?)?			{BEGIN args; return(TSYM);}
<cmd>stk(t(r(c?)?)?)?			{BEGIN args; return(TSTKTRC);}
<cmd>stack(t(r(c?)?)?)?			{BEGIN args; return(TSTKTRC);}
<cmd>h(e(l(p?)?)?)?			{BEGIN args; return(THELP);}
<cmd>.					{BEGIN args; return(yytext[0]);}
<showop>s(y(m(b(o(l?)?)?)?)?)?		{BEGIN cmd; return(TSYMBOL);}
<showop>vms(t(a(t(s?)?)?)?)?		{BEGIN cmd; return(TVMSTATS);}
<showop>vma(d(d(r?)?)?)?		{BEGIN cmd; return(TVMADDR);}
<showop>i(o(s(t(a(t(s?)?)?)?)?)?)?	{BEGIN cmd; return(TIOSTATS);}
%{ 
   /*  Commented out as a temp hack for networking
<showop>n(e(t(w(o(r(k?)?)?)?)?)?)?	{BEGIN cmd; return(TNETWORK);}
   */
%}
<showop>d(n(l(c(s(t(a(t(s?)?)?)?)?)?)?)?)? {BEGIN cmd; return(TDNLCSTATS);}
<showop>[ \t]				{;}
<showop>"#".*\n				{BEGIN cmd; return('\n');}
<showop>\n				{BEGIN cmd; return('\n');}
<showop>">"				{BEGIN redir; yyless(yyleng-1);}
<showop>.				{BEGIN cmd; return(BADCHAR);}
<scanop>[\-A-Za-z]+			{BEGIN args; return(TSCANOP);}
<scanop>[ \t]				{;}
<scanop>"#".*\n				{BEGIN cmd; return('\n');}
<scanop>\n				{BEGIN cmd; return('\n');}
<scanop>">"				{BEGIN redir; yyless(yyleng-1);}
<scanop>.				{BEGIN cmd; return(BADCHAR);}
<driverop>[\-_A-Za-z0-9().:/>\t ]+   	{BEGIN args; return(TSTRING);}
<driverop>\n				{BEGIN cmd; return('\n');}
<netop>opt(i(o(n?)?)?)?			{ return(TNOPTION); }
<netop>(help|"?")			{BEGIN args; return(THELP);}
<netop>[_A-Za-z0-9]+			{BEGIN args; return(TSTRING);}
<netop>[ \t]				{;}
<netop>"#".*\n				{BEGIN cmd; return('\n');}
<netop>\n				{BEGIN cmd; return('\n');}
<netop>">"				{BEGIN redir; yyless(yyleng-1);}
<netop>.				{BEGIN cmd; return(BADCHAR);}
<args>"*"				{return('*');}
<args>"+"				{return('+');}
<args>"-"				{return('-');}
<args>"&"				{return('&');}
<args>"|"				{return('|');}
<args>"%"				{return('%');}
<args>"."				{return('.');}
<args>"="				{return('=');}
<args>"/"				{return('/');}
<args>"("				{return('(');}
<args>")"				{return(')');}
<args>"$"				{return('$');}
<args>"@"				{return('@');}
<args>[_A-Za-z][_A-Za-z0-9]+ {yylval = convertaddr(yytext,yyleng); return(NUMBER);}
<args>[A-Za-z][_A-Za-z0-9]+ {yylval = convertaddr(yytext,yyleng); return(NUMBER);}
<args>"0x"[A-Fa-f0-9]+	{yylval = converthex(yytext,yyleng); return(NUMBER);}
<args>[1-9][0-9]*	{yylval = convertdec(yytext,yyleng); return(NUMBER);}
<args>0[0-7]*		{yylval = convertoct(yytext,yyleng); return(NUMBER);}
<args>[a-z]			{return(yytext[0]);}
%{ 
   /*  Commented out as a temp hack for networking
<args>dtom/"("			{return(TDTOM);}
   */
%}
<args>cmhash/"("		{return(TCMHASH);}
<args>cmtopg/"("		{return(TCMTOPG);}
<args>pgtocm/"("		{return(TPGTOCM);}
<args>btoc/"("			{return(TBTOC);}
<args>ctob/"("			{return(TCTOB);}
<args>btop/"("			{return(TBTOP);}
<args>ptob/"("			{return(TPTOB);}
<args>kmxtob/"("		{return(TKMXTOB);}
<args>btokmx/"("		{return(TBTOKMX);}
<args>upadd/"("	 		{return(TUPADD);}
<args>uvadd/"("	 		{return(TUVADD);}
<args>pgtopde/"("		{return(TPGTOPDE);}
<args>vtoi/"("			{return(TVTOI);}
<args>itov/"("			{return(TITOV);}
<args>[ \t]			{;}
<args>"#".*\n			{BEGIN cmd; return('\n');}
<args>\n			{BEGIN cmd; return('\n');}
<args>">"			{BEGIN redir; yyless(yyleng-1);}
<args>.				{BEGIN cmd; return(BADCHAR);}
<redir>">"			{return(TGR);}
<redir>">>"			{return(TGR2);}
<redir>[^ \t\n]+		{BEGIN cmd; return(TPATH);}
<redir>[ \t]			{;}
<redir>"#".*\n			{BEGIN cmd; return('\n');}
<redir>\n			{BEGIN cmd; return('\n');}
<redir>.			{BEGIN cmd; return(BADCHAR);}
^\n				{BEGIN cmd; yyless(yyleng -1);}
^.				{BEGIN cmd; yyless(yyleng -1);}
%% 

/* low level conversion routines */
convertaddr(s,leng)
int leng;
char s[];
{
	int i;
	char buf[80];

	/* Copy yytext into local buffer and add \0 */
	if (leng > 79)
		leng = 79;
	for (i = 0; i < leng ; i++){
		buf[i] = s[i];
	}
	buf[i] = '\0';

	return(lookup(buf));
}



converthex(s,leng) 
int	leng;
char 	s[];
{
	int i,n=0;
	
	for (i=2; i < leng; ++i) {
		if(s[i] >= '0' && s[i] <= '9')
			n = 16 * n + s[i] - '0';
		if(s[i] >= 'a' && s[i] <= 'f') 
			n = 16 * n + (s[i] - 'a' + 10);
		if(s[i] >= 'A' && s[i] <= 'F')
			n = 16 * n + (s[i] - 'A' + 10);
		} 
	return(n);
}

convertdec(s,leng) 
int	leng;
char 	s[];
{
	int i,n=0;

	for (i=0; i < leng; ++i)
		n = 10 * n + s[i] - '0';
	return(n);
}


convertoct(s,leng) 
int	leng;
char 	s[];
{
	int i,n=0;

	for (i=0; i < leng; ++i)
		n = 8 * n + s[i] - '0';
	return(n);
}


/* do system command.  If command is empty, just fork a shell */
dosystem(s,len) 
int	len;
char 	*s;
{
	extern char *getenv();
	char *shell;

	if (len == 2) {
		if ((shell = getenv("SHELL")) == (char *) 0) 
			shell == "/bin/sh";
		system(shell);
	}
	else
		system(++s);
}

