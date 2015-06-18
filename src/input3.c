/*
**********************************************************************

INPUT3.C -- Input data parser for EPANET

VERSION:    2.00
DATE:       5/30/00
            9/7/00
            10/25/00
            3/1/01
            6/24/02
            8/15/07    (2.00.11)
            2/14/08    (2.00.12)
AUTHOR:     L. Rossman
            US EPA - NRMRL

This module parses data from each line of input from file InFile.
All functions in this module are called from newline() in INPUT2.C.

**********************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <math.h>
#include "hash.h"
#include "text.h"
#include "epanet2.h"
#include "types.h"
#include "funcs.h"
#define  EXTERN  extern
#include "vars.h"

///* Defined in enumstxt.h in EPANET.C */
extern char *MixTxt[];
extern char *Fldname[]; 
//
///* Defined in INPUT2.C */
//extern char      *Tok[MAXTOKS];
//extern STmplist  *PrevPat;
//extern STmplist  *PrevCurve;
//
//extern STmplist  *PrevCoord;
//extern int       Ntokens;


int  juncdata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes junction data                             
**  Format:                                                      
**    [JUNCTIONS]                                              
**      id  elev.  (demand)  (demand pattern)                  
**--------------------------------------------------------------
*/
{
   int      n, p = 0;
   double    el,y = 0.0;
   Pdemand  demand;
   STmplist *pat;
   Snode *Node = m->network.Node;

/* Add new junction to data base */
   n = m->Ntokens;
   if (m->network.Nnodes == m->MaxNodes)
     return(200);
  
   m->network.Njuncs++;
   m->network.Nnodes++;
  
   if (!addnodeID(m, m->network.Njuncs, m->Tok[0]))
     return(215);
  
  
/* Check for valid data */
   if (n < 2)
     return(OW_ERR_SYNTAX);
   if (!getfloat(m->Tok[1],&el))
     return(202);
   if (n >= 3  && !getfloat(m->Tok[2],&y))
     return(202);
   if (n >= 4) {
      pat = findID(m->Tok[3],m->Patlist);
      if (pat == NULL)
        return(205);
      p = pat->i;
   }

/* Save junction data */
   Node[m->network.Njuncs].El  = el;
   Node[m->network.Njuncs].C0  = 0.0;
   Node[m->network.Njuncs].S   = NULL;
   Node[m->network.Njuncs].Ke  = 0.0;
   Node[m->network.Njuncs].Rpt = 0;

/* Create a new demand record */
/*** Updated 6/24/02 ***/
   if (n >= 3)
   {
      demand = (struct Sdemand *) malloc(sizeof(struct Sdemand));
      if (demand == NULL) return(101);
      demand->Base = y;
      demand->Pat = p;
      demand->next = Node[m->network.Njuncs].D;
      Node[m->network.Njuncs].D = demand;
      m->hydraulics.NodeDemand[m->network.Njuncs] = y;
   }
   else m->hydraulics.NodeDemand[m->network.Njuncs] = MISSING;
/*** end of update ***/
   return(0);
}                        /* end of juncdata */


int  tankdata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                              
**  Output:  returns error code                                
**  Purpose: processes tank & reservoir data                   
**  Format:                                                    
**   [RESERVOIRS]                                            
**     id elev (pattern)                                     
**   [TANKS]                                                 
**     id elev (pattern)                                     
**     id elev initlevel minlevel maxlevel diam (minvol vcurve)         
**--------------------------------------------------------------
*/
{
   int   i,               /* Node index */
         n,               /* # data items */
         p = 0,           /* Fixed grade time pattern index */
         vcurve = 0;      /* Volume curve index */
   double el        = 0.0, /* Elevation */
         initlevel = 0.0, /* Initial level */
         minlevel  = 0.0, /* Minimum level */
         maxlevel  = 0.0, /* Maximum level */
         minvol    = 0.0, /* Minimum volume */
         diam      = 0.0, /* Diameter */
         area;            /* X-sect. area */
   STmplist *t;
   Snode *Node = m->network.Node;
   Stank *Tank = m->network.Tank;
   STmplist *Curvelist = m->Curvelist;
   char **Tok = m->Tok;

/* Add new tank to data base */
   n = m->Ntokens;
  
   if (m->network.Ntanks == m->MaxTanks ||  m->network.Nnodes == m->MaxNodes)
     return(200);
  
   m->network.Ntanks++;
   m->network.Nnodes++;
   i = m->MaxJuncs + m->network.Ntanks;                    /* i = node index.     */
  
   if (!addnodeID(m, i, Tok[0]))
     return(215);    /* Add ID to database. */

/* Check for valid data */
   if (n < 2) return(OW_ERR_SYNTAX);                   /* Too few fields.   */
   if (!getfloat(Tok[1],&el)) return(202);   /* Read elevation    */
   if (n <= 3)                               /* Tank is reservoir.*/
   {
      if (n == 3)                            /* Pattern supplied  */
      {
         t = findID(Tok[2], m->Patlist);
         if (t == NULL) return(205);
         p = t->i;
      }
   }
   else if (n < 6) {
     return(OW_ERR_SYNTAX);              /* Too few fields for tank.*/
   }
   else {
      /* Check for valid input data */
      if (!getfloat(Tok[2],&initlevel)) return(202);
      if (!getfloat(Tok[3],&minlevel))  return(202);
      if (!getfloat(Tok[4],&maxlevel))  return(202);
      if (!getfloat(Tok[5],&diam))      return(202);
      if (diam < 0.0)                   return(202);
      if (n >= 7
      && !getfloat(Tok[6],&minvol))     return(202);

      /* If volume curve supplied check it exists */
      if (n == 8)
      {                           
         t = findID(Tok[7],Curvelist);
         if (t == NULL) return(202);
         vcurve = t->i;
      }
   }

   int iTank = m->network.Ntanks;
   Node[i].Rpt           = 0;
   Node[i].El            = el;               /* Elevation.           */
   Node[i].C0            = 0.0;              /* Init. quality.       */
   Node[i].S             = NULL;             /* WQ source data       */     
   Node[i].Ke            = 0.0;              /* Emitter coeff.       */
   Tank[iTank].Node     = i;                /* Node index.          */
   Tank[iTank].H0       = initlevel;        /* Init. level.         */
   Tank[iTank].Hmin     = minlevel;         /* Min. level.          */
   Tank[iTank].Hmax     = maxlevel;         /* Max level.           */
   Tank[iTank].A        = diam;             /* Diameter.            */
   Tank[iTank].Pat      = p;                /* Fixed grade pattern. */
   Tank[iTank].Kb       = MISSING;          /* Reaction coeff.      */
   /*
   *******************************************************************
    NOTE: The min, max, & initial volumes set here are based on a     
       nominal tank diameter. They will be modified in INPUT1.C if    
       a volume curve is supplied for this tank.                      
   *******************************************************************
   */
   area = PI*SQR(diam)/4.0;
   Tank[iTank].Vmin = area*minlevel;
   if (minvol > 0.0) {
     Tank[iTank].Vmin = minvol;
   }
   Tank[iTank].V0 = Tank[iTank].Vmin + area*(initlevel - minlevel);
   Tank[iTank].Vmax = Tank[iTank].Vmin + area*(maxlevel - minlevel);

   Tank[iTank].Vcurve   = vcurve;           /* Volume curve         */
   Tank[iTank].MixModel = MIX1;             /* Completely mixed     */
   Tank[iTank].V1max    = 1.0;              /* Compart. size ratio  */
   return(0);
}                        /* end of tankdata */


