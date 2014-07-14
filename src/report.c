/*
*********************************************************************
                                                                   
REPORT.C -- Reporting Routines for EPANET Program                
                                                                    
VERSION:    2.00                                               
DATE:       5/30/00
            6/24/02
            8/15/07  (2.00.11)
            2/14/08  (2.00.12)
AUTHOR:     L. Rossman
            US EPA - NRMRL
                                                                    
This module contains various procedures (all beginning with       
'write') that are called from other modules to write formatted    
output to a report file.

It also contains function disconnected(), called from writehydwarn()
and writehyderr(), that checks if a hydraulic solution causes a
network to become disconnected. 

The function writeline(m,s) is used throughout to write a
formatted string S to the report file.                                          
                                                                    
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
#include <time.h>
#include "hash.h"
#include "text.h"
#include "epanet2.h"
#include "types.h"
#include "funcs.h"
#define  EXTERN  extern
#include "vars.h"

#define   MAXCOUNT 10     /* Max. # of disconnected nodes listed */
long      LineNum;        /* Current line number     */
long      PageNum;        /* Current page number     */
char      DateStamp[26];  /* Current date & time     */
char      Fprinterr;      /* File write error flag   */

///* Defined in enumstxt.h in EPANET.C */
extern char *NodeTxt[];
extern char *LinkTxt[];
extern char *StatTxt[];
extern char *TstatTxt[];
extern char *LogoTxt[];
extern char *RptFormTxt[];


typedef   REAL4 *Pfloat;
void      writenodetable(OW_Model *m, Pfloat *);
void      writelinktable(OW_Model *m, Pfloat *);


int  writereport(OW_Model *m)
/*
**------------------------------------------------------
**   Input:   none                                      
**   Output:  returns error code                        
**   Purpose: writes formatted output report to file    
**                                                      
**   Calls strcomp() from the EPANET.C module.                                            
**------------------------------------------------------
*/
{
   char  tflag;
   FILE  *tfile;
   int   errcode = 0;

   /* If no secondary report file specified then    */
   /* write formatted output to primary report file. */
   Fprinterr = FALSE;
   if (m->Rptflag && strlen(m->Rpt2Fname) == 0 && m->RptFile != NULL)
   {
      writecon(FMT17);
      writecon(m->Rpt1Fname);
      if (m->Energyflag) writeenergy(m);
      errcode = writeresults(m);
   }

   /* A secondary report file was specified */
   else if (strlen(m->Rpt2Fname) > 0)
   {

      /* If secondary report file has same name as either input */
      /* or primary report file then use primary report file.   */
      if (strcomp(m->Rpt2Fname,m->InpFname) ||
          strcomp(m->Rpt2Fname,m->Rpt1Fname))
      {
         writecon(FMT17);
         writecon(m->Rpt1Fname);
         if (m->Energyflag) writeenergy(m);
         errcode = writeresults(m);
      }

      /* Otherwise write report to secondary report file. */
      else
      {

         /* Try to open file */
         tfile = m->RptFile;
         tflag = m->Rptflag;
         if ((m->RptFile = fopen(m->Rpt2Fname,"wt")) == NULL)
         {
            m->RptFile = tfile;
            m->Rptflag = tflag;
            errcode = 303;
         }

         /* Write full formatted report to file */
         else
         {
            m->Rptflag = 1; 
            writecon(FMT17);
            writecon(m->Rpt2Fname);
            writelogo(m);
            if (m->Summaryflag) writesummary(m);
            if (m->Energyflag)  writeenergy(m);
            errcode = writeresults(m);
            fclose(m->RptFile);
            m->RptFile = tfile;
            m->Rptflag = tflag;
         }
      }
   }

   /* Special error handler for write-to-file error */
   if (Fprinterr) {
     errmsg(m, 309);
   }
   return(errcode);
}                        /* End of writereport */


void  writelogo(OW_Model *m)
/*
**--------------------------------------------------------------
**   Input:   none                                                
**   Output:  none                                                
**   Purpose: writes program logo to report file.                 
**--------------------------------------------------------------
*/
{
   int    i;
   time_t timer;         /* time_t structure & functions time() & */
                         /* ctime() are defined in time.h         */
   time(&timer);
   strcpy(DateStamp,ctime(&timer));
   PageNum = 1;
   LineNum = 2;
   fprintf(m->RptFile,FMT18);
   fprintf(m->RptFile,"%s",DateStamp);
   for (i=0; LogoTxt[i] != NULL; i++) writeline(m,LogoTxt[i]);
   writeline(m,"");
}                        /* End of writelogo */


