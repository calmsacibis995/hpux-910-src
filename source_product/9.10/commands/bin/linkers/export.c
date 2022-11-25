#ifdef DLD
#include "dld.h"
#define CONST const
int hash (CONST char *name);
#else
#include "ld.defs.h"
#define CONST
#endif


ExpLookup(namep, highwater, sympp, stringt, expt, hasht, hashsize)
CONST char                 *namep;
int                         highwater;
CONST struct export_entry **sympp;
CONST char                 *stringt;
CONST struct export_entry  *expt;
CONST struct hash_entry    *hasht;
int                         hashsize;
{
   int                                 expi;
   int                                 hashval;
   register int                        expname = -1;
   register CONST struct export_entry *expp;
   int                                 namecmp;

   *sympp = NULL;

#ifdef DLD
   hashval = dld_last_hash % hashsize;
#if DEBUG & 2
   printf("dld_last_hash = %d; hashsize = %d; hashval = %d\n",dld_last_hash,hashsize,hashval);
#endif
#else
   hashval = hash(namep, hashsize);
#endif

   for (expi = hasht[hashval].symbol; expi != -1; expi = expp->next_export) {

      expp = expt + expi;

      if (expp->name == expname) {
         continue;
      }

      expname = expp->name;

      if ((namecmp = strcmp(namep, stringt + expname)) < 0)  {
         return(0);
      }

      if (namecmp > 0)  {
         continue;
      }

      if (highwater == -1) {
         *sympp = expp;
#ifdef DLD
         g_eindex = expi;
#endif
         return(1);
      }
      else
#pragma BBA_IGNORE
         ;

      while (expp->name == expname && expp->highwater > highwater) {

         expi = expp->next_export;

         if (expi == -1) {
            return(0);
         }

         expp = expt + expi;
      }

      if (expp->name != expname) {
         return(0);
      }

      *sympp = expp;
#ifdef DLD
      g_eindex = expi;
#endif
      return(1);
   }
   return(0);
}


#ifndef DLD
/* The following value is used internally by ExpEnter and ExpReloc.  It should
 * be some bit in the regular symbol table types which is not used, and since
 * the type in an export list is a short, this is easy to accomplish.
 */
#define  EXPORT_PLT    0x4000

ExpEnter(sp, seg, val, highwater, size, 
		 next_symbol, mod_imports, next_module)
struct symbol *sp;
int    seg, highwater;
long   val, size, next_symbol, mod_imports, next_module;
{
   
   register struct export_entry *expp;

   if (expindex == ld_expsize) {
      ld_expsize *= 2;
      exports = (struct export_entry *) 
                realloc(exports, ld_expsize * sizeof(struct export_entry));
	  shl_exports = (struct shl_export_entry *)
           realloc( shl_exports, ld_expsize * sizeof(struct shl_export_entry));
   }

   expp = exports + expindex;

   enter_string_table( sp );
   expp->name = sp->sname - stringt;

   expp->type = seg | (sp->s.n_flags & NLIST_EXPORT_PLT_FLAG ? EXPORT_PLT : 0);
   expp->value = val;
   expp->highwater = highwater;
   expp->size = size;
#ifdef NO_INTRA_FIX
   expp->next_export = sp->sindex;    /* Changed Later by ExpHash */
#else
   expp->next_export = symindex;      /* Changed Later by ExpHash */
#endif

   if( shlib_level == SHLIB_BUILD )
   {
	   shl_exports[expindex].next_symbol = next_symbol;
	   shl_exports[expindex].dmodule = mod_imports;
	   shl_exports[expindex].next_module = (next_module == -1 ? 
											          expindex : next_module);
   }

   return( expindex++ );
}

