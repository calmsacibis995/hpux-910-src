#ifdef __cplusplus
   #include <stdio.h>
   extern "C" {
     extern void yyerror(char *);
     extern int yylex();
   }
#endif	/* __cplusplus */ 

# line 2 "convert.y"
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



# line 53 "convert.y"
typedef union 	{
		char *ptr;
		str_list * strings;
		int  ival;
	} YYSTYPE;
# define T_YES 257
# define T_NO 258
# define T_P2P 259
# define T_QUIET 260
# define T_SUPPLIER 261
# define T_GW 262
# define T_EOL 263
# define T_UNKNOWN 264
# define T_COMMENT 265
# define T_VALIDAS 266
# define T_ASYS 267
# define T_EGPMAX 268
# define T_EGPNEIGH 269
# define T_METRICIN 270
# define T_EGPMETRICOUT 271
# define T_ASIN 272
# define T_ASOUT 273
# define T_AS 274
# define T_NOGENDEFAULT 275
# define T_ACCEPTDEFAULT 276
# define T_DEFAULTOUT 277
# define T_VALIDATE 278
# define T_INTF 279
# define T_SOURCENET 280
# define T_GOODNAME 281
# define T_TRACE 282
# define T_INTERNAL 283
# define T_EXTERNAL 284
# define T_ROUTE 285
# define T_UPDATE 286
# define T_ICMP 287
# define T_STAMP 288
# define T_GENERAL 289
# define T_ALL 290
# define T_TRSTDRIPGW 291
# define T_TRSTDHELLOGW 292
# define T_SRCRIPGW 293
# define T_SRCHELLOGW 294
# define T_NORIPOUT 295
# define T_NORIPFROM 296
# define T_NOHELLOFROM 297
# define T_NOHELLOOUT 298
# define T_PASSIVEINTF 299
# define T_INTFMETRIC 300
# define T_RECONSTMETRIC 301
# define T_FIXEDMETRIC 302
# define T_PROTO 303
# define T_DONTLISTEN 304
# define T_DONTLISTENHOST 305
# define T_LISTEN 306
# define T_LISTENHOST 307
# define T_ANNOUNCE 308
# define T_EGPMETRIC 309
# define T_ANNOUNCEHOST 310
# define T_NOANNOUNCE 311
# define T_NOANNOUNCEHOST 312
# define T_DEFAULTEGPMETRIC 313
# define T_DEFAULTGW 314
# define T_EGPNET 315
# define T_MARTIAN 316
# define T_NET 317
# define T_HOST 318
# define T_PASSIVE 319
# define T_ACTIVE 320
# define T_METRIC 321
# define T_ANNOUNCEAS 322
# define T_RESTRICT 323
# define T_ASLIST 324
# define T_NORESTRICT 325
# define T_NOANNOUNCEAS 326
# define T_INT 327
# define T_RIP 328
# define T_EGP 329
# define T_HELLO 330
# define T_IPADDR 331
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif

/* __YYSCLASS defines the scoping/storage class for global objects
 * that are NOT renamed by the -p option.  By default these names
 * are going to be 'static' so that multi-definition errors
 * will not occur with multiple parsers.
 * If you want (unsupported) access to internal names you need
 * to define this to be null so it implies 'extern' scope.
 * This should not be used in conjunction with -p.
 */
#ifndef __YYSCLASS
# define __YYSCLASS static
#endif
YYSTYPE yylval;
__YYSCLASS YYSTYPE yyval;
typedef int yytabelem;
# define YYERRCODE 256

# line 604 "convert.y"


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

__YYSCLASS yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 95,
	263, 61,
	265, 61,
	-2, 59,
	};
# define YYNPROD 143
# define YYLAST 408
__YYSCLASS yytabelem yyact[]={

     6,   256,     5,    74,    46,    47,    48,   168,   167,   169,
   262,   142,   143,   144,   146,   149,   150,   151,   152,    49,
   168,   167,   169,    97,   106,   115,   114,   113,    50,    51,
    52,    53,    54,    56,    57,    55,    58,    59,    60,    61,
   111,    62,    63,    64,    65,    66,   253,    67,    68,    69,
    70,    71,    72,    73,    75,    76,   147,   145,   148,    77,
   252,   251,   250,    78,   211,    43,    45,    44,   238,    98,
   233,   222,   218,    96,   217,   216,   215,   230,   214,   213,
   136,   106,   156,   155,   132,   131,   124,    94,    93,   175,
   173,   176,   174,   210,   209,   208,   236,   235,   234,   232,
   231,   229,   204,   228,   227,   226,   225,   224,   223,   157,
   192,   165,   164,   163,   162,   159,   158,   170,   181,   182,
   183,   184,   185,   186,   187,   188,   189,   190,   191,    85,
    86,    88,    87,    89,    91,    85,    86,    88,    87,    89,
    84,   254,    80,   133,    81,    85,    86,    88,    87,    89,
    82,   172,   171,   161,   160,   137,   134,   255,   242,   237,
   101,   180,   110,   141,     3,   135,    83,    79,   100,   140,
    99,   179,   139,    42,    41,    40,    39,    38,    37,    36,
    35,    34,    33,    32,    31,    30,    29,    28,   112,    27,
    26,    25,    24,    23,    22,    21,    20,    19,    18,    17,
    16,    15,    14,    13,    12,    11,    10,     9,     8,    95,
     7,    90,    92,     4,     2,     1,   203,     0,     0,     0,
   102,   103,   104,   116,   117,   118,   119,   120,   121,   122,
   123,     0,   125,     0,     0,   128,   129,   130,     0,     0,
     0,   126,   127,   105,   107,   108,   109,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   138,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   166,     0,     0,
   153,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   154,     0,     0,     0,   194,
   177,     0,     0,   178,   193,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   195,   196,     0,     0,   199,   200,   201,   202,   197,
   198,   205,   206,   207,     0,     0,     0,     0,     0,     0,
     0,   212,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   219,   220,   221,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   244,   245,   239,
   240,   241,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   243,     0,     0,   246,   247,   248,   249,
     0,     0,     0,   260,   261,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   257,   258,   259 };
