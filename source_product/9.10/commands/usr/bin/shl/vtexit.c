/* @(#) $Revision: 29.1 $ */   
/* default vtexit() and vtquit() routines. vtexit() and vtquit()     */
/* are called from routines in vtproto.c. If some sort               */
/* of cleanup is needed, then a different vtexit() or vtquit()       */
/* should be written that the loader would find before this version. */

void
vtquit()
{
    exit(0);
}

void
vtexit()
{
    exit(0);
}