ExpHash()
{
   register long                *next;
   register struct export_entry *expp;
   register struct export_entry *expp2;
   register struct symbol       *s;
   int                           name = -1;
   int                           hashval;
   int                           strcmpval;
   int                           i;
   int                           expi;
   char sbuffer[25];

   ld_exphashsize = expindex * 2 + 1;

   exphash = (struct hash_entry *) 
	     calloc(ld_exphashsize, sizeof(struct hash_entry));

   for (i = 0; i < ld_exphashsize; i++) {
      exphash[i].symbol = -1;
   }

   /* for every export table entry index "i" */
   for (i = 0; i < expindex; i++) {

      expp = exports + i;

      /* get symbol table pointer (index was stored here by ExpEnter) */
      s = symtab + expp->next_export;
      /* for now, mark this as last in chain */
      expp->next_export = -1;

#if 0
      if ((expi = s->expindex) != -1) {
         if (expp->highwater == 0) {
            ExpCopy(i--,--expindex);
            continue;
         }
         if (exports[expi].highwater == 0) {
            expp->next_export = exports[expi].next_export;
            ExpCopy(expi,i);
            ExpCopy(i--,--expindex);
            continue;
         }
      }

      /* record export table index in symbol table */
      s->expindex = i;
#endif

      /* compute hash value for this name */
      hashval = hash(stringt + expp->name, ld_exphashsize);

      name = -1;

      /* start where hash says to start; follow "next_export" */
      /* in case you are wondering,
         we are looking for the right sport in the hash chain */
      for (next = &exphash[hashval].symbol;;next = &expp2->next_export) {

         /* empty slot found - MINE! */
         if (*next == -1) {
            *next = i;
            break;
         }

         /* slot taken - who is this guy in my slot, anyhow? */
         expp2 = exports + *next;

         /* if it's a relative of the guy in the previous slot, try again */
         if (expp2->name == name) {
            continue;
         }

         /* it's someone new */
         name = expp2->name;

         /* WE HAVE MET THE ENEMY... */
         if ((strcmpval = strcmp(expp->name + stringt, name + stringt)) > 0) {
            /* we are still later in the alphabet */
            continue;
         }

         if (strcmpval < 0) {
	    /* cut in front of this guy */
	    expp->next_export = *next;
	    *next = i;
	    break;
         }

         /* ...AND IT IS US! */
#ifdef NO_INTRA_FIX
	 /* if this isn't the "best" version, update symtab to point to front */
	 if (expp2->highwater > expp->highwater)
		s->expindex = expp2 - exports;
#else
	    s->expindex = -1;
#endif
         /* now search until we reach a new name, or a lower H2O mark */
         while (expp2->name == name && expp->highwater <= expp2->highwater ) {
	    if (expp->highwater == expp2->highwater && (bflag || iflag)) {
	       sprintf(sbuffer,"with highwater %u",expp->highwater);
	       error(e6,stringt+name,sbuffer);
	       exit_status = 1;
	       break;
	    }
	    /* stop if we reach the end of hash chain */
	    if (expp2->next_export == -1) {
	       next = &expp2->next_export;
	       break;
	    }
	    /* otherwise, try next guy in chain */
	    next = &expp2->next_export;
	    expp2 = exports + *next;
	 }
	 /* search terminated - either new name, lower H2O, or end of chain */
	 /* "next" at this point is the address of the "next_export" field
	    of the guy we want to insert *after*;
	    "*next" then is the export index we want to cut in front of */
	 expp->next_export = *next;
	 *next = i;
	 break;
      }
   }
}
#endif
   
/* This hash function was taken from 'hashpjw' in the Dragon book (pg. 436) */
#ifdef DLD
hash (sym)
#else
hash(sym, buckets)
#endif
CONST char *sym;
{
	register CONST char *p;
	unsigned int h, g;

	for( p = sym, h = 0; *p; p++ )
	{
		h = (h << 4) + *p;
		if( g = (h & 0xf0000000) )
		{
			h ^= (g >> 24);
			h ^= g;
		}
	}

#ifdef DLD
	return((int)h);
#else
	return( (int)(h % buckets) );
#endif
}

#ifndef  DLD
ExpSet(expi, val, size, seg)
int expi;
int val;
int size;
int seg;
{
   assert( expi >= 0 );

   exports[expi].value = val;
   exports[expi].size = size;
   /* Note that the following line does not set the EXPORT_PLT flag */
   exports[expi].type = seg;
}

ExpReloc()
{
   int i, t;

   for (i = 0; i < expindex; i++) 
   {
	  t = (exports[i].type & EXTERN2 ? TYPE_EXTERN2 : 0);
      switch (exports[i].type & FOURBITS) 
	  {
         case UNDEF:
#pragma		BBA_IGNORE
             fprintf( stderr, "Undefined in export list slot %d\n", i);
             break;

         case ABS:
			 exports[i].type = TYPE_ABSOLUTE;
             break;

         case TEXT:
			 if( exports[i].type & EXPORT_PLT )
			 {
				 /* Special case for exported PLT's, generated by AddShlib
			      */
				 exports[i].type = TYPE_SHL_PROC;
				 exports[i].value += plt_start;
			 }
			 else
			 {
				 exports[i].type = TYPE_PROCEDURE;
				 exports[i].value += (torigin + shlibtextsize);
			 }
             break;

         case DATA:
			 exports[i].type = TYPE_DATA;
             exports[i].value += dorigin;
             break;

         case BSS:
			 exports[i].type = TYPE_BSS;
			 exports[i].value += borigin;
             break;

         case COMM:
			 exports[i].type = TYPE_COMMON;
			 exports[i].value += corigin;
             break;

         default:
#pragma		BBA_IGNORE
            fprintf( stderr, "Strange type in export list slot %d\n", i);
             break;
      }
	  exports[i].type |= t;
   }
}
#endif

#if 0
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static int make_prime(num)
register int num;
{       register int *p = primes;
        while (num > *p)
        {       if (*p == 0) return num;
                p++;
        }
        return *p;
}

static int primes[] = {
	
#endif