__YYSCLASS yytabelem yypact[]={

  -263, -3000,  -263, -3000,  -121,  -113, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000,  -122,  -128,  -112,  -239,  -240,  -258, -3000,
  -258,  -258,  -258,  -258,  -307,  -307,  -307,  -307,  -250,  -304,
  -305,  -306,  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,
  -241,  -258,  -258,  -258,  -258,  -258,  -258,  -242,  -243, -3000,
 -3000,  -120, -3000,  -106,  -247, -3000, -3000, -3000, -3000, -3000,
  -107,  -247, -3000, -3000, -3000, -3000, -3000, -3000, -3000,  -272,
 -3000,  -258, -3000, -3000, -3000, -3000,  -307, -3000, -3000, -3000,
 -3000, -3000, -3000,  -244,  -245,  -194,  -163,  -164,  -108,  -109,
  -165,  -166,  -167,  -168, -3000,  -321, -3000, -3000,  -157,  -110,
  -111,  -233,  -234, -3000,  -247, -3000, -3000,  -247, -3000,  -152,
  -272, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000,  -321,  -250,  -250,
  -258,  -258,  -250,  -250,  -250,  -250,  -219, -3000, -3000, -3000,
  -258,  -258,  -258,  -229,  -230,  -231,  -260, -3000, -3000,  -152,
 -3000,  -248,  -249,  -251,  -252,  -253, -3000, -3000,  -255, -3000,
  -258,  -258,  -258, -3000,  -256,  -195,  -196,  -197,  -198,  -199,
  -200,  -202,  -226,  -220,  -257,  -223,  -224,  -225,  -259,  -259,
  -259,  -259, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000,  -321,  -321, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000,  -265,  -266,  -267,  -281, -3000,  -281,
  -281,  -281,  -321,  -321, -3000, -3000,  -308,  -308,  -308,  -308,
 -3000,  -321,  -321, -3000, -3000, -3000,  -317, -3000, -3000, -3000,
 -3000, -3000, -3000 };
__YYSCLASS yytabelem yypgo[]={

     0,   166,   165,   157,   216,   141,   158,   188,   168,   160,
   162,   159,   215,   214,   164,   213,   210,   208,   207,   206,
   205,   204,   203,   202,   201,   200,   199,   198,   197,   196,
   195,   194,   193,   192,   191,   190,   189,   187,   186,   185,
   184,   183,   182,   181,   180,   179,   178,   177,   176,   175,
   174,   173,   172,   171,   161,   170,   169,   163 };
__YYSCLASS yytabelem yyr1[]={

     0,    12,    13,    13,    14,    14,    14,    14,    15,    15,
    15,    15,    15,    15,    15,    15,    15,    15,    15,    15,
    15,    15,    15,    15,    15,    15,    15,    15,    15,    15,
    15,    15,    15,    15,    15,    15,    15,    15,    15,    15,
    15,    15,    15,    15,    16,    16,    16,    17,    17,    17,
    18,     1,     1,     1,     1,     1,     2,    19,    20,    52,
    21,    21,    53,    53,    54,    54,    54,    54,    54,    54,
    54,    54,    54,    54,    54,    54,    55,    22,    56,    56,
    57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
    57,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
    42,     3,     3,    43,    44,    44,     4,     4,    45,    46,
    47,    50,    50,    51,    51,    48,    49,    10,    10,     6,
     6,     5,     5,     5,     7,     7,    11,    11,     8,     8,
     9,     9,     9 };
__YYSCLASS yytabelem yyr2[]={

     0,     2,     4,     2,     4,     6,     4,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     5,     9,     7,     5,     9,     7,
     5,     3,     3,     3,     3,     3,     3,     5,     5,     1,
     8,     5,     2,     4,     5,     5,     5,     5,     5,     3,
     3,     5,     3,     5,     5,     5,     1,     6,     2,     4,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     5,     5,     5,     5,     5,     5,     5,     5,     5,
     7,     7,    11,    13,    13,    13,    13,    15,    15,    15,
    15,     5,     1,     5,    11,    11,     5,     1,     5,     5,
    13,    11,    11,    11,    11,    15,    15,     3,     3,     5,
     1,     3,     3,     3,     5,     3,     5,     3,     5,     3,
     3,     3,     3 };
