/*
*******************************************************************************
                                                                      
QUALITY.C -- Water Quality Simulator for EPANET Program         
                                                                      
VERSION:    2.00
DATE:       5/29/00
            9/7/00
            10/25/00
            8/15/07    (2.00.11)
            2/14/08    (2.00.12)
AUTHOR:     L. Rossman
            US EPA - NRMRL
                                                                      
  This module contains the network water quality simulator.           
                                                                      
  For each time period, hydraulic results are read in from the        
  binary file HydFile, hydraulic and water quality results are        
  written to the binary output file OutFile (if the current period    
  is a reporting period), and the water quality is transported
  and reacted over the duration of the time period.                                      

  The entry points for this module are:
    openqual()   -- called from ENopenQ() in EPANET.C
    initqual()   -- called from ENinitQ() in EPANET.C
    runqual()    -- called from ENrunQ() in EPANET.C
    nextqual()   -- called from ENnextQ() in EPANET.C
    stepqual()   -- called from ENstepQ() in EPANET.C
    closequal()  -- called from ENcloseQ() in EPANET.C
                                                                      
  Calls are made to:
    AllocInit()
    Alloc()
    AllocFree()   
  in MEMPOOL.C to utilize a memory pool to prevent excessive malloc'ing  
  when constantly creating and destroying pipe sub-segments during    
  the water quality transport calculations.

  Calls are also made to:
    readhyd()
    readhydstep()
    savenetdata()
    saveoutput()
    savefinaloutput()
  in OUTPUT.C to retrieve hydraulic results and save all results.

******************************************************************************* 
*/

#include <stdio.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <math.h>
#include "hash.h"
#include "text.h"
#include "epanet2.h"
#include "types.h"
#include "funcs.h"
#define  EXTERN  extern
#include "vars.h"
#include "mempool.h"

/*
** Macros to identify upstream & downstream nodes of a link
** under the current flow and to compute link volume
*/
//#define   UP_NODE(x)   ( (FlowDir[(x)]=='+') ? Link[(x)].N1 : Link[(x)].N2 )
//#define   DOWN_NODE(x) ( (FlowDir[(x)]=='+') ? Link[(x)].N2 : Link[(x)].N1 )
//#define   LINKVOL(k)   ( 0.785398*Link[(k)].Len*SQR(Link[(k)].Diam) )
// these defs are deprecated. see below for replacements

//Pseg      FreeSeg;              /* Pointer to unused segment               */
//Pseg      *FirstSeg,            /* First (downstream) segment in each pipe */
//          *LastSeg;             /* Last (upstream) segment in each pipe    */
//char      *FlowDir;             /* Flow direction for each pipe            */
//double    *VolIn;               /* Total volume inflow to node             */
//double    *MassIn;              /* Total mass inflow to node               */
//double    Sc;                   /* Schmidt Number                          */
//double    Bucf;                 /* Bulk reaction units conversion factor   */
//double    Tucf;                 /* Tank reaction units conversion factor   */
//
///*** Moved to vars.h ***/                                                      //(2.00.12 - LR)
////char      Reactflag;            /* Reaction indicator                      */
//
//char      OutOfMemory;          /* Out of memory indicator                 */
//static    alloc_handle_t *SegPool; // Memory pool for water quality segments   //(2.00.11 - LR)

// replacing defs with some real functions that reference the model wrapper.
int UP_NODE(OW_Model *m, int iLink) {
  return ( (m->FlowDir[(iLink)]=='+') ? m->Link[(iLink)].N1 : m->Link[(iLink)].N2 );
}
int DOWN_NODE(OW_Model *m, int iLink) {
  return ( (m->FlowDir[(iLink)]=='+') ? m->Link[(iLink)].N2 : m->Link[(iLink)].N1 );
}
int LINKVOL(OW_Model *m, int iLink) {
  return ( 0.785398 * m->Link[(iLink)].Len * SQR(m->Link[(iLink)].Diam) );
}



int  openqual(OW_Model *m)
/*
**--------------------------------------------------------------
**   Input:   none     
**   Output:  returns error code                                          
**   Purpose: opens WQ solver system 
**--------------------------------------------------------------
*/
{
   int errcode = 0;
   int n;

   /* Allocate memory pool for WQ segments */
   m->OutOfMemory = FALSE;
   m->SegPool = AllocInit();                                                      //(2.00.11 - LR)
   if (m->SegPool == NULL) errcode = 101;                                         //(2.00.11 - LR)

   /* Allocate scratch array & reaction rate array*/
   m->TempQual  = (double *) calloc(MAX((m->Nnodes+1),(m->Nlinks+1)),sizeof(double));
   m->PipeRateCoeff  = (double *) calloc((m->Nlinks+1), sizeof(double));
   ERRCODE(MEMCHECK(m->TempQual));
   ERRCODE(MEMCHECK(m->PipeRateCoeff));

   /* Allocate memory for WQ solver */
   n        = m->Nlinks + m->Ntanks + 1;
   m->FirstSeg = (Pseg *) calloc(n, sizeof(Pseg));
   m->LastSeg  = (Pseg *) calloc(n, sizeof(Pseg));
   m->FlowDir  = (char *) calloc(n, sizeof(char));
   n        = m->Nnodes+1;
   m->VolIn    = (double *) calloc(n, sizeof(double));
   m->MassIn   = (double *) calloc(n, sizeof(double));
   ERRCODE(MEMCHECK(m->FirstSeg));
   ERRCODE(MEMCHECK(m->LastSeg));
   ERRCODE(MEMCHECK(m->FlowDir));
   ERRCODE(MEMCHECK(m->VolIn));
   ERRCODE(MEMCHECK(m->MassIn));
   return(errcode);
}

/* Local function to compute unit conversion factor for bulk reaction rates */
   double getucf(double order)
   {
      if (order < 0.0) order = 0.0;
      if (order == 1.0) return(1.0);
      else return(1./pow(LperFT3,(order-1.0)));
   }


void  initqual(OW_Model *m)
/*
**--------------------------------------------------------------
**   Input:   none     
**   Output:  none                                          
**   Purpose: re-initializes WQ solver system 
**--------------------------------------------------------------
*/
{
   int i;

   /* Initialize quality, tank volumes, & source mass flows */
   for (i=1; i <= m->Nnodes; i++) m->NodeQual[i] = m->Node[i].C0;
   for (i=1; i <= m->Ntanks; i++) m->Tank[i].C = m->Node[m->Tank[i].Node].C0;
   for (i=1; i <= m->Ntanks; i++) m->Tank[i].V = m->Tank[i].V0;
   for (i=1; i <= m->Nnodes; i++) {
     if (m->Node[i].S != NULL) {
       m->Node[i].S->Smass = 0.0;
     }
   }
  
   m->QTankVolumes = calloc(m->Ntanks, sizeof(double)); // keep track of previous step's tank volumes.
   m->QLinkFlow    = calloc(m->Nlinks, sizeof(double)); // keep track of previous step's link flows.
  
   /* Set WQ parameters */
   m->Bucf = 1.0;
   m->Tucf = 1.0;
   m->Reactflag = 0;
   if (m->Qualflag != NONE)
   {
      /* Initialize WQ at trace node (if applicable) */
      if (m->Qualflag == TRACE) m->NodeQual[m->TraceNode] = 100.0;

      /* Compute Schmidt number */
      if (m->Diffus > 0.0)
         m->Sc = m->Viscos/m->Diffus;
      else
         m->Sc = 0.0;

      /* Compute unit conversion factor for bulk react. coeff. */
      m->Bucf = getucf(m->BulkOrder);
      m->Tucf = getucf(m->TankOrder);

      /* Check if modeling a reactive substance */
      m->Reactflag = setReactflag(m);

      /* Reset memory pool */
      m->FreeSeg = NULL;
      AllocSetPool(m->SegPool);                                                   //(2.00.11 - LR)
      AllocReset();                                                            //(2.00.11 - LR)
   }

   /* Initialize avg. reaction rates */
   m->Wbulk = 0.0;
   m->Wwall = 0.0;
   m->Wtank = 0.0;
   m->Wsource = 0.0;

   /* Re-position hydraulics file */
  if (!m->OpenHflag) {
   fseek(m->HydFile, m->HydOffset, SEEK_SET);
  }
   

   /* Set elapsed times to zero */
   m->Htime = 0;
   m->Qtime = 0;
   m->Rtime = m->Rstart;
   m->Nperiods = 0;
  
  initsegs(m);
}


