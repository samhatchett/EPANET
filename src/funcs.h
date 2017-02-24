/*
**************************************************************************
                                                                   
FUNCS.H -- Function Prototypes for EPANET Program                       
                                                                   
VERSION:    2.00
DATE:       5/8/00
            9/25/00
            10/25/00
            12/29/00
            3/1/01
            2/14/08    (2.00.12)
AUTHOR:     L. Rossman
            US EPA - NRMRL
                                                                                
**************************************************************************
*/

/*****************************************************************/
/*   Most float arguments have been changed to double - 7/3/07   */
/*****************************************************************/

/* ------- EPANET.C --------------------*/
/*
**  NOTE: The exportable functions that can be called
**        via the DLL are prototyped in TOOLKIT.H.
*/

#ifndef FUNCS_H
#define FUNCS_H

void    initpointers(EN_Project *m);               /* Initializes pointers       */
int     allocdata(EN_Project *m);                  /* Allocates memory           */
void    freeTmplist(STmplist *);          /* Frees items in linked list */
void    freeFloatlist(SFloatlist *);      /* Frees list of floats       */
void    freedata(EN_Project *m);                   /* Frees allocated memory     */
int     openfiles(EN_Project *m, char *,char *,char *);  /* Opens input & report files */
int     openhydfile(EN_Project *m);                /* Opens hydraulics file      */
int     openoutfile(EN_Project *m);                /* Opens binary output file   */
int     strcomp(char *, char *);          /* Compares two strings       */
char*   getTmpName(char* fname);          /* Gets temporary file name   */     //(2.00.12 - LR)
double  interp(int, double *,             /* Interpolates a data curve  */
               double *, double);
int     findnode(EN_Project *m, char *);                 /* Finds node's index from ID */
int     findlink(EN_Project *m, char *);                 /* Finds link's index from ID */
void    geterrmsg(int, char* msgOut);                   /* Gets text of error message */
void    errmsg(EN_Project *m, int);                      /* Reports program error      */
void    writecon(char *);                 /* Writes text to console     */
void    writewin(char *);                 /* Passes text to calling app */

/* ------- INPUT1.C --------------------*/
int     getdata(EN_Project *m);                    /* Gets network data          */
void    setdefaults(EN_Project *m);                /* Sets default values        */
void    initreport(EN_Project *m);                 /* Initializes report options */
void    adjustdata(EN_Project *m);                 /* Adjusts input data         */
int     inittanks(EN_Project *m);                  /* Initializes tank levels    */
void    initunits(EN_Project *m);                  /* Determines reporting units */
void    convertunits(EN_Project *m);               /* Converts data to std. units*/

/* -------- INPUT2.C -------------------*/
int     netsize(EN_Project *m);                    /* Determines network size    */
int     readdata(EN_Project *m);                   /* Reads in network data      */
int     newline(EN_Project *m, int, char *);             /* Processes new line of data */
int     addnodeID(EN_Project *m, int, char *);           /* Adds node ID to data base  */
int     addlinkID(EN_Project *m, int, char *);           /* Adds link ID to data base  */
int     addpattern(EN_Project *m, char *);               /* Adds pattern to data base  */
int     addcurve(EN_Project *m, char *);                 /* Adds curve to data base    */
int     addcoord(EN_Project *m, char *);                 /* Adds coord to data base    */
STmplist *findID(char *, STmplist *);     /* Locates ID on linked list  */
int     unlinked(EN_Project *m);                   /* Checks for unlinked nodes  */
int     getpumpparams(EN_Project *m);              /* Computes pump curve coeffs.*/
int     getpatterns(EN_Project *m);                /* Gets pattern data from list*/
int     getcurves(EN_Project *m);                  /* Gets curve data from list  */
int     getcoords(EN_Project *m);                  /* Gets coordinate data from list  */
int     findmatch(char *,char *[]);       /* Finds keyword in line      */
int     match(char *, char *);            /* Checks for word match      */
int     gettokens(char *s, char** Tok, int maxToks, char *comment);                /* Tokenizes input line       */
int     getfloat(char *, double *);       /* Converts string to double   */
double  hour(char *, char *);             /* Converts time to hours     */
int     setreport(EN_Project *m, char *);                /* Processes reporting command*/
void    inperrmsg(EN_Project *m, int,int,char *);        /* Input error message        */

