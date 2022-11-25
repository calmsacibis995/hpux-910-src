# FACET/TERM  605W  (80/132 column w/o simultask)  description file 04/18/89
#    See also 605nopc for 80 column only.
#    See also 605pc for 80 column only with simultask.
## @(#) $Header: 605W.FS,v 66.2 90/09/20 12:27:45 kb Exp $
xon_xoff
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
columns_wide_clears_screen
lines=24
### NO enter_ca_mode=
### NO exit_ca_mode=
use=605com.fc