int  pipedata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                              
**  Output:  returns error code                                
**  Purpose: processes pipe data                               
**  Format:                                                    
**    [PIPE]                                                
**    id  node1  node2  length  diam  rcoeff (lcoeff) (status)          
**--------------------------------------------------------------
*/
{
   int   j1,                     /* Start-node index  */
         j2,                     /* End-node index    */
         n;                      /* # data items      */
   char  type = PIPE,            /* Link type         */
         status = OPEN;          /* Link status       */
   double length,                 /* Link length       */
         diam,                   /* Link diameter     */
         rcoeff,                 /* Roughness coeff.  */
         lcoeff = 0.0;           /* Minor loss coeff. */

  char **Tok = m->Tok;
  
/* Add new pipe to data base */
   n = m->Ntokens;
   if (m->network.Nlinks == m->MaxLinks)
     return(200);
  
   m->network.Npipes++;
   m->network.Nlinks++;
   if (!addlinkID(m, m->network.Nlinks, Tok[0])) return(215);

/* Check for valid data */
   if (n < 6) {
     return(OW_ERR_SYNTAX);
   }
   if ((j1 = findnode(m,Tok[1])) == 0 || (j2 = findnode(m,Tok[2])) == 0) {
     return(203);
   }

/*** Updated 10/25/00 ***/
   if (j1 == j2)
     return(222);

   if (!getfloat(Tok[3],&length) ||
       !getfloat(Tok[4],&diam)   ||
       !getfloat(Tok[5],&rcoeff)
      ) return(202);

   if (length <= 0.0 ||
       diam   <= 0.0 ||
       rcoeff <= 0.0
      ) return(202);

   /* Case where either loss coeff. or status supplied */
   if (n == 7)
   {
      if      (match(Tok[6],w_CV))        type = CV;
      else if (match(Tok[6],w_CLOSED))    status = CLOSED;
      else if (match(Tok[6],w_OPEN))      status = OPEN;
      else if (!getfloat(Tok[6],&lcoeff)) return(202);
   }

   /* Case where both loss coeff. and status supplied */
   if (n == 8)
   {
      if (!getfloat(Tok[6],&lcoeff))   return(202);
      if      (match(Tok[7],w_CV))     type = CV;
      else if (match(Tok[7],w_CLOSED)) status = CLOSED;
      else if (match(Tok[7],w_OPEN))   status = OPEN;
      else return(202);
   }
   if (lcoeff < 0.0) return(202);

/* Save pipe data */
   Slink *Link = m->network.Link;
   int Nlinks = m->network.Nlinks;
  
   Link[Nlinks].N1    = j1;                  /* Start-node index */
   Link[Nlinks].N2    = j2;                  /* End-node index   */
   Link[Nlinks].Len   = length;              /* Length           */
   Link[Nlinks].Diam  = diam;                /* Diameter         */
   Link[Nlinks].Kc    = rcoeff;              /* Rough. coeff     */
   Link[Nlinks].Km    = lcoeff;              /* Loss coeff       */
   Link[Nlinks].Kb    = MISSING;             /* Bulk coeff       */
   Link[Nlinks].Kw    = MISSING;             /* Wall coeff       */
   Link[Nlinks].Type  = type;                /* Link type        */
   Link[Nlinks].Stat  = status;              /* Link status      */
   Link[Nlinks].Rpt   = 0;                   /* Report flag      */
   return(0);
}                        /* end of pipedata */


int  pumpdata(OW_Project *mod)
/*
**--------------------------------------------------------------
** Input:   none                                                
** Output:  returns error code                                  
** Purpose: processes pump data                                 
** Formats:                                                     
**  [PUMP]                                                     
**   (Version 1.x Format):                                              
**   id  node1  node2  power                                   
**   id  node1  node2  h1    q1                                
**   id  node1  node2  h0    h1   q1   h2   q2                 
**   (Version 2 Format):                                              
**   id  node1  node2  KEYWORD value {KEYWORD value ...}       
**   where KEYWORD = [POWER,HEAD,PATTERN,SPEED]                
**--------------------------------------------------------------
*/
{
   int   j,
         j1,                    /* Start-node index */
         j2,                    /* End-node index   */
         m, n;                  /* # data items     */
   double y;
   STmplist *t;                 /* Pattern record   */

/* Add new pump to data base */
   n = mod->Ntokens;
  char **Tok = mod->Tok;
  
   if (mod->network.Nlinks == mod->MaxLinks ||
       mod->network.Npumps == mod->MaxPumps
      ) return(200);
   mod->network.Nlinks++;
   mod->network.Npumps++;
   if (!addlinkID(mod, mod->network.Nlinks, Tok[0])) return(215);

/* Check for valid data */
   if (n < 4) return(OW_ERR_SYNTAX);
   if ((j1 = findnode(mod, Tok[1])) == 0 ||
       (j2 = findnode(mod, Tok[2])) == 0
      ) return(203);

/*** Updated 10/25/00 ***/
   if (j1 == j2) return(222);    

/* Save pump data */
  Slink *Link = mod->network.Link;
  Spump *Pump = mod->network.Pump;
  int Nlinks = mod->network.Nlinks;
  int Npumps = mod->network.Npumps;
  
   Link[Nlinks].N1    = j1;               /* Start-node index.  */
   Link[Nlinks].N2    = j2;               /* End-node index.    */
   Link[Nlinks].pumpLinkIdx  = Npumps;           /* Pump index.        */
   Link[Nlinks].Len   = 0.0;              /* Link length.       */
   Link[Nlinks].Kc    = 1.0;              /* Speed factor.      */
   Link[Nlinks].Km    = 0.0;              /* Horsepower.        */
   Link[Nlinks].Kb    = 0.0;
   Link[Nlinks].Kw    = 0.0;
   Link[Nlinks].Type  = PUMP;             /* Link type.         */
   Link[Nlinks].Stat  = OPEN;             /* Link status.       */
   Link[Nlinks].Rpt   = 0;                /* Report flag.       */
   Pump[Npumps].Link = Nlinks;            /* Link index.        */
   Pump[Npumps].Ptype = NOCURVE;          /* Type of pump curve */
   Pump[Npumps].Hcurve = 0;               /* Pump curve index   */
   Pump[Npumps].Ecurve = 0;               /* Effic. curve index */
   Pump[Npumps].Upat   = 0;               /* Utilization pattern*/
   Pump[Npumps].Ecost  = 0.0;             /* Unit energy cost   */
   Pump[Npumps].Epat   = 0;               /* Energy cost pattern*/

/* If 4-th token is a number then input follows Version 1.x format */
/* so retrieve pump curve parameters */
   if (getfloat(Tok[3],&(mod->X[0])))
   {
      m = 1;
      for (j=4; j<n; j++)
      {
         if (!getfloat(Tok[j],&(mod->X[m])))
           return(202);
         m++;
      }
      return(getpumpcurve(mod, m));          /* Get pump curve params */
   }

/* Otherwise input follows Version 2 format */
/* so retrieve keyword/value pairs.         */
   m = 4;
   while (m < n)
   {
      if (match(Tok[m-1],w_POWER))          /* Const. HP curve       */
      {
         y = atof(Tok[m]);
         if (y <= 0.0) return(202);
         Pump[Npumps].Ptype = CONST_HP;
         Link[Nlinks].Km = y;
      }
      else if (match(Tok[m-1],w_HEAD))      /* Custom pump curve      */
      {
         t = findID(Tok[m],mod->Curvelist);
         if (t == NULL) return(206);
         Pump[Npumps].Hcurve = t->i;
      }
      else if (match(Tok[m-1],w_PATTERN))   /* Speed/status pattern */
      {
         t = findID(Tok[m],mod->Patlist);
         if (t == NULL) return(205);
         Pump[Npumps].Upat = t->i;
      }
      else if (match(Tok[m-1],w_SPEED))     /* Speed setting */
      {
         if (!getfloat(Tok[m],&y)) return(202);
         if (y < 0.0) return(202);
         Link[Nlinks].Kc = y;
      }
      else
        return(OW_ERR_SYNTAX);
      m = m + 2;                          /* Skip to next keyword token */
   }
   return(0);
}                        /* end of pumpdata */


