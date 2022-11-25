## FacetTerm  m3220  (80 column only)  description file  08/29/90
## Description for micro-vitec 3220 terminal.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: m3220.FS,v 70.1 92/03/09 16:10:39 ssa Exp $
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
use=vt100c.fm