int runqual(OW_Model *m, long *t)
/*
**--------------------------------------------------------------
**   Input:   none     
**   Output:  t = pointer to current simulation time (sec)
**   Returns: error code                                          
**   Purpose: retrieves hydraulics for next hydraulic time step
**            (at time *t) and saves current results to file
**--------------------------------------------------------------
*/
{
   long    hydtime;       /* Hydraulic solution time */
   long    hydstep;       /* Hydraulic time step     */
   int     errcode = 0;

   /* Update reported simulation time */
   *t = m->Qtime;

   /* Read hydraulic solution from hydraulics file */
   if (m->Qtime == m->Htime)
   {
      errcode = gethyd(m, &hydtime, &hydstep);
      if (!m->OpenHflag) { // test for sequential vs stepwise
        // sequential
        m->Htime = hydtime + hydstep;
      }
      else {
        // stepwise calculation - hydraulic results are already in memory
        for (int i=1; i<= m->Ntanks; ++i) {
          m->QTankVolumes[i-1] = m->Tank[i].V;
        }
        
        for (int i=1; i<= m->Nlinks; ++i)
        {
          if (m->hydraulics.LinkStatus[i] <= CLOSED) {
            m->QLinkFlow[i-1] = m->hydraulics.LinkFlows[i];
          }
        }

      }
   }
   else {
        // stepwise calculation
        for (int i=1; i<= m->Ntanks; ++i) {
          m->QTankVolumes[i-1] = m->Tank[i].V;
        }
        
        for (int i=1; i<= m->Nlinks; ++i)
        {
          if (m->hydraulics.LinkStatus[i] <= CLOSED) {
            m->QLinkFlow[i-1] = m->hydraulics.LinkFlows[i];
          }
        }

  }
   
   return(errcode);
}


int nextqual(OW_Model *m, long *tstep)
/*
**--------------------------------------------------------------
**   Input:   none     
**   Output:  tstep = pointer to time step (sec)
**   Returns: error code                                          
**   Purpose: updates WQ conditions until next hydraulic 
**            solution occurs (after *tstep secs.)
**--------------------------------------------------------------
*/
{
   long    hydstep;       /* Hydraulic solution time step */
   int     errcode = 0;

   /* Determine time step */
   *tstep = 0;
  
  // hydstep = Htime - Qtime;
  
  if (m->Htime <= m->Dur) {
    hydstep = m->Htime - m->Qtime;
  }
  else hydstep = 0;
  
  double *tankVolumes;
  
  // if we're operating in stepwise mode, capture the tank levels so we can restore them later.
  if (m->OpenHflag) {
    tankVolumes = calloc(m->Ntanks, sizeof(double));
    for (int i=1; i <= m->Ntanks; ++i) {
      if (m->Tank[i].A != 0) { // skip reservoirs
        tankVolumes[i-1] = m->Tank[i].V;
      }
    }
    
    // restore the previous step's tank volumes
    for (int i=1; i <= m->Ntanks; i++) {
      if (m->Tank[i].A != 0) { // skip reservoirs again
        int n = m->Tank[i].Node;
        m->Tank[i].V = m->QTankVolumes[i-1];
        m->hydraulics.NodeHead[n] = tankgrade(m, i, m->Tank[i].V);
      }
    }
    
    // restore the previous step's pipe link flows
    for (int i=1; i <= m->Nlinks; i++) {
      if (m->hydraulics.LinkStatus[i] <= CLOSED) {
        m->hydraulics.LinkFlows[i] = 0.0;
      }
    }

  }
  
   /* Perform water quality routing over this time step */
   if (m->Qualflag != NONE && hydstep > 0) {
     transport(m,hydstep);
   }
  
   /* Update current time */
   if (m->OutOfMemory)
     errcode = 101;
  
   if (!errcode)
     *tstep = hydstep;
  
   m->Qtime += hydstep;

   /* Save final output if no more time steps */
   if (!errcode && m->Saveflag && *tstep == 0) errcode = savefinaloutput(m);
  
  // restore tank levels to post-runH state, if needed.
  if (m->OpenHflag) {
    for (int i=1; i <= m->Ntanks; i++) {
      if (m->Tank[i].A != 0) { // skip reservoirs again
        int n = m->Tank[i].Node;
        m->Tank[i].V = tankVolumes[i-1];
        m->hydraulics.NodeHead[n] = tankgrade(m, i, m->Tank[i].V);
      }
    }
    
    for (int i=1; i <= m->Nlinks; ++i) {
      if (m->hydraulics.LinkStatus[i] <= CLOSED) {
        m->hydraulics.LinkFlows[i] = m->QLinkFlow[i-1];
      }
    }
    
    free(tankVolumes);
  }
  
   return(errcode);
}


int stepqual(OW_Model *m, long *tleft)
/*
**--------------------------------------------------------------
**   Input:   none     
**   Output:  tleft = pointer to time left in simulation
**   Returns: error code                                          
**   Purpose: updates WQ conditions over a single WQ time step
**--------------------------------------------------------------
*/
{  long dt, hstep, t, tstep;
   int  errcode = 0;
   tstep = m->Qstep;
   do
   {
      dt = tstep;
      hstep = m->Htime - m->Qtime;
      if (hstep < dt)
      {
         dt = hstep;
         if (m->Qualflag != NONE) transport(m, dt);
         m->Qtime += dt;
         errcode = runqual(m,&t);
         m->Qtime = t;
      }
      else
      {
         if (m->Qualflag != NONE) transport(m,dt);
         m->Qtime += dt;
      }
      tstep -= dt;
      if (m->OutOfMemory) errcode = 101;
   }  while (!errcode && tstep > 0);
   *tleft = m->Dur - m->Qtime;
   if (!errcode && m->Saveflag && *tleft == 0) {
     errcode = savefinaloutput(m);
   }
   return(errcode);
}


int closequal(OW_Model *m)
/*
**--------------------------------------------------------------
**   Input:   none     
**   Output:  returns error code                                          
**   Purpose: closes WQ solver system 
**--------------------------------------------------------------
*/
{
   int errcode = 0;

   /* Free memory pool */
   if ( m->SegPool )                                                              //(2.00.11 - LR)
   {                                                                           //(2.00.11 - LR)
        AllocSetPool(m->SegPool);                                                 //(2.00.11 - LR)
        AllocFreePool();                                                       //(2.00.11 - LR)
   }                                                                           //(2.00.11 - LR)

   free(m->FirstSeg);
   free(m->LastSeg);
   free(m->FlowDir);
   free(m->VolIn);
   free(m->MassIn);
   free(m->PipeRateCoeff);
   free(m->TempQual);
   free(m->QTankVolumes);
   free(m->QLinkFlow);
   return(errcode);
}


