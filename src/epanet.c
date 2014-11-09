/*
*******************************************************************************

EPANET.C -- Hydraulic & Water Quality Simulator for Water Distribution Networks

VERSION:    2.00
DATE:       5/30/00
            9/7/00
            10/25/00
            3/1/01
            11/19/01
            6/24/02
            8/15/07    (2.00.11)
            2/14/08    (2.00.12)
            2014-07-10 (2.1)
AUTHOR:     L. Rossman
            US EPA - NRMRL
            Also, Jim Uber, Feng Shang, Sam Hatchett...

EPANET performs extended period hydraulic and water quality analysis of
looped, pressurized piping networks. The program consists of the
following code modules:
  
    EPANET.C  -- main module providing supervisory control
    INPUT1.C  -- controls processing of input data
    INPUT2.C  -- reads data from input file
    INPUT3.C  -- parses individual lines of input data
    INPFILE.C -- saves modified input data to a text file
    RULES.C   -- implements rule-based control of piping system
    HYDRAUL.C -- computes extended period hydraulic behavior
    QUALITY.C -- tracks transport & fate of water quality
    OUTPUT.C  -- handles transfer of data to and from binary files
    REPORT.C  -- handles reporting of results to text file
    SMATRIX.C -- sparse matrix linear equation solver routines
    MEMPOOL.C -- memory allocation routines
    HASH.C    -- hash table routines

The program can be compiled as either a stand-alone console application
or as a dynamic link library (DLL) of function calls depending on whether
the macro identifier 'DLL' is defined or not.

See TOOLKIT.H for function prototypes of exported DLL functions
See FUNCS.H for prototypes of all other functions
See TYPES.H for declaration of global constants and data structures
See VARS.H for declaration of global variables
See TEXT.H for declaration of all string constants
See ENUMSTXT.H for assignment of string constants to enumerated types

The following naming conventions are used in all modules of this program:
1. Names of exportable functions in the DLL begin with the "EN" prefix.
2. All other function names are lowercase.
3. Global variable names begin with an uppercase letter.
4. Local variable names are all lowercase.
5. Declared constants and enumerated values defined in TYPES.H are
   all uppercase.
6. String constants defined in TEXT.H begin with a lower case character
   followed by an underscore and then all uppercase characters (e.g.
   t_HEADLOSS)

--------------------------------------------------------------------------

This is the main module of the EPANET program. It uses a series of
functions, all beginning with the letters EN, to control program behavior.
See the main() and ENepanet() functions below for the simplest example of
these.

This module calls the following functions that reside in other modules:
   RULES.C
     initrules()
     allocrules()
     closerules()
   INPUT1.C
     getdata()
     initreport()
   INPUT2.C
     netsize()
     setreport()
   HYDRAUL.C
     openhyd()
     inithyd()
     runhyd()
     nexthyd()
     closehyd()
     resistance()
     tankvolume()
     getenergy()
     setlinkstatus()
     setlinksetting()
   QUALITY.C
     openqual()
     initqual()
     runqual()
     nextqual()
     stepqual()
     closequal()
   REPORT.C
     writeline()
     writelogo()
     writereport()
   HASH.C
     ENHashTablecreate()
     ENHashTableFind()
     ENHashTableFree()

The macro ERRCODE(x) is defined in TYPES.H. It says if the current
value of the error code variable (errcode) is not fatal (< 100) then
execute function x and set the error code equal to its return value.

*******************************************************************************
*/

/*** Need to define WINDOWS to use the getTmpName function ***/                //(2.00.12 - LR)
// --- define WINDOWS
#undef WINDOWS
#ifdef _WIN32
  #define WINDOWS
#endif
#ifdef __WIN32__
  #define WINDOWS
#endif
/************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <math.h>
#include <float.h>                                                             //(2.00.12 - LR)
 
#include "text.h"
#include "types.h"
#include "epanet2.h"
#include "enumstxt.h"
#include "funcs.h"
#define  EXTERN
#include "vars.h"

void (* viewprog) (char *);     /* Pointer to progress viewing function */   


/*
----------------------------------------------------------------
   Entry point used to compile a Windows DLL
----------------------------------------------------------------
*/

/*** This code is no longer required *****                                     //(2.00.11 - LR)
#ifdef DLL
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* reserved)
{
        viewprog = NULL;
        return 1;
}
#endif
*****************************************/


/*
----------------------------------------------------------------
   Entry point used to compile a stand-alone executable.
----------------------------------------------------------------
*/

#ifdef CLE                                                                     //(2.00.11 - LR)

int   main(int argc, char *argv[])
/*--------------------------------------------------------------
**  Input:   argc    = number of command line arguments
**           *argv[] = array of command line arguments
**  Output:  none
**  Purpose: main program segment
**
**  Command line for stand-alone operation is:
**    progname f1  f2  f3
**  where progname = name of executable this code was compiled to,
**  f1 = name of input file, f2 = name of report file, and
**  f3 = name of binary output file (optional).
**--------------------------------------------------------------
*/
{
    char *f1,*f2,*f3;
    char blank[] = "";
    int  errcode;

/* Check for proper number of command line arguments */
    if (argc < 3) writecon(FMT03);
    else
    {

    /* Call the main control function */
       f1 = argv[1];
       f2 = argv[2];
       if (argc > 3) f3 = argv[3];
       else          f3 = blank;
       writecon(FMT01);
       errcode = ENepanet(f1,f2,f3,NULL);
       if (errcode > 0) writecon(FMT11);
       else if (en_defaultModel->Warnflag > 0) writecon(FMT10);
       else writecon(FMT09);
    }
    return(0);
}                                       /* End of main */
#endif


/*
----------------------------------------------------------------
   Functions for opening & closing the EPANET system
----------------------------------------------------------------
*/


/*** updated 3/1/01 ***/
int DLLEXPORT ENepanet(char *f1, char *f2, char *f3, void (*pviewprog) (char *))

/*------------------------------------------------------------------------
**   Input:   f1 = pointer to name of input file              
**            f2 = pointer to name of report file             
**            f3 = pointer to name of binary output file      
**            pviewprog = see note below                 
**   Output:  none  
**  Returns: error code                              
**  Purpose: runs a complete EPANET simulation
**
**  The pviewprog() argument is a pointer to a callback function
**  that takes a character string (char *) as its only parameter.
**  The function would reside in and be used by the calling
**  program to display the progress messages that EPANET generates
**  as it carries out its computations. If this feature is not
**  needed then the argument should be NULL.
**-------------------------------------------------------------------------
*/
{
  
    int  errcode = 0;
    viewprog = pviewprog;
    ERRCODE(ENopen(f1, f2, f3));
    if (en_defaultModel->Hydflag != USE) {
      ERRCODE(ENsolveH());
    }
    ERRCODE(ENsolveQ());
    ERRCODE(ENreport());
    ENclose();
    return(errcode);
}


int DLLEXPORT ENopen(char *f1, char *f2, char *f3)
/*----------------------------------------------------------------
**  Input:   f1 = pointer to name of input file              
**           f2 = pointer to name of report file             
**           f3 = pointer to name of binary output file      
**  Output:  none 
**  Returns: error code                              
**  Purpose: opens EPANET input file & reads in network data
**----------------------------------------------------------------
*/
{
  int err;
  OW_Project *newModel;
  err = OW_open(f1, &newModel, f2, f3);
  en_defaultModel = newModel;
  return err;
}


int DLLEXPORT ENsaveinpfile(char *filename)
/*----------------------------------------------------------------
**  Input:   filename = name of INP file
**  Output:  none 
**  Returns: error code                              
**  Purpose: saves current data base to file                        
**----------------------------------------------------------------
*/
{
  return OW_saveinpfile(en_defaultModel, filename);
}


int DLLEXPORT ENclose()
/*----------------------------------------------------------------
**  Input:   none                    
**  Output:  none 
**  Returns: error code
**  Purpose: frees all memory & files used by EPANET                 
**----------------------------------------------------------------
*/
{
  return OW_close(en_defaultModel);
}


/*
----------------------------------------------------------------
   Functions for running a hydraulic analysis
----------------------------------------------------------------
*/


int DLLEXPORT ENsolveH()
/*----------------------------------------------------------------
**  Input:   none                    
**  Output:  none 
**  Returns: error code                              
**  Purpose: solves for network hydraulics in all time periods                          
**----------------------------------------------------------------
*/
{
  return OW_solveH(en_defaultModel);
}


int DLLEXPORT ENsaveH()
/*----------------------------------------------------------------
**  Input:   none                   
**  Output:  none 
**  Returns: error code                              
**  Purpose: saves hydraulic results to binary file.
**
**  Must be called before ENreport() if no WQ simulation made.
**  Should not be called if ENsolveQ() will be used.                  
**----------------------------------------------------------------
*/
{
  return OW_saveH(en_defaultModel);
}


int DLLEXPORT ENopenH()
/*----------------------------------------------------------------
**  Input:   none                   
**  Output:  none 
**  Returns: error code                              
**  Purpose: sets up data structures for hydraulic analysis          
**----------------------------------------------------------------
*/
{
  return OW_openH(en_defaultModel);
}


/*** Updated 3/1/01 ***/
int DLLEXPORT ENinitH(int flag)
/*----------------------------------------------------------------
**  Input:   flag = 2-digit flag where 1st (left) digit indicates
**                  if link flows should be re-initialized (1) or
**                  not (0) and 2nd digit indicates if hydraulic
**                  results should be saved to file (1) or not (0)
**  Output:  none 
**  Returns: error code
**  Purpose: initializes hydraulic analysis          
**----------------------------------------------------------------
*/
{
  return OW_initH(en_defaultModel, flag);
}


int DLLEXPORT ENrunH(long *t)
/*----------------------------------------------------------------
**  Input:   none (no need to supply a value for *t)
**  Output:  *t = current simulation time (seconds) 
**  Returns: error/warning code                              
**  Purpose: solves hydraulics for conditions at time t.
** 
**  This function is used in a loop with ENnextH() to run
**  an extended period hydraulic simulation.
**  See ENsolveH() for an example.
**----------------------------------------------------------------
*/
{
  return OW_runH(en_defaultModel, t);
}


int DLLEXPORT ENnextH(long *tstep)
/*----------------------------------------------------------------
**  Input:   none (no need to supply a value for *tstep)
**  Output:  *tstep = time (seconds) until next hydraulic event
**                    (0 marks end of simulation period)
**  Returns: error code                              
**  Purpose: determines time until next hydraulic event.
** 
**  This function is used in a loop with ENrunH() to run
**  an extended period hydraulic simulation.
**  See ENsolveH() for an example.
**----------------------------------------------------------------
*/
{
  return OW_nextH(en_defaultModel, tstep);
}


int DLLEXPORT ENcloseH()
/*----------------------------------------------------------------
**  Input:   none                   
**  Output:  none 
**  Returns: error code
**  Purpose: frees data allocated by hydraulics solver       
**----------------------------------------------------------------
*/
{
  return OW_closeH(en_defaultModel);
}


int DLLEXPORT ENsavehydfile(char *filename)
/*----------------------------------------------------------------
**  Input:   filename = name of file
**  Output:  none 
**  Returns: error code
**  Purpose: copies binary hydraulics file to disk
**----------------------------------------------------------------
*/
{
  return OW_savehydfile(en_defaultModel, filename);
}


int DLLEXPORT ENusehydfile(char *filename)
/*----------------------------------------------------------------
**  Input:   filename = name of file
**  Output:  none 
**  Returns: error code
**  Purpose: opens previously saved binary hydraulics file
**----------------------------------------------------------------
*/
{
  return OW_usehydfile(en_defaultModel, filename);
}


/*
----------------------------------------------------------------
   Functions for running a WQ analysis
----------------------------------------------------------------
*/


int DLLEXPORT ENsolveQ()
/*----------------------------------------------------------------
**  Input:   none                    
**  Output:  none 
**  Returns: error code                              
**  Purpose: solves for network water quality in all time periods
**----------------------------------------------------------------
*/
{
  return OW_solveQ(en_defaultModel);
}


int DLLEXPORT ENopenQ()
/*----------------------------------------------------------------
**  Input:   none                    
**  Output:  none 
**  Returns: error code                              
**  Purpose: sets up data structures for WQ analysis
**----------------------------------------------------------------
*/
{
  return OW_openQ(en_defaultModel);
}


int DLLEXPORT ENinitQ(int saveflag)
/*----------------------------------------------------------------
**  Input:   saveflag = EN_SAVE (1) if results saved to file,
**                      EN_NOSAVE (0) if not                    
**  Output:  none 
**  Returns: error code
**  Purpose: initializes WQ analysis
**----------------------------------------------------------------
*/
{
  return OW_initQ(en_defaultModel, saveflag);
}


int DLLEXPORT ENrunQ(long *t)
/*----------------------------------------------------------------
**  Input:   none (no need to supply a value for *t)
**  Output:  *t = current simulation time (seconds) 
**  Returns: error code                              
**  Purpose: retrieves hydraulic & WQ results at time t.
**
**  This function is used in a loop with ENnextQ() to run
**  an extended period WQ simulation. See ENsolveQ() for
**  an example.
**----------------------------------------------------------------
*/
{
  return OW_runQ(en_defaultModel, t);
}


int DLLEXPORT ENnextQ(long *tstep)
/*----------------------------------------------------------------
**  Input:   none (no need to supply a value for *tstep)
**  Output:  *tstep = time (seconds) until next hydraulic event
**                    (0 marks end of simulation period)
**  Returns: error code                              
**  Purpose: advances WQ simulation to next hydraulic event.
**
**  This function is used in a loop with ENrunQ() to run
**  an extended period WQ simulation. See ENsolveQ() for
**  an example.
**----------------------------------------------------------------
*/
{
  return OW_nextQ(en_defaultModel, tstep);
}


int DLLEXPORT ENstepQ(long *tleft)
/*----------------------------------------------------------------
**  Input:   none
**  Output:  *tleft = time left in overall simulation (seconds) 
**  Returns: error code                              
**  Purpose: advances WQ simulation by a single WQ time step
**
**  This function is used in a loop with ENrunQ() to run
**  an extended period WQ simulation.
**----------------------------------------------------------------
*/
{
  return OW_stepQ(en_defaultModel, tleft);
}


int DLLEXPORT ENcloseQ()
/*----------------------------------------------------------------
**  Input:   none                    
**  Output:  none 
**  Returns: error code                              
**  Purpose: frees data allocated by WQ solver
**----------------------------------------------------------------
*/
{
  return OW_closeQ(en_defaultModel);
}


/*
----------------------------------------------------------------
   Functions for generating an output report
----------------------------------------------------------------
*/


int DLLEXPORT ENwriteline(char *line)
/*----------------------------------------------------------------
**  Input:   line = text string                    
**  Output:  none 
**  Returns: error code                              
**  Purpose: writes line of text to report file                            
**----------------------------------------------------------------
*/
{
  return OW_writeline(en_defaultModel, line);
}