int  valvedata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes valve data                                
**  Format:                                                      
**     [VALVE]                                                 
**        id  node1  node2  diam  type  setting (lcoeff)       
**--------------------------------------------------------------
*/
{
   int   j1,                    /* Start-node index   */
         j2,                    /* End-node index     */
         n;                     /* # data items       */
   char  status = ACTIVE,       /* Valve status       */
         type;                  /* Valve type         */
   double diam = 0.0,            /* Valve diameter     */
         setting,               /* Valve setting      */
         lcoeff = 0.0;          /* Minor loss coeff.  */
   STmplist *t;                 /* Curve record       */
   char **Tok = m->Tok;
  
/* Add new valve to data base */
   n = m->Ntokens;
   if (m->network.Nlinks == m->MaxLinks ||
       m->network.Nvalves == m->MaxValves
      ) return(200);
   m->network.Nvalves++;
   m->network.Nlinks++;
   if (!addlinkID(m,m->network.Nlinks,Tok[0])) return(215);

/* Check for valid data */
   if (n < 6)
     return(OW_ERR_SYNTAX);
   if ((j1 = findnode(m,Tok[1])) == 0 || (j2 = findnode(m,Tok[2])) == 0)
     return(203);

/*** Updated 10/25/00 ***/
   if (j1 == j2)
     return(222);

   if      (match(Tok[4],w_PRV))
     type = PRV;
   else if (match(Tok[4],w_PSV))
     type = PSV;
   else if (match(Tok[4],w_PBV))
     type = PBV;
   else if (match(Tok[4],w_FCV))
     type = FCV;
   else if (match(Tok[4],w_TCV))
    type = TCV;
   else if (match(Tok[4],w_GPV))
     type = GPV;
   else
     return(OW_ERR_SYNTAX);                      /* Illegal valve type.*/
   if (!getfloat(Tok[3],&diam))
     return(202);
   if (diam <= 0.0)
     return(202);             /* Illegal diameter.*/
  
   if (type == GPV)                          /* Headloss curve for GPV */
   {
      t = findID(Tok[5],m->Curvelist);
      if (t == NULL) return(206);
      setting = t->i;

/*** Updated 9/7/00 ***/
      status = OPEN;

   }
   else if (!getfloat(Tok[5],&setting)) return(202);
   if (n >= 7 &&
       !getfloat(Tok[6],&lcoeff)
      ) return(202);

/* Check that PRV, PSV, or FCV not connected to a tank & */
/* check for illegal connections between pairs of valves.*/
   if ((j1 > m->network.Njuncs || j2 > m->network.Njuncs) &&
       (type == PRV || type == PSV || type == FCV)
      ) return(219);
   if (!valvecheck(m,type,j1,j2)) return(220);

/* Save valve data */
  int Nlinks = m->network.Nlinks;
  int Nvalves = m->network.Nvalves;
  Slink *Link = &(m->network.Link[Nlinks]);
  Svalve *Valve = m->network.Valve;
  
  
   Link->N1     = j1;                 /* Start-node index. */
   Link->N2     = j2;                 /* End-node index.   */
   Link->Diam   = diam;               /* Valve diameter.   */
   Link->Len    = 0.0;                /* Link length.      */
   Link->Kc     = setting;            /* Valve setting.    */
   Link->Km     = lcoeff;             /* Loss coeff        */
   Link->Kb     = 0.0;
   Link->Kw     = 0.0;
   Link->Type   = type;               /* Valve type.       */
   Link->Stat   = status;             /* Valve status.     */
   Link->Rpt    = 0;                  /* Report flag.      */
   Valve[Nvalves].Link = Nlinks;             /* Link index.       */
   return(0);
}                        /* end of valvedata */


int  patterndata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes time pattern data                         
**  Format:                                                      
**     [PATTERNS]                                              
**        id  mult1  mult2 .....                               
**--------------------------------------------------------------
*/
{
   int  i,n;
   double x;
   SFloatlist *f;
   STmplist   *p;
   char **Tok = m->Tok;
   STmplist  *PrevPat = m->PrevPat;
  
   n = m->Ntokens - 1;
   if (n < 1) return(OW_ERR_SYNTAX);            /* Too few values        */
   if (                               /* Check for new pattern */
          PrevPat != NULL &&
          strcmp(Tok[0],PrevPat->ID) == 0
      ) p = PrevPat;
   else p = findID(Tok[0],m->Patlist);
   if (p == NULL) return(205);
   for (i=1; i<=n; i++)               /* Add multipliers to list */
   {
       if (!getfloat(Tok[i],&x)) return(202);
       f = (SFloatlist *) malloc(sizeof(SFloatlist));
       if (f == NULL) return(101);
       f->value = x;
       f->next = p->x;
       p->x = f;
   }
   m->network.Pattern[p->i].Length += n;         /* Save # multipliers for pattern */
   PrevPat = p;                       /* Set previous pattern pointer */
   return(0);
}                        /* end of patterndata */


int  curvedata(OW_Project *m)
/*
**------------------------------------------------------
**  Input:   none                                        
**  Output:  returns error code                          
**  Purpose: processes curve data                        
**  Format:                                              
**     [CURVES]                                        
**      CurveID   x-value  y-value                    
**------------------------------------------------------
*/
{
   double      x,y;
   SFloatlist *fx, *fy;
   STmplist   *c;
  
  char **Tok = m->Tok;
  STmplist  *PrevCurve = m->PrevCurve;

   /* Check for valid curve ID */
   if (m->Ntokens < 3) return(OW_ERR_SYNTAX);
   if (
          PrevCurve != NULL &&
          strcmp(Tok[0],PrevCurve->ID) == 0
      ) c = PrevCurve;
   else c = findID(Tok[0],m->Curvelist);
   if (c == NULL) return(205);

   /* Check for valid data */
   if (!getfloat(Tok[1],&x)) return(202);
   if (!getfloat(Tok[2],&y)) return(202);

   /* Add new data point to curve's linked list */
   fx = (SFloatlist *) malloc(sizeof(SFloatlist));
   fy = (SFloatlist *) malloc(sizeof(SFloatlist));
   if (fx == NULL || fy == NULL) return(101);
   fx->value = x;
   fx->next = c->x;
   c->x = fx;
   fy->value = y;
   fy->next = c->y;
   c->y = fy;
   m->network.Curve[c->i].Npts++;

   /* Save the pointer to this curve */
   PrevCurve = c;
   return(0);
}

int  coordata(OW_Project *m)
/*
 **--------------------------------------------------------------
 **  Input:   none
 **  Output:  returns error code
 **  Purpose: processes coordinate data
 **  Format:
 **    [COORD]
 **      id  x  y
 **--------------------------------------------------------------
 */
{
	double      x,y;
	SFloatlist *fx, *fy;
	STmplist   *c;
  
  char **Tok = m->Tok;
  STmplist  *PrevCoord = m->PrevCoord;
  
	/* Check for valid curve ID */
	if (m->Ntokens < 3) return(OW_ERR_SYNTAX);
  
	if (
      PrevCoord != NULL &&
      strcmp(Tok[0],PrevCoord->ID) == 0
      ) c = PrevCoord;
	else c = findID(Tok[0],m->Coordlist);
  
  //	c = findID(Tok[0],Coordlist);
	if (c == NULL) return(205);
  
	/* Check for valid data */
	if (!getfloat(Tok[1],&x)) return(202);
	if (!getfloat(Tok[2],&y)) return(202);
  
	/* Add new data point to curve's linked list */
	fx = (SFloatlist *) malloc(sizeof(SFloatlist));
	fy = (SFloatlist *) malloc(sizeof(SFloatlist));
	if (fx == NULL || fy == NULL) return(101);
	fx->value = x;
	fx->next = c->x;
	c->x = fx;
	fy->value = y;
	fy->next = c->y;
	c->y = fy;
	//Curve[c->i].Npts++;
  
	/* Save the pointer to this curve */
	PrevCoord = c;
	return(0);
  
	/* Save coordn data */
	//Coord[Njuncs].X  = x;
	//Coord[Njuncs].Y  = y;
  
}                        /* end of coordata */

int  demanddata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes node demand data                          
**  Format:                                                      
**     [DEMANDS]                                               
**        MULTIPLY  factor                                     
**        node  base_demand  (pattern)                         
**
**  NOTE: Demands entered in this section replace those 
**        entered in the [JUNCTIONS] section
**--------------------------------------------------------------
*/
{
   int  j,n,p = 0;
   double y;
   Pdemand demand;
   STmplist *pat;
  
  char **Tok = m->Tok;
  double *NodeDemand = m->hydraulics.NodeDemand;
  Snode *Node = m->network.Node;

/* Extract data from tokens */
   n = m->Ntokens;
   if (n < 2) return(OW_ERR_SYNTAX);
   if (!getfloat(Tok[1],&y)) return(202);

/* If MULTIPLY command, save multiplier */
   if (match(Tok[0],w_MULTIPLY))
   {
      if (y <= 0.0) return(202);
      else m->Dmult = y;
      return(0);
   }

/* Otherwise find node (and pattern) being referenced */
   if ((j = findnode(m,Tok[0])) == 0) return(208);
   if (j > m->network.Njuncs) return(208);
   if (n >= 3)
   {
      pat = findID(Tok[2],m->Patlist);
      if (pat == NULL)  return(205);
      p = pat->i;
   }

/* Replace any demand entered in [JUNCTIONS] section */
/* (Such demand was temporarily stored in D[]) */

/*** Updated 6/24/02 ***/
   demand = Node[j].D;
   if (demand && NodeDemand[j] != MISSING)
   {
      demand->Base = y;
      demand->Pat  = p;
      NodeDemand[j] = MISSING;
   }
/*** End of update ***/

/* Otherwise add a new demand to this junction */
   else
   {
      demand = (struct Sdemand *) malloc(sizeof(struct Sdemand));
      if (demand == NULL) return(101);
      demand->Base = y;
      demand->Pat = p;
      demand->next = Node[j].D;
      Node[j].D = demand;
   }
   return(0);
}                        /* end of demanddata */


