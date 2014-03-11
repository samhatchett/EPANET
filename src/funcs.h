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

void    initpointers(Model *m);               /* Initializes pointers       */
int     allocdata(Model *m);                  /* Allocates memory           */
void    freeTmplist(STmplist *);          /* Frees items in linked list */
void    freeFloatlist(SFloatlist *);      /* Frees list of floats       */
void    freedata(Model *m);                   /* Frees allocated memory     */
int     openfiles(Model *m, char *,char *,char *);  /* Opens input & report files */
int     openhydfile(Model *m);                /* Opens hydraulics file      */
int     openoutfile(Model *m);                /* Opens binary output file   */
int     strcomp(char *, char *);          /* Compares two strings       */
char*   getTmpName(char* fname);          /* Gets temporary file name   */     //(2.00.12 - LR)
double  interp(int, double *,             /* Interpolates a data curve  */
               double *, double);
int     findnode(Model *m, char *);                 /* Finds node's index from ID */
int     findlink(Model *m, char *);                 /* Finds link's index from ID */
void    geterrmsg(int, char* msgOut);                   /* Gets text of error message */
void    errmsg(int);                      /* Reports program error      */
void    writecon(char *);                 /* Writes text to console     */
void    writewin(char *);                 /* Passes text to calling app */

/* ------- INPUT1.C --------------------*/
int     getdata(Model *m);                    /* Gets network data          */
void    setdefaults(Model *m);                /* Sets default values        */
void    initreport(Model *m);                 /* Initializes report options */
void    adjustdata(Model *m);                 /* Adjusts input data         */
int     inittanks(Model *m);                  /* Initializes tank levels    */
void    initunits(Model *m);                  /* Determines reporting units */
void    convertunits(Model *m);               /* Converts data to std. units*/

/* -------- INPUT2.C -------------------*/
int     netsize(Model *m);                    /* Determines network size    */
int     readdata(Model *m);                   /* Reads in network data      */
int     newline(Model *m, int, char *);             /* Processes new line of data */
int     addnodeID(Model *m, int, char *);           /* Adds node ID to data base  */
int     addlinkID(Model *m, int, char *);           /* Adds link ID to data base  */
int     addpattern(Model *m, char *);               /* Adds pattern to data base  */
int     addcurve(Model *m, char *);                 /* Adds curve to data base    */
int     addcoord(Model *m, char *);                 /* Adds coord to data base    */
STmplist *findID(char *, STmplist *);     /* Locates ID on linked list  */
int     unlinked(Model *m);                   /* Checks for unlinked nodes  */
int     getpumpparams(Model *m);              /* Computes pump curve coeffs.*/
int     getpatterns(Model *m);                /* Gets pattern data from list*/
int     getcurves(Model *m);                  /* Gets curve data from list  */
int     getcoords(Model *m);                  /* Gets coordinate data from list  */
int     findmatch(char *,char *[]);       /* Finds keyword in line      */
int     match(char *, char *);            /* Checks for word match      */
int     gettokens(char *s, char** Tok, int maxToks);                /* Tokenizes input line       */
int     getfloat(char *, double *);       /* Converts string to double   */
double  hour(char *, char *);             /* Converts time to hours     */
int     setreport(Model *m, char *);                /* Processes reporting command*/
void    inperrmsg(Model *m, int,int,char *);        /* Input error message        */

/* ---------- INPUT3.C -----------------*/
int     juncdata(Model *m);                   /* Processes junction data    */
int     tankdata(Model *m);                   /* Processes tank data        */
int     pipedata(Model *m);                   /* Processes pipe data        */
int     pumpdata(Model *m);                   /* Processes pump data        */
int     valvedata(Model *m);                  /* Processes valve data       */
int     patterndata(Model *m);                /* Processes pattern data     */
int     curvedata(Model *m);                  /* Processes curve data       */
int     coordata(Model *m);                   /* Processes coordinate data       */
int     demanddata(Model *m);                 /* Processes demand data      */
int     controldata(Model *m);                /* Processes simple controls  */
int     energydata(Model *m);                 /* Processes energy data      */
int     sourcedata(Model *m);                 /* Processes source data      */
int     emitterdata(Model *m);                /* Processes emitter data     */
int     qualdata(Model *m);                   /* Processes quality data     */
int     reactdata(Model *m);                  /* Processes reaction data    */
int     mixingdata(Model *m);                 /* Processes tank mixing data */
int     statusdata(Model *m);                 /* Processes link status data */
int     reportdata(Model *m);                 /* Processes report options   */
int     timedata(Model *m);                   /* Processes time options     */
int     optiondata(Model *m);                 /* Processes analysis options */
int     optionchoice(Model *m, int);                /* Processes option choices   */
int     optionvalue(Model *m, int);                 /* Processes option values    */
int     getpumpcurve(Model *m, int);                /* Constructs a pump curve    */
int     powercurve(double, double, double,/* Coeffs. of power pump curve*/
                   double, double, double *,
                   double *, double *);
