# FacetTerm  Dt100  (80/132 col)  description file 08/27/90
#    See also dt100 for 80 column only.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF  ANSI
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Dt100.FS,v 70.1 92/03/09 15:52:48 ssa Exp $
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
### NO columns_wide_clears_screen
use=wy75com.fc
use=wy75.fm
