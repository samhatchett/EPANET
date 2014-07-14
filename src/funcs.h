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

void    initpointers(OW_Model *m);               /* Initializes pointers       */
int     allocdata(OW_Model *m);                  /* Allocates memory           */
void    freeTmplist(STmplist *);          /* Frees items in linked list */
void    freeFloatlist(SFloatlist *);      /* Frees list of floats       */
void    freedata(OW_Model *m);                   /* Frees allocated memory     */
int     openfiles(OW_Model *m, char *,char *,char *);  /* Opens input & report files */
int     openhydfile(OW_Model *m);                /* Opens hydraulics file      */
int     openoutfile(OW_Model *m);                /* Opens binary output file   */
int     strcomp(char *, char *);          /* Compares two strings       */
char*   getTmpName(char* fname);          /* Gets temporary file name   */     //(2.00.12 - LR)
double  interp(int, double *,             /* Interpolates a data curve  */
               double *, double);
int     findnode(OW_Model *m, char *);                 /* Finds node's index from ID */
int     findlink(OW_Model *m, char *);                 /* Finds link's index from ID */
void    geterrmsg(int, char* msgOut);                   /* Gets text of error message */
void    errmsg(OW_Model *m, int);                      /* Reports program error      */
void    writecon(char *);                 /* Writes text to console     */
void    writewin(char *);                 /* Passes text to calling app */

/* ------- INPUT1.C --------------------*/
int     getdata(OW_Model *m);                    /* Gets network data          */
void    setdefaults(OW_Model *m);                /* Sets default values        */
void    initreport(OW_Model *m);                 /* Initializes report options */
void    adjustdata(OW_Model *m);                 /* Adjusts input data         */
int     inittanks(OW_Model *m);                  /* Initializes tank levels    */
void    initunits(OW_Model *m);                  /* Determines reporting units */
void    convertunits(OW_Model *m);               /* Converts data to std. units*/

/* -------- INPUT2.C -------------------*/
int     netsize(OW_Model *m);                    /* Determines network size    */
int     readdata(OW_Model *m);                   /* Reads in network data      */
int     newline(OW_Model *m, int, char *);             /* Processes new line of data */
int     addnodeID(OW_Model *m, int, char *);           /* Adds node ID to data base  */
int     addlinkID(OW_Model *m, int, char *);           /* Adds link ID to data base  */
int     addpattern(OW_Model *m, char *);               /* Adds pattern to data base  */
int     addcurve(OW_Model *m, char *);                 /* Adds curve to data base    */
int     addcoord(OW_Model *m, char *);                 /* Adds coord to data base    */
STmplist *findID(char *, STmplist *);     /* Locates ID on linked list  */
int     unlinked(OW_Model *m);                   /* Checks for unlinked nodes  */
int     getpumpparams(OW_Model *m);              /* Computes pump curve coeffs.*/
int     getpatterns(OW_Model *m);                /* Gets pattern data from list*/
int     getcurves(OW_Model *m);                  /* Gets curve data from list  */
int     getcoords(OW_Model *m);                  /* Gets coordinate data from list  */
int     findmatch(char *,char *[]);       /* Finds keyword in line      */
int     match(char *, char *);            /* Checks for word match      */
int     gettokens(char *s, char** Tok, int maxToks);                /* Tokenizes input line       */
int     getfloat(char *, double *);       /* Converts string to double   */
double  hour(char *, char *);             /* Converts time to hours     */
int     setreport(OW_Model *m, char *);                /* Processes reporting command*/
void    inperrmsg(OW_Model *m, int,int,char *);        /* Input error message        */

/* ---------- INPUT3.C -----------------*/
int     juncdata(OW_Model *m);                   /* Processes junction data    */
int     tankdata(OW_Model *m);                   /* Processes tank data        */
int     pipedata(OW_Model *m);                   /* Processes pipe data        */
int     pumpdata(OW_Model *m);                   /* Processes pump data        */
int     valvedata(OW_Model *m);                  /* Processes valve data       */
int     patterndata(OW_Model *m);                /* Processes pattern data     */
int     curvedata(OW_Model *m);                  /* Processes curve data       */
int     coordata(OW_Model *m);                   /* Processes coordinate data       */
int     demanddata(OW_Model *m);                 /* Processes demand data      */
int     controldata(OW_Model *m);                /* Processes simple controls  */
int     energydata(OW_Model *m);                 /* Processes energy data      */
int     sourcedata(OW_Model *m);                 /* Processes source data      */
int     emitterdata(OW_Model *m);                /* Processes emitter data     */
int     qualdata(OW_Model *m);                   /* Processes quality data     */
int     reactdata(OW_Model *m);                  /* Processes reaction data    */
int     mixingdata(OW_Model *m);                 /* Processes tank mixing data */
int     statusdata(OW_Model *m);                 /* Processes link status data */
int     reportdata(OW_Model *m);                 /* Processes report options   */
int     timedata(OW_Model *m);                   /* Processes time options     */
int     optiondata(OW_Model *m);                 /* Processes analysis options */
int     optionchoice(OW_Model *m, int);                /* Processes option choices   */
int     optionvalue(OW_Model *m, int);                 /* Processes option values    */
int     getpumpcurve(OW_Model *m, int);                /* Constructs a pump curve    */
int     powercurve(double, double, double,/* Coeffs. of power pump curve*/
                   double, double, double *,
                   double *, double *);