int  gethyd(OW_Model *m, long *hydtime, long *hydstep)
/*
**-----------------------------------------------------------
**   Input:   none     
**   Output:  hydtime = pointer to hydraulic solution time
**            hydstep = pointer to hydraulic time step
**   Returns: error code                                          
**   Purpose: retrieves hydraulic solution and hydraulic
**            time step for next hydraulic event
**
**   NOTE: when this function is called, WQ results have
**         already been updated to the point in time when
**         the next hydraulic event occurs.
**-----------------------------------------------------------
*/
{
   int errcode = 0;

  // if hydraulics are not open, then we're operating in sequential mode.
  // else hydraulics are open, so use the hydraulic results in memory rather than reading from the temp file.
  if (!m->OpenHflag) {
    /* Read hydraulic results from file */
    if (!readhyd(m,hydtime)) return(307);
    if (!readhydstep(m,hydstep)) return(307);
    m->Htime = *hydtime;
  }

   /* Save current results to output file */
   if (m->Htime >= m->Rtime)
   {
      if (m->Saveflag)
      {
         errcode = saveoutput(m);
         m->Nperiods++;
      }
      m->Rtime += m->Rstep;
   }

   /* If simulating WQ: */
   if (m->Qualflag != NONE && m->Qtime < m->Dur)
   {

      /* Compute reaction rate coeffs. */
     if (m->Reactflag && m->Qualflag != AGE) {
       ratecoeffs(m);
     }
     
      /* Initialize pipe segments (at time 0) or  */
      /* else re-orient segments if flow reverses.*/
      //if (Qtime == 0)
      //  initsegs();
      //else
     // if hydraulics are open, or if we're in sequential mode (where qtime can increase)
     if (m->OpenHflag || m->Qtime != 0) {
       reorientsegs(m);
     }

   }
   return(errcode);
}


char  setReactflag(OW_Model *m)
/*
**-----------------------------------------------------------
**   Input:   none     
**   Output:  returns 1 for reactive WQ constituent, 0 otherwise                                          
**   Purpose: checks if reactive chemical being simulated            
**-----------------------------------------------------------
*/
{
   int  i;
   if      (m->Qualflag == TRACE) return(0);
   else if (m->Qualflag == AGE)   return(1);
   else
   {
      for (i=1; i <= m->Nlinks; i++)
      {
         if (m->Link[i].Type <= PIPE)
         {
            if (m->Link[i].Kb != 0.0 || m->Link[i].Kw != 0.0) return(1);
         }
      }
      for (i=1; i <= m->Ntanks; i++)
         if (m->Tank[i].Kb != 0.0) return(1);
   }
   return(0);
}


void  transport(OW_Model *m, long tstep)
/*
**--------------------------------------------------------------
**   Input:   tstep = length of current time step     
**   Output:  none
**   Purpose: transports constituent mass through pipe network        
**            under a period of constant hydraulic conditions.        
**--------------------------------------------------------------
*/
{
   long   qtime, dt;
  
   /* Repeat until elapsed time equals hydraulic time step */

   AllocSetPool(m->SegPool);                                                      //(2.00.11 - LR)
   qtime = 0;
   while (!m->OutOfMemory && qtime < tstep)
   {                                  /* Qstep is quality time step */
      dt = MIN(m->Qstep, tstep-qtime);    /* Current time step */
      qtime += dt;                    /* Update elapsed time */
      if (m->Reactflag) updatesegs(m, dt);  /* Update quality in inner link segs */
      accumulate(m, dt);                 /* Accumulate flow at nodes */
      updatenodes(m, dt);                /* Update nodal quality */
      sourceinput(m, dt);                /* Compute inputs from sources */
      release(m, dt);                    /* Release new nodal flows */
   }
   updatesourcenodes(m, tstep);          /* Update quality at source nodes */
  
}


void  initsegs(OW_Model *m)
/*
**--------------------------------------------------------------
**   Input:   none     
**   Output:  none
**   Purpose: initializes water quality segments                      
**--------------------------------------------------------------
*/
{
   int     j,k;
   double   c,v;

   /* Examine each link */
   for (k=1; k <= m->Nlinks; k++)
   {

      /* Establish flow direction */
     m->FlowDir[k] = '+';
     if (m->hydraulics.LinkFlows[k] < 0.) {
       m->FlowDir[k] = '-';
     }

      /* Set segs to zero */
      m->LastSeg[k] = NULL;
      m->FirstSeg[k] = NULL;

      /* Find quality of downstream node */
      j = DOWN_NODE(m,k);
      if (j <= m->Njuncs) c = m->NodeQual[j];
      else             c = m->Tank[j - m->Njuncs].C;

      /* Fill link with single segment with this quality */
      addseg(m,k,LINKVOL(m,k),c);
   }

   /* Initialize segments in tanks that use them */
   for (j=1; j <= m->Ntanks; j++)
   {

      /* Skip reservoirs & complete mix tanks */
      if (m->Tank[j].A == 0.0
      ||  m->Tank[j].MixModel == MIX1) continue;

      /* Tank segment pointers are stored after those for links */
      k = m->Nlinks + j;
      c = m->Tank[j].C;
      m->LastSeg[k] = NULL;
      m->FirstSeg[k] = NULL;

      /* Add 2 segments for 2-compartment model */
      if (m->Tank[j].MixModel == MIX2)
      {
         v = MAX(0, m->Tank[j].V - m->Tank[j].V1max);
         addseg(m,k,v,c);
         v = m->Tank[j].V - v;
         addseg(m,k,v,c);
      }

      /* Add one segment for FIFO & LIFO models */
      else
      {
         v = m->Tank[j].V;
         addseg(m,k,v,c);
      }
   }
}


void  reorientsegs(OW_Model *m)
/*
**--------------------------------------------------------------
**   Input:   none     
**   Output:  none
**   Purpose: re-orients segments (if flow reverses)                  
**--------------------------------------------------------------
*/
{
   Pseg   seg, nseg, pseg;
   int    k;
   char   newdir;

   /* Examine each link */
   for (k=1; k <= m->Nlinks; k++)
   {

      /* Find new flow direction */
     newdir = '+';
     if (m->hydraulics.LinkFlows[k] == 0.0) {
       newdir = m->FlowDir[k];
     }
     else if (m->hydraulics.LinkFlows[k] < 0.0) {
       newdir = '-';
     }

      /* If direction changes, then reverse order of segments */
      /* (first to last) and save new direction */
      if (newdir != m->FlowDir[k])
      {
         seg = m->FirstSeg[k];
         m->FirstSeg[k] = m->LastSeg[k];
         m->LastSeg[k] = seg;
         pseg = NULL;
         while (seg != NULL)
         {
            nseg = seg->prev;
            seg->prev = pseg;
            pseg = seg;
            seg = nseg;
         }
         m->FlowDir[k] = newdir;
      }
   }
}


