/*
**********************************************************************
                                                                    
RULES.C -- Rule processor module for EPANET                  
                                                                
VERSION:    2.00                          
DATE:       5/8/00
            9/7/00
            10/25/00
            3/1/01
            8/15/07    (2.00.11)
AUTHOR:     L. Rossman
            US EPA - NRMRL
                                                                    
  The entry points for this module are:
     initrules()  -- called from ENopen() in EPANET.C
     addrule()    -- called from netsize() in INPUT2.C
     allocrules() -- called from allocdata() in EPANET.C
     ruledata()   -- called from newline() in INPUT2.C
     freerules()  -- called from freedata() in EPANET.C
     checkrules() -- called from ruletimestep() in HYDRAUL.C

**********************************************************************
*/
#include <stdio.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include "hash.h"
#include "text.h"
#include "epanet2.h"
#include "types.h"
#include "funcs.h"
#define  EXTERN  extern
#include "vars.h"
#include "errors.h"

enum    Rulewords      {r_RULE,r_IF,r_AND,r_OR,r_THEN,r_ELSE,r_PRIORITY,r_ERROR};
char    *Ruleword[]  = {w_RULE,w_IF,w_AND,w_OR,w_THEN,w_ELSE,w_PRIORITY,NULL};

enum    Varwords       {r_DEMAND, r_HEAD, r_GRADE, r_LEVEL, r_PRESSURE,
                        r_FLOW, r_STATUS, r_SETTING, r_POWER, r_TIME,
                        r_CLOCKTIME, r_FILLTIME, r_DRAINTIME};
char    *Varword[]   = {w_DEMAND, w_HEAD, w_GRADE, w_LEVEL, w_PRESSURE,
                        w_FLOW, w_STATUS, w_SETTING, w_POWER,w_TIME,
                        w_CLOCKTIME,w_FILLTIME,w_DRAINTIME, NULL};

enum    Objects        {r_JUNC,r_RESERV,r_TANK,r_PIPE,r_PUMP,r_VALVE,
                        r_NODE,r_LINK,r_SYSTEM};
char    *Object[]    = {w_JUNC,w_RESERV,w_TANK,w_PIPE,w_PUMP,w_VALVE,
                        w_NODE,w_LINK,w_SYSTEM,NULL};

/* NOTE: place "<=" & ">=" before "<" & ">" so that findmatch() works correctly. */
enum    Operators      { EQ, NE,  LE,  GE,  LT, GT, IS,  NOT,  BELOW,  ABOVE};
char    *Operator[]  = {"=","<>","<=",">=","<",">",w_IS,w_NOT,w_BELOW,w_ABOVE,NULL};

enum    Values         {IS_NUMBER,IS_OPEN,IS_CLOSED,IS_ACTIVE};
char    *Value[]     = {"XXXX",   w_OPEN, w_CLOSED, w_ACTIVE,NULL};

/* External variables declared in INPUT2.C */
//extern char      *Tok[MAXTOKS];
//extern int       Ntokens;

/*
**   Local function prototypes are defined here and not in FUNCS.H 
**   because some of them utilize the Premise and Action structures
**   defined locally in this module.
*/
void    newrule(OW_Project *m);
int     newpremise(OW_Project *m, int);
int     newaction(OW_Project *m);
int     newpriority(OW_Project *m);
int     evalpremises(OW_Project *m, int);
void    updateactlist(OW_Project *m, int, struct Action *);
int     checkaction(OW_Project *m, int, struct Action *);
int     checkpremise(OW_Project *m, struct Premise *);
int     checktime(OW_Project *m, struct Premise *);
int     checkstatus(OW_Project *m, struct Premise *);
int     checkvalue(OW_Project *m, struct Premise *);
int     takeactions(OW_Project *m);
void    clearactlist(OW_Project *m);
void    clearrules(OW_Project *m);
void    ruleerrmsg(OW_Project *m, int);


