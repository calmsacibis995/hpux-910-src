# FACET/TERM  605pc (80 column with simultask) description file 07/27/89
#    see also 605nopc for 80 column only.
#    see also 605Wnopc for 80/132 column.
#
#     !!! IMPORTANT NOTE !!!
#     Set Terminal to: no parity
#
## @(#) $Header: 605pc.FS,v 66.2 90/09/20 12:28:13 kb Exp $
xon_xoff
graphics_8_bit
graphics_escape_control
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
columns_wide_clears_screen
lines=24
### NO enter_ca_mode=
### NO exit_ca_mode=
use=605pccom.fc
use=605com.fc
