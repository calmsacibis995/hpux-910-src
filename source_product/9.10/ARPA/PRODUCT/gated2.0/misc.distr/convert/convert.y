%{
#ifdef hpux
char sccsid[] = "@(#)conv_config 1.0\n";
char Version[] = "conv_config 1.0\n";
#endif

#include "convert.h"
#include "warnings.h"

extern unsigned char yytext[];

proto		p[3];

intf	*intfmetrics=NULL;

str_list *passiveintf=NULL, *martianlist=NULL; 

rt	*defaultrt, *routes;

neighbor	*egpneighbors, *group[MAX_GROUP];

unsigned int	traceflags=0;;

cntl *propagate, *accept;

str_list *as=NULL;

#define screate(x) (x *) malloc(sizeof(x))


%}
%token T_YES T_NO T_P2P T_QUIET T_SUPPLIER T_GW
%token T_EOL T_UNKNOWN T_COMMENT T_VALIDAS

%token T_ASYS T_EGPMAX T_EGPNEIGH T_METRICIN T_EGPMETRICOUT T_ASIN T_ASOUT
%token T_AS T_NOGENDEFAULT T_ACCEPTDEFAULT T_DEFAULTOUT T_VALIDATE T_INTF
%token T_SOURCENET T_GOODNAME

%token T_TRACE T_INTERNAL T_EXTERNAL T_ROUTE T_UPDATE T_ICMP T_STAMP T_GENERAL 
%token T_ALL

%token T_TRSTDRIPGW T_TRSTDHELLOGW T_SRCRIPGW T_SRCHELLOGW T_NORIPOUT 
%token T_NORIPFROM T_NOHELLOFROM T_NOHELLOOUT

%token T_PASSIVEINTF T_INTFMETRIC T_RECONSTMETRIC T_FIXEDMETRIC T_PROTO 
%token T_DONTLISTEN T_DONTLISTENHOST T_LISTEN T_LISTENHOST T_ANNOUNCE 
%token T_EGPMETRIC T_ANNOUNCEHOST  T_NOANNOUNCE T_NOANNOUNCEHOST 
%token T_DEFAULTEGPMETRIC T_DEFAULTGW T_EGPNET T_MARTIAN  T_NET T_HOST
%token T_PASSIVE T_ACTIVE T_METRIC

%token T_ANNOUNCEAS T_RESTRICT T_ASLIST T_NORESTRICT T_NOANNOUNCEAS 

%union	{
		char *ptr;
		str_list * strings;
		int  ival;
	}

%token  <strings>	T_INT T_RIP T_EGP T_HELLO
%type	<ival>		modes 
%type  	<strings>	gwno egpmetric metricopt protocol protocols
%type	<strings> 	ipaddrs symnames symname ifaddrs intlist
%token  <strings> 	T_IPADDR T_ALL T_GOODNAME

%start oldconf

%%

oldconf		:	stmts
		;

stmts		: 	stmts line 
		|	line 
		;

line		:	stmt T_EOL
		|	stmt T_COMMENT T_EOL
		|	T_COMMENT T_EOL
		|	T_EOL
		;

stmt 		:	ripstmt 
		|	hellostmt
		|	egpstmt 
		|	asstmt  
		|	egpmaxstmt 
		|	egpneighstmt 
		|	tracestmt
		|	trstdripgwstmt
		|	trstdhellogwstmt
		|	srcripgwstmt
		|	srchellogwstmt
		|	noripoutstmt
		|	nohellooutstmt
		|	noripfromstmt
		|	nohellofromstmt
		|	passivestmt
		|	intfmetricstmt
		|	recnstmetrcstmt
		|	fixdmetricstmt
		|	dontlistenstmt
		|	dontlistenhoststmt
		|	listenstmt
		|	listenhoststmt
		|	anncestmt
		|	anncehoststmt
		|	noanncestmt
		|	noanncehoststmt
		|	defltegpmetricstmt
		|	defltgwstmt
		|	egpnetstmt
		|	martianstmt
		|	validas
		|	netstmt
		|	hoststmt
		|	announceasstmt
		|	noannounceasstmt
		;

ripstmt		:	T_RIP modes 
			{
			    p[PR_RIP].mode = $2;
			}
		|	T_RIP modes T_GW gwno
			{
			    p[PR_RIP].mode = $2;
			    p[PR_RIP].gw_value = $4;
			    messages |= WARN_GATEWAY;
			}
		|	T_RIP T_GW gwno
			{
			    p[PR_RIP].mode = MODE_SUPPLIER;
			    p[PR_RIP].gw_value = $3;
			    messages |= WARN_GATEWAY;
			}
		;