/* ---------- INPUT3.C -----------------*/
int     juncdata(EN_Project *m);                   /* Processes junction data    */
int     tankdata(EN_Project *m);                   /* Processes tank data        */
int     pipedata(EN_Project *m);                   /* Processes pipe data        */
int     pumpdata(EN_Project *m);                   /* Processes pump data        */
int     valvedata(EN_Project *m);                  /* Processes valve data       */
int     patterndata(EN_Project *m);                /* Processes pattern data     */
int     curvedata(EN_Project *m);                  /* Processes curve data       */
int     coordata(EN_Project *m);                   /* Processes coordinate data       */
int     demanddata(EN_Project *m);                 /* Processes demand data      */
int     controldata(EN_Project *m);                /* Processes simple controls  */
int     energydata(EN_Project *m);                 /* Processes energy data      */
int     sourcedata(EN_Project *m);                 /* Processes source data      */
int     emitterdata(EN_Project *m);                /* Processes emitter data     */
int     qualdata(EN_Project *m);                   /* Processes quality data     */
int     reactdata(EN_Project *m);                  /* Processes reaction data    */
int     mixingdata(EN_Project *m);                 /* Processes tank mixing data */
int     statusdata(EN_Project *m);                 /* Processes link status data */
int     reportdata(EN_Project *m);                 /* Processes report options   */
int     timedata(EN_Project *m);                   /* Processes time options     */
int     optiondata(EN_Project *m);                 /* Processes analysis options */
int     optionchoice(EN_Project *m, int);                /* Processes option choices   */
int     optionvalue(EN_Project *m, int);                 /* Processes option values    */
int     getpumpcurve(EN_Project *m, int);                /* Constructs a pump curve    */
int     powercurve(double, double, double,/* Coeffs. of power pump curve*/
                   double, double, double *,
                   double *, double *);
int     valvecheck(EN_Project *m, int, int, int);        /* Checks valve placement     */
void    changestatus(EN_Project *m, int, char, double);  /* Changes status of a link   */

/* -------------- RULES.C --------------*/
void    initrules(EN_Project *m);                  /* Initializes rule base      */
void    addrule(EN_Project *m, char *);                  /* Adds rule to rule base     */
int     allocrules(EN_Project *m);                 /* Allocates memory for rule  */
int     parseRuleData(EN_Project *m);                   /* Processes rule input data  */
int     checkrules(EN_Project *m, long);                 /* Checks all rules           */
void    freerules(EN_Project *m);                  /* Frees rule base memory     */

/* ------------- REPORT.C --------------*/
int     writereport(EN_Project *m);                /* Writes formatted report    */
void    writelogo(EN_Project *m);                  /* Writes program logo        */
void    writesummary(EN_Project *m);               /* Writes network summary     */
void    writehydstat(EN_Project *m,int,double);          /* Writes hydraulic status    */
void    writeenergy(EN_Project *m);                /* Writes energy usage        */
int     writeresults(EN_Project *m);               /* Writes node/link results   */
void    writeheader(EN_Project *m, int,int);             /* Writes heading on report   */
void    writeline(EN_Project *m, char *);                /* Writes line to report file */
void    writeReportFileLine(EN_Project *m, char *s);
void    writerelerr(EN_Project *m, int, double);          /* Writes convergence error   */
void    writestatchange(EN_Project *m, int,char,char);   /* Writes link status change  */
void    writecontrolaction(EN_Project *m, int, int);     /* Writes control action taken*/
void    writeruleaction(EN_Project *m, int, char *);     /* Writes rule action taken   */
int     writehydwarn(EN_Project *m, int,double);          /* Writes hydraulic warnings  */
void    writehyderr(EN_Project *m, int);                 /* Writes hydraulic error msg.*/
int     disconnected(EN_Project *m);               /* Checks for disconnections  */
void    marknodes(EN_Project *m, int, int *, char *);    /* Identifies connected nodes */
void    getclosedlink(EN_Project *m, int, char *);       /* Finds a disconnecting link */
void    writelimits(EN_Project *m, int,int);             /* Writes reporting limits    */
int     checklimits(EN_Project *m, double *,int,int);     /* Checks variable limits     */
void    writetime(EN_Project *m, char *);                /* Writes current clock time  */
char    *clocktime(char *, long);         /* Converts time to hrs:min   */
char    *fillstr(char *, char, int);      /* Fills string with character*/
int     getnodetype(EN_Project *m, int);                 /* Determines node type       */

/* --------- HYDRAUL.C -----------------*/
int     openhyd(EN_Project *m);                    /* Opens hydraulics solver    */

/*** Updated 3/1/01 ***/
void    inithyd(EN_Project *m, int);                     /* Re-sets initial conditions */

int     runhyd(EN_Project *m, long *);                   /* Solves 1-period hydraulics */
int     nexthyd(EN_Project *m, long *);                  /* Moves to next time period  */
void    closehyd(EN_Project *m);                   /* Closes hydraulics solver   */
int     allocmatrix(EN_Project *m);                /* Allocates matrix coeffs.   */
void    freematrix(EN_Project *m);                 /* Frees matrix coeffs.       */
void    initlinkflow(EN_Project *m, int, char, double);  /* Initializes link flow      */
void    setlinkflow(EN_Project *m, int, double);         /* Sets link flow via headloss*/
void    setlinkstatus(EN_Project *m, int, char, char *,  /* Sets link status           */
                      double *);