int  controldata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes simple controls                         
**  Formats:                                                     
**  [CONTROLS]                                                 
**  LINK  linkID  setting IF NODE      nodeID {BELOW/ABOVE}  level 
**  LINK  linkID  setting AT TIME      value  (units)               
**  LINK  linkID  setting AT CLOCKTIME value  (units)           
**   (0)   (1)      (2)   (3) (4)       (5)     (6)          (7)
**--------------------------------------------------------------
*/
{
   int   i = 0,                /* Node index             */
         k,                    /* Link index             */
         n;                    /* # data items           */
   char  status = ACTIVE,      /* Link status            */
         type;                 /* Link or control type   */
   double setting = MISSING,    /* Link setting           */
         time = 0.0,           /* Simulation time        */
         level = 0.0;          /* Pressure or tank level */
  
  char **Tok = m->Tok;
  Slink *Link = m->network.Link;
  Scontrol *Control = m->network.Control;
  
/* Check for sufficient number of input tokens */
   n = m->Ntokens;
   if (n < 6) {
     return(OW_ERR_SYNTAX);
   }
/* Check that controlled link exists */
   k = findlink(m,Tok[1]);
   if (k == 0) {
     return(OW_ERR_UNDEF_LINK);
   }
   type = Link[k].Type;
   if (type == CV) {
     return(OW_ERR_CONTROL_CV);         /* Cannot control check valve. */
   }
/*** Updated 9/7/00 ***/
/* Parse control setting into a status level or numerical setting. */
   if (match(Tok[2],w_OPEN))
   {
     status = OPEN;
     if (type == PUMP) {
       setting = 1.0;
     } else if (type == GPV) {
       setting = Link[k].Kc;
     }
   }
   else if (match(Tok[2],w_CLOSED))
   {
      status = CLOSED;
      if (type == PUMP) setting = 0.0;
      if (type == GPV)  setting = Link[k].Kc;
   }
   else if (type == GPV) return(206);
   else if (!getfloat(Tok[2],&setting)) return(202);

/*** Updated 3/1/01 ***/
/* Set status for pump in case speed setting was supplied */
/* or for pipe if numerical setting was supplied */

   if (type == PUMP || type == PIPE)
   {
      if (setting != MISSING)
      {
         if (setting < 0.0)       return(202);
         else if (setting == 0.0) status = CLOSED;
         else                     status = OPEN;
      }
   }

/* Determine type of control */
   if      (match(Tok[4],w_TIME))      type = TIMER;
   else if (match(Tok[4],w_CLOCKTIME)) type = TIMEOFDAY;
   else
   {
      if (n < 8) return(OW_ERR_SYNTAX);
      if ((i = findnode(m,Tok[5])) == 0) return(203);
      if      (match(Tok[6],w_BELOW)) type = LOWLEVEL;
      else if (match(Tok[6],w_ABOVE)) type = HILEVEL;
      else return(OW_ERR_SYNTAX);
   }

/* Parse control level or time */
   switch (type)
   {
      case TIMER:
      case TIMEOFDAY:
         if (n == 6) time = hour(Tok[5],"");
         if (n == 7) time = hour(Tok[5],Tok[6]);
         if (time < 0.0) return(OW_ERR_SYNTAX);
         break;
      case LOWLEVEL:
      case HILEVEL:   
         if (!getfloat(Tok[7],&level)) return(202);
         break;
   }

/* Fill in fields of control data structure */
   m->network.Ncontrols++;
  int Ncontrols = m->network.Ncontrols;
   if (Ncontrols > m->MaxControls) return(200);
   Control[Ncontrols].isEnabled = EN_ENABLE;
   Control[Ncontrols].Link     = k;
   Control[Ncontrols].Node     = i;
   Control[Ncontrols].Type     = type;
   Control[Ncontrols].Status   = status;
   Control[Ncontrols].Setting  = setting;
   Control[Ncontrols].Time     = (long)(3600.0*time);
   if (type == TIMEOFDAY)
      Control[Ncontrols].Time %= SECperDAY;
   Control[Ncontrols].Grade    = level;
   return(0);
}                        /* end of controldata */


int  sourcedata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes water quality source data                   
**  Formats:                                                     
**     [SOURCE]                                                
**        node  sourcetype  quality  (pattern start stop)
**                                                              
**  NOTE: units of mass-based source are mass/min                
**--------------------------------------------------------------
*/
{
   int   i,                  /* Token with quality value */
         j,                  /* Node index    */
         n,                  /* # data items  */
         p = 0;              /* Time pattern  */
   char  type = CONCEN;      /* Source type   */
   double c0 = 0;             /* Init. quality */
   STmplist *pat;
   Psource  source;
   char **Tok = m->Tok;
  
   n = m->Ntokens;
   if (n < 2) return(OW_ERR_SYNTAX);
   if ((j = findnode(m,Tok[0])) == 0) return(203);
   /* NOTE: Under old format, SourceType not supplied so let  */
   /*       i = index of token that contains quality value.   */
   i = 2;
   if      (match(Tok[1],w_CONCEN))    type = CONCEN;
   else if (match(Tok[1],w_MASS))      type = MASS;
   else if (match(Tok[1],w_SETPOINT))  type = SETPOINT;
   else if (match(Tok[1],w_FLOWPACED)) type = FLOWPACED;
   else i = 1;
   if (!getfloat(Tok[i],&c0)) return(202);      /* Illegal WQ value */

   if (n > i+1 && strlen(Tok[i+1]) > 0 && strcmp(Tok[i+1], "*") != 0 )         //(2.00.11 - LR)
   {
       pat = findID(Tok[i+1],m->Patlist);
       if (pat == NULL) return(205);            /* Illegal pattern. */
       p = pat->i;
   }

   source = (struct Ssource *) malloc(sizeof(struct Ssource));
   if (source == NULL) return(101);
   source->C0 = c0;
   source->Pat = p;
   source->Type = type;
   m->network.Node[j].S = source;
   return(0);
}                        /* end of sourcedata */


int  emitterdata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes junction emitter data                     
**  Format:                                                     
**     [EMITTER]                                               
**        node   K                                     
**--------------------------------------------------------------
*/
{
   int   j,                  /* Node index    */
         n;                  /* # data items  */
   double k;                  /* Flow coeff,   */
   char **Tok = m->Tok;
   n = m->Ntokens;
  
   if (n < 2) return(OW_ERR_SYNTAX);
   if ((j = findnode(m,Tok[0])) == 0) return(203);
   if (j > m->network.Njuncs) return(209);                 /* Not a junction.*/
   if (!getfloat(Tok[1],&k)) return(202);
   if (k < 0.0) return(202);
   m->network.Node[j].Ke = k;
   return(0);
}


int  qualdata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes initial water quality data                
**  Formats:
**     [QUALITY]                                               
**        node   initqual
**        node1  node2    initqual
**--------------------------------------------------------------
*/
{
   int   j,n;
   long  i,i0,i1;
   double c0;
  
  int Nnodes = m->network.Nnodes;
  Snode *Node = m->network.Node;
  char **Tok = m->Tok;
  
   if (Nnodes == 0) return(208);        /* No nodes defined yet */
   n = m->Ntokens;
   if (n < 2) return(0);
   if (n == 2)                          /* Single node entered  */
   {
      if ( (j = findnode(m,Tok[0])) == 0) return(0);
      if (!getfloat(Tok[1],&c0)) return(209);
      Node[j].C0 = c0;
   }
   else                                 /* Node range entered    */
   {
      if (!getfloat(Tok[2],&c0)) return(209);
   
      /* If numerical range supplied, then use numerical comparison */
      if ((i0 = atol(Tok[0])) > 0 && (i1 = atol(Tok[1])) > 0)
      {
         for (j=1; j<=Nnodes; j++)
         {
            i = atol(Node[j].ID);
            if (i >= i0 && i <= i1) Node[j].C0 = c0;
         }
      }
      else
      {
         for (j=1; j<=Nnodes; j++)
            if ((strcmp(Tok[0],Node[j].ID) <= 0) &&
                (strcmp(Tok[1],Node[j].ID) >= 0)
               ) Node[j].C0 = c0;
      }
   }
   return(0);
}                        /* end of qualdata */