void initrules(OW_Project *m)
/*
**--------------------------------------------------------------
**    Initializes rule base.
**    Called by ENopen() in EPANET.C module
**--------------------------------------------------------------
*/
{
   m->RuleState = r_PRIORITY;
   m->Rule = NULL;
}


void addrule(OW_Project *m, char *tok)
/*
**--------------------------------------------------------------
**    Updates rule count if RULE keyword found in line of input.
**    Called by netsize() in INPUT2.C module.
**--------------------------------------------------------------
*/
{
   if (match(tok,w_RULE)) m->MaxRules++;
}


int  allocrules(OW_Project *m)
/*
**--------------------------------------------------------------
**    Allocates memory for rule-based controls.
**    Called by allocdata() in EPANET.C module.
**--------------------------------------------------------------
*/
{
   m->Rule = (struct aRule *) calloc(m->MaxRules + 1,sizeof(struct aRule));
   if (m->Rule == NULL)
     return(101);
   else return(0);
}


void freerules(OW_Project *m)
/*
**--------------------------------------------------------------
**    Frees memory used for rule-based controls.
**    Called by freedata() in EPANET.C module.
**--------------------------------------------------------------
*/
{
   clearrules(m);
   free(m->Rule);
}


int checkrules(OW_Project *m, long dt)
/*
**-----------------------------------------------------
**    Checks which rules should fire at current time.
**    Called by ruletimestep() in HYDRAUL.C.
**-----------------------------------------------------
*/
{
   int i,
       r;    /* Number of actions actually taken */

   /* Start of rule evaluation time interval */
   m->Time1 = m->Htime - dt + 1;

   /* Iterate through each rule */
   m->ActList = NULL;
   r = 0;
   for (i=1; i <= m->Nrules; i++)
   {
      /* If premises true, add THEN clauses to action list. */
      if (evalpremises(m,i) == TRUE)
        updateactlist(m,i,m->Rule[i].Tchain);

      /* If premises false, add ELSE actions to list. */
      else
      {
          if (m->Rule[i].Fchain != NULL)
            updateactlist(m,i,m->Rule[i].Fchain);
      }
   }

   /* Execute actions then clear list. */
   if (m->ActList != NULL)
     r = takeactions(m);
  
   clearactlist(m);
   return(r);
}


int  ruledata(OW_Project *m)
/*
**--------------------------------------------------------------
**    Parses a line from [RULES] section of input.
**    Called by newline() in INPUT2.C module.
**    Tok[] is global array of tokens parsed from input line.
**--------------------------------------------------------------
*/
{
   int    key,                      /* Keyword code */
          err;

   /* Exit if current rule has an error */
   if (m->RuleState == r_ERROR) return(0);

   /* Find the key word that begins the rule statement */
   err = 0;
   key = findmatch(m->Tok[0],Ruleword);
   switch (key)
   {
      case -1:     err = OW_ERR_SYNTAX;      /* Unrecognized keyword */
                   break;
      case r_RULE: m->Nrules++;
                   newrule(m);
                   m->RuleState = r_RULE;
                   break;
      case r_IF:   if (m->RuleState != r_RULE)
                   {
                      err = 221;   /* Mis-placed IF clause */
                      break;
                   }
                   m->RuleState = r_IF;
                   err = newpremise(m,r_AND);
                   break;
      case r_AND:  if (m->RuleState == r_IF) err = newpremise(m,r_AND);
                   else if (m->RuleState == r_THEN || m->RuleState == r_ELSE)
                      err = newaction(m);
                   else err = 221;
                   break;
      case r_OR:   if (m->RuleState == r_IF) err = newpremise(m,r_OR);
                   else err = 221;
                   break;
      case r_THEN: if (m->RuleState != r_IF)
                   {
                      err = 221;   /* Mis-placed THEN clause */
                      break;
                   }
                   m->RuleState = r_THEN;
                   err = newaction(m);
                   break;
      case r_ELSE: if (m->RuleState != r_THEN)
                   {
                      err = 221;   /* Mis-placed ELSE clause */
                      break;
                   }
                   m->RuleState = r_ELSE;
                   err = newaction(m);
                   break;
      case r_PRIORITY: if (m->RuleState != r_THEN && m->RuleState != r_ELSE)
                       {
                          err = 221;
                          break;
                       }
                       m->RuleState = r_PRIORITY;
                       err = newpriority(m);
                       break;
      default:         err = OW_ERR_SYNTAX;
   }

   /* Set RuleState to r_ERROR if errors found */
   if (err)
   {
      m->RuleState = r_ERROR;
      ruleerrmsg(m,err);
      err = 200;
   }
   return(err);
}


