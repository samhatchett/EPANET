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

void    initpointers(OW_Project *m);               /* Initializes pointers       */
int     allocdata(OW_Project *m);                  /* Allocates memory           */
void    freeTmplist(STmplist *);          /* Frees items in linked list */
void    freeFloatlist(SFloatlist *);      /* Frees list of floats       */
void    freedata(OW_Project *m);                   /* Frees allocated memory     */
int     openfiles(OW_Project *m, char *,char *,char *);  /* Opens input & report files */
int     openhydfile(OW_Project *m);                /* Opens hydraulics file      */
int     openoutfile(OW_Project *m);                /* Opens binary output file   */
int     strcomp(char *, char *);          /* Compares two strings       */
char*   getTmpName(char* fname);          /* Gets temporary file name   */     //(2.00.12 - LR)
double  interp(int, double *,             /* Interpolates a data curve  */
               double *, double);
int     findnode(OW_Project *m, char *);                 /* Finds node's index from ID */
int     findlink(OW_Project *m, char *);                 /* Finds link's index from ID */
void    geterrmsg(int, char* msgOut);                   /* Gets text of error message */
void    errmsg(OW_Project *m, int);                      /* Reports program error      */
void    writecon(char *);                 /* Writes text to console     */
void    writewin(char *);                 /* Passes text to calling app */

/* ------- INPUT1.C --------------------*/
int     getdata(OW_Project *m);                    /* Gets network data          */
void    setdefaults(OW_Project *m);                /* Sets default values        */
void    initreport(OW_Project *m);                 /* Initializes report options */
void    adjustdata(OW_Project *m);                 /* Adjusts input data         */
int     inittanks(OW_Project *m);                  /* Initializes tank levels    */
void    initunits(OW_Project *m);                  /* Determines reporting units */
void    convertunits(OW_Project *m);               /* Converts data to std. units*/

/* -------- INPUT2.C -------------------*/
int     netsize(OW_Project *m);                    /* Determines network size    */
int     readdata(OW_Project *m);                   /* Reads in network data      */
int     newline(OW_Project *m, int, char *);             /* Processes new line of data */
int     addnodeID(OW_Project *m, int, char *);           /* Adds node ID to data base  */
int     addlinkID(OW_Project *m, int, char *);           /* Adds link ID to data base  */
int     addpattern(OW_Project *m, char *);               /* Adds pattern to data base  */
int     addcurve(OW_Project *m, char *);                 /* Adds curve to data base    */
int     addcoord(OW_Project *m, char *);                 /* Adds coord to data base    */
STmplist *findID(char *, STmplist *);     /* Locates ID on linked list  */
int     unlinked(OW_Project *m);                   /* Checks for unlinked nodes  */
int     getpumpparams(OW_Project *m);              /* Computes pump curve coeffs.*/
int     getpatterns(OW_Project *m);                /* Gets pattern data from list*/
int     getcurves(OW_Project *m);                  /* Gets curve data from list  */
int     getcoords(OW_Project *m);                  /* Gets coordinate data from list  */
int     findmatch(char *,char *[]);       /* Finds keyword in line      */
int     match(char *, char *);            /* Checks for word match      */
int     gettokens(char *s, char** Tok, int maxToks);                /* Tokenizes input line       */
int     getfloat(char *, double *);       /* Converts string to double   */
double  hour(char *, char *);             /* Converts time to hours     */
int     setreport(OW_Project *m, char *);                /* Processes reporting command*/
void    inperrmsg(OW_Project *m, int,int,char *);        /* Input error message        */

/* ---------- INPUT3.C -----------------*/
int     juncdata(OW_Project *m);                   /* Processes junction data    */
int     tankdata(OW_Project *m);                   /* Processes tank data        */
int     pipedata(OW_Project *m);                   /* Processes pipe data        */
int     pumpdata(OW_Project *m);                   /* Processes pump data        */
int     valvedata(OW_Project *m);                  /* Processes valve data       */
int     patterndata(OW_Project *m);                /* Processes pattern data     */
int     curvedata(OW_Project *m);                  /* Processes curve data       */
int     coordata(OW_Project *m);                   /* Processes coordinate data       */
int     demanddata(OW_Project *m);                 /* Processes demand data      */
int     controldata(OW_Project *m);                /* Processes simple controls  */
int     energydata(OW_Project *m);                 /* Processes energy data      */
int     sourcedata(OW_Project *m);                 /* Processes source data      */
int     emitterdata(OW_Project *m);                /* Processes emitter data     */
int     qualdata(OW_Project *m);                   /* Processes quality data     */
int     reactdata(OW_Project *m);                  /* Processes reaction data    */
int     mixingdata(OW_Project *m);                 /* Processes tank mixing data */
int     statusdata(OW_Project *m);                 /* Processes link status data */
int     reportdata(OW_Project *m);                 /* Processes report options   */
int     timedata(OW_Project *m);                   /* Processes time options     */
int     optiondata(OW_Project *m);                 /* Processes analysis options */
int     optionchoice(OW_Project *m, int);                /* Processes option choices   */
int     optionvalue(OW_Project *m, int);                 /* Processes option values    */
int     getpumpcurve(OW_Project *m, int);                /* Constructs a pump curve    */
int     powercurve(double, double, double,/* Coeffs. of power pump curve*/
                   double, double, double *,
                   double *, double *);
