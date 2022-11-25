/*
static char rcsId[] =
  "@(#) $Header: hierarchy.c,v 66.6 90/09/26 22:38:01 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include "hierarchy.h"

#define NULL 0


/*****************************************************************************/

#ifndef KEYSHELL
/*VARARGS*/
void *HierRoot(hier)
Hier *hier;
{
  return hier->root;
}
#endif

/*VARARGS*/
void *HierParent(hier)
Hier *hier;
{
  return hier->parent;
}

/*VARARGS*/
void *HierHeadChild(hier)
Hier *hier;
{
  return LinkHead(&hier->children);
}

/*VARARGS*/
void *HierTailChild(hier)
Hier *hier;
{
  return LinkTail(&hier->children);
}

/*VARARGS*/
void *HierNthChild(hier, n)
Hier *hier;
int n;
{
  return LinkNth(&hier->children, n);
}

#ifndef KEYSHELL
/*VARARGS*/
void *HierNextSibling(hier)
Hier *hier;
{
  return LinkNext(&hier->siblings);
}
#endif

/*VARARGS*/
void *HierPrevSibling(hier)
Hier *hier;
{
  return LinkPrev(&hier->siblings);
}

/*VARARGS*/
void HierCreateRoot(root)
Hier *root;
{
  LinkCreateBase(root);
  LinkCreateBase(&root->children);
  root->parent = NULL;
  root->root = root;
}

#ifndef KEYSHELL
/*VARARGS*/
void HierAddHeadChild(exist, hier, preserve)
Hier *exist;
Hier *hier;
int preserve;
{
  LinkAddHead(&exist->children, hier);
  if (! preserve) {
    LinkCreateBase(&hier->children);
  }
  hier->parent = exist;
  hier->root = exist->root;
}
#endif

/*VARARGS*/
void HierAddTailChild(exist, hier, preserve)
Hier *exist;
Hier *hier;
int preserve;
{
  LinkAddTail(&exist->children, hier);
  if (! preserve) {
    LinkCreateBase(&hier->children);
  }
  hier->parent = exist;
  hier->root = exist->root;
}

/*VARARGS*/
void HierAddBeforeSibling(exist, hier, preserve)
Hier *exist;
Hier *hier;
int preserve;
{
  LinkAddBefore(exist, hier);
  if (! preserve) {
    LinkCreateBase(&hier->children);
  }
  hier->parent = exist->parent;
  hier->root = exist->root;
}

/*VARARGS*/
void HierAddAfterSibling(exist, hier, preserve)
Hier *exist;
Hier *hier;
int preserve;
{
  LinkAddAfter(exist, hier);
  if (! preserve) {
    LinkCreateBase(&hier->children);
  }
  hier->parent = exist->parent;
  hier->root = exist->root;
}

/*VARARGS*/
void *HierRemove(exist)
Hier *exist;
{
  (void)LinkRemove(exist);
  exist->parent = 0;
  exist->root = 0;
  return exist;
}

/*VARARGS*/
int HierChildCount(exist)
Hier *exist;
{
  return LinkCount(&exist->children);
}

/*****************************************************************************/
