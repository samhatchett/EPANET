//
//  networkBuilder.c
//  epanet
//
//  Created by Sam Hatchett on 10/21/14.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "epanet2.h"
#include "types.h"


extern void initpointers(OW_Project *m); // epanet.c
extern int  allocdata(OW_Project *m);
extern void setdefaults(OW_Project *m);
extern int  addpattern(OW_Project *m, char *id);
extern void adjustdata(OW_Project *m);
extern void initunits(OW_Project *m);

int DLLEXPORT OW_newNetwork(OW_Project **modelObj)
{
  
  *modelObj = 0;
  OW_Project *m = calloc(1, sizeof(OW_Project));
  
  
  /* Set system flags */
  m->Openflag  = TRUE;
  m->OpenHflag = FALSE;
  m->OpenQflag = FALSE;
  m->SaveHflag = FALSE;
  m->SaveQflag = FALSE;
  m->Warnflag  = FALSE;
  
  m->Messageflag = TRUE;
  m->Rptflag = 1;
  
  /* Initialize global pointers to NULL. */
  initpointers(m);
  allocdata(m);
  setdefaults(m);
  initunits(m);
  
  m->MaxPats = -1;
  addpattern(m,"");
  m->Pattern[0].Length = 1;
  m->Pattern[0].F = (double *) calloc(1, sizeof(double));
  m->Pattern[0].F[0] = 1.0;
  
  *modelObj = m;
  return EN_OK;
}

// network creation api set
int DLLEXPORT OW_startEditingNetwork(OW_Project *modelObj)
{
  
}

int DLLEXPORT OW_addNode(OW_Project *m, EN_NodeType type, char *name)
{
  
  Pdemand  demand;
  
  ///////////////////////////////
  // add the node ID
  int index;
  OW_getnodeindex(m, name, &index);
  if (index != 0) {
    return 203;
  }
  
  m->Nnodes++;
  int n_idx = m->Nnodes;
  
  // allocate a new node
  
  int newMemSize = n_idx + 1;
  m->Node = realloc(m->Node, newMemSize * sizeof(Snode));
  m->NodeQual = realloc(m->NodeQual, newMemSize * sizeof(double));
  m->hydraulics.NodeDemand = realloc(m->hydraulics.NodeDemand, newMemSize * sizeof(double));
  m->hydraulics.NodeHead = realloc(m->hydraulics.NodeHead, newMemSize * sizeof(double));
  
  Snode *node = &(m->Node[n_idx]);
  /* initialize junction data */
  
  strncpy(node->ID, name, MAXID);
  ENHashTableInsert(m->NodeHashTable, node->ID, n_idx);        /* see HASH.C */
  node->El  = 0.0;
  node->C0  = 0.0;
  node->S   = NULL;
  node->Ke  = 0.0;
  node->Rpt = 0;
  
  /* Create a new demand record */
  demand = (struct Sdemand *) malloc(sizeof(struct Sdemand));
  if (demand == NULL) {
    return(101);
  }
  demand->Base = 0;
  demand->Pat = 0;
  demand->next = NULL;
  node->D = demand;
  m->hydraulics.NodeDemand[n_idx] = 0;
  
  if (type == EN_JUNCTION) {
    m->Njuncs++;
  }
  if (type == EN_TANK || type == EN_RESERVOIR) {
    
    m->Ntanks++;
    // allocate new tank
    m->Tank = realloc(m->Tank, (m->Ntanks + 1) * sizeof(Stank));
    
    int iTank = m->Ntanks;
    Stank *Tank = m->Tank;
    
    Tank[iTank].Node     = n_idx;    /* Node index.          */
    Tank[iTank].H0       = 0;        /* Init. level.         */
    Tank[iTank].Hmin     = 0;        /* Min. level.          */
    Tank[iTank].Hmax     = 0;        /* Max level.           */
    Tank[iTank].A        = (type == EN_TANK ) ? 1.0 : 0.0; // reservoirs are known by their zero area.
    Tank[iTank].Pat      = 0;        /* Fixed grade pattern. */
    Tank[iTank].Kb       = MISSING;  /* Reaction coeff.      */
    Tank[iTank].Vmin = 0;
    Tank[iTank].V0 = 0;
    Tank[iTank].Vmax = 0;
    Tank[iTank].Vcurve   = 0;           /* Volume curve         */
    Tank[iTank].MixModel = MIX1;             /* Completely mixed     */
    Tank[iTank].V1max    = 1.0;              /* Compart. size ratio  */
  }
  
  
  
  
  return EN_OK;
  
}

int DLLEXPORT OW_addLink(OW_Project *m, EN_LinkType type, char *name, char *upstreamNode, char* downstreamNode)
{
  
  int linkIndex;
  OW_getlinkindex(m, name, &linkIndex);
  if (linkIndex != 0) {
    return 203;
  }
  
  m->Nlinks++;
  int l_idx = m->Nlinks;
  
  // allocate new link
  
  int newMemSize = l_idx + 1;
  m->Link = realloc(m->Link, newMemSize * sizeof(Slink));
  m->hydraulics.LinkFlows = realloc(m->hydraulics.LinkFlows, newMemSize * sizeof(double));
  m->hydraulics.LinkSetting = realloc(m->hydraulics.LinkSetting, newMemSize * sizeof(double));
  m->hydraulics.LinkStatus = realloc(m->hydraulics.LinkStatus, newMemSize * sizeof(char));
  
  Slink *link = &(m->Link[l_idx]);
  
  // find start/end link
  int n1_idx, n2_idx;
  OW_getnodeindex(m, upstreamNode, &n1_idx);
  OW_getnodeindex(m, downstreamNode, &n2_idx);
  
  strncpy(link->ID, name, MAXID);
  ENHashTableInsert(m->LinkHashTable, link->ID, l_idx);        /* see HASH.C */
  link->N1 = n1_idx;
  link->N2 = n2_idx;
  link->Len = 0;
  link->Diam = 0;
  link->Kc = 0;
  link->Km = 0;
  link->Kb = 0;
  link->Kw = 0;
  link->Type = type;
  link->Stat = OPEN;
  
  m->hydraulics.LinkStatus[l_idx] = m->Link[l_idx].Stat;
  
  
  if (type == EN_PIPE) {
    //
  }
  else if (type == EN_PUMP) {
    m->Npumps++;
    int pump_idx = m->Npumps;
    m->Pump = realloc(m->Pump, (pump_idx + 1) * sizeof(Spump));
    
    Spump *pump = &(m->Pump[pump_idx]);
    pump->Link = l_idx;
    pump->Ptype = NOCURVE;
    pump->Hcurve = 0;
    pump->Ecurve = 0;
    pump->Upat = 0;
    pump->Ecost = 0;
    pump->Epat = 0;
  }
  else /* it's a valve */ {
    m->Nvalves++;
    int valve_idx = m->Nvalves;
    m->Valve = realloc(m->Valve, (valve_idx + 1) * sizeof(Svalve));
    Svalve *valve = &(m->Valve[valve_idx]);
    valve->Link = l_idx;
  }
  
  return EN_OK;
  
}

int DLLEXPORT OW_stopEditingNetwork(OW_Project *m)
{
  
  adjustdata(m); // fill in missing data with defaults
}