int     valvecheck(OW_Project *m, int, int, int);        /* Checks valve placement     */
void    changestatus(OW_Project *m, int, char, double);  /* Changes status of a link   */

/* -------------- RULES.C --------------*/
void    initrules(OW_Project *m);                  /* Initializes rule base      */
void    addrule(OW_Project *m, char *);                  /* Adds rule to rule base     */
int     allocrules(OW_Project *m);                 /* Allocates memory for rule  */
int     parseRuleData(OW_Project *m);                   /* Processes rule input data  */
int     checkrules(OW_Project *m, long);                 /* Checks all rules           */
void    freerules(OW_Project *m);                  /* Frees rule base memory     */

/* ------------- REPORT.C --------------*/
int     writereport(OW_Project *m);                /* Writes formatted report    */
void    writelogo(OW_Project *m);                  /* Writes program logo        */
void    writesummary(OW_Project *m);               /* Writes network summary     */
void    writehydstat(OW_Project *m,int,double);          /* Writes hydraulic status    */
void    writeenergy(OW_Project *m);                /* Writes energy usage        */
int     writeresults(OW_Project *m);               /* Writes node/link results   */
void    writeheader(OW_Project *m, int,int);             /* Writes heading on report   */
void    writeline(OW_Project *m, char *);                /* Writes line to report file */
void    writerelerr(OW_Project *m, int, double);          /* Writes convergence error   */
void    writestatchange(OW_Project *m, int,char,char);   /* Writes link status change  */
void    writecontrolaction(OW_Project *m, int, int);     /* Writes control action taken*/
void    writeruleaction(OW_Project *m, int, char *);     /* Writes rule action taken   */
int     writehydwarn(OW_Project *m, int,double);          /* Writes hydraulic warnings  */
void    writehyderr(OW_Project *m, int);                 /* Writes hydraulic error msg.*/
int     disconnected(OW_Project *m);               /* Checks for disconnections  */
void    marknodes(OW_Project *m, int, int *, char *);    /* Identifies connected nodes */
void    getclosedlink(OW_Project *m, int, char *);       /* Finds a disconnecting link */
void    writelimits(OW_Project *m, int,int);             /* Writes reporting limits    */
int     checklimits(OW_Project *m, double *,int,int);     /* Checks variable limits     */
void    writetime(OW_Project *m, char *);                /* Writes current clock time  */
char    *clocktime(char *, long);         /* Converts time to hrs:min   */
char    *fillstr(char *, char, int);      /* Fills string with character*/
int     getnodetype(OW_Project *m, int);                 /* Determines node type       */

/* --------- HYDRAUL.C -----------------*/
int     openhyd(OW_Project *m);                    /* Opens hydraulics solver    */

/*** Updated 3/1/01 ***/
void    inithyd(OW_Project *m, int);                     /* Re-sets initial conditions */

int     runhyd(OW_Project *m, long *);                   /* Solves 1-period hydraulics */
int     nexthyd(OW_Project *m, long *);                  /* Moves to next time period  */
void    closehyd(OW_Project *m);                   /* Closes hydraulics solver   */
int     allocmatrix(OW_Project *m);                /* Allocates matrix coeffs.   */
void    freematrix(OW_Project *m);                 /* Frees matrix coeffs.       */
void    initlinkflow(OW_Project *m, int, char, double);  /* Initializes link flow      */
void    setlinkflow(OW_Project *m, int, double);         /* Sets link flow via headloss*/
void    setlinkstatus(OW_Project *m, int, char, char *,  /* Sets link status           */
                      double *);
void    setlinksetting(OW_Project *m, int, double,       /* Sets pump/valve setting    */
                       char *, double *);