void  clearactlist(OW_Project *m)
/*
**----------------------------------------------------------
**    Clears memory used for action list
**----------------------------------------------------------
*/
{
   struct ActItem *a;
   struct ActItem *anext;
   a = m->ActList;
   while (a != NULL)
   {
      anext = a->next;
      free(a);
      a = anext;
   }
}


void  clearrules(OW_Project *m)
/*
**-----------------------------------------------------------
**    Clears memory used for premises & actions for all rules
**-----------------------------------------------------------
*/
{
   struct Premise *p;
   struct Premise *pnext;
   struct Action  *a;
   struct Action  *anext;
   int i;
   for (i=1; i <= m->Nrules; i++)
   {
      p = m->Rule[i].Pchain;
      while (p != NULL)
      {
         pnext = p->next;
         free(p);
         p = pnext;
      }
      a = m->Rule[i].Tchain;
      while (a != NULL)
      {
         anext = a->next;
         free(a);
         a = anext;
      }
      a = m->Rule[i].Fchain;
      while (a != NULL)
      {
         anext = a->next;
         free(a);
         a = anext;
      }
   }
}


void  newrule(OW_Project *m)
/*
**----------------------------------------------------------
**    Adds new rule to rule base
**----------------------------------------------------------
*/
{
  struct aRule *Rule = m->Rule;
   strncpy(Rule[m->Nrules].label, m->Tok[1], MAXID);
   Rule[m->Nrules].Pchain = NULL;
   Rule[m->Nrules].Tchain = NULL;
   Rule[m->Nrules].Fchain = NULL;
   Rule[m->Nrules].priority = 0.0;
   m->Plast = NULL;
}