int DLLEXPORT ENreport()
/*----------------------------------------------------------------
**  Input:   none                    
**  Output:  none 
**  Returns: error code                              
**  Purpose: writes report to report file                            
**----------------------------------------------------------------
*/
{
  return OW_report(en_defaultModel);
}


int  DLLEXPORT ENresetreport()
/*----------------------------------------------------------------
**  Input:   none                    
**  Output:  none 
**  Returns: error code
**  Purpose: resets report options to default values                            
**----------------------------------------------------------------
*/
{
  return OW_resetreport(en_defaultModel);
}


int  DLLEXPORT ENsetreport(char *s)
/*----------------------------------------------------------------
**  Input:   s = report format command                    
**  Output:  none
**  Returns: error code                              
**  Purpose: processes a reporting format command                            
**----------------------------------------------------------------
*/
{
  return OW_setreport(en_defaultModel, s);
}


/*
----------------------------------------------------------------
   Functions for retrieving network information
----------------------------------------------------------------
*/

/*** Updated 10/25/00 ***/
int DLLEXPORT ENgetversion(int *v)
/*----------------------------------------------------------------
**  Input:    none
**  Output:   *v = version number of the source code
**  Returns:  error code (should always be 0)
**  Purpose:  retrieves a number assigned to the most recent
**            update of the source code. This number, set by the
**            constant CODEVERSION found in TYPES.H,  began with
**            20001 and increases by 1 with each new update.
**----------------------------------------------------------------
*/
{
  return OW_getversion(v);
}


int DLLEXPORT ENgetcontrol(int cindex, int *ctype, int *lindex,
              EN_API_FLOAT_TYPE *setting, int *nindex, EN_API_FLOAT_TYPE *level)
/*----------------------------------------------------------------
**  Input:   cindex   = control index (position of control statement
**                      in the input file, starting from 1) 
**  Output:  *ctype   = control type code (see TOOLKIT.H)
**           *lindex  = index of controlled link
**           *setting = control setting on link
**           *nindex  = index of controlling node (0 for TIMER
**                      or TIMEOFDAY control)
**           *level   = control level (tank level, junction
**                      pressure, or time (seconds))
**  Returns: error code                              
**  Purpose: retrieves parameters that define a simple control                 
**----------------------------------------------------------------
*/
{
  return OW_getcontrol(en_defaultModel, cindex, ctype, lindex, setting, nindex, level);
}         


int DLLEXPORT ENgetcount(int code, int *count)
/*----------------------------------------------------------------
**  Input:   code = component code (see TOOLKIT.H)                    
**  Output:  *count = number of components in network
**  Returns: error code                              
**  Purpose: retrieves the number of components of a 
**           given type in the network  
**----------------------------------------------------------------
*/
{
  return OW_getcount(en_defaultModel, code, count);
}


int  DLLEXPORT ENgetoption(int code, EN_API_FLOAT_TYPE *value)
/*----------------------------------------------------------------
**  Input:   code = option code (see TOOLKIT.H)
**  Output:  *value = option value
**  Returns: error code                              
**  Purpose: gets value for an analysis option 
**----------------------------------------------------------------
*/
{
  return OW_getoption(en_defaultModel, code, value);
}


int DLLEXPORT ENgettimeparam(int code, long *value)
/*----------------------------------------------------------------
**  Input:   code = time parameter code (see TOOLKIT.H)
**  Output:  *value = value of time parameter 
**  Returns: error code                              
**  Purpose: retrieves value of specific time parameter                 
**----------------------------------------------------------------
*/
{
  return OW_gettimeparam(en_defaultModel, code, value);
}


int DLLEXPORT ENgetflowunits(int *code)
/*----------------------------------------------------------------
**  Input:   none                    
**  Output:  *code = code of flow units in use 
**                   (see TOOLKIT.H or TYPES.H)
**  Returns: error code                              
**  Purpose: retrieves flow units code 
**----------------------------------------------------------------
*/
{
  return OW_getflowunits(en_defaultModel, code);
}


int  DLLEXPORT  ENgetpatternindex(char *id, int *index)
/*----------------------------------------------------------------
**  Input:   id     = time pattern ID
**  Output:  *index = index of time pattern in list of patterns
**  Returns: error code                              
**  Purpose: retrieves index of time pattern with specific ID 
**----------------------------------------------------------------
*/
{
  return OW_getpatternindex(en_defaultModel, id, index);
}


int DLLEXPORT ENgetpatternid(int index, char *id)
/*----------------------------------------------------------------
**  Input:   index = index of time pattern
**  Output:  id    = pattern ID
**  Returns: error code                              
**  Purpose: retrieves ID of a time pattern with specific index
**
**  NOTE: 'id' must be able to hold MAXID characters
**----------------------------------------------------------------
*/
{
  return OW_getpatternid(en_defaultModel, index, id);
}


int DLLEXPORT ENgetpatternlen(int index, int *len)
/*----------------------------------------------------------------
**  Input:   index = index of time pattern
**  Output:  *len  = pattern length (number of multipliers)
**  Returns: error code                              
**  Purpose: retrieves number of multipliers in a time pattern
**----------------------------------------------------------------
*/
{
  return OW_getpatternlen(en_defaultModel, index, len);
}


int DLLEXPORT ENgetpatternvalue(int index, int period, EN_API_FLOAT_TYPE *value)
/*----------------------------------------------------------------
**  Input:   index  = index of time pattern
**           period = pattern time period
**  Output:  *value = pattern multiplier
**  Returns: error code                              
**  Purpose: retrieves multiplier for a specific time period
**           and pattern
**----------------------------------------------------------------
*/
{
  return OW_getpatternvalue(en_defaultModel, index, period, value);
}


int  DLLEXPORT ENgetqualtype(int *qualcode, int *tracenode)
/*----------------------------------------------------------------
**  Input:   none
**  Output:  *qualcode  = WQ analysis code number (see TOOLKIT.H)
**           *tracenode = index of node being traced (if
**                        qualocode = WQ tracing)
**  Returns: error code                              
**  Purpose: retrieves type of quality analysis called for 
**----------------------------------------------------------------
*/
{
  return OW_getqualtype(en_defaultModel, qualcode, tracenode);
}

int DLLEXPORT ENgetqualinfo(int *qualcode, char *chemname, char *chemunits, int *tracenode)
{
  return OW_getqualinfo(en_defaultModel, qualcode, chemname, chemunits, tracenode);
}

int  DLLEXPORT ENgeterror(int errcode, char *errmsg, int n)
/*----------------------------------------------------------------
**  Input:   errcode = error/warning code number
**           n       = maximum length of string errmsg
**  Output:  errmsg  = text of error/warning message
**  Returns: error code
**  Purpose: retrieves text of error/warning message 
**----------------------------------------------------------------
*/
{
  return OW_geterror(errcode, errmsg, n);
}

int  DLLEXPORT ENgetstatistic(int code, EN_API_FLOAT_TYPE* value)
/*----------------------------------------------------------------
 **  Input:   code    = type of simulation statistic to retrieve
 **  Output:  value   = value of requested statistic
 **  Returns: error code
 **  Purpose: retrieves hydraulic simulation statistic
 **----------------------------------------------------------------
 */
{
  return OW_getstatistic(en_defaultModel, code, value);
}

/*
----------------------------------------------------------------
   Functions for retrieving node data
----------------------------------------------------------------
*/


int DLLEXPORT ENgetnodeindex(char *id, int *index)
/*----------------------------------------------------------------
**  Input:   id = node ID
**  Output:  *index = index of node in list of nodes 
**  Returns: error code                              
**  Purpose: retrieves index of a node with specific ID 
**----------------------------------------------------------------
*/
{
  return OW_getnodeindex(en_defaultModel, id, index);
}


int DLLEXPORT ENgetnodeid(int index, char *id)
/*----------------------------------------------------------------
**  Input:   index = index of node in list of nodes                    
**  Output:  id = node ID
**  Returns: error code                              
**  Purpose: retrieves ID of a node with specific index
**
**  NOTE: 'id' must be able to hold MAXID characters
**----------------------------------------------------------------
*/
{
  return OW_getnodeid(en_defaultModel, index, id);
}


int  DLLEXPORT ENgetnodetype(int index, EN_NodeType *code)
/*----------------------------------------------------------------
**  Input:   index = node index                    
**  Output:  *code = node type code number (see TOOLKIT.H)
**  Returns: error code                              
**  Purpose: retrieves node type of specific node 
**----------------------------------------------------------------
*/
{
  return OW_getnodetype(en_defaultModel, index, code);
}


int DLLEXPORT ENgetcoord(int index, EN_API_FLOAT_TYPE *x, EN_API_FLOAT_TYPE *y)
/*----------------------------------------------------------------
 **  Input:   index = node index
 **  Output:  *x = value of node's coordinate
 **           *x = value of node's coordinate
 **  Returns: error code
 **  Purpose: retrieves coordinate x, y for a node
 **----------------------------------------------------------------
 */
{
  return OW_getcoord(en_defaultModel, index, x, y);
}

int DLLEXPORT ENgetnodevalue(int index, int code, EN_API_FLOAT_TYPE *value)
/*----------------------------------------------------------------
**  Input:   index = node index
**           code  = node parameter code (see TOOLKIT.H)
**  Output:  *value = value of node's parameter
**  Returns: error code                              
**  Purpose: retrieves parameter value for a node   
**----------------------------------------------------------------
*/
{
  return OW_getnodevalue(en_defaultModel, index, code, value);
}


/*
----------------------------------------------------------------
   Functions for retrieving link data
----------------------------------------------------------------
*/
   

int DLLEXPORT ENgetlinkindex(char *id, int *index)
/*----------------------------------------------------------------
**  Input:   id = link ID
**  Output:  *index = index of link in list of links
**  Returns: error code                              
**  Purpose: retrieves index of a link with specific ID 
**----------------------------------------------------------------
*/
{
  return OW_getlinkindex(en_defaultModel, id, index);
}


int DLLEXPORT ENgetlinkid(int index, char *id)
/*----------------------------------------------------------------
**  Input:   index = index of link in list of links
**  Output:  id = link ID
**  Returns: error code                              
**  Purpose: retrieves ID of a link with specific index
**
**  NOTE: 'id' must be able to hold MAXID characters
**----------------------------------------------------------------
*/
{
  return OW_getlinkid(en_defaultModel, index, id);
}


int  DLLEXPORT ENgetlinktype(int index, int *code)
/*------------------------------------------------------------------
**  Input:   index = link index                    
**  Output:  *code = link type code number (see TOOLKIT.H)
**  Returns: error code                              
**  Purpose: retrieves link type of specific link 
**------------------------------------------------------------------
*/
{
  return OW_getlinktype(en_defaultModel, index, code);
}


int  DLLEXPORT ENgetlinknodes(int index, int *node1, int *node2)
/*----------------------------------------------------------------
**  Input:   index = link index                    
**  Output:  *node1 = index of link's starting node
**           *node2 = index of link's ending node
**  Returns: error code                              
**  Purpose: retrieves end nodes of a specific link 
**----------------------------------------------------------------
*/
{
  return OW_getlinknodes(en_defaultModel, index, node1, node2);
}


int DLLEXPORT ENgetlinkvalue(int index, int code, EN_API_FLOAT_TYPE *value)
/*------------------------------------------------------------------
**  Input:   index = link index
**           code  = link parameter code (see TOOLKIT.H)                   
**  Output:  *value = value of link's parameter
**  Returns: error code                              
**  Purpose: retrieves parameter value for a link   
**------------------------------------------------------------------
*/
{
  return OW_getlinkvalue(en_defaultModel, index, code, value);
}


int  DLLEXPORT ENgetcurve(int curveIndex, char* id, int *nValues, EN_API_FLOAT_TYPE **xValues, EN_API_FLOAT_TYPE **yValues)
/*----------------------------------------------------------------
 **  Input:   curveIndex = curve index
 **  Output:  *nValues = number of points on curve
 **           *xValues = values for x
 **           *yValues = values for y
 **  Returns: error code
 **  Purpose: retrieves end nodes of a specific link
 **----------------------------------------------------------------
 */
{
  return OW_getcurve(en_defaultModel, curveIndex, id, nValues, xValues, yValues);
}


/*
----------------------------------------------------------------
   Functions for changing network data 
----------------------------------------------------------------
*/


int DLLEXPORT ENsetcontrol(int cindex, int ctype, int lindex,
              EN_API_FLOAT_TYPE setting, int nindex, EN_API_FLOAT_TYPE level)
/*----------------------------------------------------------------
**  Input:   cindex  = control index (position of control statement
**                     in the input file, starting from 1)
**           ctype   = control type code (see TOOLKIT.H)
**           lindex  = index of controlled link
**           setting = control setting applied to link
**           nindex  = index of controlling node (0 for TIMER
**                     or TIMEOFDAY control)
**           level   = control level (tank level, junction pressure,
**                     or time (seconds))
**  Output:  none
**  Returns: error code                              
**  Purpose: specifies parameters that define a simple control                 
**----------------------------------------------------------------
*/
{
  return OW_setcontrol(en_defaultModel, cindex, ctype, lindex, setting, nindex, level);
}         

    
int DLLEXPORT ENsetnodevalue(int index, int code, EN_API_FLOAT_TYPE v)
/*----------------------------------------------------------------
**  Input:   index = node index
**           code  = node parameter code (see TOOLKIT.H)
**           value = parameter value
**  Output:  none
**  Returns: error code                              
**  Purpose: sets input parameter value for a node 
**----------------------------------------------------------------
*/
{
  return OW_setnodevalue(en_defaultModel, index, code, v);
}


int DLLEXPORT ENsetlinkvalue(int index, int code, EN_API_FLOAT_TYPE v)
/*----------------------------------------------------------------
**  Input:   index = link index
**           code  = link parameter code (see TOOLKIT.H)
**           v = parameter value
**  Output:  none
**  Returns: error code                              
**  Purpose: sets input parameter value for a link 
**----------------------------------------------------------------
*/
{
  return OW_setlinkvalue(en_defaultModel, index, code, v);
}


int  DLLEXPORT  ENaddpattern(char *id)
/*----------------------------------------------------------------
**   Input:   id = ID name of the new pattern
**   Output:  none
**   Returns: error code                              
**   Purpose: adds a new time pattern appended to the end of the
**            existing patterns.
**----------------------------------------------------------------
*/
{
  return OW_addpattern(en_defaultModel, id);
}

   
int  DLLEXPORT  ENsetpattern(int index, EN_API_FLOAT_TYPE *f, int n)
/*----------------------------------------------------------------
**   Input:   index = time pattern index
**            *f    = array of pattern multipliers
**            n     = number of time periods in pattern
**   Output:  none
**   Returns: error code                              
**   Purpose: sets multipliers for a specific time pattern 
**----------------------------------------------------------------
*/
{
  return OW_setpattern(en_defaultModel, index, f, n);
}

   
int  DLLEXPORT  ENsetpatternvalue(int index, int period, EN_API_FLOAT_TYPE value)
/*----------------------------------------------------------------
**  Input:   index  = time pattern index
**           period = time pattern period
**           value  = pattern multiplier
**  Output:  none
**  Returns: error code                              
**  Purpose: sets multiplier for a specific time period and pattern 
**----------------------------------------------------------------
*/
{
  return OW_setpatternvalue(en_defaultModel, index, period, value);
}


