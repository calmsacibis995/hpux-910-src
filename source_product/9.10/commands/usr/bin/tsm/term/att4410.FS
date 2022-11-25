# FacetTerm  att4410  (80 column only)  description file 12/21/90
#    see also Att4410 for 80/132 columns.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: att4410.FS,v 70.1 92/03/09 16:45:41 ssa Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
### columns_wide_off=\E[?3l
pass=\E[?3l
parm_mode_val-NOTIMP..-PASS....=?3
### NOT_IN_80 columns_wide_clears_screen
lines=24
use=att4410com.fc
use=att605.fm