int  newpremise(OW_Project *mod, int logop)
/*
**--------------------------------------------------------------------
**   Adds new premise to current rule.
**   Formats are:
**     IF/AND/OR <object> <id> <variable> <operator> <value>
**     IF/AND/OR  SYSTEM <variable> <operator> <value> (units)
**
**   Calls findmatch() and hour() in INPUT2.C.
**   Calls findnode() and findlink() in EPANET.C.
**---------------------------------------------------------------------
*/
{
   int   i,j,k,m,r,s,v;
   double x;
   struct Premise *p;

   /* Check for correct number of tokens */
   if (mod->Ntokens != 5 && mod->Ntokens != 6) return(OW_ERR_SYNTAX);

   /* Find network object & id if present */
   i = findmatch(mod->Tok[1],Object);
   if (i == r_SYSTEM)
   { 
      j = 0;
      v = findmatch(mod->Tok[2],Varword);
      if (v != r_DEMAND && v != r_TIME && v != r_CLOCKTIME) return(OW_ERR_SYNTAX);
   }
   else
   {
      v = findmatch(mod->Tok[3],Varword);
      if (v < 0) return(OW_ERR_SYNTAX);
      switch (i) 
      {
         case r_NODE:
         case r_JUNC:
         case r_RESERV:
         case r_TANK:   k = r_NODE; break;
         case r_LINK:
         case r_PIPE:
         case r_PUMP:
         case r_VALVE:  k = r_LINK; break;
         default: return(OW_ERR_SYNTAX);
      }
      i = k;
      if (i == r_NODE)
      {
         j = findnode(mod,mod->Tok[2]);
         if (j == 0) return(203);
         switch (v)
         {
            case r_DEMAND:
            case r_HEAD:
            case r_GRADE:
            case r_LEVEL:
            case r_PRESSURE: break;

/*** Updated 9/7/00 ***/
            case r_FILLTIME:
            case r_DRAINTIME: if (j <= mod->Njuncs) return(OW_ERR_SYNTAX); break;

            default: return(OW_ERR_SYNTAX);
         }
      }
      else
      {
         j = findlink(mod,mod->Tok[2]);
         if (j == 0) return(204);
         switch (v)
         {
            case r_FLOW:
            case r_STATUS:
            case r_SETTING: break;
            default: return(OW_ERR_SYNTAX);
         }
      }
   }

   /* Parse relational operator (r) and check for synonyms */
   if (i == r_SYSTEM)
     m = 3;
   else
     m = 4;
  
   k = findmatch(mod->Tok[m],Operator);
   if (k < 0)
     return(OW_ERR_SYNTAX);
   switch(k)
   {
      case IS:    r = EQ; break;
      case NOT:   r = NE; break;
      case BELOW: r = LT; break;
      case ABOVE: r = GT; break;
      default:    r = k;
   }

   /* Parse for status (s) or numerical value (x) */
   s = 0;
   x = MISSING;
   if (v == r_TIME || v == r_CLOCKTIME)
   {
      if (mod->Ntokens == 6)
         x = hour(mod->Tok[4],mod->Tok[5])*3600.;
      else
         x = hour(mod->Tok[4],"")*3600.;
      if (x < 0.0) return(202);
   }
   else if ((k = findmatch(mod->Tok[mod->Ntokens-1],Value)) > IS_NUMBER) s = k;
   else
   {
      if (!getfloat(mod->Tok[mod->Ntokens-1],&x)) return(202);
      if (v == r_FILLTIME || v == r_DRAINTIME) x = x*3600.0;                   //(2.00.11 - LR)
   }

   
         
   /* Create new premise structure */
   p = (struct Premise *) malloc(sizeof(struct Premise));
   if (p == NULL) return(101);
   p->object = i;
   p->index =  j;
   p->variable = v;
   p->relop = r;
   p->logop = logop;
   p->status   = s;
   p->value    = x;

   /* Add premise to current rule's premise list */
   p->next = NULL;
   if (mod->Plast == NULL)
     mod->Rule[mod->Nrules].Pchain = p;
   else
     mod->Plast->next = p;
   mod->Plast = p;
   return(0);
}


int  newaction(OW_Project *m)
/*
**----------------------------------------------------------
**   Adds new action to current rule.
**   Format is:
**      THEN/ELSE/AND LINK <id> <variable> IS <value>
**
**   Calls findlink() from EPANET.C.
**   Calls getfloat() and findmatch() from INPUT2.C.
**----------------------------------------------------------
*/
{
   int   j,k,s;
   double x;
   struct Action *a;

  Slink *Link = m->Link;
  struct aRule *Rule = m->Rule;
  
   /* Check for correct number of tokens */
   if (m->Ntokens != 6) return(OW_ERR_SYNTAX);

   /* Check that link exists */
   j = findlink(m,m->Tok[2]);
   if (j == 0) return(204);

/***  Updated 9/7/00  ***/
   /* Cannot control a CV */
   if (Link[j].Type == CV) return(207);

   /* Find value for status or setting */
   s = -1;
   x = MISSING;
   if ((k = findmatch(m->Tok[5],Value)) > IS_NUMBER) s = k;
   else
   {
      if (!getfloat(m->Tok[5],&x)) return(202);
      if (x < 0.0) return(202);
   }

/*** Updated 9/7/00 ***/
   /* Cannot change setting for a GPV ***/
   if (x != MISSING && Link[j].Type == GPV) return(202);

/*** Updated 3/1/01 ***/
   /* Set status for pipe in case setting was specified */
   if (x != MISSING && Link[j].Type == PIPE)
   {
      if (x == 0.0) s = IS_CLOSED;
      else          s = IS_OPEN;
      x = MISSING;
   }

   /* Create a new action structure */
   a = (struct Action *) malloc(sizeof(struct Action));
   if (a == NULL) return(101);
   a->link = j;
   a->status = s;
   a->setting = x;

   /* Add action to current rule's action list */
   if (m->RuleState == r_THEN)
   {
     a->next = Rule[m->Nrules].Tchain;
     Rule[m->Nrules].Tchain = a;
   }
   else
   {
      a->next = Rule[m->Nrules].Fchain;
      Rule[m->Nrules].Fchain = a;
   }
   return(0);
}