int  DLLEXPORT  ENsettimeparam(int code, long value)
/*----------------------------------------------------------------
**  Input:   code  = time parameter code (see TOOLKIT.H)
**           value = time parameter value
**  Output:  none
**  Returns: error code                              
**  Purpose: sets value for time parameter 
**----------------------------------------------------------------
*/
{
  return OW_settimeparam(en_defaultModel, code, value);
}


int  DLLEXPORT ENsetoption(int code, EN_API_FLOAT_TYPE v)
/*----------------------------------------------------------------
**  Input:   code  = option code (see TOOLKIT.H)
**           v = option value
**  Output:  none
**  Returns: error code
**  Purpose: sets value for an analysis option
**----------------------------------------------------------------
*/
{
  return OW_setoption(en_defaultModel, code, v);
}
 

int  DLLEXPORT ENsetstatusreport(int code)
/*----------------------------------------------------------------
**  Input:   code = status reporting code (0, 1, or 2)
**  Output:  none
**  Returns: error code                              
**  Purpose: sets level of hydraulic status reporting 
**----------------------------------------------------------------
*/
{
  return OW_setstatusreport(en_defaultModel, code);
}


int  DLLEXPORT ENsetqualtype(int qualcode, char *chemname,
                               char *chemunits, char *tracenode)
/*----------------------------------------------------------------
**  Input:   qualcode  = WQ parameter code (see TOOLKIT.H)
**           chemname  = name of WQ constituent 
**           chemunits = concentration units of WQ constituent
**           tracenode = ID of node being traced
**  Output:  none
**  Returns: error code                              
**  Purpose: sets type of quality analysis called for
**
**  NOTE: chemname and chemunits only apply when WQ analysis
**        is for chemical. tracenode only applies when WQ
**        analysis is source tracing.
**----------------------------------------------------------------
*/
{
  return OW_setqualtype(en_defaultModel, qualcode, chemname, chemunits, tracenode);
}



int  DLLEXPORT ENgetnumdemands(int nodeIndex, int *numDemands)
{
  return OW_getnumdemands(en_defaultModel, nodeIndex, numDemands);
}

int  DLLEXPORT ENgetbasedemand(int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE *baseDemand)
{
  return OW_getbasedemand(en_defaultModel, nodeIndex, demandIdx, baseDemand);
}

int  DLLEXPORT ENsetbasedemand(int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE baseDemand)
{
  return OW_setbasedemand(en_defaultModel, nodeIndex, demandIdx, baseDemand);
}

int  DLLEXPORT ENgetdemandpattern(int nodeIndex, int demandIdx, int *pattIdx)
{
  return OW_getdemandpattern(en_defaultModel, nodeIndex, demandIdx, pattIdx);
}

int DLLEXPORT ENgetaveragepatternvalue(int index, EN_API_FLOAT_TYPE *value)
/*----------------------------------------------------------------
 **  Input:   index  = index of time pattern
 **           period = pattern time period
 **  Output:  *value = pattern multiplier
 **  Returns: error code
 **  Purpose: retrieves multiplier for a specific time period
 **           and pattern
 **----------------------------------------------------------------
 */
{
  return OW_getaveragepatternvalue(en_defaultModel, index, value);
}




//====================================================//
//=== end of <=2.00.13 api functions           =======//
//====================================================//





/*
----------------------------------------------------------------
   Functions for opening files 
----------------------------------------------------------------
*/


int   openfiles(OW_Project *m, char *f1, char *f2, char *f3)
/*----------------------------------------------------------------
**  Input:   f1 = pointer to name of input file                  
**           f2 = pointer to name of report file                 
**           f3 = pointer to name of binary output file          
**  Output:  none
**  Returns: error code                                  
**  Purpose: opens input & report files                          
**----------------------------------------------------------------
*/
{
/* Initialize file pointers to NULL */
   m->InFile = NULL;
   m->RptFile = NULL;
   m->OutFile = NULL;
   m->HydFile = NULL;

/* Save file names */
   strncpy(m->InpFname,f1,MAXFNAME);
   strncpy(m->Rpt1Fname,f2,MAXFNAME);
   strncpy(m->OutFname,f3,MAXFNAME);
   if (strlen(f3) > 0)
     m->Outflag = SAVE;                                         //(2.00.12 - LR)
   else
     m->Outflag = SCRATCH;                                                     //(2.00.12 - LR)

/* Check that file names are not identical */
   if (strcomp(f1,f2) || strcomp(f1,f3) || (strcomp(f2,f3) && (strlen(f2) > 0 || strlen(f3) > 0)))
   {
      writecon(FMT04);
      return(301);
   }

/* Attempt to open input and report files */
   if ((m->InFile = fopen(f1,"rt")) == NULL)
   {
      writecon(FMT05);
      writecon(f1);
      return(302);
   }
   if (strlen(f2) == 0) m->RptFile = stdout;
   else if ((m->RptFile = fopen(f2,"wt")) == NULL)
   {
      writecon(FMT06);
      return(303);
   }

   return(0);
}                                       /* End of openfiles */


int  openhydfile(OW_Project *m)
/*----------------------------------------------------------------
** Input:   none
** Output:  none
** Returns: error code
** Purpose: opens file that saves hydraulics solution
**----------------------------------------------------------------
*/
{
   INT4 nsize[6];                       /* Temporary array */
   INT4 magic;
   INT4 version;
   int errcode = 0;

/* If HydFile currently open, then close it if its not a scratch file */
   if (m->HydFile != NULL)
   {
      if (m->Hydflag == SCRATCH) {
        return(0);
      }
      fclose(m->HydFile);
   }
      
/* Use Hydflag to determine the type of hydraulics file to use. */
/* Write error message if the file cannot be opened.            */
   m->HydFile = NULL;
   switch(m->Hydflag)
   {
     case SCRATCH:
       getTmpName(m->HydFname);                                     //(2.00.12 - LR)
       m->HydFile = fopen(m->HydFname, "w+b");                         //(2.00.12 - LR)
       break;
     case SAVE:
       m->HydFile = fopen(m->HydFname,"w+b");
       break;
     case USE:
       m->HydFile = fopen(m->HydFname,"rb");
       break;
   }
   if (m->HydFile == NULL) return(305);

/* If a previous hydraulics solution is not being used, then */
/* save the current network size parameters to the file.     */
   if (m->Hydflag != USE)
   {
      magic = MAGICNUMBER;
      version = VERSION;
      nsize[0] = m->Nnodes;
      nsize[1] = m->Nlinks;
      nsize[2] = m->Ntanks;
      nsize[3] = m->Npumps;
      nsize[4] = m->Nvalves;
      nsize[5] = (int)m->Dur;
      fwrite(&magic,   sizeof(INT4), 1, m->HydFile);
      fwrite(&version, sizeof(INT4), 1, m->HydFile);
      fwrite(nsize,    sizeof(INT4), 6, m->HydFile);
   }

/* If a previous hydraulics solution is being used, then */
/* make sure its network size parameters match those of  */
/* the current network.                                  */
   if (m->Hydflag == USE)
   {
      fread(&magic, sizeof(INT4), 1, m->HydFile);
      if (magic != MAGICNUMBER)
        return(306);
      fread(&version,sizeof(INT4), 1, m->HydFile);
      if (version != VERSION)
        return(306);
      if (fread(nsize,sizeof(INT4), 6, m->HydFile) < 6)
        return(306);
      if (nsize[0] != m->Nnodes  || nsize[1] != m->Nlinks ||
          nsize[2] != m->Ntanks  || nsize[3] != m->Npumps ||
          nsize[4] != m->Nvalves || nsize[5] != m->Dur)
        return(306);
     
      m->SaveHflag = TRUE;
   }

/* Save current position in hydraulics file  */
/* where storage of hydraulic results begins */
   m->HydOffset = ftell(m->HydFile);
   return(errcode);
}


int  openoutfile(OW_Project *m)
/*----------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Returns: error code
**  Purpose: opens binary output file.
**----------------------------------------------------------------
*/
{
   int errcode = 0;

/* Close output file if already opened */
   if (m->OutFile != NULL)
     fclose(m->OutFile);
   m->OutFile = NULL;
   if (m->TmpOutFile != NULL)
     fclose(m->TmpOutFile);
   m->TmpOutFile = NULL;

   if (m->Outflag == SCRATCH)
     remove(m->OutFname);                                   //(2.00.12 - LR)
   remove(m->TmpFname);                                                           //(2.00.12 - LR)

/* If output file name was supplied, then attempt to */
/* open it. Otherwise open a temporary output file.  */
   //if (strlen(OutFname) != 0)                                                //(2.00.12 - LR)
   if (m->Outflag == SAVE)                                                        //(2.00.12 - LR)
   {
      if ( (m->OutFile = fopen(m->OutFname,"w+b")) == NULL)
      {
         writecon(FMT07);
         errcode = 304;
      }
   }
   //else if ( (OutFile = tmpfile()) == NULL)                                  //(2.00.12 - LR)
   else                                                                        //(2.00.12 - LR)
   {
      getTmpName(m->OutFname);                                                    //(2.00.12 - LR)
      if ( (m->OutFile = fopen(m->OutFname,"w+b")) == NULL)                          //(2.00.12 - LR)
	  {
         writecon(FMT08);
         errcode = 304;
	  }
   }

/* Save basic network data & energy usage results */
   ERRCODE(savenetdata(m));
   m->OutOffset1 = ftell(m->OutFile);
   ERRCODE(saveenergy(m));
   m->OutOffset2 = ftell(m->OutFile);

/* Open temporary file if computing time series statistic */
   if (!errcode)
   {
      if (m->Tstatflag != SERIES)
      {
         //if ( (TmpOutFile = tmpfile()) == NULL) errcode = 304;               //(2.00.12 - LR)
         getTmpName(m->TmpFname);                                                 //(2.00.12 - LR)
         m->TmpOutFile = fopen(m->TmpFname, "w+b");                                  //(2.00.12 - LR)
         if (m->TmpOutFile == NULL) errcode = 304;                                //(2.00.12 - LR)
      }
      else m->TmpOutFile = m->OutFile;
   }
   return(errcode);
}


/*
----------------------------------------------------------------
   Global memory management functions 
----------------------------------------------------------------
*/


void initpointers(OW_Project *m)
/*----------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: initializes global pointers to NULL
**----------------------------------------------------------------
*/
{
   m->hydraulics.NodeDemand        = NULL;
   m->NodeQual = NULL;
   m->hydraulics.NodeHead        = NULL;
   m->hydraulics.LinkFlows        = NULL;
   m->PipeRateCoeff        = NULL;
   m->hydraulics.LinkStatus        = NULL;
   m->hydraulics.LinkSetting        = NULL;
   m->hydraulics.OldStat  = NULL;

   m->Node     = NULL;
   m->Link     = NULL;
   m->Tank     = NULL;
   m->Pump     = NULL;
   m->Valve    = NULL;
   m->Pattern  = NULL;
   m->Curve    = NULL;
   m->Control  = NULL;
   m->Coord    = NULL;

   m->X        = NULL;
   m->Patlist  = NULL;
   m->Curvelist = NULL;
   m->Coordlist = NULL;
   m->Adjlist  = NULL;
   m->hydraulics.solver.Aii      = NULL;
   m->hydraulics.solver.Aij      = NULL;
   m->hydraulics.solver.F        = NULL;
   m->hydraulics.solver.P        = NULL;
   m->hydraulics.solver.Y        = NULL;
   m->hydraulics.solver.Order    = NULL;
   m->hydraulics.solver.Row      = NULL;
   m->hydraulics.solver.Ndx      = NULL;
   m->hydraulics.solver.XLNZ     = NULL;
   m->hydraulics.solver.NZSUB    = NULL;
   m->hydraulics.solver.LNZ      = NULL;
   m->NodeHashTable      = NULL;
   m->LinkHashTable      = NULL;
   initrules(m);
}


int  allocdata(OW_Project *m)
/*----------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Returns: error code
**  Purpose: allocates memory for network data structures
**----------------------------------------------------------------
*/
{
   int n;
   int errcode = 0;

/* Allocate node & link ID hash tables */
   m->NodeHashTable = ENHashTableCreate();
   m->LinkHashTable = ENHashTableCreate();
   ERRCODE(MEMCHECK(m->NodeHashTable));
   ERRCODE(MEMCHECK(m->LinkHashTable));

/* Allocate memory for network nodes */
/*************************************************************
 NOTE: Because network components of a given type are indexed
       starting from 1, their arrays must be sized 1
       element larger than the number of components.
*************************************************************/
   if (!errcode)
   {
      n = m->MaxNodes + 1;
      m->Node = (Snode *)  calloc(n, sizeof(Snode));
      m->hydraulics.NodeDemand   = (double *) calloc(n, sizeof(double));
      m->NodeQual = (double *) calloc(n, sizeof(double));
      m->hydraulics.NodeHead    = (double *) calloc(n, sizeof(double));
      ERRCODE(MEMCHECK(m->Node));
      ERRCODE(MEMCHECK(m->hydraulics.NodeDemand));
      ERRCODE(MEMCHECK(m->NodeQual));
      ERRCODE(MEMCHECK(m->hydraulics.NodeHead));
   }

/* Allocate memory for network links */
   if (!errcode)
   {
      n = m->MaxLinks + 1;
      m->Link = (Slink *) calloc(n, sizeof(Slink));
      m->hydraulics.LinkFlows    = (double *) calloc(n, sizeof(double));
      m->hydraulics.LinkSetting    = (double *) calloc(n, sizeof(double));
      m->hydraulics.LinkStatus    = (char  *) calloc(n, sizeof(char));
      ERRCODE(MEMCHECK(m->Link));
      ERRCODE(MEMCHECK(m->hydraulics.LinkFlows));
      ERRCODE(MEMCHECK(m->hydraulics.LinkSetting));
      ERRCODE(MEMCHECK(m->hydraulics.LinkStatus));
   } 

/* Allocate memory for tanks, sources, pumps, valves,   */
/* controls, demands, time patterns, & operating curves */
   if (!errcode)
   {
      m->Tank    = (Stank *)    calloc(m->MaxTanks+1,   sizeof(Stank));
      m->Pump    = (Spump *)    calloc(m->MaxPumps+1,   sizeof(Spump));
      m->Valve   = (Svalve *)   calloc(m->MaxValves+1,  sizeof(Svalve));
      m->Control = (Scontrol *) calloc(m->MaxControls+1,sizeof(Scontrol));
      m->Pattern = (Spattern *) calloc(m->MaxPats+1,    sizeof(Spattern));
      m->Curve   = (Scurve *)   calloc(m->MaxCurves+1,  sizeof(Scurve));
      m->Coord   = (Scoord *)   calloc(m->MaxNodes+1,   sizeof(Scoord));
      ERRCODE(MEMCHECK(m->Tank));
      ERRCODE(MEMCHECK(m->Pump));
      ERRCODE(MEMCHECK(m->Valve));
      ERRCODE(MEMCHECK(m->Control));
      ERRCODE(MEMCHECK(m->Pattern));
      ERRCODE(MEMCHECK(m->Curve));
      ERRCODE(MEMCHECK(m->Coord));
   }

/* Initialize pointers used in patterns, curves, and demand category lists */
   if (!errcode)
   {
      for (n=0; n <= m->MaxPats; n++)
      {
         m->Pattern[n].Length = 0;
         m->Pattern[n].F = NULL;
      }
      for (n=0; n <= m->MaxCurves; n++)
      {
         m->Curve[n].Npts = 0;
         m->Curve[n].Type = -1;
         m->Curve[n].X = NULL;
         m->Curve[n].Y = NULL;
      }
     
     for (n=0; n <= m->MaxNodes; n++)
     {
       // node demand
       m->Node[n].D = NULL;
       /* Allocate memory for coord data */
       m->Coord[n].X = (double *) calloc(1, sizeof(double));
       m->Coord[n].Y = (double *) calloc(1, sizeof(double));
       if (m->Coord[n].X == NULL || m->Coord[n].Y == NULL) {
         return(101);
       }
       m->Coord[n].X[0] = 0;
       m->Coord[n].Y[0] = 0;
     }
     
   }

/* Allocate memory for rule base (see RULES.C) */
   if (!errcode) {
     errcode = allocrules(m);
   }
   return(errcode);
}                                       /* End of allocdata */


