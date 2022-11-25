/* HPUX_ID: @(#) $Revision: 49.1 $  */


/* killpg() - 4.2BSD compatible kill process group [see BSDPROC(OS)] */

killpg(pgrp, sig)
int pgrp, sig;
{
    return kill(-pgrp, sig);
}