int  reactdata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes reaction coeff. data                      
**  Formats:                                                     
**     [REACTIONS]
**        ORDER     {BULK/WALL/TANK} value                       
**        GLOBAL    BULK             coeff                                  
**        GLOBAL    WALL             coeff                                  
**        BULK      link1  (link2)   coeff                          
**        WALL      link1  (link2)   coeff                          
**        TANK      node1  (node2)   coeff                          
**        LIMITING  POTENTIAL        value                          
**        ROUGHNESS CORRELATION      value
**--------------------------------------------------------------
*/
{
   int   item,j,n;
   long  i,i1,i2;
   double y;

  Snode *Node = m->network.Node;
  Slink *Link = m->network.Link;
  Stank *Tank = m->network.Tank;
  int Njuncs = m->network.Njuncs;
  int Nnodes = m->network.Nnodes;
  int Nlinks = m->network.Nlinks;
  char **Tok = m->Tok;
  
/* Skip line if insufficient data */
   n = m->Ntokens;
   if (n < 3) return(0);

/* Process input depending on keyword */
   if (match(Tok[0],w_ORDER))                    /* Reaction order */
   {
      if (!getfloat(Tok[n-1],&y)) return(213);
      if      (match(Tok[1],w_BULK)) m->BulkOrder = y;
      else if (match(Tok[1],w_TANK)) m->TankOrder = y;
      else if (match(Tok[1],w_WALL))
      {
         if (y == 0.0) m->WallOrder = 0.0;
         else if (y == 1.0) m->WallOrder = 1.0;
         else return(213);
      }
      else return(213);
      return(0);
   }
   if (match(Tok[0],w_ROUGHNESS))                /* Roughness factor */
   {
      if (!getfloat(Tok[n-1],&y)) return(213);
      m->Rfactor = y;
      return(0);
   }
   if (match(Tok[0],w_LIMITING))                 /* Limiting potential */
   {
      if (!getfloat(Tok[n-1],&y)) return(213);
      /*if (y < 0.0) return(213);*/
      m->Climit = y;
      return(0);
   }
   if (match(Tok[0],w_GLOBAL))                   /* Global rates */
   {
      if (!getfloat(Tok[n-1],&y)) return(213);
      if      (match(Tok[1],w_BULK)) m->Kbulk = y;
      else if (match(Tok[1],w_WALL)) m->Kwall = y;
      else return(OW_ERR_SYNTAX);
      return(0);
   }
   if      (match(Tok[0],w_BULK)) item = 1;      /* Individual rates */
   else if (match(Tok[0],w_WALL)) item = 2;
   else if (match(Tok[0],w_TANK)) item = 3;
   else return(OW_ERR_SYNTAX);
   strcpy(Tok[0],Tok[1]);                        /* Save id in Tok[0] */
   if (item == 3)                                /* Tank rates */
   {
      if (!getfloat(Tok[n-1],&y)) return(209);   /* Rate coeff. */
      if (n == 3)
      {
          if ( (j = findnode(m,Tok[1])) <= Njuncs) return(0);
          Tank[j-Njuncs].Kb = y;
      }
      else
      {
       /* If numerical range supplied, then use numerical comparison */
         if ((i1 = atol(Tok[1])) > 0 && (i2 = atol(Tok[2])) > 0)
         {
            for (j=Njuncs+1; j<=Nnodes; j++)
            {
               i = atol(Node[j].ID);
               if (i >= i1 && i <= i2) Tank[j-Njuncs].Kb = y;
            }
         }
         else for (j=Njuncs+1; j<=Nnodes; j++)
            if ((strcmp(Tok[1],Node[j].ID) <= 0) &&
                (strcmp(Tok[2],Node[j].ID) >= 0)
               ) Tank[j-Njuncs].Kb = y;
      }
   }
   else                                          /* Link rates */
   {
      if (!getfloat(Tok[n-1],&y)) return(211);   /* Rate coeff. */
      if (Nlinks == 0) return(0);
      if (n == 3)                                /* Single link */
      {
         if ( (j = findlink(m,Tok[1])) == 0) return(0);
         if (item == 1) Link[j].Kb = y;
         else           Link[j].Kw = y;
      }
      else                                       /* Range of links */
      {
       /* If numerical range supplied, then use numerical comparison */
         if ((i1 = atol(Tok[1])) > 0 && (i2 = atol(Tok[2])) > 0)
         {
            for (j=1; j<=Nlinks; j++)
            {
               i = atol(Link[j].ID);
               if (i >= i1 && i <= i2)
               {
                  if (item == 1) Link[j].Kb = y;
                  else           Link[j].Kw = y;
               }
            }
         }
         else for (j=1; j<=Nlinks; j++)
            if ((strcmp(Tok[1],Link[j].ID) <= 0) &&
                (strcmp(Tok[2],Link[j].ID) >= 0) )
            {
               if (item == 1) Link[j].Kb = y;
               else           Link[j].Kw = y;
            }
      }
   }
   return(0);
}                        /* end of reactdata */


int  mixingdata(OW_Project *m)
/*
**-------------------------------------------------------------
**  Input:   none                                               
**  Output:  returns error code                                 
**  Purpose: processes tank mixing data                         
**  Format:                                                     
**    [MIXING]                                                   
**     TankID  MixModel  FractVolume
**-------------------------------------------------------------
*/
{
   int   i,j,n;
   double v;

  Stank *Tank = m->network.Tank;
  int Njuncs = m->network.Njuncs;
  int Nnodes = m->network.Nnodes;
  char **Tok = m->Tok;
  
   if (Nnodes == 0) return(208);        /* No nodes defined yet */
   n = m->Ntokens;
   if (n < 2) return(0);
   if ( (j = findnode(m,Tok[0])) <= Njuncs) return(0);
   if ( (i = findmatch(Tok[1],MixTxt)) < 0) return(OW_ERR_SYNTAX);
   v = 1.0;
   if ( (i == MIX2) &&
        (n == 3) &&
        (!getfloat(Tok[2],&v))          /* Get frac. vol. for 2COMP model */
      ) return(209);
   if (v == 0.0) v = 1.0;               /* v can't be zero */
   n = j - Njuncs;
   if (Tank[n].A == 0.0) return(0);     /* Tank is a reservoir */
   Tank[n].MixModel = (char)i;
   Tank[n].V1max = v;
   return(0);
}


int  statusdata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes link initial status data                          
**  Formats:                                                     
**    [STATUS]
**       link   value
**       link1  (link2)  value                                   
**--------------------------------------------------------------
*/
{
   int   j,n;
   long  i,i0,i1;
   double y = 0.0;
   char  status = ACTIVE;

  Slink *Link = m->network.Link;
  int Nlinks = m->network.Nlinks;
  char **Tok = m->Tok;

   if (Nlinks == 0)
     return(210);
   n = m->Ntokens - 1;
   if (n < 1)
     return(OW_ERR_SYNTAX);

/* Check for legal status setting */
  if (match(Tok[n],w_OPEN)) {
     status = OPEN;
  }
  else if (match(Tok[n],w_CLOSED)) {
     status = CLOSED;
  }
  else if (!getfloat(Tok[n],&y)) {
     return(211);
  }
  
  if (y < 0.0) {
    return(OW_ERR_ILLEGAL_VAL_LINK);
  }

/* Single link ID supplied */
   if (n == 1)
   {
      if ( (j = findlink(m,Tok[0])) == 0) {
        return(0);
      }
     
      /* Cannot change status of a Check Valve */
      if (Link[j].Type == CV) {
        return(211);
      }

/*** Updated 9/7/00 ***/      
      /* Cannot change setting for a GPV */
      if (Link[j].Type == GPV &&  status == ACTIVE) {
        return(211);
      }

      changestatus(m,j,status,y);
   }

/* Range of ID's supplied */
   else
   {
      /* Numerical range supplied */
      if ((i0 = atol(Tok[0])) > 0 && (i1 = atol(Tok[1])) > 0)
      {
         for (j=1; j<=Nlinks; j++)
         {
            i = atol(Link[j].ID);
            if (i >= i0 && i <= i1) {
             changestatus(m,j,status,y);
            }
         }
      }
      else
         for (j=1; j<=Nlinks; j++)
            if ( (strcmp(Tok[0],Link[j].ID) <= 0) &&
                 (strcmp(Tok[1],Link[j].ID) >= 0)
               ) changestatus(m,j,status,y);
   }
   return(0);
}              /* end of statusdata */