void  freeTmplist(STmplist *t)
/*----------------------------------------------------------------
**  Input:   t = pointer to start of a temporary list
**  Output:  none
**  Purpose: frees memory used for temporary storage
**           of pattern & curve data
**----------------------------------------------------------------
*/
{
   STmplist   *tnext;
   while (t != NULL)
   {
       tnext = t->next;
       freeFloatlist(t->x);
       freeFloatlist(t->y);
       free(t);
       t = tnext;
   }
}


void  freeFloatlist(SFloatlist *f)
/*----------------------------------------------------------------
**  Input:   f = pointer to start of list of floats
**  Output:  none
**  Purpose: frees memory used for storing list of floats
**----------------------------------------------------------------
*/
{
   SFloatlist *fnext;
   while (f != NULL)
   {
      fnext = f->next;
      free(f);
      f = fnext;
   }
}


void  freedata(OW_Project *m)
/*----------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: frees memory allocated for network data structures.        
**----------------------------------------------------------------
*/
{
    int j;
    Pdemand demand, nextdemand;
    Psource source;

/* Free memory for computed results */
    free(m->hydraulics.NodeDemand);
    free(m->NodeQual);
    free(m->hydraulics.NodeHead);
    free(m->hydraulics.LinkFlows);
    free(m->hydraulics.LinkSetting);
    free(m->hydraulics.LinkStatus);

/* Free memory for node data */
    if (m->Node != NULL)
    {
      for (j=0; j <= m->MaxNodes; j++)
      {
      /* Free memory used for demand category list */
         demand = m->Node[j].D;
         while (demand != NULL)
         {
            nextdemand = demand->next;
            free(demand);
            demand = nextdemand;
         }
      /* Free memory used for WQ source data */
         source = m->Node[j].S;
         if (source != NULL) {
           free(source);
         }
      }
      free(m->Node);
    }

/* Free memory for other network objects */
    free(m->Link);
    free(m->Tank);
    free(m->Pump);
    free(m->Valve);
    free(m->Control);

/* Free memory for time patterns */
    if (m->Pattern != NULL)
    {
       for (j=0; j <= m->MaxPats; j++) {
         free(m->Pattern[j].F);
       }
       free(m->Pattern);
    }

/* Free memory for curves */
    if (m->Curve != NULL)
    {
       for (j=0; j <= m->MaxCurves; j++)
       {
          free(m->Curve[j].X);
          free(m->Curve[j].Y);
       }
       free(m->Curve);
    }
  
  // free coordinates
  if (m->Coord != NULL) {
    for (j = 0; j <= m->MaxCoords; j++) {
      free(m->Coord[j].X);
      free(m->Coord[j].Y);
    }
    free(m->Coord);
  }

/* Free memory for rule base (see RULES.C) */
    freerules(m);

/* Free hash table memory */
    if (m->NodeHashTable != NULL) ENHashTableFree(m->NodeHashTable);
    if (m->LinkHashTable != NULL) ENHashTableFree(m->LinkHashTable);
  
  return;
}


/*
----------------------------------------------------------------
   General purpose functions 
----------------------------------------------------------------
*/

/*** New function for 2.00.12 ***/                                             //(2.00.12 - LR)
char* getTmpName(char* fname)
//
//  Input:   fname = file name string
//  Output:  returns pointer to file name
//  Purpose: creates a temporary file name with path prepended to it.
//
{
    char name[MAXFNAME+1];
    int  n;

    // --- for Windows systems:
    #ifdef WINDOWS
      // --- use system function tmpnam() to create a temporary file name
      tmpnam(name);

      // --- if user supplied the name of a temporary directory,
      //     then make it be the prefix of the full file name
      n = (int)strlen(TmpDir);
      if ( n > 0 )
      {
          strcpy(fname, TmpDir);
          if ( fname[n-1] != '\\' ) strcat(fname, "\\");
      }

      // --- otherwise, use the relative path notation as the file name
      //     prefix so that the file will be placed in the current directory
      else
      {
          strcpy(fname, ".\\");
      }

      // --- now add the prefix to the file name
      strcat(fname, name);

    // --- for non-Windows systems:
    #else
      // --- use system function mkstemp() to create a temporary file name
      strcpy(fname, "enXXXXXX");
      mkstemp(fname);
    #endif
    return fname;
}


int  strcomp(char *s1, char *s2)
/*---------------------------------------------------------------
**  Input:   s1 = character string
**           s2 = character string
**  Output:  none
**  Returns: 1 if s1 is same as s2, 0 otherwise
**  Purpose: case insensitive comparison of strings s1 & s2
**---------------------------------------------------------------
*/
{
   int i;
   for (i=0; UCHAR(s1[i]) == UCHAR(s2[i]); i++)
     if (!s1[i+1] && !s2[i+1]) return(1);
   return(0);
}                                       /*  End of strcomp  */


double  interp(int n, double x[], double y[], double xx)
/*----------------------------------------------------------------
**  Input:   n  = number of data pairs defining a curve
**           x  = x-data values of curve
**           y  = y-data values of curve
**           xx = specified x-value
**  Output:  none
**  Returns: y-value on curve at x = xx
**  Purpose: uses linear interpolation to find y-value on a
**           data curve corresponding to specified x-value.
**  NOTE:    does not extrapolate beyond endpoints of curve.
**----------------------------------------------------------------
*/
{
    int    k,m;
    double  dx,dy;

    m = n - 1;                          /* Highest data index      */
    if (xx <= x[0]) return(y[0]);       /* xx off low end of curve */
    for (k=1; k<=m; k++)                /* Bracket xx on curve     */
    {
        if (x[k] >= xx)                 /* Interp. over interval   */
        {
            dx = x[k]-x[k-1];
            dy = y[k]-y[k-1];
            if (ABS(dx) < TINY) return(y[k]);
            else return(y[k] - (x[k]-xx)*dy/dx);
        }
    }
    return(y[m]);                       /* xx off high end of curve */
}                       /* End of interp */


int   findnode(OW_Project *m, char *id)
/*----------------------------------------------------------------
**  Input:   id = node ID
**  Output:  none
**  Returns: index of node with given ID, or 0 if ID not found
**  Purpose: uses hash table to find index of node with given ID
**----------------------------------------------------------------
*/
{
   return(ENHashTableFind(m->NodeHashTable,id));
}


int  findlink(OW_Project *m, char *id)
/*----------------------------------------------------------------
**  Input:   id = link ID
**  Output:  none
**  Returns: index of link with given ID, or 0 if ID not found
**  Purpose: uses hash table to find index of link with given ID
**----------------------------------------------------------------
*/
{
   return(ENHashTableFind(m->LinkHashTable,id));
}


void geterrmsg(int errcode, char *msgOut)
/*----------------------------------------------------------------
**  Input:   errcode = error code
**  Output:  none
**  Returns: pointer to string with error message
**  Purpose: retrieves text of error message
**----------------------------------------------------------------
*/
{
  
   switch (errcode)
   {                                   /* Warnings */
/*
      case 1:     strcpy(Msg,WARN1);   break;
      case 2:     strcpy(Msg,WARN2);   break;
      case 3:     strcpy(Msg,WARN3);   break;
      case 4:     strcpy(Msg,WARN4);   break;
      case 5:     strcpy(Msg,WARN5);   break;
      case 6:     strcpy(Msg,WARN6);   break;
*/      
                                       /* System Errors */
      case 101:   strcpy(msgOut,ERR101);  break;
      case 102:   strcpy(msgOut,ERR102);  break;
      case 103:   strcpy(msgOut,ERR103);  break;
      case 104:   strcpy(msgOut,ERR104);  break;
      case 105:   strcpy(msgOut,ERR105);  break;
      case 106:   strcpy(msgOut,ERR106);  break;
      case 107:   strcpy(msgOut,ERR107);  break;
      case 108:   strcpy(msgOut,ERR108);  break;
      case 109:   strcpy(msgOut,ERR109);  break;
      case 110:   strcpy(msgOut,ERR110);  break;
      case 120:   strcpy(msgOut,ERR120);  break;

                                       /* Input Errors */
      case 200:  strcpy(msgOut,ERR200);   break;
      case 223:  strcpy(msgOut,ERR223);   break;
      case 224:  strcpy(msgOut,ERR224);   break;

                                       /* Toolkit function errors */
      case 202:  sprintf(msgOut,ERR202,t_FUNCCALL,""); break;
      case 203:  sprintf(msgOut,ERR203,t_FUNCCALL,""); break;
      case 204:  sprintf(msgOut,ERR204,t_FUNCCALL,""); break;
      case 205:  sprintf(msgOut,ERR205,t_FUNCCALL,""); break;
      case 207:  sprintf(msgOut,ERR207,t_FUNCCALL,""); break;
      case 240:  sprintf(msgOut,ERR240,t_FUNCCALL,""); break;
      case 241:  sprintf(msgOut,ERR241,t_FUNCCALL,""); break;
      case 250:  sprintf(msgOut,ERR250);  break;
      case 251:  sprintf(msgOut,ERR251);  break;

                                       /* File Errors */
      case 301:  strcpy(msgOut,ERR301);   break;
      case 302:  strcpy(msgOut,ERR302);   break;
      case 303:  strcpy(msgOut,ERR303);   break;
      case 304:  strcpy(msgOut,ERR304);   break;
      case 305:  strcpy(msgOut,ERR305);   break;
      case 306:  strcpy(msgOut,ERR306);   break;
      case 307:  strcpy(msgOut,ERR307);   break;
      case 308:  strcpy(msgOut,ERR308);   break;
      case 309:  strcpy(msgOut,ERR309);   break;

	  case 401:  strcpy(msgOut,ERR401); break;
      default:   strcpy(msgOut,"");
   }
   return;
}


void  errmsg(OW_Project *m, int errcode)
/*----------------------------------------------------------------
**  Input:   errcode = error code
**  Output:  none
**  Purpose: writes error message to report file
**----------------------------------------------------------------
*/
{
   char msg[MAXMSG+1];
   if (errcode == 309)    /* Report file write error -  */
   {                      /* Do not write msg to file.  */
      writecon("\n  ");
      geterrmsg(errcode, msg);
      writecon(msg);
   }      
   else if (m->RptFile != NULL && m->Messageflag)
   {
      geterrmsg(errcode, msg);
      writeline(m,msg);
   }
}


void  writecon(char *s)
/*----------------------------------------------------------------
**  Input:   text string                                         
**  Output:  none                                                
**  Purpose: writes string of characters to console              
**----------------------------------------------------------------
*/
{
#ifdef CLE                                                                     //(2.00.11 - LR)
   fprintf(stdout,s);
   fflush(stdout);
#endif
}


void writewin(char *s)
/*----------------------------------------------------------------
**  Input:   text string                                         
**  Output:  none                                                
**  Purpose: passes character string to viewprog() in
**           application which calls the EPANET DLL 
**----------------------------------------------------------------
*/
{
#ifdef DLL
   char progmsg[MAXMSG+1];
   if (viewprog != NULL)
   {
      strncpy(progmsg,s,MAXMSG);
      viewprog(progmsg);
   }
#endif
}


/*************************** END OF EPANET.C ***************************/


// ================================================================//
//   open water analytics expanded methods below                   //
// ================================================================//

int  DLLEXPORT OW_open(char *inpFile, OW_Project **modelOut, char *rptFile, char *binOutFile)
{
  
  *modelOut = 0;
  OW_Project *m = calloc(1, sizeof(OW_Project));
  
  
  int  errcode = 0;
  
  /*** Updated 9/7/00 ***/
  /* Reset math coprocessor */
#ifdef DLL
  _fpreset();
#endif
  
  /* Set system flags */
  m->Openflag  = FALSE;
  m->OpenHflag = FALSE;
  m->OpenQflag = FALSE;
  m->SaveHflag = FALSE;
  m->SaveQflag = FALSE;
  m->Warnflag  = FALSE;
  
  /*** Updated 9/7/00 ***/
  m->Messageflag = TRUE;
  
  /* If binary output file being used, then   */
  /* do not write full results to Report file */
  /* (use it only for status reports).        */
  m->Rptflag = 0;
  if (strlen(binOutFile) == 0) {
    m->Rptflag = 1;
  }
  
  /*** Updated 9/7/00 ***/
  /*** Previous code segment ignored. ***/
  /*** Rptflag now always set to 1.   ***/
  m->Rptflag = 1;
  
  /* Initialize global pointers to NULL. */
  initpointers(m);
  
  /* Open input & report files */
  ERRCODE( openfiles(m, inpFile, rptFile, binOutFile) );
  if (errcode > 0)
  {
    errmsg(m, errcode);
    return(errcode);
  }
  writelogo(m);
  
  /* Find network size & allocate memory for data */
  writecon(FMT02);
  writewin(FMT100);
  ERRCODE(netsize(m));
  ERRCODE(allocdata(m));
  
  /* Retrieve input data */
  ERRCODE(getdata(m));
  
  /* Free temporary linked lists used for Patterns & Curves */
  freeTmplist(m->Patlist);
  freeTmplist(m->Curvelist);
  freeTmplist(m->Coordlist);
  
  /* If using previously saved hydraulics then open its file */
  if (m->Hydflag == USE) {
    ERRCODE(openhydfile(m));
  }
  
  /* Write input summary to report file */
  if (!errcode)
  {
    if (m->Summaryflag) writesummary(m);
    writetime(m,FMT104);
    m->Openflag = TRUE;
  }
  else errmsg(m, errcode);
  
  *modelOut = m;
  
  return(errcode);
}

int  DLLEXPORT OW_saveinpfile(OW_Project *m, char *filename)
{
  if (!m->Openflag) return(102);
  return(saveinpfile(m, filename));
}


