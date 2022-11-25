# FacetTerm  mcg3101   (80 column only)  description file  08/28/90
## For the micro-colour graphics 3101 from micro vitec
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: mcg3101.FS,v 70.1 92/03/09 16:50:20 ssa Exp $
##--- MULTIPAGE ---##
pages=3
parm_display_page_next=\E[%p1%dU
parm_display_page_prev=\E[%p1%dV
## 
use=vt220mode7.fc
use=vt220narr.fc
use=vt220mode.fc
use=vt220com.fc
use=mvattr.fc
use=vt220func.fc
use=vt220keys.fc
pass-ansi_mode=\E%%!1
##				auto_color  on=\E[=3h  off=\E[=3l
perwindow-5-ORWLSQ=\E[=3l
perwindow_also-.=\E[=3h
perwindow_default-auto_color_off=\E[=3l
use=vt100.fm