void  updatesegs(OW_Model *m, long dt)
/*
**-------------------------------------------------------------
**   Input:   t = time from last WQ segment update     
**   Output:  none
**   Purpose: reacts material in pipe segments up to time t               
**-------------------------------------------------------------
*/
{
   int    k;
   Pseg   seg;
   double  cseg, rsum, vsum;

   /* Examine each link in network */
   for (k=1; k <= m->Nlinks; k++)
   {

      /* Skip zero-length links (pumps & valves) */
      rsum = 0.0;
      vsum = 0.0;
      if (m->Link[k].Len == 0.0) continue;

      /* Examine each segment of the link */
      seg = m->FirstSeg[k];
      while (seg != NULL)
      {

            /* React segment over time dt */
            cseg = seg->c;
            seg->c = pipereact(m, k, seg->c, seg->v, dt);

            /* Accumulate volume-weighted reaction rate */
            if (m->Qualflag == CHEM)
            {
               rsum += ABS((seg->c - cseg))*seg->v;
               vsum += seg->v;
            }
            seg = seg->prev;
      }

      /* Normalize volume-weighted reaction rate */
     if (vsum > 0.0) {
       m->PipeRateCoeff[k] = rsum/vsum/dt*SECperDAY;
     }
     else {
       m->PipeRateCoeff[k] = 0.0;
     }
   }
}


void  removesegs(OW_Model *m, int k)
/*
**-------------------------------------------------------------
**   Input:   k = link index     
**   Output:  none
**   Purpose: removes all segments in link k                                 
**-------------------------------------------------------------
*/
{
    Pseg seg;
    seg = m->FirstSeg[k];
    while (seg != NULL)
    {
        m->FirstSeg[k] = seg->prev;
        seg->prev = m->FreeSeg;
        m->FreeSeg = seg;
        seg = m->FirstSeg[k];
    }
    m->LastSeg[k] = NULL;
}


void  addseg(OW_Model *m, int k, double v, double c)
/*
**-------------------------------------------------------------
**   Input:   k = link segment
**            v = segment volume
**            c = segment quality
**   Output:  none
**   Purpose: adds a segment to start of link k (i.e., upstream
**            of current last segment).
**-------------------------------------------------------------
*/
{
    Pseg seg;

    if (m->FreeSeg != NULL)
    {
       seg = m->FreeSeg;
       m->FreeSeg = seg->prev;
    }
    else
    {
        seg = (struct Sseg *) Alloc(sizeof(struct Sseg));
        if (seg == NULL)
        {
           m->OutOfMemory = TRUE;
           return;
        }     
    }
    seg->v = v;
    seg->c = c;
    seg->prev = NULL;
  
    if (m->FirstSeg[k] == NULL)
      m->FirstSeg[k] = seg;
  
    if (m->LastSeg[k] != NULL)
      m->LastSeg[k]->prev = seg;
  
    m->LastSeg[k] = seg;
}


void accumulate(OW_Model *m, long dt)
/*
**-------------------------------------------------------------
**   Input:   dt = current WQ time step
**   Output:  none
**   Purpose: accumulates mass flow at nodes and updates nodal
**            quality   
**-------------------------------------------------------------
*/
{
   int    i,j,k;
   double  cseg,v,vseg;
   Pseg   seg;
  
  Pseg *LastSeg = m->LastSeg;

   /* Re-set memory used to accumulate mass & volume */
   memset(m->VolIn,0,(m->Nnodes+1)*sizeof(double));
   memset(m->MassIn,0,(m->Nnodes+1)*sizeof(double));
   memset(m->TempQual,0,(m->Nnodes+1)*sizeof(double));

   /* Compute average conc. of segments adjacent to each node */
   /* (For use if there is no transport through the node) */
   for (k=1; k <= m->Nlinks; k++)
   {
      j = DOWN_NODE(m,k);             /* Downstream node */
      if (m->FirstSeg[k] != NULL)      /* Accumulate concentrations */
      {
         m->MassIn[j] += m->FirstSeg[k]->c;
         m->VolIn[j]++;
      }
      j = UP_NODE(m,k);              /* Upstream node */
      if (m->LastSeg[k] != NULL)      /* Accumulate concentrations */
      {
         m->MassIn[j] += LastSeg[k]->c;
         m->VolIn[j]++;
      }
   }
  
  for (k=1; k <= m->Nnodes; k++) {
    if (m->VolIn[k] > 0.0) {
      m->TempQual[k] = m->MassIn[k] / m->VolIn[k];
    }
  }
  
   /* Move mass from first segment of each pipe into downstream node */
   memset(m->VolIn,0,(m->Nnodes+1)*sizeof(double));
   memset(m->MassIn,0,(m->Nnodes+1)*sizeof(double));
   for (k=1; k <= m->Nlinks; k++)
   {
      i = UP_NODE(m,k);               /* Upstream node */
      j = DOWN_NODE(m,k);             /* Downstream node */
      v = ABS(m->hydraulics.LinkFlows[k])*dt;             /* Flow volume */

////  Start of deprecated code segment  ////                                   //(2.00.12 - LR)
         
      /* If link volume < flow volume, then transport upstream    */
      /* quality to downstream node and remove all link segments. */
/*      if (LINKVOL(k) < v)
      {
         VolIn[j] += v;
         seg = FirstSeg[k];
         cseg = NodeQuali];
         if (seg != NULL) cseg = seg->c;
         MassIn[j] += v*cseg;
         removesegs(k);
      }
*/
      /* Otherwise remove flow volume from leading segments */
      /* and accumulate flow mass at downstream node        */
      //else

////  End of deprecated code segment.  ////                                    //(2.00.12 - LR)

      while (v > 0.0)                                                          //(2.00.12 - LR)
      {
         /* Identify leading segment in pipe */
         seg = m->FirstSeg[k];
         if (seg == NULL) break;

         /* Volume transported from this segment is */
         /* minimum of flow volume & segment volume */
         /* (unless leading segment is also last segment) */
         vseg = seg->v;
         vseg = MIN(vseg,v);
         if (seg == m->LastSeg[k]) vseg = v;

         /* Update volume & mass entering downstream node  */
         cseg = seg->c;
         m->VolIn[j] += vseg;
         m->MassIn[j] += vseg*cseg;

         /* Reduce flow volume by amount transported */
         v -= vseg;

         /* If all of segment's volume was transferred, then */
         /* replace leading segment with the one behind it   */
         /* (Note that the current seg is recycled for later use.) */
         if (v >= 0.0 && vseg >= seg->v)
         {
            m->FirstSeg[k] = seg->prev;
            if (m->FirstSeg[k] == NULL) m->LastSeg[k] = NULL;
            seg->prev = m->FreeSeg;
            m->FreeSeg = seg;
         }

         /* Otherwise reduce segment's volume */
         else
         {
            seg->v -= vseg;
         }
      }     /* End while */
   }        /* Next link */
}


void updatenodes(OW_Model *m, long dt)
/*
**---------------------------------------------------------------------------
**   Input:   dt = current WQ time step     
**   Output:  none
**   Purpose: updates concentration at all nodes to mixture of accumulated
**            inflow from connecting pipes.
**
**  Note:     Does not account for source flow effects. TempQual[i] contains
**            average concen. of segments adjacent to node i, used in case
**            there was no inflow into i.
**---------------------------------------------------------------------------
*/
{
  int i;
  
  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeQual = m->NodeQual;
  int Njuncs = m->Njuncs;
  char Qualflag = m->Qualflag;
  double *TempQual = m->TempQual;
  double *VolIn = m->VolIn;
  
  /* Update junction quality */
  for (i=1; i<=Njuncs; i++)
  {
    if (NodeDemand[i] < 0.0) {
      VolIn[i] -= NodeDemand[i]*dt;
    }
    if (VolIn[i] > 0.0) {
      NodeQual[i] = m->MassIn[i]/VolIn[i];
    }
    else {
      NodeQual[i] = TempQual[i];
    }
  }
  
  /* Update tank quality */
  updatetanks(m,dt);
  
  /* For flow tracing, set source node concen. to 100. */
  if (Qualflag == TRACE)
    NodeQual[m->TraceNode] = 100.0;
}


