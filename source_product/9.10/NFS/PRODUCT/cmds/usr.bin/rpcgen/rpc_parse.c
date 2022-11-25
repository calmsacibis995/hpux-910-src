/* 
 @(#)rpc_parse:	$Revision: 1.12.109.1 $	$Date: 91/11/19 14:09:35 $  
*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * rpc_parse.c, Parser for the RPC protocol compiler 
 * Copyright (C) 1987 Sun Microsystems, Inc.
 */
#include <stdio.h>
#include "rpc_util.h"
#include "rpc_scan.h"
#include "rpc_parse.h"

/* NOTE: rpc_main.c, rpc_parse.c, rpc_scan.c and rpc_util.c share a 	*/
/* single message catalog (rpgen.cat).  For that reason we have 	*/
/* allocated messages 1 through 20 for rpc_main.c, 21 through 40 for  	*/
/* rpc_parse.c, 41 through 60 to rpc_scan.c and from 61 on for		*/
/* rpc_util.c.  If we need more than 20 messages in this file we will 	*/
/* need to take into account the message numbers that are already 	*/
/* used by the other files.						*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#endif NLS

extern char tempbuf[];

/*
 * return the next definition you see
 */
definition *
get_definition()
{
	definition *defp;
	token tok;

	defp = ALLOC(definition);
	get_token(&tok);
	switch (tok.kind) {
	case TOK_STRUCT:
		def_struct(defp);
		break;
	case TOK_UNION:
		def_union(defp);
		break;
	case TOK_TYPEDEF:
		def_typedef(defp);
		break;
	case TOK_ENUM:
		def_enum(defp);
		break;
	case TOK_PROGRAM:
		def_program(defp);
		break;
	case TOK_CONST:
		def_const(defp);
		break;
	case TOK_EOF:
		return (NULL);
		break;
	default:
		strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,21, "definition keyword expected")));
		error(tempbuf);
	}
	scan(TOK_SEMICOLON, &tok);
	isdefined(defp);
	return (defp);
}

static
isdefined(defp)
	definition *defp;
{
	STOREVAL(&defined, defp);
}


static
def_struct(defp)
	definition *defp;
{
	token tok;
	declaration dec;
	decl_list *decls;
	decl_list **tailp;

	defp->def_kind = DEF_STRUCT;

	scan(TOK_IDENT, &tok);
	defp->def_name = tok.str;
	scan(TOK_LBRACE, &tok);
	tailp = &defp->def.st.decls;
	do {
		get_declaration(&dec, DEF_STRUCT);
		decls = ALLOC(decl_list);
		decls->decl = dec;
		*tailp = decls;
		tailp = &decls->next;
		scan(TOK_SEMICOLON, &tok);
		peek(&tok);
	} while (tok.kind != TOK_RBRACE);
	get_token(&tok);
	*tailp = NULL;
}

static
def_program(defp)
	definition *defp;
{
	token tok;
	version_list *vlist;
	version_list **vtailp;
	proc_list *plist;
	proc_list **ptailp;

	defp->def_kind = DEF_PROGRAM;
	scan(TOK_IDENT, &tok);
	defp->def_name = tok.str;
	scan(TOK_LBRACE, &tok);
	vtailp = &defp->def.pr.versions;
	scan(TOK_VERSION, &tok);
	do {
		scan(TOK_IDENT, &tok);
		vlist = ALLOC(version_list);
		vlist->vers_name = tok.str;
		scan(TOK_LBRACE, &tok);
		ptailp = &vlist->procs;
		do {
			plist = ALLOC(proc_list);
			get_type(&plist->res_prefix, &plist->res_type, DEF_PROGRAM);
			if (streq(plist->res_type, "opaque")) {
				strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,22, "illegal result type")));
				error(tempbuf);
			}
			scan(TOK_IDENT, &tok);
			plist->proc_name = tok.str;
			scan(TOK_LPAREN, &tok);
			get_type(&plist->arg_prefix, &plist->arg_type, DEF_PROGRAM);
			if (streq(plist->arg_type, "opaque")) {
				strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,23, "illegal argument type")));
				error(tempbuf);
			}
			scan(TOK_RPAREN, &tok);
			scan(TOK_EQUAL, &tok);
			scan_num(&tok);
			scan(TOK_SEMICOLON, &tok);
			plist->proc_num = tok.str;
			*ptailp = plist;
			ptailp = &plist->next;
			peek(&tok);
		} while (tok.kind != TOK_RBRACE);
		/*
		 * HPNFS
		 * A fix here because Sun did not finish off with a NULL
		 * at the end of their program list.  This caused rpcgen
		 * to sometimes get a segmentation violation.
		 */
		*ptailp = NULL;
		*vtailp = vlist;
		vtailp = &vlist->next;
		scan(TOK_RBRACE, &tok);
		scan(TOK_EQUAL, &tok);
		scan_num(&tok);
		vlist->vers_num = tok.str;
		scan(TOK_SEMICOLON, &tok);
		scan2(TOK_VERSION, TOK_RBRACE, &tok);
	} while (tok.kind == TOK_VERSION);
	scan(TOK_EQUAL, &tok);
	scan_num(&tok);
	defp->def.pr.prog_num = tok.str;
	*vtailp = NULL;
}

