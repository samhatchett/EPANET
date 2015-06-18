/*
*********************************************************************

INPUT1.C -- Input Functions for EPANET Program

VERSION:    2.00
DATE:       5/30/00
            9/7/00
            11/19/01
            6/24/02
            2/14/08     (2.00.12)
AUTHOR:     L. Rossman
            US EPA - NRMRL

  This module initializes, retrieves, and adjusts the input
  data for a network simulation.

  The entry point for this module is:
     getdata() -- called from ENopen() in EPANET.C.

*********************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
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

/*
  --------------------- Module Global Variables  ----------------------
*/

#define   MAXITER   200      /* Default max. # hydraulic iterations    */      //(2.00.12 - LR)
#define   HACC      0.001    /* Default hydraulics convergence ratio   */
#define   HTOL      0.0005   /* Default hydraulic head tolerance (ft)  */

/*** Updated 11/19/01 ***/
#define   QTOL      0.0001   /* Default flow rate tolerance (cfs)      */

#define   AGETOL    0.01     /* Default water age tolerance (hrs)      */
#define   CHEMTOL   0.01     /* Default concentration tolerance        */
#define   PAGESIZE  0        /* Default uses no page breaks            */
#define   SPGRAV    1.0      /* Default specific gravity               */
#define   EPUMP     75       /* Default pump efficiency                */
#define   DEFPATID  "1"      /* Default demand pattern ID              */

/*
  These next three parameters are used in the hydraulics solver:
*/

#define   RQTOL     1E-7     /* Default low flow resistance tolerance  */
#define   CHECKFREQ 2        /* Default status check frequency         */
#define   MAXCHECK  10       /* Default # iterations for status checks */
#define   DAMPLIMIT 0        /* Default damping threshold              */      //(2.00.12 - LR)

extern char *Fldname[];      /* Defined in enumstxt.h in EPANET.C      */
extern char *RptFlowUnitsTxt[];


int  getdata(OW_Project *m)
/*
**----------------------------------------------------------------
**  Input:   none
**  Output:  returns error code
**  Purpose: reads in network data from disk file
**----------------------------------------------------------------
*/
{
   int errcode = 0;
   setdefaults(m);                /* Assign default data values     */
   initreport(m);                 /* Initialize reporting options   */
   rewind(m->InFile);               /* Rewind input file              */
   ERRCODE(readdata(m));          /* Read in network data           */
   if (!errcode) adjustdata(m);   /* Adjust data for default values */
   if (!errcode) initunits(m);    /* Initialize units on input data */
   ERRCODE(inittanks(m));         /* Initialize tank volumes        */
   if (!errcode) convertunits(m); /* Convert units on input data    */
   return(errcode);
}                       /*  End of getdata  */