void  writesummary(OW_Model *m)
/*
**--------------------------------------------------------------
**   Input:   none                                                
**   Output:  none                                                
**   Purpose: writes summary system information to report file    
**--------------------------------------------------------------
*/
{
   char s[MAXFNAME+1];
   int  i;
   int  nres = 0;

   for (i=0; i<3; i++)
   {
      if (strlen(m->Title[i]) > 0)
      {
         sprintf(s,"%-.70s",m->Title[i]);
         writeline(m,s);
      }
   }
   writeline(m," ");
   sprintf(s,FMT19,m->InpFname);
   writeline(m,s);
   sprintf(s,FMT20,m->Njuncs);
   writeline(m,s);
   for (i=1; i <= m->Ntanks; i++) {
     if (m->Tank[i].A == 0.0) {
       nres++;
     }
   }
   sprintf(s,FMT21a,nres);
   writeline(m,s);
   sprintf(s,FMT21b,m->Ntanks-nres);
   writeline(m,s);
   sprintf(s,FMT22,m->Npipes);
   writeline(m,s);
   sprintf(s,FMT23,m->Npumps);
   writeline(m,s);
   sprintf(s,FMT24,m->Nvalves);
   writeline(m,s);
   sprintf(s,FMT25,RptFormTxt[m->Formflag]);
   writeline(m,s);
   sprintf(s,FMT26, m->Hstep * m->Ucf[TIME], m->Field[TIME].Units);
   writeline(m,s);
   sprintf(s,FMT27,m->Hacc);
   writeline(m,s);

   sprintf(s,FMT27a,m->CheckFreq);                                                //(2.00.12 - LR)
   writeline(m,s);                                                               //(2.00.12 - LR)
   sprintf(s,FMT27b,m->MaxCheck);                                                 //(2.00.12 - LR)
   writeline(m,s);                                                               //(2.00.12 - LR)
   sprintf(s,FMT27c,m->DampLimit);                                                //(2.00.12 - LR)
   writeline(m,s);                                                               //(2.00.12 - LR)

   sprintf(s,FMT28,m->MaxIter);
   writeline(m,s);
   if (m->Qualflag == NONE || m->Dur == 0.0)
      sprintf(s,FMT29);
   else if (m->Qualflag == CHEM)
      sprintf(s,FMT30,m->ChemName);
   else if (m->Qualflag == TRACE)
      sprintf(s,FMT31,m->Node[m->TraceNode].ID);
   else if (m->Qualflag == AGE)
      sprintf(s,FMT32);
   writeline(m,s);
   if (m->Qualflag != NONE && m->Dur > 0)
   {
      sprintf(s,FMT33,(float)m->Qstep/60.0);
      writeline(m,s);
      sprintf(s,FMT34,m->Ctol*m->Ucf[QUALITY],m->Field[QUALITY].Units);
      writeline(m,s);
   }
   sprintf(s,FMT36,m->SpGrav);
   writeline(m,s);
   sprintf(s,FMT37a,m->Viscos/VISCOS);
   writeline(m,s);
   sprintf(s,FMT37b,m->Diffus/DIFFUS);
   writeline(m,s);
   sprintf(s,FMT38,m->Dmult);
   writeline(m,s);
   sprintf(s,FMT39,m->Dur * m->Ucf[TIME], m->Field[TIME].Units);
   writeline(m,s);
   if (m->Rptflag)
   {
      sprintf(s,FMT40);
      writeline(m,s);
      if (m->Nodeflag == 0)  writeline(m,FMT41);
      if (m->Nodeflag == 1)  writeline(m,FMT42);
      if (m->Nodeflag == 2)  writeline(m,FMT43);
      writelimits(m,DEMAND,QUALITY);
      if (m->Linkflag == 0)  writeline(m,FMT44);
      if (m->Linkflag == 1)  writeline(m,FMT45);
      if (m->Linkflag == 2)  writeline(m,FMT46);
      writelimits(m,DIAM,HEADLOSS);
   }
   writeline(m," ");
}                        /* End of writesummary */


void  writehydstat(OW_Model *m, int iter, double relerr)
/*
**--------------------------------------------------------------
**   Input:   iter   = # iterations to find hydraulic solution        
**            relerr = convergence error in hydraulic solution    
**   Output:  none                                                
**   Purpose: writes hydraulic status report for solution found
**            at current time period to report file       
**--------------------------------------------------------------
*/
{
   int    i,n;
   char   newstat;
   char   s1[MAXLINE+1];

/*** Updated 6/24/02 ***/
   char   atime[13];

  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeHead = m->hydraulics.NodeHead;
  double *Ucf = m->Ucf;
  Snode *Node = m->Node;
  Slink *Link = m->Link;
  Stank *Tank = m->Tank;
  char *LinkStatus = m->hydraulics.LinkStatus;
  int Nlinks = m->Nlinks;
  SField *Field = m->Field;
  char *OldStat = m->hydraulics.OldStat;
  
  
   /* Display system status */
   strcpy(atime,clocktime(m->Atime, m->Htime));
   if (iter > 0)
   {
      if (relerr <= m->Hacc)
         sprintf(s1,FMT58,atime,iter);
      else
         sprintf(s1,FMT59,atime,iter,relerr);
      writeline(m,s1);
   }

   /*
      Display status changes for tanks.
      D[n] is net inflow to tank at node n.
      Old tank status is stored in OldStat[]
      at indexes Nlinks+1 to Nlinks+Ntanks.
   */
   for (i=1; i <= m->Ntanks; i++)
   {
      n = Tank[i].Node;
      if (ABS(NodeDemand[n]) < 0.001) newstat = CLOSED;
      else if (NodeDemand[n] >  0.0)  newstat = FILLING;
      else if (NodeDemand[n] <  0.0)  newstat = EMPTYING;
      else newstat = OldStat[Nlinks+i];
      if (newstat != OldStat[Nlinks+i])
      {
         if (Tank[i].A > 0.0)
            sprintf(s1,FMT50,atime,Node[n].ID,StatTxt[newstat],
               (NodeHead[n]-Node[n].El)*Ucf[HEAD],Field[HEAD].Units);
         else sprintf(s1,FMT51,atime,Node[n].ID,StatTxt[newstat]);
         writeline(m,s1);
         OldStat[Nlinks+i] = newstat;
      }
   }

   /* Display status changes for links */
   for (i=1; i<=Nlinks; i++)
   {
      if (LinkStatus[i] != OldStat[i])
      {
         if (m->Htime == 0)
            sprintf(s1,FMT52,atime,LinkTxt[Link[i].Type],Link[i].ID,
               StatTxt[LinkStatus[i]]);
         else sprintf(s1,FMT53,atime,LinkTxt[Link[i].Type],Link[i].ID,
            StatTxt[OldStat[i]],StatTxt[LinkStatus[i]]);
         writeline(m,s1);
         OldStat[i] = LinkStatus[i];
      }
   }
   writeline(m," ");
}                        /* End of writehydstat */