hellostmt	:	T_HELLO modes 
			{
			    p[PR_HELLO].mode = $2;
			}
		|	T_HELLO modes T_GW gwno 
			{
			    p[PR_HELLO].mode = $2;
			    p[PR_HELLO].gw_value = $4;
			    messages |= WARN_GATEWAY;
			}
		|	T_HELLO T_GW gwno
			{
			    p[PR_HELLO].mode = MODE_SUPPLIER;
			    p[PR_HELLO].gw_value = $3;
			    messages |= WARN_GATEWAY;
			}
		;

egpstmt		:	T_EGP modes
			{
			    p[PR_EGP].mode = $2;
			}
		;

modes		:	T_YES 		{ $$ = MODE_ON; }
		|	T_NO 		{ $$ = MODE_OFF; }
		|	T_QUIET		{ $$ = MODE_QUIET; }
		|	T_P2P		{ $$ = MODE_P2P; }
		|	T_SUPPLIER	{ $$ = MODE_SUPPLIER; }
		;

gwno		:	T_INT	{ $$ = $1; }
		;
		

asstmt		:	T_ASYS T_INT 
			{
			    as = $2;
			}
		;

egpmaxstmt	:	T_EGPMAX T_INT 
			{
			    p[PR_EGP].maxup = $2;
			}
		;

egpneighstmt	:	T_EGPNEIGH symname {
			   struct _neighbor *en = screate(neighbor);
			     en->next = egpneighbors;
			     en->host = $2;
			     egpneighbors = en;
			} egpneighopts 
		|	T_EGPNEIGH symname
			{
			   struct _neighbor *en = screate(neighbor);
			     en->next = egpneighbors;
			     en->host = $2;
			     egpneighbors = en;
			} 
		;

egpneighopts	:	egpneighopt 
		|	egpneighopts egpneighopt
		;

egpneighopt	:	T_METRICIN T_INT  { messages |= WARN_METRICIN;}
		|	T_EGPMETRICOUT T_INT {egpneighbors->metricout=$2;}
		|	T_ASIN T_INT  {if (!egpneighbors->asin)
					   egpneighbors->asin=$2;
				       messages |= WARN_ASIN;
				      }
		|	T_ASOUT T_INT {egpneighbors->asout=$2;}
		|	T_AS T_INT   { egpneighbors->asin=$2;
				       messages |= WARN_ASIN;
				     }
		|	T_NOGENDEFAULT {egpneighbors->nogendefault= TRUE;}
		|	T_ACCEPTDEFAULT {egpneighbors->acceptdefault=TRUE;}
		|	T_DEFAULTOUT T_INT {egpneighbors->defaultout = TRUE;
					    messages |= WARN_DEFOUT;
				      }
		|	T_VALIDATE  {messages |= WARN_VALIDATE;}
		|	T_INTF symname {egpneighbors->interface = $2;}
		|	T_SOURCENET symname {egpneighbors->srcnet = $2;}
		|	T_GW symname {egpneighbors->gateway = $2;}
		;

tracestmt	:	T_TRACE {  if (traceflags) 
				      printf("Ignored duplicate traceflags statement in line %d\n",ln);
				} traceflags
		;

traceflags	:	traceflag
		|	traceflags traceflag
		;

traceflag	:	T_INTERNAL 	{ traceflags|=TR_INT;}
		|	T_EXTERNAL 	{ traceflags|=TR_EXT;}
		|	T_ROUTE 	{ traceflags|=TR_RTE;}
		|	T_EGP 		{ traceflags|=TR_EGP;}
		|	T_UPDATE 	{ traceflags|=TR_UPD;}
		|	T_RIP 		{ traceflags|=TR_RIP;}
		|	T_HELLO 	{ traceflags|=TR_HEL;}
		|	T_ICMP 		{ traceflags|=TR_ICMP;}
		|	T_STAMP 	{ traceflags|=TR_STMP;}
		|	T_GENERAL 	{ traceflags|=TR_GEN;
					  traceflags|=TR_EGP;
					  messages |= WARN_GENTRACE;
					}
		|	T_ALL 		{ traceflags|=TR_ALL;}
		;

trstdripgwstmt	: 	T_TRSTDRIPGW symnames
			{ 
			  if (p[PR_RIP].trustedgw != NULL)
			      catstr(p[PR_RIP].trustedgw,$2);
			  else
			      p[PR_RIP].trustedgw = $2;	
			}
		;
	
trstdhellogwstmt :	T_TRSTDHELLOGW symnames
			{
			  if (p[PR_HELLO].trustedgw != NULL)
			      catstr(p[PR_HELLO].trustedgw,$2);
			  else
			      p[PR_HELLO].trustedgw = $2;	
			}
		 ;