__YYSCLASS yytabelem yychk[]={

 -3000,   -12,   -13,   -14,   -15,   265,   263,   -16,   -17,   -18,
   -19,   -20,   -21,   -22,   -23,   -24,   -25,   -26,   -27,   -28,
   -29,   -30,   -31,   -32,   -33,   -34,   -35,   -36,   -37,   -38,
   -39,   -40,   -41,   -42,   -43,   -44,   -45,   -46,   -47,   -48,
   -49,   -50,   -51,   328,   330,   329,   267,   268,   269,   282,
   291,   292,   293,   294,   295,   298,   296,   297,   299,   300,
   301,   302,   304,   305,   306,   307,   308,   310,   311,   312,
   313,   314,   315,   316,   266,   317,   318,   322,   326,   -14,
   263,   265,   263,    -1,   262,   257,   258,   260,   259,   261,
    -1,   262,    -1,   327,   327,    -9,   331,   281,   327,   -55,
    -8,    -9,    -8,    -8,    -8,    -7,   331,    -7,    -7,    -7,
   -10,   290,    -7,   331,   331,   331,    -9,    -9,    -9,    -9,
    -9,    -9,    -9,    -9,   327,    -9,    -8,    -8,    -9,    -9,
    -9,   327,   327,   263,   262,    -2,   327,   262,    -2,   -52,
   -56,   -57,   283,   284,   285,   329,   286,   328,   330,   287,
   288,   289,   290,    -8,    -7,   327,   327,   303,   279,   279,
   262,   262,   279,   279,   279,   279,    -5,   329,   328,   330,
   274,   262,   262,   323,   325,   323,   325,    -2,    -2,   -53,
   -54,   270,   271,   272,   273,   274,   275,   276,   277,   278,
   279,   280,   262,   -57,    -5,   -10,   -10,    -8,    -8,   -10,
   -10,   -10,   -10,    -4,   321,    -9,    -9,    -9,   324,   324,
   324,   324,   -54,   327,   327,   327,   327,   327,   327,    -9,
    -9,    -9,   327,   303,   303,   303,   303,   303,   303,   303,
   303,   320,   319,   327,   321,   321,   321,   -11,   327,   -11,
   -11,   -11,    -6,    -6,    -5,    -5,    -6,    -6,    -6,    -6,
   327,   327,   327,   327,    -5,    -3,   309,    -3,    -3,    -3,
    -5,    -5,   327 };