int  energydata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes pump energy data                          
**  Formats:                                                     
**    [ENERGY]                                                   
**       GLOBAL         {PRICE/PATTERN/EFFIC}  value                 
**       PUMP   id      {PRICE/PATTERN/EFFIC}  value                 
**       DEMAND CHARGE  value                                    
**--------------------------------------------------------------
*/
{
   int j,k,n;
   double y;
   STmplist *t;

  Slink *Link = m->network.Link;
  Spump *Pump = m->network.Pump;
  STmplist *Patlist = m->Patlist;
  STmplist *Curvelist = m->Curvelist;
  char **Tok = m->Tok;

  
/* Check for sufficient data */
   n = m->Ntokens;
   if (n < 3) return(OW_ERR_SYNTAX);

/* Check first keyword */
   if (match(Tok[0],w_DMNDCHARGE))               /* Demand charge */
   {
      if (!getfloat(Tok[2], &y)) return(213);
      m->Dcost = y;
      return(0);
   }
   if (match(Tok[0],w_GLOBAL))                   /* Global parameter */
   {
      j = 0;
   }
   else if (match(Tok[0],w_PUMP))                /* Pump-specific parameter */
   {
      if (n < 4) return(OW_ERR_SYNTAX);
      k = findlink(m,Tok[1]);                      /* Check that pump exists */
      if (k == 0) return(216);
      if (Link[k].Type != PUMP) return(216);
      j = Link[k].pumpLinkIdx;
   }
   else return(OW_ERR_SYNTAX);

/* Find type of energy parameter */      
   if (match(Tok[n-2],w_PRICE))                  /* Energy price */
   {
      if (!getfloat(Tok[n-1],&y))
      {
         if (j == 0) return(213);
         else return(217);
      }
      if (j == 0) m->Ecost = y;
      else Pump[j].Ecost = y;
      return(0);
   }    
   else if (match(Tok[n-2],w_PATTERN))           /* Price pattern */
   {
      t = findID(Tok[n-1],Patlist);              /* Check if pattern exists */
      if (t == NULL)
      {
         if (j == 0) return(213);
         else return(217);
      }
      if (j == 0) m->Epat = t->i;
      else Pump[j].Epat = t->i;
      return(0);
   }
   else if (match(Tok[n-2],w_EFFIC))             /* Pump efficiency */
   {
      if (j == 0)
      {
         if (!getfloat(Tok[n-1], &y)) return(213);
         if (y <= 0.0) return(213);
         m->Epump = y;
      }
      else
      {
         t = findID(Tok[n-1],Curvelist);         /* Check if curve exists */ 
         if (t == NULL) return(217);
         Pump[j].Ecurve = t->i;
      }
      return(0);
   }
   return(OW_ERR_SYNTAX);
}


int  reportdata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes report options data                       
**  Formats:                                                     
**    PAGE     linesperpage                                      
**    STATUS   {NONE/YES/FULL}
**    SUMMARY  {YES/NO}
**    MESSAGES {YES/NO}
**    ENERGY   {NO/YES}                                   
**    NODES    {NONE/ALL}                                        
**    NODES    node1  node2 ...                                  
**    LINKS    {NONE/ALL}                                        
**    LINKS    link1  link2 ...                                  
**    FILE     filename
**    variable {YES/NO}                                          
**    variable {BELOW/ABOVE/PRECISION}  value 
**--------------------------------------------------------------
*/
{
   int    i,j,n;
   double  y;

  SField *Field = m->Field;
  char **Tok = m->Tok;

   n = m->Ntokens - 1;
   if (n < 1) return(OW_ERR_SYNTAX);

/* Value for page size */
   if (match(Tok[0],w_PAGE))
   {
      if (!getfloat(Tok[n],&y))   return(213);
      if (y < 0.0 || y > 255.0) return(213);
      m->PageSize = (int) y;
      return(0);
   }

/* Request that status reports be written */
   if (match(Tok[0],w_STATUS))
   {
      if (match(Tok[n],w_NO))   m->Statflag = FALSE;
      if (match(Tok[n],w_YES))  m->Statflag = TRUE;
      if (match(Tok[n],w_FULL)) m->Statflag = FULL;
      return(0);
   }

/* Request summary report */
   if (match(Tok[0],w_SUMMARY))
   {
      if (match(Tok[n],w_NO))  m->Summaryflag = FALSE;
      if (match(Tok[n],w_YES)) m->Summaryflag = TRUE;
      return(0);
   }

/* Request error/warning message reporting */
   if (match(Tok[0],w_MESSAGES))
   {
      if (match(Tok[n],w_NO))  m->Messageflag = FALSE;
      if (match(Tok[n],w_YES)) m->Messageflag = TRUE;
      return(0);
   }
   

/* Request an energy usage report */
   if (match(Tok[0],w_ENERGY))
   {
      if (match(Tok[n],w_NO))  m->Energyflag = FALSE;
      if (match(Tok[n],w_YES)) m->Energyflag = TRUE;
      return(0);
   }

/* Particular reporting nodes specified */
   if (match(Tok[0],w_NODE))
   {
      if      (match(Tok[n],w_NONE)) m->Nodeflag = 0;  /* No nodes */
      else if (match(Tok[n],w_ALL))  m->Nodeflag = 1;  /* All nodes */
      else
      {
         if (m->network.Nnodes == 0) return(208);
         for (i=1; i<=n; i++)
         {
            if ( (j = findnode(m,Tok[i])) == 0) return(208);
            m->network.Node[j].Rpt = 1;
         }
         m->Nodeflag = 2;
      }
      return(0);
   }

/* Particular reporting links specified */
   if (match(Tok[0],w_LINK))
   {
      if      (match(Tok[n],w_NONE)) m->Linkflag = 0;
      else if (match(Tok[n],w_ALL))  m->Linkflag = 1;
      else
      {
         if (m->network.Nlinks == 0) return(210);
         for (i=1; i<=n; i++)
         {
            if ( (j = findlink(m,Tok[i])) == 0) return(210);
            m->network.Link[j].Rpt = 1;
         }
         m->Linkflag = 2;
      }
      return(0);
   }

/* Check if input is a reporting criterion. */

/*** Special case needed to distinguish "HEAD" from "HEADLOSS" ***/            //(2.00.11 - LR)
   if (strcomp(Tok[0], w_HEADLOSS)) i = HEADLOSS;                              //(2.00.11 - LR)
   else i = findmatch(Tok[0],Fldname);                                         //(2.00.11 - LR)
   if (i >= 0)                                                                 //(2.00.11 - LR)
/*****************************************************************/            //(2.00.11 - LR)
   {
      if (i > FRICTION) return(OW_ERR_SYNTAX);
      if (m->Ntokens == 1 || match(Tok[1],w_YES))
      {
         Field[i].Enabled = TRUE;
         return(0);
      }
      if (match(Tok[1],w_NO))
      {
         Field[i].Enabled = FALSE;
         return(0);
      }
      if (m->Ntokens < 3) return(OW_ERR_SYNTAX);
      if      (match(Tok[1],w_BELOW))  j = LOW;   /* Get relation operator */
      else if (match(Tok[1],w_ABOVE))  j = HI;    /* or precision keyword  */
      else if (match(Tok[1],w_PRECISION)) j = PREC;
      else return(OW_ERR_SYNTAX);
      if (!getfloat(Tok[2],&y)) return(OW_ERR_SYNTAX);
      if (j == PREC)
      {
         Field[i].Enabled = TRUE;
         Field[i].Precision = ROUND(y);
      }
      else Field[i].RptLim[j] = y;                /* Report limit value */
      return(0);
   }

/* Name of external report file */
   if (match(Tok[0],w_FILE))
   {
      strncpy(m->Rpt2Fname,Tok[1],MAXFNAME);
      return(0);
   }

/* If get to here then return error condition */
   return(OW_ERR_SYNTAX);
}                        /* end of reportdata */


int  timedata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes time options data                         
**  Formats:                                                     
**    STATISTIC                  {NONE/AVERAGE/MIN/MAX/RANGE}                                          
**    DURATION                   value   (units)                 
**    HYDRAULIC TIMESTEP         value   (units)                 
**    QUALITY TIMESTEP           value   (units)                 
**    MINIMUM TRAVELTIME         value   (units)
**    RULE TIMESTEP              value   (units)                 
**    PATTERN TIMESTEP           value   (units)
**    PATTERN START              value   (units)              
**    REPORT TIMESTEP            value   (units)                 
**    REPORT START               value   (units)                 
**    START CLOCKTIME            value   (AM PM)
**-------------------------------------------------------------
*/
{
   int    n;
   long   t;
   double  y;
  char **Tok = m->Tok;

  
   n = m->Ntokens - 1;
   if (n < 1) return(OW_ERR_SYNTAX);

/* Check if setting time statistic flag */
   if (match(Tok[0],w_STATISTIC))
   {
      if      (match(Tok[n],w_NONE))  m->Tstatflag = SERIES;
      else if (match(Tok[n],w_NO))    m->Tstatflag = SERIES;
      else if (match(Tok[n],w_AVG))   m->Tstatflag = AVG;
      else if (match(Tok[n],w_MIN))   m->Tstatflag = MIN;
      else if (match(Tok[n],w_MAX))   m->Tstatflag = MAX;
      else if (match(Tok[n],w_RANGE)) m->Tstatflag = RANGE;
      else return(OW_ERR_SYNTAX);
      return(0);
   }

/* Convert text time value to numerical value in seconds */
/* Examples:
**    5           = 5 * 3600 sec
**    5 MINUTES   = 5 * 60   sec
**    13:50       = 13*3600 + 50*60 sec
**    1:50 pm     = (12+1)*3600 + 50*60 sec
*/
     
   if (!getfloat(Tok[n],&y))
   {
      if ( (y = hour(Tok[n],"")) < 0.0)
      {
         if ( (y = hour(Tok[n-1],Tok[n])) < 0.0) return(213);
      }
   }
   t = (long)(3600.0*y+0.5);
/* Process the value assigned to the matched parameter */
   if      (match(Tok[0],w_DURATION))  m->Dur = t;      /* Simulation duration */
   else if (match(Tok[0],w_HYDRAULIC)) m->Hstep = t;    /* Hydraulic time step */
   else if (match(Tok[0],w_QUALITY))   m->Qstep = t;    /* Quality time step   */
   else if (match(Tok[0],w_RULE))      m->Rulestep = t; /* Rule time step      */
   else if (match(Tok[0],w_MINIMUM))   return(0);    /* Not used anymore    */
   else if (match(Tok[0],w_PATTERN))
   {
      if (match(Tok[1],w_TIME))       m->Pstep = t;     /* Pattern time step   */
      else if (match(Tok[1],w_START)) m->Pstart = t;    /* Pattern start time  */
      else return(OW_ERR_SYNTAX);
   }
   else if (match(Tok[0],w_REPORT))
   {
      if      (match(Tok[1],w_TIME))  m->Rstep = t;     /* Reporting time step  */
      else if (match(Tok[1],w_START)) m->Rstart = t;    /* Reporting start time */
      else return(OW_ERR_SYNTAX);
   }                                                 /* Simulation start time*/
   else if (match(Tok[0],w_START))    m->Tstart = t % SECperDAY;
   else return(OW_ERR_SYNTAX);
   return(0);
}                        /* end of timedata */