void  writeenergy(OW_Model *m)
/*
**-------------------------------------------------------------
**   Input:   none                                               
**   Output:  none                                               
**   Purpose: writes energy usage report to report file          
**-------------------------------------------------------------
*/
{
    int    j;
    double csum;
    char   s[MAXLINE+1];
  
  Slink *Link = m->Link;
  Spump *Pump = m->Pump;
  
  
    if (m->Npumps == 0) return;
    writeline(m," ");
    writeheader(m,ENERHDR,0);
    csum = 0.0;
    for (j=1; j <= m->Npumps; j++)
    {
        csum += Pump[j].Energy[5];
        if (LineNum == (long)m->PageSize) writeheader(m,ENERHDR,1);
        sprintf(s,"%-8s  %6.2f %6.2f %9.2f %9.2f %9.2f %9.2f",
          Link[Pump[j].Link].ID,Pump[j].Energy[0],Pump[j].Energy[1],
          Pump[j].Energy[2],Pump[j].Energy[3],Pump[j].Energy[4],
          Pump[j].Energy[5]);
        writeline(m,s);
    }
    fillstr(s,'-',63);
    writeline(m,s);

/*** Updated 6/24/02 ***/
    sprintf(s,FMT74,"",m->Emax*m->Dcost);
    writeline(m,s);
    sprintf(s,FMT75,"",csum+m->Emax*m->Dcost);
/*** End of update ***/

    writeline(m,s);
    writeline(m," ");
}                       /* End of writeenergy */


int  writeresults(OW_Model *mod)
/*
**--------------------------------------------------------------
**   Input:   none                                                
**   Output:  returns error code                                  
**   Purpose: writes simulation results to report file            
**--------------------------------------------------------------
*/
{
   Pfloat *x;                /* Array of pointers to floats */
   int    j,m,n,np,nnv,nlv;
   int    errcode = 0;

   /*
   **-----------------------------------------------------------
   **  NOTE:  The OutFile contains results for 4 node variables       
   **         (demand, head, pressure, & quality) and 8 link
   **         variables (flow, velocity, headloss, quality,
   **         status, setting, reaction rate & friction factor)
   **         at each reporting time.                                         
   **-----------------------------------------------------------
   */

   /* Return if no output file */
   if (mod->OutFile == NULL) return(106);

   /* Return if no nodes or links selected for reporting */
   /* or if no node or link report variables enabled.    */
   if (!mod->Nodeflag && !mod->Linkflag) return(errcode);
   nnv = 0;
   for (j=ELEV; j<=QUALITY; j++) {
     nnv += mod->Field[j].Enabled;
   }
   nlv = 0;
   for (j=LENGTH; j<=FRICTION; j++) {
     nlv += mod->Field[j].Enabled;
   }
   if (nnv == 0 && nlv == 0)
     return(errcode);

   /* Allocate memory for output variables. */
   /* m = larger of # node variables & # link variables */
   /* n = larger of # nodes & # links */
   m = MAX( (QUALITY-DEMAND+1), (FRICTION-FLOW+1) );
   n = MAX( (mod->Nnodes+1), (mod->Nlinks+1));
   x = (Pfloat *) calloc(m, sizeof(Pfloat));
   ERRCODE( MEMCHECK(x) );
   if (errcode) return(errcode);
   for (j=0; j<m; j++)
   {
      x[j] = (REAL4 *) calloc(n, sizeof(REAL4));
      ERRCODE( MEMCHECK(x[j]) );
   }
   if (errcode) return(errcode);

   /* Re-position output file & initialize report time. */
   fseek(mod->OutFile,mod->OutOffset2,SEEK_SET);
   mod->Htime = mod->Rstart;

   /* For each reporting time: */
   for (np=1; np <= mod->Nperiods; np++)
   {

      /* Read in node results & write node table. */
      /* (Remember to offset x[j] by 1 because array is zero-based). */
      for (j = DEMAND; j <= QUALITY; j++)
         fread((x[j-DEMAND])+1,sizeof(REAL4),mod->Nnodes,mod->OutFile);
      if (nnv > 0 && mod->Nodeflag > 0) writenodetable(mod,x);

      /* Read in link results & write link table. */
      for (j=FLOW; j<=FRICTION; j++)
         fread((x[j-FLOW])+1,sizeof(REAL4),mod->Nlinks,mod->OutFile);
      if (nlv > 0 && mod->Linkflag > 0)
        writelinktable(mod,x);
      mod->Htime += mod->Rstep;
   }

   /* Free allocated memory */
   for (j=0; j<m; j++) {
     free(x[j]);
   }
   free(x);
   return(errcode);
}                        /* End of writereport */