int     valvecheck(Model *m, int, int, int);        /* Checks valve placement     */
void    changestatus(Model *m, int, char, double);  /* Changes status of a link   */

/* -------------- RULES.C --------------*/
void    initrules(Model *m);                  /* Initializes rule base      */
void    addrule(Model *m, char *);                  /* Adds rule to rule base     */
int     allocrules(Model *m);                 /* Allocates memory for rule  */
int     ruledata(Model *m);                   /* Processes rule input data  */
int     checkrules(Model *m, long);                 /* Checks all rules           */
void    freerules(Model *m);                  /* Frees rule base memory     */

/* ------------- REPORT.C --------------*/
int     writereport(Model *m);                /* Writes formatted report    */
void    writelogo(Model *m);                  /* Writes program logo        */
void    writesummary(Model *m);               /* Writes network summary     */
void    writehydstat(Model *m,int,double);          /* Writes hydraulic status    */
void    writeenergy(Model *m);                /* Writes energy usage        */
int     writeresults(Model *m);               /* Writes node/link results   */
void    writeheader(Model *m, int,int);             /* Writes heading on report   */
void    writeline(Model *m, char *);                /* Writes line to report file */
void    writerelerr(Model *m, int, double);          /* Writes convergence error   */
void    writestatchange(Model *m, int,char,char);   /* Writes link status change  */
void    writecontrolaction(Model *m, int, int);     /* Writes control action taken*/
void    writeruleaction(Model *m, int, char *);     /* Writes rule action taken   */
int     writehydwarn(Model *m, int,double);          /* Writes hydraulic warnings  */
void    writehyderr(Model *m, int);                 /* Writes hydraulic error msg.*/
int     disconnected(Model *m);               /* Checks for disconnections  */
void    marknodes(Model *m, int, int *, char *);    /* Identifies connected nodes */
void    getclosedlink(Model *m, int, char *);       /* Finds a disconnecting link */
void    writelimits(Model *m, int,int);             /* Writes reporting limits    */
int     checklimits(Model *m, double *,int,int);     /* Checks variable limits     */
void    writetime(Model *m, char *);                /* Writes current clock time  */
char    *clocktime(char *, long);         /* Converts time to hrs:min   */
char    *fillstr(char *, char, int);      /* Fills string with character*/
int     getnodetype(Model *m, int);                 /* Determines node type       */

/* --------- HYDRAUL.C -----------------*/
int     openhyd(Model *m);                    /* Opens hydraulics solver    */

/*** Updated 3/1/01 ***/
void    inithyd(int);                     /* Re-sets initial conditions */

int     runhyd(Model *m, long *);                   /* Solves 1-period hydraulics */
int     nexthyd(Model *m, long *);                  /* Moves to next time period  */
void    closehyd(Model *m);                   /* Closes hydraulics solver   */
int     allocmatrix(Model *m);                /* Allocates matrix coeffs.   */
void    freematrix(Model *m);                 /* Frees matrix coeffs.       */
void    initlinkflow(Model *m, int, char, double);  /* Initializes link flow      */
void    setlinkflow(Model *m, int, double);         /* Sets link flow via headloss*/
void    setlinkstatus(Model *m, int, char, char *,  /* Sets link status           */
                      double *);
void    setlinksetting(Model *m, int, double,       /* Sets pump/valve setting    */
                       char *, double *);