srcripgwstmt	:	T_SRCRIPGW symnames
			{ 
			  if (p[PR_RIP].srcgw != NULL)
			      catstr(p[PR_RIP].srcgw,$2);
			  else
			      p[PR_RIP].srcgw = $2;	
			}
		;

srchellogwstmt	:	T_SRCHELLOGW symnames
			{
			  if (p[PR_HELLO].srcgw != NULL)
			      catstr(p[PR_HELLO].srcgw,$2);
			  else
			      p[PR_HELLO].srcgw = $2;	
			}
		;

noripoutstmt	:	T_NORIPOUT ipaddrs  
			{ 
			  if (p[PR_RIP].noout != NULL ) 
				catstr(p[PR_RIP].noout,$2);
			  else
				p[PR_RIP].noout = $2;
			}
		;

nohellooutstmt	:	T_NOHELLOOUT ipaddrs
			{ 
			  if (p[PR_HELLO].noout != NULL ) 
				catstr(p[PR_HELLO].noout,$2);
			  else
				p[PR_HELLO].noout = $2;
			}
		;
	
noripfromstmt	:	T_NORIPFROM ipaddrs
			{ 
			  if (p[PR_RIP].noin != NULL ) 
				catstr(p[PR_RIP].noin,$2);
			  else
				p[PR_RIP].noin = $2;
			}
		;

nohellofromstmt	:	T_NOHELLOFROM ipaddrs
			{ 
			  if (p[PR_HELLO].noin != NULL ) 
				catstr(p[PR_HELLO].noin,$2);
			  else
				p[PR_HELLO].noin = $2;
			}
		;

passivestmt	:	T_PASSIVEINTF ifaddrs 
			{ 
			  if (passiveintf) 
				catstr(passiveintf,$2);
			  else
				passiveintf = $2;
			}
		;

intfmetricstmt	:	T_INTFMETRIC T_IPADDR T_INT
			{ 
			    intf *ifp;
			    ifp = screate(intf);
			    ifp->name = $2;
			    ifp->metric = $3;
			    ifp->next = intfmetrics;
			    intfmetrics =  ifp;
			}

		;

recnstmetrcstmt	:	T_RECONSTMETRIC T_IPADDR T_INT
			{ 
			    messages |= WARN_RECONST;
			}
		;

fixdmetricstmt	:	T_FIXEDMETRIC T_IPADDR T_PROTO protocol T_INT
			{ 
			    messages |= WARN_FIXMETRIC;
			}
		;


dontlistenstmt	: 	T_DONTLISTEN symname T_INTF ifaddrs T_PROTO protocols
			{ 
			    cntl *dln = screate(cntl);

			    dln->use = FALSE;
			    dln->addr = $2;
			    dln->intf = $4;
			    dln->proto = $6;
			    dln->next = accept;
			    accept = dln;
			}
		;

dontlistenhoststmt : T_DONTLISTENHOST symname T_INTF ifaddrs T_PROTO protocols
			{ 
			    cntl *dln = screate(cntl);

			    dln->use = FALSE;
			    dln->addr = $2;
			    dln->intf = $4;
			    dln->proto = $6;
			    dln->next = accept;
			    accept = dln;
			}
		   ;

listenstmt	:	T_LISTEN symname T_GW symnames T_PROTO protocol
			{   
			    cntl *ltn=screate(cntl);

			    ltn->use = TRUE;
			    ltn->addr = $2;
			    ltn->intf = $4;
			    ltn->proto = $6;
			    ltn->next = accept;
			    accept = ltn;
			}
		;

listenhoststmt	:	T_LISTENHOST symname T_GW symnames T_PROTO protocol
			{ 
			    cntl *ltn=screate(cntl);

			    ltn->use = TRUE;
			    ltn->addr = $2;
			    ltn->intf = $4;
			    ltn->proto = $6;
			    ltn->next = accept;
			    accept = ltn;
			}
		;

anncestmt	:  T_ANNOUNCE symname T_INTF ifaddrs T_PROTO protocols egpmetric
			{ 
			    cntl *lhn=screate(cntl);

			    lhn->use = TRUE;
			    lhn->addr = $2;
			    lhn->intf = $4;
			    lhn->proto = $6;
			    lhn->metric = $7;
			    lhn->next = propagate;
			    propagate = lhn;
			}
		;

anncehoststmt	:  T_ANNOUNCEHOST symname T_INTF ifaddrs T_PROTO protocols egpmetric
			{
			    cntl *lhn=screate(cntl);

			    lhn->use = TRUE;
			    lhn->addr = $2;
			    lhn->intf = $4;
			    lhn->proto = $6;
			    lhn->metric = $7;
			    lhn->next = propagate;
			    propagate = lhn;
			}
		;