void  writenodetable(OW_Model *m, Pfloat *x)
/*
**---------------------------------------------------------------
**   Input:   x = pointer to node results for current time
**   Output:  none                                  
**   Purpose: writes node results for current time to report file            
**---------------------------------------------------------------
*/
{
   int    i,j;
   char   s[MAXLINE+1],s1[16];
   double y[MAXVAR];

  double *Ucf = m->Ucf;
  Snode *Node = m->Node;
  int Njuncs = m->Njuncs;
  SField *Field = m->Field;

  
   /* Write table header */
   writeheader(m,NODEHDR,0);

   /* For each node: */
   for (i=1; i <= m->Nnodes; i++)
   {

      /* Place results for each node variable in y */
      y[ELEV] = Node[i].El*Ucf[ELEV];
      for (j=DEMAND; j<=QUALITY; j++) y[j] = *((x[j-DEMAND])+i);

      /* Check if node gets reported on */
      if ((m->Nodeflag == 1 || Node[i].Rpt) && checklimits(m,y,ELEV,QUALITY))
      {

         /* Check if new page needed */
         if (LineNum == (long)m->PageSize) writeheader(m,NODEHDR,1);

         /* Add node ID and each reported field to string s */
         sprintf(s,"%-15s",Node[i].ID);
         for (j=ELEV; j<=QUALITY; j++)
         {
            if (Field[j].Enabled == TRUE)
            {

/*** Updated 6/24/02 ***/
               if (fabs(y[j]) > 1.e6) sprintf(s1, "%10.2e", y[j]);
               else                   sprintf(s1, "%10.*f", Field[j].Precision, y[j]);
/*** End of update ***/

               strcat(s, s1);
            }
         }

         /* Note if node is a reservoir/tank */
         if (i > Njuncs)
         {
            strcat(s, "  ");
            strcat(s, NodeTxt[getnodetype(m,i)]);
         }

         /* Write results for node */
         writeline(m,s);
      }
   }
   writeline(m," ");
}


void  writelinktable(OW_Model *m, Pfloat *x)
/*
**---------------------------------------------------------------
**   Input:   x = pointer to link results for current time
**   Output:  none                                  
**   Purpose: writes link results for current time to report file            
**---------------------------------------------------------------
*/
{
   int    i,j,k;
   char   s[MAXLINE+1],s1[16];
   double y[MAXVAR];

  double *Ucf = m->Ucf;
  Slink *Link = m->Link;
  int Nlinks = m->Nlinks;
  SField *Field = m->Field;
  
  
   /* Write table header */
   writeheader(m,LINKHDR,0);

   /* For each link: */
   for (i=1; i<=Nlinks; i++)
   {

      /* Place results for each link variable in y */
      y[LENGTH] = Link[i].Len*Ucf[LENGTH];
      y[DIAM] = Link[i].Diam*Ucf[DIAM];
      for (j=FLOW; j<=FRICTION; j++) y[j] = *((x[j-FLOW])+i);

      /* Check if link gets reported on */
      if ((m->Linkflag == 1 || Link[i].Rpt) && checklimits(m,y,DIAM,FRICTION))
      {

         /* Check if new page needed */
         if (LineNum == (long)m->PageSize) writeheader(m,LINKHDR,1);

         /* Add link ID and each reported field to string s */
         sprintf(s,"%-15s",Link[i].ID);
         for (j=LENGTH; j<=FRICTION; j++)
         {
            if (Field[j].Enabled == TRUE)
            {
               if (j == STATUS)
               {
                  if      (y[j] <= CLOSED) k = CLOSED;
                  else if (y[j] == ACTIVE) k = ACTIVE;
                  else                     k = OPEN;
                  sprintf(s1, "%10s", StatTxt[k]);
               }

/*** Updated 6/24/02 ***/
               else
               {
                  if (fabs(y[j]) > 1.e6) sprintf(s1, "%10.2e", y[j]);
                  else                   sprintf(s1, "%10.*f", Field[j].Precision, y[j]);
               }
/*** End of update ***/

               strcat(s, s1);
            }
         }

         /* Note if link is a pump or valve */
         if ( (j = Link[i].Type) > PIPE)
         {
            strcat(s, "  ");
            strcat(s, LinkTxt[j]);
         }

         /* Write results for link */
         writeline(m,s);
      }
   }
   writeline(m," ");
}