void sourceinput(OW_Model *m, long dt)
/*
**---------------------------------------------------------------------
**   Input:   dt = current WQ time step     
**   Output:  none
**   Purpose: computes contribution (if any) of mass additions from WQ
**            sources at each node.
**---------------------------------------------------------------------
*/
{
   int   j,n;
   double massadded = 0.0, s, volout;
   double qout, qcutoff;
   Psource source;

  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeQual = m->NodeQual;
  Snode *Node = m->Node;
  Stank *Tank = m->Tank;
  int Njuncs = m->Njuncs;
  int Ntanks = m->Ntanks;
  int Nnodes = m->Nnodes;
  char Qualflag = m->Qualflag;
  double *TempQual = m->TempQual;

  
  
   /* Establish a flow cutoff which indicates no outflow from a node */
   qcutoff = 10.0*TINY;

   /* Zero-out the work array TempQual */
   memset(TempQual,0,(Nnodes+1)*sizeof(double));
   if (Qualflag != CHEM) return;

   /* Consider each node */
   for (n=1; n<=Nnodes; n++)
   {
      double thisDemand = NodeDemand[n];
      /* Skip node if no WQ source */
      source = Node[n].S;
      if (source == NULL) continue;
      if (source->C0 == 0.0) continue;
    
      /* Find total flow volume leaving node */
      if (n <= Njuncs) volout = m->VolIn[n];  /* Junctions */
      else volout = m->VolIn[n] - (thisDemand * dt);    /* Tanks */
      qout = volout / (double) dt;

      /* Evaluate source input only if node outflow > cutoff flow */
      if (qout > qcutoff)
      {

         /* Mass added depends on type of source */
         s = sourcequal(m,source);
         switch(source->Type)
         {
            /* Concen. Source: */
            /* Mass added = source concen. * -(demand) */
            case CONCEN:

               /* Only add source mass if demand is negative */
               if (thisDemand < 0.0)
               {
                  massadded = -s*thisDemand*dt;

                  /* If node is a tank then set concen. to 0. */
                  /* (It will be re-set to true value in updatesourcenodes()) */
                  if (n > Njuncs) NodeQual[n] = 0.0;
               }
               else massadded = 0.0;
               break;

            /* Mass Inflow Booster Source: */
            case MASS:
               massadded = s*dt;
               break;

            /* Setpoint Booster Source: */
            /* Mass added is difference between source */
            /* & node concen. times outflow volume  */
            case SETPOINT:
             if (s > NodeQual[n]) {
               massadded = (s-NodeQual[n])*volout;
             }
             else {
               massadded = 0.0;
             }
             break;

            /* Flow-Paced Booster Source: */
            /* Mass added = source concen. times outflow volume */
            case FLOWPACED:
               massadded = s*volout;
               break;
         }

         /* Source concen. contribution = (mass added / outflow volume) */
         TempQual[n] = massadded/volout;

         /* Update total mass added for time period & simulation */
         source->Smass += massadded;
         if (m->Htime >= m->Rstart) m->Wsource += massadded;
      }
   }

   /* Add mass inflows from reservoirs to Wsource*/
   if (m->Htime >= m->Rstart)
   {
      for (j=1; j <= Ntanks; j++)
      {
         if (Tank[j].A == 0.0)
         {
            n = Njuncs + j;
            volout = m->VolIn[n] - NodeDemand[n]*dt;
            if (volout > 0.0) m->Wsource += volout * NodeQual[n];
         }
      }
   }
}


void release(OW_Model *m, long dt)
/*
**---------------------------------------------------------
**   Input:   dt = current WQ time step
**   Output:  none
**   Purpose: creates new segments in outflow links from nodes.
**---------------------------------------------------------
*/
{
   int    k,n;
   double  c,q,v;
   Pseg   seg;

  
  double *NodeQual = m->NodeQual;
  double *LinkFlows = m->hydraulics.LinkFlows;
  int Nlinks = m->Nlinks;
  double *TempQual = m->TempQual;

  
   /* Examine each link */
   for (k=1; k<=Nlinks; k++)
   {

      /* Ignore links with no flow */
      if (LinkFlows[k] == 0.0) continue;

      /* Find flow volume released to link from upstream node */
      /* (NOTE: Flow volume is allowed to be > link volume.) */
      n = UP_NODE(m,k);
      q = ABS(LinkFlows[k]);
      v = q*dt;

      /* Include source contribution in quality released from node. */
      c = NodeQual[n] + TempQual[n];

      /* If link has a last seg, check if its quality     */
      /* differs from that of the flow released from node.*/
      if ( (seg = m->LastSeg[k]) != NULL)
      {
         /* Quality of seg close to that of node */
         if (ABS(seg->c - c) < m->Ctol)
         {
            seg->c = (seg->c*seg->v + c*v) / (seg->v + v);                     //(2.00.11 - LR)
            seg->v += v;
         }

         /* Otherwise add a new seg to end of link */
         else addseg(m,k,v,c);
      }

      /* If link has no segs then add a new one. */
      else addseg(m,k,LINKVOL(m,k),c);
   }
}


void  updatesourcenodes(OW_Model *m, long dt)
/*
**---------------------------------------------------
**   Input:   dt = current WQ time step     
**   Output:  none
**   Purpose: updates quality at source nodes.
**            (TempQual[n] = concen. added by source at node n)
**---------------------------------------------------
*/
{
   int i,n;
   Psource source;

  double *NodeQual = m->NodeQual;
  Snode *Node = m->Node;
  Stank *Tank = m->Tank;
  int Njuncs = m->Njuncs;
  int Nnodes = m->Nnodes;
  char Qualflag = m->Qualflag;
  double *TempQual = m->TempQual;
  
   if (Qualflag != CHEM) return;

   /* Examine each WQ source node */
   for (n=1; n<=Nnodes; n++)
   {
      source = Node[n].S;
      if (source == NULL) continue;

      /* Add source to current node concen. */
      NodeQual[n] += TempQual[n];

      /* For tanks, node concen. = internal concen. */
      if (n > Njuncs)
      {
         i = n - Njuncs;
         if (Tank[i].A > 0.0) NodeQual[n] = Tank[i].C;
      }

      /* Normalize mass added at source to time step */
      source->Smass /= (double)dt;
   }
}


void  updatetanks(OW_Model *m, long dt)
/*
**---------------------------------------------------
**   Input:   dt = current WQ time step     
**   Output:  none
**   Purpose: updates tank volumes & concentrations            
**---------------------------------------------------
*/
{
    int   i,n;

  double *NodeQual = m->NodeQual;
  Snode *Node = m->Node;
  Stank *Tank = m->Tank;
  int Ntanks = m->Ntanks;

   /* Examine each reservoir & tank */
   for (i=1; i<=Ntanks; i++)
   {
      n = Tank[i].Node;
      /* Use initial quality for reservoirs */
      if (Tank[i].A == 0.0)
      {
         NodeQual[n] = Node[n].C0;
      }
      /* Update tank WQ based on mixing model */
      else {
        switch(Tank[i].MixModel)
        {
          case MIX2: tankmix2(m,i,dt); break;
          case FIFO: tankmix3(m,i,dt); break;
          case LIFO: tankmix4(m,i,dt); break;
          default:   tankmix1(m,i,dt); break;
        }
        
      }
   }
}