int     valvecheck(OW_Model *m, int, int, int);        /* Checks valve placement     */
void    changestatus(OW_Model *m, int, char, double);  /* Changes status of a link   */

/* -------------- RULES.C --------------*/
void    initrules(OW_Model *m);                  /* Initializes rule base      */
void    addrule(OW_Model *m, char *);                  /* Adds rule to rule base     */
int     allocrules(OW_Model *m);                 /* Allocates memory for rule  */
int     ruledata(OW_Model *m);                   /* Processes rule input data  */
int     checkrules(OW_Model *m, long);                 /* Checks all rules           */
void    freerules(OW_Model *m);                  /* Frees rule base memory     */

/* ------------- REPORT.C --------------*/
int     writereport(OW_Model *m);                /* Writes formatted report    */
void    writelogo(OW_Model *m);                  /* Writes program logo        */
void    writesummary(OW_Model *m);               /* Writes network summary     */
void    writehydstat(OW_Model *m,int,double);          /* Writes hydraulic status    */
void    writeenergy(OW_Model *m);                /* Writes energy usage        */
int     writeresults(OW_Model *m);               /* Writes node/link results   */
void    writeheader(OW_Model *m, int,int);             /* Writes heading on report   */
void    writeline(OW_Model *m, char *);                /* Writes line to report file */
void    writerelerr(OW_Model *m, int, double);          /* Writes convergence error   */
void    writestatchange(OW_Model *m, int,char,char);   /* Writes link status change  */
void    writecontrolaction(OW_Model *m, int, int);     /* Writes control action taken*/
void    writeruleaction(OW_Model *m, int, char *);     /* Writes rule action taken   */
int     writehydwarn(OW_Model *m, int,double);          /* Writes hydraulic warnings  */
void    writehyderr(OW_Model *m, int);                 /* Writes hydraulic error msg.*/
int     disconnected(OW_Model *m);               /* Checks for disconnections  */
void    marknodes(OW_Model *m, int, int *, char *);    /* Identifies connected nodes */
void    getclosedlink(OW_Model *m, int, char *);       /* Finds a disconnecting link */
void    writelimits(OW_Model *m, int,int);             /* Writes reporting limits    */
int     checklimits(OW_Model *m, double *,int,int);     /* Checks variable limits     */
void    writetime(OW_Model *m, char *);                /* Writes current clock time  */
char    *clocktime(char *, long);         /* Converts time to hrs:min   */
char    *fillstr(char *, char, int);      /* Fills string with character*/
int     getnodetype(OW_Model *m, int);                 /* Determines node type       */

/* --------- HYDRAUL.C -----------------*/
int     openhyd(OW_Model *m);                    /* Opens hydraulics solver    */

/*** Updated 3/1/01 ***/
void    inithyd(OW_Model *m, int);                     /* Re-sets initial conditions */

int     runhyd(OW_Model *m, long *);                   /* Solves 1-period hydraulics */
int     nexthyd(OW_Model *m, long *);                  /* Moves to next time period  */
void    closehyd(OW_Model *m);                   /* Closes hydraulics solver   */
int     allocmatrix(OW_Model *m);                /* Allocates matrix coeffs.   */
void    freematrix(OW_Model *m);                 /* Frees matrix coeffs.       */
void    initlinkflow(OW_Model *m, int, char, double);  /* Initializes link flow      */
void    setlinkflow(OW_Model *m, int, double);         /* Sets link flow via headloss*/
void    setlinkstatus(OW_Model *m, int, char, char *,  /* Sets link status           */
                      double *);
void    setlinksetting(OW_Model *m, int, double,       /* Sets pump/valve setting    */
                       char *, double *);