void  writeheader(OW_Model *m, int type, int contin)
/*
**--------------------------------------------------------------
**   Input:   type   = table type                                
**            contin = table continuation flag                    
**   Output:  none                                                
**   Purpose: writes column headings for output report tables             
**--------------------------------------------------------------
*/
{
   char   s[MAXLINE+1],s1[MAXLINE+1],s2[MAXLINE+1],s3[MAXLINE+1];
   int    i,n;

   /* Move to next page if < 11 lines remain on current page. */
   if (m->Rptflag && LineNum+11 > (long)m->PageSize)
   {
     while (LineNum < (long)m->PageSize) {
       writeline(m," ");
     }
   }
   writeline(m," ");

   /* Hydraulic Status Table */
   if (type == STATHDR)
   {
      sprintf(s,FMT49);
      if (contin) strcat(s,t_CONTINUED);
      writeline(m,s);
      fillstr(s,'-',70);
      writeline(m,s);
   }

   /* Energy Usage Table */
   if (type == ENERHDR)
   {
      if (m->Unitsflag == SI) strcpy(s1,t_perM3);
      else                 strcpy(s1,t_perMGAL);
      sprintf(s,FMT71);
      if (contin) strcat(s,t_CONTINUED);
      writeline(m,s);
      fillstr(s,'-',63);
      writeline(m,s);
      sprintf(s,FMT72);
      writeline(m,s);
      sprintf(s,FMT73,s1);
      writeline(m,s);
      fillstr(s,'-',63);
      writeline(m,s);
   }

   /* Node Results Table */
   if (type == NODEHDR)
   {
      if      (m->Tstatflag == RANGE)  sprintf(s,FMT76,t_DIFFER);
      else if (m->Tstatflag != SERIES) sprintf(s,FMT76,TstatTxt[m->Tstatflag]);
      else if (m->Dur == 0)            sprintf(s,FMT77);
      else                          sprintf(s,FMT78,clocktime(m->Atime,m->Htime));
      if (contin) strcat(s,t_CONTINUED);
      writeline(m,s);
      n = 15;
      sprintf(s2,"%15s","");
      strcpy(s,t_NODEID);
      sprintf(s3,"%-15s",s);
      for (i=ELEV; i<QUALITY; i++) if (m->Field[i].Enabled == TRUE)
      {
        n += 10;
        sprintf(s,"%10s",m->Field[i].Name);
        strcat(s2,s);
        sprintf(s,"%10s",m->Field[i].Units);
        strcat(s3,s);
      }
      if (m->Field[QUALITY].Enabled == TRUE)
      {
        n += 10;
        sprintf(s,"%10s",m->ChemName);
        strcat(s2,s);
        sprintf(s,"%10s",m->ChemUnits);
        strcat(s3,s);
      }
      fillstr(s1,'-',n);
      writeline(m,s1);
      writeline(m,s2);
      writeline(m,s3);
      writeline(m,s1);
   }

   /* Link Results Table */
   if (type == LINKHDR)
   {
      if      (m->Tstatflag == RANGE)  sprintf(s,FMT79,t_DIFFER);
      else if (m->Tstatflag != SERIES) sprintf(s,FMT79,TstatTxt[m->Tstatflag]);
      else if (m->Dur == 0)            sprintf(s,FMT80);
      else                          sprintf(s,FMT81,clocktime(m->Atime,m->Htime));
      if (contin) strcat(s,t_CONTINUED);
      writeline(m,s);
      n = 15;
      sprintf(s2,"%15s","");
      strcpy(s,t_LINKID);
      sprintf(s3,"%-15s",s);
      for (i=LENGTH; i<=FRICTION; i++) if (m->Field[i].Enabled == TRUE)
      {
        n += 10;
        sprintf(s,"%10s",m->Field[i].Name);
        strcat(s2,s);
        sprintf(s,"%10s",m->Field[i].Units);
        strcat(s3,s);
      }
      fillstr(s1,'-',n);
      writeline(m,s1);
      writeline(m,s2);
      writeline(m,s3);
      writeline(m,s1);
  }
}                        /* End of writeheader */


void  writeline(OW_Model *m, char *s)
/*
**--------------------------------------------------------------
**   Input:   *s = text string                                    
**   Output:  none                                                
**   Purpose: writes a line of output to report file              
**--------------------------------------------------------------
*/
{
   if (m->RptFile == NULL) return;
   if (m->Rptflag)
   {
      if (LineNum == (long)m->PageSize)
      {
         PageNum++;
         if (fprintf(m->RptFile,FMT82,PageNum,m->Title[0]) == EOF)
            Fprinterr = TRUE;
         LineNum = 3;
      }
   }
   if (fprintf(m->RptFile,"\n  %s",s) == EOF) Fprinterr = TRUE;
   LineNum++;
}                        /* End of writeline */


void  writerelerr(OW_Model *m, int iter, double relerr)
/*
**-----------------------------------------------------------------
**   Input:   iter   = current iteration of hydraulic solution    
**            relerr = current convergence error                  
**   Output:  none                                                
**   Purpose: writes out convergence status of hydraulic solution 
**-----------------------------------------------------------------
*/
{
   if (iter == 0)
   {
      sprintf(m->Msg, FMT64, clocktime(m->Atime,m->Htime));
      writeline(m,m->Msg);
   }
   else
   {
      sprintf(m->Msg,FMT65,iter,relerr);
      writeline(m,m->Msg);
   }
}                        /* End of writerelerr */