int  newpriority(OW_Project *m)
/*
**---------------------------------------------------
**    Adds priority rating to current rule
**---------------------------------------------------
*/
{
    double x;
    if (!getfloat(m->Tok[1],&x))
      return(202);
  
    m->Rule[m->Nrules].priority = x;
    return(0);
}


int  evalpremises(OW_Project *m, int i)
/*
**----------------------------------------------------------
**    Checks if premises to rule i are true
**----------------------------------------------------------
*/
{
    int result;
    struct Premise *p;

    result = TRUE;
    p = m->Rule[i].Pchain;
    while (p != NULL)
    {
        if (p->logop == r_OR)
        {
            if (result == FALSE)
            {
                result = checkpremise(m,p);
            }
        }
        else
        {
            if (result == FALSE) return(FALSE);
            result = checkpremise(m,p);
        }
        p = p->next;
    }
    return(result);
}

 
int  checkpremise(OW_Project *m, struct Premise *p)
/*
**----------------------------------------------------------
**    Checks if a particular premise is true
**----------------------------------------------------------
*/
{
    if (p->variable == r_TIME || p->variable == r_CLOCKTIME)
       return(checktime(m,p));
    else if (p->status > IS_NUMBER)
       return(checkstatus(m,p));
    else
       return(checkvalue(m,p));
}


int  checktime(OW_Project *m, struct Premise *p)
/*
**------------------------------------------------------------
**    Checks if condition on system time holds
**------------------------------------------------------------
*/
{
   char  flag;
   long  t1,t2,x;

   /* Get start and end of rule evaluation time interval */ 
   if (p->variable == r_TIME)
   {
        t1 = m->Time1;
        t2 = m->Htime;
   }
   else if (p->variable == r_CLOCKTIME)
   {
        t1 = (m->Time1 + m->Tstart) % SECperDAY;
        t2 = (m->Htime + m->Tstart) % SECperDAY;
   }
   else return(0);

   /* Test premise's time */
   x = (long)(p->value);
   switch (p->relop)
   {
      /* For inequality, test against current time */
        case LT: if (t2 >= x) return(0); break;
        case LE: if (t2 >  x) return(0); break;
        case GT: if (t2 <= x) return(0); break;
        case GE: if (t2 <  x) return(0); break;

      /* For equality, test if within interval */
        case EQ:
        case NE:
           flag = FALSE;
           if (t2 < t1)     /* E.g., 11:00 am to 1:00 am */
           {
              if (x >= t1 || x <= t2) flag = TRUE;
           }
           else
           {
              if (x >= t1 && x <= t2) flag = TRUE;
           }
           if (p->relop == EQ && flag == FALSE) return(0);
           if (p->relop == NE && flag == TRUE)  return(0);
           break;
   }

   /* If we get to here then premise was satisfied */
   return(1);
}


int  checkstatus(OW_Project *m, struct Premise *p)
/*
**------------------------------------------------------------
**    Checks if condition on link status holds
**------------------------------------------------------------
*/
{
    char i;
    int  j;
    switch (p->status)
    {
        case IS_OPEN:
        case IS_CLOSED:
        case IS_ACTIVE:
                i = m->hydraulics.LinkStatus[p->index];
                if      (i <= CLOSED) j = IS_CLOSED;
                else if (i == ACTIVE) j = IS_ACTIVE;
                else                  j = IS_OPEN;
                if (j == p->status &&
                p->relop == EQ)     return(1);
                if (j != p->status &&
                p->relop == NE) return(1);
    }
    return(0);
}


