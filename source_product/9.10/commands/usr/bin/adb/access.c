/* @(#) $Revision: 66.1 $ */      

#include "defs.h"

MSG		ODDADR;
MSG		BADDAT;
MSG		BADTXT;
MSG		EOUTADD;
MSG		ECORADD;
MSG		ESUBADD;
MAP		txtmap;
MAP		datmap;
int		wtflag;
STRING		errflg;
int		errno;
int		pid;
POS		accessf();
char		coremapped;
COREMAP		*cmaps;

/* file handling and access routines */

put(adr,space,value)
long	adr;
{
	accessf(PT_WIUSER,adr,space,value);
}

POS	get(adr, space)
long	adr;
{
	register unsigned data = accessf(PT_RIUSER,adr,space,0);
	return((data>>16)&0xFFFF);
}

getword(adr, space)
{
	register unsigned data = accessf(PT_RIUSER,adr,space,0);
	return data;
}

POS	chkget(n, space)
long	n;
{
	register int		w;

	w = get(n, space);
	chkerr();
	return(w);
}

POS	accessf(mode,adr,space,value)
long	adr;
register int mode;
{
	register int w1, file;
	int w;

	if (space == NSP) return(0);

	if (pid)	/* tracing on? */
	{    if ((adr&01) && (mode == PT_WIUSER)) error(ODDADR);
	     if (mode == PT_WIUSER)
	     {
		w1 = ptrace(PT_RIUSER, pid, adr, 0);
		value = (w1&0xFFFF) | (value&0xFFFF0000);
	     }
	     w = ptrace(mode, pid, (adr&~01), value);
	     if (adr&01)
	     {    w1 = ptrace(mode, pid, (adr+1), value);
		  w = ((w<<8)&0xFFFF0000) | ((w1>>8)&0xFFFF);
	     }
	     if (errno) errflg = ESUBADD;
	     return(w);
	}
	w = 0;
	if (mode==PT_WIUSER && wtflag==0) error("not in write mode");
	if (!chkmap(&adr,space)) return(0);
	file=(space&DSP?datmap.ufd:txtmap.ufd);
	if ( longseek(file,adr)==0 ||
	     (mode == PT_RIUSER ? read(file,&w,4) : write(file,&value,2))
	     < 1)
	{     errflg=(space&DSP?ECORADD:EOUTADD);
	}
	return(w);

}

chkmap(adr,space)
register long	*adr;
register int		space;
{   if ((space&DSP) && coremapped)
    { COREMAP *thismap;
	for (thismap = cmaps; thismap; thismap = thismap->next_map)
	{   if ((!(space&STAR)) || (thismap->type == CORE_STACK))
		if (within (*adr, thismap->b, thismap->e))
		{   *adr += thismap->f - thismap->b;
		    return(1);
		}
	}
	errflg = ECORADD;
	return(0);
    }
    else
    { register MAPPTR amap;
	amap =(space&DSP ? &datmap : &txtmap);
    	if (space&STAR || !within(*adr,amap->b1,amap->e1))
	{   if (within (*adr,amap->b2,amap->e2))
	       	*adr += (amap->f2)-(amap->b2); 
	    else
	    { 	errflg = (space&DSP ? ECORADD : EOUTADD);
	       	return(0);
	    }
	} else  *adr += (amap->f1)-(amap->b1);
	return(1);
    }
}

within(adr,lbd,ubd)
long	adr, lbd, ubd;
{
	return(adr>=lbd && adr<ubd);
}
