/*
*********************************************************************
                                                                   
HYDRAUL.C --  Hydraulic Simulator for EPANET Program         
                                                                   
VERSION:    2.00
DATE:       6/5/00
            9/7/00
            10/25/00
            12/29/00
            3/1/01
            11/19/01
            6/24/02
            8/15/07    (2.00.11)
            2/14/08    (2.00.12)
AUTHOR:     L. Rossman
            US EPA - NRMRL
                                                                   
  This module contains the network hydraulic simulator.            
  It simulates the network's hydraulic behavior over an            
  an extended period of time and writes its results to the         
  binary file HydFile.
                                             
  The entry points for this module are:
     openhyd()    -- called from ENopenH() in EPANET.C
     inithyd()    -- called from ENinitH() in EPANET.C
     runhyd()     -- called from ENrunH() in EPANET.C
     nexthyd()    -- called from ENnextH() in EPANET.C
     closehyd()   -- called from ENcloseH() in EPANET.C
     tankvolume() -- called from ENsetnodevalue() in EPANET.C
     setlinkstatus(),
     setlinksetting(),
     resistance()-- all called from ENsetlinkvalue() in EPANET.C

  External functions called by this module are:
     createsparse() -- see SMATRIX.C
     freesparse()   -- see SMATRIX.C
     linsolve()     -- see SMATRIX.C
     checkrules()   -- see RULES.C
     interp()       -- see EPANET.C
     savehyd()      -- see OUTPUT.C
     savehydstep()  -- see OUTPUT.C
     writehydstat() -- see REPORT.C
     writehyderr()  -- see REPORT.C
     writehydwarn() -- see REPORT.C
*******************************************************************
*/
#include <math.h>
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

#define   QZERO  1.e-6  /* Equivalent to zero flow */
#define   CBIG   1.e8   /* Big coefficient         */
#define   CSMALL 1.e-6  /* Small coefficient       */

/* Constants used for computing Darcy-Weisbach friction factor */
#define A1  0.314159265359e04  /* 1000*PI */
#define A2  0.157079632679e04  /* 500*PI  */
#define A3  0.502654824574e02  /* 16*PI   */
#define A4  6.283185307        /* 2*PI    */
#define A8  4.61841319859      /* 5.74*(PI/4)^.9 */
#define A9  -8.685889638e-01   /* -2/ln(10)      */
#define AA  -1.5634601348      /* -2*.9*2/ln(10) */
#define AB  3.28895476345e-03  /* 5.74/(4000^.9) */
#define AC  -5.14214965799e-03 /* AA*AB */

/*** Updated 3/1/01 ***/
///* Flag used to halt taking further time steps */
//int Haltflag;
//
///* Relaxation factor used for updating flow changes */                         //(2.00.11 - LR)
//double RelaxFactor;                                                            //(2.00.11 - LR)

/* Function to find flow coeffs. through open/closed valves */                 //(2.00.11 - LR)
void valvecoeff(OW_Project *m, int k);                                                        //(2.00.11 - LR)


int  openhyd(OW_Project *m)
/*
 *--------------------------------------------------------------
 *  Input:   none     
 *  Output:  returns error code                                          
 *  Purpose: opens hydraulics solver system 
 *--------------------------------------------------------------
*/
{
   int  i;
   int  errcode = 0;
   ERRCODE(createsparse(m));     /* See SMATRIX.C  */
   ERRCODE(allocmatrix(m));      /* Allocate solution matrices */
  for (i=1; i <= m->Nlinks; i++) {   /* Initialize flows */
      initlinkflow(m,i,m->Link[i].Stat,m->Link[i].Kc);
  }
   return(errcode);
}


/*** Updated 3/1/01 ***/
void inithyd(OW_Project *m, int initflag)
/*
**--------------------------------------------------------------
**  Input:   initflag > 0 if link flows should be re-initialized
**                    = 0 if not
**  Output:  none                                          
**  Purpose: initializes hydraulics solver system 
**--------------------------------------------------------------
*/
{
   int i,j;

   /* Initialize tanks */
   for (i=1; i <= m->Ntanks; i++)
   {
      m->Tank[i].V = m->Tank[i].V0;
      m->hydraulics.NodeHead[m->Tank[i].Node] = m->Tank[i].H0;

/*** Updated 10/25/00 ***/
      m->hydraulics.NodeDemand[m->Tank[i].Node] = 0.0;

      m->hydraulics.OldStat[m->Nlinks+i] = TEMPCLOSED;
   }

   /* Initialize emitter flows */
   memset(m->hydraulics.EmitterFlows,0,(m->Nnodes+1)*sizeof(double));
   for (i=1; i <= m->Njuncs; i++)
      if (m->Node[i].Ke > 0.0) m->hydraulics.EmitterFlows[i] = 1.0;

   /* Initialize links */
   for (i=1; i <= m->Nlinks; i++)
   {
   /* Initialize status and setting */
      m->hydraulics.LinkStatus[i] = m->Link[i].Stat;
      m->hydraulics.LinkSetting[i] = m->Link[i].Kc;

      /* Start active control valves in ACTIVE position */                     //(2.00.11 - LR)
      if (
           (m->Link[i].Type == PRV || m->Link[i].Type == PSV
            || m->Link[i].Type == FCV)                                            //(2.00.11 - LR)
            && (m->Link[i].Kc != MISSING)
          ) {
        m->hydraulics.LinkStatus[i] = ACTIVE;                                                      //(2.00.11 - LR)
      }

/*** Updated 3/1/01 ***/
      /* Initialize flows if necessary */
     if (m->hydraulics.LinkStatus[i] <= CLOSED) {
       m->hydraulics.LinkFlows[i] = QZERO;
     }
      else if (ABS(m->hydraulics.LinkFlows[i]) <= QZERO || initflag > 0) {
         initlinkflow(m,i, m->hydraulics.LinkStatus[i], m->hydraulics.LinkSetting[i]);
      }

      /* Save initial status */
      m->hydraulics.OldStat[i] = m->hydraulics.LinkStatus[i];
   }

   /* Reset pump energy usage */
   for (i=1; i <= m->Npumps; i++)
   {
     for (j=0; j<6; j++) {
       m->Pump[i].Energy[j] = 0.0;
     }
   }

   /* Re-position hydraulics file */
  if (m->Saveflag) {
    fseek(m->HydFile,m->HydOffset,SEEK_SET);
  }

/*** Updated 3/1/01 ***/
   /* Initialize current time */
   m->Haltflag = 0;
   m->Htime = 0;
   m->Hydstep = 0;
   m->Rtime = m->Rstep;
}


int   runhyd(OW_Project *m, long *t)
/*
**--------------------------------------------------------------
**  Input:   none     
**  Output:  t = pointer to current time (in seconds)
**  Returns: error code                                          
**  Purpose: solves network hydraulics in a single time period 
**--------------------------------------------------------------
*/
{
   int   iter;                          /* Iteration count   */
   int   errcode;                       /* Error code        */
   double relerr;                        /* Solution accuracy */

   /* Find new demands & control actions */
   *t = m->Htime;
   demands(m);
   controls(m);

   /* Solve network hydraulic equations */
   errcode = netsolve(m, &iter,&relerr);
   if (!errcode)
   {
      /* Report new status & save results */
      if (m->Statflag) writehydstat(m,iter,relerr);

     /* solution info */
     m->_relativeError = relerr;
     m->_iterations = iter;
     
/*** Updated 3/1/01 ***/
      /* If system unbalanced and no extra trials */
      /* allowed, then activate the Haltflag.     */
      if (relerr > m->Hacc && m->ExtraIter == -1)
        m->Haltflag = 1;

      /* Report any warning conditions */
      if (!errcode) errcode = writehydwarn(m,iter,relerr);
   }
   return(errcode);
}                               /* end of runhyd */


int  nexthyd(OW_Project *m, long *tstep)
/*
**--------------------------------------------------------------
**  Input:   none     
**  Output:  tstep = pointer to time step (in seconds)
**  Returns: error code                                          
**  Purpose: finds length of next time step & updates tank
**           levels and rule-based contol actions 
**--------------------------------------------------------------
*/
{
   long  hydstep;         /* Actual time step  */
   int   errcode = 0;     /* Error code        */

/*** Updated 3/1/01 ***/
   /* Save current results to hydraulics file and */
   /* force end of simulation if Haltflag is active */
   if (m->Saveflag) errcode = savehyd(m,&(m->Htime));
   if (m->Haltflag) m->Htime = m->Dur;

   /* Compute next time step & update tank levels */
   *tstep = 0;
   hydstep = 0;
   if (m->Htime < m->Dur) hydstep = timestep(m);
   if (m->Saveflag) errcode = savehydstep(m,&hydstep);

   /* Compute pumping energy */
   if (m->Dur == 0) addenergy(m,0);
   else if (m->Htime < m->Dur) addenergy(m,hydstep);

   /* Update current time. */
   if (m->Htime < m->Dur)  /* More time remains */
   {
      m->Htime += hydstep;
      if (m->Htime >= m->Rtime) m->Rtime += m->Rstep;
   }
   else
   {
      m->Htime++;          /* Force completion of analysis */
     if (m->OpenQflag) {
       m->Qtime++; // force completion of wq analysis too
     }
   }
   *tstep = hydstep;
   return(errcode);
}
  