__YYSCLASS yytabelem yydef[]={

     0,    -2,     1,     3,     0,     0,     7,     8,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
    31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
    41,    42,    43,     0,     0,     0,     0,     0,     0,    76,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     2,
     4,     0,     6,    44,     0,    51,    52,    53,    54,    55,
    47,     0,    50,    57,    58,    -2,   140,   141,   142,     0,
    91,   139,    92,    93,    94,    95,   135,    96,    97,    98,
    99,   127,   128,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,   113,     0,   118,   119,     0,     0,
     0,     0,     0,     5,     0,    46,    56,     0,    49,     0,
    77,    78,    80,    81,    82,    83,    84,    85,    86,    87,
    88,    89,    90,   138,   134,   100,   101,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   117,   131,   132,   133,
     0,     0,     0,     0,     0,     0,     0,    45,    48,    60,
    62,     0,     0,     0,     0,     0,    69,    70,     0,    72,
     0,     0,     0,    79,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    63,    64,    65,    66,    67,    68,    71,    73,
    74,    75,   102,   130,   130,     0,     0,   130,   130,   130,
   130,   114,   115,   116,     0,     0,     0,   121,   137,   122,
   123,   124,   103,   104,   105,   106,   112,   112,   112,   112,
   120,     0,     0,   136,   129,   107,     0,   108,   109,   110,
   125,   126,   111 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

__YYSCLASS yytoktype yytoks[] =
{
	"T_YES",	257,
	"T_NO",	258,
	"T_P2P",	259,
	"T_QUIET",	260,
	"T_SUPPLIER",	261,
	"T_GW",	262,
	"T_EOL",	263,
	"T_UNKNOWN",	264,
	"T_COMMENT",	265,
	"T_VALIDAS",	266,
	"T_ASYS",	267,
	"T_EGPMAX",	268,
	"T_EGPNEIGH",	269,
	"T_METRICIN",	270,
	"T_EGPMETRICOUT",	271,
	"T_ASIN",	272,
	"T_ASOUT",	273,
	"T_AS",	274,
	"T_NOGENDEFAULT",	275,
	"T_ACCEPTDEFAULT",	276,
	"T_DEFAULTOUT",	277,
	"T_VALIDATE",	278,
	"T_INTF",	279,
	"T_SOURCENET",	280,
	"T_GOODNAME",	281,
	"T_TRACE",	282,
	"T_INTERNAL",	283,
	"T_EXTERNAL",	284,
	"T_ROUTE",	285,
	"T_UPDATE",	286,
	"T_ICMP",	287,
	"T_STAMP",	288,
	"T_GENERAL",	289,
	"T_ALL",	290,
	"T_TRSTDRIPGW",	291,
	"T_TRSTDHELLOGW",	292,
	"T_SRCRIPGW",	293,
	"T_SRCHELLOGW",	294,
	"T_NORIPOUT",	295,
	"T_NORIPFROM",	296,
	"T_NOHELLOFROM",	297,
	"T_NOHELLOOUT",	298,
	"T_PASSIVEINTF",	299,
	"T_INTFMETRIC",	300,
	"T_RECONSTMETRIC",	301,
	"T_FIXEDMETRIC",	302,
	"T_PROTO",	303,
	"T_DONTLISTEN",	304,
	"T_DONTLISTENHOST",	305,
	"T_LISTEN",	306,
	"T_LISTENHOST",	307,
	"T_ANNOUNCE",	308,
	"T_EGPMETRIC",	309,
	"T_ANNOUNCEHOST",	310,
	"T_NOANNOUNCE",	311,
	"T_NOANNOUNCEHOST",	312,
	"T_DEFAULTEGPMETRIC",	313,
	"T_DEFAULTGW",	314,
	"T_EGPNET",	315,
	"T_MARTIAN",	316,
	"T_NET",	317,
	"T_HOST",	318,
	"T_PASSIVE",	319,
	"T_ACTIVE",	320,
	"T_METRIC",	321,
	"T_ANNOUNCEAS",	322,
	"T_RESTRICT",	323,
	"T_ASLIST",	324,
	"T_NORESTRICT",	325,
	"T_NOANNOUNCEAS",	326,
	"T_INT",	327,
	"T_RIP",	328,
	"T_EGP",	329,
	"T_HELLO",	330,
	"T_IPADDR",	331,
	"-unknown-",	-1	/* ends search */
};

__YYSCLASS char * yyreds[] =
{
	"-no such reduction-",
	"oldconf : stmts",
	"stmts : stmts line",
	"stmts : line",
	"line : stmt T_EOL",
	"line : stmt T_COMMENT T_EOL",
	"line : T_COMMENT T_EOL",
	"line : T_EOL",
	"stmt : ripstmt",
	"stmt : hellostmt",
	"stmt : egpstmt",
	"stmt : asstmt",
	"stmt : egpmaxstmt",
	"stmt : egpneighstmt",
	"stmt : tracestmt",
	"stmt : trstdripgwstmt",
	"stmt : trstdhellogwstmt",
	"stmt : srcripgwstmt",
	"stmt : srchellogwstmt",
	"stmt : noripoutstmt",
	"stmt : nohellooutstmt",
	"stmt : noripfromstmt",
	"stmt : nohellofromstmt",
	"stmt : passivestmt",
	"stmt : intfmetricstmt",
	"stmt : recnstmetrcstmt",
	"stmt : fixdmetricstmt",
	"stmt : dontlistenstmt",
	"stmt : dontlistenhoststmt",
	"stmt : listenstmt",
	"stmt : listenhoststmt",
	"stmt : anncestmt",
	"stmt : anncehoststmt",
	"stmt : noanncestmt",
	"stmt : noanncehoststmt",
	"stmt : defltegpmetricstmt",
	"stmt : defltgwstmt",
	"stmt : egpnetstmt",
	"stmt : martianstmt",
	"stmt : validas",
	"stmt : netstmt",
	"stmt : hoststmt",
	"stmt : announceasstmt",
	"stmt : noannounceasstmt",
	"ripstmt : T_RIP modes",
	"ripstmt : T_RIP modes T_GW gwno",
	"ripstmt : T_RIP T_GW gwno",
	"hellostmt : T_HELLO modes",
	"hellostmt : T_HELLO modes T_GW gwno",
	"hellostmt : T_HELLO T_GW gwno",
	"egpstmt : T_EGP modes",
	"modes : T_YES",
	"modes : T_NO",
	"modes : T_QUIET",
	"modes : T_P2P",
	"modes : T_SUPPLIER",
	"gwno : T_INT",
	"asstmt : T_ASYS T_INT",
	"egpmaxstmt : T_EGPMAX T_INT",
	"egpneighstmt : T_EGPNEIGH symname",
	"egpneighstmt : T_EGPNEIGH symname egpneighopts",
	"egpneighstmt : T_EGPNEIGH symname",
	"egpneighopts : egpneighopt",
	"egpneighopts : egpneighopts egpneighopt",
	"egpneighopt : T_METRICIN T_INT",
	"egpneighopt : T_EGPMETRICOUT T_INT",
	"egpneighopt : T_ASIN T_INT",
	"egpneighopt : T_ASOUT T_INT",
	"egpneighopt : T_AS T_INT",
	"egpneighopt : T_NOGENDEFAULT",
	"egpneighopt : T_ACCEPTDEFAULT",
	"egpneighopt : T_DEFAULTOUT T_INT",
	"egpneighopt : T_VALIDATE",
	"egpneighopt : T_INTF symname",
	"egpneighopt : T_SOURCENET symname",
	"egpneighopt : T_GW symname",
	"tracestmt : T_TRACE",
	"tracestmt : T_TRACE traceflags",
	"traceflags : traceflag",
	"traceflags : traceflags traceflag",
	"traceflag : T_INTERNAL",
	"traceflag : T_EXTERNAL",
	"traceflag : T_ROUTE",
	"traceflag : T_EGP",
	"traceflag : T_UPDATE",
	"traceflag : T_RIP",
	"traceflag : T_HELLO",
	"traceflag : T_ICMP",
	"traceflag : T_STAMP",
	"traceflag : T_GENERAL",
	"traceflag : T_ALL",
	"trstdripgwstmt : T_TRSTDRIPGW symnames",
	"trstdhellogwstmt : T_TRSTDHELLOGW symnames",
	"srcripgwstmt : T_SRCRIPGW symnames",
	"srchellogwstmt : T_SRCHELLOGW symnames",
	"noripoutstmt : T_NORIPOUT ipaddrs",
	"nohellooutstmt : T_NOHELLOOUT ipaddrs",
	"noripfromstmt : T_NORIPFROM ipaddrs",
	"nohellofromstmt : T_NOHELLOFROM ipaddrs",
	"passivestmt : T_PASSIVEINTF ifaddrs",
	"intfmetricstmt : T_INTFMETRIC T_IPADDR T_INT",
	"recnstmetrcstmt : T_RECONSTMETRIC T_IPADDR T_INT",
	"fixdmetricstmt : T_FIXEDMETRIC T_IPADDR T_PROTO protocol T_INT",
	"dontlistenstmt : T_DONTLISTEN symname T_INTF ifaddrs T_PROTO protocols",
	"dontlistenhoststmt : T_DONTLISTENHOST symname T_INTF ifaddrs T_PROTO protocols",
	"listenstmt : T_LISTEN symname T_GW symnames T_PROTO protocol",
	"listenhoststmt : T_LISTENHOST symname T_GW symnames T_PROTO protocol",
	"anncestmt : T_ANNOUNCE symname T_INTF ifaddrs T_PROTO protocols egpmetric",
	"anncehoststmt : T_ANNOUNCEHOST symname T_INTF ifaddrs T_PROTO protocols egpmetric",
	"noanncestmt : T_NOANNOUNCE symname T_INTF ifaddrs T_PROTO protocols egpmetric",
	"noanncehoststmt : T_NOANNOUNCEHOST symname T_INTF ifaddrs T_PROTO protocols egpmetric",
	"egpmetric : T_EGPMETRIC T_INT",
	"egpmetric : /* empty */",
	"defltegpmetricstmt : T_DEFAULTEGPMETRIC T_INT",
	"defltgwstmt : T_DEFAULTGW symname protocol metricopt T_ACTIVE",
	"defltgwstmt : T_DEFAULTGW symname protocol metricopt T_PASSIVE",
	"metricopt : T_METRIC T_INT",
	"metricopt : /* empty */",
	"egpnetstmt : T_EGPNET symnames",
	"martianstmt : T_MARTIAN symnames",
	"validas : T_VALIDAS symname T_AS symname T_METRIC T_INT",
	"announceasstmt : T_ANNOUNCEAS T_INT T_RESTRICT T_ASLIST intlist",
	"announceasstmt : T_ANNOUNCEAS T_INT T_NORESTRICT T_ASLIST intlist",
	"noannounceasstmt : T_NOANNOUNCEAS T_INT T_RESTRICT T_ASLIST intlist",
	"noannounceasstmt : T_NOANNOUNCEAS T_INT T_NORESTRICT T_ASLIST intlist",
	"netstmt : T_NET symname T_GW symname T_METRIC T_INT protocol",
	"hoststmt : T_HOST symname T_GW symname T_METRIC T_INT protocol",
	"ifaddrs : T_ALL",
	"ifaddrs : ipaddrs",
	"protocols : protocols protocol",
	"protocols : /* empty */",
	"protocol : T_EGP",
	"protocol : T_RIP",
	"protocol : T_HELLO",
	"ipaddrs : T_IPADDR ipaddrs",
	"ipaddrs : T_IPADDR",
	"intlist : intlist T_INT",
	"intlist : T_INT",
	"symnames : symname symnames",
	"symnames : symname",
	"symname : T_IPADDR",
	"symname : T_GOODNAME",
	"symname : T_INT",
};
#endif /* YYDEBUG */
#define YYFLAG  (-3000)
/* @(#) $Revision: 66.3 $ */    

/*
** Skeleton parser driver for yacc output
*/

#if defined(NLS) && !defined(NL_SETN)
#include <msgbuf.h>
#endif

#ifndef nl_msg
#define nl_msg(i,s) (s)
#endif

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab

#ifndef __RUNTIME_YYMAXDEPTH
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#else
#define YYACCEPT	{free_stacks(); return(0);}
#define YYABORT		{free_stacks(); return(1);}
#endif

#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( (nl_msg(30001,"syntax error - cannot backup")) );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
/* define for YYFLAG now generated by yacc program. */
/*#define YYFLAG		(FLAGVAL)*/

/*
** global variables used by the parser
*/
# ifndef __RUNTIME_YYMAXDEPTH
__YYSCLASS YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
__YYSCLASS int yys[ YYMAXDEPTH ];		/* state stack */
# else
__YYSCLASS YYSTYPE *yyv;			/* pointer to malloc'ed value stack */
__YYSCLASS int *yys;			/* pointer to malloc'ed stack stack */
#ifdef __cplusplus
	extern char *malloc(int);
	extern char *realloc(char *, int);
	extern void free();
# else
	extern char *malloc();
	extern char *realloc();
	extern void free();
# endif /* __cplusplus */
static int allocate_stacks(); 
static void free_stacks();
# ifndef YYINCREMENT
# define YYINCREMENT (YYMAXDEPTH/2) + 10
# endif
# endif	/* __RUNTIME_YYMAXDEPTH */
long  yymaxdepth = YYMAXDEPTH;

__YYSCLASS YYSTYPE *yypv;			/* top of value stack */
__YYSCLASS int *yyps;			/* top of state stack */

__YYSCLASS int yystate;			/* current state */
__YYSCLASS int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
__YYSCLASS int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */

	/*
	** Initialize externals - yyparse may be called more than once
	*/
# ifdef __RUNTIME_YYMAXDEPTH
	if (allocate_stacks()) YYABORT;
# endif
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
# ifndef __RUNTIME_YYMAXDEPTH
			yyerror( (nl_msg(30002,"yacc stack overflow")) );
			YYABORT;
# else
			/* save old stack bases to recalculate pointers */
			YYSTYPE * yyv_old = yyv;
			int * yys_old = yys;
			yymaxdepth += YYINCREMENT;
			yys = (int *) realloc(yys, yymaxdepth * sizeof(int));
			yyv = (YYSTYPE *) realloc(yyv, yymaxdepth * sizeof(YYSTYPE));
			if (yys==0 || yyv==0) {
			    yyerror( (nl_msg(30002,"yacc stack overflow")) );
			    YYABORT;
			    }
			/* Reset pointers into stack */
			yy_ps = (yy_ps - yys_old) + yys;
			yyps = (yyps - yys_old) + yys;
			yy_pv = (yy_pv - yyv_old) + yyv;
			yypv = (yypv - yyv_old) + yyv;
# endif

		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( (nl_msg(30003,"syntax error")) );
				yynerrs++;
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 44:
# line 121 "convert.y"
{
			    p[PR_RIP].mode = yypvt[-0].ival;
			} break;
case 45:
# line 125 "convert.y"
{
			    p[PR_RIP].mode = yypvt[-2].ival;
			    p[PR_RIP].gw_value = yypvt[-0].strings;
			    messages |= WARN_GATEWAY;
			} break;
case 46:
# line 131 "convert.y"
{
			    p[PR_RIP].mode = MODE_SUPPLIER;
			    p[PR_RIP].gw_value = yypvt[-0].strings;
			    messages |= WARN_GATEWAY;
			} break;
case 47:
# line 139 "convert.y"
{
			    p[PR_HELLO].mode = yypvt[-0].ival;
			} break;
case 48:
# line 143 "convert.y"
{
			    p[PR_HELLO].mode = yypvt[-2].ival;
			    p[PR_HELLO].gw_value = yypvt[-0].strings;
			    messages |= WARN_GATEWAY;
			} break;
case 49:
# line 149 "convert.y"
{
			    p[PR_HELLO].mode = MODE_SUPPLIER;
			    p[PR_HELLO].gw_value = yypvt[-0].strings;
			    messages |= WARN_GATEWAY;
			} break;
case 50:
# line 157 "convert.y"
{
			    p[PR_EGP].mode = yypvt[-0].ival;
			} break;
case 51:
# line 162 "convert.y"
{ yyval.ival = MODE_ON; } break;
case 52:
# line 163 "convert.y"
{ yyval.ival = MODE_OFF; } break;
case 53:
# line 164 "convert.y"
{ yyval.ival = MODE_QUIET; } break;
case 54:
# line 165 "convert.y"
{ yyval.ival = MODE_P2P; } break;
case 55:
# line 166 "convert.y"
{ yyval.ival = MODE_SUPPLIER; } break;
case 56:
# line 169 "convert.y"
{ yyval.strings = yypvt[-0].strings; } break;
case 57:
# line 174 "convert.y"
{
			    as = yypvt[-0].strings;
			} break;
case 58:
# line 180 "convert.y"
{
			    p[PR_EGP].maxup = yypvt[-0].strings;
			} break;
case 59:
# line 185 "convert.y"
{
			   struct _neighbor *en = screate(neighbor);
			     en->next = egpneighbors;
			     en->host = yypvt[-0].strings;
			     egpneighbors = en;
			} break;
case 61:
# line 192 "convert.y"
{
			   struct _neighbor *en = screate(neighbor);
			     en->next = egpneighbors;
			     en->host = yypvt[-0].strings;
			     egpneighbors = en;
			} break;
case 64:
# line 204 "convert.y"
{ messages |= WARN_METRICIN;} break;
case 65:
# line 205 "convert.y"
{egpneighbors->metricout=yypvt[-0].strings;} break;
case 66:
# line 206 "convert.y"
{if (!egpneighbors->asin)
					   egpneighbors->asin=yypvt[-0].strings;
				       messages |= WARN_ASIN;
				      } break;
case 67:
# line 210 "convert.y"
{egpneighbors->asout=yypvt[-0].strings;} break;
case 68:
# line 211 "convert.y"
{ egpneighbors->asin=yypvt[-0].strings;
				       messages |= WARN_ASIN;
				     } break;
case 69:
# line 214 "convert.y"
{egpneighbors->nogendefault= TRUE;} break;
case 70:
# line 215 "convert.y"
{egpneighbors->acceptdefault=TRUE;} break;
case 71:
# line 216 "convert.y"
{egpneighbors->defaultout = TRUE;
					    messages |= WARN_DEFOUT;
				      } break;
case 72:
# line 219 "convert.y"
{messages |= WARN_VALIDATE;} break;
case 73:
# line 220 "convert.y"
{egpneighbors->interface = yypvt[-0].strings;} break;
case 74:
# line 221 "convert.y"
{egpneighbors->srcnet = yypvt[-0].strings;} break;
case 75:
# line 222 "convert.y"
{egpneighbors->gateway = yypvt[-0].strings;} break;
case 76:
# line 225 "convert.y"
{  if (traceflags) 
				      printf("Ignored duplicate traceflags statement in line %d\n",ln);
				} break;
case 80:
# line 234 "convert.y"
{ traceflags|=TR_INT;} break;
case 81:
# line 235 "convert.y"
{ traceflags|=TR_EXT;} break;
case 82:
# line 236 "convert.y"
{ traceflags|=TR_RTE;} break;
case 83:
# line 237 "convert.y"
{ traceflags|=TR_EGP;} break;
case 84:
# line 238 "convert.y"
{ traceflags|=TR_UPD;} break;
case 85:
# line 239 "convert.y"
{ traceflags|=TR_RIP;} break;
case 86:
# line 240 "convert.y"
{ traceflags|=TR_HEL;} break;
case 87:
# line 241 "convert.y"
{ traceflags|=TR_ICMP;} break;
case 88:
# line 242 "convert.y"
{ traceflags|=TR_STMP;} break;
case 89:
# line 243 "convert.y"
{ traceflags|=TR_GEN;
					  traceflags|=TR_EGP;
					  messages |= WARN_GENTRACE;
					} break;
case 90:
# line 247 "convert.y"
{ traceflags|=TR_ALL;} break;
case 91:
# line 251 "convert.y"
{ 
			  if (p[PR_RIP].trustedgw != NULL)
			      catstr(p[PR_RIP].trustedgw,yypvt[-0].strings);
			  else
			      p[PR_RIP].trustedgw = yypvt[-0].strings;	
			} break;
case 92:
# line 260 "convert.y"
{
			  if (p[PR_HELLO].trustedgw != NULL)
			      catstr(p[PR_HELLO].trustedgw,yypvt[-0].strings);
			  else
			      p[PR_HELLO].trustedgw = yypvt[-0].strings;	
			} break;
case 93:
# line 269 "convert.y"
{ 
			  if (p[PR_RIP].srcgw != NULL)
			      catstr(p[PR_RIP].srcgw,yypvt[-0].strings);
			  else
			      p[PR_RIP].srcgw = yypvt[-0].strings;	
			} break;
case 94:
# line 278 "convert.y"
{
			  if (p[PR_HELLO].srcgw != NULL)
			      catstr(p[PR_HELLO].srcgw,yypvt[-0].strings);
			  else
			      p[PR_HELLO].srcgw = yypvt[-0].strings;	
			} break;
case 95:
# line 287 "convert.y"
{ 
			  if (p[PR_RIP].noout != NULL ) 
				catstr(p[PR_RIP].noout,yypvt[-0].strings);
			  else
				p[PR_RIP].noout = yypvt[-0].strings;
			} break;
case 96:
# line 296 "convert.y"
{ 
			  if (p[PR_HELLO].noout != NULL ) 
				catstr(p[PR_HELLO].noout,yypvt[-0].strings);
			  else
				p[PR_HELLO].noout = yypvt[-0].strings;
			} break;
case 97:
# line 305 "convert.y"
{ 
			  if (p[PR_RIP].noin != NULL ) 
				catstr(p[PR_RIP].noin,yypvt[-0].strings);
			  else
				p[PR_RIP].noin = yypvt[-0].strings;
			} break;
case 98:
# line 314 "convert.y"
{ 
			  if (p[PR_HELLO].noin != NULL ) 
				catstr(p[PR_HELLO].noin,yypvt[-0].strings);
			  else
				p[PR_HELLO].noin = yypvt[-0].strings;
			} break;
case 99:
# line 323 "convert.y"
{ 
			  if (passiveintf) 
				catstr(passiveintf,yypvt[-0].strings);
			  else
				passiveintf = yypvt[-0].strings;
			} break;
case 100:
# line 332 "convert.y"
{ 
			    intf *ifp;
			    ifp = screate(intf);
			    ifp->name = yypvt[-1].strings;
			    ifp->metric = yypvt[-0].strings;
			    ifp->next = intfmetrics;
			    intfmetrics =  ifp;
			} break;
case 101:
# line 344 "convert.y"
{ 
			    messages |= WARN_RECONST;
			} break;
case 102:
# line 350 "convert.y"
{ 
			    messages |= WARN_FIXMETRIC;
			} break;
case 103:
# line 357 "convert.y"
{ 
			    cntl *dln = screate(cntl);

			    dln->use = FALSE;
			    dln->addr = yypvt[-4].strings;
			    dln->intf = yypvt[-2].strings;
			    dln->proto = yypvt[-0].strings;
			    dln->next = accept;
			    accept = dln;
			} break;
case 104:
# line 370 "convert.y"
{ 
			    cntl *dln = screate(cntl);

			    dln->use = FALSE;
			    dln->addr = yypvt[-4].strings;
			    dln->intf = yypvt[-2].strings;
			    dln->proto = yypvt[-0].strings;
			    dln->next = accept;
			    accept = dln;
			} break;
case 105:
# line 383 "convert.y"
{   
			    cntl *ltn=screate(cntl);

			    ltn->use = TRUE;
			    ltn->addr = yypvt[-4].strings;
			    ltn->intf = yypvt[-2].strings;
			    ltn->proto = yypvt[-0].strings;
			    ltn->next = accept;
			    accept = ltn;
			} break;
case 106:
# line 396 "convert.y"
{ 
			    cntl *ltn=screate(cntl);

			    ltn->use = TRUE;
			    ltn->addr = yypvt[-4].strings;
			    ltn->intf = yypvt[-2].strings;
			    ltn->proto = yypvt[-0].strings;
			    ltn->next = accept;
			    accept = ltn;
			} break;
case 107:
# line 409 "convert.y"
{ 
			    cntl *lhn=screate(cntl);

			    lhn->use = TRUE;
			    lhn->addr = yypvt[-5].strings;
			    lhn->intf = yypvt[-3].strings;
			    lhn->proto = yypvt[-1].strings;
			    lhn->metric = yypvt[-0].strings;
			    lhn->next = propagate;
			    propagate = lhn;
			} break;
case 108:
# line 423 "convert.y"
{
			    cntl *lhn=screate(cntl);

			    lhn->use = TRUE;
			    lhn->addr = yypvt[-5].strings;
			    lhn->intf = yypvt[-3].strings;
			    lhn->proto = yypvt[-1].strings;
			    lhn->metric = yypvt[-0].strings;
			    lhn->next = propagate;
			    propagate = lhn;
			} break;
case 109:
# line 437 "convert.y"
{
			    cntl *lhn=screate(cntl);

			    lhn->use = FALSE;
			    lhn->addr = yypvt[-5].strings;
			    lhn->intf = yypvt[-3].strings;
			    lhn->proto = yypvt[-1].strings;
			    lhn->metric = yypvt[-0].strings;
			    lhn->next = propagate;
			    propagate = lhn;
			} break;
case 110:
# line 451 "convert.y"
{
			    cntl *lhn=screate(cntl);

			    lhn->use = FALSE;
			    lhn->addr = yypvt[-5].strings;
			    lhn->intf = yypvt[-3].strings;
			    lhn->proto = yypvt[-1].strings;
			    lhn->metric = yypvt[-0].strings;
			    lhn->next = propagate;
			    propagate = lhn;
			} break;
case 111:
# line 464 "convert.y"
{ yyval.strings = yypvt[-0].strings; } break;
case 112:
# line 465 "convert.y"
{ yyval.strings = NULL; } break;
case 113:
# line 469 "convert.y"
{
				p[PR_EGP].defaultmetric = yypvt[-0].strings;
			} break;
case 114:
# line 475 "convert.y"
{
			    rt *dr=screate(rt);

			    dr->gw = yypvt[-3].strings;
			    dr->proto = yypvt[-2].strings;
			    dr->metric = yypvt[-1].strings;
			    dr->active = TRUE;
			    dr->next =defaultrt;
			    defaultrt = dr;
			} break;
case 115:
# line 486 "convert.y"
{
			    rt *dr=screate(rt);

			    dr->gw = yypvt[-3].strings;
			    dr->proto = yypvt[-2].strings;
			    dr->metric = yypvt[-1].strings;
			    dr->active = FALSE;
			    dr->next =defaultrt;
			    defaultrt = dr;
			} break;
case 116:
# line 499 "convert.y"
{yyval.strings = yypvt[-0].strings;} break;
case 117:
# line 500 "convert.y"
{ yyval.strings = 0;} break;
case 118:
# line 504 "convert.y"
{
			    messages |= WARN_EGPNETS;
			} break;
case 119:
# line 510 "convert.y"
{
			    if (martianlist)
				catstr(martianlist,yypvt[-0].strings);
			    else
				martianlist = yypvt[-0].strings;
			} break;
case 120:
# line 519 "convert.y"
{
				messages |= WARN_VALIDAS;
			} break;
case 121:
# line 525 "convert.y"
{
			    messages |= WARN_ANNCEAS;
			} break;
case 122:
# line 529 "convert.y"
{
			    messages |= WARN_ANNCEAS;
			} break;
case 123:
# line 535 "convert.y"
{
				messages |= WARN_ANNCEAS;
			    } break;
case 124:
# line 539 "convert.y"
{
				messages |= WARN_ANNCEAS;
			    } break;
case 125:
# line 545 "convert.y"
{
			    rt *sr = screate(rt);

			    sr->addr = yypvt[-5].strings;
			    sr->gw = yypvt[-3].strings;
			    sr->metric = yypvt[-1].strings;
			    sr->proto = yypvt[-0].strings;
			    sr->next = routes;
			    routes = sr;
			} break;
case 126:
# line 558 "convert.y"
{
			    rt *sr = screate(rt);

			    sr->addr = yypvt[-5].strings;
			    sr->gw = yypvt[-3].strings;
			    sr->metric = yypvt[-1].strings;
			    sr->proto = yypvt[-0].strings;
			    sr->next = routes;
			    routes = sr;
			} break;
case 127:
# line 570 "convert.y"
{yyval.strings=yypvt[-0].strings;} break;
case 128:
# line 571 "convert.y"
{yyval.strings=yypvt[-0].strings;} break;
case 129:
# line 574 "convert.y"
{ yyval.strings=yypvt[-0].strings; yypvt[-0].strings->next = yypvt[-1].strings;} break;
case 130:
# line 575 "convert.y"
{ yyval.strings = NULL; } break;
case 131:
# line 579 "convert.y"
{ yyval.strings = yypvt[-0].strings; } break;
case 132:
# line 580 "convert.y"
{ yyval.strings = yypvt[-0].strings; } break;
case 133:
# line 581 "convert.y"
{ yyval.strings = yypvt[-0].strings; } break;
case 134:
# line 584 "convert.y"
{yypvt[-1].strings->next = yypvt[-0].strings;  yyval.strings=yypvt[-1].strings;} break;
case 135:
# line 585 "convert.y"
{yyval.strings = yypvt[-0].strings; yypvt[-0].strings->next=NULL;} break;
case 136:
# line 588 "convert.y"
{ yyval.strings = yypvt[-0].strings; 
					yypvt[-0].strings->next = yypvt[-1].strings;
				      } break;
case 137:
# line 591 "convert.y"
{ yyval.strings = yypvt[-0].strings; } break;
case 138:
# line 594 "convert.y"
{yypvt[-1].strings->next = yypvt[-0].strings; yyval.strings=yypvt[-1].strings;} break;
case 139:
# line 595 "convert.y"
{yyval.strings = yypvt[-0].strings;} break;
case 140:
# line 598 "convert.y"
{yyval.strings=yypvt[-0].strings; yypvt[-0].strings->next = NULL;} break;
case 141:
# line 599 "convert.y"
{yyval.strings=yypvt[-0].strings; yypvt[-0].strings->next = NULL;} break;
case 142:
# line 600 "convert.y"
{ yyval.strings=yypvt[-0].strings; yypvt[-0].strings->next=NULL;} break;
	}
	goto yystack;		/* reset registers in driver code */
}

# ifdef __RUNTIME_YYMAXDEPTH

static int allocate_stacks() {
	/* allocate the yys and yyv stacks */
	yys = (int *) malloc(yymaxdepth * sizeof(int));
	yyv = (YYSTYPE *) malloc(yymaxdepth * sizeof(YYSTYPE));

	if (yys==0 || yyv==0) {
	   yyerror( (nl_msg(30004,"unable to allocate space for yacc stacks")) );
	   return(1);
	   }
	else return(0);

}


static void free_stacks() {
	if (yys!=0) free((char *) yys);
	if (yyv!=0) free((char *) yyv);
}

# endif  /* defined(__RUNTIME_YYMAXDEPTH) */