noanncestmt	: T_NOANNOUNCE symname T_INTF ifaddrs T_PROTO protocols egpmetric
			{
			    cntl *lhn=screate(cntl);

			    lhn->use = FALSE;
			    lhn->addr = $2;
			    lhn->intf = $4;
			    lhn->proto = $6;
			    lhn->metric = $7;
			    lhn->next = propagate;
			    propagate = lhn;
			}
		;

noanncehoststmt	: T_NOANNOUNCEHOST symname T_INTF ifaddrs T_PROTO protocols egpmetric 
			{
			    cntl *lhn=screate(cntl);

			    lhn->use = FALSE;
			    lhn->addr = $2;
			    lhn->intf = $4;
			    lhn->proto = $6;
			    lhn->metric = $7;
			    lhn->next = propagate;
			    propagate = lhn;
			}
		;

egpmetric	:	T_EGPMETRIC T_INT	{ $$ = $2; }
		|				{ $$ = NULL; }
		;

defltegpmetricstmt :	T_DEFAULTEGPMETRIC T_INT 
			{
				p[PR_EGP].defaultmetric = $2;
			}
		   ;

defltgwstmt	   : 	T_DEFAULTGW symname protocol metricopt T_ACTIVE
			{
			    rt *dr=screate(rt);

			    dr->gw = $2;
			    dr->proto = $3;
			    dr->metric = $4;
			    dr->active = TRUE;
			    dr->next =defaultrt;
			    defaultrt = dr;
			}
		   |	T_DEFAULTGW symname protocol metricopt T_PASSIVE
			{
			    rt *dr=screate(rt);

			    dr->gw = $2;
			    dr->proto = $3;
			    dr->metric = $4;
			    dr->active = FALSE;
			    dr->next =defaultrt;
			    defaultrt = dr;
			}
		   ;


metricopt	:	T_METRIC T_INT	{$$ = $2;}
		|	{ $$ = 0;}
		;

egpnetstmt	   :	T_EGPNET symnames
			{
			    messages |= WARN_EGPNETS;
			}
		   ;

martianstmt	   : 	T_MARTIAN symnames
		 	{
			    if (martianlist)
				catstr(martianlist,$2);
			    else
				martianlist = $2;
			}
		   ;

validas		:	T_VALIDAS  symname T_AS symname T_METRIC T_INT
			{
				messages |= WARN_VALIDAS;
			}
		;

announceasstmt	:	T_ANNOUNCEAS  T_INT T_RESTRICT T_ASLIST intlist
			{
			    messages |= WARN_ANNCEAS;
			}
		|	T_ANNOUNCEAS  T_INT T_NORESTRICT T_ASLIST intlist
			{
			    messages |= WARN_ANNCEAS;
			}
		;

noannounceasstmt    :	T_NOANNOUNCEAS T_INT T_RESTRICT T_ASLIST intlist
			    {
				messages |= WARN_ANNCEAS;
			    }
		    |   T_NOANNOUNCEAS T_INT T_NORESTRICT T_ASLIST intlist
			    {
				messages |= WARN_ANNCEAS;
			    }
		    ;

netstmt		:    T_NET symname T_GW symname T_METRIC T_INT protocol
			{
			    rt *sr = screate(rt);

			    sr->addr = $2;
			    sr->gw = $4;
			    sr->metric = $6;
			    sr->proto = $7;
			    sr->next = routes;
			    routes = sr;
			}
		;

hoststmt	:    T_HOST symname T_GW symname T_METRIC T_INT protocol
			{
			    rt *sr = screate(rt);

			    sr->addr = $2;
			    sr->gw = $4;
			    sr->metric = $6;
			    sr->proto = $7;
			    sr->next = routes;
			    routes = sr;
			}
		;

ifaddrs		:	T_ALL   {$$=$1;}
		|	ipaddrs {$$=$1;}
		;

protocols	:	protocols protocol	{ $$=$2; $2->next = $1;}
		|	{ $$ = NULL; }
		;
			

protocol	:	T_EGP	{ $$ = $1; }
		|	T_RIP   { $$ = $1; }
		|	T_HELLO  { $$ = $1; }
		;

ipaddrs		:	T_IPADDR ipaddrs {$1->next = $2;  $$=$1;}
		|	T_IPADDR {$$ = $1; $1->next=NULL;}
		;

intlist		:	intlist T_INT { $$ = $2; 
					$2->next = $1;
				      }
		|	T_INT 	      { $$ = $1; }
		;

symnames	:	symname symnames  {$1->next = $2; $$=$1;}
		|	symname  {$$ = $1;}
		;

