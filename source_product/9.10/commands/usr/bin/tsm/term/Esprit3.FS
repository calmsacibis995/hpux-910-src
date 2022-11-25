# FacetTerm Esprit3 (80/132 column)  description file 08/30/91
#  Esprit III Color VDT - Tested on PCVision Emulator
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Esprit3.FS,v 70.1 92/03/09 16:55:01 ssa Exp $
xon_xoff
### graphics_8_bit
graphics_8_bit
### graphics_escape_control
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
### columns_wide_clears_screen
lines=24
##--- INIT ---##
### xon_xoff 
enter_ca_mode=^O
use=esprit3com.fc
use=tvi900keys.fc
use=esprit3.fm