int  checkvalue(OW_Project *m, struct Premise *p)
/*
**----------------------------------------------------------
**    Checks if numerical condition on a variable is true.
**    Uses tolerance of 0.001 when testing conditions.
**----------------------------------------------------------
*/
{
    int   i,j,v;
    double x,
          tol = 1.e-3;
  
  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeHead = m->hydraulics.NodeHead;
  double *Ucf = m->Ucf;
  Snode *Node = m->Node;
  Slink *Link = m->Link;
  Stank *Tank = m->Tank;
  double *LinkFlows = m->hydraulics.LinkFlows;
  double *LinkSetting = m->hydraulics.LinkSetting;
  int Njuncs = m->Njuncs;
  double Dsystem = m->Dsystem;
  
    i = p->index;
    v = p->variable;
    switch (v)
    {

/*** Updated 10/25/00 ***/
        case r_DEMAND:    if (p->object == r_SYSTEM) x = Dsystem * Ucf[DEMAND];
                          else x = NodeDemand[i]*Ucf[DEMAND];
                          break;

        case r_HEAD:
        case r_GRADE:     x = NodeHead[i]*Ucf[HEAD];
                          break;
        case r_PRESSURE:  x = (NodeHead[i]-Node[i].El)*Ucf[PRESSURE];
                          break;
        case r_LEVEL:     x = (NodeHead[i]-Node[i].El)*Ucf[HEAD];
                          break;
        case r_FLOW:      x = ABS(LinkFlows[i])*Ucf[FLOW];
                          break;
        case r_SETTING:   if (LinkSetting[i] == MISSING) return(0);
                          x = LinkSetting[i];
                          switch (Link[i].Type)
                          {
                             case PRV:
                             case PSV:
                             case PBV:  x = x*Ucf[PRESSURE]; break;
                             case FCV:  x = x*Ucf[FLOW];     break;
                          }
                          break;
        case r_FILLTIME:  if (i <= Njuncs) return(0);
                          j = i-Njuncs;
                          if (Tank[j].A == 0.0) return(0);
                          if (NodeDemand[i] <= TINY) return(0);
                          x = (Tank[j].Vmax - Tank[j].V)/NodeDemand[i];
                          break;
        case r_DRAINTIME: if (i <= Njuncs) return(0);
                          j = i-Njuncs;
                          if (Tank[j].A == 0.0) return(0);
                          if (NodeDemand[i] >= -TINY) return(0);
                          x = (Tank[j].Vmin - Tank[j].V)/NodeDemand[i];
                          break;
        default:          return(0);
    }
    switch (p->relop)
    {
        case EQ:        if (ABS(x - p->value) > tol) return(0);
                        break;
        case NE:        if (ABS(x - p->value) < tol) return(0);
                        break;
        case LT:        if (x > p->value + tol) return(0); break;
        case LE:        if (x > p->value - tol) return(0); break;
        case GT:        if (x < p->value - tol) return(0); break;
        case GE:        if (x < p->value + tol) return(0); break;
    }
    return(1);
}


void  updateactlist(OW_Project *m, int i, struct Action *actions)
/*
**---------------------------------------------------
**    Adds rule's actions to action list
**---------------------------------------------------
*/
{
   struct ActItem *item;
   struct Action *a;
  

   /* Iterate through each action of Rule i */
   a = actions;
   while (a != NULL)
   {
      /* Add action to list if not already on it */
      if (!checkaction(m,i,a))
      {
         item = (struct ActItem *) malloc(sizeof(struct ActItem));
         if (item != NULL)
         {
            item->action = a;
            item->ruleindex = i;
            item->next = m->ActList;
            m->ActList = item;
         }
      }
      a = a->next;
   }
}