symname		:	T_IPADDR	{$$=$1; $1->next = NULL;}
		|	T_GOODNAME	{$$=$1; $1->next = NULL;}
		|	T_INT		{ $$=$1; $1->next=NULL;}
		;


%%

int ln=1;

main()
{
    char c[40], *a, fname[64], *novalue="-1", *modeprint(), *protoprint();
    int i, cf, res, prt, g;

    p[PR_RIP].mode = -1;		/* marked unused */
    p[PR_HELLO].mode = -1;	/* marked unused */
    p[PR_EGP].mode = -1;		/* marked unused */

    p[PR_RIP].gw_value  = screate(str_list);
    p[PR_RIP].gw_value->str = novalue;
    p[PR_HELLO].gw_value = p[PR_RIP].gw_value;

#ifdef 	DEBUG
    printf("Converting original config file to lower case\n");
#endif	/* DEBUG */

    sprintf(fname,"/tmp/#GateDCnv%d",getpid());

    if ( (cf=open(fname, O_RDWR | O_CREAT, 0666)) < 0)
	perror("open");

    while  ( (res=read(0,c,40)) > 0 ) {
	for (i=0, a=c; i < res; i++, a++)  
	    if ((*a >= 'A') && (*a <='Z')) *a=*a+0x20;
	write(cf,c,res);
    }
    if ( res < 0 ) perror("read");

#ifdef DEBUG
    printf("Move the lowercased config file to stdin\n");
#endif

    /* switch the lowercase configuration file to the stdin */

    close(0);
    dup(cf, 0);
    close(cf);
    lseek(0,0,SEEK_SET);

    /* ready for the parser to read the config file */

    yyparse();

    unlink(fname);

    printf("\n/*** Number of lines read from the old config file: %d  ***/\n\n",ln);

/* print out the trace statement */

    if (traceflags) 
    {
	printf("traceoptions");
	if ( (traceflags & TR_ALL) == TR_ALL) 
	    printf(" all");
	else 
	{
	    if ((traceflags & TR_GEN) == TR_GEN) 
		printf(" general egp");
	    else 
	    {
		if (traceflags & TR_INT) printf(" internal");
		if (traceflags & TR_EXT) printf(" external");
		if (traceflags & TR_RTE) printf(" route");
		if (traceflags & TR_EGP) printf(" egp");
	    }
	    if (traceflags & TR_UPD) printf(" update");
	    if (traceflags & TR_RIP) printf(" rip");
	    if (traceflags & TR_HEL) printf(" hello");
	    if (traceflags & TR_ICMP) printf(" icmp");
	    if (!(traceflags & TR_STMP)) printf(" nostamp");
	}
	printf(";\n\n");
    }
    warnprint(WARN_GENTRACE);

/* print out the autonomoussystem statement */

    if (as)
	printf("autonomoussystem %s;\n\n",as->str);

/* print out the interface statements */

    if ((passiveintf) || (intfmetrics))
    {
	intf *ifp = intfmetrics;

	while (ifp) 
	{
	    str_list *pif = passiveintf;
	    str_list *ppif = pif;

	    while (pif) 
	    {
		if ( strcmp(pif->str,ifp->name->str) == 0)
		{
		    printf("interface %s passive metric %s;\n",ifp->name->str,ifp->metric->str);
		    if (pif == passiveintf)
			passiveintf=pif->next;
		    else 
			ppif->next=pif->next;

		    break;
		}
		else
		{
		    ppif=pif;
		    pif=pif->next;
		}
	    }
	    if (!pif)	/* if a passive intf wasn't found, print metric alone */
		printf("interface %s metric %s;\n",ifp->name->str,ifp->metric->str);

	    ifp=ifp->next;
	}
	while ( passiveintf ) 
	{
	    printf("interface %s passive;\n",passiveintf->str);
	    passiveintf=passiveintf->next;
	}
	printf("\n");
    }

/* print out the martian list */

   if (martianlist)
   {
	printf("martians {\n");
	while (martianlist) 
	{
	    printf("    %s;\n",martianlist->str);
	    martianlist=martianlist->next;
	}
	printf("};\n\n");
   }

/* print out the RIP and HELLO protocol statements */
 
   for (prt=PR_RIP; prt<=PR_HELLO; prt++) 
   {
	if (p[prt].mode != -1) 		
   	{
	    int both=0;

	    printf("%s %s {\n",protoprint(prt), modeprint(p[prt].mode));

	    if (samestrs(p[prt].noin, p[prt].noout)) both=1;
		
            if (p[prt].noin)
	    {
	    	printf("    interface ");
            	while (p[prt].noin) 
	    	{
		    printf("%s ", p[prt].noin->str);
		    p[prt].noin = p[prt].noin->next;
	        }
		if (both)
	            printf("no%sin no%sout;\n",protoprint(prt),protoprint(prt));
		else
	            printf("no%sin;\n",protoprint(prt));
	    }
            if ((p[prt].noout) && !both)
	    {
	    	printf("    interface ");
            	while (p[prt].noout) 
	    	{
		    printf("%s ", p[prt].noout->str);
		    p[prt].noout = p[prt].noout->next;
	    	}
	    	printf("no%sout;\n",protoprint(prt));
	    }
	    if (p[prt].trustedgw)
	    {
	    	printf("    trustedgateways");
            	while (p[prt].trustedgw) 
	    	{
		    printf(" %s", p[prt].trustedgw->str);
		    p[prt].trustedgw = p[prt].trustedgw->next;
	    	}
	    	printf(";\n");
	    }
	    if (p[prt].srcgw)
	    {
	    	printf("    sourcegateways");
            	while (p[prt].srcgw) 
	    	{
	 	    printf(" %s", p[prt].srcgw->str);
		    p[prt].srcgw = p[prt].srcgw->next;
	    	}
	        printf(";\n");
	    }
    	    printf("};\n\n");
   	}
    }

	warnprint(WARN_GATEWAY);


    if (p[PR_EGP].mode != -1) {
	printf("egp %s {\n", modeprint(p[PR_EGP].mode));
	if ( p[PR_EGP].defaultmetric )
	    printf("    defaultmetric %s;\n", p[PR_EGP].defaultmetric->str);
	if (egpneighbors)
	{
	    g=group_neighbors();
	    for (i=0;i<=g;i++)
	    {
		printf("    group");
		if (group[i]->asin)
		    printf(" asin %s", group[i]->asin->str);
		if (group[i]->asout)
		    printf(" asout %s", group[i]->asout->str);
		if (p[PR_EGP].maxup)
		    printf(" maxup %s", p[PR_EGP].maxup->str);
		printf (" {\n");
		while (group[i]) 
		{
		    printf("\tneighbor %s",group[i]->host->str);
		    if (group[i]->metricout)
		    	printf(" metricout %s",group[i]->metricout->str);
		    if (group[i]->nogendefault)
		    	printf(" nogendefault");
		    if (group[i]->acceptdefault)
		    	printf(" acceptdefault");
		    if (group[i]->defaultout)
		    	printf(" propagatedefault");
		    if (group[i]->gateway)
		    	printf(" gateway %s",group[i]->gateway->str);
		    if (group[i]->interface)
		    	printf(" interface %s",group[i]->interface->str);
		    if (group[i]->srcnet)
		    	printf(" sourcenet %s",group[i]->srcnet->str);
		    printf(";\n");
		    group[i]=group[i]->next;
		}
		printf("    };\n");
	    }  /* loop through groups */
	}  /* egpneighbors */
	printf("};\n\n");
   } /* egp.mode != -1 */

   warnprint(WARN_METRICIN|WARN_ASIN|WARN_DEFOUT|WARN_VALIDATE);

/* Print out static route statements */

   if ((routes) || (defaultrt))
   {
	rt *r=routes;

	printf("static {\n");
	while (r)
	{
	    printf("    %s gateway %s;\n",r->addr->str,r->gw->str);
	    r=r->next;
	}
	if (defaultrt)
	{
	    if (defaultrt->active)
		printf("    default gateway %s preference 255;\n",defaultrt->gw->str);
	    else
		printf("    default gateway %s;\n",defaultrt->gw->str);
	}
	printf("};\n\n");
   }

   warnprint(WARN_DEFROUTE);


/* Print out accept statements */

   group_cntl(accept);

   while (accept) 
   {
	str_list *ca;

again:	printf("accept proto %s",accept->proto->str);
	if (accept->use != TRUE)	/* its a nolisten accept clause */
	{
	    if (strcmp(accept->intf->str,"all") != 0)
	         printf(" interface "); printstrs(accept->intf); 
	    printf(" {\n");
	    ca=accept->addr;
	    while (ca)
	    {
	    	printf("    nolisten %s;\n", ca->str);
		ca=ca->next;
	    }
	    ca=accept->opps;
	    while (ca)
	    {
	    	printf("    listen %s;\n", ca->str);
		ca=ca->next;
	    }
	    printf("};\n");
	}
	else				/* its a listen accept clause */
	{
	    printf(" gateway "); printstrs(accept->intf); printf(" {\n");
	    ca=accept->addr;
	    while (ca)
	    {
	         printf("    listen %s;\n", ca->str);
		 ca=ca->next;
	    }
	    ca=accept->opps;
	    while (ca)
	    {
		 printf("    nolisten %s;\n",ca->str);
		 ca=ca->next;
	    }
	    printf("};\n");
	}

	if (accept->proto->next)  /* there is another protocol for this rt */
	{
	     accept->proto = accept->proto->next;
	     goto again;
	}
	accept = accept->next;
    	printf("\n");
    }

    warnprint(WARN_VALIDAS);
	    
/* Print out propagate statements */

   group_cntl(propagate);

   if (defaultrt)
   {  
	printf("propagate proto %s", defaultrt->proto->str);
	if (defaultrt->metric)
	    printf(" metric %s",defaultrt->metric->str);
	else
	    if ((strcmp("rip",defaultrt->proto->str)==0) &&
		 (p[PR_RIP].gw_value) &&
		 strcmp(p[PR_RIP].gw_value->str,"-1") )
		printf(" metric %s",p[PR_RIP].gw_value->str);
	    else if ((strcmp("hello",defaultrt->proto->str)==0) &&
		     (p[PR_HELLO].gw_value) &&
		     strcmp(p[PR_HELLO].gw_value->str,"-1") )
		printf(" metric %s",p[PR_HELLO].gw_value->str);
	    else if (strcmp("egp",defaultrt->proto->str)==0)
		    if (as) printf(" as %s", as->str);
		    else printf(" as ???");
	printf(" {\n    proto static {\n\tannounce default;\n    };\n};\n\n");
   }
   while (propagate) 
   {
	str_list *ca;

again2:	if (strcmp(propagate->proto->str,"egp")==0)
	{
	    printf("/* At the '???' in the propagate statement below, place the AS number that \n   these routes should be propagated to. Then remove this comment */\n");
	    printf("/* propagate proto egp as ??? metric %s { \n",
			propagate->metric->str);
	    printf("     *** At the '???' in the proto statement below, place the protocol (method)\n     from which the routes were learned. Then remove this comment and\n     uncomment the statement. ***\n");
	    printf("     proto ??? {\n");
	    ca=propagate->addr;
	    while (ca)
	    {
	        if (propagate->use == TRUE)
		    printf("\tannounce %s;\n",ca->str);
	        else
		    printf("\tnoannounce %s;\n",ca->str);
		ca = ca->next;
	    }
	    ca=propagate->opps;
	    while (ca)
	    {
	        if (propagate->use != TRUE)
		    printf("\tannounce %s;\n",ca->str);
	        else
		    printf("\tnoannounce %s;\n",ca->str);
		ca = ca->next;
	    }
	    printf("    };\n};  */\n");
	}
	else
	{
	    printf("/*  propagate proto %s   ", propagate->proto->str);
	    if (strcmp(propagate->intf->str, "all") != 0)
	    {
		printf("interface "); printstrs(propagate->intf); 
	    }
	    printf("{\n     *** At the '???' in the statement below, place the protocol (method)\n     from which the routes were learned. Then remove this comment and\n     uncomment the statement. ***\n");
	    printf("    proto ??? {\n");
	    ca=propagate->addr;
	    while (ca)
	    {
		if (propagate->use == TRUE)
		    printf("\tannounce %s;\n",ca->str);
	        else
		    printf("\tnoannounce %s;\n",ca->str);
		ca = ca->next;
	    }
	    ca = propagate->opps;
	    while (ca)
	    {
	        if (propagate->use != TRUE)
		    printf("\tannounce %s;\n",ca->str);
	        else
		    printf("\tnoannounce %s;\n",ca->str);
		ca = ca->next;
	    }
	    printf("    };\n}; */\n");
	}
	if (propagate->proto->next)
	{
	    propagate->proto = propagate->proto->next;
	    goto again2;
	}
	propagate = propagate->next;

    	printf("\n"); 
    }
    warnprint(WARN_EGPNETS|WARN_ANNCEAS);

    warnprint(WARN_RECONST|WARN_FIXMETRIC);

}