void  writestatchange(OW_Model *m, int k, char s1, char s2)
/*
**--------------------------------------------------------------
**   Input:   k  = link index                                     
**            s1 = old link status                                
**            s2 = new link status                                
**   Output:  none                                                
**   Purpose: writes change in link status to output report       
**--------------------------------------------------------------
*/
{
   int    j1,j2;
   double setting;
  Slink *Link = m->Link;
  double *LinkSetting = m->hydraulics.LinkSetting;
  double *Ucf = m->Ucf;


/* We have a pump/valve setting change instead of a status change */
   if (s1 == s2)
   {

/*** Updated 10/25/00 ***/
      setting = LinkSetting[k];   //Link[k].Kc;

      switch (Link[k].Type)
      {
         case PRV:
         case PSV:
         case PBV: setting *= Ucf[PRESSURE]; break;
         case FCV: setting *= Ucf[FLOW];
      }
      sprintf(m->Msg,FMT56,LinkTxt[Link[k].Type],Link[k].ID,setting);
      writeline(m,m->Msg);
      return;
   }

/* We have a status change. Write the old & new status types. */
   if      (s1 == ACTIVE) j1 = ACTIVE;
   else if (s1 <= CLOSED) j1 = CLOSED;
   else                   j1 = OPEN;
   if      (s2 == ACTIVE) j2 = ACTIVE;
   else if (s2 <= CLOSED) j2 = CLOSED;
   else                   j2 = OPEN;
   if (j1 != j2)
   {
      sprintf(m->Msg,FMT57,LinkTxt[Link[k].Type],Link[k].ID,
              StatTxt[j1],StatTxt[j2]);
      writeline(m,m->Msg);
   }
}                        /* End of writestatchange */


void writecontrolaction(OW_Model *m, int k, int i)
/*
----------------------------------------------------------------
**   Input:   k  = link index                                     
**            i  = control index                                
**   Output:  none                                                
**   Purpose: writes control action taken to status report
**--------------------------------------------------------------
*/
{
   int n;
  
  Snode *Node = m->Node;
  Slink *Link = m->Link;
  Scontrol *Control = m->Control;
  
   switch (Control[i].Type)
   {
      case LOWLEVEL:
      case HILEVEL:
         n = Control[i].Node;
         sprintf(m->Msg,FMT54,clocktime(m->Atime,m->Htime),LinkTxt[Link[k].Type],
            Link[k].ID,NodeTxt[getnodetype(m,n)],Node[n].ID);
         break;
      case TIMER:
      case TIMEOFDAY:
         sprintf(m->Msg,FMT55,clocktime(m->Atime,m->Htime),LinkTxt[Link[k].Type],
            Link[k].ID);
         break;
      default: return;
   }
   writeline(m,m->Msg);
}


void writeruleaction(OW_Model *m, int k, char *ruleID)
/*
**--------------------------------------------------------------
**   Input:   k  = link index                                     
**            *ruleID  = rule ID
**   Output:  none                                                
**   Purpose: writes rule action taken to status report
**--------------------------------------------------------------
*/
{
   sprintf(m->Msg,FMT63,clocktime(m->Atime,m->Htime),LinkTxt[m->Link[k].Type], m->Link[k].ID,ruleID);
   writeline(m,m->Msg);
}


int  writehydwarn(OW_Model *m, int iter, double relerr)
/*
**--------------------------------------------------------------
**   Input:   iter = # iterations to find hydraulic solution      
**   Output:  warning flag code                                                
**   Purpose: writes hydraulic warning message to report file
**
**   Note: Warning conditions checked in following order:
**         1. System balanced but unstable
**         2. Negative pressures
**         3. FCV cannot supply flow or PRV/PSV cannot maintain pressure
**         4. Pump out of range
**         5. Network disconnected
**         6. System unbalanced
**--------------------------------------------------------------
*/
{
   int  i,j;
   char flag = 0;
   char s;                                                                     //(2.00.11 - LR)

  double *NodeDemand = m->hydraulics.NodeDemand;
  double *NodeHead = m->hydraulics.NodeHead;
  Snode *Node = m->Node;
  Slink *Link = m->Link;
  Spump *Pump = m->Pump;
  Svalve *Valve = m->Valve;
  double *LinkFlows = m->hydraulics.LinkFlows;
  double *LinkSetting = m->hydraulics.LinkSetting;
  char *LinkStatus = m->hydraulics.LinkStatus;
  int Njuncs = m->Njuncs;
  int Npumps = m->Npumps;
  int Nvalves = m->Nvalves;
  
  
   /* Check if system unstable */
   if (iter > m->MaxIter && relerr <= m->Hacc)
   {
      sprintf(m->Msg,WARN02,clocktime(m->Atime,m->Htime));
      if (m->Messageflag) writeline(m,m->Msg);
      flag = 2;
   }

   /* Check for negative pressures */
   for (i=1; i<=Njuncs; i++)
   {
      if (NodeHead[i] < Node[i].El && NodeDemand[i] > 0.0)
      {
         sprintf(m->Msg,WARN06,clocktime(m->Atime,m->Htime));
         if (m->Messageflag)
           writeline(m,m->Msg);
         flag = 6;
         break;
      }
   }

   /* Check for abnormal valve condition */
   for (i=1; i<=Nvalves; i++)
   {
      j = Valve[i].Link;
      if (LinkStatus[j] >= XFCV)
      {
         sprintf(m->Msg,WARN05,LinkTxt[Link[j].Type],Link[j].ID,
            StatTxt[LinkStatus[j]],clocktime(m->Atime,m->Htime));
         if (m->Messageflag) writeline(m,m->Msg);
         flag = 5;
      }
   }

   /* Check for abnormal pump condition */
   for (i=1; i<=Npumps; i++)
   {
      j = Pump[i].Link;
      s = LinkStatus[j];                                                                //(2.00.11 - LR)
      if (LinkStatus[j] >= OPEN)                                                        //(2.00.11 - LR)
      {                                                                        //(2.00.11 - LR)
          if (LinkFlows[j] > LinkSetting[j]*Pump[i].Qmax)
            s = XFLOW;                             //(2.00.11 - LR)
        
          if (LinkFlows[j] < 0.0)
            s = XHEAD;                                           //(2.00.11 - LR)
      }                                                                        //(2.00.11 - LR)
      if (s == XHEAD || s == XFLOW)                                            //(2.00.11 - LR)
      {                                    
         sprintf(m->Msg,WARN04,Link[j].ID,StatTxt[s],                             //(2.00.11 - LR)
                 clocktime(m->Atime,m->Htime));
         if (m->Messageflag) writeline(m,m->Msg);
         flag = 4;
      }
   }

   /* Check if system is unbalanced */
   if (iter > m->MaxIter && relerr > m->Hacc)
   {
      sprintf(m->Msg,WARN01,clocktime(m->Atime,m->Htime));
      if (m->ExtraIter == -1) strcat(m->Msg,t_HALTED);
      if (m->Messageflag) writeline(m,m->Msg);
      flag = 1;
   }

   /* Check for disconnected network */
   /* & update global warning flag   */
   if (flag > 0)
   {
      disconnected(m);
      m->Warnflag = flag;
   }
   return(flag);
}                        /* End of writehydwarn */