////  Deprecated version of tankmix1  ////                                     //(2.00.12 - LR)
//void  tankmix1(int i, long dt)
/*
**---------------------------------------------
**   Input:   i = tank index
**            dt = current WQ time step     
**   Output:  none
**   Purpose: complete mix tank model                     
**---------------------------------------------
*/
//{
//    int   n;
//    double cin;

//   /* Blend inflow with contents */
//   n = Tank[i].Node;
//   if (VolIn[n] > 0.0) cin = MassIn[n]/VolIn[n];
//   else                 cin = 0.0;
//   if (Tank[i].V > 0.0)
//      Tank[i].C = tankreact(Tank[i].C,Tank[i].V,Tank[i].Kb,dt) +
//                  (cin - Tank[i].C)*VolIn[n]/Tank[i].V;
//   else Tank[i].C = cin;
//   Tank[i].C = MAX(0.0, Tank[i].C);

//   /* Update tank volume & nodal quality */
//   Tank[i].V += D[n]*dt;
//   NodeQual[n] = Tank[i].C;
//}


////  New version of tankmix1  ////                                            //(2.00.12 - LR)
void  tankmix1(OW_Model *m, int i, long dt)
/*
**---------------------------------------------
**   Input:   i = tank index
**            dt = current WQ time step     
**   Output:  none
**   Purpose: complete mix tank model                     
**---------------------------------------------
*/
{
    int   n;
    double cin;
    double c, cmax, vold, vin;

  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeQual = m->NodeQual;
  Stank *Tank = m->Tank;
  
   /* React contents of tank */
   c = tankreact(m,Tank[i].C,Tank[i].V,Tank[i].Kb,dt);

   /* Determine tank & volumes */
   vold = Tank[i].V;
   n = Tank[i].Node;
   Tank[i].V += NodeDemand[n]*dt;
   vin  = m->VolIn[n];

   /* Compute inflow concen. */
   if (vin > 0.0)
     cin = m->MassIn[n]/vin;
   else
     cin = 0.0;
  
   cmax = MAX(c, cin);

   /* Mix inflow with tank contents */
   if (vin > 0.0)
     c = (c*vold + cin*vin)/(vold + vin);
  
   c = MIN(c, cmax);
   c = MAX(c, 0.0);
   Tank[i].C = c;
   NodeQual[n] = Tank[i].C;
}

/*** Updated 10/25/00 ***/
////  New version of tankmix2  ////                                            //(2.00.12 - LR) 
void  tankmix2(OW_Model *m, int i, long dt)
/*
**------------------------------------------------
**   Input:   i = tank index
**            dt = current WQ time step     
**   Output:  none
**   Purpose: 2-compartment tank model                      
**            (seg1 = mixing zone,
**             seg2 = ambient zone)      
**------------------------------------------------
*/
{
    int     k,n;
    double  cin,        /* Inflow quality */
            vin,        /* Inflow volume */
            vt,         /* Transferred volume */
            vnet,       /* Net volume change */
            v1max;      /* Full mixing zone volume */
   Pseg     seg1,seg2;  /* Compartment segments */

  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeQual = m->NodeQual;
  Stank *Tank = m->Tank;
  int Nlinks = m->Nlinks;
  
   /* Identify segments for each compartment */
   k = Nlinks + i;
   seg1 = m->LastSeg[k];
   seg2 = m->FirstSeg[k];
   if (seg1 == NULL || seg2 == NULL) return;

   /* React contents of each compartment */
   seg1->c = tankreact(m,seg1->c,seg1->v,Tank[i].Kb,dt);
   seg2->c = tankreact(m,seg2->c,seg2->v,Tank[i].Kb,dt);

   /* Find inflows & outflows */
   n = Tank[i].Node;
   vnet = NodeDemand[n]*dt;
   vin = m->VolIn[n];
   if (vin > 0.0) cin = m->MassIn[n]/vin;
   else           cin = 0.0;
   v1max = Tank[i].V1max;

   /* Tank is filling */
   vt = 0.0;
   if (vnet > 0.0)
   {
      vt = MAX(0.0, (seg1->v + vnet - v1max));
      if (vin > 0.0)
      {
         seg1->c = ((seg1->c)*(seg1->v) + cin*vin) / (seg1->v + vin);
      }
      if (vt > 0.0)
      {
         seg2->c = ((seg2->c)*(seg2->v) + (seg1->c)*vt) / (seg2->v + vt);
      }
   }

   /* Tank is emptying */
   if (vnet < 0.0)
   {
      if (seg2->v > 0.0)
      {
         vt = MIN(seg2->v, (-vnet));
      }
      if (vin + vt > 0.0)
      {
         seg1->c = ((seg1->c)*(seg1->v) + cin*vin + (seg2->c)*vt) /
                   (seg1->v + vin + vt);
      }
   }

   /* Update segment volumes */
   if (vt > 0.0)
   {
      seg1->v = v1max;
      if (vnet > 0.0) seg2->v += vt;
      else            seg2->v = MAX(0.0, ((seg2->v)-vt));
   }
   else
   {
      seg1->v += vnet;
      seg1->v = MIN(seg1->v, v1max);
      seg1->v = MAX(0.0, seg1->v);
      seg2->v = 0.0;
   }
   Tank[i].V += vnet;
   Tank[i].V = MAX(0.0, Tank[i].V);

   /* Use quality of mixed compartment (seg1) to */
   /* represent quality of tank since this is where */
   /* outflow begins to flow from */
   Tank[i].C = seg1->c;
   NodeQual[n] = Tank[i].C;
}