printstrs( s1 )
   str_list *s1;
{
   str_list *t = s1;

   while (t != NULL) {
	printf("%s ", t->str);
	t=t->next;
   }
}

catstr( s1, s2)
str_list *s1, *s2;
{
  str_list *t = s1;

   if ( !s1 ) return;
   while (t->next != NULL) t=t->next;

   t->next=s2;
}

char *protoprint(p)
int p;
{
   switch(p)
   {
	case PR_RIP: return("rip"); 
		     break;
	case PR_HELLO: return("hello"); 
		     break;
	case PR_EGP: return("egp"); 
		     break;
   }
}

char *modeprint(m)
int m;
{
    switch(m)
    {
	case MODE_ON: return("on");
		      break;
	case MODE_OFF: return("off");
		       break;
	case MODE_SUPPLIER: return("supplier");
		       break;
	case MODE_QUIET: return("quiet");
		       break;
	case MODE_P2P: return("pointtopoint");
		       break;
   }
   return(NULL);
};

int group_neighbors()
{
    neighbor *pn, *n;
    int i=0;

    while (egpneighbors) 
    {
	group[i] = pn = egpneighbors;
	n = egpneighbors->next;
	egpneighbors=n;
	group[i]->next = NULL;
        while (n)
        {
	    if ((strcmp(group[i]->asin->str,n->asin->str) == 0) &&
	   	(strcmp(group[i]->asout->str,n->asout->str) == 0))
	    {
		neighbor *hold;

		/* add this neighbor to the current group chain */

		hold=n->next;		/* keep next spot in global chain */
		n->next = group[i]->next;
		group[i]->next = n;

		/* remove this neighbor from the global chain */

		if (n == egpneighbors)		
		    egpneighbors=hold;	/* was this neighbor at the top? */
		else {
		    pn->next = hold;	/* or down the chain */
		}
		n=hold;   /* move on to next neighbor */
		/* pn stays with the same neighbor, since this new one
		   is now next */
	    }
	    else 
	    {
		pn=n;
		n = n->next;
	    }
	}
        i++;
    }
    return(i-1);

}