void  setdefaults(OW_Project *m)
/*
**----------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: assigns default values to global variables
**----------------------------------------------------------------
*/
{
   strncpy(m->Title[0],"",MAXMSG);
   strncpy(m->Title[1],"",MAXMSG);
   strncpy(m->Title[2],"",MAXMSG);
   strncpy(m->TmpDir,"",MAXFNAME);                                                //(2.00.12 - LR)
   strncpy(m->TmpFname,"",MAXFNAME);                                              //(2.00.12 - LR)
   strncpy(m->HydFname,"",MAXFNAME);
   strncpy(m->MapFname,"",MAXFNAME);
   strncpy(m->ChemName,t_CHEMICAL,MAXID);
   strncpy(m->ChemUnits,u_MGperL,MAXID);
   strncpy(m->DefPatID,DEFPATID,MAXID);
   m->Hydflag   = SCRATCH;         /* No external hydraulics file    */
   m->Qualflag  = NONE;            /* No quality simulation          */
   m->Formflag  = HW;              /* Use Hazen-Williams formula     */
   m->Unitsflag = US;              /* US unit system                 */
   m->Flowflag  = GPM;             /* Flow units are gpm             */
   m->Pressflag = PSI;             /* Pressure units are psi         */
   m->Tstatflag = SERIES;          /* Generate time series output    */
   m->Warnflag  = FALSE;           /* Warning flag is off            */
   m->Htol      = HTOL;            /* Default head tolerance         */
   m->Qtol      = QTOL;            /* Default flow tolerance         */
   m->Hacc      = HACC;            /* Default hydraulic accuracy     */
   m->Ctol      = MISSING;         /* No pre-set quality tolerance   */
   m->MaxIter   = MAXITER;         /* Default max. hydraulic trials  */
   m->ExtraIter = -1;              /* Stop if network unbalanced     */
   m->Dur       = 0;               /* 0 sec duration (steady state)  */
   m->Tstart    = 0;               /* Starting time of day           */
   m->Pstart    = 0;               /* Starting pattern period        */
   m->Hstep     = 3600;            /* 1 hr hydraulic time step       */
   m->Qstep     = 0;               /* No pre-set quality time step   */
   m->Pstep     = 3600;            /* 1 hr time pattern period       */
   m->Rstep     = 3600;            /* 1 hr reporting period          */
   m->Rulestep  = 0;               /* No pre-set rule time step      */
   m->Rstart    = 0;               /* Start reporting at time 0      */
   m->TraceNode = 0;               /* No source tracing              */
   m->BulkOrder = 1.0;             /* 1st-order bulk reaction rate   */
   m->WallOrder = 1.0;             /* 1st-order wall reaction rate   */
   m->TankOrder = 1.0;             /* 1st-order tank reaction rate   */
   m->Kbulk     = 0.0;             /* No global bulk reaction        */
   m->Kwall     = 0.0;             /* No global wall reaction        */
   m->Climit    = 0.0;             /* No limiting potential quality  */
   m->Diffus    = MISSING;         /* Temporary diffusivity          */
   m->Rfactor   = 0.0;             /* No roughness-reaction factor   */
   m->Viscos    = MISSING;         /* Temporary viscosity            */
   m->SpGrav    = SPGRAV;          /* Default specific gravity       */
   m->DefPat    = 0;               /* Default demand pattern index   */
   m->Epat      = 0;               /* No energy price pattern        */
   m->Ecost     = 0.0;             /* Zero unit energy cost          */
   m->Dcost     = 0.0;             /* Zero energy demand charge      */
   m->Epump     = EPUMP;           /* Default pump efficiency        */
   m->Emax      = 0.0;             /* Zero peak energy usage         */
   m->Qexp      = 2.0;             /* Flow exponent for emitters     */
   m->Dmult     = 1.0;             /* Demand multiplier              */
   m->RQtol     = RQTOL;           /* Default hydraulics parameters  */
   m->CheckFreq = CHECKFREQ;
   m->MaxCheck  = MAXCHECK;
   m->DampLimit = DAMPLIMIT;                                                      //(2.00.12 - LR)
}                       /*  End of setdefaults  */


void  initreport(OW_Project *m)
/*
**----------------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: initializes reporting options
**----------------------------------------------------------------------
*/
{
   int i;
   strncpy(m->Rpt2Fname,"",MAXFNAME);
   m->PageSize    = PAGESIZE;      /* Default page size for report   */
   m->Summaryflag = TRUE;          /* Write summary report           */
   m->Messageflag = TRUE;          /* Report error/warning messages  */
   m->Statflag    = FALSE;         /* No hydraulic status reports    */
   m->Energyflag  = FALSE;         /* No energy usage report         */
   m->Nodeflag    = 0;             /* No reporting on nodes          */
   m->Linkflag    = 0;             /* No reporting on links          */
   for (i=0; i < MAXVAR; i++)     /* For each reporting variable:   */
   {
      SField *Field = m->Field;
      strncpy(Field[i].Name,Fldname[i],MAXID);
      Field[i].Enabled = FALSE;        /* Not included in report  */
      Field[i].Precision = 2;          /* 2 decimal precision     */

/*** Updated 6/24/02 ***/
      Field[i].RptLim[LOW] =   SQR(BIG); /* No reporting limits   */
      Field[i].RptLim[HI]  =  -SQR(BIG);
   }
   m->Field[FRICTION].Precision = 3;
   for (i=DEMAND; i<=QUALITY; i++) {
     m->Field[i].Enabled = TRUE;
   }
   for (i=FLOW; i<=HEADLOSS; i++) {
     m->Field[i].Enabled = TRUE;
   }
}