void    setlinksetting(EN_Project *m, int, double,       /* Sets pump/valve setting    */
                       char *, double *);
void    resistance(EN_Project *m, int);                  /* Computes resistance coeff. */
void    demands(EN_Project *m);                    /* Computes current demands   */
int     controls(EN_Project *m);                   /* Controls link settings     */
long    timestep(EN_Project *m);                   /* Computes new time step     */
int     tanktimestep(EN_Project *m, long *);             /* Time till tanks fill/drain */
int     controltimestep(EN_Project *m, long *);          /* Time till control action   */
void    ruletimestep(EN_Project *m, long *);             /* Time till rule action      */
void    addenergy(EN_Project *m, long);                  /* Accumulates energy usage   */
void    getenergy(EN_Project *m, int, double *, double *); /* Computes link energy use   */
void    tanklevels(EN_Project *m, long);                 /* Computes new tank levels   */
double  tankvolume(EN_Project *m, int,double);           /* Finds tank vol. from grade */
double  tankgrade(EN_Project *m, int,double);            /* Finds tank grade from vol. */
int     netsolve(EN_Project *m, int *,double *);         /* Solves network equations   */
int     badvalve(EN_Project *m, int);                    /* Checks for bad valve       */
int     valvestatus(EN_Project *m);                /* Updates valve status       */
int     linkstatus(EN_Project *m);                 /* Updates link status        */
char    cvstatus(EN_Project *m, char,double,double);     /* Updates CV status          */
char    pumpstatus(EN_Project *m, int,double);           /* Updates pump status        */
char    prvstatus(EN_Project *m, int,char,double,        /* Updates PRV status         */
                  double,double);
char    psvstatus(EN_Project *m, int,char,double,        /* Updates PSV status         */
                  double,double);
char    fcvstatus(EN_Project *m, int,char,double,        /* Updates FCV status         */
                  double);
void    tankstatus(EN_Project *m, int,int,int);          /* Checks if tank full/empty  */
int     pswitch(EN_Project *m);                    /* Pressure switch controls   */
double  newflows(EN_Project *m);                   /* Updates link flows         */
void    newcoeffs(EN_Project *m);                  /* Computes matrix coeffs.    */
void    linkcoeffs(EN_Project *m);                 /* Computes link coeffs.      */
void    nodecoeffs(EN_Project *m);                 /* Computes node coeffs.      */
void    valvecoeffs(EN_Project *m);                /* Computes valve coeffs.     */
void    pipecoeff(EN_Project *m, int k);                   /* Computes pipe coeff.       */
double  DWcoeff(EN_Project *m, int, double *);           /* Computes D-W coeff.        */
void    pumpcoeff(EN_Project *m, int);                   /* Computes pump coeff.       */

/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void    curvecoeff(EN_Project *m, int,double,double *,   /* Computes curve coeffs.     */
                             double *);

void    gpvcoeff(EN_Project *m, int);                    /* Computes GPV coeff.        */
void    pbvcoeff(EN_Project *m, int);                    /* Computes PBV coeff.        */
void    tcvcoeff(EN_Project *m, int);                    /* Computes TCV coeff.        */
void    prvcoeff(EN_Project *m, int,int,int);            /* Computes PRV coeff.        */
void    psvcoeff(EN_Project *m, int,int,int);            /* Computes PSV coeff.        */
void    fcvcoeff(EN_Project *m, int,int,int);            /* Computes FCV coeff.        */
void    emittercoeffs(EN_Project *m);              /* Computes emitter coeffs.   */
double  emitflowchange(EN_Project *m, int i);              /* Computes new emitter flow  */

/* ----------- SMATRIX.C ---------------*/
int     createsparse(EN_Project *m);               /* Creates sparse matrix      */
int     allocsparse(EN_Project *m);                /* Allocates matrix memory    */
void    freesparse(EN_Project *m);                 /* Frees matrix memory        */
int     buildlists(EN_Project *m, int);                  /* Builds adjacency lists     */
int     paralink(EN_Project *m, int, int, int);          /* Checks for parallel links  */
void    xparalinks(EN_Project *m);                 /* Removes parallel links     */
void    freelists(EN_Project *m);                  /* Frees adjacency lists      */
void    countdegree(EN_Project *m);                /* Counts links at each node  */
int     reordernodes(EN_Project *m);               /* Finds a node re-ordering   */
int     mindegree(EN_Project *m, int, int);              /* Finds min. degree node     */
int     growlist(EN_Project *m, int);                    /* Augments adjacency list    */
int     newlink(EN_Project *m, Padjlist);                /* Adds fill-ins for a node   */
int     linked(EN_Project *m, int, int);                 /* Checks if 2 nodes linked   */
int     addlink(EN_Project *m, int, int, int);           /* Creates new fill-in        */
int     storesparse(EN_Project *m, int);                 /* Stores sparse matrix       */
int     ordersparse(EN_Project *m, int);                 /* Orders matrix storage      */
void    transpose(int,int *,int *,        /* Transposes sparse matrix   */
        int *,int *,int *,int *,int *);
