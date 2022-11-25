# FACET/TERM  wyse75  (80 column only)  description file 08/27/90
#    See also Wyse75 for 80/132 column.
#
#     !!! IMPORTANT NOTE !!!
#     Set Terminal to: XON/XOFF  ANSI
#
## @(#) $Header: wyse75.FS,v 66.2 90/09/20 12:57:21 kb Exp $
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
### NOT_IN_80 columns_wide_off=\E[?3l
pass=\E[?3l
parm_mode_val-NOTIMP..-PASS....=?3
### NOT_IN_80 NO columns_wide_clears_screen
use=wyse75com.fc