static
def_enum(defp)
	definition *defp;
{
	token tok;
	enumval_list *elist;
	enumval_list **tailp;

	defp->def_kind = DEF_ENUM;
	scan(TOK_IDENT, &tok);
	defp->def_name = tok.str;
	scan(TOK_LBRACE, &tok);
	tailp = &defp->def.en.vals;
	do {
		scan(TOK_IDENT, &tok);
		elist = ALLOC(enumval_list);
		elist->name = tok.str;
		elist->assignment = NULL;
		scan3(TOK_COMMA, TOK_RBRACE, TOK_EQUAL, &tok);
		if (tok.kind == TOK_EQUAL) {
			scan_num(&tok);
			elist->assignment = tok.str;
			scan2(TOK_COMMA, TOK_RBRACE, &tok);
		}
		*tailp = elist;
		tailp = &elist->next;
	} while (tok.kind != TOK_RBRACE);
	*tailp = NULL;
}

static
def_const(defp)
	definition *defp;
{
	token tok;

	defp->def_kind = DEF_CONST;
	scan(TOK_IDENT, &tok);
	defp->def_name = tok.str;
	scan(TOK_EQUAL, &tok);
	scan2(TOK_IDENT, TOK_STRCONST, &tok);
	defp->def.co = tok.str;
}

static
def_union(defp)
	definition *defp;
{
	token tok;
	declaration dec;
	case_list *cases;
	case_list **tailp;

	defp->def_kind = DEF_UNION;
	scan(TOK_IDENT, &tok);
	defp->def_name = tok.str;
	scan(TOK_SWITCH, &tok);
	scan(TOK_LPAREN, &tok);
	get_declaration(&dec, DEF_UNION);
	/*
	 * HPNFS
	 * This change will prevent anything but a simple variable to be
	 * the switching variable in a union definition.  Before, an
	 * array could have been used but it makes no sense semantically.
	 */
	if (dec.rel != REL_ALIAS) {
		strcpy(tempbuf, (catgets(nlmsg_fd,NL_SETN,30,"only simple declaration allowed in switch")));
		error(tempbuf);
	}
	defp->def.un.enum_decl = dec;
	tailp = &defp->def.un.cases;
	scan(TOK_RPAREN, &tok);
	scan(TOK_LBRACE, &tok);
	scan(TOK_CASE, &tok);
	while (tok.kind == TOK_CASE) {
		/* scan(TOK_IDENT, &tok);  Gone to add case 'a': */
		scan2(TOK_IDENT, TOK_CHARCONST, &tok);
		cases = ALLOC(case_list);
		cases->case_name = tok.str;
		scan(TOK_COLON, &tok);
		get_declaration(&dec, DEF_UNION);
		cases->case_decl = dec;
		*tailp = cases;
		tailp = &cases->next;
		scan(TOK_SEMICOLON, &tok);
		scan3(TOK_CASE, TOK_DEFAULT, TOK_RBRACE, &tok);
	}
	*tailp = NULL;
	if (tok.kind == TOK_DEFAULT) {
		scan(TOK_COLON, &tok);
		get_declaration(&dec, DEF_UNION);
		defp->def.un.default_decl = ALLOC(declaration);
		*defp->def.un.default_decl = dec;
		scan(TOK_SEMICOLON, &tok);
		scan(TOK_RBRACE, &tok);
	} else {
		defp->def.un.default_decl = NULL;
	}
}


