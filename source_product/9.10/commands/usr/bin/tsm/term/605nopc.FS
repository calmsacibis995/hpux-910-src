# FACET/TERM  605nopc (80 column without simultask) description file 04/18/89
#    see also 605pc for with simultask
#    see also 605Wnopc for 80/132 column.
## @(#) $Header: 605nopc.FS,v 66.2 90/09/20 12:28:07 kb Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
columns_wide_clears_screen
lines=24
### NO enter_ca_mode=
### NO exit_ca_mode=
use=605com.fc