void    resistance(OW_Model *m, int);                  /* Computes resistance coeff. */
void    demands(OW_Model *m);                    /* Computes current demands   */
int     controls(OW_Model *m);                   /* Controls link settings     */
long    timestep(OW_Model *m);                   /* Computes new time step     */
void    tanktimestep(OW_Model *m, long *);             /* Time till tanks fill/drain */
void    controltimestep(OW_Model *m, long *);          /* Time till control action   */
void    ruletimestep(OW_Model *m, long *);             /* Time till rule action      */
void    addenergy(OW_Model *m, long);                  /* Accumulates energy usage   */
void    getenergy(OW_Model *m, int, double *, double *); /* Computes link energy use   */
void    tanklevels(OW_Model *m, long);                 /* Computes new tank levels   */
double  tankvolume(OW_Model *m, int,double);           /* Finds tank vol. from grade */
double  tankgrade(OW_Model *m, int,double);            /* Finds tank grade from vol. */
int     netsolve(OW_Model *m, int *,double *);         /* Solves network equations   */
int     badvalve(OW_Model *m, int);                    /* Checks for bad valve       */
int     valvestatus(OW_Model *m);                /* Updates valve status       */
int     linkstatus(OW_Model *m);                 /* Updates link status        */
char    cvstatus(OW_Model *m, char,double,double);     /* Updates CV status          */
char    pumpstatus(OW_Model *m, int,double);           /* Updates pump status        */
char    prvstatus(OW_Model *m, int,char,double,        /* Updates PRV status         */
                  double,double);
char    psvstatus(OW_Model *m, int,char,double,        /* Updates PSV status         */
                  double,double);
char    fcvstatus(OW_Model *m, int,char,double,        /* Updates FCV status         */
                  double);
void    tankstatus(OW_Model *m, int,int,int);          /* Checks if tank full/empty  */
int     pswitch(OW_Model *m);                    /* Pressure switch controls   */
double  newflows(OW_Model *m);                   /* Updates link flows         */
void    newcoeffs(OW_Model *m);                  /* Computes matrix coeffs.    */
void    linkcoeffs(OW_Model *m);                 /* Computes link coeffs.      */
void    nodecoeffs(OW_Model *m);                 /* Computes node coeffs.      */
void    valvecoeffs(OW_Model *m);                /* Computes valve coeffs.     */
void    pipecoeff(OW_Model *m, int k);                   /* Computes pipe coeff.       */
double  DWcoeff(OW_Model *m, int, double *);           /* Computes D-W coeff.        */
void    pumpcoeff(OW_Model *m, int);                   /* Computes pump coeff.       */

/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void    curvecoeff(OW_Model *m, int,double,double *,   /* Computes curve coeffs.     */
                             double *);

void    gpvcoeff(OW_Model *m, int);                    /* Computes GPV coeff.        */
void    pbvcoeff(OW_Model *m, int);                    /* Computes PBV coeff.        */
void    tcvcoeff(OW_Model *m, int);                    /* Computes TCV coeff.        */
void    prvcoeff(OW_Model *m, int,int,int);            /* Computes PRV coeff.        */
void    psvcoeff(OW_Model *m, int,int,int);            /* Computes PSV coeff.        */
void    fcvcoeff(OW_Model *m, int,int,int);            /* Computes FCV coeff.        */
void    emittercoeffs(OW_Model *m);              /* Computes emitter coeffs.   */
double  emitflowchange(OW_Model *m, int i);              /* Computes new emitter flow  */

/* ----------- SMATRIX.C ---------------*/
int     createsparse(OW_Model *m);               /* Creates sparse matrix      */
int     allocsparse(OW_Model *m);                /* Allocates matrix memory    */
void    freesparse(OW_Model *m);                 /* Frees matrix memory        */
int     buildlists(OW_Model *m, int);                  /* Builds adjacency lists     */
int     paralink(OW_Model *m, int, int, int);          /* Checks for parallel links  */
void    xparalinks(OW_Model *m);                 /* Removes parallel links     */
void    freelists(OW_Model *m);                  /* Frees adjacency lists      */
void    countdegree(OW_Model *m);                /* Counts links at each node  */
int     reordernodes(OW_Model *m);               /* Finds a node re-ordering   */
int     mindegree(OW_Model *m, int, int);              /* Finds min. degree node     */
int     growlist(OW_Model *m, int);                    /* Augments adjacency list    */
int     newlink(OW_Model *m, Padjlist);                /* Adds fill-ins for a node   */
int     linked(OW_Model *m, int, int);                 /* Checks if 2 nodes linked   */
int     addlink(OW_Model *m, int, int, int);           /* Creates new fill-in        */
int     storesparse(OW_Model *m, int);                 /* Stores sparse matrix       */
int     ordersparse(OW_Model *m, int);                 /* Orders matrix storage      */
void    transpose(int,int *,int *,        /* Transposes sparse matrix   */
        int *,int *,int *,int *,int *);