void  adjustdata(OW_Project *m)
/*
**----------------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: adjusts data after input file has been processed
**----------------------------------------------------------------------
*/
{
   int   i;
   double ucf;                   /* Unit conversion factor */
   Pdemand demand;              /* Pointer to demand record */

/* Use 1 hr pattern & report time step if none specified */
   if (m->Pstep <= 0)
     m->Pstep = 3600;
   if (m->Rstep == 0)
     m->Rstep = m->Pstep;

/* Hydraulic time step cannot be greater than pattern or report time step */
   if (m->Hstep <=  0)
     m->Hstep = 3600;
   if (m->Hstep > m->Pstep)
     m->Hstep = m->Pstep;
   if (m->Hstep > m->Rstep)
     m->Hstep = m->Rstep;

/* Report start time cannot be greater than simulation duration */
   if (m->Rstart > m->Dur)
     m->Rstart = 0;

/* No water quality analysis for steady state run */
   if (m->Dur == 0)
     m->Qualflag = NONE;

/* If no quality timestep, then make it 1/10 of hydraulic timestep */
   if (m->Qstep == 0)
     m->Qstep = m->Hstep/10;

/* If no rule time step then make it 1/10 of hydraulic time step; */
/* Rule time step cannot be greater than hydraulic time step */
   if (m->Rulestep == 0)
     m->Rulestep = m->Hstep/10;
  
   m->Rulestep = MIN(m->Rulestep, m->Hstep);

/* Quality timestep cannot exceed hydraulic timestep */
   m->Qstep = MIN(m->Qstep, m->Hstep);

/* If no quality tolerance, then use default values */
   if (m->Ctol == MISSING)
   {
      if (m->Qualflag == AGE)
        m->Ctol = AGETOL;
      else
        m->Ctol = CHEMTOL;
   }

/* Determine unit system based on flow units */
   switch (m->Flowflag)
   {
      case LPS:          /* Liters/sec */
      case LPM:          /* Liters/min */
      case MLD:          /* megaliters/day  */
      case CMH:          /* cubic meters/hr */
      case CMD:          /* cubic meters/day */
         m->Unitsflag = SI;
         break;
      default:
         m->Unitsflag = US;
   }

/* Revise pressure units depending on flow units */
   if (m->Unitsflag != SI)
     m->Pressflag = PSI;
   else if (m->Pressflag == PSI)
     m->Pressflag = METERS;

/* Store value of viscosity & diffusivity */
   ucf = 1.0;
   if (m->Unitsflag == SI)
     ucf = SQR(MperFT);

   if (m->Viscos == MISSING)     /* No value supplied */
      m->Viscos = VISCOS;
   else if (m->Viscos > 1.e-3)   /* Multiplier supplied */
      m->Viscos = m->Viscos*VISCOS;
   else                       /* Actual value supplied */
      m->Viscos = m->Viscos/ucf;

   if (m->Diffus == MISSING)
      m->Diffus = DIFFUS;
   else if (m->Diffus > 1.e-4)
      m->Diffus = m->Diffus*DIFFUS;
   else
      m->Diffus = m->Diffus/ucf;

/*
  Set exponent in head loss equation and adjust flow-resistance tolerance.
*/
   if (m->Formflag == HW)
     m->Hexp = 1.852;
   else
     m->Hexp = 2.0;

/*** Updated 9/7/00 ***/
/*** No adjustment made to flow-resistance tolerance ***/
   /*RQtol = RQtol/Hexp;*/

/* See if default reaction coeffs. apply */
   for (i=1; i <= m->network.Nlinks; i++)
   {
      Slink *Link = m->network.Link;
      if (Link[i].Type > PIPE)
        continue;
      if (Link[i].Kb == MISSING)
        Link[i].Kb = m->Kbulk;  /* Bulk coeff. */
      if (Link[i].Kw == MISSING)                      /* Wall coeff. */
      {
      /* Rfactor is the pipe roughness correlation factor */
         if (m->Rfactor == 0.0)
           Link[i].Kw = m->Kwall;
         else if ((Link[i].Kc > 0.0) && (Link[i].Diam > 0.0))
         {
            if (m->Formflag == HW)
              Link[i].Kw = m->Rfactor/Link[i].Kc;
            if (m->Formflag == DW)
              Link[i].Kw = m->Rfactor/ABS(log(Link[i].Kc/Link[i].Diam));
            if (m->Formflag == CM)
              Link[i].Kw = m->Rfactor*Link[i].Kc;
         }
         else
           Link[i].Kw = 0.0;
      }
   }
   for (i=1; i <= m->network.Ntanks; i++) {
     if (m->network.Tank[i].Kb == MISSING) {
        m->network.Tank[i].Kb = m->Kbulk;
     }
   }

/* Use default pattern if none assigned to a demand */
   for (i=1; i <= m->network.Nnodes; i++) {
      for (demand = m->network.Node[i].D; demand != NULL; demand = demand->next) {
        if (demand->Pat == 0) {
          demand->Pat = m->DefPat;
        }
      }
   }

/* Remove QUALITY as a reporting variable if no WQ analysis */
   if (m->Qualflag == NONE)
     m->Field[QUALITY].Enabled = FALSE;

}                       /*  End of adjustdata  */


