# FacetTerm  m3320   (80 column only)  description file  05/13/91
## For the micro-colour graphics 3320 from micro vitec
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: m3320.FS,v 70.1 92/03/09 16:11:40 ssa Exp $
set-red_screen=\E[31m
##--- MULTIPAGE ---##
pages=10
parm_display_page_next=\E[%p1%dU
parm_display_page_prev=\E[%p1%dV
## 
use=vt220mode7.fc
use=vt220narr.fc
use=vt220mode.fc
use=vt220com.fc
use=m3320attr.fc
use=vt220func.fc
use=vt220keys.fc
pass-ansi_mode=\E%%!1
##				auto_color  on=\E[=3h  off=\E[=3l
perwindow-5-ORWLSQ=\E[=3l
perwindow_also-.=\E[=3h
perwindow_default-auto_color_off=\E[=3l
display_page_number=\E[%p1%{1}%+%d\040P
enter_ca_mode=\E[=3;24;80t\E[=4;24;80t\E[=5;24;80t\E[=6;24;80t\E[=7;24;80t\E[=8;24;80t\E[=9;24;80t\E[=10;24;80t\E[=11;24;80t\E[=2;24;80t\E[1\040P
use=vt100c.fm
