/*
*********************************************************************
                                                                   
INPFILE.C -- Save Input Function for EPANET Program                
                                                                    
VERSION:    2.00                                               
DATE:       5/8/00
            3/1/01
            11/19/01                                          
            6/24/02
            8/15/07    (2.00.11)
            2/14/08    (2.00.12)
AUTHOR:     L. Rossman
            US EPA - NRMRL
                                                                    
This module contains the function saveinpfile() which saves the
data describing a piping network to a file in EPANET's text format.                                    
                                                                    
********************************************************************
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

///* Defined in enumstxt.h in EPANET.C */
extern char *LinkTxt[];
extern char *FormTxt[];
extern char *StatTxt[];
extern char *FlowUnitsTxt[];
extern char *PressUnitsTxt[];
extern char *ControlTxt[];
extern char *SourceTxt[];
extern char *MixTxt[];
extern char *TstatTxt[];
extern char *RptFlagTxt[];
extern char *SectTxt[];


void  saveauxdata(OW_Project *m, FILE *f)                                                     //(2.00.12 - LR)
/*
------------------------------------------------------------
  Writes auxilary data from original input file to new file.
------------------------------------------------------------
*/
{
   int   sect,newsect;
   char  *tok; 
   char  line[MAXLINE+1];
   char  s[MAXLINE+1];

   sect = -1;
   rewind(m->InFile);
   while (fgets(line,MAXLINE,m->InFile) != NULL)
   {
   /* Check if line begins with a new section heading */
      strcpy(s,line);
      tok = strtok(s,SEPSTR);
      if (tok != NULL && *tok == '[')
      {
         newsect = findmatch(tok,SectTxt);
         if (newsect >= 0)
         {
            sect = newsect;
            if (sect == _END) break;
            switch(sect)
            {
               case _RULES:
               case _COORDS:
               case _VERTICES:
               case _LABELS:
               case _BACKDROP:
               case _TAGS: fprintf(f, "%s", line);                             //(2.00.12 - LR)
            }
            continue;
         }
         else continue;
      }

   /* Write lines appearing in the section to file */
      switch(sect)
      {
          case _RULES:
          case _COORDS:
          case _VERTICES:
          case _LABELS:
          case _BACKDROP:
          case _TAGS: fprintf(f, "%s", line);                                  //(2.00.12 - LR)
      }
   }
}


////  This function was heavily modified.  ////                                //(2.00.12 - LR)