int  inittanks(OW_Project *m)
/*
**---------------------------------------------------------------
**  Input:   none
**  Output:  returns error code
**  Purpose: initializes volumes in non-cylindrical tanks
**---------------------------------------------------------------
*/
{
    int   i,j,n = 0;
    double a;
    int   errcode = 0,
          levelerr;

    for (j=1; j <= m->network.Ntanks; j++)
    {
      Scurve *curve;
      Stank *tank = &(m->network.Tank[j]);

    /* Skip reservoirs */
        if (tank->A == 0.0) continue;

    /* Check for valid lower/upper tank levels */
        levelerr = 0;
        if (tank->H0   > tank->Hmax ||
            tank->Hmin > tank->Hmax ||
            tank->H0   < tank->Hmin
           ) levelerr = 1;

    /* Check that tank heights are within volume curve */
        i = tank->Vcurve;
      
        if (i > 0)
        {
           curve = &(m->network.Curve[i]);
           n = m->network.Curve[i].Npts - 1;
           if (tank->Hmin < curve->X[0] || tank->Hmax > curve->X[n]) {
             levelerr = 1;
           }
        }

   /* Report error in levels if found */
        if (levelerr)
        {
            sprintf(m->Msg,ERR225, m->network.Node[tank->Node].ID);
            writeline(m,m->Msg);
            errcode = 200;
        }

    /* Else if tank has a volume curve, */
        else if (i > 0)
        {
        /* Find min., max., and initial volumes from curve */
           tank->Vmin = interp(curve->Npts,curve->X,
                               curve->Y,tank->Hmin);
           tank->Vmax = interp(curve->Npts,curve->X,
                               curve->Y,tank->Hmax);
           tank->V0   = interp(curve->Npts,curve->X,
                               curve->Y,tank->H0);

        /* Find a "nominal" diameter for tank */
           a = (curve->Y[n] - curve->Y[0])/
               (curve->X[n] - curve->X[0]);
           tank->A = sqrt(4.0*a/PI);
        }
    }
    return(errcode);
}                       /* End of inittanks */