int  DLLEXPORT OW_close(OW_Project *m)
{
  if (!m) {
    return 0;
  }
  if (m->Openflag)
    writetime(m,FMT105);
  
  if (!m->Openflag) {
    // not actually open.
    return 0;
  }
  
  freedata(m);
  
  if (m->TmpOutFile != m->OutFile)                                                  //(2.00.12 - LR)
  {                                                                           //(2.00.12 - LR)
    if (m->TmpOutFile != NULL) {
      fclose(m->TmpOutFile);                              //(2.00.12 - LR)
    }
    m->TmpOutFile=NULL;
    remove(m->TmpFname);                                                        //(2.00.12 - LR)
  }                                                                           //(2.00.12 - LR)
  
  if (m->InFile  != NULL) {
    fclose(m->InFile);
    m->InFile=NULL;
  }
  if (m->RptFile != NULL) {
    fclose(m->RptFile);
    m->RptFile=NULL;
  }
  if (m->HydFile != NULL) {
    fclose(m->HydFile);
    m->HydFile=NULL;
  }
  if (m->OutFile != NULL) {
    fclose(m->OutFile);
    m->OutFile=NULL;
  }
  
  if (m->Hydflag == SCRATCH) {
    remove(m->HydFname);                                   //(2.00.12 - LR)
  }
  if (m->Outflag == SCRATCH) {
    remove(m->OutFname);                                   //(2.00.12 - LR)
  }
  m->Openflag  = FALSE;
  m->OpenHflag = FALSE;
  m->SaveHflag = FALSE;
  m->OpenQflag = FALSE;
  m->SaveQflag = FALSE;
  
  free(en_defaultModel);
  
  return(0);
}



int  DLLEXPORT OW_solveH(OW_Project *m)
{
  int  errcode;
  long t, tstep;
  
  /* Open hydraulics solver */
  errcode = OW_openH(m);
  if (!errcode)
  {
    /* Initialize hydraulics */
    errcode = OW_initH(m, EN_SAVE);
    writecon(FMT14);
    
    /* Analyze each hydraulic period */
    if (!errcode) do
    {
      
      /* Display progress message */
      
      /*** Updated 6/24/02 ***/
      sprintf(m->Msg,"%-10s",clocktime(m->Atime,m->Htime));
      
      writecon(m->Msg);
      sprintf(m->Msg,FMT101,m->Atime);
      writewin(m->Msg);
      
      /* Solve for hydraulics & advance to next time period */
      tstep = 0;
      ERRCODE(OW_runH(m, &t));
      ERRCODE(OW_nextH(m, &tstep));
      
      /*** Updated 6/24/02 ***/
      writecon("\b\b\b\b\b\b\b\b\b\b");
    }
    while (tstep > 0);
  }
  
  /* Close hydraulics solver */
  
  /*** Updated 6/24/02 ***/
  writecon("\b\b\b\b\b\b\b\b                     ");
  
  OW_closeH(m);
  errcode = MAX(errcode, m->Warnflag);
  return(errcode);
}


int  DLLEXPORT OW_saveH(OW_Project *m)
{
  char tmpflag;
  int  errcode;
  
  /* Check if hydraulic results exist */
  if (!m->SaveHflag) return(104);
  
  /* Temporarily turn off WQ analysis */
  tmpflag = m->Qualflag;
  m->Qualflag = NONE;
  
  /* Call WQ solver to simply transfer results */
  /* from Hydraulics file to Output file at    */
  /* fixed length reporting time intervals.    */
  errcode = OW_solveQ(m);
  
  /* Restore WQ analysis option */
  m->Qualflag = tmpflag;
  if (errcode) {
    errmsg(m, errcode);
  }
  return(errcode);
}


int  DLLEXPORT OW_openH(OW_Project *m)
{
  int  errcode = 0;
  
  /* Check that input data exists */
  m->OpenHflag = FALSE;
  m->SaveHflag = FALSE;
  if (!m->Openflag) {
    return(102);
  }
  
  /* Check that previously saved hydraulics file not in use */
  if (m->Hydflag == USE) {
    return(107);
  }
  
  /* Open hydraulics solver */
  ERRCODE(openhyd(m));
  if (!errcode) {
    m->OpenHflag = TRUE;
  }
  else {
    errmsg(m, errcode);
  }
  return(errcode);
}


int  DLLEXPORT OW_initH(OW_Project *m, int flag)
{
  int errcode = 0;
  int sflag, fflag;
  
  /* Reset status flags */
  m->SaveHflag = FALSE;
  m->Warnflag = FALSE;
  
  /* Get values of save-to-file flag and reinitialize-flows flag */
  fflag = flag/EN_INITFLOW;
  sflag = flag - fflag*EN_INITFLOW;
  
  /* Check that hydraulics solver was opened */
  if (!m->OpenHflag) return(103);
  
  /* Open hydraulics file */
  m->Saveflag = FALSE;
  if (sflag > 0)
  {
    errcode = openhydfile(m);
    if (!errcode) {
      m->Saveflag = TRUE;
    }
    else {
      errmsg(m, errcode);
    }
  }
  
  /* Initialize hydraulics */
  inithyd(m,fflag);
  if (m->Statflag > 0) writeheader(m,STATHDR,0);
  return(errcode);
}


int  DLLEXPORT OW_runH(OW_Project *m, long *t)
{
  int errcode;
  *t = 0;
  if (!m->OpenHflag) {
    return(103);
  }
  errcode = runhyd(m,t);
  if (errcode) {
    errmsg(m, errcode);
  }
  return(errcode);
}


int  DLLEXPORT OW_nextH(OW_Project *m, long *tstep)
{
  int errcode;
  *tstep = 0;
  if (!m->OpenHflag) return(103);
  errcode = nexthyd(m, tstep);
  if (errcode) {
    errmsg(m, errcode);
  }
  else if (m->Saveflag && *tstep == 0) {
    m->SaveHflag = TRUE;
  }
  return(errcode);
}


int  DLLEXPORT OW_closeH(OW_Project *m)
{
  if (!m->Openflag) return(102);
  closehyd(m);
  m->OpenHflag = FALSE;
  return(0);
}


int  DLLEXPORT OW_savehydfile(OW_Project *m, char *filename)
{
  
  FILE *f;
  int   c;
  
  /* Check that hydraulics results exist */
  if (m->HydFile == NULL || !m->SaveHflag) return(104);
  
  /* Open file */
  if ( (f = fopen(filename,"w+b")) == NULL) return(305);
  
  /* Copy from HydFile to f */
  fseek(m->HydFile, 0, SEEK_SET);
  while ( (c = fgetc(m->HydFile)) != EOF) {
    fputc(c, f);
  }
  fclose(f);
  return(0);
}


int  DLLEXPORT OW_usehydfile(OW_Project *m, char *filename)
{
  int errcode;
  
  /* Check that input data exists & hydraulics system closed */
  if (!m->Openflag) return(102);
  if (m->OpenHflag) return(108);
  
  /* Try to open hydraulics file */
  strncpy(m->HydFname, filename, MAXFNAME);
  m->Hydflag = USE;
  m->SaveHflag = TRUE;
  errcode = openhydfile(m);
  
  /* If error, then reset flags */
  if (errcode)
  {
    strcpy(m->HydFname, "");
    m->Hydflag = SCRATCH;
    m->SaveHflag = FALSE;
  }
  return(errcode);
}



int  DLLEXPORT OW_solveQ(OW_Project *m)
{
  int  errcode;
  long t, tstep;
  
  /* Open WQ solver */
  errcode = OW_openQ(m);
  if (!errcode)
  {
    /* Initialize WQ */
    errcode = OW_initQ(m, EN_SAVE);
    if (m->Qualflag) writecon(FMT15);
    else
    {
      writecon(FMT16);
      writewin(FMT103);
    }
    
    /* Analyze each hydraulic period */
    if (!errcode) do
    {
      
      /* Display progress message */
      
      /*** Updated 6/24/02 ***/
      sprintf(m->Msg,"%-10s",clocktime(m->Atime,m->Htime));
      
      writecon(m->Msg);
      if (m->Qualflag)
      {
        sprintf(m->Msg, FMT102, m->Atime);
        writewin(m->Msg);
      }
      
      /* Retrieve current network solution & update WQ to next time period */
      tstep = 0;
      ERRCODE(OW_runQ(m, &t));
      ERRCODE(OW_nextQ(m, &tstep));
      
      /*** Updated 6/24/02 ***/
      writecon("\b\b\b\b\b\b\b\b\b\b");
      
    }  while (tstep > 0);
    
  }
  
  /* Close WQ solver */
  
  /*** Updated 6/24/02 ***/
  writecon("\b\b\b\b\b\b\b\b                     ");
  OW_closeQ(m);
  return(errcode);
}


int  DLLEXPORT OW_openQ(OW_Project *m)
{
  int errcode = 0;
  
  /* Check that hydraulics results exist */
  m->OpenQflag = FALSE;
  m->SaveQflag = FALSE;
  if (!m->Openflag) {
    return(102);
  }
  // !LT! todo - check for SaveHflag / set sequential/step mode
  //if (!SaveHflag) return(104);
  
  /* Open WQ solver */
  ERRCODE(openqual(m));
  if (!errcode) {
    m->OpenQflag = TRUE;
  }
  else {
    errmsg(m, errcode);
  }
  return(errcode);
}


int  DLLEXPORT OW_initQ(OW_Project *m, int saveflag)
{
  int errcode = 0;
  if (!m->OpenQflag) return(105);
  initqual(m);
  m->SaveQflag = FALSE;
  m->Saveflag = FALSE;
  if (saveflag)
  {
    errcode = openoutfile(m);
    if (!errcode) {
      m->Saveflag = TRUE;
    }
  }
  return(errcode);
}


int  DLLEXPORT OW_runQ(OW_Project *m, long *t)
{
  int errcode;
  *t = 0;
  if (!m->OpenQflag) {
    return(105);
  }
  errcode = runqual(m, t);
  if (errcode) {
    errmsg(m, errcode);
  }
  return(errcode);
}


int  DLLEXPORT OW_nextQ(OW_Project *m, long *tstep)
{
  int errcode;
  *tstep = 0;
  if (!m->OpenQflag) {
    return(105);
  }
  errcode = nextqual(m, tstep);
  if (!errcode && m->Saveflag && *tstep == 0) {
    m->SaveQflag = TRUE;
  }
  if (errcode) {
    errmsg(m, errcode);
  }
  return(errcode);
}


int  DLLEXPORT OW_stepQ(OW_Project *m, long *tleft)
{
  int errcode;
  *tleft = 0;
  if (!m->OpenQflag) {
    return(105);
  }
  errcode = stepqual(m, tleft);
  if (!errcode && m->Saveflag && *tleft == 0) {
    m->SaveQflag = TRUE;
  }
  if (errcode) {
    errmsg(m, errcode);
  }
  return(errcode);
}


int  DLLEXPORT OW_closeQ(OW_Project *m)
{
  if (!m->Openflag) return(102);
  closequal(m);
  m->OpenQflag = FALSE;
  return(0);
}



int  DLLEXPORT OW_writeline(OW_Project *m, char *line)
{
  if (!m->Openflag) return(102);
  writeline(m,line);
  return(0);
}


int  DLLEXPORT OW_report(OW_Project *m)
{
  int  errcode;
  
  /* Check if results saved to binary output file */
  if (!m->SaveQflag) {
    return(106);
  }
  errcode = writereport(m);
  if (errcode) {
    errmsg(m, errcode);
  }
  return(errcode);
}


int  DLLEXPORT OW_resetreport(OW_Project *m)
{
  int i;
  if (!m->Openflag) return(102);
  initreport(m);
  for (i=1; i <= m->Nnodes; i++) m->Node[i].Rpt = 0;
  for (i=1; i <= m->Nlinks; i++) m->Link[i].Rpt = 0;
  return(0);
}


int  DLLEXPORT OW_setreport(OW_Project *m, char *s)
{
  char s1[MAXLINE+1];
  if (!m->Openflag) return(102);
  if (strlen(s) > MAXLINE) return(250);
  strcpy(s1,s);
  if (setreport(m,s1) > 0) return(250);
  else return(0);
}



int  DLLEXPORT OW_getcontrol(OW_Project *m, int controlIndex, int *controlType, int *linkIdx, EN_API_FLOAT_TYPE *setting, int *nodeIdx, EN_API_FLOAT_TYPE *level)
{
  double s, lvl;
  
  s = 0.0;
  lvl = 0.0;
  *controlType = 0;
  *linkIdx= 0;
  *nodeIdx = 0;
  if (!m->Openflag) return(102);
  if (controlIndex < 1 || controlIndex > m->Ncontrols) return(241);
  *controlType = m->Control[controlIndex].Type;
  *linkIdx = m->Control[controlIndex].Link;
  s = m->Control[controlIndex].Setting;
  if (m->Control[controlIndex].Setting != MISSING) switch (m->Link[*linkIdx].Type)
  {
    case PRV:
    case PSV:
    case PBV: s *= m->Ucf[PRESSURE]; break;
    case FCV: s *= m->Ucf[FLOW];
  }
  else if (m->Control[controlIndex].Status == OPEN) s = 1.0;
  
  /*** Updated 3/1/01 ***/
  else s = 0.0;
  
  *nodeIdx = m->Control[controlIndex].Node;
  if (*nodeIdx > m->Njuncs) {
    lvl = (m->Control[controlIndex].Grade - m->Node[*nodeIdx].El) * m->Ucf[ELEV];
  }
  else if (*nodeIdx > 0) {
    lvl = (m->Control[controlIndex].Grade - m->Node[*nodeIdx].El) * m->Ucf[PRESSURE];
  }
  else {
    lvl = (EN_API_FLOAT_TYPE)m->Control[controlIndex].Time;
  }
  *setting = (EN_API_FLOAT_TYPE)s;
  *level = (EN_API_FLOAT_TYPE)lvl;
  return(0);
}


int  DLLEXPORT OW_getcount(OW_Project *m, int code, int *count)
{
  *count = 0;
  if (!m->Openflag) return(102);
  
  switch (code)
  {
    case EN_NODECOUNT:    *count = m->Nnodes;    break;
    case EN_TANKCOUNT:    *count = m->Ntanks;    break;
    case EN_LINKCOUNT:    *count = m->Nlinks;    break;
    case EN_PATCOUNT:     *count = m->Npats;     break;
    case EN_CURVECOUNT:   *count = m->Ncurves;   break;
    case EN_CONTROLCOUNT: *count = m->Ncontrols; break;
    default: return(251);
  }
  return(0);
}


int  DLLEXPORT OW_getoption(OW_Project *m, int code, EN_API_FLOAT_TYPE *value)
{
  double v = 0.0;
  *value = 0.0;
  if (!m->Openflag) return(102);
  switch (code)
  {
    case EN_TRIALS:     v = (double)m->MaxIter;
      break;
    case EN_ACCURACY:   v = m->Hacc;
      break;
    case EN_TOLERANCE:  v = m->Ctol * m->Ucf[QUALITY];
      break;
    case EN_EMITEXPON:  if (m->Qexp > 0.0) v = 1.0 / m->Qexp;
      break;
    case EN_DEMANDMULT: v = m->Dmult;
      break;
    default:            return(251);
  }
  *value = (EN_API_FLOAT_TYPE)v;
  return(0);
}