int     linsolve(OW_Model *m, int, double *, double *, /* Solution of linear eqns.   */
                 double *);               /* via Cholesky factorization */

/* ----------- QUALITY.C ---------------*/
int     openqual(OW_Model *m);                   /* Opens WQ solver system     */
void    initqual(OW_Model *m);                   /* Initializes WQ solver      */
int     runqual(OW_Model *m, long *);                  /* Gets current WQ results    */
int     nextqual(OW_Model *m, long *);                 /* Updates WQ by hyd.timestep */
int     stepqual(OW_Model *m, long *);                 /* Updates WQ by WQ time step */
int     closequal(OW_Model *m);                  /* Closes WQ solver system    */
int     gethyd(OW_Model *m, long *, long *);           /* Gets next hyd. results     */
char    setReactflag(OW_Model *m);               /* Checks for reactive chem.  */
void    transport(OW_Model *m, long);                  /* Transports mass in network */
void    initsegs(OW_Model *m);                   /* Initializes WQ segments    */
void    reorientsegs(OW_Model *m);               /* Re-orients WQ segments     */
void    updatesegs(OW_Model *m, long);                 /* Updates quality in segments*/
void    removesegs(OW_Model *m, int);                  /* Removes a WQ segment       */
void    addseg(OW_Model *m, int,double,double);        /* Adds a WQ segment to pipe  */
void    accumulate(OW_Model *m, long);                 /* Sums mass flow into node   */
void    updatenodes(OW_Model *m, long);                /* Updates WQ at nodes        */
void    sourceinput(OW_Model *m, long);                /* Computes source inputs     */
void    release(OW_Model *m, long);                    /* Releases mass from nodes   */
void    updatetanks(OW_Model *m, long);                /* Updates WQ in tanks        */
void    updatesourcenodes(OW_Model *m, long);          /* Updates WQ at source nodes */
void    tankmix1(OW_Model *m, int, long);              /* Complete mix tank model    */
void    tankmix2(OW_Model *m, int, long);              /* 2-compartment tank model   */
void    tankmix3(OW_Model *m, int, long);              /* FIFO tank model            */
void    tankmix4(OW_Model *m, int, long);              /* LIFO tank model            */
double  sourcequal(OW_Model *m, Psource);              /* Finds WQ input from source */
double  avgqual(OW_Model *m, int);                     /* Finds avg. quality in pipe */
void    ratecoeffs(OW_Model *m);                 /* Finds wall react. coeffs.  */
double  piperate(OW_Model *m, int);                    /* Finds wall react. coeff.   */
double  pipereact(OW_Model *m, int,double,double,long);/* Reacts water in a pipe     */
double  tankreact(OW_Model *m, double,double,double,
                  long);                  /* Reacts water in a tank     */
double  bulkrate(OW_Model *m, double,double,double);   /* Finds bulk reaction rate   */
double  wallrate(OW_Model *m, double,double,double,double);/* Finds wall reaction rate   */


/* ------------ OUTPUT.C ---------------*/
int     savenetdata(OW_Model *m);                /* Saves basic data to file   */
int     savehyd(OW_Model *m, long *);                  /* Saves hydraulic solution   */
int     savehydstep(OW_Model *m, long *);              /* Saves hydraulic timestep   */
int     saveenergy(OW_Model *mvoid);                 /* Saves energy usage         */
int     readhyd(OW_Model *m, long *);                  /* Reads hydraulics from file */
int     readhydstep(OW_Model *m, long *);              /* Reads time step from file  */
int     saveoutput(OW_Model *m);                 /* Saves results to file      */
int     nodeoutput(OW_Model *m, int, REAL4 *, double); /* Saves node results to file */
int     linkoutput(OW_Model *m, int, REAL4 *, double); /* Saves link results to file */
int     savefinaloutput(OW_Model *m);            /* Finishes saving output     */
int     savetimestat(OW_Model *m, REAL4 *, char);      /* Saves time stats to file   */
int     savenetreacts(OW_Model *m, double, double,
                      double, double);    /* Saves react. rates to file */
int     saveepilog(OW_Model *m);                 /* Saves output file epilog   */


/* ------------ INPFILE.C --------------*/
int     saveinpfile(OW_Model *m, char *);              /* Saves network to text file  */

#endif