void  closehyd(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none     
**  Output:  returns error code                                          
**  Purpose: closes hydraulics solver system 
**--------------------------------------------------------------
*/
{
   freesparse(m);           /* see SMATRIX.C */
   freematrix(m);
}


int  allocmatrix(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns error code                                  
**  Purpose: allocates memory used for solution matrix coeffs.   
**--------------------------------------------------------------
*/
{
   int errcode = 0;
   m->hydraulics.solver.Aii = (double *) calloc(m->Nnodes+1,sizeof(double));
   m->hydraulics.solver.Aij = (double *) calloc(m->Ncoeffs+1,sizeof(double));
   m->hydraulics.solver.F   = (double *) calloc(m->Nnodes+1,sizeof(double));
   m->hydraulics.EmitterFlows   = (double *) calloc(m->Nnodes+1,sizeof(double));
   m->hydraulics.solver.P   = (double *) calloc(m->Nlinks+1,sizeof(double));
   m->hydraulics.solver.Y   = (double *) calloc(m->Nlinks+1,sizeof(double));
   m->X   = (double *) calloc(MAX((m->Nnodes+1),(m->Nlinks+1)),sizeof(double));
   m->hydraulics.OldStat = (char *) calloc(m->Nlinks+m->Ntanks+1, sizeof(char));
   ERRCODE(MEMCHECK(m->hydraulics.solver.Aii));
   ERRCODE(MEMCHECK(m->hydraulics.solver.Aij));
   ERRCODE(MEMCHECK(m->hydraulics.solver.F));
   ERRCODE(MEMCHECK(m->hydraulics.EmitterFlows));
   ERRCODE(MEMCHECK(m->hydraulics.solver.P));
   ERRCODE(MEMCHECK(m->hydraulics.solver.Y));
   ERRCODE(MEMCHECK(m->X));
   ERRCODE(MEMCHECK(m->hydraulics.OldStat));
   return(errcode);
}                               /* end of allocmatrix */


void  freematrix(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  none                                                
**  Purpose: frees memory used for solution matrix coeffs.       
**--------------------------------------------------------------
*/
{  
   free(m->hydraulics.solver.Aii);
   free(m->hydraulics.solver.Aij);
   free(m->hydraulics.solver.F);
   free(m->hydraulics.EmitterFlows);
   free(m->hydraulics.solver.P);
   free(m->hydraulics.solver.Y);
   free(m->X);
   free(m->hydraulics.OldStat);
}                               /* end of freematrix */


void  initlinkflow(OW_Project *m, int i, char s, double k)
/*
**--------------------------------------------------------------------
**  Input:   i = link index
**           s = link status
**           k = link setting (i.e., pump speed)
**  Output:  none                                                      
**  Purpose: sets initial flow in link to QZERO if link is closed,
**           to design flow for a pump, or to flow at velocity of
**           1 fps for other links.
**--------------------------------------------------------------------
*/
{
  
  if (s == CLOSED) {
    m->hydraulics.LinkFlows[i] = QZERO;
  }
  else if (m->Link[i].Type == PUMP) {
    int pumpIndex = m->Link[i].pumpLinkIdx;
    m->hydraulics.LinkFlows[i] = k * m->Pump[pumpIndex].Q0;
  }
  else {
    m->hydraulics.LinkFlows[i] = PI*SQR(m->Link[i].Diam)/4.0;
  }
}


/*** Updated 9/7/00 ***/
void  setlinkflow(OW_Project *m, int k, double dh)
/*
**--------------------------------------------------------------
**  Input:   k = link index
**           dh = headloss across link
**  Output:  none   
**  Purpose: sets flow in link based on current headloss                                              
**--------------------------------------------------------------
*/
{
   int   i,p;
   double h0;
   double  x,y;

   switch (m->Link[k].Type)
   {
       case CV:
       case PIPE:

       /* For Darcy-Weisbach formula: */
       /* use approx. inverse of formula. */
          if (m->Formflag == DW)
          {
             x = -log(m->hydraulics.LinkSetting[k]/3.7/m->Link[k].Diam);
             y = sqrt(ABS(dh)/m->Link[k].R/1.32547);
             m->hydraulics.LinkFlows[k] = x*y;
          }

       /* For Hazen-Williams or Manning formulas: */
       /* use inverse of formula. */
          else
          {
             x = ABS(dh) / m->Link[k].R;
             y = 1.0 / m->Hexp;
             m->hydraulics.LinkFlows[k] = pow(x,y);
          }

       /* Change sign of flow to match sign of headloss */
       if (dh < 0.0) {
         m->hydraulics.LinkFlows[k] = -(m->hydraulics.LinkFlows[k]);
       }
         break;

       case PUMP:

       /* Convert headloss to pump head gain */
          dh = -dh;
          p = m->Link[k].pumpLinkIdx;

       /* For custom pump curve, interpolate from curve */
          if (m->Pump[p].Ptype == CUSTOM)
          {
             dh = -dh * m->Ucf[HEAD] / SQR(m->hydraulics.LinkSetting[k]);
             i = m->Pump[p].Hcurve;
            double curvePoint = interp(m->Curve[i].Npts,
                                       m->Curve[i].Y,
                                       m->Curve[i].X,
                                       dh);
             m->hydraulics.LinkFlows[k] = curvePoint * m->hydraulics.LinkSetting[k] / m->Ucf[FLOW];
          }

       /* Otherwise use inverse of power curve */
          else
          {
             h0 = -SQR(m->hydraulics.LinkSetting[k])*m->Pump[p].H0;
             x = pow(m->hydraulics.LinkSetting[k], 2.0 - m->Pump[p].N);
             x = ABS(h0-dh)/(m->Pump[p].R*x),
             y = 1.0/m->Pump[p].N;
             m->hydraulics.LinkFlows[k] = pow(x,y);
          }
          break;
   }
}


void  setlinkstatus(OW_Project *m, int index, char value, char *s, double *k)
/*----------------------------------------------------------------
**  Input:   index  = link index
**           value  = 0 (CLOSED) or 1 (OPEN)
**           s      = pointer to link status
**           k      = pointer to link setting
**  Output:  none
**  Purpose: sets link status to OPEN or CLOSED 
**----------------------------------------------------------------
*/
{
   /* Status set to open */
   if (value == 1)
   {
      /* Adjust link setting for pumps & valves */
      if (m->Link[index].Type == PUMP) *k = 1.0;

/*** Updated 9/7/00 ***/
      if (m->Link[index].Type >  PUMP
      &&  m->Link[index].Type != GPV) *k = MISSING;

      /* Reset link flow if it was originally closed */
//      if (*s <= CLOSED) initlinkflow(index, OPEN, *k);
      *s = OPEN;
   }

   /* Status set to closed */ 
   else if (value == 0)
   {
      /* Adjust link setting for pumps & valves */
      if (m->Link[index].Type == PUMP) *k = 0.0;

/*** Updated 9/7/00 ***/
      if (m->Link[index].Type >  PUMP
      &&  m->Link[index].Type != GPV) *k = MISSING;
      
      /* Reset link flow if it was originally open */
//      if (*s > CLOSED) initlinkflow(index, CLOSED, *k);
      *s = CLOSED;
   }
}


void  setlinksetting(OW_Project *m, int index, double value, char *s, double *k)
/*----------------------------------------------------------------
**  Input:   index  = link index
**           value  = pump speed or valve setting
**           s      = pointer to link status
**           k      = pointer to link setting
**  Output:  none
**  Purpose: sets pump speed or valve setting, adjusting link
**           status and flow when necessary
**----------------------------------------------------------------
*/
{
   /* For a pump, status is OPEN if speed > 0, CLOSED otherwise */
   if (m->Link[index].Type == PUMP)
   {
      *k = value;
      if (value > 0 && *s <= CLOSED)
      {
         *s = OPEN;
//         initlinkflow(index, OPEN, value);
      }
      if (value == 0 && *s > CLOSED)
      {
          *s = CLOSED;
//          initlinkflow(index, CLOSED, value);
      }
   }

/***  Updated 9/7/00  ***/
   /* For FCV, activate it */
   else if (m->Link[index].Type == FCV)
   {
//      if (*s <= CLOSED) initlinkflow(index, OPEN, value);
      *k = value;
      *s = ACTIVE;
   }

   /* Open closed control valve with fixed status (setting = MISSING) */
   else
   {
      if (*k == MISSING && *s <= CLOSED)
      {
//         initlinkflow(index, OPEN, value);
         *s = OPEN;
      }
      *k = value;
   }
} 


void  resistance(OW_Project *m, int k)
/*
**--------------------------------------------------------------------
**  Input:   k = link index                                                      
**  Output:  none                                                      
**  Purpose: computes link flow resistance      
**--------------------------------------------------------------------
*/
{
  
   double e,d,L;
   m->Link[k].R = CSMALL;
   //if (Link[k].Type == PIPE || Link[k].Type == CV)                           //(2.00.11 - LR)
   switch (m->Link[k].Type)
   {

   /* Link is a pipe. Compute resistance based on headloss formula. */
   /* Friction factor for D-W formula gets included during solution */
   /* process in pipecoeff() function.                              */
       case CV:
       case PIPE: 
         e = m->Link[k].Kc;                 /* Roughness coeff. */
         d = m->Link[k].Diam;               /* Diameter */
         L = m->Link[k].Len;                /* Length */
         switch(m->Formflag)
         {
            case HW: m->Link[k].R = 4.727*L/pow(e,m->Hexp)/pow(d,4.871);
                     break;
            case DW: m->Link[k].R = L/2.0/32.2/d/SQR(PI*SQR(d)/4.0);
                     break;
            case CM: m->Link[k].R = SQR(4.0*e/(1.49*PI*d*d)) * pow((d/4.0),-1.333)*L;
         }
         break;

   /* Link is a pump. Use negligible resistance. */
      case PUMP:
         m->Link[k].R = CBIG;  //CSMALL;
         break;


   /* Link is a valve. Compute resistance for open valve assuming  */
   /* length is 2*diameter and friction factor is 0.02. Use with   */
   /* other formulas as well since resistance should be negligible.*/

/*** This way of treating valve resistance has been deprecated  ***/           //(2.00.11 - LR)
/*** since resulting resistance is not always negligible.       ***/           //(2.00.11 - LR)
/*
      default:
         d = Link[k].Diam;
         L = 2.0*d;
         Link[k].R = 0.02*L/2.0/32.2/d/SQR(PI*SQR(d)/4.0);
         break;
*/
   }
}


void  demands(OW_Project *m)
/*
**--------------------------------------------------------------------
**  Input:   none                                                      
**  Output:  none                                                      
**  Purpose: computes demands at nodes during current time period      
**--------------------------------------------------------------------
*/
{
   int i,j,n;
   long k,p;
   double djunc, sum;
   Pdemand demand;

   /* Determine total elapsed number of pattern periods */
   p = (m->Htime + m->Pstart) / m->Pstep;

   /* Update demand at each node according to its assigned pattern */
   m->Dsystem = 0.0;          /* System-wide demand */
   for (i=1; i<=m->Njuncs; i++)
   {
      sum = 0.0;
      for (demand = m->Node[i].D; demand != NULL; demand = demand->next)
      {
         /*
            pattern period (k) = (elapsed periods) modulus
                                 (periods per pattern)
         */
         j = demand->Pat;
         k = p % (long) m->Pattern[j].Length;
         djunc = (demand->Base) * m->Pattern[j].F[k] * m->Dmult;
         if (djunc > 0.0) {
           m->Dsystem += djunc;
         }
         sum += djunc;
      }
      m->hydraulics.NodeDemand[i] = sum;
   }

   /* Update head at fixed grade nodes with time patterns. */
   for (n=1; n <= m->Ntanks; n++)
   {
      if (m->Tank[n].A == 0.0)
      {
         j = m->Tank[n].Pat;
         if (j > 0)
         {
            k = p % (long) m->Pattern[j].Length;
            i = m->Tank[n].Node;
            m->hydraulics.NodeHead[i] = m->Node[i].El * m->Pattern[j].F[k];
         }
      }
   }

   /* Update status of pumps with utilization patterns */
   for (n=1; n <= m->Npumps; n++)
   {
      j = m->Pump[n].Upat;
      if (j > 0)
      {
         i = m->Pump[n].Link;
         k = p % (long) m->Pattern[j].Length;
         setlinksetting(m, i, m->Pattern[j].F[k], &m->hydraulics.LinkStatus[i], &m->hydraulics.LinkSetting[i]);
      }
   }
}                        /* End of demands */


int  controls(OW_Project *m)
/*
**---------------------------------------------------------------------
**  Input:   none                                                   
**  Output:  number of links whose setting changes                  
**  Purpose: implements simple controls based on time or tank levels  
**---------------------------------------------------------------------
*/
{
   int   i, k, n, reset, setsum;
   double h, vplus;
   double v1, v2;
   double k1, k2;
   char  s1, s2;

   /* Examine each control statement */
   setsum = 0;
   for (i=1; i <= m->Ncontrols; i++)
   {
     Scontrol *control = &(m->Control[i]);
      /* Make sure that link is defined */
      reset = 0;
      if ( (k = control->Link) <= 0) continue;

      /* Link is controlled by tank level */
      if ((n = control->Node) > 0 && n > m->Njuncs)
      {
         h = m->hydraulics.NodeHead[n];
         vplus = ABS(m->hydraulics.NodeDemand[n]);
         v1 = tankvolume(m, n - m->Njuncs, h);
         v2 = tankvolume(m, n - m->Njuncs, m->Control[i].Grade);
         if (control->Type == LOWLEVEL && v1 <= v2 + vplus)
            reset = 1;
         if (control->Type == HILEVEL && v1 >= v2 - vplus)
            reset = 1;
      }

      /* Link is time-controlled */
      if (control->Type == TIMER)
      {
          if (control->Time == m->Htime) reset = 1;
      }

      /* Link is time-of-day controlled */
      if (control->Type == TIMEOFDAY)
      {
          if ((m->Htime + m->Tstart) % SECperDAY == control->Time) reset = 1;
      }

      /* Update link status & pump speed or valve setting */
      if (reset == 1)
      {
         if (m->hydraulics.LinkStatus[k] <= CLOSED) s1 = CLOSED;
         else                s1 = OPEN;
         s2 = control->Status;
         k1 = m->hydraulics.LinkSetting[k];
         k2 = k1;
         if (m->Link[k].Type > PIPE) k2 = control->Setting;
         if (s1 != s2 || k1 != k2)
         {
            m->hydraulics.LinkStatus[k] = s2;
            m->hydraulics.LinkSetting[k] = k2;
           if (m->Statflag) {
             writecontrolaction(m,k,i);
           }
 //           if (s1 != s2) initlinkflow(k, S[k], K[k]);
            setsum++;
         }
      }   
   }
   return(setsum);
}                        /* End of controls */


long  timestep(OW_Project *m)
/*
**----------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns time step until next change in hydraulics   
**  Purpose: computes time step to advance hydraulic simulation  
**----------------------------------------------------------------
*/
{
   long   n,t,tstep;

   /* Normal time step is hydraulic time step */
   tstep = m->Hstep;

   /* Revise time step based on time until next demand period */
   n = ((m->Htime + m->Pstart) / m->Pstep) + 1;   /* Next pattern period   */
   t = n * m->Pstep - m->Htime;              /* Time till next period */
   if (t > 0 && t < tstep) tstep = t;

   /* Revise time step based on time until next reporting period */
   t = m->Rtime - m->Htime;
   if (t > 0 && t < tstep) {
     tstep = t;
   }
  
   /* Revise time step based on smallest time to fill or drain a tank */
   tanktimestep(m, &tstep);

   /* Revise time step based on smallest time to activate a control */
   controltimestep(m, &tstep);

   /* Evaluate rule-based controls (which will also update tank levels) */
   if (m->Nrules > 0)
     ruletimestep(m, &tstep);
   else
     tanklevels(m, tstep);
   return(tstep);
}


void  tanktimestep(OW_Project *m, long *tstep)
/*
**-----------------------------------------------------------------
**  Input:   *tstep = current time step                                                
**  Output:  *tstep = modified current time step   
**  Purpose: revises time step based on shortest time to fill or
**           drain a tank  
**-----------------------------------------------------------------
*/
{
   int    i,n;   
   double  h,q,v;
   long   t;

   /* (D[n] is net flow rate into (+) or out of (-) tank at node n) */
   for (i=1; i <= m->Ntanks; i++)
   {
      if (m->Tank[i].A == 0.0) continue;           /* Skip reservoirs     */
      n = m->Tank[i].Node;
      h = m->hydraulics.NodeHead[n];                                 /* Current tank grade  */
      q = m->hydraulics.NodeDemand[n];                                 /* Flow into tank      */
      if (ABS(q) <= QZERO) continue;
      if (q > 0.0 && h < m->Tank[i].Hmax)
      {
         v = m->Tank[i].Vmax - m->Tank[i].V;          /* Volume to fill      */
      }
      else if (q < 0.0 && h > m->Tank[i].Hmin)
      {
         v = m->Tank[i].Vmin - m->Tank[i].V;          /* Volume to drain (-) */
      }
      else continue;
      t = (long)ROUND(v/q);                     /* Time to fill/drain  */
      if (t > 0 && t < *tstep) *tstep = t;
   }
}


void  controltimestep(OW_Project *m, long *tstep)
/*
**------------------------------------------------------------------
**  Input:   *tstep = current time step                                                
**  Output:  *tstep = modified current time step   
**  Purpose: revises time step based on shortest time to activate
**           a simple control  
**------------------------------------------------------------------
*/
{
   int   i,j,k,n;
   double h,q,v;
   long  t,t1,t2;

   for (i=1; i <= m->Ncontrols; i++)
   {
      t = 0;
      Scontrol *control = &(m->Control[i]);
      if ( (n = control->Node) > 0)           /* Node control:       */
      {
         if ((j = n - m->Njuncs) <= 0) continue;     /* Node is a tank      */
         h = m->hydraulics.NodeHead[n];                              /* Current tank grade  */
         q = m->hydraulics.NodeDemand[n];                              /* Flow into tank      */
         if (ABS(q) <= QZERO) {
           continue;
         }
         if
         ( (h < control->Grade &&
            control->Type == HILEVEL &&       /* Tank below hi level */
            q > 0.0)                            /* & is filling        */
         || (h > control->Grade &&
             control->Type == LOWLEVEL &&     /* Tank above low level */
             q < 0.0)                           /* & is emptying        */
         )
         {                                      /* Time to reach level  */
            v = tankvolume(m, j,control->Grade) - m->Tank[j].V;
            t = (long)ROUND(v/q);
         }
      }

      if (control->Type == TIMER)             /* Time control:        */
      {              
         if (control->Time > m->Htime)
             t = control->Time - m->Htime;
      }

      if (control->Type == TIMEOFDAY)         /* Time-of-day control: */
      {
         t1 = (m->Htime + m->Tstart) % SECperDAY;
         t2 = control->Time;
         if (t2 >= t1) t = t2 - t1;
         else t = SECperDAY - t1 + t2;
      }

      if (t > 0 && t < *tstep)               /* Revise time step     */
      {
         /* Check if rule actually changes link status or setting */
         k = control->Link;
         if (
              (m->Link[k].Type > PIPE && m->hydraulics.LinkSetting[k] != control->Setting) ||
              (m->hydraulics.LinkStatus[k] != control->Status)
            )
            *tstep = t;
      }
   }
}                        /* End of timestep */


void  ruletimestep(OW_Project *m, long *tstep)
/*
**--------------------------------------------------------------
**  Input:   *tstep = current time step (sec)                            
**  Output:  *tstep = modified time step                                               
**  Purpose: updates next time step by checking if any rules
**           will fire before then; also updates tank levels.                               
**--------------------------------------------------------------
*/
{
   long tnow,      /* Start of time interval for rule evaluation */
        tmax,      /* End of time interval for rule evaluation   */
        dt,        /* Normal time increment for rule evaluation  */
        dt1;       /* Actual time increment for rule evaluation  */

   /* Find interval of time for rule evaluation */
   tnow = m->Htime;
   tmax = tnow + *tstep;

   /* If no rules, then time increment equals current time step */
   if (m->Nrules == 0)
   {
      dt = *tstep;
      dt1 = dt;
   }

   /* Otherwise, time increment equals rule evaluation time step and */
   /* first actual increment equals time until next even multiple of */
   /* Rulestep occurs. */
   else
   {
      dt = m->Rulestep;
      dt1 = m->Rulestep - (tnow % m->Rulestep);
   }

   /* Make sure time increment is no larger than current time step */
   dt = MIN(dt, *tstep);
   dt1 = MIN(dt1, *tstep);
   if (dt1 == 0) dt1 = dt;

   /* Step through time, updating tank levels, until either  */
   /* a rule fires or we reach the end of evaluation period. */
   /*
   ** Note: we are updating the global simulation time (Htime)
   **       here because it is used by functions in RULES.C
   **       to evaluate rules when checkrules() is called.
   **       It is restored to its original value after the
   **       rule evaluation process is completed (see below).
   **       Also note that dt1 will equal dt after the first
   **       time increment is taken.
   */
   do
   {
      m->Htime += dt1;               /* Update simulation clock */
      tanklevels(m, dt1);            /* Find new tank levels    */
      if (checkrules(m,dt1)) break; /* Stop if rules fire      */
      dt = MIN(dt, tmax - m->Htime); /* Update time increment   */
      dt1 = dt;                   /* Update actual increment */
   }  while (dt > 0);             /* Stop if no time left    */

   /* Compute an updated simulation time step (*tstep) */
   /* and return simulation time to its original value */
   *tstep = m->Htime - tnow;
   m->Htime = tnow;
}
   

void  addenergy(OW_Project *mod, long hstep)
/*
**-------------------------------------------------------------
**  Input:   hstep = time step (sec)                            
**  Output:  none                                               
**  Purpose: accumulates pump energy usage                               
**-------------------------------------------------------------
*/
{
    int   i,j,k;
    long  m,n;
    double c0,c,             /* Energy cost (cost/kwh) */
          f0,               /* Energy cost factor */
          dt,               /* Time interval (hr) */
          e,                /* Pump efficiency (fraction) */
          q,                /* Pump flow (cfs) */
          p,                /* Pump energy (kw) */
          psum = 0.0;       /* Total energy (kw) */

   /* Determine current time interval in hours */
  if      (mod->Dur == 0) {
    dt = 1.0;
  }
  else if (mod->Htime < mod->Dur) {
    dt = (double) hstep / 3600.0;
  }
  else {
    dt = 0.0;
  }
  if (dt == 0.0) {
    return;
  }
   n = (mod->Htime + mod->Pstart) / mod->Pstep;

   /* Compute default energy cost at current time */
   c0 = mod->Ecost;
   f0 = 1.0;
   if (mod->Epat > 0)
   {
      m = n % (long)mod->Pattern[mod->Epat].Length;
      f0 = mod->Pattern[mod->Epat].F[m];
   }

   /* Examine each pump */
   for (j=1; j <= mod->Npumps; j++)
   {
     Spump *pump = &(mod->Pump[j]);
     
      /* Skip closed pumps */
      k = pump->Link;
      if (mod->hydraulics.LinkStatus[k] <= CLOSED) {
        continue;
      }
      q = MAX(QZERO, ABS(mod->hydraulics.LinkFlows[k]));

      /* Find pump-specific energy cost */
      if (pump->Ecost > 0.0) {
        c = pump->Ecost;
      }
      else c = c0;

      if ( (i = pump->Epat) > 0)
      {
          m = n % (long)mod->Pattern[i].Length;
          c *= mod->Pattern[i].F[m];
      }
      else c *= f0;

      /* Find pump energy & efficiency */
      getenergy(mod, k,&p,&e);
      psum += p;

      /* Update pump's cumulative statistics */
      pump->Energy[0] += dt;            /* Time on-line */
      pump->Energy[1] += e*dt;          /* Effic.-hrs   */
      pump->Energy[2] += p/q*dt;        /* kw/cfs-hrs   */
      pump->Energy[3] += p*dt;          /* kw-hrs       */
      pump->Energy[4] = MAX(pump->Energy[4],p);
      pump->Energy[5] += c*p*dt;        /* cost-hrs.    */
   }

   /* Update maximum kw value */
   mod->Emax = MAX(mod->Emax,psum);
}                       /* End of pumpenergy */


void  getenergy(OW_Project *m, int k, double *kw, double *eff)
/*
**----------------------------------------------------------------
**  Input:   k    = link index                         
**  Output:  *kw  = kwatt energy used
**           *eff = efficiency (pumps only)
**  Purpose: computes flow energy associated with link k                                           
**----------------------------------------------------------------
*/
{
   int   i,j;
   double dh, q, e;

/*** Updated 6/24/02 ***/
   /* No energy if link is closed */
   if (m->hydraulics.LinkStatus[k] <= CLOSED)
   {
      *kw = 0.0;
      *eff = 0.0;
      return;
   }
/*** End of update ***/

   /* Determine flow and head difference */
   q = ABS(m->hydraulics.LinkFlows[k]);
   dh = ABS(m->hydraulics.NodeHead[m->Link[k].N1] - m->hydraulics.NodeHead[m->Link[k].N2]);

   /* For pumps, find effic. at current flow */
   if (m->Link[k].Type == PUMP)
   {
      j = m->Link[k].pumpLinkIdx;
      e = m->Epump;
      if ( (i = m->Pump[j].Ecurve) > 0) {
         e = interp(m->Curve[i].Npts,
                    m->Curve[i].X,
                    m->Curve[i].Y,
                    q * m->Ucf[FLOW]);
      }
      e = MIN(e, 100.0);
      e = MAX(e, 1.0);
      e /= 100.0;
   }
   else e = 1.0;

   /* Compute energy */
   *kw = dh * q * m->SpGrav / 8.814 / e * KWperHP;
   *eff = e;
}


void  tanklevels(OW_Project *m, long tstep)
/*
**----------------------------------------------------------------
**  Input:   tstep = current time step                         
**  Output:  none                                                
**  Purpose: computes new water levels in tanks after current    
**           time step                                           
**----------------------------------------------------------------
*/
{
   int   i,n;
   double dv;

   for (i=1; i <= m->Ntanks; i++)
   {

      /* Skip reservoirs */
      if (m->Tank[i].A == 0.0) continue;

      /* Update the tank's volume & water elevation */
      n = m->Tank[i].Node;
      dv = m->hydraulics.NodeDemand[n]*tstep;
      m->Tank[i].V += dv;

      /*** Updated 6/24/02 ***/
      /* Check if tank full/empty within next second */
      if (m->Tank[i].V + m->hydraulics.NodeDemand[n] >= m->Tank[i].Vmax) {
        m->Tank[i].V = m->Tank[i].Vmax;
      }
      else if (m->Tank[i].V - m->hydraulics.NodeDemand[n] <= m->Tank[i].Vmin) {
        m->Tank[i].V = m->Tank[i].Vmin;
      }
      m->hydraulics.NodeHead[n] = tankgrade(m, i,m->Tank[i].V);
   }
}                       /* End of tanklevels */


double  tankvolume(OW_Project *m, int i, double h)
/*
**--------------------------------------------------------------------
**  Input:   i = tank index                                         
**           h = water elevation in tank                                
**  Output:  returns water volume in tank                           
**  Purpose: finds water volume in tank i corresponding to elev. h. 
**--------------------------------------------------------------------
*/
{  
   int j;

   /* Use level*area if no volume curve */
   j = m->Tank[i].Vcurve;
   if (j == 0) {
     return(m->Tank[i].Vmin + ((h - m->Tank[i].Hmin) * m->Tank[i].A));
   }

   /* If curve exists, interpolate on h to find volume v */
   /* remembering that volume curve is in original units.*/
   else {
     return(
            interp(
                   m->Curve[j].Npts,
                   m->Curve[j].X,
                   m->Curve[j].Y,
                   (h - m->Node[m->Tank[i].Node].El) * m->Ucf[HEAD]) / m->Ucf[VOLUME]
            );
   }

}                       /* End of tankvolume */


double  tankgrade(OW_Project *m, int i, double v)
/*
**-------------------------------------------------------------------
**  Input:   i = tank index                                         
**           v = volume in tank                                     
**  Output:  returns water level in tank                            
**  Purpose: finds water level in tank i corresponding to volume v. 
**-------------------------------------------------------------------
*/
{
  
   int j;

   /* Use area if no volume curve */
   j = m->Tank[i].Vcurve;
   if (j == 0) {
     return(m->Tank[i].Hmin + (v - m->Tank[i].Vmin)/m->Tank[i].A);
   }

   /* If curve exists, interpolate on volume (originally the Y-variable */
   /* but used here as the X-variable) to find new level above bottom.  */
   /* Remember that volume curve is stored in original units.           */
   else {
     double curvePt = interp(m->Curve[j].Npts,
                             m->Curve[j].Y,
                             m->Curve[j].X,
                             v * m->Ucf[VOLUME]);
     
     return(m->Node[m->Tank[i].Node].El + curvePt / m->Ucf[HEAD]);
   }

}                        /* End of tankgrade */


int  netsolve(OW_Project *m, int *iter, double *relerr)
/*
**-------------------------------------------------------------------
**  Input:   none                                                
**  Output:  *iter   = # of iterations to reach solution         
**           *relerr = convergence error in solution             
**           returns error code                                  
**  Purpose: solves network nodal equations for heads and flows    
**           using Todini's Gradient algorithm                   
**
*** Updated 9/7/00 *** 
*** Updated 2.00.11 ***
*** Updated 2.00.12 ***                                                            
**  Notes:   Status checks on CVs, pumps and pipes to tanks are made
**           every CheckFreq iteration, up until MaxCheck iterations
**           are reached. Status checks on control valves are made
**           every iteration if DampLimit = 0 or only when the
**           convergence error is at or below DampLimit. If DampLimit
**           is > 0 then future computed flow changes are only 60% of
**           their full value. A complete status check on all links
**           is made when convergence is achieved. If convergence is
**           not achieved in MaxIter trials and ExtraIter > 0 then
**           another ExtraIter trials are made with no status changes
**           made to any links and a warning message is generated.
**                                                             
**   This procedure calls linsolve() which appears in SMATRIX.C. 
**-------------------------------------------------------------------
*/
{
   int    i;                     /* Node index */
   int    errcode = 0;           /* Node causing solution error */
   int    nextcheck;             /* Next status check trial */
   int    maxtrials;             /* Max. trials for convergence */
   double newerr;                /* New convergence error */
   int    valveChange;           /* Valve status change flag */
   int    statChange;

   /* Initialize status checking & relaxation factor */   
   nextcheck = m->CheckFreq;
   m->RelaxFactor = 1.0;
  
   /* Repeat iterations until convergence or trial limit is exceeded. */
   /* (ExtraIter used to increase trials in case of status cycling.)  */
  
   if (m->Statflag == FULL)
     writerelerr(m,0,0);

   maxtrials = m->MaxIter;
  
   if (m->ExtraIter > 0)
     maxtrials += m->ExtraIter;
  
   *iter = 1;
   while (*iter <= maxtrials)
   {
      /*
      ** Compute coefficient matrices A & F and solve A*H = F 
      ** where H = heads, A = Jacobian coeffs. derived from 
      ** head loss gradients, & F = flow correction terms.
      ** Solution for H is returned in F from call to linsolve().
      */
      newcoeffs(m);
      errcode = linsolve(m, m->Njuncs, m->hydraulics.solver.Aii, m->hydraulics.solver.Aij, m->hydraulics.solver.F);

      /* Take action depending on error code */
      if (errcode < 0) break;    /* Memory allocation problem */
      if (errcode > 0)           /* Ill-conditioning problem */
      {
         /* If control valve causing problem, fix its status & continue, */
         /* otherwise end the iterations with no solution.               */
         if (badvalve(m, m->hydraulics.solver.Order[errcode])) continue;
         else break;
      }

      /* Update current solution. */
      /* (Row[i] = row of solution matrix corresponding to node i). */
      for (i=1; i <= m->Njuncs; i++) {
        double head = m->hydraulics.solver.F[m->hydraulics.solver.Row[i]];
        m->hydraulics.NodeHead[i] = head;   /* Update heads */
      }
      newerr = newflows(m);                          /* Update flows */
      *relerr = newerr;

      /* Write convergence error to status report if called for */
      if (m->Statflag == FULL) writerelerr(m,*iter,*relerr);

      /* Apply solution damping & check for change in valve status */
      m->RelaxFactor = 1.0;
      valveChange = FALSE;
      if ( m->DampLimit > 0.0 )
      {
          if( *relerr <= m->DampLimit )
          {
             m->RelaxFactor = 0.6;
             valveChange = valvestatus(m);
          }
      }
      else valveChange = valvestatus(m);

      /* Check for convergence */
      if (*relerr <= m->Hacc)
      {
         /* We have convergence. Quit if we are into extra iterations. */
         if (*iter > m->MaxIter) break;

         /* Quit if no status changes occur. */
         statChange = FALSE;
         if (valveChange)  statChange = TRUE;
         if (linkstatus(m)) statChange = TRUE;
         if (pswitch(m))    statChange = TRUE;
         if (!statChange)  break;

         /* We have a status change so continue the iterations */
         nextcheck = *iter + m->CheckFreq;
      }

      /* No convergence yet. See if its time for a periodic status */
      /* check  on pumps, CV's, and pipes connected to tanks.      */
      else if (*iter <= m->MaxCheck && *iter == nextcheck)
      {
         linkstatus(m);
         nextcheck += m->CheckFreq;
      }
      (*iter)++;
   }

   /* Iterations ended. Report any errors. */
   if (errcode < 0) errcode = 101;      /* Memory allocation error */
   else if (errcode > 0)
   {
      writehyderr(m,m->hydraulics.solver.Order[errcode]);      /* Ill-conditioned eqns. error */
      errcode = 110;
   }

   /* Add any emitter flows to junction demands */
   for (i=1; i <= m->Njuncs; i++) {
     m->hydraulics.NodeDemand[i] += m->hydraulics.EmitterFlows[i];
   }
   return(errcode);
}                        /* End of netsolve */


int  badvalve(OW_Project *m, int n)
/*
**-----------------------------------------------------------------
**  Input:   n = node index                                                
**  Output:  returns 1 if node n belongs to an active control valve,
**           0 otherwise  
**  Purpose: determines if a node belongs to an active control valve
**           whose setting causes an inconsistent set of eqns. If so,
**           the valve status is fixed open and a warning condition
**           is generated.
**-----------------------------------------------------------------
*/
{
   int i,k,n1,n2;
   for (i=1; i <= m->Nvalves; i++)
   {
      k = m->Valve[i].Link;
      n1 = m->Link[k].N1;
      n2 = m->Link[k].N2;
      if (n == n1 || n == n2)
      {
         if (m->Link[k].Type == PRV ||
             m->Link[k].Type == PSV ||
             m->Link[k].Type == FCV)
         {
            if (m->hydraulics.LinkStatus[k] == ACTIVE)
            {
               if (m->Statflag == FULL)
               {
                  sprintf(m->Msg,FMT61,clocktime(m->Atime,m->Htime),m->Link[k].ID);
                  writeline(m,m->Msg);
               }
              if (m->Link[k].Type == FCV) {
                m->hydraulics.LinkStatus[k] = XFCV;
              }
              else {
                m->hydraulics.LinkStatus[k] = XPRESSURE;
              }
               return(1);
            }
         }
         return(0);
      }
   }
   return(0);
}
   

int  valvestatus(OW_Project *m)
/*
**-----------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns 1 if any pressure or flow control valve                   //(2.00.11 - LR)
**           changes status, 0 otherwise                                       //(2.00.11 - LR) 
**  Purpose: updates status for PRVs & PSVs whose status                       //(2.00.12 - LR)
**           is not fixed to OPEN/CLOSED
**-----------------------------------------------------------------
*/
{
   int   change = FALSE,            /* Status change flag      */
         i,k,                       /* Valve & link indexes    */
         n1,n2;                     /* Start & end nodes       */
   char  s;                         /* Valve status settings   */
   double hset;                     /* Valve head setting      */

   for (i=1; i <= m->Nvalves; i++)                   /* Examine each valve   */
   {
      k = m->Valve[i].Link;                        /* Link index of valve  */
      if (m->hydraulics.LinkSetting[k] == MISSING) continue;            /* Valve status fixed   */
      n1 = m->Link[k].N1;                          /* Start & end nodes    */
      n2 = m->Link[k].N2;
      s  = m->hydraulics.LinkStatus[k];                                /* Save current status  */

//      if (s != CLOSED                           /* No change if flow is */  //(2.00.11 - LR)
//      && ABS(Q[k]) < Qtol) continue;            /* negligible.          */  //(2.00.11 - LR)

      switch (m->Link[k].Type)                     /* Evaluate new status: */
      {
         case PRV:  hset = m->Node[n2].El + m->hydraulics.LinkSetting[k];
                    m->hydraulics.LinkStatus[k] = prvstatus(m,k,s,hset,m->hydraulics.NodeHead[n1],m->hydraulics.NodeHead[n2]);
                    break;
         case PSV:  hset = m->Node[n1].El + m->hydraulics.LinkSetting[k];
                    m->hydraulics.LinkStatus[k] = psvstatus(m,k,s,hset,m->hydraulics.NodeHead[n1],m->hydraulics.NodeHead[n2]);
                    break;

////  FCV status checks moved back into the linkstatus() function ////           //(2.00.12 - LR)
//         case FCV:  S[k] = fcvstatus(k,s,NodeHead[n1],NodeHead[n2]);                         //(2.00.12 - LR)
//                    break;                                                     //(2.00.12 - LR)

         default:   continue;
      }

/*** Updated 9/7/00 ***/
      /* Do not reset flow in valve if its status changes. */
      /* This strategy improves convergence. */

      /* Check for status change */
      if (s != m->hydraulics.LinkStatus[k])
      {
         if (m->Statflag == FULL)
           writestatchange(m,k,s,m->hydraulics.LinkStatus[k]);
         change = TRUE;
      }
   }
   return(change);
}                       /* End of valvestatus() */


/*** Updated 9/7/00 ***/
int  linkstatus(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns 1 if any link changes status, 0 otherwise   
**  Purpose: determines new status for pumps, CVs, FCVs & pipes                //(2.00.12 - LR)
**           to tanks.                                              
**--------------------------------------------------------------
*/
{
  
  
   int   change = FALSE,             /* Status change flag      */
         k,                          /* Link index              */
         n1,                         /* Start node index        */
         n2;                         /* End node index          */
   double dh;                        /* Head difference         */
   char  status;                     /* Current status          */

   /* Examine each link */
   for (k=1; k <= m->Nlinks; k++)
   {
      n1 = m->Link[k].N1;
      n2 = m->Link[k].N2;
      dh = m->hydraulics.NodeHead[n1] - m->hydraulics.NodeHead[n2];

      /* Re-open temporarily closed links (status = XHEAD or TEMPCLOSED) */
      status = m->hydraulics.LinkStatus[k];
     if (status == XHEAD || status == TEMPCLOSED) {
       m->hydraulics.LinkStatus[k] = OPEN;
     }

      /* Check for status changes in CVs and pumps */
     if (m->Link[k].Type == CV) {
       m->hydraulics.LinkStatus[k] = cvstatus(m,m->hydraulics.LinkStatus[k],dh,m->hydraulics.LinkFlows[k]);
     }
     if (m->Link[k].Type == PUMP && m->hydraulics.LinkStatus[k] >= OPEN && m->hydraulics.LinkSetting[k] > 0.0) {                 //(2.00.11 - LR)
         m->hydraulics.LinkStatus[k] = pumpstatus(m,k,-dh);
     }

      /* Check for status changes in non-fixed FCVs */
      if (m->Link[k].Type == FCV && m->hydraulics.LinkSetting[k] != MISSING)                              //(2.00.12 - LR)//
         m->hydraulics.LinkStatus[k] = fcvstatus(m,k,status, m->hydraulics.NodeHead[n1], m->hydraulics.NodeHead[n2]);                               //(2.00.12 - LR)//

      /* Check for flow into (out of) full (empty) tanks */
     if (n1 > m->Njuncs || n2 > m->Njuncs) {
       tankstatus(m,k,n1,n2);
     }

      /* Note change in link status; do not revise link flow */                //(2.00.11 - LR)
      if (status != m->hydraulics.LinkStatus[k])
      {
         change = TRUE;
        if (m->Statflag == FULL) {
          writestatchange(m,k,status,m->hydraulics.LinkStatus[k]);
        }

         //if (S[k] <= CLOSED) Q[k] = QZERO;                                   //(2.00.11 - LR)
         //else setlinkflow(k, dh);                                            //(2.00.11 - LR)
      }
   }
   return(change);
}                        /* End of linkstatus */


char  cvstatus(OW_Project *m, char s, double dh, double q)
/*
**--------------------------------------------------
**  Input:   s  = current status
**           dh = headloss
**           q  = flow
**  Output:  returns new link status                 
**  Purpose: updates status of a check valve.        
**--------------------------------------------------
*/
{
   /* Prevent reverse flow through CVs */
   if (ABS(dh) > m->Htol)
   {
      if (dh < -(m->Htol))
        return(CLOSED);
      else if (q < -(m->Qtol))
        return(CLOSED);
      else
        return(OPEN);
   }
   else
   {
      if (q < -(m->Qtol))
        return(CLOSED);
      else
        return(s);
   }
}


char  pumpstatus(OW_Project *m, int k, double dh)
/*
**--------------------------------------------------
**  Input:   k  = link index                         
**           dh = head gain
**  Output:  returns new pump status                 
**  Purpose: updates status of an open pump.               
**--------------------------------------------------
*/
{
   int   p;
   double hmax;

   /* Prevent reverse flow through pump */
   p = m->Link[k].pumpLinkIdx;
  if (m->Pump[p].Ptype == CONST_HP) {
    hmax = BIG;
  }
  else {
    hmax = SQR(m->hydraulics.LinkSetting[k]) * m->Pump[p].Hmax;
  }
  if (dh > hmax + m->Htol) {
    return(XHEAD);
  }

/*** Flow higher than pump curve no longer results in a status change ***/     //(2.00.11 - LR)
   /* Check if pump cannot deliver flow */                                     //(2.00.11 - LR)
   //if (Q[k] > K[k]*Pump[p].Qmax + Qtol) return(XFLOW);                       //(2.00.11 - LR)
   return(OPEN);
}


char  prvstatus(OW_Project *m, int k, char s, double hset, double h1, double h2)
/*
**-----------------------------------------------------------
**  Input:   k    = link index                                
**           s    = current status                            
**           hset = valve head setting                        
**           h1   = head at upstream node                     
**           h2   = head at downstream node                   
**  Output:  returns new valve status                         
**  Purpose: updates status of a pressure reducing valve.     
**-----------------------------------------------------------
*/
{
   char  status;     /* New valve status */
   double hml;        /* Minor headloss   */
   double htol = m->Htol;

   status = s;
   if (m->hydraulics.LinkSetting[k] == MISSING) return(status);       /* Status fixed by user */
   hml = m->Link[k].Km*SQR(m->hydraulics.LinkFlows[k]);                /* Head loss when open  */

/*** Status rules below have changed. ***/                                     //(2.00.11 - LR)

  
  int isTooLargeNegative = (m->hydraulics.LinkFlows[k] < -(m->Qtol));
  
   switch (s)
   {
      case ACTIVE:
       if (isTooLargeNegative) {
         status = CLOSED;
       }
       else if (h1-hml < hset-htol) {
         status = OPEN;                           //(2.00.11 - LR)
       }
       else {
         status = ACTIVE;
       }
         break;
      case OPEN:
       if (isTooLargeNegative) {
         status = CLOSED;
       }
       else if (h2 >= hset+htol) {
         status = ACTIVE;                         //(2.00.11 - LR)
       }
       else {
         status = OPEN;
       }
         break;
      case CLOSED:
       if ( h1 >= hset+htol && h2 < hset-htol) {
         status = ACTIVE;                         //(2.00.11 - LR)
       }
       else if (h1 < hset-htol && h1 > h2+htol) {
         status = OPEN;                           //(2.00.11 - LR)
       }
       else {
         status = CLOSED;
       }
       break;
      case XPRESSURE:
       if (isTooLargeNegative) {
         status = CLOSED;
       }
         break;
   }
   return(status);
}


char  psvstatus(OW_Project *m, int k, char s, double hset, double h1, double h2)
/*
**-----------------------------------------------------------
**  Input:   k    = link index                                
**           s    = current status                            
**           hset = valve head setting                        
**           h1   = head at upstream node                     
**           h2   = head at downstream node                   
**  Output:  returns new valve status                         
**  Purpose: updates status of a pressure sustaining valve.   
**-----------------------------------------------------------
*/
{
   char  status;       /* New valve status */
   double hml;          /* Minor headloss   */
   double htol = m->Htol;

   status = s;
  if (m->hydraulics.LinkSetting[k] == MISSING) {
    return(status);       /* Status fixed by user */
  }
   hml = m->Link[k].Km*SQR(m->hydraulics.LinkFlows[k]);                /* Head loss when open  */

/*** Status rules below have changed. ***/                                     //(2.00.11 - LR)

   switch (s)
   {
      case ACTIVE:
         if (m->hydraulics.LinkFlows[k] < -(m->Qtol))            status = CLOSED;
         else if (h2+hml > hset+htol) status = OPEN;                           //(2.00.11 - LR)
         else                         status = ACTIVE;
         break;
      case OPEN:
         if (m->hydraulics.LinkFlows[k] < -(m->Qtol))            status = CLOSED;
         else if (h1 < hset-htol)     status = ACTIVE;                         //(2.00.11 - LR)
         else                         status = OPEN;
         break;
      case CLOSED:
         if (h2 > hset+htol                                                    //(2.00.11 - LR)
          && h1 > h2+htol)            status = OPEN;                           //(2.00.11 - LR)
         else if (h1 >= hset+htol && h1 > h2+htol)       status = ACTIVE;                         //(2.00.11 - LR)
         else                         status = CLOSED;
         break;
      case XPRESSURE:
         if (m->hydraulics.LinkFlows[k] < -(m->Qtol))            status = CLOSED;
         break;
   }
   return(status);
}


char  fcvstatus(OW_Project *m, int k, char s, double h1, double h2)
/*
**-----------------------------------------------------------
**  Input:   k    = link index                                
**           s    = current status                            
**           h1   = head at upstream node                     
**           h2   = head at downstream node                   
**  Output:  returns new valve status                         
**  Purpose: updates status of a flow control valve.          
**                                                          
**    Valve status changes to XFCV if flow reversal.       
**    If current status is XFCV and current flow is        
**    above setting, then valve becomes active.             
**    If current status is XFCV, and current flow          
**    positive but still below valve setting, then          
**    status remains same.                                  
**-----------------------------------------------------------
*/
{
  
   char  status;        /* New valve status */
   status = s;
   if (h1 - h2 < -(m->Htol)) {
     status = XFCV;
   }
   else if ( m->hydraulics.LinkFlows[k] < -(m->Qtol) ) {
     status = XFCV;                          //(2.00.11 - LR)
   }
   else if (s == XFCV && m->hydraulics.LinkFlows[k] >= m->hydraulics.LinkSetting[k]) {
     status = ACTIVE;
   }
   return(status);
}


/*** Updated 9/7/00 ***/
/*** Updated 11/19/01 ***/
void  tankstatus(OW_Project *m, int k, int n1, int n2)
/*
**----------------------------------------------------------------
**  Input:   k  = link index                                     
**           n1 = start node of link
**           n2 = end node of link                              
**  Output:  none                                                
**  Purpose: closes link flowing into full or out of empty tank  
**----------------------------------------------------------------
*/
{
   int   i,n;
   double h,q;

   /* Make node n1 be the tank */
   q = m->hydraulics.LinkFlows[k];
   i = n1 - m->Njuncs;
   if (i <= 0)
   {
      i = n2 - m->Njuncs;
      if (i <= 0) return;
      n = n1;
      n1 = n2;
      n2 = n;
      q = -q;
   }
   h = m->hydraulics.NodeHead[n1] - m->hydraulics.NodeHead[n2];

   /* Skip reservoirs & closed links */
   if (m->Tank[i].A == 0.0 || m->hydraulics.LinkStatus[k] <= CLOSED) return;

   /* If tank full, then prevent flow into it */
   if (m->hydraulics.NodeHead[n1] >= m->Tank[i].Hmax - m->Htol)
   {

      /* Case 1: Link is a pump discharging into tank */
      if ( m->Link[k].Type == PUMP )
      {
        if (m->Link[k].N2 == n1) {
          m->hydraulics.LinkStatus[k] = TEMPCLOSED;
        }
      }

      /* Case 2: Downstream head > tank head */
      /* (i.e., an open outflow check valve would close) */
      else if (cvstatus(m, OPEN, h, q) == CLOSED) {
        m->hydraulics.LinkStatus[k] = TEMPCLOSED;
      }
   }

   /* If tank empty, then prevent flow out of it */
   if (m->hydraulics.NodeHead[n1] <= m->Tank[i].Hmin + m->Htol)
   {

      /* Case 1: Link is a pump discharging from tank */
      if ( m->Link[k].Type == PUMP)
      {
        if (m->Link[k].N1 == n1) {
          m->hydraulics.LinkStatus[k] = TEMPCLOSED;
        }
      }

      /* Case 2: Tank head > downstream head */
      /* (i.e., a closed outflow check valve would open) */
      else if (cvstatus(m, CLOSED, h, q) == OPEN) {
        m->hydraulics.LinkStatus[k] = TEMPCLOSED;
      }
   }
}                        /* End of tankstatus */


int  pswitch(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns 1 if status of any link changes, 0 if not   
**  Purpose: adjusts settings of links controlled by junction    
**           pressures after a hydraulic solution is found       
**--------------------------------------------------------------
*/
{
   int   i,                 /* Control statement index */
         k,                 /* Link being controlled */
         n,                 /* Node controlling link */
         reset,             /* Flag on control conditions */
         change,            /* Flag for status or setting change */
         anychange = 0;     /* Flag for 1 or more changes */
   char  s;                 /* Current link status */

   /* Check each control statement */
   for (i=1; i <= m->Ncontrols; i++)
   {
      reset = 0;
      if ( (k = m->Control[i].Link) <= 0) continue;

      /* Determine if control based on a junction, not a tank */
      if ( (n = m->Control[i].Node) > 0 && n <= m->Njuncs)
      {
         /* Determine if control conditions are satisfied */
         if (m->Control[i].Type == LOWLEVEL
             && m->hydraulics.NodeHead[n] <= m->Control[i].Grade + m->Htol )
             reset = 1;
         if (m->Control[i].Type == HILEVEL
             && m->hydraulics.NodeHead[n] >= m->Control[i].Grade - m->Htol )
             reset = 1;
      }

      /* Determine if control forces a status or setting change */
      if (reset == 1)
      {
         change = 0;
         s = m->hydraulics.LinkStatus[k];
         if (m->Link[k].Type == PIPE)
         {
            if (s != m->Control[i].Status) change = 1;
         }
         if (m->Link[k].Type == PUMP)
         {
            if (m->hydraulics.LinkSetting[k] != m->Control[i].Setting) change = 1;
         }
         if (m->Link[k].Type >= PRV)
         {
            if (m->hydraulics.LinkSetting[k] != m->Control[i].Setting) change = 1;
            else if (m->hydraulics.LinkSetting[k] == MISSING &&
                     s != m->Control[i].Status) change = 1;
         }

         /* If a change occurs, update status & setting */
         if (change)
         {
            m->hydraulics.LinkStatus[k] = m->Control[i].Status;
           if (m->Link[k].Type > PIPE) {
             m->hydraulics.LinkSetting[k] = m->Control[i].Setting;
           }
           if (m->Statflag == FULL) {
             writestatchange(m,k,s,m->hydraulics.LinkStatus[k]);
           }

            /* Re-set flow if status has changed */
//            if (S[k] != s) initlinkflow(k, S[k], K[k]);
            anychange = 1;
         }
      }
   }
   return(anychange);
}                        /* End of pswitch */


double newflows(OW_Project *m)
/*
**----------------------------------------------------------------
**  Input:   none                                                
**  Output:  returns solution convergence error                  
**  Purpose: updates link flows after new nodal heads computed   
**----------------------------------------------------------------
*/
{
   double  dh,                    /* Link head loss       */
           dq;                    /* Link flow change     */
   double  dqsum,                 /* Network flow change  */
           qsum;                  /* Network total flow   */
   int   k, n, n1, n2;

   /* Initialize net inflows (i.e., demands) at tanks */
   for (n = m->Njuncs+1; n <= m->Nnodes; n++) {
     m->hydraulics.NodeDemand[n] = 0.0;
   }

   /* Initialize sum of flows & corrections */
   qsum  = 0.0;
   dqsum = 0.0;

   /* Update flows in all links */
   for (k=1; k <= m->Nlinks; k++)
   {

      /*
      ** Apply flow update formula:                   
      **   dq = Y - P*(new head loss)                 
      **    P = 1/(dh/dq)                             
      **    Y = P*(head loss based on current flow)   
      ** where P & Y were computed in newcoeffs().   
      */

      n1 = m->Link[k].N1;
      n2 = m->Link[k].N2;
      dh = m->hydraulics.NodeHead[n1] - m->hydraulics.NodeHead[n2];
      dq = m->hydraulics.solver.Y[k] - m->hydraulics.solver.P[k]*dh;

      /* Adjust flow change by the relaxation factor */                        //(2.00.11 - LR)
      dq *= m->RelaxFactor;                                                       //(2.00.11 - LR)

      /* Prevent flow in constant HP pumps from going negative */
      if (m->Link[k].Type == PUMP)
      {
         n = m->Link[(k)].pumpLinkIdx;
         if (m->Pump[n].Ptype == CONST_HP && dq > m->hydraulics.LinkFlows[k]) {
           dq = m->hydraulics.LinkFlows[k]/2.0;
         }
      }
      m->hydraulics.LinkFlows[k] -= dq;

      /* Update sum of absolute flows & flow corrections */
      qsum += ABS(m->hydraulics.LinkFlows[k]);
      dqsum += ABS(dq);

      /* Update net flows to tanks */
      if ( m->hydraulics.LinkStatus[k] > CLOSED )                                                     //(2.00.12 - LR)
      {
         if (n1 > m->Njuncs) m->hydraulics.NodeDemand[n1] -= m->hydraulics.LinkFlows[k];
         if (n2 > m->Njuncs) m->hydraulics.NodeDemand[n2] += m->hydraulics.LinkFlows[k];
      }

   }

   /* Update emitter flows */
   for (k=1; k <= m->Njuncs; k++)
   {
      if (m->Node[k].Ke == 0.0) continue;
      dq = emitflowchange(m,k);
      m->hydraulics.EmitterFlows[k] -= dq;
      qsum += ABS(m->hydraulics.EmitterFlows[k]);
      dqsum += ABS(dq);
   }

   /* Return ratio of total flow corrections to total flow */
   if (qsum > m->Hacc) return(dqsum/qsum);
   else return(dqsum);

}                        /* End of newflows */


void   newcoeffs(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none                                                
**  Output:  none                                                
**  Purpose: computes coefficients of linearized network eqns.   
**--------------------------------------------------------------
*/
{
   memset(m->hydraulics.solver.Aii,0,(m->Nnodes+1)*sizeof(double));   /* Reset coeffs. to 0 */
   memset(m->hydraulics.solver.Aij,0,(m->Ncoeffs+1)*sizeof(double));
   memset(m->hydraulics.solver.F,0,(m->Nnodes+1)*sizeof(double));
   memset(m->X,0,(m->Nnodes+1)*sizeof(double));
   memset(m->hydraulics.solver.P,0,(m->Nlinks+1)*sizeof(double));
   memset(m->hydraulics.solver.Y,0,(m->Nlinks+1)*sizeof(double));
   linkcoeffs(m);                            /* Compute link coeffs.  */
   emittercoeffs(m);                         /* Compute emitter coeffs.*/
   nodecoeffs(m);                            /* Compute node coeffs.  */
   valvecoeffs(m);                           /* Compute valve coeffs. */
}                        /* End of newcoeffs */


void  linkcoeffs(OW_Project *m)
/*
**--------------------------------------------------------------
**   Input:   none                                                
**   Output:  none                                                
**   Purpose: computes solution matrix coefficients for links     
**--------------------------------------------------------------
*/
{
   int   k,n1,n2;

   /* Examine each link of network */
   for (k=1; k <= m->Nlinks; k++)
   {
      n1 = m->Link[k].N1;           /* Start node of link */
      n2 = m->Link[k].N2;           /* End node of link   */

      int row1 = m->hydraulics.solver.Row[n1];
      int row2 = m->hydraulics.solver.Row[n2];
     
     
      /* Compute P[k] = 1 / (dh/dQ) and Y[k] = h * P[k]   */
      /* for each link k (where h = link head loss).      */
      /* FCVs, PRVs, and PSVs with non-fixed status       */
      /* are analyzed later.                              */

      switch (m->Link[k].Type)
      {
         case CV:
         case PIPE:  pipecoeff(m,k); break;
         case PUMP:  pumpcoeff(m,k); break;
         case PBV:   pbvcoeff(m,k);  break;
         case TCV:   tcvcoeff(m,k);  break;
         case GPV:   gpvcoeff(m,k);  break;
         case FCV:   
         case PRV:
         case PSV:   /* If valve status fixed then treat as pipe */
                     /* otherwise ignore the valve for now. */
                     if (m->hydraulics.LinkSetting[k] == MISSING) {
                       valvecoeff(m,k);  //pipecoeff(k);      //(2.00.11 - LR)
                     }
                     else continue;
                     break;
         default:    continue;                  
      }                                         

      /* Update net nodal inflows (X), solution matrix (A) and RHS array (F) */
      /* (Use covention that flow out of node is (-), flow into node is (+)) */
      m->X[n1] -= m->hydraulics.LinkFlows[k];
      m->X[n2] += m->hydraulics.LinkFlows[k];
      m->hydraulics.solver.Aij[m->hydraulics.solver.Ndx[k]] -= m->hydraulics.solver.P[k];              /* Off-diagonal coeff. */
      if (n1 <= m->Njuncs)                 /* Node n1 is junction */
      {
         m->hydraulics.solver.Aii[row1] += m->hydraulics.solver.P[k];          /* Diagonal coeff. */
         double rhs = m->hydraulics.solver.Y[k];
         m->hydraulics.solver.F[row1] += rhs;            /* RHS coeff.      */
      }
      else {
        m->hydraulics.solver.F[row2] += (m->hydraulics.solver.P[k] * m->hydraulics.NodeHead[n1]);  /* Node n1 is a tank   */
      }
      if (n2 <= m->Njuncs)                 /* Node n2 is junction */
      {
         m->hydraulics.solver.Aii[row2] += m->hydraulics.solver.P[k];          /* Diagonal coeff. */
         double rhs = m->hydraulics.solver.Y[k];
         m->hydraulics.solver.F[row2] -= rhs;            /* RHS coeff.      */
      }
      else { /* Node n2 is a tank   */
        double rhs = (m->hydraulics.solver.P[k] * m->hydraulics.NodeHead[n2]);
        m->hydraulics.solver.F[row1] += rhs;
      }
     
     // check for nans
     int nank;
     for (nank=1; nank <= m->Nnodes; nank++)
     {
       double f = m->hydraulics.solver.F[nank];
       //fprintf(stdout, "%f", f);
       if (isnan(f)) {
         // is nan
         //fprintf(stdout, "  <------------   nan");
       }
       if (isinf(f)) {
         // is nan
         //fprintf(stdout, "  <------------  INF");
       }
       //fprintf(stdout, "\n");
     }
     
   }

}                        /* End of linkcoeffs */


void  nodecoeffs(OW_Project *m)
/*
**----------------------------------------------------------------
**  Input:   none                                                
**  Output:  none                                                
**  Purpose: completes calculation of nodal flow imbalance (X)   
**           & flow correction (F) arrays                        
**----------------------------------------------------------------
*/
{
   int   i;

   /* For junction nodes, subtract demand flow from net */
   /* flow imbalance & add imbalance to RHS array F.    */
   for (i=1; i <= m->Njuncs; i++)
   {
      m->X[i] -= m->hydraulics.NodeDemand[i];
      m->hydraulics.solver.F[m->hydraulics.solver.Row[i]] += m->X[i];
   }
}                        /* End of nodecoeffs */


void  valvecoeffs(OW_Project *m)
/*
**--------------------------------------------------------------
**   Input:   none                                                
**   Output:  none                                                
**   Purpose: computes matrix coeffs. for PRVs, PSVs & FCVs       
**            whose status is not fixed to OPEN/CLOSED            
**--------------------------------------------------------------
*/
{
   int i,k,n1,n2;

   for (i=1; i <= m->Nvalves; i++)                   /* Examine each valve   */
   {
      k = m->Valve[i].Link;                        /* Link index of valve  */
      if (m->hydraulics.LinkSetting[k] == MISSING) continue;            /* Valve status fixed   */
      n1 = m->Link[k].N1;                          /* Start & end nodes    */
      n2 = m->Link[k].N2;
      switch (m->Link[k].Type)                     /* Call valve-specific  */
      {                                         /*   function           */
         case PRV:  prvcoeff(m,k,n1,n2); break;
         case PSV:  psvcoeff(m,k,n1,n2); break;
         case FCV:  fcvcoeff(m,k,n1,n2); break;
         default:   continue;
      }
   }
}                        /* End of valvecoeffs */


void  emittercoeffs(OW_Project *m)
/*
**--------------------------------------------------------------
**   Input:   none                                                
**   Output:  none                                                
**   Purpose: computes matrix coeffs. for emitters
**
**   Note: Emitters consist of a fictitious pipe connected to
**         a fictitious reservoir whose elevation equals that
**         of the junction. The headloss through this pipe is
**         Ke*(Flow)^Qexp, where Ke = emitter headloss coeff.
**--------------------------------------------------------------
*/
{
   int   i;
   double  ke;
   double  p;
   double  q;
   double  y;
   double  z;
   for (i=1; i <= m->Njuncs; i++)
   {
      if (m->Node[i].Ke == 0.0) continue;
      ke = MAX(CSMALL, m->Node[i].Ke);
      q = m->hydraulics.EmitterFlows[i];
      z = ke*pow(ABS(q),m->Qexp);
      p = m->Qexp*z/ABS(q);
      if (p < m->RQtol)
        p = 1.0 / m->RQtol;
      else
        p = 1.0/p;
      y = SGN(q)*z*p;
      m->hydraulics.solver.Aii[m->hydraulics.solver.Row[i]] += p;
      m->hydraulics.solver.F[m->hydraulics.solver.Row[i]] += y + p * m->Node[i].El;
      m->X[i] -= q;
   }
}


double  emitflowchange(OW_Project *m, int i)
/*
**--------------------------------------------------------------
**   Input:   i = node index
**   Output:  returns change in flow at an emitter node                                                
**   Purpose: computes flow change at an emitter node
**--------------------------------------------------------------
*/
{
   double ke, p;
   ke = MAX(CSMALL, m->Node[i].Ke);
   p = m->Qexp * ke * pow(ABS(m->hydraulics.EmitterFlows[i]),(m->Qexp-1.0));
   if (p < m->RQtol)
      p = 1 / m->RQtol;
   else
      p = 1.0/p;
  
   return(m->hydraulics.EmitterFlows[i] / m->Qexp - p*(m->hydraulics.NodeHead[i] - m->Node[i].El));
}


void  pipecoeff(OW_Project *m, int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index                                      
**   Output:  none                                                
**  Purpose:  computes P & Y coefficients for pipe k              
**                                                              
**    P = inverse head loss gradient = 1/(dh/dQ)                
**    Y = flow correction term = h*P                            
**--------------------------------------------------------------
*/
{
   double  hpipe,     /* Normal head loss          */
         hml,       /* Minor head loss           */
         ml,        /* Minor loss coeff.         */
         p,         /* q*(dh/dq)                 */
         q,         /* Abs. value of flow        */
         r,         /* Resistance coeff.         */
         r1,        /* Total resistance factor   */
         f,         /* D-W friction factor       */
         dfdq;      /* Derivative of fric. fact. */

   /* For closed pipe use headloss formula: h = CBIG*q */
   if (m->hydraulics.LinkStatus[k] <= CLOSED)
   {
      m->hydraulics.solver.P[k] = 1.0/CBIG;
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k];
      return;
   }

   /* Evaluate headloss coefficients */
   q = ABS(m->hydraulics.LinkFlows[k]);                         /* Absolute flow       */
   ml = m->Link[k].Km;                       /* Minor loss coeff.   */
   r = m->Link[k].R;                         /* Resistance coeff.   */
   f = 1.0;                               /* D-W friction factor */
  if (m->Formflag == DW) {
    f = DWcoeff(m,k,&dfdq);
  }
   r1 = f*r+ml;
 
   /* Use large P coefficient for small flow resistance product */
   if (r1*q < m->RQtol)
   {
      m->hydraulics.solver.P[k] = 1.0 / m->RQtol;
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k] / m->Hexp;
      return;
   }

   /* Compute P and Y coefficients */
   if (m->Formflag == DW)                  /* D-W eqn. */
   {
      hpipe = r1*SQR(q);                /* Total head loss */
      p = 2.0*r1*q;                     /* |dh/dQ| */
     /* + dfdq*r*q*q;*/                 /* Ignore df/dQ term */
      p = 1.0/p;
      m->hydraulics.solver.P[k] = p;
      m->hydraulics.solver.Y[k] = SGN(m->hydraulics.LinkFlows[k])*hpipe*p;
   }
   else                                 /* H-W or C-M eqn.   */
   {
      hpipe = r*pow(q, m->Hexp);            /* Friction head loss  */
      p = m->Hexp * hpipe;                   /* Q*dh(friction)/dQ   */
      if (ml > 0.0)
      {
         hml = ml*q*q;                  /* Minor head loss   */
         p += 2.0*hml;                  /* Q*dh(Total)/dQ    */
      }
      else  hml = 0.0;
      p = m->hydraulics.LinkFlows[k]/p;                       /* 1 / (dh/dQ) */
      m->hydraulics.solver.P[k] = ABS(p);
      m->hydraulics.solver.Y[k] = p*(hpipe + hml);
   }
}                        /* End of pipecoeff */


double DWcoeff(OW_Project *m, int k, double *dfdq)
/*
**--------------------------------------------------------------
**   Input:   k = link index                                      
**   Output:  returns Darcy-Weisbach friction factor              
**   Purpose: computes Darcy-Weisbach friction factor             
**                                                              
**    Uses interpolating polynomials developed by               
**    E. Dunlop for transition flow from 2000 < Re < 4000.      
**
**   df/dq term is ignored as it slows convergence rate.
**--------------------------------------------------------------
*/
{
   double q,             /* Abs. value of flow */
          f;             /* Friction factor    */
   double x1,x2,x3,x4,
          y1,y2,y3,
          fa,fb,r;
   double s,w;

   *dfdq = 0.0;
   if (m->Link[k].Type > PIPE) return(1.0); /* Only apply to pipes */
   q = ABS(m->hydraulics.LinkFlows[k]);
   s = m->Viscos * m->Link[k].Diam;
   w = q/s;                       /* w = Re(Pi/4) */
   if (w >= A1)                   /* Re >= 4000; Colebrook Formula */
   {
      y1 = A8/pow(w,0.9);
      y2 = m->Link[k].Kc/(3.7 * m->Link[k].Diam) + y1;
      y3 = A9*log(y2);
      f = 1.0/SQR(y3);
      /*  *dfdq = (2.0+AA*y1/(y2*y3))*f; */   /* df/dq */
   }
   else if (w > A2)              /* Re > 2000; Interpolation formula */
   {
      y2 = m->Link[k].Kc/(3.7 * m->Link[k].Diam) + AB;
      y3 = A9*log(y2);
      fa = 1.0/SQR(y3);
      fb = (2.0+AC/(y2*y3))*fa;
      r = w/A2;
      x1 = 7.0*fa - fb;
      x2 = 0.128 - 17.0*fa + 2.5*fb;
      x3 = -0.128 + 13.0*fa - (fb+fb);
      x4 = r*(0.032 - 3.0*fa + 0.5*fb);
      f = x1 + r*(x2 + r*(x3+x4));
      /*  *dfdq = (x1 + x1 + r*(3.0*x2 + r*(4.0*x3 + 5.0*x4)));  */
   }
   else if (w > A4)              /* Laminar flow: Hagen-Poiseuille Formula */
   {
      f = A3*s/q;
      /*  *dfdq = A3*s; */
   }
   else
   {
      f = 8.0;
      *dfdq = 0.0;
   }
   return(f);
}                        /* End of DWcoeff */


/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void  pumpcoeff(OW_Project *m, int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index                                      
**   Output:  none                                                
**   Purpose: computes P & Y coeffs. for pump in link k           
**--------------------------------------------------------------
*/
{
   int   p;         /* Pump index             */
   double h0,       /* Shutoff head           */
         q,         /* Abs. value of flow     */
         r,         /* Flow resistance coeff. */
         n;         /* Flow exponent coeff.   */

   double setting = m->hydraulics.LinkSetting[k];
  
   /* Use high resistance pipe if pump closed or cannot deliver head */
   if (m->hydraulics.LinkStatus[k] <= CLOSED || setting == 0.0)
   {
      //pipecoeff(k);                                                          //(2.00.11 - LR)
      m->hydraulics.solver.P[k] = 1.0/CBIG;                                                         //(2.00.11 - LR)
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k];                                                             //(2.00.11 - LR)
      return;
   }

   q = ABS(m->hydraulics.LinkFlows[k]);
   q = MAX(q,TINY);
   p = m->Link[k].pumpLinkIdx;

   /* Get pump curve coefficients for custom pump curve. */
   if (m->Pump[p].Ptype == CUSTOM)
   {
      /* Find intercept (h0) & slope (r) of pump curve    */
      /* line segment which contains speed-adjusted flow. */
      curvecoeff(m, m->Pump[p].Hcurve, q/setting, &h0, &r);

      /* Determine head loss coefficients. */
      m->Pump[p].H0 = -h0;
      m->Pump[p].R  = -r;
      m->Pump[p].N  = 1.0;
   }

   /* Adjust head loss coefficients for pump speed. */
   h0 = SQR(setting) * m->Pump[p].H0;
   n  = m->Pump[p].N;
   r  = m->Pump[p].R * pow(setting,2.0-n);
   if (n != 1.0) r = n*r*pow(q,n-1.0);

   /* Compute inverse headloss gradient (P) and flow correction factor (Y) */
   m->hydraulics.solver.P[k] = 1.0/MAX(r, m->RQtol);
   m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k]/n + m->hydraulics.solver.P[k]*h0;
}                        /* End of pumpcoeff */


/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void  curvecoeff(OW_Project *m, int i, double q, double *h0, double *r)
/*
**-------------------------------------------------------------------
**   Input:   i   = curve index                                        
**            q   = flow rate
**   Output:  *h0  = head at zero flow (y-intercept)                   
**            *r  = dHead/dFlow (slope)                                
**   Purpose: computes intercept and slope of head v. flow curve       
**            at current flow.                                         
**-------------------------------------------------------------------
*/
{
   int   k1, k2, npts;
   double *x, *y;

   /* Remember that curve is stored in untransformed units */
   q *= m->Ucf[FLOW];
   x = m->Curve[i].X;           /* x = flow */
   y = m->Curve[i].Y;           /* y = head */
   npts = m->Curve[i].Npts;

   /* Find linear segment of curve that brackets flow q */
   k2 = 0;
   while (k2 < npts && x[k2] < q) k2++;
   if      (k2 == 0)    k2++;
   else if (k2 == npts) k2--;
   k1  = k2 - 1;

   /* Compute slope and intercept of this segment */
   *r  = (y[k2]-y[k1])/(x[k2]-x[k1]);
   *h0 = y[k1] - (*r)*x[k1];

   /* Convert units */
   *h0 = (*h0) / m->Ucf[HEAD];
   *r  = (*r) * m->Ucf[FLOW] / m->Ucf[HEAD];
}                       /* End of curvecoeff */


void  gpvcoeff(OW_Project *m, int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index
**   Output:  none                                                
**   Purpose: computes P & Y coeffs. for general purpose valve   
**--------------------------------------------------------------
*/
{
   double h0,        /* Headloss curve intercept */
          q,         /* Abs. value of flow       */
          r;         /* Flow resistance coeff.   */

/*** Updated 9/7/00 ***/
   /* Treat as a pipe if valve closed */
   if (m->hydraulics.LinkStatus[k] == CLOSED) valvecoeff(m,k); //pipecoeff(k);                          //(2.00.11 - LR)

   /* Otherwise utilize headloss curve   */
   /* whose index is stored in K */
   else
   {
      /* Find slope & intercept of headloss curve. */
      q = ABS(m->hydraulics.LinkFlows[k]);
      q = MAX(q,TINY);

/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
      curvecoeff(m, (int)ROUND(m->hydraulics.LinkSetting[k]), q, &h0, &r);

      /* Compute inverse headloss gradient (P) */
      /* and flow correction factor (Y).       */
      m->hydraulics.solver.P[k] = 1.0 / MAX(r, m->RQtol);
      m->hydraulics.solver.Y[k] = m->hydraulics.solver.P[k]*(h0 + r*q)*SGN(m->hydraulics.LinkFlows[k]);                                        //(2.00.11 - LR)
   }
}
 

void  pbvcoeff(OW_Project *m, int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index                                      
**   Output:  none                                                
**   Purpose: computes P & Y coeffs. for pressure breaker valve   
**--------------------------------------------------------------
*/
{
   /* If valve fixed OPEN or CLOSED then treat as a pipe */
  if (m->hydraulics.LinkSetting[k] == MISSING || m->hydraulics.LinkSetting[k] == 0.0) {
    valvecoeff(m,k);  //pipecoeff(k);         //(2.00.11 - LR)
  }

   /* If valve is active */
   else
   {
      /* Treat as a pipe if minor loss > valve setting */
      if (m->Link[k].Km*SQR(m->hydraulics.LinkFlows[k]) > m->hydraulics.LinkSetting[k]) valvecoeff(m,k);  //pipecoeff(k);         //(2.00.11 - LR)

      /* Otherwise force headloss across valve to be equal to setting */
      else
      {
         m->hydraulics.solver.P[k] = CBIG;
         m->hydraulics.solver.Y[k] = m->hydraulics.LinkSetting[k]*CBIG;
      }
   }
}                        /* End of pbvcoeff */


void  tcvcoeff(OW_Project *m, int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index                                      
**   Output:  none                                                
**   Purpose: computes P & Y coeffs. for throttle control valve   
**--------------------------------------------------------------
*/
{
   double km;

   /* Save original loss coeff. for open valve */
   km = m->Link[k].Km;

   /* If valve not fixed OPEN or CLOSED, compute its loss coeff. */
   if (m->hydraulics.LinkSetting[k] != MISSING) {
     m->Link[k].Km = 0.02517 * m->hydraulics.LinkSetting[k]/(SQR(m->Link[k].Diam)*SQR(m->Link[k].Diam));
   }
   /* Then apply usual pipe formulas */
   valvecoeff(m,k);  //pipecoeff(k);                                             //(2.00.11 - LR)

   /* Restore original loss coeff. */
   m->Link[k].Km = km;
}                        /* End of tcvcoeff */


void  prvcoeff(OW_Project *m, int k, int n1, int n2)
/*
**--------------------------------------------------------------
**   Input:   k    = link index                                   
**            n1   = upstream node of valve                       
**            n2   = downstream node of valve                       
**   Output:  none                                                
**   Purpose: computes solution matrix coeffs. for pressure       
**            reducing valves                                     
**--------------------------------------------------------------
*/
{
   int   i,j;                       /* Rows of solution matrix */
   double hset;                      /* Valve head setting      */
   i  = m->hydraulics.solver.Row[n1];                    /* Matrix rows of nodes    */
   j  = m->hydraulics.solver.Row[n2];
   hset   = m->Node[n2].El + m->hydraulics.LinkSetting[k];     /* Valve setting           */

   if (m->hydraulics.LinkStatus[k] == ACTIVE)
   {
      /*
         Set coeffs. to force head at downstream 
         node equal to valve setting & force flow (when updated in       
         newflows()) equal to flow imbalance at downstream node. 
      */
      m->hydraulics.solver.P[k] = 0.0;
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k] + m->X[n2];       /* Force flow balance   */
      m->hydraulics.solver.F[j] += (hset*CBIG);       /* Force head = hset    */
      m->hydraulics.solver.Aii[j] += CBIG;            /*   at downstream node */
     if (m->X[n2] < 0.0) {
       m->hydraulics.solver.F[i] += m->X[n2];
     }
      return;
   }

   /* 
      For OPEN, CLOSED, or XPRESSURE valve
      compute matrix coeffs. using the valvecoeff() function.                  //(2.00.11 - LR)
   */
   valvecoeff(m,k);  /*pipecoeff(k);*/                                           //(2.00.11 - LR)
   m->hydraulics.solver.Aij[m->hydraulics.solver.Ndx[k]] -= m->hydraulics.solver.P[k];
   m->hydraulics.solver.Aii[i] += m->hydraulics.solver.P[k];
   m->hydraulics.solver.Aii[j] += m->hydraulics.solver.P[k];
   m->hydraulics.solver.F[i] += (m->hydraulics.solver.Y[k] - m->hydraulics.LinkFlows[k]);
   m->hydraulics.solver.F[j] -= (m->hydraulics.solver.Y[k] - m->hydraulics.LinkFlows[k]);
}                        /* End of prvcoeff */


void  psvcoeff(OW_Project *m, int k, int n1, int n2)
/*
**--------------------------------------------------------------
**   Input:   k    = link index                                   
**            n1   = upstream node of valve                       
**            n2   = downstream node of valve                       
**   Output:  none                                                
**   Purpose: computes solution matrix coeffs. for pressure       
**            sustaining valve                                    
**--------------------------------------------------------------
*/
{
   int   i,j;                       /* Rows of solution matrix */
   double hset;                      /* Valve head setting      */
   i  = m->hydraulics.solver.Row[n1];                    /* Matrix rows of nodes    */
   j  = m->hydraulics.solver.Row[n2];
   hset   = m->Node[n1].El + m->hydraulics.LinkSetting[k];     /* Valve setting           */

   if (m->hydraulics.LinkStatus[k] == ACTIVE)
   {
      /*
         Set coeffs. to force head at upstream 
         node equal to valve setting & force flow (when updated in       
         newflows()) equal to flow imbalance at upstream node. 
      */
      m->hydraulics.solver.P[k] = 0.0;
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k] - m->X[n1];              /* Force flow balance   */
      m->hydraulics.solver.F[i] += (hset*CBIG);              /* Force head = hset    */
      m->hydraulics.solver.Aii[i] += CBIG;                   /*   at upstream node   */
      if (m->X[n1] > 0.0)
        m->hydraulics.solver.F[j] += m->X[n1];
      return;
   }

   /* 
      For OPEN, CLOSED, or XPRESSURE valve
      compute matrix coeffs. using the valvecoeff() function.                  //(2.00.11 - LR)
   */
   valvecoeff(m,k);  /*pipecoeff(k);*/                                           //(2.00.11 - LR)
   m->hydraulics.solver.Aij[m->hydraulics.solver.Ndx[k]] -= m->hydraulics.solver.P[k];
   m->hydraulics.solver.Aii[i] += m->hydraulics.solver.P[k];
   m->hydraulics.solver.Aii[j] += m->hydraulics.solver.P[k];
   m->hydraulics.solver.F[i] += (m->hydraulics.solver.Y[k] - m->hydraulics.LinkFlows[k]);
   m->hydraulics.solver.F[j] -= (m->hydraulics.solver.Y[k] - m->hydraulics.LinkFlows[k]);
}                        /* End of psvcoeff */


void  fcvcoeff(OW_Project *m, int k, int n1, int n2)
/*
**--------------------------------------------------------------
**   Input:   k    = link index                                   
**            n1   = upstream node of valve                       
**            n2   = downstream node of valve                       
**   Output:  none                                                
**   Purpose: computes solution matrix coeffs. for flow control   
**            valve                                               
**--------------------------------------------------------------
*/
{
   int   i,j;                   /* Rows in solution matrix */
   double q;                     /* Valve flow setting      */
   q = m->hydraulics.LinkSetting[k];
   i = m->hydraulics.solver.Row[n1];
   j = m->hydraulics.solver.Row[n2];

   /*
      If valve active, break network at valve and treat  
      flow setting as external demand at upstream node   
      and external supply at downstream node.            
   */
   if (m->hydraulics.LinkStatus[k] == ACTIVE)
   {
      m->X[n1] -= q;
      m->hydraulics.solver.F[i] -= q;
      m->X[n2] += q;
      m->hydraulics.solver.F[j] += q;
      /*P[k] = 0.0;*/
      m->hydraulics.solver.P[k] = 1.0/CBIG;                                                         //(2.00.11 - LR)
      m->hydraulics.solver.Aij[m->hydraulics.solver.Ndx[k]] -= m->hydraulics.solver.P[k];                                                     //(2.00.11 - LR)
      m->hydraulics.solver.Aii[i] += m->hydraulics.solver.P[k];                                                          //(2.00.11 - LR)
      m->hydraulics.solver.Aii[j] += m->hydraulics.solver.P[k];                                                          //(2.00.11 - LR)
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k] - q;
   }
   /*
     Otherwise treat valve as an open pipe
   */
   else 
   {
      valvecoeff(m,k);  //pipecoeff(k);                                          //(2.00.11 - LR)
      m->hydraulics.solver.Aij[m->hydraulics.solver.Ndx[k]] -= m->hydraulics.solver.P[k];
      m->hydraulics.solver.Aii[i] += m->hydraulics.solver.P[k];
      m->hydraulics.solver.Aii[j] += m->hydraulics.solver.P[k];
      m->hydraulics.solver.F[i] += (m->hydraulics.solver.Y[k] - m->hydraulics.LinkFlows[k]);
      m->hydraulics.solver.F[j] -= (m->hydraulics.solver.Y[k] - m->hydraulics.LinkFlows[k]);
   }
}                        /* End of fcvcoeff */


/*** New function added. ***/                                                  //(2.00.11 - LR)
void valvecoeff(OW_Project *m, int k)
/*
**--------------------------------------------------------------
**   Input:   k    = link index                                   
**   Output:  none                                                
**   Purpose: computes solution matrix coeffs. for a completely
**            open, closed, or throttled control valve.                                               
**--------------------------------------------------------------
*/
{
   double p;

   // Valve is closed. Use a very small matrix coeff.
   if (m->hydraulics.LinkStatus[k] <= CLOSED)
   {
      m->hydraulics.solver.P[k] = 1.0/CBIG;
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k];
      return;
   }

   // Account for any minor headloss through the valve
   if (m->Link[k].Km > 0.0)
   {
      p = 2.0 * m->Link[k].Km * fabs(m->hydraulics.LinkFlows[k]);
      if ( p < m->RQtol ) {
        p = m->RQtol;
      }
      m->hydraulics.solver.P[k] = 1.0/p;
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k]/2.0;
   }
   else
   {
      m->hydraulics.solver.P[k] = 1.0 / m->RQtol;
      m->hydraulics.solver.Y[k] = m->hydraulics.LinkFlows[k];
   }
}

/****************  END OF HYDRAUL.C  ***************/