int  DLLEXPORT OW_gettimeparam(OW_Project *m, int code, long *value)
{
  *value = 0;
  if (!m->Openflag) return(102);
  if (code < EN_DURATION || code > EN_NEXTEVENT) return(251);
  switch (code)
  {
    case EN_DURATION:     *value = m->Dur;       break;
    case EN_HYDSTEP:      *value = m->Hstep;     break;
    case EN_QUALSTEP:     *value = m->Qstep;     break;
    case EN_PATTERNSTEP:  *value = m->Pstep;     break;
    case EN_PATTERNSTART: *value = m->Pstart;    break;
    case EN_REPORTSTEP:   *value = m->Rstep;     break;
    case EN_REPORTSTART:  *value = m->Rstart;    break;
    case EN_STATISTIC:    *value = m->Tstatflag; break;
    case EN_PERIODS:      *value = m->Nperiods;  break;
    case EN_STARTTIME:    *value = m->Tstart;    break;  /* Added TNT 10/2/2009 */
    case EN_HTIME:        *value = m->Htime;     break;
    case EN_NEXTEVENT:
      *value = m->Hstep;     // find the lesser of the hydraulic time step length, or the time to next fill/empty
      tanktimestep(m,value);
      break;
  }
  return(0);
}


int  DLLEXPORT OW_getflowunits(OW_Project *m, int *code)
{
  *code = -1;
  if (!m->Openflag) return(102);
  *code = m->Flowflag;
  return(0);
}


int  DLLEXPORT OW_getpatternindex(OW_Project *m, char *id, int *index)
{
  int i;
  *index = 0;
  if (!m->Openflag) return(102);
  for (i=1; i <= m->Npats; i++)
  {
    if (strcmp(id, m->Pattern[i].ID) == 0)
    {
      *index = i;
      return(0);
    }
  }
  *index = 0;
  return(205);
}


int  DLLEXPORT OW_getpatternid(OW_Project *m, int index, char *id)
{
  strcpy(id,"");
  if (!m->Openflag) return(102);
  if (index < 1 || index > m->Npats) return(205);
  strcpy(id,m->Pattern[index].ID);
  return(0);
}


int  DLLEXPORT OW_getpatternlen(OW_Project *m, int index, int *len)
{
  if (!m->Openflag) return(102);
  if (index < 1 || index > m->Npats) return(205);
  *len = m->Pattern[index].Length;
  return(0);
}


int  DLLEXPORT OW_getpatternvalue(OW_Project *m, int index, int period, EN_API_FLOAT_TYPE *value)
{
  *value = 0.0;
  if (!m->Openflag) return(102);
  if (index < 1 || index > m->Npats) return(205);
  if (period < 1 || period > m->Pattern[index].Length) return(251);
  *value = (EN_API_FLOAT_TYPE)m->Pattern[index].F[period-1];
  return(0);
}


int  DLLEXPORT OW_getaveragepatternvalue(OW_Project *m, int index, EN_API_FLOAT_TYPE *value)
{
  *value = 0.0;
  if (!m->Openflag)
    return(102);
  if (index < 1 || index > m->Npats)
    return(205);
  //if (period < 1 || period > Pattern[index].Length) return(251);
  int i;
  for (i=0; i < m->Pattern[index].Length; i++) {
    *value += m->Pattern[index].F[i];
  }
  *value /= (EN_API_FLOAT_TYPE)m->Pattern[index].Length;
  return(0);
}


int  DLLEXPORT OW_getqualtype(OW_Project *m, int *qualcode, int *tracenode)
{
  *tracenode = 0;
  if (!m->Openflag) return(102);
  *qualcode = m->Qualflag;
  if (m->Qualflag == TRACE) *tracenode = m->TraceNode;
  return(0);
}


int  DLLEXPORT OW_geterror(int errcode, char *errmsg, int n)
{
  switch (errcode)
  {
    case 1:  strncpy(errmsg,WARN1,n);   break;
    case 2:  strncpy(errmsg,WARN2,n);   break;
    case 3:  strncpy(errmsg,WARN3,n);   break;
    case 4:  strncpy(errmsg,WARN4,n);   break;
    case 5:  strncpy(errmsg,WARN5,n);   break;
    case 6:  strncpy(errmsg,WARN6,n);   break;
    default:
      geterrmsg(errcode, errmsg);
      break;
  }
  if (strlen(errmsg) == 0)
    return(251);
  else
    return(0);
}


int  DLLEXPORT OW_getstatistic(OW_Project *m, int code, EN_API_FLOAT_TYPE* value)
{
  switch (code) {
    case EN_ITERATIONS:
      *value = (EN_API_FLOAT_TYPE)m->_iterations;
      break;
    case EN_RELATIVEERROR:
      *value = (EN_API_FLOAT_TYPE)m->_relativeError;
      break;
    default:
      break;
  }
  return 0;
}



int  DLLEXPORT OW_getnodeindex(OW_Project *m, char *id, int *index)
{
  *index = 0;
  if (!m->Openflag) {
    return(102);
  }
  *index = findnode(m,id);
  if (*index == 0) {
    return(203);
  }
  else {
    return(0);
  }
}


int  DLLEXPORT OW_getnodeid(OW_Project *m, int index, char *id)
{
  strcpy(id,"");
  if (!m->Openflag) return(102);
  if (index < 1 || index > m->Nnodes) return(203);
  strcpy(id,m->Node[index].ID);
  return(0);
}


int  DLLEXPORT OW_getnodetype(OW_Project *m, int index, EN_NodeType *code)
{
  *code = -1;
  if (!m->Openflag) return(102);
  if (index < 1 || index > m->Nnodes) return(203);
  if (index <= m->Njuncs) *code = EN_JUNCTION;
  else
  {
    if (m->Tank[index - m->Njuncs].A == 0.0) *code = EN_RESERVOIR;
    else *code = EN_TANK;
  }
  return(0);
}


int  DLLEXPORT OW_getnodevalue(OW_Project *m, int index, int code, EN_API_FLOAT_TYPE *value)
{
  int tankIndex = 0;
  int isTank = 0;
  if ( index > m->Njuncs ) {
    isTank = 1;
    tankIndex = index - m->Njuncs;
  }
  
  double v = 0.0;
  Pdemand demand;
  Psource source;
  
  /* Check for valid arguments */
  *value = 0.0;
  if (!m->Openflag) return(102);
  if (index <= 0 || index > m->Nnodes) return(203);
  
  /* Retrieve called-for parameter */
  switch (code)
  {
    case EN_ELEVATION:
      v = m->Node[index].El * m->Ucf[ELEV];
      break;
      
    case EN_BASEDEMAND:
      v = 0.0;
      /* NOTE: primary demand category is last on demand list */
      if (index <= m->Njuncs)
        for (demand = m->Node[index].D; demand != NULL; demand = demand->next) {
          v = (demand->Base);
        }
      v *= m->Ucf[FLOW];
      break;
      
    case EN_PATTERN:
      v = 0.0;
      /* NOTE: primary demand category is last on demand list */
      if (index <= m->Njuncs)
      {
        for (demand = m->Node[index].D; demand != NULL; demand = demand->next)
          v = (double)(demand->Pat);
      }
      else v = (double)(m->Tank[index - m->Njuncs].Pat);
      break;
      
    case EN_EMITTER:
      v = 0.0;
      if (m->Node[index].Ke > 0.0)
        v = m->Ucf[FLOW]/pow((m->Ucf[PRESSURE]* m->Node[index].Ke),(1.0/m->Qexp));
      break;
      
    case EN_INITQUAL:
      v = m->Node[index].C0 * m->Ucf[QUALITY];
      break;
      
      /*** Additional parameters added for retrieval ***/                            //(2.00.11 - LR)
    case EN_SOURCEQUAL:
    case EN_SOURCETYPE:
    case EN_SOURCEMASS:
    case EN_SOURCEPAT:
      source = m->Node[index].S;
      if (source == NULL) return(240);
      if (code == EN_SOURCEQUAL)      v = source->C0;
      else if (code == EN_SOURCEMASS) v = source->Smass*60.0;
      else if (code == EN_SOURCEPAT)  v = source->Pat;
      else                            v = source->Type;
      break;
      
    case EN_TANKLEVEL:
      if (!isTank) return(251);
      v = (m->Tank[tankIndex].H0 - m->Node[index].El) * m->Ucf[ELEV];
      break;
      
      /*** New parameter added for retrieval ***/                                    //(2.00.11 - LR)
    case EN_INITVOLUME:                                                      //(2.00.11 - LR)
      v = 0.0;                                                              //(2.00.11 - LR)
      if ( isTank ) {
        v = m->Tank[tankIndex].V0 * m->Ucf[VOLUME];          //(2.00.11 - LR)
      }
      break;                                                                //(2.00.11 - LR)
      
      /*** New parameter added for retrieval ***/                                    //(2.00.11 - LR)
    case EN_MIXMODEL:                                                        //(2.00.11 - LR)
      v = MIX1;                                                             //(2.00.11 - LR)
      if ( index > m->Njuncs ) v = m->Tank[tankIndex].MixModel;                //(2.00.11 - LR)
      break;                                                                //(2.00.11 - LR)
      
      /*** New parameter added for retrieval ***/                                    //(2.00.11 - LR)
    case EN_MIXZONEVOL:                                                      //(2.00.11 - LR)
      v = 0.0;                                                              //(2.00.11 - LR)
      if ( index > m->Njuncs ) v = m->Tank[tankIndex].V1max * m->Ucf[VOLUME];       //(2.00.11 - LR)
      break;                                                                //(2.00.11 - LR)
      
    case EN_DEMAND:
      v = m->hydraulics.NodeDemand[index] * m->Ucf[FLOW];
      break;
      
    case EN_HEAD:
      v = m->hydraulics.NodeHead[index] * m->Ucf[HEAD];
      break;
      
    case EN_PRESSURE:
      v = (m->hydraulics.NodeHead[index] - m->Node[index].El) * m->Ucf[PRESSURE];
      break;
      
    case EN_QUALITY:
      v = m->NodeQual[index] * m->Ucf[QUALITY];
      break;
      
      /*** New parameters added for retrieval begins here   ***/                     //(2.00.12 - LR)
      /*** (Thanks to Nicolas Basile of Ecole Polytechnique ***/
      /***  de Montreal for suggesting some of these.)      ***/
      
    case EN_TANKDIAM:
      v = 0.0;
      if ( index > m->Njuncs )
      {
        v = sqrt(4.0/PI*m->Tank[tankIndex].A) * m->Ucf[ELEV];
      }
      break;
      
    case EN_MINVOLUME:
      v = 0.0;
      if ( index > m->Njuncs ) v = m->Tank[tankIndex].Vmin * m->Ucf[VOLUME];
      break;
      
    case EN_MAXVOLUME: // !sph
      v = 0.0;
      if ( index > m->Njuncs ) v = m->Tank[tankIndex].Vmax * m->Ucf[VOLUME];
      break;
      
    case EN_VOLCURVE:
      v = 0.0;
      if ( index > m->Njuncs ) v = m->Tank[tankIndex].Vcurve;
      break;
      
    case EN_MINLEVEL:
      v = 0.0;
      if ( index > m->Njuncs )
      {
        v = (m->Tank[tankIndex].Hmin - m->Node[index].El) * m->Ucf[ELEV];
      }
      break;
      
    case EN_MAXLEVEL:
      v = 0.0;
      if ( index > m->Njuncs )
      {
        v = (m->Tank[tankIndex].Hmax - m->Node[index].El) * m->Ucf[ELEV];
      }
      break;
      
    case EN_MIXFRACTION:
      v = 1.0;
      if ( isTank && m->Tank[tankIndex].Vmax > 0.0)
      {
        v = m->Tank[tankIndex].V1max / m->Tank[tankIndex].Vmax;
      }
      break;
      
    case EN_TANK_KBULK:
      v = 0.0;
      if (index > m->Njuncs) v = m->Tank[tankIndex].Kb * SECperDAY;
      break;
      
      /***  New parameter additions ends here. ***/                                  //(2.00.12 - LR)
      
    case EN_TANKVOLUME:
      if (index <= m->Njuncs) return(251);
      v = tankvolume(m, tankIndex, m->hydraulics.NodeHead[index]) * m->Ucf[VOLUME];
      break;
      
    default: return(251);
  }
  *value = (EN_API_FLOAT_TYPE)v;
  return(0);
}


int  DLLEXPORT OW_getcoord(OW_Project *m, int index, EN_API_FLOAT_TYPE *x, EN_API_FLOAT_TYPE *y)
{
  *x = m->Coord[index].X[0];
  *y = m->Coord[index].Y[0];
  return 0;
}



int  DLLEXPORT OW_getnumdemands(OW_Project *m, int nodeIndex, int *numDemands)
{
  Pdemand d;
	int n=0;
	/* Check for valid arguments */
	if (!m->Openflag)
    return(102);
	if (nodeIndex <= 0 || nodeIndex > m->Nnodes)
    return(203);
	for (d = m->Node[nodeIndex].D; d != NULL; d = d->next) {
    n++;
  }
	*numDemands=n;
	return 0;
}


int  DLLEXPORT OW_getbasedemand(OW_Project *m, int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE *baseDemand)
{
  Pdemand d;
  int n=1;
  /* Check for valid arguments */
  if (!m->Openflag) return(102);
  if (nodeIndex <= 0 || nodeIndex > m->Nnodes) return(203);
  if (nodeIndex <= m->Njuncs) {
    for (d = m->Node[nodeIndex].D; n < demandIdx && d != NULL; d = d->next)
      n++;
    if (n!=demandIdx)
      return(253);
    
    *baseDemand=(EN_API_FLOAT_TYPE)(d->Base * m->Ucf[FLOW]);
  }
  else {
    *baseDemand=(EN_API_FLOAT_TYPE)(0.0);
  }
  return 0;
}


int  DLLEXPORT OW_getdemandpattern(OW_Project *m, int nodeIndex, int demandIdx, int *pattIdx)
{
  Pdemand d;
	int n=1;
	/* Check for valid arguments */
	if (!m->Openflag)
    return(102);
	if (nodeIndex <= 0 || nodeIndex > m->Nnodes)
    return(203);
	for (d = m->Node[nodeIndex].D; n < demandIdx && d != NULL; d = d->next) {
    n++;
  }
	if (n != demandIdx)
    return(253);
  
	*pattIdx=d->Pat;
	return 0;
}



int  DLLEXPORT OW_getlinkindex(OW_Project *m, char *id, int *index)
{
  *index = 0;
  if (!m->Openflag) return(102);
  *index = findlink(m,id);
  if (*index == 0) return(204);
  else return(0);
}


int  DLLEXPORT OW_getlinkid(OW_Project *m, int index, char *id)
{
  strcpy(id,"");
  if (!m->Openflag) return(102);
  if (index < 1 || index > m->Nlinks) return(204);
  strcpy(id,m->Link[index].ID);
  return(0);
}