void  writehyderr(OW_Model *m, int errnode)
/*
**-----------------------------------------------------------
**   Input:   none                                          
**   Output:  none                                          
**   Purpose: outputs status & checks connectivity when     
**            network hydraulic equations cannot be solved. 
**-----------------------------------------------------------
*/
{
   sprintf(m->Msg,FMT62,clocktime(m->Atime,m->Htime), m->Node[errnode].ID);
   if (m->Messageflag) writeline(m,m->Msg);
   writehydstat(m,0,0);
   disconnected(m);
}                        /* End of writehyderr */


int  disconnected(OW_Model *m)
/*
**-------------------------------------------------------------------
**   Input:   None                                                  
**   Output:  Returns number of disconnected nodes                  
**   Purpose: Tests current hydraulic solution to see if any closed 
**            links have caused the network to become disconnected. 
**-------------------------------------------------------------------
*/
{
   int  i, j;
   int  count, mcount;
   int  errcode = 0;
   int  *nodelist;
   char *marked;
  
  double *NodeDemand = m->hydraulics.NodeDemand;
  Snode *Node = m->Node;
  int Njuncs = m->Njuncs;
  int Ntanks = m->Ntanks;
  int Nnodes = m->Nnodes;
  char Messageflag = m->Messageflag;
  char *Msg = m->Msg;

   /* Allocate memory for node list & marked list */
   nodelist = (int *)  calloc(Nnodes+1,sizeof(int));
   marked   = (char *) calloc(Nnodes+1,sizeof(char));
   ERRCODE(MEMCHECK(nodelist));
   ERRCODE(MEMCHECK(marked));
   if (errcode) return(0);

   /* Place tanks on node list and marked list */
   for (i=1; i<=Ntanks; i++)
   {
      j = Njuncs + i;
      nodelist[i] = j;
      marked[j] = 1;
   }

   /* Place junctions with negative demands on the lists */
   mcount = Ntanks;
   for (i=1; i<=Njuncs; i++)
   {
      if (NodeDemand[i] < 0.0)
      {
         mcount++;
         nodelist[mcount] = i;
         marked[i] = 1;
      }
   }

   /* Mark all nodes that can be connected to tanks */
   /* and count number of nodes remaining unmarked. */
   marknodes(m,mcount,nodelist,marked);
   j = 0;
   count = 0;
   for (i=1; i <= Njuncs; i++)
   {
      if (!marked[i] && NodeDemand[i] != 0.0)  /* Skip if no demand */
      {
         count++;
         if (count <= MAXCOUNT && Messageflag)
         {
            sprintf(Msg,WARN03a,Node[i].ID,clocktime(m->Atime,m->Htime));
            writeline(m,Msg);
         }
         j = i;                       /* Last unmarked node */
      }
   }

   /* Report number of unmarked nodes and find closed link */
   /* on path from node j back to a tank.                  */
   if (count > 0 && Messageflag)
   {
      if (count > MAXCOUNT)
      {
         sprintf(Msg, WARN03b, count-MAXCOUNT, clocktime(m->Atime,m->Htime));
         writeline(m,Msg);
      }
      getclosedlink(m,j,marked);
   }

   /* Free allocated memory */
   free(nodelist);
   free(marked);
   return(count);
}                   /* End of disconnected() */