group_cntl(clist)
cntl *clist;
{
    cntl *ctl=clist, *cur, *prev;

    while (ctl)
    {
	prev=ctl;
	cur=ctl->next;
        while (cur)
	{
	    if (  samestrs(cur->metric,ctl->metric)  &&
		  samestrs(cur->proto,ctl->proto) && 
		  samestrs(cur->intf,ctl->intf)      )
	    {
		if ((ctl->use) ^ (cur->use))
		{
		    cur->addr->next = ctl->opps;
		    ctl->opps = cur->addr;
		}
		else	/* they are both the same usage */
		{
		    cur->addr->next = ctl->addr;
		    ctl->addr = cur->addr;
		}
		prev->next=cur->next;
		cur = cur->next;
	    }
	    else
	    {
		prev=cur;
		cur = cur->next;
	    }
	}
	ctl = ctl->next;
    }
}

int samestrs(s1,s2)
str_list *s1, *s2;
{
    str_list *h1=s1, *h2=s2;

    if ((!h1 && h2) || (h1 && !h2))
	return(FALSE);
   if (countstr(h1) != countstr(h2))
	return(FALSE);

    while (h1) 
    {
	h2=s2;		/* set to top of list 2 */
	while (h2)
	{
	     if (strcmp(h1->str, h2->str) == 0)
		break;
	     h2=h2->next;
	}
	if (!h2) 	/* no match for h1->str was found in h2 list */
	    return(FALSE);
	h1 = h1->next;
   }
   return(TRUE);
}