int     linsolve(EN_Project *m, int, double *, double *, /* Solution of linear eqns.   */
                 double *);               /* via Cholesky factorization */

/* ----------- QUALITY.C ---------------*/
int     openqual(EN_Project *m);                   /* Opens WQ solver system     */
void    initqual(EN_Project *m);                   /* Initializes WQ solver      */
int     runqual(EN_Project *m, long *);                  /* Gets current WQ results    */
int     nextqual(EN_Project *m, long *);                 /* Updates WQ by hyd.timestep */
int     stepqual(EN_Project *m, long *);                 /* Updates WQ by WQ time step */
int     closequal(EN_Project *m);                  /* Closes WQ solver system    */
int     gethyd(EN_Project *m, long *, long *);           /* Gets next hyd. results     */
char    setReactflag(EN_Project *m);               /* Checks for reactive chem.  */
void    transport(EN_Project *m, long);                  /* Transports mass in network */
void    initsegs(EN_Project *m);                   /* Initializes WQ segments    */
void    reorientsegs(EN_Project *m);               /* Re-orients WQ segments     */
void    updatesegs(EN_Project *m, long);                 /* Updates quality in segments*/
void    removesegs(EN_Project *m, int);                  /* Removes a WQ segment       */
void    addseg(EN_Project *m, int,double,double);        /* Adds a WQ segment to pipe  */
void    accumulate(EN_Project *m, long);                 /* Sums mass flow into node   */
void    updatenodes(EN_Project *m, long);                /* Updates WQ at nodes        */
void    sourceinput(EN_Project *m, long);                /* Computes source inputs     */
void    release(EN_Project *m, long);                    /* Releases mass from nodes   */
void    updatetanks(EN_Project *m, long);                /* Updates WQ in tanks        */
void    updatesourcenodes(EN_Project *m, long);          /* Updates WQ at source nodes */
void    tankmix1(EN_Project *m, int, long);              /* Complete mix tank model    */
void    tankmix2(EN_Project *m, int, long);              /* 2-compartment tank model   */
void    tankmix3(EN_Project *m, int, long);              /* FIFO tank model            */
void    tankmix4(EN_Project *m, int, long);              /* LIFO tank model            */
double  sourcequal(EN_Project *m, Psource);              /* Finds WQ input from source */
double  avgqual(EN_Project *m, int);                     /* Finds avg. quality in pipe */
void    ratecoeffs(EN_Project *m);                 /* Finds wall react. coeffs.  */
double  piperate(EN_Project *m, int);                    /* Finds wall react. coeff.   */
double  pipereact(EN_Project *m, int,double,double,long);/* Reacts water in a pipe     */
double  tankreact(EN_Project *m, double,double,double,
                  long);                  /* Reacts water in a tank     */
double  bulkrate(EN_Project *m, double,double,double);   /* Finds bulk reaction rate   */
double  wallrate(EN_Project *m, double,double,double,double);/* Finds wall reaction rate   */


/* ------------ OUTPUT.C ---------------*/
int     savenetdata(EN_Project *m);                /* Saves basic data to file   */
int     savehyd(EN_Project *m, long *);                  /* Saves hydraulic solution   */
int     savehydstep(EN_Project *m, long *);              /* Saves hydraulic timestep   */
int     saveenergy(EN_Project *mvoid);                 /* Saves energy usage         */
int     readhyd(EN_Project *m, long *);                  /* Reads hydraulics from file */
int     readhydstep(EN_Project *m, long *);              /* Reads time step from file  */
int     saveoutput(EN_Project *m);                 /* Saves results to file      */
int     nodeoutput(EN_Project *m, int, REAL4 *, double); /* Saves node results to file */
int     linkoutput(EN_Project *m, int, REAL4 *, double); /* Saves link results to file */
int     savefinaloutput(EN_Project *m);            /* Finishes saving output     */
int     savetimestat(EN_Project *m, REAL4 *, char);      /* Saves time stats to file   */
int     savenetreacts(EN_Project *m, double, double,
                      double, double);    /* Saves react. rates to file */
int     saveepilog(EN_Project *m);                 /* Saves output file epilog   */


/* ------------ INPFILE.C --------------*/
int     saveinpfile(EN_Project *m, char *);              /* Saves network to text file  */

#endif