void    resistance(OW_Project *m, int);                  /* Computes resistance coeff. */
void    demands(OW_Project *m);                    /* Computes current demands   */
int     controls(OW_Project *m);                   /* Controls link settings     */
long    timestep(OW_Project *m);                   /* Computes new time step     */
void    tanktimestep(OW_Project *m, long *);             /* Time till tanks fill/drain */
void    controltimestep(OW_Project *m, long *);          /* Time till control action   */
void    ruletimestep(OW_Project *m, long *);             /* Time till rule action      */
void    addenergy(OW_Project *m, long);                  /* Accumulates energy usage   */
void    getenergy(OW_Project *m, int, double *, double *); /* Computes link energy use   */
void    tanklevels(OW_Project *m, long);                 /* Computes new tank levels   */
double  tankvolume(OW_Project *m, int,double);           /* Finds tank vol. from grade */
double  tankgrade(OW_Project *m, int,double);            /* Finds tank grade from vol. */
int     netsolve(OW_Project *m, int *,double *);         /* Solves network equations   */
int     badvalve(OW_Project *m, int);                    /* Checks for bad valve       */
int     valvestatus(OW_Project *m);                /* Updates valve status       */
int     linkstatus(OW_Project *m);                 /* Updates link status        */
char    cvstatus(OW_Project *m, char,double,double);     /* Updates CV status          */
char    pumpstatus(OW_Project *m, int,double);           /* Updates pump status        */
char    prvstatus(OW_Project *m, int,char,double,        /* Updates PRV status         */
                  double,double);
char    psvstatus(OW_Project *m, int,char,double,        /* Updates PSV status         */
                  double,double);
char    fcvstatus(OW_Project *m, int,char,double,        /* Updates FCV status         */
                  double);
void    tankstatus(OW_Project *m, int,int,int);          /* Checks if tank full/empty  */
int     pswitch(OW_Project *m);                    /* Pressure switch controls   */
double  newflows(OW_Project *m);                   /* Updates link flows         */
void    newcoeffs(OW_Project *m);                  /* Computes matrix coeffs.    */
void    linkcoeffs(OW_Project *m);                 /* Computes link coeffs.      */
void    nodecoeffs(OW_Project *m);                 /* Computes node coeffs.      */
void    valvecoeffs(OW_Project *m);                /* Computes valve coeffs.     */
void    pipecoeff(OW_Project *m, int k);                   /* Computes pipe coeff.       */
double  DWcoeff(OW_Project *m, int, double *);           /* Computes D-W coeff.        */
void    pumpcoeff(OW_Project *m, int);                   /* Computes pump coeff.       */

/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void    curvecoeff(OW_Project *m, int,double,double *,   /* Computes curve coeffs.     */
                             double *);

void    gpvcoeff(OW_Project *m, int);                    /* Computes GPV coeff.        */
void    pbvcoeff(OW_Project *m, int);                    /* Computes PBV coeff.        */
void    tcvcoeff(OW_Project *m, int);                    /* Computes TCV coeff.        */
void    prvcoeff(OW_Project *m, int,int,int);            /* Computes PRV coeff.        */
void    psvcoeff(OW_Project *m, int,int,int);            /* Computes PSV coeff.        */
void    fcvcoeff(OW_Project *m, int,int,int);            /* Computes FCV coeff.        */
void    emittercoeffs(OW_Project *m);              /* Computes emitter coeffs.   */
double  emitflowchange(OW_Project *m, int i);              /* Computes new emitter flow  */

/* ----------- SMATRIX.C ---------------*/
int     createsparse(OW_Project *m);               /* Creates sparse matrix      */
int     allocsparse(OW_Project *m);                /* Allocates matrix memory    */
void    freesparse(OW_Project *m);                 /* Frees matrix memory        */
int     buildlists(OW_Project *m, int);                  /* Builds adjacency lists     */
int     paralink(OW_Project *m, int, int, int);          /* Checks for parallel links  */
void    xparalinks(OW_Project *m);                 /* Removes parallel links     */
void    freelists(OW_Project *m);                  /* Frees adjacency lists      */
void    countdegree(OW_Project *m);                /* Counts links at each node  */
int     reordernodes(OW_Project *m);               /* Finds a node re-ordering   */
int     mindegree(OW_Project *m, int, int);              /* Finds min. degree node     */
int     growlist(OW_Project *m, int);                    /* Augments adjacency list    */
int     newlink(OW_Project *m, Padjlist);                /* Adds fill-ins for a node   */
int     linked(OW_Project *m, int, int);                 /* Checks if 2 nodes linked   */
int     addlink(OW_Project *m, int, int, int);           /* Creates new fill-in        */
int     storesparse(OW_Project *m, int);                 /* Stores sparse matrix       */
int     ordersparse(OW_Project *m, int);                 /* Orders matrix storage      */
void    transpose(int,int *,int *,        /* Transposes sparse matrix   */
        int *,int *,int *,int *,int *);
