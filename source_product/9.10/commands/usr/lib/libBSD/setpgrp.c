/* HPUX_ID: @(#) $Revision: 63.1 $  */


/* setpgrp() - 4.2BSD compatible set process group [see BSDPROC(OS)] */

setpgrp(pid, pgrp)
int pid, pgrp;
{
    return setpgrp2(pid, pgrp);
}
