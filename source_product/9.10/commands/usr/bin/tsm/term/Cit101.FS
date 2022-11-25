# FacetTerm  Cit101  (80/132 column)  description file 05/18/90
#    CIT 50+ in ANSI mode
#    See also cit101 for 80 column only.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF MODE
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Cit101.FS,v 70.1 92/03/09 15:52:43 ssa Exp $
use=vt100ans.fc
## screen_width_change_preserve_mode=\E[>9h
enter_ca_mode=\E[>9h
xon_xoff
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
### columns_wide_clears_screen
lines=24
use=vt100com.fc
use=vt220func.fc
use=cit101keys.fc
use=cit101com.fc
use=vt100.fm