void  tankmix3(OW_Model *m, int i, long dt)
/*
**----------------------------------------------------------
**   Input:   i = tank index
**            dt = current WQ time step     
**   Output:  none
**   Purpose: First-In-First-Out (FIFO) tank model                    
**----------------------------------------------------------
*/
{
   int   k,n;
   double vin,vnet,vout,vseg;
   double cin,vsum,csum;
   Pseg  seg;
  
  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeQual = m->NodeQual;
  Stank *Tank = m->Tank;
  int Nlinks = m->Nlinks;
  Pseg FreeSeg = m->FreeSeg;
  Pseg *FirstSeg = m->FirstSeg;
  Pseg *LastSeg = m->LastSeg;
  double *VolIn = m->VolIn;
  double *MassIn = m->MassIn;
  
  
   k = Nlinks + i;
   if (LastSeg[k] == NULL || FirstSeg[k] == NULL) return;

   /* React contents of each compartment */
   if (m->Reactflag)
   {
      seg = FirstSeg[k];
      while (seg != NULL)
      {
         seg->c = tankreact(m,seg->c,seg->v,Tank[i].Kb,dt);
         seg = seg->prev;
      }
   }

   /* Find inflows & outflows */
   n = Tank[i].Node;
   vnet = NodeDemand[n]*dt;
   vin = VolIn[n];
   vout = vin - vnet;
   if (vin > 0.0) cin = MassIn[n]/VolIn[n];
   else           cin = 0.0;
   Tank[i].V += vnet;
   Tank[i].V = MAX(0.0, Tank[i].V);                                            //(2.00.12 - LR)

   /* Withdraw flow from first segment */
   vsum = 0.0;
   csum = 0.0;
   while (vout > 0.0)
   {
      seg = FirstSeg[k];
      if (seg == NULL) break;
      vseg = seg->v;           /* Flow volume from leading seg */
      vseg = MIN(vseg,vout);
      if (seg == LastSeg[k]) vseg = vout;
      vsum += vseg;
      csum += (seg->c)*vseg;
      vout -= vseg;            /* Remaining flow volume */
      if (vout >= 0.0 && vseg >= seg->v)  /* Seg used up */
      {
         if (seg->prev)                                                        //(2.00.12 - LR)
         {                                                                     //(2.00.12 - LR)
            FirstSeg[k] = seg->prev;
            //if (FirstSeg[k] == NULL) LastSeg[k] = NULL;                      //(2.00.12 - LR)
            seg->prev = FreeSeg;
            FreeSeg = seg;
         }                                                                     //(2.00.12 - LR)
      }
      else                /* Remaining volume in segment */
      {
         seg->v -= vseg;
      }
   }

   /* Use quality withdrawn from 1st segment */
   /* to represent overall quality of tank */
   if (vsum > 0.0) Tank[i].C = csum/vsum;
   else            Tank[i].C = FirstSeg[k]->c;
   NodeQual[n] = Tank[i].C;

   /* Add new last segment for new flow entering tank */
   if (vin > 0.0)
   {
      if ( (seg = LastSeg[k]) != NULL)
      {
         /* Quality is the same, so just add flow volume to last seg */
         if (ABS(seg->c - cin) < m->Ctol) seg->v += vin;

         /* Otherwise add a new seg to tank */
         else addseg(m,k,vin,cin);
      }

      /* If no segs left then add a new one. */
      else addseg(m,k,vin,cin);
   }
}   


void  tankmix4(OW_Model *m, int i, long dt)
/*
**----------------------------------------------------------
**   Input:   i = tank index
**            dt = current WQ time step     
**   Output:  none
**   Purpose: Last In-First Out (LIFO) tank model                     
**----------------------------------------------------------
*/
{
   int   k, n;
   double vin, vnet, cin, vsum, csum, vseg;
   Pseg  seg, tmpseg;

  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeQual = m->NodeQual;
  Stank *Tank = m->Tank;
  int Nlinks = m->Nlinks;
  Pseg FreeSeg = m->FreeSeg;
  Pseg *FirstSeg = m->FirstSeg;
  Pseg *LastSeg = m->LastSeg;
  double *VolIn = m->VolIn;
  double *MassIn = m->MassIn;
  
   k = Nlinks + i;
   if (LastSeg[k] == NULL || FirstSeg[k] == NULL) return;

   /* React contents of each compartment */
   if (m->Reactflag)
   {
      seg = LastSeg[k];
      while (seg != NULL)
      {
         seg->c = tankreact(m,seg->c,seg->v,Tank[i].Kb,dt);
         seg = seg->prev;
      }
   }

   /* Find inflows & outflows */
   n = Tank[i].Node;
   vnet = NodeDemand[n]*dt;
   vin = VolIn[n];
   if (vin > 0.0) cin = MassIn[n]/VolIn[n];
   else           cin = 0.0;
   Tank[i].V += vnet;
   Tank[i].V = MAX(0.0, Tank[i].V);                                            //(2.00.12 - LR)
   Tank[i].C = LastSeg[k]->c;

   /* If tank filling, then create new last seg */ 
   if (vnet > 0.0)
   {
      if ( (seg = LastSeg[k]) != NULL)
      {
         /* Quality is the same, so just add flow volume to last seg */
         if (ABS(seg->c - cin) < m->Ctol) seg->v += vnet;

         /* Otherwise add a new last seg to tank */
         /* which points to old last seg */ 
         else
         {
            tmpseg = seg;
            LastSeg[k] = NULL;
            addseg(m,k,vnet,cin);
            LastSeg[k]->prev = tmpseg;
         }
      }

      /* If no segs left then add a new one. */
      else addseg(m,k,vnet,cin);

      /* Update reported tank quality */
      Tank[i].C = LastSeg[k]->c;
   }

   /* If net emptying then remove last segments until vnet consumed */
   else if (vnet < 0.0)
   {
      vsum = 0.0;
      csum = 0.0;
      vnet = -vnet;
      while (vnet > 0.0)
      {
         seg = LastSeg[k];
         if (seg == NULL) break;
         vseg = seg->v;
         vseg = MIN(vseg,vnet);
         if (seg == FirstSeg[k]) vseg = vnet;
         vsum += vseg;
         csum += (seg->c)*vseg;
         vnet -= vseg;
         if (vnet >= 0.0 && vseg >= seg->v)  /* Seg used up */
         {
            if (seg->prev)                                                     //(2.00.12 - LR)
            {                                                                  //(2.00.12 - LR)
               LastSeg[k] = seg->prev;
               //if (LastSeg[k] == NULL) FirstSeg[k] = NULL;                   //(2.00.12 - LR)
               seg->prev = FreeSeg;
               FreeSeg = seg;
            }                                                                  //(2.00.12 - LR)
         }
         else                /* Remaining volume in segment */
         {
            seg->v -= vseg;
         }
      }
      /* Reported tank quality is mixture of flow released and any inflow */
      Tank[i].C = (csum + MassIn[n])/(vsum + vin);
   }
   NodeQual[n] = Tank[i].C;
}         


double  sourcequal(OW_Model *m, Psource source)
/*
**--------------------------------------------------------------
**   Input:   j = source index
**   Output:  returns source WQ value
**   Purpose: determines source concentration in current time period  
**--------------------------------------------------------------
*/
{
   int   i;
   long  k;
   double c;

   /* Get source concentration (or mass flow) in original units */
   c = source->C0;

   /* Convert mass flow rate from min. to sec. */
   /* and convert concen. from liters to cubic feet */
   if (source->Type == MASS) c /= 60.0;
   else c /= m->Ucf[QUALITY];

   /* Apply time pattern if assigned */
   i = source->Pat;
   if (i == 0) return(c);
   k = ((m->Qtime + m->Pstart) / m->Pstep) % (long)m->Pattern[i].Length;
   return(c * m->Pattern[i].F[k]);
}