int  optiondata(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: processes [OPTIONS] data                            
**--------------------------------------------------------------
*/
{
   int i,n;

   n = m->Ntokens - 1;
   i = optionchoice(m,n);         /* Option is a named choice    */
   if (i >= 0) return(i);
   return(optionvalue(m,n));      /* Option is a numerical value */
}                        /* end of optiondata */


int  optionchoice(OW_Project *m, int n)
/*
**--------------------------------------------------------------
**  Input:   n = index of last input token saved in Tok[]          
**  Output:  returns error code or 0 if option belongs to        
**           those listed below, or -1 otherwise                 
**  Purpose: processes fixed choice [OPTIONS] data               
**  Formats:                                                     
**    UNITS               CFS/GPM/MGD/IMGD/AFD/LPS/LPM/MLD/CMH/CMD/SI
**    PRESSURE            PSI/KPA/M                          
**    HEADLOSS            H-W/D-W/C-M                        
**    HYDRAULICS          USE/SAVE  filename                  
**    QUALITY             NONE/AGE/TRACE/CHEMICAL  (TraceNode) 
**    MAP                 filename                               
**    VERIFY              filename                               
**    UNBALANCED          STOP/CONTINUE {Niter}
**    PATTERN             id
**--------------------------------------------------------------
*/
{
  char **Tok = m->Tok;

  /* Check if 1st token matches a parameter name and */
  /* process the input for the matched parameter     */
   if (n < 0) return(OW_ERR_SYNTAX);
   if (match(Tok[0],w_UNITS))
   {
      if (n < 1) return(0);
      else if (match(Tok[1],w_CFS))  m->Flowflag = CFS;
      else if (match(Tok[1],w_GPM))  m->Flowflag = GPM;
      else if (match(Tok[1],w_AFD))  m->Flowflag = AFD;
      else if (match(Tok[1],w_MGD))  m->Flowflag = MGD;
      else if (match(Tok[1],w_IMGD)) m->Flowflag = IMGD;
      else if (match(Tok[1],w_LPS))  m->Flowflag = LPS;
      else if (match(Tok[1],w_LPM))  m->Flowflag = LPM;
      else if (match(Tok[1],w_CMH))  m->Flowflag = CMH;
      else if (match(Tok[1],w_CMD))  m->Flowflag = CMD;
      else if (match(Tok[1],w_MLD))  m->Flowflag = MLD;
      else if (match(Tok[1],w_SI))   m->Flowflag = LPS;
      else return(OW_ERR_SYNTAX);
   }
   else if (match(Tok[0],w_PRESSURE))
   {
      if (n < 1) return(0);
      else if (match(Tok[1],w_PSI))    m->Pressflag = PSI;
      else if (match(Tok[1],w_KPA))    m->Pressflag = KPA;
      else if (match(Tok[1],w_METERS)) m->Pressflag = METERS;
      else return(OW_ERR_SYNTAX);
   }
   else if (match(Tok[0],w_HEADLOSS))
   {
      if (n < 1) return(0);
      else if (match(Tok[1],w_HW)) m->Formflag = HW;
      else if (match(Tok[1],w_DW)) m->Formflag = DW;
      else if (match(Tok[1],w_CM)) m->Formflag = CM;
      else return(OW_ERR_SYNTAX);
   }
   else if (match(Tok[0],w_HYDRAULIC))
   {
      if (n < 2) return(0);
      else if (match(Tok[1],w_USE))  m->Hydflag = USE;
      else if (match(Tok[1],w_SAVE)) m->Hydflag = SAVE;
      else return(OW_ERR_SYNTAX);
      strncpy(m->HydFname,Tok[2],MAXFNAME);
   }
   else if (match(Tok[0],w_QUALITY))
   {
      if (n < 1) return(0);
      else if (match(Tok[1],w_NONE))  m->Qualflag = NONE;
      else if (match(Tok[1],w_CHEM))  m->Qualflag = CHEM;
      else if (match(Tok[1],w_AGE))   m->Qualflag = AGE;
      else if (match(Tok[1],w_TRACE)) m->Qualflag = TRACE;
      else
      {
         m->Qualflag = CHEM;
         strncpy(m->ChemName,Tok[1],MAXID);
         if (n >= 2) strncpy(m->ChemUnits,Tok[2],MAXID);
      }
      if (m->Qualflag == TRACE)                  /* Source tracing option */
      {
      /* Copy Trace Node ID to Tok[0] for error reporting */
         strcpy(Tok[0],"");
         if (n < 2) return(212);
         strcpy(Tok[0],Tok[2]);
         m->TraceNode = findnode(m,Tok[2]);
         if (m->TraceNode == 0) return(212);
         strncpy(m->ChemName,u_PERCENT,MAXID);
         strncpy(m->ChemUnits,Tok[2],MAXID);
      }
      if (m->Qualflag == AGE)
      {
         strncpy(m->ChemName,w_AGE,MAXID);
         strncpy(m->ChemUnits,u_HOURS,MAXID);
      }
   }
   else if (match(Tok[0],w_MAP))
   {
      if (n < 1) return(0);
      strncpy(m->MapFname,Tok[1],MAXFNAME);        /* Map file name */
   }
   else if (match(Tok[0],w_VERIFY))
   {
      /* Backward compatibility for verification file */
   }
   else if (match(Tok[0],w_UNBALANCED))         /* Unbalanced option */
   {
      if (n < 1) return(0);
      if (match(Tok[1],w_STOP)) m->ExtraIter = -1;
      else if (match(Tok[1],w_CONTINUE))
      {
         if (n >= 2) m->ExtraIter = atoi(Tok[2]);
         else m->ExtraIter = 0;
      }
      else return(OW_ERR_SYNTAX);
   }
   else if (match(Tok[0],w_PATTERN))            /* Pattern option */
   {
      if (n < 1) return(0);
      strncpy(m->DefPatID,Tok[1],MAXID);
   }
   else return(-1);
   return(0);
}                        /* end of optionchoice */


int  optionvalue(OW_Project *m, int n)
/*
**------------------------------------------------------------- 
**  Input:   *line = line read from input file                   
**  Output:  returns error code                                  
**  Purpose: processes numerical value [OPTIONS] data            
**  Formats:
**    DEMAND MULTIPLIER   value
**    EMITTER EXPONENT    value                                                     
**    VISCOSITY           value                                  
**    DIFFUSIVITY         value                                  
**    SPECIFIC GRAVITY    value                                  
**    TRIALS              value                                  
**    ACCURACY            value                                  
**    TOLERANCE           value                                  
**    SEGMENTS            value  (not used)                                 
**  ------ Undocumented Options -----                            
**    HTOL                value                                  
**    QTOL                value                                  
**    RQTOL               value                                  
**    CHECKFREQ           value                                  
**    MAXCHECK            value
**    DAMPLIMIT           value                                                //(2.00.12 - LR)                                  
**--------------------------------------------------------------
*/
{
   int    nvalue = 1;   /* Index of token with numerical value */
   double  y;
  char **Tok = m->Tok;


/* Check for obsolete SEGMENTS keyword */
   if (match(Tok[0],w_SEGMENTS)) return(0);

/* Check for missing value (which is permissible) */
   if (match(Tok[0],w_SPECGRAV) || match(Tok[0],w_EMITTER)
   || match(Tok[0],w_DEMAND)) nvalue = 2;
   if (n < nvalue) return(0);

/* Check for valid numerical input */
   if (!getfloat(Tok[nvalue],&y)) return(213);

/* Check for WQ tolerance option (which can be 0) */
   if (match(Tok[0],w_TOLERANCE))
   {
      if (y < 0.0) return(213);
      m->Ctol = y;         /* Quality tolerance*/
      return(0);
   }

/* Check for Diffusivity option */
   if (match(Tok[0],w_DIFFUSIVITY))
   {
      if (y < 0.0) return(213);
      m->Diffus = y;
      return(0);
   }

/* Check for Damping Limit option */                                           //(2.00.12 - LR)
   if (match(Tok[0],w_DAMPLIMIT))
   {
      m->DampLimit = y;
      return(0);
   }

/* All other options must be > 0 */
   if (y <= 0.0) return(213);

/* Assign value to specified option */
   if      (match(Tok[0],w_VISCOSITY))   m->Viscos = y;       /* Viscosity */
   else if (match(Tok[0],w_SPECGRAV))    m->SpGrav = y;       /* Spec. gravity */
   else if (match(Tok[0],w_TRIALS))      m->MaxIter = (int)y; /* Max. trials */
   else if (match(Tok[0],w_ACCURACY))                      /* Accuracy */
   {
      y = MAX(y,1.e-5);                                  
      y = MIN(y,1.e-1);
      m->Hacc = y;
   }
   else if (match(Tok[0],w_HTOL))        m->Htol = y;
   else if (match(Tok[0],w_QTOL))        m->Qtol = y;
   else if (match(Tok[0],w_RQTOL))
   {
      if (y >= 1.0) return(213);
      m->RQtol = y;
   }
   else if (match(Tok[0],w_CHECKFREQ))   m->CheckFreq = (int)y;
   else if (match(Tok[0],w_MAXCHECK))    m->MaxCheck = (int)y;
   else if (match(Tok[0],w_EMITTER))     m->Qexp = 1.0/y;
   else if (match(Tok[0],w_DEMAND))      m->Dmult = y;
   else return(OW_ERR_SYNTAX);
   return(0);
}                        /* end of optionvalue */


int  getpumpcurve(OW_Project *m, int n)
/*
**--------------------------------------------------------
**  Input:   n = number of parameters for pump curve
**  Output:  returns error code
**  Purpose: processes pump curve data for Version 1.1-
**           style input data
**  Notes:
**    1. Called by pumpdata() in INPUT3.C
**    2. Current link index & pump index of pump being
**       processed is found in global variables Nlinks
**       and Npumps, respectively
**    3. Curve data read from input line is found in
**       global variables X[0],...X[n-1]
**---------------------------------------------------------
*/
{
   double a,b,c,h0,h1,h2,q1,q2;

  int Npumps = m->network.Npumps;
  
   if (n == 1)                /* Const. HP curve       */
   {
      if (m->X[0] <= 0.0) return(202);
      m->network.Pump[Npumps].Ptype = CONST_HP;
      m->network.Link[m->network.Nlinks].Km = m->X[0];
   }
   else
   {
      if (n == 2)             /* Generic power curve   */
      {
         q1 = m->X[1];
         h1 = m->X[0];
         h0 = 1.33334*h1;
         q2 = 2.0*q1;
         h2 = 0.0;
      }
      else if (n >= 5)        /* 3-pt. power curve     */
      {
         h0 = m->X[0];
         h1 = m->X[1];
         q1 = m->X[2];
         h2 = m->X[3];
         q2 = m->X[4];
      }
      else return(202);
      m->network.Pump[Npumps].Ptype = POWER_FUNC;
      if (!powercurve(h0,h1,h2,q1,q2,&a,&b,&c)) return(206);
      m->network.Pump[Npumps].H0 = -a;
      m->network.Pump[Npumps].R  = -b;
      m->network.Pump[Npumps].N  = c;
      m->network.Pump[Npumps].Q0 = q1;
      m->network.Pump[Npumps].Qmax  = pow((-a/b),(1.0/c));
      m->network.Pump[Npumps].Hmax  = h0;
   }
   return(0);
}


int  powercurve(double h0, double h1, double h2, double q1,
                double q2, double *a, double *b, double *c)
/*
**---------------------------------------------------------
**  Input:   h0 = shutoff head
**           h1 = design head
**           h2 = head at max. flow
**           q1 = design flow
**           q2 = max. flow
**  Output:  *a, *b, *c = pump curve coeffs. (H = a-bQ^c),
**           Returns 1 if sucessful, 0 otherwise.
**  Purpose: computes coeffs. for pump curve
**----------------------------------------------------------
*/
{
    double h4,h5;
    if (
          h0      < TINY ||
          h0 - h1 < TINY ||
          h1 - h2 < TINY ||
          q1      < TINY ||
          q2 - q1 < TINY
                           ) return(0);
    *a = h0;
    h4 = h0 - h1;
    h5 = h0 - h2;
    *c = log(h5/h4)/log(q2/q1);
    if (*c <= 0.0 || *c > 20.0) return(0);
    *b = -h4/pow(q1,*c);

    /*** Updated 6/24/02 ***/
    if (*b >= 0.0) return(0);

    return(1);
}


int  valvecheck(OW_Project *m, int type, int j1, int j2)
/*
**--------------------------------------------------------------
**  Input:   type = valve type                                   
**           j1   = index of upstream node                       
**           j2   = index of downstream node                     
**  Output:  returns 1 for legal connection, 0 otherwise         
**  Purpose: checks for legal connections between PRVs & PSVs    
**--------------------------------------------------------------
*/
{
   int  k, vk, vj1, vj2, vtype;

  Slink *Link = m->network.Link;
  Svalve *Valve = m->network.Valve;
  int Nvalves = m->network.Nvalves;
  
   /* Examine each existing valve */
   for (k=1; k <= Nvalves; k++)
   {
      vk = Valve[k].Link;
      vj1 = Link[vk].N1;
      vj2 = Link[vk].N2;
      vtype = Link[vk].Type;

      /* Cannot have two PRVs sharing downstream nodes or in series */
      if (vtype == PRV && type == PRV)
      {
         if (vj2 == j2 ||
             vj2 == j1 ||
             vj1 == j2   ) return(0);
      }

      /* Cannot have two PSVs sharing upstream nodes or in series */
      if (vtype == PSV && type == PSV)
      {
         if (vj1 == j1 ||
             vj1 == j2 ||
             vj2 == j1   ) return(0);
      }

      /* Cannot have PSV connected to downstream node of PRV */
      if (vtype == PSV && type == PRV && vj1 == j2) return(0);
      if (vtype == PRV && type == PSV && vj2 == j1) return(0);

/*** Updated 3/1/01 ***/
      /* Cannot have PSV connected to downstream node of FCV */
      /* nor have PRV connected to upstream node of FCV */
      if (vtype == FCV && type == PSV && vj2 == j1) return(0);
      if (vtype == FCV && type == PRV && vj1 == j2) return(0);

/*** Updated 4/14/05 ***/
      if (vtype == PSV && type == FCV && vj1 == j2) return (0);
      if (vtype == PRV && type == FCV && vj2 == j1) return (0);
   }
   return(1);
}                   /* End of valvecheck */


void  changestatus(OW_Project *m, int j, char status, double y)
/*
**--------------------------------------------------------------
**  Input:   j      = link index                                   
**           status = status setting (OPEN, CLOSED)                       
**           y      = numerical setting (pump speed, valve
**                    setting)
**  Output:  none
**  Purpose: changes status or setting of a link
**
**  NOTE: If status = ACTIVE, then a numerical setting (y) was
**        supplied. If status = OPEN/CLOSED, then numerical
**        setting is 0.
**--------------------------------------------------------------
*/
{
  Slink *Link = m->network.Link;
  
   if (Link[j].Type == PIPE || Link[j].Type == GPV) {
     if (status != ACTIVE) {
        Link[j].Stat = status;
     }
   }
   else if (Link[j].Type == PUMP) {
      if (status == ACTIVE) {
         Link[j].Kc = y;
         status = OPEN;
         if (y == 0.0) {
           status = CLOSED;
         }
      }
      else if (status == OPEN) {
        Link[j].Kc = 1.0;
      }
      Link[j].Stat = status;
   }
   else if (Link[j].Type >= PRV) {
      Link[j].Kc = y;
      Link[j].Stat = status;
      if (status != ACTIVE) {
        Link[j].Kc = MISSING;
      }
   }
}                        /* end of changestatus */

/********************** END OF INPUT3.C ************************/

