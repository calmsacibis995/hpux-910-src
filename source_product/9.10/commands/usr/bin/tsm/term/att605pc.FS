# FacetTerm  att605pc (80 column with simultask) description file 12/21/90
#    see also att605nopc for 80 column only.
#    see also att605     for 80/132 column.
#
#     !!! IMPORTANT NOTE !!!
#     Set Terminal to: no parity
#     Set Line to:     8 bit
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: att605pc.FS,v 70.1 92/03/09 16:45:17 ssa Exp $
xon_xoff
graphics_8_bit
graphics_escape_control
delete_is_a_character
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
columns_wide_clears_screen
lines=24
### NO enter_ca_mode=
### NO exit_ca_mode=
use=att605pccm.fc
use=att605com.fc
use=att605colr.fc
use=att605.fm