void  initunits(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: determines unit conversion factors
**--------------------------------------------------------------
*/
{
   double  dcf,  /* distance conversion factor      */
           ccf,  /* concentration conversion factor */
           qcf,  /* flow conversion factor          */
           hcf,  /* head conversion factor          */
           pcf,  /* pressure conversion factor      */
           wcf;  /* energy conversion factor        */

   SField *Field = m->Field;
   char Flowflag = m->Flowflag;
   char Pressflag = m->Pressflag;
   double SpGrav = m->SpGrav;
   double *Ucf = m->Ucf;
  
   if (m->Unitsflag == SI)                            /* SI units */
   {
      strcpy(Field[DEMAND].Units,RptFlowUnitsTxt[Flowflag]);
      strcpy(Field[ELEV].Units,u_METERS);
      strcpy(Field[HEAD].Units,u_METERS);
      if (Pressflag == METERS) strcpy(Field[PRESSURE].Units,u_METERS);
      else strcpy(Field[PRESSURE].Units,u_KPA);
      strcpy(Field[LENGTH].Units,u_METERS);
      strcpy(Field[DIAM].Units,u_MMETERS);
      strcpy(Field[FLOW].Units,RptFlowUnitsTxt[Flowflag]);
      strcpy(Field[VELOCITY].Units,u_MperSEC);
      strcpy(Field[HEADLOSS].Units,u_per1000M);
      strcpy(Field[FRICTION].Units,"");
      strcpy(Field[POWER].Units,u_KW);
      dcf = 1000.0*MperFT;
      qcf = LPSperCFS;
      if (Flowflag == LPM) qcf = LPMperCFS;
      if (Flowflag == MLD) qcf = MLDperCFS;
      if (Flowflag == CMH) qcf = CMHperCFS;
      if (Flowflag == CMD) qcf = CMDperCFS;
      hcf = MperFT;
      if (Pressflag == METERS) pcf = MperFT * SpGrav;
      else pcf = KPAperPSI*PSIperFT * SpGrav;
      wcf = KWperHP;
   }
   else                                         /* US units */
   {
      strcpy(Field[DEMAND].Units,RptFlowUnitsTxt[Flowflag]);
      strcpy(Field[ELEV].Units,u_FEET);
      strcpy(Field[HEAD].Units,u_FEET);
      strcpy(Field[PRESSURE].Units,u_PSI);
      strcpy(Field[LENGTH].Units,u_FEET);
      strcpy(Field[DIAM].Units,u_INCHES);
      strcpy(Field[FLOW].Units,RptFlowUnitsTxt[Flowflag]);
      strcpy(Field[VELOCITY].Units,u_FTperSEC);
      strcpy(Field[HEADLOSS].Units,u_per1000FT);
      strcpy(Field[FRICTION].Units,"");
      strcpy(Field[POWER].Units,u_HP);
      dcf = 12.0;
      qcf = 1.0;
      if (Flowflag == GPM) qcf = GPMperCFS;
      if (Flowflag == MGD) qcf = MGDperCFS;
      if (Flowflag == IMGD)qcf = IMGDperCFS;
      if (Flowflag == AFD) qcf = AFDperCFS;
      hcf = 1.0;
      pcf = PSIperFT*SpGrav;
      wcf = 1.0;
   }
   strcpy(Field[QUALITY].Units,"");
   ccf = 1.0;
   if (m->Qualflag == CHEM)
   {
      ccf = 1.0/LperFT3;
      strncpy(Field[QUALITY].Units, m->ChemUnits,MAXID);
      strncpy(Field[REACTRATE].Units, m->ChemUnits,MAXID);
      strcat(Field[REACTRATE].Units, t_PERDAY);
   }
   else if (m->Qualflag == AGE)
     strcpy(Field[QUALITY].Units,u_HOURS);
   else if (m->Qualflag == TRACE)
     strcpy(Field[QUALITY].Units,u_PERCENT);
  
   Ucf[DEMAND]    = qcf;
   Ucf[ELEV]      = hcf;
   Ucf[HEAD]      = hcf;
   Ucf[PRESSURE]  = pcf;
   Ucf[QUALITY]   = ccf;
   Ucf[LENGTH]    = hcf;
   Ucf[DIAM]      = dcf;
   Ucf[FLOW]      = qcf;
   Ucf[VELOCITY]  = hcf;
   Ucf[HEADLOSS]  = hcf;
   Ucf[LINKQUAL]  = ccf;
   Ucf[REACTRATE] = ccf;
   Ucf[FRICTION]  = 1.0;
   Ucf[POWER]     = wcf;
   Ucf[VOLUME]    = hcf*hcf*hcf;
   if (m->Hstep < 1800)                    /* Report time in mins.    */
   {                                    /* if hydraulic time step  */
      Ucf[TIME] = 1.0/60.0;             /* is less than 1/2 hour.  */
      strcpy(Field[TIME].Units,u_MINUTES);
   }
   else
   {
      Ucf[TIME] = 1.0/3600.0;
      strcpy(Field[TIME].Units,u_HOURS);
   }

}                       /*  End of initunits  */


void  convertunits(OW_Project *m)
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: converts units of input data
**--------------------------------------------------------------
*/
{
   int   i,j,k;
   double ucf;        /* Unit conversion factor */
   Pdemand demand;   /* Pointer to demand record */

  double *Ucf = m->Ucf;
  Scontrol *Control = m->network.Control;
  Snode *Node = m->network.Node;
  
/* Convert nodal elevations & initial WQ */
/* (WQ source units are converted in QUALITY.C */
   for (i=1; i <= m->network.Nnodes; i++)
   {
      m->network.Node[i].El /= Ucf[ELEV];
      m->network.Node[i].C0 /= Ucf[QUALITY];
   }

/* Convert demands */
   for (i=1; i <= m->network.Njuncs; i++)
   {
       for (demand = m->network.Node[i].D; demand != NULL; demand = demand->next)
          demand->Base /= Ucf[DEMAND];
   }

/* Convert emitter discharge coeffs. to head loss coeff. */
   ucf = pow(Ucf[FLOW], m->Qexp) / Ucf[PRESSURE];
  for (i=1; i <= m->network.Njuncs; i++) {
    if (m->network.Node[i].Ke > 0.0) {
       m->network.Node[i].Ke = ucf/pow(m->network.Node[i].Ke, m->Qexp);
    }
  }

/* Initialize tank variables (convert tank levels to elevations) */
   for (j=1; j <= m->network.Ntanks; j++)
   {
     Stank *tank = &(m->network.Tank[j]);
     Snode *node = &(m->network.Node[tank->Node]);
      i = tank->Node;
      tank->H0 = node->El + tank->H0 / Ucf[ELEV];
      tank->Hmin = node->El + tank->Hmin / Ucf[ELEV];
      tank->Hmax = node->El + tank->Hmax / Ucf[ELEV];
      tank->A = PI*SQR(tank->A / Ucf[ELEV])/4.0;
      tank->V0 /= Ucf[VOLUME];
      tank->Vmin /= Ucf[VOLUME];
      tank->Vmax /= Ucf[VOLUME];
      tank->Kb /= SECperDAY;
      tank->V = tank->V0;
      tank->C = node->C0;
      tank->V1max *= tank->Vmax;
   }

/* Convert WQ option concentration units */
   m->Climit /= Ucf[QUALITY];
   m->Ctol   /= Ucf[QUALITY];

/* Convert global reaction coeffs. */
   m->Kbulk /= SECperDAY;
   m->Kwall /= SECperDAY;

/* Convert units of link parameters */
   for (k=1; k <= m->network.Nlinks; k++)
   {
     Slink *link = &(m->network.Link[k]);
      if (link->Type <= PIPE)
      {
      /* Convert pipe parameter units:                         */
      /*    - for Darcy-Weisbach formula, convert roughness    */
      /*      from millifeet (or mm) to ft (or m)              */
      /*    - for US units, convert diameter from inches to ft */
         if (m->Formflag  == DW) {
           link->Kc /= (1000.0 * Ucf[ELEV]);
         }
         link->Diam /= Ucf[DIAM];
         link->Len /= Ucf[LENGTH];

      /* Convert minor loss coeff. from V^2/2g basis to Q^2 basis */
         link->Km = 0.02517*link->Km/SQR(link->Diam)/SQR(link->Diam);
      
      /* Convert units on reaction coeffs. */
         link->Kb /= SECperDAY;
         link->Kw /= SECperDAY;
      }

      else if (link->Type == PUMP )
      {
      /* Convert units for pump curve parameters */
         i = link->pumpLinkIdx;
         Spump *pump = &(m->network.Pump[i]);
         if (pump->Ptype == CONST_HP)
         {
         /* For constant hp pump, convert kw to hp */
           if (m->Unitsflag == SI) {
             pump->R /= Ucf[POWER];
           }
         }
         else
         {
         /* For power curve pumps, convert     */
         /* shutoff head and flow coefficient  */
            if (pump->Ptype == POWER_FUNC)
            {
               pump->H0 /= Ucf[HEAD];
               pump->R  *= (pow(Ucf[FLOW],pump->N)/Ucf[HEAD]);
            }
         /* Convert flow range & max. head units */
            pump->Q0   /= Ucf[FLOW];
            pump->Qmax /= Ucf[FLOW];
            pump->Hmax /= Ucf[HEAD];
         }
      }

      else
      {
      /* For flow control valves, convert flow setting    */
      /* while for other valves convert pressure setting  */
         link->Diam /= Ucf[DIAM];
         link->Km = 0.02517*link->Km/SQR(link->Diam)/SQR(link->Diam);
         if (link->Kc != MISSING) {
           switch (link->Type)
           {
             case FCV:
               link->Kc /= Ucf[FLOW];
               break;
             case PRV:
             case PSV:
             case PBV:
               link->Kc /= Ucf[PRESSURE];
               break;
           }
         }
      }

   /* Compute flow resistances */
      resistance(m,k);
   }

/* Convert units on control settings */
   for (i=1; i <= m->network.Ncontrols; i++)
   {
      if ( (k = Control[i].Link) == 0)
        continue;
      if ( (j = Control[i].Node) > 0) {
      /* j = index of controlling node, and if           */
      /* j > Njuncs, then control is based on tank level */
      /* otherwise control is based on nodal pressure    */
         if (j > m->network.Njuncs)
              Control[i].Grade = Node[j].El + Control[i].Grade/Ucf[ELEV];
         else Control[i].Grade = Node[j].El + Control[i].Grade/Ucf[PRESSURE];
      }

      /* Convert units on valve settings */
      if (Control[i].Setting != MISSING) switch (m->network.Link[k].Type)
      {
         case PRV:
         case PSV:
         case PBV:
            Control[i].Setting /= Ucf[PRESSURE];
            break;
         case FCV:
            Control[i].Setting /= Ucf[FLOW];
      }
   }
}                       /*  End of convertunits  */

/************************ END OF INPUT1.C ************************/

