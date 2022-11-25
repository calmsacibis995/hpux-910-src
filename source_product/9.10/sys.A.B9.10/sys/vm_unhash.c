/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_unhash.c,v $
 * $Revision: 1.3.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/18 13:46:41 $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/pfdat.h"
#include "../h/kernel.h"

/* parameters controlling the unhash daemon */
int maxunhash = 5;
int unhash_frequency = 1;

int unhash;  /* variable whose address the unhash daemon sleeps on */

/*
 * schedule unhash daemon
 */
schedunhash()
{

        /* wake up unhash daemon */
        wakeup((caddr_t)&unhash);
        timeout(schedunhash, (caddr_t) 0, hz / unhash_frequency);
}


/*
 * Daemon which unhashes pages on the free list.  It allows
 * the interrupt stack to get pages without having to call
 * vn_rele which could go to sleep.
 */
unhashdaemon()
{

        while (1) {
                sleep(&unhash, PSWP);
                unhashpages(maxunhash);
        }
}

/*
 * Attempt to ensure that some number of pages at the beginning
 * of the free list are unhashed.  This allows the interrupt stack
 * to get pages without having to call vn_rele (note: vn_rele
 * can not be called on the interrupt stack.
 */
unhashpages(unhash)
        int unhash;
{
        register pfd_t *pfd;
        int found;

        /*
         * Search the free list looking for unhashable pages.  The
         * free list is ordered so that unhashed pages always appear
         * at the beginning of the list and hashed pages are always
         * at the end.
         *
         * Note that after unhashing a page, we need to start over
         * at the beginning of the list and found starts at zero so
         * we avoid counting unhashed pages twice.
         */
retry:
        found = 0;
        pfdatlstlock();
        for (pfd = phead.pf_next; pfd != &phead; pfd = pfd->pf_next) {
                if ((found >= unhash) || (!cpfdatlock(pfd)))
                        break;
                else {
                        /*
                         * Now if the page is hashed we unhash it.
                         * If the page is not hashed, we keep looking
                         * until unhash number of pages are recorded
                         * as unhashed.
                         */
                        found++;
                        if (PAGEINHASH(pfd)) {
                                pfdatlstunlock();
                                pageremove(pfd);
                                pfdatunlock(pfd);
                                goto retry;
                        }
                        pfdatunlock(pfd);
                }
        }
        pfdatlstunlock();
}


#ifdef NEVER_CALLED
/*
 * get parameters of unhash daemon
 */
get_unhash_params(unhashpages, frequency)
	int *unhashpages;
	int *frequency;
{
        *unhashpages = maxunhash;
        *frequency = unhash_frequency;
}



/*
 * set parameters of unhash daemon
 */
int
set_unhash_params(unhashpages, frequency)
	int unhashpages;
	int frequency;
{
        if (unhashpages <= 0 || frequency <= 0)
                return(-1);

        if (frequency > hz)
                frequency = hz;

        maxunhash = unhashpages;
        unhash_frequency = frequency;

        return(0);
}
#endif /* NEVER_CALLED */


