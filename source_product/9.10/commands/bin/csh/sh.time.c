/* @(#) $Revision: 70.1 $ */      
/**************************************************************
 * C Shell - routines handling process timing and niceing
 **************************************************************/

#include "sh.h"

#ifdef VMUNIX
struct	vtimes vm0;
#else
struct	tms times0;
#endif

settimes()
{

	time(&time0);
#ifdef VMUNIX
	vtimes(&vm0, 0);
#else
	times(&times0);
#endif
}

/*
 * dotime is only called if it is truly a builtin function and not a
 * prefix to another command
 */
dotime()
{
	time_t timedol;


#ifdef VMUNIX
	struct vtimes vm1, vmch;

	vtimes(&vm1, &vmch);
	vmsadd(&vm1, &vmch);
#else
	struct  tms timesdol;
	time_t  user_time,system_time;

	times(&timesdol);
	user_time   =   (timesdol.tms_utime + timesdol.tms_cutime)
		      - (times0.tms_utime + times0.tms_cutime);
	system_time =   (timesdol.tms_stime + timesdol.tms_cstime)
		      - (times0.tms_stime + times0.tms_cstime);
#endif
	time(&timedol);
#ifdef VMUNIX
	pvtimes(&vm0, &vm1, timedol - time0);
#else
	ptimes(user_time, system_time, timedol - time0);
#endif
}

/*
 * donice is only called when it on the line by itself or with a +- value
 */
donice(v)
	register CHAR **v;
{
	register CHAR *cp;

	v++, cp = *v++;
	if (cp == 0) {
#ifndef V6
/*
   Don't know what the original intent was, but this doesn't work
   on HP-UX
		nice(20);
		nice(-10);
 */
#endif
		nice(4);
	} else if (*v == 0 && any(cp[0], "+-")) {
#ifndef V6
/*
		nice(20);
		nice(-10);
*/
#endif
		nice(getn(cp));
	}
}

#ifndef VMUNIX
ptimes(utime, stime, etime)
	register time_t utime, stime, etime;
{

	p60ths(utime);
	printf("u ");
	p60ths(stime);
	printf("s ");
	psecs(etime);
	printf(" %d%%\n", (int) (100 * (utime+stime) /
		(HZ * (etime ? etime : 1))));
}

#else
vmsadd(vp, wp)
	register struct vtimes *vp, *wp;
{

	vp->vm_utime += wp->vm_utime;
	vp->vm_stime += wp->vm_stime;
	vp->vm_nswap += wp->vm_nswap;
	vp->vm_idsrss += wp->vm_idsrss;
	vp->vm_ixrss += wp->vm_ixrss;
	if (vp->vm_maxrss < wp->vm_maxrss)
		vp->vm_maxrss = wp->vm_maxrss;
	vp->vm_majflt += wp->vm_majflt;
	vp->vm_minflt += wp->vm_minflt;
	vp->vm_inblk += wp->vm_inblk;
	vp->vm_oublk += wp->vm_oublk;
}

pvtimes(v0, v1, sec)
	register struct vtimes *v0, *v1;
	time_t sec;
{
	register time_t t =
	    (v1->vm_utime-v0->vm_utime)+(v1->vm_stime-v0->vm_stime);
	register char *cp;
	register int i;
	register struct varent *vp = adrof("time");

	cp = "%Uu %Ss %E %P %X+%Dk %I+%Oio %Fpf+%Ww";
	if (vp && vp->vec[0] && vp->vec[1])
		cp = vp->vec[1];
	for (; *cp; cp++)
	if (*cp != '%')
		putchar(*cp);
	else if (cp[1]) switch(*++cp) {

	case 'U':
		p60ths(v1->vm_utime - v0->vm_utime);
		break;

	case 'S':
		p60ths(v1->vm_stime - v0->vm_stime);
		break;

	case 'E':
		psecs(sec);
		break;

	case 'P':
		printf("%d%%", (int) ((100 * t) / (60 * (sec ? sec : 1))));
		break;

	case 'W':
		i = v1->vm_nswap - v0->vm_nswap;
		printf("%d", i);
		break;

	case 'X':
		printf("%d", t == 0 ? 0 : (v1->vm_ixrss-v0->vm_ixrss)/(2*t));
		break;

	case 'D':
		printf("%d", t == 0 ? 0 : (v1->vm_idsrss-v0->vm_idsrss)/(2*t));
		break;

	case 'K':
		printf("%d", t == 0 ? 0 : ((v1->vm_ixrss+v1->vm_idsrss) -
		   (v0->vm_ixrss+v0->vm_idsrss))/(2*t));
		break;

	case 'M':
		printf("%d", v1->vm_maxrss/2);
		break;

	case 'F':
		printf("%d", v1->vm_majflt-v0->vm_majflt);
		break;

	case 'R':
		printf("%d", v1->vm_minflt-v0->vm_minflt);
		break;

	case 'I':
		printf("%d", v1->vm_inblk-v0->vm_inblk);
		break;

	case 'O':
		printf("%d", v1->vm_oublk-v0->vm_oublk);
		break;

	}
	putchar('\n');
}
#endif