static
def_typedef(defp)
	definition *defp;
{
	declaration dec;

	defp->def_kind = DEF_TYPEDEF;
	get_declaration(&dec, DEF_TYPEDEF);
	defp->def_name = dec.name;
	defp->def.ty.old_prefix = dec.prefix;
	defp->def.ty.old_type = dec.type;
	defp->def.ty.rel = dec.rel;
	defp->def.ty.array_max = dec.array_max;
}


static
get_declaration(dec, dkind)
	declaration *dec;
	defkind dkind;
{
	token tok;

	get_type(&dec->prefix, &dec->type, dkind);
	dec->rel = REL_ALIAS;
	if (streq(dec->type, "void")) {
		return;
	}
	scan2(TOK_STAR, TOK_IDENT, &tok);
	if (tok.kind == TOK_STAR) {
		dec->rel = REL_POINTER;
		scan(TOK_IDENT, &tok);
	}
	dec->name = tok.str;
	if (peekscan(TOK_LBRACKET, &tok)) {
		if (dec->rel == REL_POINTER) {
			strcpy(tempbuf, (catgets(nlmsg_fd,NL_SETN,24, "no array-of-pointer declarations -- use typedef")));
			error(tempbuf);
		}
		dec->rel = REL_VECTOR;
		scan_num(&tok);
		dec->array_max = tok.str;
		scan(TOK_RBRACKET, &tok);
	} else if (peekscan(TOK_LANGLE, &tok)) {
		if (dec->rel == REL_POINTER) {
			strcpy(tempbuf, (catgets(nlmsg_fd,NL_SETN,25, "no array-of-pointer declarations -- use typedef")));
			error(tempbuf);
		}
		dec->rel = REL_ARRAY;
		if (peekscan(TOK_RANGLE, &tok)) {
			dec->array_max = "~0";	/* unspecified size, use max */
		} else {
			scan_num(&tok);
			dec->array_max = tok.str;
			scan(TOK_RANGLE, &tok);
		}
	}
	if (streq(dec->type, "opaque")) {
		if (dec->rel != REL_ARRAY && dec->rel != REL_VECTOR) {
			strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,26, "array declaration expected")));
			error(tempbuf);
		}
	} else if (streq(dec->type, "string")) {
		if (dec->rel != REL_ARRAY) {
			strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,27, "variable-length array declaration expected")));
			error(tempbuf);
		}
	}
}


static
get_type(prefixp, typep, dkind)
	char **prefixp;
	char **typep;
	defkind dkind;
{
	token tok;

	*prefixp = NULL;
	get_token(&tok);
	switch (tok.kind) {
	case TOK_IDENT:
		*typep = tok.str;
		break;
	case TOK_STRUCT:
	case TOK_ENUM:
	case TOK_UNION:
		*prefixp = tok.str;
		scan(TOK_IDENT, &tok);
		*typep = tok.str;
		break;
	case TOK_UNSIGNED:
		unsigned_dec(typep);
		break;
	case TOK_SHORT:
		*typep = "short";
		(void) peekscan(TOK_INT, &tok);
		break;
	case TOK_LONG:
		*typep = "long";
		(void) peekscan(TOK_INT, &tok);
		break;
	case TOK_VOID:
		if (dkind != DEF_UNION && dkind != DEF_PROGRAM) {
			strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,28, "voids allowed only inside union and program definitions")));
			error(tempbuf);
		}
		*typep = tok.str;
		break;
	case TOK_STRING:
	case TOK_OPAQUE:
	case TOK_CHAR:
	case TOK_INT:
	case TOK_FLOAT:
	case TOK_DOUBLE:
	case TOK_BOOL:
		*typep = tok.str;
		break;
	default:
		strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,29, "expected type specifier")));
		error(tempbuf);
	}
}


static
unsigned_dec(typep)
	char **typep;
{
	token tok;

	peek(&tok);
	switch (tok.kind) {
	case TOK_CHAR:
		get_token(&tok);
		*typep = "u_char";
		break;
	case TOK_SHORT:
		get_token(&tok);
		*typep = "u_short";
		(void) peekscan(TOK_INT, &tok);
		break;
	case TOK_LONG:
		get_token(&tok);
		*typep = "u_long";
		(void) peekscan(TOK_INT, &tok);
		break;
	case TOK_INT:
		get_token(&tok);
		*typep = "u_int";
		break;
	default:
		*typep = "u_int";
		break;
	}
}