int  checkaction(OW_Project *m, int i, struct Action *a)
/*
**-----------------------------------------------------------
**    Checks if an action is already on the Action List
**-----------------------------------------------------------
*/
{
   int i1,k,k1;
   struct ActItem *item;
   struct Action *a1;

   /* Search action list for link named in action */
   k = a->link;                 /* Action applies to link k */
   item = m->ActList;
   while (item != NULL)
   {
      a1 = item->action;
      i1 = item->ruleindex;
      k1 = a1->link;

      /* If link on list then replace action if rule has higher priority. */
      if (k1 == k)
      {
         if (m->Rule[i].priority > m->Rule[i1].priority)
         {
            item->action = a;
            item->ruleindex = i;
         }
         return(1);
      }
      item = item->next;
   }
   return(0);
}


int  takeactions(OW_Project *m)
/*
**-----------------------------------------------------------
**    Implements actions on action list
**-----------------------------------------------------------
*/
{
    struct Action *a;
    struct ActItem *item;
    char   flag;
    int    k, s, n;
    double  tol = 1.e-3,
           v, x;
  
  struct aRule *Rule = m->Rule;
  double *Ucf = m->Ucf;
  Slink *Link = m->Link;
  double *LinkSetting = m->hydraulics.LinkSetting;
  char *LinkStatus = m->hydraulics.LinkStatus;

    n = 0;
    item = m->ActList;
    while (item != NULL)
    {
        flag = FALSE;
        a = item->action;
        k = a->link;
        s = LinkStatus[k];
        v = LinkSetting[k];
        x = a->setting;

        /* Switch link from closed to open */
        if (a->status == IS_OPEN && s <= CLOSED)
        {
            setlinkstatus(m, k, 1, &LinkStatus[k], &LinkSetting[k]);
            flag = TRUE;
        }

        /* Switch link from not closed to closed */ 
        else if (a->status == IS_CLOSED && s > CLOSED)
        {
            setlinkstatus(m, k, 0, &LinkStatus[k], &LinkSetting[k]);
            flag = TRUE;
        }

        /* Change link's setting */
        else if (x != MISSING)
        {
            switch(Link[k].Type)
            {
                case PRV:
                case PSV:
                case PBV:    x = x/Ucf[PRESSURE];  break;
                case FCV:    x = x/Ucf[FLOW];      break;
            }
            if (ABS(x-v) > tol)
            {
                setlinksetting(m, k, x, &LinkStatus[k], &LinkSetting[k]);
                flag = TRUE;
            }
        }

        /* Report rule action */
        if (flag == TRUE)
        {
           n++;
           if (m->Statflag)
             writeruleaction(m,k,Rule[item->ruleindex].label);
        }

        /* Move to next action on list */
        item = item->next;
    }
    return(n);
}


void  ruleerrmsg(OW_Project *m, int err)
/*
**-----------------------------------------------------------
**    Reports error message
**-----------------------------------------------------------
*/
{
   int    i;
   char   label[81];
   char   fmt[256];
  
  char *Msg = m->Msg;
  int Nrules = m->Nrules;
  struct aRule *Rule = m->Rule;
  
   switch (err)
   {
      case OW_ERR_SYNTAX:   strcpy(fmt,R_ERR201);  break;
      case 202:   strcpy(fmt,R_ERR202);  break;
      case 203:   strcpy(fmt,R_ERR203);  break;
      case 204:   strcpy(fmt,R_ERR204);  break;

/***  Updated on 9/7/00  ***/
      case 207:   strcpy(fmt,R_ERR207);  break;

      case 221:   strcpy(fmt,R_ERR221);  break;
      default:    return;
   }
   if (Nrules > 0)
   {
      strcpy(label,t_RULE);
      strcat(label," ");
      strcat(label,Rule[Nrules].label);
   }
   else strcpy(label,t_RULES_SECT);
   sprintf(Msg,fmt);
   strcat(Msg,label);
   strcat(Msg,":");
   writeline(m,Msg);
   strcpy(fmt,m->Tok[0]);
   for (i=1; i < m->Ntokens; i++)
   {
      strcat(fmt," ");
      strcat(fmt,m->Tok[i]);
   }
   writeline(m,fmt);
}
   
/***************** END OF RULES.C ******************/   