void    resistance(Model *m, int);                  /* Computes resistance coeff. */
void    demands(Model *m);                    /* Computes current demands   */
int     controls(Model *m);                   /* Controls link settings     */
long    timestep(Model *m);                   /* Computes new time step     */
void    tanktimestep(Model *m, long *);             /* Time till tanks fill/drain */
void    controltimestep(Model *m, long *);          /* Time till control action   */
void    ruletimestep(Model *m, long *);             /* Time till rule action      */
void    addenergy(Model *m, long);                  /* Accumulates energy usage   */
void    getenergy(Model *m, int, double *, double *); /* Computes link energy use   */
void    tanklevels(Model *m, long);                 /* Computes new tank levels   */
double  tankvolume(Model *m, int,double);           /* Finds tank vol. from grade */
double  tankgrade(Model *m, int,double);            /* Finds tank grade from vol. */
int     netsolve(Model *m, int *,double *);         /* Solves network equations   */
int     badvalve(Model *m, int);                    /* Checks for bad valve       */
int     valvestatus(Model *m);                /* Updates valve status       */
int     linkstatus(Model *m);                 /* Updates link status        */
char    cvstatus(Model *m, char,double,double);     /* Updates CV status          */
char    pumpstatus(Model *m, int,double);           /* Updates pump status        */
char    prvstatus(Model *m, int,char,double,        /* Updates PRV status         */
                  double,double);
char    psvstatus(Model *m, int,char,double,        /* Updates PSV status         */
                  double,double);
char    fcvstatus(Model *m, int,char,double,        /* Updates FCV status         */
                  double);
void    tankstatus(Model *m, int,int,int);          /* Checks if tank full/empty  */
int     pswitch(Model *m);                    /* Pressure switch controls   */
double  newflows(Model *m);                   /* Updates link flows         */
void    newcoeffs(Model *m);                  /* Computes matrix coeffs.    */
void    linkcoeffs(Model *m);                 /* Computes link coeffs.      */
void    nodecoeffs(Model *m);                 /* Computes node coeffs.      */
void    valvecoeffs(Model *m);                /* Computes valve coeffs.     */
void    pipecoeff(Model *m, int k);                   /* Computes pipe coeff.       */
double  DWcoeff(Model *m, int, double *);           /* Computes D-W coeff.        */
void    pumpcoeff(Model *m, int);                   /* Computes pump coeff.       */

/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void    curvecoeff(Model *m, int,double,double *,   /* Computes curve coeffs.     */
                             double *);

void    gpvcoeff(Model *m, int);                    /* Computes GPV coeff.        */
void    pbvcoeff(Model *m, int);                    /* Computes PBV coeff.        */
void    tcvcoeff(Model *m, int);                    /* Computes TCV coeff.        */
void    prvcoeff(Model *m, int,int,int);            /* Computes PRV coeff.        */
void    psvcoeff(Model *m, int,int,int);            /* Computes PSV coeff.        */
void    fcvcoeff(Model *m, int,int,int);            /* Computes FCV coeff.        */
void    emittercoeffs(Model *m);              /* Computes emitter coeffs.   */
double  emitflowchange(Model *m, int i);              /* Computes new emitter flow  */

/* ----------- SMATRIX.C ---------------*/
int     createsparse(Model *m);               /* Creates sparse matrix      */
int     allocsparse(Model *m);                /* Allocates matrix memory    */
void    freesparse(Model *m);                 /* Frees matrix memory        */
int     buildlists(Model *m, int);                  /* Builds adjacency lists     */
int     paralink(Model *m, int, int, int);          /* Checks for parallel links  */
void    xparalinks(Model *m);                 /* Removes parallel links     */
void    freelists(Model *m);                  /* Frees adjacency lists      */
void    countdegree(Model *m);                /* Counts links at each node  */
int     reordernodes(Model *m);               /* Finds a node re-ordering   */
int     mindegree(Model *m, int, int);              /* Finds min. degree node     */
int     growlist(Model *m, int);                    /* Augments adjacency list    */
int     newlink(Model *m, Padjlist);                /* Adds fill-ins for a node   */
int     linked(Model *m, int, int);                 /* Checks if 2 nodes linked   */
int     addlink(Model *m, int, int, int);           /* Creates new fill-in        */
int     storesparse(Model *m, int);                 /* Stores sparse matrix       */
int     ordersparse(Model *m, int);                 /* Orders matrix storage      */
void    transpose(int,int *,int *,        /* Transposes sparse matrix   */
        int *,int *,int *,int *,int *);