void  marknodes(OW_Model *mod, int m, int *nodelist, char *marked)
/*
**----------------------------------------------------------------
**   Input:   m = number of source nodes
**            nodelist[] = list of nodes to be traced from          
**            marked[]   = TRUE if node connected to source         
**   Output:  None.                                                 
**   Purpose: Marks all junction nodes connected to tanks.          
**----------------------------------------------------------------
*/
{
   int   i, j, k, n;
   Padjlist alink;

   /* Scan each successive entry of node list */
   n = 1;
   while (n <= m )
   {

      /* Scan all nodes connected to current node */
      i = nodelist[n];
      for (alink = mod->Adjlist[i]; alink != NULL; alink = alink->next)
      {

         /* Get indexes of connecting link and node */
         k = alink->link;
         j = alink->node;
         if (marked[j]) continue;

         /* Check if valve connection is in correct direction */
         switch (mod->Link[k].Type)
         {
            case CV:
            case PRV:
            case PSV: if (j == mod->Link[k].N1) continue;
         }

         /* Mark connection node if link not closed */
         if (mod->hydraulics.LinkStatus[k] > CLOSED)
         {
            marked[j] = 1;
            m++;
            nodelist[m] = j;
         }
      }
      n++;
   }
}                   /* End of marknodes() */


void getclosedlink(OW_Model *m, int i, char *marked)
/*
**----------------------------------------------------------------
**   Input:   i = junction index                                    
**            marked[] = marks nodes already examined               
**   Output:  None.                                                 
**   Purpose: Determines if a closed link connects to junction i.   
**----------------------------------------------------------------
*/
{
   int j,k;
   Padjlist alink;
   marked[i] = 2;
   for (alink = m->Adjlist[i]; alink != NULL; alink = alink->next)
   {
      k = alink->link;
      j = alink->node;
      if (marked[j] == 2) continue;
      if (marked[j] == 1)
      {
         sprintf(m->Msg, WARN03c, m->Link[k].ID);
         writeline(m,m->Msg);
         return;
      }
      else getclosedlink(m,j,marked);
   }
}
      

void  writelimits(OW_Model *m, int j1, int j2)
/*
**--------------------------------------------------------------
**   Input:   j1 = index of first output variable                 
**            j2 = index of last output variable                  
**   Output:  none                                                
**   Purpose: writes reporting criteria to output report          
**--------------------------------------------------------------
*/
{
   int  j;
  
  SField *Field = m->Field;
  char *Msg = m->Msg;
  
   for (j=j1; j<=j2; j++)
   {
      if (Field[j].RptLim[LOW] < BIG)
      {
         sprintf(Msg,FMT47,
                 Field[j].Name,Field[j].RptLim[LOW],Field[j].Units);
         writeline(m,Msg);
      }
      if (Field[j].RptLim[HI] > -BIG)
      {
         sprintf(Msg,FMT48,
                 Field[j].Name,Field[j].RptLim[HI],Field[j].Units);
         writeline(m,Msg);
      }
   }
}                        /* End of writelimits */
   

int  checklimits(OW_Model *m, double *y, int j1, int j2)
/*
**--------------------------------------------------------------
**   Input:   *y = array of output results                        
**            j1 = index of first output variable                 
**            j2 = index of last output variable                  
**   Output:  returns 1 if criteria met, 0 otherwise              
**   Purpose: checks if output reporting criteria is met          
**--------------------------------------------------------------
*/
{
   int j;
   for (j=j1; j<=j2; j++)
   {
      if (y[j] > m->Field[j].RptLim[LOW]
      ||  y[j] < m->Field[j].RptLim[HI]) return(0);
   }
   return(1);
}                        /* End of checklim */


void writetime(OW_Model *m, char *fmt)
/*
**----------------------------------------------------------------
**   Input:   fmt = format string                             
**   Output:  none                             
**   Purpose: writes starting/ending time of a run to report file              
**----------------------------------------------------------------
*/
{
  time_t timer;
  time(&timer);
  sprintf(m->Msg, fmt, ctime(&timer));
  writeline(m,m->Msg);
}


char *clocktime(char *atime, long seconds)
/*
**--------------------------------------------------------------
**   Input:   seconds = time in seconds                             
**   Output:  atime = time in hrs:min                             
**            (returns pointer to atime)                          
**   Purpose: converts time in seconds to hours:minutes format              
**--------------------------------------------------------------
*/
{
/*** Updated 6/24/02 ***/
    int h,m,s;
    h = (int)seconds/3600;
    m = (seconds % 3600) / 60;
    s = (int)seconds - 3600*h - 60*m;
    sprintf(atime, "%01d:%02d:%02d", h,m,s);
    return(atime);
}                        /* End of clocktime */


char *fillstr(char *s, char ch, int n)
/*
**---------------------------------------------------------
**  Fills n bytes of s to character ch.
**  NOTE: does not check for overwriting s. 
**---------------------------------------------------------
*/
{
   int i;
   for (i=0; i<=n; i++) s[i] = ch;
   s[n+1] = '\0';
   return(s);
}


int  getnodetype(OW_Model *m, int i)
/*
**---------------------------------------------------------
**  Determines type of node with index i
**  (junction = 0, reservoir = 1, tank = 2). 
**---------------------------------------------------------
*/
{
   if (i <= m->Njuncs) return(0);
   if (m->Tank[i-m->Njuncs].A == 0.0) return(1);
   return(2);
}

/********************* END OF REPORT.C ********************/
