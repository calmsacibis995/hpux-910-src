# FacetTerm  Em4010   (80/132 column)  description file  09/10/90
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Em4010.FS,v 70.1 92/03/09 16:45:59 ssa Exp $
xon_xoff
pass=\E F
##--- MULTIPAGE ---##
# DCS emulator switchable 80/132 columns
enter_ca_mode-80_columns..=\E[?3l
## 
###############################################
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
### parm_mode_val-NOTIMP..-IGNORE..=?3
parm_mode_val-COLSWIDE-00000000=?3
### columns_wide_clears_screen
lines=24
###############################################
use=em4010ans.fc
use=vt220com.fc
use=vt220attr.fc
use=vt220func.fc
use=em4010keys.fc
use=em4010stat.fc
use=vt100.fm
use=em4010grap.fc