int  DLLEXPORT OW_getlinktype(OW_Project *m, int index, EN_LinkType *type)
{
  *type = -1;
  if (!m->Openflag) return(102);
  if (index < 1 || index > m->Nlinks) return(204);
  *type = m->Link[index].Type;
  return(0);
}


int  DLLEXPORT OW_getlinknodes(OW_Project *m, int index, int *node1, int *node2)
{
  *node1 = 0;
  *node2 = 0;
  if (!m->Openflag) return(102);
  if (index < 1 || index > m->Nlinks) return(204);
  *node1 = m->Link[index].N1;
  *node2 = m->Link[index].N2;
  return(0);
}


int  DLLEXPORT OW_getlinkvalue(OW_Project *m, int index, int code, EN_API_FLOAT_TYPE *value)
{
  double a,h,q, v = 0.0;
  
  /* Check for valid arguments */
  *value = 0.0;
  if (!m->Openflag) return(102);
  if (index <= 0 || index > m->Nlinks) return(204);
  
  /* Retrieve called-for parameter */
  switch (code)
  {
    case EN_DIAMETER:
      if (m->Link[index].Type == PUMP) v = 0.0;
      else v = m->Link[index].Diam * m->Ucf[DIAM];
      break;
      
    case EN_LENGTH:
      v = m->Link[index].Len * m->Ucf[ELEV];
      break;
      
    case EN_ROUGHNESS:
      if (m->Link[index].Type <= PIPE)
      {
        if (m->Formflag == DW)
          v = m->Link[index].Kc*(1000.0 * m->Ucf[ELEV]);
        else v = m->Link[index].Kc;
      }
      else v = 0.0;
      break;
      
    case EN_MINORLOSS:
      if (m->Link[index].Type != PUMP)
      {
        v = m->Link[index].Km;
        v *= (SQR(m->Link[index].Diam)*SQR(m->Link[index].Diam)/0.02517);
      }
      else v = 0.0;
      break;
      
    case EN_INITSTATUS:
      if (m->Link[index].Stat <= CLOSED) v = 0.0;
      else v = 1.0;
      break;
      
    case EN_INITSETTING:
      if (m->Link[index].Type == PIPE || m->Link[index].Type == CV)
        return(OW_getlinkvalue(m, index, EN_ROUGHNESS, value));
      v = m->Link[index].Kc;
      switch (m->Link[index].Type)
    {
      case PRV:
      case PSV:
      case PBV:
        v *= m->Ucf[PRESSURE];
        break;
      case FCV:
        v *= m->Ucf[FLOW];
        break;
      default:
        break;
    }
      break;
      
    case EN_KBULK:
      v = m->Link[index].Kb*SECperDAY;
      break;
      
    case EN_KWALL:
      v = m->Link[index].Kw*SECperDAY;
      break;
      
    case EN_FLOW:
      
      /*** Updated 10/25/00 ***/
      if (m->hydraulics.LinkStatus[index] <= CLOSED) v = 0.0;
      else v = m->hydraulics.LinkFlows[index] * m->Ucf[FLOW];
      break;
      
    case EN_VELOCITY:
      if (m->Link[index].Type == PUMP) v = 0.0;
      
      /*** Updated 11/19/01 ***/
      else if (m->hydraulics.LinkStatus[index] <= CLOSED) v = 0.0;
      
      else
      {
        q = ABS(m->hydraulics.LinkFlows[index]);
        a = PI*SQR(m->Link[index].Diam)/4.0;
        v = q/a * m->Ucf[VELOCITY];
      }
      break;
      
    case EN_HEADLOSS:
      
      /*** Updated 11/19/01 ***/
      if (m->hydraulics.LinkStatus[index] <= CLOSED) v = 0.0;
      
      else
      {
        h = m->hydraulics.NodeHead[m->Link[index].N1] - m->hydraulics.NodeHead[m->Link[index].N2];
        if (m->Link[index].Type != PUMP) h = ABS(h);
        v = h * m->Ucf[HEADLOSS];
      }
      break;
      
    case EN_STATUS:
      if (m->hydraulics.LinkStatus[index] <= CLOSED) v = 0.0;
      else v = 1.0;
      break;
      
    case EN_SETTING:
      if (m->Link[index].Type == PIPE || m->Link[index].Type == CV) {
        return(OW_getlinkvalue(m, index, EN_ROUGHNESS, value));
      }
      if (m->hydraulics.LinkSetting[index] == MISSING) {
        v = 0.0;
      }
      else {
        v = m->hydraulics.LinkSetting[index];
      }
      switch (m->Link[index].Type)
    {
      case PRV:
      case PSV:
      case PBV: v *= m->Ucf[PRESSURE]; break;
      case FCV: v *= m->Ucf[FLOW];
    }
      break;
      
    case EN_ENERGY:
      getenergy(m, index, &v, &a);
      break;
      
    case EN_LINKQUAL:
      v = avgqual(m, index) * m->Ucf[LINKQUAL];
      break;
      
    default: return(251);
  }
  *value = (EN_API_FLOAT_TYPE)v;
  return(0);
}



int  DLLEXPORT OW_getcurve(OW_Project *m, int curveIndex, char* id, int *nValues, EN_API_FLOAT_TYPE **xValues, EN_API_FLOAT_TYPE **yValues)
{
  int err = 0;
  
  Scurve curve = m->Curve[curveIndex];
  int nPoints = curve.Npts;
  
  EN_API_FLOAT_TYPE *pointX = calloc(nPoints, sizeof(EN_API_FLOAT_TYPE));
  EN_API_FLOAT_TYPE *pointY = calloc(nPoints, sizeof(EN_API_FLOAT_TYPE));
  int iPoint;
  for (iPoint = 0; iPoint < nPoints; iPoint++) {
    double x = curve.X[iPoint] * m->Ucf[LENGTH];
    double y = curve.Y[iPoint] * m->Ucf[VOLUME];
    pointX[iPoint] = (EN_API_FLOAT_TYPE)x;
    pointY[iPoint] = (EN_API_FLOAT_TYPE)y;
  }
  
  strncpy(id, curve.ID, MAXID);
  *nValues = nPoints;
  *xValues = pointX;
  *yValues = pointY;
  
  return err;
}



int  DLLEXPORT OW_getversion(int *v)
{
  *v = CODEVERSION;
  return(0);
}



int  DLLEXPORT OW_setcontrol(OW_Project *m, int cindex, int ctype, int lindex, EN_API_FLOAT_TYPE setting, int nindex, EN_API_FLOAT_TYPE level)
{
  char   status = ACTIVE;
  long   t = 0;
  double s = setting, lvl = level;
  
  /* Check that input file opened */
  if (!m->Openflag) return(102);
  
  /* Check that control exists */
  if (cindex < 1 || cindex > m->Ncontrols) return(241);
  
  /* Check that controlled link exists */
  if (lindex == 0)
  {
    m->Control[cindex].Link = 0;
    return(0);
  }
  if (lindex < 0 || lindex > m->Nlinks) return(204);
  
  /* Cannot control check valve. */
  if (m->Link[lindex].Type == CV) return(207);
  
  /* Check for valid parameters */
  if (ctype < 0 || ctype > EN_TIMEOFDAY) return(251);
  if (ctype == EN_LOWLEVEL || ctype == EN_HILEVEL)
  {
    if (nindex < 1 || nindex > m->Nnodes) return(203);
  }
  else nindex = 0;
  if (s < 0.0 || lvl < 0.0) return(202);
  
  /* Adjust units of control parameters */
  switch (m->Link[lindex].Type)
  {
    case PRV:
    case PSV:
    case PBV:  s /= m->Ucf[PRESSURE];
      break;
    case FCV:  s /= m->Ucf[FLOW];
      break;
      
      /*** Updated 9/7/00 ***/
    case GPV:  if (s == 0.0) status = CLOSED;
    else if (s == 1.0) status = OPEN;
    else return(202);
      s = m->Link[lindex].Kc;
      break;
      
    case PIPE:
    case PUMP: status = OPEN;
      if (s == 0.0) status = CLOSED;
  }
  if (ctype == LOWLEVEL || ctype == HILEVEL)
  {
    if (nindex > m->Njuncs) lvl = m->Node[nindex].El + (level / m->Ucf[ELEV]);
    else lvl = m->Node[nindex].El + (level / m->Ucf[PRESSURE]);
  }
  if (ctype == TIMER)     t = (long)ROUND(lvl);
  if (ctype == TIMEOFDAY) t = (long)ROUND(lvl) % SECperDAY;
  
  /* Reset control's parameters */
  m->Control[cindex].Type = (char)ctype;
  m->Control[cindex].Link = lindex;
  m->Control[cindex].Node = nindex;
  m->Control[cindex].Status = status;
  m->Control[cindex].Setting = s;
  m->Control[cindex].Grade = lvl;
  m->Control[cindex].Time = t;
  return(0);
}


int  DLLEXPORT OW_setnodevalue(OW_Project *m, int index, int code, EN_API_FLOAT_TYPE v)
{
  int  j;
  Pdemand demand;
  Psource source;
  double value = v;
  
  if (!m->Openflag) return(102);
  if (index <= 0 || index > m->Nnodes) return(203);
  
  int isTank = 0;
  int tankIndex = 0;
  if (index > m->Njuncs) {
    isTank = 1;
    tankIndex = index - m->Njuncs;
  }
  
  switch (code)
  {
    case EN_ELEVATION:
      if (!isTank) {
        m->Node[index].El = value / m->Ucf[ELEV];
      }
      else
      {
        value = (value / m->Ucf[ELEV]) - m->Node[index].El;
        j = index - m->Njuncs;
        m->Tank[tankIndex].H0 += value;
        m->Tank[tankIndex].Hmin += value;
        m->Tank[tankIndex].Hmax += value;
        m->Node[index].El += value;
        m->hydraulics.NodeHead[index] += value;
      }
      break;
      
    case EN_BASEDEMAND:
      /* NOTE: primary demand category is last on demand list */
      if (!isTank)
      {
        for (demand = m->Node[index].D; demand != NULL; demand = demand ->next)
        {
          if (demand->next == NULL) {
            demand->Base = value / m->Ucf[FLOW];
          }
        }
      }
      break;
      
    case EN_PATTERN:
      /* NOTE: primary demand category is last on demand list */
      j = ROUND(value);
      if (j < 0 || j > m->Npats) return(205);
      if (!isTank)
      {
        for (demand = m->Node[index].D; demand != NULL; demand = demand ->next)
        {
          if (demand->next == NULL) demand->Pat = j;
        }
      }
      else {
        m->Tank[index - m->Njuncs].Pat = j;
      }
      break;
      
    case EN_EMITTER:
      if (isTank)
        return(203);
      if (value < 0.0)
        return(202);
      if (value > 0.0)
        value = pow((m->Ucf[FLOW]/value), m->Qexp) / m->Ucf[PRESSURE];
      m->Node[index].Ke = value;
      break;
      
    case EN_INITQUAL:
      if (value < 0.0) return(202);
      m->Node[index].C0 = value / m->Ucf[QUALITY];
      if (isTank) {
        m->Tank[tankIndex].C = m->Node[index].C0;
      }
      break;
      
    case EN_SOURCEQUAL:
    case EN_SOURCETYPE:
    case EN_SOURCEPAT:
      if (value < 0.0) return(202);
      source = m->Node[index].S;
      if (source == NULL)
      {
        source = (struct Ssource *) malloc(sizeof(struct Ssource));
        if (source == NULL) return(101);
        source->Type = CONCEN;
        source->C0 = 0.0;
        source->Pat = 0;
        m->Node[index].S = source;
      }
      if (code == EN_SOURCEQUAL) {
        source->C0 = value;
      }
      else if (code == EN_SOURCEPAT)
      {
        j = ROUND(value);
        if (j < 0 || j > m->Npats) return(205);
        source->Pat = j;
      }
      else // code == EN_SOURCETYPE
      {
        j = ROUND(value);
        if ( j < CONCEN || j > FLOWPACED) return(251);
        else source->Type = (char)j;
      }
      return(0);
      
    case EN_TANKLEVEL:
      if (!isTank) return(251);
      
      if (m->Tank[tankIndex].A == 0.0)  /* Tank is a reservoir */
      {
        m->Tank[tankIndex].H0 = value / m->Ucf[ELEV];
        m->Tank[tankIndex].Hmin = m->Tank[tankIndex].H0;
        m->Tank[tankIndex].Hmax = m->Tank[tankIndex].H0;
        m->Node[index].El = m->Tank[tankIndex].H0;
        m->hydraulics.NodeHead[index] = m->Tank[tankIndex].H0;
      }
      else
      {
        value = m->Node[index].El + value / m->Ucf[ELEV];
        if (value > m->Tank[tankIndex].Hmax
            ||  value < m->Tank[tankIndex].Hmin) return(202);
        m->Tank[tankIndex].H0 = value;
        m->Tank[tankIndex].V0 = tankvolume(m, tankIndex, m->Tank[tankIndex].H0);
        // Resetting Volume in addition to initial volume
        m->Tank[tankIndex].V = m->Tank[tankIndex].V0;
        m->hydraulics.NodeHead[index] = m->Tank[tankIndex].H0;
      }
      break;
      
      /*** New parameters added for retrieval begins here   ***/                     //(2.00.12 - LR)
      /*** (Thanks to Nicolas Basile of Ecole Polytechnique ***/
      /***  de Montreal for suggesting some of these.)      ***/
      
    case EN_TANKDIAM:
      if (value <= 0.0) return(202);
      
      if (isTank && m->Tank[tankIndex].A > 0.0)
      {
        value /= m->Ucf[ELEV];
        m->Tank[tankIndex].A = PI*SQR(value)/4.0;
        m->Tank[tankIndex].Vmin = tankvolume(m, tankIndex, m->Tank[tankIndex].Hmin);
        m->Tank[tankIndex].V0 = tankvolume(m, tankIndex, m->Tank[tankIndex].H0);
        m->Tank[tankIndex].Vmax = tankvolume(m, tankIndex, m->Tank[tankIndex].Hmax);
      }
      break;
      
    case EN_MINVOLUME:
      if (value < 0.0) return(202);
      
      if (isTank && m->Tank[tankIndex].A > 0.0)
      {
        m->Tank[tankIndex].Vmin = value / m->Ucf[VOLUME];
        m->Tank[tankIndex].V0 = tankvolume(m, tankIndex, m->Tank[tankIndex].H0);
        m->Tank[tankIndex].Vmax = tankvolume(m, tankIndex, m->Tank[tankIndex].Hmax);
      }
      break;
      
    case EN_MINLEVEL:
      if (value < 0.0) return(202);
      if (isTank && m->Tank[tankIndex].A > 0.0)
      {
        if (m->Tank[tankIndex].Vcurve > 0)
          return(202);
        
        m->Tank[tankIndex].Hmin = (value / m->Ucf[ELEV]) + m->Node[index].El;
        m->Tank[tankIndex].Vmin = tankvolume(m, tankIndex, m->Tank[tankIndex].Hmin);
      }
      break;
      
    case EN_MAXLEVEL:
      if (value < 0.0) return(202);
      if (isTank && m->Tank[tankIndex].A > 0.0)
      {
        if (m->Tank[tankIndex].Vcurve > 0)
          return(202);
        
        m->Tank[tankIndex].Hmax = value / m->Ucf[ELEV] + m->Node[index].El;
        m->Tank[tankIndex].Vmax = tankvolume(m, tankIndex, m->Tank[tankIndex].Hmax);
      }
      break;
      
    case EN_MIXMODEL:
      j = ROUND(value);
      if (j < MIX1 || j > LIFO) return(202);
      if (isTank && m->Tank[tankIndex].A > 0.0)
      {
        m->Tank[tankIndex].MixModel = (char)j;
      }
      break;
      
    case EN_MIXFRACTION:
      if (value < 0.0 || value > 1.0) return(202);
      if (tankIndex > 0 && m->Tank[tankIndex].A > 0.0)
      {
        m->Tank[tankIndex].V1max = value * m->Tank[tankIndex].Vmax;
      }
      break;
      
    case EN_TANK_KBULK:
      if (isTank && m->Tank[tankIndex].A > 0.0)
      {
        m->Tank[tankIndex].Kb = value/SECperDAY;
        m->Reactflag = 1;
      }
      break;
      
      /***  New parameter additions ends here. ***/                                  //(2.00.12 - LR)
      
    default: return(251);
  }
  return(0);
}