int countstr(s)
str_list *s;
{
    int i=0;
    str_list *h=s;
  
    while (h)
    {
	i++;
	h=h->next;
    }
    return(i);
}

warnprint(flag)
unsigned int flag;
{
    int do_these = messages & flag;

    if (do_these)
    {
	printf("    /***** WARNINGS... \n\n");
	if (do_these & WARN_GENTRACE) 	
		printf("%s\n\n",warnings[0]);
	if (do_these & WARN_GATEWAY) 	
		printf("%s\n\n",warnings[1]);
	if (do_these & WARN_METRICIN) 	
		printf("%s\n\n",warnings[2]);
	if (do_these & WARN_ASIN) 		
		printf("%s\n\n",warnings[3]);
	if (do_these & WARN_DEFOUT) 	
		printf("%s\n\n",warnings[4]);
	if (do_these & WARN_VALIDATE) 	
		printf("%s\n\n",warnings[5]);
	if (do_these & WARN_RECONST) 	
		printf("%s\n\n",warnings[6]);
	if (do_these & WARN_FIXMETRIC) 	
		printf("%s\n\n",warnings[7]);
	if (do_these & WARN_DEFROUTE) 	
		printf("%s\n\n",warnings[8]);
	if (do_these & WARN_EGPNETS) 	
		printf("%s\n\n",warnings[9]);
	if (do_these & WARN_VALIDAS) 	
		printf("%s\n\n",warnings[10]);
	if (do_these & WARN_ANNCEAS) 	
		printf("%s\n\n",warnings[11]);
	printf("    *****/\n\n");
    }
}


yyerror()
{
	printf("Error: in line %d\n",ln);
        printf("Stopped at %s\n", yytext);

}

