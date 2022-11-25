# FacetTerm  Att605  (80/132 column w/o simultask)  description file 12/21/90
#    See also att605nopc for 80 column only.
#    See also att605pc   for 80 column only with simultask.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Att605.FS,v 70.1 92/03/09 16:47:12 ssa Exp $
xon_xoff
delete_is_a_character
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
columns_wide_clears_screen
lines=24
### NO enter_ca_mode=
### NO exit_ca_mode=
use=att605com.fc
use=att605colr.fc
use=att605.fm
