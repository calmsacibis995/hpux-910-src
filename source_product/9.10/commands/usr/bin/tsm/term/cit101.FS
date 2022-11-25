# FacetTerm  cit101  (80 column only)  description file 05/18/90
#    CIT 50+ in ANSI mode
#    See also Cit101 for 80/132 columns.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF MODE
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: cit101.FS,v 70.1 92/03/09 15:57:11 ssa Exp $
use=vt100ans.fc
enter_ca_mode=\E[>9h
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
### NOT_IN_80 columns_wide_off=\E[?3l
pass=\E[?3l
parm_mode_val-NOTIMP..-PASS....=?3
### NOT_IN_80 columns_wide_clears_screen
lines=24
use=vt100com.fc
use=vt220func.fc
use=cit101keys.fc
use=cit101com.fc
use=vt100.fm
