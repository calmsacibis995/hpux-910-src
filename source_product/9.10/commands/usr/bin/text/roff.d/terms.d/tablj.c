/* @(#) $Revision: 38.2 $ */    
#define INCH 240
/*
HP 2686 Laser Printer (Laserjet)
nroff driving tables
width and code tables
*/

struct termtable tlj = {
/*bset*/	0,
/*breset*/	0,
/*Hor*/		INCH/10,
/*Vert*/        INCH/12,
/*Newline*/	INCH/6,
/*Char*/	INCH/10,
#ifdef NLS16
/*Kchar*/	2*INCH/10,
#endif
/*Em*/		INCH/10,
/*Halfline*/	INCH/12,
/*Adj*/		INCH/10,
/*twinit*/      "\033&l6d0e66F\033&a1l0M\033&k12H",
/*twrest*/	"",
/*twnl*/        "\n\017",
/*hlr*/         "\033&a-60V",
/*hlf*/         "\033=",
/*flr*/         "\033&a-120V",
/*bdon*/        "\033)s0s3B\016",
/*bdoff*/       "\017",
/*iton*/        "\033)s1S\016",
/*itoff*/       "\017",
/*ploton*/	"",
/*plotoff*/	"",
/*up*/		"",
/*down*/	"",
/*right*/	"",
/*left*/	"",
/*codetab*/
#include "code.lp"