double  avgqual(OW_Model *m, int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index
**   Output:  returns WQ value
**   Purpose: computes average quality in link k                      
**--------------------------------------------------------------
*/
{
   double  vsum = 0.0,
          msum = 0.0;
   Pseg   seg;
  
  Pseg *FirstSeg = m->FirstSeg;

   if (m->Qualflag == NONE) return(0.);
   seg = FirstSeg[k];
   while (seg != NULL)
   {
       vsum += seg->v;
       msum += (seg->c)*(seg->v);
       seg = seg->prev;
   }
   if (vsum > 0.0) return(msum/vsum);
   else return( (m->NodeQual[m->Link[k].N1] + m->NodeQual[m->Link[k].N2])/2. );
}


void  ratecoeffs(OW_Model *m)
/*
**--------------------------------------------------------------
**   Input:   none                                                
**   Output:  none                                                
**   Purpose: determines wall reaction coeff. for each pipe       
**--------------------------------------------------------------
*/
{
   int   k;
   double kw;

   for (k=1; k <= m->Nlinks; k++)
   {
      kw = m->Link[k].Kw;
      if (kw != 0.0) kw = piperate(m,k);
      m->Link[k].Rc = kw;
      m->PipeRateCoeff[k] = 0.0;
   }
}                         /* End of ratecoeffs */


double piperate(OW_Model *m, int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index                                      
**   Output:  returns reaction rate coeff. for 1st-order wall     
**            reactions or mass transfer rate coeff. for 0-order  
**            reactions                                           
**   Purpose: finds wall reaction rate coeffs.                    
**--------------------------------------------------------------
*/
{
   double a,d,u,kf,kw,y,Re,Sh;
  
  double *Ucf = m->Ucf;
  Slink *Link = m->Link;
  double *LinkFlows = m->hydraulics.LinkFlows;

   d = Link[k].Diam;                    /* Pipe diameter, ft */

/* Ignore mass transfer if Schmidt No. is 0 */
   if (m->Sc == 0.0)
   {
      if (m->WallOrder == 0.0) return(BIG);
      else return(Link[k].Kw*(4.0/d)/Ucf[ELEV]);
   }

/* Compute Reynolds No. */
   a = PI*d*d/4.0;
   u = ABS(LinkFlows[k])/a;
   Re = u * d / m->Viscos;

/* Compute Sherwood No. for stagnant flow  */
/* (mass transfer coeff. = Diffus./radius) */
   if (Re < 1.0) Sh = 2.0;

/* Compute Sherwood No. for turbulent flow */
/* using the Notter-Sleicher formula.      */
   else if (Re >= 2300.0)
      Sh = 0.0149*pow(Re,0.88)*pow(m->Sc,0.333);

/* Compute Sherwood No. for laminar flow */
/* using Graetz solution formula.        */
   else
   {
      y = d/Link[k].Len*Re*m->Sc;
      Sh = 3.65+0.0668*y/(1.0+0.04*pow(y,0.667));
   }

/* Compute mass transfer coeff. (in ft/sec) */
   kf = Sh * m->Diffus / d;

/* For zero-order reaction, return mass transfer coeff. */
   if (m->WallOrder == 0.0) return(kf);

/* For first-order reaction, return apparent wall coeff. */
   kw = Link[k].Kw/Ucf[ELEV];       /* Wall coeff, ft/sec */
   kw = (4.0/d)*kw*kf/(kf+ABS(kw)); /* Wall coeff, 1/sec  */
   return(kw);
}                         /* End of piperate */


double  pipereact(OW_Model *m, int k, double c, double v, long dt)
/*
**------------------------------------------------------------
**   Input:   k = link index
**            c = current WQ in segment
**            v = segment volume
**            dt = time step
**   Output:  returns new WQ value
**   Purpose: computes new quality in a pipe segment after
**            reaction occurs              
**------------------------------------------------------------
*/
{
   double cnew, dc, dcbulk, dcwall, rbulk, rwall;
  
  Slink *Link = m->Link;

   /* For water age (hrs), update concentration by timestep */
   if (m->Qualflag == AGE) return(c+(double)dt/3600.0);

   /* Otherwise find bulk & wall reaction rates */
   rbulk = bulkrate(m,c,Link[k].Kb,m->BulkOrder)*m->Bucf;
   rwall = wallrate(m,c,Link[k].Diam,Link[k].Kw,Link[k].Rc);

   /* Find change in concentration over timestep */
   dcbulk = rbulk*(double)dt;
   dcwall = rwall*(double)dt;

   /* Update cumulative mass reacted */
   if (m->Htime >= m->Rstart)
   {
      m->Wbulk += ABS(dcbulk)*v;
      m->Wwall += ABS(dcwall)*v;
   }

   /* Update concentration */
   dc = dcbulk + dcwall;
   cnew = c + dc;
   cnew = MAX(0.0,cnew);
   return(cnew);
}


double  tankreact(OW_Model *m, double c, double v, double kb, long dt)
/*
**-------------------------------------------------------
**   Input:   c = current WQ in tank
**            v = tank volume
**            kb = reaction coeff.
**            dt = time step
**   Output:  returns new WQ value
**   Purpose: computes new quality in a tank after
**            reaction occurs
**-------------------------------------------------------
*/
{
   double cnew, dc, rbulk;

/*** Updated 9/7/00 ***/
   /* If no reaction then return current WQ */
   if (!m->Reactflag) return(c);

   /* For water age, update concentration by timestep */
   if (m->Qualflag == AGE) return(c + (double)dt/3600.0);

   /* Find bulk reaction rate */
   rbulk = bulkrate(m,c,kb,m->TankOrder)*m->Tucf;

   /* Find concentration change & update quality */
   dc = rbulk*(double)dt;
   if (m->Htime >= m->Rstart) m->Wtank += ABS(dc)*v;
   cnew = c + dc;
   cnew = MAX(0.0,cnew);
   return(cnew);
}
   

double  bulkrate(OW_Model *m, double c, double kb, double order)
/*
**-----------------------------------------------------------
**   Input:   c = current WQ concentration
**            kb = bulk reaction coeff.
**            order = bulk reaction order
**   Output:  returns bulk reaction rate
**   Purpose: computes bulk reaction rate (mass/volume/time)           
**-----------------------------------------------------------
*/
{
   double c1;

   /* Find bulk reaction potential taking into account */
   /* limiting potential & reaction order. */

      /* Zero-order kinetics: */
      if (order == 0.0) c = 1.0;

      /* Michaelis-Menton kinetics: */
      else if (order < 0.0)
      {
         c1 = m->Climit + SGN(kb)*c;
         if (ABS(c1) < TINY) c1 = SGN(c1)*TINY;
         c = c/c1;
      }

      /* N-th order kinetics: */
      else
      {
         /* Account for limiting potential */
         if (m->Climit == 0.0) c1 = c;
         else c1 = MAX(0.0, SGN(kb)*(m->Climit-c));

         /* Compute concentration potential */
         if (order == 1.0) c = c1;
         else if (order == 2.0) c = c1*c;
         else c = c1*pow(MAX(0.0,c),order-1.0);
      }

   /* Reaction rate = bulk coeff. * potential) */
   if (c < 0) c = 0;
   return(kb*c);
}


double  wallrate(OW_Model *m, double c, double d, double kw, double kf)
/*
**------------------------------------------------------------
**   Input:   c = current WQ concentration
**            d = pipe diameter
**            kw = intrinsic wall reaction coeff.
**            kf = mass transfer coeff. for 0-order reaction
**                 (ft/sec) or apparent wall reaction coeff.
**                 for 1-st order reaction (1/sec)
**   Output:  returns wall reaction rate in mass/ft3/sec
**   Purpose: computes wall reaction rate
**------------------------------------------------------------
*/
{
   if (kw == 0.0 || d == 0.0) return(0.0);
   if (m->WallOrder == 0.0)       /* 0-order reaction */
   {
      kf = SGN(kw)*c*kf;       /* Mass transfer rate (mass/ft2/sec)*/
      kw = kw*SQR(m->Ucf[ELEV]);  /* Reaction rate (mass/ft2/sec) */
      if (ABS(kf) < ABS(kw))   /* Reaction mass transfer limited */
         kw = kf;  
      return(kw*4.0/d);        /* Reaction rate (mass/ft3/sec) */
   }
   else return(c*kf);          /* 1st-order reaction */
}

/************************* End of QUALITY.C ***************************/