int     linsolve(Model *m, int, double *, double *, /* Solution of linear eqns.   */
                 double *);               /* via Cholesky factorization */

/* ----------- QUALITY.C ---------------*/
int     openqual(Model *m);                   /* Opens WQ solver system     */
void    initqual(Model *m);                   /* Initializes WQ solver      */
int     runqual(Model *m, long *);                  /* Gets current WQ results    */
int     nextqual(Model *m, long *);                 /* Updates WQ by hyd.timestep */
int     stepqual(Model *m, long *);                 /* Updates WQ by WQ time step */
int     closequal(Model *m);                  /* Closes WQ solver system    */
int     gethyd(Model *m, long *, long *);           /* Gets next hyd. results     */
char    setReactflag(Model *m);               /* Checks for reactive chem.  */
void    transport(Model *m, long);                  /* Transports mass in network */
void    initsegs(Model *m);                   /* Initializes WQ segments    */
void    reorientsegs(Model *m);               /* Re-orients WQ segments     */
void    updatesegs(Model *m, long);                 /* Updates quality in segments*/
void    removesegs(Model *m, int);                  /* Removes a WQ segment       */
void    addseg(Model *m, int,double,double);        /* Adds a WQ segment to pipe  */
void    accumulate(Model *m, long);                 /* Sums mass flow into node   */
void    updatenodes(Model *m, long);                /* Updates WQ at nodes        */
void    sourceinput(Model *m, long);                /* Computes source inputs     */
void    release(Model *m, long);                    /* Releases mass from nodes   */
void    updatetanks(Model *m, long);                /* Updates WQ in tanks        */
void    updatesourcenodes(Model *m, long);          /* Updates WQ at source nodes */
void    tankmix1(Model *m, int, long);              /* Complete mix tank model    */
void    tankmix2(Model *m, int, long);              /* 2-compartment tank model   */
void    tankmix3(Model *m, int, long);              /* FIFO tank model            */
void    tankmix4(Model *m, int, long);              /* LIFO tank model            */
double  sourcequal(Model *m, Psource);              /* Finds WQ input from source */
double  avgqual(Model *m, int);                     /* Finds avg. quality in pipe */
void    ratecoeffs(Model *m);                 /* Finds wall react. coeffs.  */
double  piperate(Model *m, int);                    /* Finds wall react. coeff.   */
double  pipereact(Model *m, int,double,double,long);/* Reacts water in a pipe     */
double  tankreact(Model *m, double,double,double,
                  long);                  /* Reacts water in a tank     */
double  bulkrate(Model *m, double,double,double);   /* Finds bulk reaction rate   */
double  wallrate(Model *m, double,double,double,double);/* Finds wall reaction rate   */


/* ------------ OUTPUT.C ---------------*/
int     savenetdata(Model *m);                /* Saves basic data to file   */
int     savehyd(Model *m, long *);                  /* Saves hydraulic solution   */
int     savehydstep(Model *m, long *);              /* Saves hydraulic timestep   */
int     saveenergy(Model *mvoid);                 /* Saves energy usage         */
int     readhyd(Model *m, long *);                  /* Reads hydraulics from file */
int     readhydstep(Model *m, long *);              /* Reads time step from file  */
int     saveoutput(Model *m);                 /* Saves results to file      */
int     nodeoutput(Model *m, int, REAL4 *, double); /* Saves node results to file */
int     linkoutput(Model *m, int, REAL4 *, double); /* Saves link results to file */
int     savefinaloutput(Model *m);            /* Finishes saving output     */
int     savetimestat(Model *m, REAL4 *, char);      /* Saves time stats to file   */
int     savenetreacts(Model *m, double, double,
                      double, double);    /* Saves react. rates to file */
int     saveepilog(Model *m);                 /* Saves output file epilog   */


/* ------------ INPFILE.C --------------*/
int     saveinpfile(Model *m, char *);              /* Saves network to text file  */

#endif