int  DLLEXPORT OW_setlinkvalue(OW_Project *m, int index, int code, EN_API_FLOAT_TYPE v)
{
  char  s;
  double r, value = v;
  
  if (!m->Openflag) return(102);
  if (index <= 0 || index > m->Nlinks) return(204);
  switch (code)
  {
    case EN_DIAMETER:
      if (m->Link[index].Type != PUMP)
      {
        if (value <= 0.0) return(202);
        value /= m->Ucf[DIAM];              /* Convert to feet */
        r = m->Link[index].Diam/value;      /* Ratio of old to new diam */
        m->Link[index].Km *= SQR(r)*SQR(r); /* Adjust minor loss factor */
        m->Link[index].Diam = value;        /* Update diameter */
        resistance(m,index);               /* Update resistance factor */
      }
      break;
      
    case EN_LENGTH:
      if (m->Link[index].Type <= PIPE)
      {
        if (value <= 0.0) return(202);
        m->Link[index].Len = value / m->Ucf[ELEV];
        resistance(m,index);
      }
      break;
      
    case EN_ROUGHNESS:
      if (m->Link[index].Type <= PIPE)
      {
        if (value <= 0.0) return(202);
        m->Link[index].Kc = value;
        if (m->Formflag  == DW) {
          m->Link[index].Kc /= (1000.0 * m->Ucf[ELEV]);
        }
        resistance(m, index);
      }
      break;
      
    case EN_MINORLOSS:
      if (m->Link[index].Type != PUMP)
      {
        if (value <= 0.0) return(202);
        m->Link[index].Km = 0.02517*value/SQR(m->Link[index].Diam)/SQR(m->Link[index].Diam);
      }
      break;
      
    case EN_INITSTATUS:
    case EN_STATUS:
      /* Cannot set status for a check valve */
      if (m->Link[index].Type == CV) return(207);
      s = (char)ROUND(value);
      if (s < 0 || s > 1) return(251);
      if (code == EN_INITSTATUS)
        setlinkstatus(m, index, s, &m->Link[index].Stat, &m->Link[index].Kc);
      else
        setlinkstatus(m, index, s, &m->hydraulics.LinkStatus[index], &m->hydraulics.LinkSetting[index]);
      break;
      
    case EN_INITSETTING:
    case EN_SETTING:
      if (value < 0.0) return(202);
      if (m->Link[index].Type == PIPE || m->Link[index].Type == CV)
        return(OW_setlinkvalue(m, index, EN_ROUGHNESS, v));
      else
      {
        switch (m->Link[index].Type)
        {
          case PUMP: break;
          case PRV:
          case PSV:
          case PBV: value /= m->Ucf[PRESSURE]; break;
          case FCV: value /= m->Ucf[FLOW]; break;
          case TCV: break;
            
            /***  Updated 9/7/00  ***/
          case GPV: return(202);  /* Cannot modify setting for GPV */
            
          default:  return(251);
        }
        if (code == EN_INITSETTING)
          setlinksetting(m, index, value, &m->Link[index].Stat, &m->Link[index].Kc);
        else
          setlinksetting(m, index, value, &m->hydraulics.LinkStatus[index], &m->hydraulics.LinkSetting[index]);
      }
      break;
      
    case EN_KBULK:
      if (m->Link[index].Type <= PIPE)
      {
        m->Link[index].Kb = value/SECperDAY;
        m->Reactflag = 1;                                                     //(2.00.12 - LR)
      }
      break;
      
    case EN_KWALL:
      if (m->Link[index].Type <= PIPE)
      {
        m->Link[index].Kw = value/SECperDAY;
        m->Reactflag = 1;                                                     //(2.00.12 - LR)
      }
      break;
      
    default: return(251);
  }
  return(0);
}


int  DLLEXPORT OW_addpattern(OW_Project *m, char *id)
{
  int i, j, n, err = 0;
  Spattern *tmpPat;
  
  /* Check if a pattern with same id already exists */
  
  if ( !m->Openflag ) return(102);
  if ( OW_getpatternindex(m, id, &i) == 0 ) return(215);
  
  /* Check that id name is not too long */
  
  if (strlen(id) > MAXID) return(250);
  
  /* Allocate memory for a new array of patterns */
  
  n = m->Npats + 1;
  tmpPat = (Spattern *) calloc(n+1, sizeof(Spattern));
  if ( tmpPat == NULL ) return(101);
  
  /* Copy contents of old pattern array to new one */
  
  for (i=0; i <= m->Npats; i++)
  {
    strcpy(tmpPat[i].ID, m->Pattern[i].ID);
    tmpPat[i].Length  = m->Pattern[i].Length;
    tmpPat[i].F = (double *) calloc(m->Pattern[i].Length, sizeof(double));
    if (tmpPat[i].F == NULL) err = 1;
    else for (j=0; j < m->Pattern[i].Length; j++)
      tmpPat[i].F[j] = m->Pattern[i].F[j];
  }
  
  /* Add the new pattern to the new array of patterns */
  
  strcpy(tmpPat[n].ID, id);
  tmpPat[n].Length = 1;
  tmpPat[n].F = (double *) calloc(tmpPat[n].Length, sizeof(double));
  if (tmpPat[n].F == NULL) err = 1;
  else tmpPat[n].F[0] = 1.0;
  
  /* Abort if memory allocation error */
  
  if (err)
  {
    for (i=0; i<=n; i++) if (tmpPat[i].F) free(tmpPat[i].F);
    free(tmpPat);
    return(101);
  }
  
  // Replace old pattern array with new one
  
  for (i=0; i <= m->Npats; i++) {
    free(m->Pattern[i].F);
  }
  free(m->Pattern);
  m->Pattern = tmpPat;
  m->Npats = n;
  m->MaxPats = n;
  return 0;
}


int  DLLEXPORT OW_setpattern(OW_Project *m, int index, EN_API_FLOAT_TYPE *f, int n)
{
  int j;
  
  /* Check for valid arguments */
  if (!m->Openflag)
    return(102);
  if (index <= 0 || index > m->Npats)
    return(205);
  if (n <= 0)
    return(202);
  
  /* Re-set number of time periods & reallocate memory for multipliers */
  m->Pattern[index].Length = n;
  m->Pattern[index].F = (double *) realloc(m->Pattern[index].F, n*sizeof(double));
  if (m->Pattern[index].F == NULL)
    return(101);
  
  /* Load multipliers into pattern */
  for (j=0; j<n; j++) {
    m->Pattern[index].F[j] = f[j];
  }
  return(0);
}


int  DLLEXPORT OW_setpatternvalue(OW_Project *m, int index, int period, EN_API_FLOAT_TYPE value)
{
  if (!m->Openflag) return(102);
  if (index  <= 0 || index  > m->Npats) return(205);
  if (period <= 0 || period > m->Pattern[index].Length) return(251);
  m->Pattern[index].F[period-1] = value;
  return(0);
}


int  DLLEXPORT OW_settimeparam(OW_Project *m, int code, long value)
{
  if (!m->Openflag)
    return(102);
  
  if (m->OpenHflag || m->OpenQflag) {
    // --> there's nothing wrong with changing certain time parameters during a simulation run, or before the run has started.
    // todo -- how to tell?
    /*
     if (code == EN_DURATION || code == EN_HTIME || code == EN_REPORTSTEP || code == EN_DURATION || Htime == 0) {
     // it's ok
     }
     else {
     return(109);
     }
     */
  }
  if (value < 0) return(202);
  switch(code)
  {
    case EN_DURATION:
      m->Dur = value;
      if (m->Rstart > m->Dur) {
        m->Rstart = 0;
      }
      break;
      
    case EN_HYDSTEP:
      if (value == 0)
        return(202);
      m->Hstep = value;
      m->Hstep = MIN(m->Pstep, m->Hstep);
      m->Hstep = MIN(m->Rstep, m->Hstep);
      m->Qstep = MIN(m->Qstep, m->Hstep);
      break;
      
    case EN_QUALSTEP:
      if (value == 0) return(202);
      m->Qstep = value;
      m->Qstep = MIN(m->Qstep, m->Hstep);
      break;
      
    case EN_PATTERNSTEP:
      if (value == 0) return(202);
      m->Pstep = value;
      if (m->Hstep > m->Pstep) {
        m->Hstep = m->Pstep;
      }
      break;
      
    case EN_PATTERNSTART:
      m->Pstart = value;
      break;
      
    case EN_REPORTSTEP:
      if (value == 0) return(202);
      m->Rstep = value;
      if (m->Hstep > m->Rstep) {
        m->Hstep = m->Rstep;
      }
      break;
      
    case EN_REPORTSTART:
      if (m->Rstart > m->Dur) return(202);
      m->Rstart = value;
      break;
      
    case EN_RULESTEP:
      if (value == 0)
        return(202);
      m->Rulestep = value;
      m->Rulestep = MIN(m->Rulestep, m->Hstep);
      break;
      
    case EN_STATISTIC:
      if (value > RANGE)
        return(202);
      m->Tstatflag = (char)value;
      break;
      
    case EN_HTIME:
      m->Htime = value;
      break;
      
    case EN_QTIME:
      m->Qtime = value;
      break;
      
    default:
      return(251);
  }
  return(0);
}


int  DLLEXPORT OW_setoption(OW_Project *m, int code, EN_API_FLOAT_TYPE v)
{
  int   i,j;
  double Ke,n,ucf, value = v;
  if (!m->Openflag)
    return(102);
  switch (code)
  {
    case EN_TRIALS:     if (value < 1.0) return(202);
      m->MaxIter = (int)value;
      break;
    case EN_ACCURACY:   if (value < 1.e-5 || value > 1.e-1) return(202);
      m->Hacc = value;
      break;
    case EN_TOLERANCE:  if (value < 0.0) return(202);
      m->Ctol = value / m->Ucf[QUALITY];
      break;
    case EN_EMITEXPON:  if (value <= 0.0) return(202);
      n = 1.0/value;
      ucf = pow(m->Ucf[FLOW],n) / m->Ucf[PRESSURE];
      for (i=1; i <= m->Njuncs; i++)
      {
        j = OW_getnodevalue(m, i,EN_EMITTER,&v);
        Ke = v;
        if (j == 0 && Ke > 0.0) {
          m->Node[i].Ke = ucf/pow(Ke,n);
        }
      }
      m->Qexp = n;
      break;
    case EN_DEMANDMULT: if (value <= 0.0) {
      return(202);
    }
      m->Dmult = value;
      break;
    default:            return(251);
  }
  return(0);
}


int  DLLEXPORT OW_setstatusreport(OW_Project *m, int code)
{
  int errcode = 0;
  if (code >= 0 && code <= 2) {
    m->Statflag = (char)code;
  }
  else {
    errcode = 202;
  }
  return(errcode);
}


int  DLLEXPORT OW_setqualtype(OW_Project *m, int qualcode, char *chemname, char *chemunits, char *tracenode)
{
  /*** Updated 3/1/01 ***/
  double ccf = 1.0;
  
  if (!m->Openflag) return(102);
  if (qualcode < EN_NONE || qualcode > EN_TRACE) return(251);
  m->Qualflag = (char)qualcode;
  if (m->Qualflag == CHEM)                   /* Chemical constituent */
  {
    strncpy(m->ChemName,chemname,MAXID);
    strncpy(m->ChemUnits,chemunits,MAXID);
    
    /*** Updated 3/1/01 ***/
    strncpy(m->Field[QUALITY].Units, m->ChemUnits,MAXID);
    strncpy(m->Field[REACTRATE].Units, m->ChemUnits,MAXID);
    strcat(m->Field[REACTRATE].Units,t_PERDAY);
    ccf =  1.0/LperFT3;
    
  }
  if (m->Qualflag == TRACE)                  /* Source tracing option */
  {
    m->TraceNode = findnode(m,tracenode);
    if (m->TraceNode == 0) return(203);
    strncpy(m->ChemName,u_PERCENT,MAXID);
    strncpy(m->ChemUnits,tracenode,MAXID);
    
    /*** Updated 3/1/01 ***/
    strcpy(m->Field[QUALITY].Units,u_PERCENT);
  }
  if (m->Qualflag == AGE)                    /* Water age analysis */
  {
    strncpy(m->ChemName,w_AGE,MAXID);
    strncpy(m->ChemUnits,u_HOURS,MAXID);
    
    /*** Updated 3/1/01 ***/
    strcpy(m->Field[QUALITY].Units,u_HOURS);
  }
  
  /*** Updated 3/1/01 ***/
  m->Ucf[QUALITY]   = ccf;
  m->Ucf[LINKQUAL]  = ccf;
  m->Ucf[REACTRATE] = ccf;
  
  return(0);
}


int  DLLEXPORT OW_getqualinfo(OW_Project *m, int *qualcode, char *chemname, char *chemunits, int *tracenode)
{
  OW_getqualtype(m, qualcode, tracenode);
  strncpy(chemname, m->ChemName,MAXID);
  strncpy(chemunits, m->ChemUnits,MAXID);
  return 0;
}


int  DLLEXPORT OW_setbasedemand(OW_Project *m, int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE baseDemand)
{
  Pdemand d;
  int n=1;
  /* Check for valid arguments */
  if (!m->Openflag) return(102);
  if (nodeIndex <= 0 || nodeIndex > m->Nnodes)
    return(203);
  if (nodeIndex <= m->Njuncs) {
    for(d = m->Node[nodeIndex].D; n < demandIdx && d != NULL; d = d->next) {
      n++;
    }
    if(n != demandIdx) {
      return(253);
    }
    d->Base = baseDemand / m->Ucf[FLOW];
  }
  return 0;
}