int  saveinpfile(OW_Project *m, char *fname)
/*
-------------------------------------------------
  Writes network data to text file.
-------------------------------------------------
*/
{
   int     i,j,n;
   double  d,kc,ke,km,ucf;
   char    s[MAXLINE+1], s1[MAXLINE+1], s2[MAXLINE+1];
   Pdemand demand;
   Psource source;
   FILE    *f;

/* Open the new text file */

   if ((f = fopen(fname,"wt")) == NULL) return(308);

/* Write [TITLE] section */

   fprintf(f,"[TITLE]");
   for (i=0; i<3; i++)
   {
      if (strlen(m->Title[i]) > 0) fprintf(f,"\n%s",m->Title[i]);
   }

/* Write [JUNCTIONS] section */
/* (Leave demands for [DEMANDS] section) */

   fprintf(f,"\n\n[JUNCTIONS]");
   for (i=1; i <= m->Njuncs; i++)
      fprintf(f,"\n %-31s %12.4f", m->Node[i].ID, m->Node[i].El * m->Ucf[ELEV]);

/* Write [RESERVOIRS] section */

   fprintf(f,"\n\n[RESERVOIRS]");
   for (i=1; i <= m->Ntanks; i++)
   {
      if (m->Tank[i].A == 0.0)
      {
         n = m->Tank[i].Node;
         sprintf(s," %-31s %12.4f",m->Node[n].ID, m->Node[n].El * m->Ucf[ELEV]);
         if ((j = m->Tank[i].Pat) > 0)
            sprintf(s1," %-31s",m->Pattern[j].ID);
         else
            strcpy(s1,"");
         fprintf(f, "\n%s %s", s,s1);
      }
   }

/* Write [TANKS] section */

   fprintf(f,"\n\n[TANKS]");
   for (i=1; i <= m->Ntanks; i++)
   {
      if (m->Tank[i].A > 0.0)
      {
         n = m->Tank[i].Node;
         sprintf(s," %-31s %12.4f %12.4f %12.4f %12.4f %12.4f %12.4f",
            m->Node[n].ID,
            m->Node[n].El * m->Ucf[ELEV],
            (m->Tank[i].H0 - m->Node[n].El) * m->Ucf[ELEV],
            (m->Tank[i].Hmin - m->Node[n].El) * m->Ucf[ELEV],
            (m->Tank[i].Hmax - m->Node[n].El) * m->Ucf[ELEV],
            sqrt(4.0 * m->Tank[i].A/PI) * m->Ucf[ELEV],
            m->Tank[i].Vmin*SQR(m->Ucf[ELEV]) * m->Ucf[ELEV]);
         if ((j = m->Tank[i].Vcurve) > 0)
            sprintf(s1,"%-31s",m->Curve[j].ID);
         else
            strcpy(s1,"");
         fprintf(f, "\n%s %s", s,s1);
      }
   }

/* Write [PIPES] section */

   fprintf(f,"\n\n[PIPES]");
   for (i=1; i <= m->Nlinks; i++)
   {
      if (m->Link[i].Type <= PIPE)
      {
         d = m->Link[i].Diam;
         kc = m->Link[i].Kc;
        
         if (m->Formflag == DW)
           kc = kc * m->Ucf[ELEV]*1000.0;
        
         km = m->Link[i].Km*SQR(d)*SQR(d)/0.02517;
         sprintf(s," %-31s %-31s %-31s %12.4f %12.4f",
            m->Link[i].ID,
            m->Node[m->Link[i].N1].ID,
            m->Node[m->Link[i].N2].ID,
            m->Link[i].Len * m->Ucf[LENGTH],
            d * m->Ucf[DIAM]);
        
         if (m->Formflag == DW)
           sprintf(s1, "%12.4f %12.4f", kc, km);
         else
           sprintf(s1, "%12.4f %12.4f", kc, km);
        
         if (m->Link[i].Type == CV)
           sprintf(s2,"CV");
         else if (m->Link[i].Stat == CLOSED)
           sprintf(s2,"CLOSED");
         else
           strcpy(s2,"");
        
         fprintf(f,"\n%s %s %s",s,s1,s2);
      }
   }

/* Write [PUMPS] section */

   fprintf(f, "\n\n[PUMPS]");
   for (i=1; i <= m->Npumps; i++)
   {
      Spump *pump = &(m->Pump[i]);
      n = pump->Link;
      sprintf(s," %-31s %-31s %-31s",
         m->Link[n].ID,
         m->Node[m->Link[n].N1].ID,
         m->Node[m->Link[n].N2].ID);

   /* Pump has constant power */
      if (pump->Ptype == CONST_HP)
         sprintf(s1, "  POWER %.4f", m->Link[n].Km);

   /* Pump has a head curve */
      else if ((j = pump->Hcurve) > 0)
         sprintf(s1, "  HEAD %s", m->Curve[j].ID);

   /* Old format used for pump curve */
      else
      {
         fprintf(f, "\n%s %12.4f %12.4f %12.4f          0.0 %12.4f",s,
                 -pump->H0 * m->Ucf[HEAD],
                 (-pump->H0 - pump->R * pow(pump->Q0,pump->N)) * m->Ucf[HEAD],
                 pump->Q0 * m->Ucf[FLOW],
                 pump->Qmax * m->Ucf[FLOW]);
         continue;
      }
      strcat(s,s1);

      if ((j = pump->Upat) > 0)
         sprintf(s1,"   PATTERN  %s",m->Pattern[j].ID);
      else strcpy(s1,"");
      strcat(s,s1);

      if (m->Link[n].Kc != 1.0)
         sprintf(s1, "  SPEED %.4f", m->Link[n].Kc);
      else strcpy(s1,"");
      strcat(s,s1);

      fprintf(f,"\n%s",s);
   }

/* Write [VALVES] section */

   fprintf(f, "\n\n[VALVES]");
   for (i=1; i <= m->Nvalves; i++)
   {
      n = m->Valve[i].Link;
      d = m->Link[n].Diam;
      kc = m->Link[n].Kc;
      if (kc == MISSING) kc = 0.0;
      switch (m->Link[n].Type)
      {
         case FCV: kc *= m->Ucf[FLOW]; break;
         case PRV:
         case PSV:
         case PBV: kc *= m->Ucf[PRESSURE]; break;
      }
      km = m->Link[n].Km*SQR(d)*SQR(d)/0.02517;

      sprintf(s," %-31s %-31s %-31s %12.4f %5s",
         m->Link[n].ID,
         m->Node[m->Link[n].N1].ID,
         m->Node[m->Link[n].N2].ID,
         d * m->Ucf[DIAM],
         LinkTxt[m->Link[n].Type]);

      if (m->Link[n].Type == GPV && (j = ROUND(m->Link[n].Kc)) > 0)
         sprintf(s1,"%-31s %12.4f", m->Curve[j].ID, km);
      else sprintf(s1,"%12.4f %12.4f",kc,km);

      fprintf(f, "\n%s %s", s,s1);
   }

/* Write [DEMANDS] section */
   
   fprintf(f, "\n\n[DEMANDS]");
   ucf = m->Ucf[DEMAND];
   for (i=1; i <= m->Njuncs; i++)
   {
      for (demand = m->Node[i].D; demand != NULL; demand = demand->next)
      {
         sprintf(s," %-31s %14.6f", m->Node[i].ID, ucf*demand->Base);
         if ((j = demand->Pat) > 0) sprintf(s1,"   %s", m->Pattern[j].ID);
         else strcpy(s1,"");
         fprintf(f,"\n%s %s",s,s1);
      }
   }

/* Write [EMITTERS] section */

   fprintf(f, "\n\n[EMITTERS]");
   for (i=1; i <= m->Njuncs; i++)
   {
      if (m->Node[i].Ke == 0.0) continue;
      ke = m->Ucf[FLOW]/pow(m->Ucf[PRESSURE] * m->Node[i].Ke, (1.0 / m->Qexp));
      fprintf(f,"\n %-31s %14.6f",m->Node[i].ID,ke);
   }

/* Write [STATUS] section */

   fprintf(f, "\n\n[STATUS]");
   for (i=1; i <= m->Nlinks; i++)
   {
      if (m->Link[i].Type <= PUMP)
      {
         if (m->Link[i].Stat == CLOSED)
            fprintf(f, "\n %-31s %s", m->Link[i].ID, StatTxt[CLOSED]);

      /* Write pump speed here for pumps with old-style pump curve input */
         else if (m->Link[i].Type == PUMP)
         {
            n = m->Link[i].pumpLinkIdx;
            if (
                 m->Pump[n].Hcurve == 0 &&
                 m->Pump[n].Ptype != CONST_HP &&
                 m->Link[i].Kc != 1.0
               )
               fprintf(f, "\n %-31s %-.4f", m->Link[i].ID, m->Link[i].Kc);
         }
      }

   /* Write fixed-status PRVs & PSVs (setting = MISSING) */
      else if (m->Link[i].Kc == MISSING)
      {
         if (m->Link[i].Stat == OPEN)
            fprintf(f, "\n %-31s %s", m->Link[i].ID, StatTxt[OPEN]);
         if (m->Link[i].Stat == CLOSED)
            fprintf(f, "\n%-31s %s", m->Link[i].ID, StatTxt[CLOSED]);
      }
   }

/* Write [PATTERNS] section */
/* (Use 6 pattern factors per line) */

   fprintf(f, "\n\n[PATTERNS]");
   for (i=1; i <= m->Npats; i++)
   {
      for (j=0; j < m->Pattern[i].Length; j++)
      {
        if (j % 6 == 0) fprintf(f,"\n %-31s", m->Pattern[i].ID);
        fprintf(f," %12.4f", m->Pattern[i].F[j]);
      }
   }

/* Write [CURVES] section */

   fprintf(f, "\n\n[CURVES]");
   for (i=1; i <= m->Ncurves; i++)
   {
      for (j=0; j < m->Curve[i].Npts; j++)
         fprintf(f,"\n %-31s %12.4f %12.4f",
            m->Curve[i].ID, m->Curve[i].X[j], m->Curve[i].Y[j]);
   }

/* Write [CONTROLS] section */

   fprintf(f, "\n\n[CONTROLS]");
   for (i=1; i <= m->Ncontrols; i++)
   {
   /* Check that controlled link exists */
      if ( (j = m->Control[i].Link) <= 0) continue;

   /* Get text of control's link status/setting */
      if (m->Control[i].Setting == MISSING)
         sprintf(s, " LINK %s %s ", m->Link[j].ID, StatTxt[m->Control[i].Status]);
      else
      {
         kc = m->Control[i].Setting;
         switch(m->Link[j].Type)
         {
            case PRV:
            case PSV:
            case PBV: kc *= m->Ucf[PRESSURE]; break;
            case FCV: kc *= m->Ucf[FLOW];     break;
         }
         sprintf(s, " LINK %s %.4f", m->Link[j].ID, kc);
      }
      
      switch (m->Control[i].Type)
      {
      /* Print level control */
         case LOWLEVEL:
         case HILEVEL:
            n = m->Control[i].Node;
            kc = m->Control[i].Grade - m->Node[n].El;
            if (n > m->Njuncs) kc *= m->Ucf[HEAD];
            else            kc *= m->Ucf[PRESSURE];
            fprintf(f, "\n%s IF NODE %s %s %.4f", s,
               m->Node[n].ID, ControlTxt[m->Control[i].Type], kc);
            break;

      /* Print timer control */
         case TIMER:
            fprintf(f, "\n%s AT %s %.4f HOURS",
               s, ControlTxt[TIMER], m->Control[i].Time/3600.);
            break;
                         
      /* Print time-of-day control */
         case TIMEOFDAY:
            fprintf(f, "\n%s AT %s %s",
               s, ControlTxt[TIMEOFDAY], clocktime(m->Atime, m->Control[i].Time));
            break;
      }
   }            

/* Write [QUALITY] section */
/* (Skip nodes with default quality of 0) */

   fprintf(f, "\n\n[QUALITY]");
   for (i=1; i <= m->Nnodes; i++)
   {
      if (m->Node[i].C0 == 0.0) continue;
      fprintf(f, "\n %-31s %14.6f", m->Node[i].ID, m->Node[i].C0 * m->Ucf[QUALITY]);
   }
      
/* Write [SOURCES] section */

   fprintf(f, "\n\n[SOURCES]");
   for (i=1; i <= m->Nnodes; i++)
   {
      source = m->Node[i].S;
      if (source == NULL) continue;
      sprintf(s," %-31s %-8s %14.6f",
         m->Node[i].ID,
         SourceTxt[source->Type],
         source->C0);
      if ((j = source->Pat) > 0)
         sprintf(s1,"%s",m->Pattern[j].ID);
      else strcpy(s1,"");
      fprintf(f,"\n%s %s",s,s1);
   }

/* Write [MIXING] section */

   fprintf(f, "\n\n[MIXING]");
   for (i=1; i <= m->Ntanks; i++)
   {
      if (m->Tank[i].A == 0.0) continue;
      fprintf(f, "\n %-31s %-8s %12.4f",
              m->Node[m->Tank[i].Node].ID,
              MixTxt[m->Tank[i].MixModel],
              (m->Tank[i].V1max / m->Tank[i].Vmax));
   }

/* Write [REACTIONS] section */

   fprintf(f, "\n\n[REACTIONS]");
   fprintf(f, "\n ORDER  BULK            %-.2f", m->BulkOrder);
   fprintf(f, "\n ORDER  WALL            %-.0f", m->WallOrder);
   fprintf(f, "\n ORDER  TANK            %-.2f", m->TankOrder);
   fprintf(f, "\n GLOBAL BULK            %-.6f", m->Kbulk*SECperDAY);
   fprintf(f, "\n GLOBAL WALL            %-.6f", m->Kwall*SECperDAY);
   if (m->Climit > 0.0)
   fprintf(f, "\n LIMITING POTENTIAL     %-.6f", m->Climit);
   if (m->Rfactor != MISSING && m->Rfactor != 0.0)
   fprintf(f, "\n ROUGHNESS CORRELATION  %-.6f", m->Rfactor);
   for (i=1; i <= m->Nlinks; i++)
   {
      if (m->Link[i].Type > PIPE) continue;
      if (m->Link[i].Kb != m->Kbulk)
         fprintf(f, "\n BULK   %-31s %-.6f", m->Link[i].ID, m->Link[i].Kb*SECperDAY);
      if (m->Link[i].Kw != m->Kwall)
         fprintf(f, "\n WALL   %-31s %-.6f", m->Link[i].ID, m->Link[i].Kw*SECperDAY);
   }
   for (i=1; i <= m->Ntanks; i++)
   {
      if (m->Tank[i].A == 0.0) continue;
      if (m->Tank[i].Kb != m->Kbulk)
         fprintf(f, "\n TANK   %-31s %-.6f",m->Node[m->Tank[i].Node].ID,
            m->Tank[i].Kb*SECperDAY);
   }

/* Write [ENERGY] section */

   fprintf(f, "\n\n[ENERGY]");
   if (m->Ecost != 0.0)
   fprintf(f, "\n GLOBAL PRICE        %-.4f", m->Ecost);
   if (m->Epat != 0)
   fprintf(f, "\n GLOBAL PATTERN      %s",  m->Pattern[m->Epat].ID);
   fprintf(f, "\n GLOBAL EFFIC        %-.4f", m->Epump);
   fprintf(f, "\n DEMAND CHARGE       %-.4f", m->Dcost);
   for (i=1; i <= m->Npumps; i++)
   {
      if (m->Pump[i].Ecost > 0.0)
         fprintf(f, "\n PUMP %-31s PRICE   %-.4f",
            m->Link[m->Pump[i].Link].ID, m->Pump[i].Ecost);
      if (m->Pump[i].Epat > 0.0)
         fprintf(f, "\n PUMP %-31s PATTERN %s",
            m->Link[m->Pump[i].Link].ID,m->Pattern[m->Pump[i].Epat].ID);
      if (m->Pump[i].Ecurve > 0.0)
         fprintf(f, "\n PUMP %-31s EFFIC   %s",
            m->Link[m->Pump[i].Link].ID,m->Curve[m->Pump[i].Ecurve].ID);
   }

/* Write [TIMES] section */

   char *Atime = m->Atime;
  
   fprintf(f, "\n\n[TIMES]");
   fprintf(f, "\n DURATION            %s",clocktime(Atime,m->Dur));
   fprintf(f, "\n HYDRAULIC TIMESTEP  %s",clocktime(Atime,m->Hstep));
   fprintf(f, "\n QUALITY TIMESTEP    %s",clocktime(Atime,m->Qstep));
   fprintf(f, "\n REPORT TIMESTEP     %s",clocktime(Atime,m->Rstep));
   fprintf(f, "\n REPORT START        %s",clocktime(Atime,m->Rstart));
   fprintf(f, "\n PATTERN TIMESTEP    %s",clocktime(Atime,m->Pstep));
   fprintf(f, "\n PATTERN START       %s",clocktime(Atime,m->Pstart));
   fprintf(f, "\n RULE TIMESTEP       %s",clocktime(Atime,m->Rulestep));
   fprintf(f, "\n START CLOCKTIME     %s",clocktime(Atime,m->Tstart));
   fprintf(f, "\n STATISTIC           %s",TstatTxt[m->Tstatflag]);

/* Write [OPTIONS] section */

   fprintf(f, "\n\n[OPTIONS]");
   fprintf(f, "\n UNITS               %s", FlowUnitsTxt[m->Flowflag]);
   fprintf(f, "\n PRESSURE            %s", PressUnitsTxt[m->Pressflag]);
   fprintf(f, "\n HEADLOSS            %s", FormTxt[m->Formflag]);
   if (m->DefPat >= 1 && m->DefPat <= m->Npats)
   fprintf(f, "\n PATTERN             %s", m->Pattern[m->DefPat].ID);
   if (m->Hydflag == USE)
   fprintf(f, "\n HYDRAULICS USE      %s", m->HydFname);
   if (m->Hydflag == SAVE)
   fprintf(f, "\n HYDRAULICS SAVE     %s", m->HydFname);
   if (m->ExtraIter == -1)
   fprintf(f, "\n UNBALANCED          STOP");
   if (m->ExtraIter >= 0)
   fprintf(f, "\n UNBALANCED          CONTINUE %d", m->ExtraIter);
   if (m->Qualflag == CHEM)
   fprintf(f, "\n QUALITY             %s %s", m->ChemName, m->ChemUnits);
   if (m->Qualflag == TRACE)
   fprintf(f, "\n QUALITY             TRACE %-31s", m->Node[m->TraceNode].ID);
   if (m->Qualflag == AGE)
   fprintf(f, "\n QUALITY             AGE");
   if (m->Qualflag == NONE)
   fprintf(f, "\n QUALITY             NONE");
   fprintf(f, "\n DEMAND MULTIPLIER   %-.4f", m->Dmult);
   fprintf(f, "\n EMITTER EXPONENT    %-.4f", 1.0 / m->Qexp);
   fprintf(f, "\n VISCOSITY           %-.6f", m->Viscos/VISCOS);
   fprintf(f, "\n DIFFUSIVITY         %-.6f", m->Diffus/DIFFUS);
   fprintf(f, "\n SPECIFIC GRAVITY    %-.6f", m->SpGrav);
   fprintf(f, "\n TRIALS              %-d",   m->MaxIter);
   fprintf(f, "\n ACCURACY            %-.8f", m->Hacc);
   fprintf(f, "\n TOLERANCE           %-.8f", m->Ctol * m->Ucf[QUALITY]);
   fprintf(f, "\n CHECKFREQ           %-d", m->CheckFreq);
   fprintf(f, "\n MAXCHECK            %-d", m->MaxCheck);
   fprintf(f, "\n DAMPLIMIT           %-.8f", m->DampLimit);

/* Write [REPORT] section */

   fprintf(f, "\n\n[REPORT]");
   fprintf(f, "\n PAGESIZE            %d", m->PageSize);
   fprintf(f, "\n STATUS              %s", RptFlagTxt[m->Statflag]);
   fprintf(f, "\n SUMMARY             %s", RptFlagTxt[m->Summaryflag]);
   fprintf(f, "\n ENERGY              %s", RptFlagTxt[m->Energyflag]);
   switch (m->Nodeflag)
   {
      case 0:
      fprintf(f, "\n NODES               NONE");
      break;
      case 1:
      fprintf(f, "\n NODES               ALL");
      break;
      default:
      j = 0;
      for (i=1; i <= m->Nnodes; i++)
      {
         if (m->Node[i].Rpt == 1)
         {
            if (j % 5 == 0) fprintf(f, "\n NODES               ");
            fprintf(f, "%s ", m->Node[i].ID);
            j++;
         }
      }
   }
   switch (m->Linkflag)
   {
      case 0:
      fprintf(f, "\n LINKS               NONE");
      break;
      case 1:
      fprintf(f, "\n LINKS               ALL");
      break;
      default:
      j = 0;
      for (i=1; i <= m->Nlinks; i++)
      {
         if (m->Link[i].Rpt == 1)
         {
            if (j % 5 == 0) fprintf(f, "\n LINKS               ");
            fprintf(f, "%s ", m->Link[i].ID);
            j++;
         }
      }
   }
   for (i=0; i<FRICTION; i++)
   {
      SField *Field = m->Field;
      if (Field[i].Enabled == TRUE)
      {
         fprintf(f, "\n %-20sPRECISION %d", Field[i].Name, Field[i].Precision);
         if (Field[i].RptLim[LOW] < BIG)
            fprintf(f, "\n %-20sBELOW %.6f", Field[i].Name, Field[i].RptLim[LOW]);
         if (Field[i].RptLim[HI] > -BIG)
            fprintf(f, "\n %-20sABOVE %.6f", Field[i].Name, Field[i].RptLim[HI]);
      }
      else fprintf(f, "\n %-20sNO", Field[i].Name);
   }
   fprintf(f, "\n");

/* Save auxilary data to new input file */
   
   saveauxdata(m,f);

/* Close the new input file */

   fprintf(f, "\n[END]");
   fclose(f);
   return(0);
}

