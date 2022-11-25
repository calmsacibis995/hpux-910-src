# FacetTerm  att605nopc (80 column without simultask) description file 12/21/90
#    see also att605pc for with simultask
#    see also Att605   for 80/132 column.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: att605nopc.FS,v 70.1 92/03/09 16:45:13 ssa Exp $
xon_xoff
columns=80
delete_is_a_character
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
columns_wide_clears_screen
lines=24
### NO enter_ca_mode=
### NO exit_ca_mode=
use=att605com.fc
use=att605colr.fc
use=att605.fm