int     linsolve(OW_Project *m, int, double *, double *, /* Solution of linear eqns.   */
                 double *);               /* via Cholesky factorization */

/* ----------- QUALITY.C ---------------*/
int     openqual(OW_Project *m);                   /* Opens WQ solver system     */
void    initqual(OW_Project *m);                   /* Initializes WQ solver      */
int     runqual(OW_Project *m, long *);                  /* Gets current WQ results    */
int     nextqual(OW_Project *m, long *);                 /* Updates WQ by hyd.timestep */
int     stepqual(OW_Project *m, long *);                 /* Updates WQ by WQ time step */
int     closequal(OW_Project *m);                  /* Closes WQ solver system    */
int     gethyd(OW_Project *m, long *, long *);           /* Gets next hyd. results     */
char    setReactflag(OW_Project *m);               /* Checks for reactive chem.  */
void    transport(OW_Project *m, long);                  /* Transports mass in network */
void    initsegs(OW_Project *m);                   /* Initializes WQ segments    */
void    reorientsegs(OW_Project *m);               /* Re-orients WQ segments     */
void    updatesegs(OW_Project *m, long);                 /* Updates quality in segments*/
void    removesegs(OW_Project *m, int);                  /* Removes a WQ segment       */
void    addseg(OW_Project *m, int,double,double);        /* Adds a WQ segment to pipe  */
void    accumulate(OW_Project *m, long);                 /* Sums mass flow into node   */
void    updatenodes(OW_Project *m, long);                /* Updates WQ at nodes        */
void    sourceinput(OW_Project *m, long);                /* Computes source inputs     */
void    release(OW_Project *m, long);                    /* Releases mass from nodes   */
void    updatetanks(OW_Project *m, long);                /* Updates WQ in tanks        */
void    updatesourcenodes(OW_Project *m, long);          /* Updates WQ at source nodes */
void    tankmix1(OW_Project *m, int, long);              /* Complete mix tank model    */
void    tankmix2(OW_Project *m, int, long);              /* 2-compartment tank model   */
void    tankmix3(OW_Project *m, int, long);              /* FIFO tank model            */
void    tankmix4(OW_Project *m, int, long);              /* LIFO tank model            */
double  sourcequal(OW_Project *m, Psource);              /* Finds WQ input from source */
double  avgqual(OW_Project *m, int);                     /* Finds avg. quality in pipe */
void    ratecoeffs(OW_Project *m);                 /* Finds wall react. coeffs.  */
double  piperate(OW_Project *m, int);                    /* Finds wall react. coeff.   */
double  pipereact(OW_Project *m, int,double,double,long);/* Reacts water in a pipe     */
double  tankreact(OW_Project *m, double,double,double,
                  long);                  /* Reacts water in a tank     */
double  bulkrate(OW_Project *m, double,double,double);   /* Finds bulk reaction rate   */
double  wallrate(OW_Project *m, double,double,double,double);/* Finds wall reaction rate   */


/* ------------ OUTPUT.C ---------------*/
int     savenetdata(OW_Project *m);                /* Saves basic data to file   */
int     savehyd(OW_Project *m, long *);                  /* Saves hydraulic solution   */
int     savehydstep(OW_Project *m, long *);              /* Saves hydraulic timestep   */
int     saveenergy(OW_Project *mvoid);                 /* Saves energy usage         */
int     readhyd(OW_Project *m, long *);                  /* Reads hydraulics from file */
int     readhydstep(OW_Project *m, long *);              /* Reads time step from file  */
int     saveoutput(OW_Project *m);                 /* Saves results to file      */
int     nodeoutput(OW_Project *m, int, REAL4 *, double); /* Saves node results to file */
int     linkoutput(OW_Project *m, int, REAL4 *, double); /* Saves link results to file */
int     savefinaloutput(OW_Project *m);            /* Finishes saving output     */
int     savetimestat(OW_Project *m, REAL4 *, char);      /* Saves time stats to file   */
int     savenetreacts(OW_Project *m, double, double,
                      double, double);    /* Saves react. rates to file */
int     saveepilog(OW_Project *m);                 /* Saves output file epilog   */


/* ------------ INPFILE.C --------------*/
int     saveinpfile(OW_Project *m, char *);              /* Saves network to text file  */

#endif
