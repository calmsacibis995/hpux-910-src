/* HPUX_ID: @(#) $Revision: 49.1 $  */


/* getpgrp() - 4.2BSD compatible get process group [see BSDPROC(OS)] */

getpgrp(pid)
int pid;
{
    return getpgrp2(pid);
}
