#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <math.h>
#include <float.h> //(2.00.12 - LR)

#include "text.h"
#include "types.h"
#include "epanet2.h"
#include "funcs.h"
#define EXTERN
#include "vars.h"

#include <stdio.h>

void (*viewprog)(char *); /* Pointer to progress viewing function */


/*
 ----------------------------------------------------------------
 Functions for opening & closing the EPANET system
 ----------------------------------------------------------------
 */

/*** updated 3/1/01 ***/
int DLLEXPORT ENepanet(char *f1, char *f2, char *f3, void (*pviewprog)(char *))

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
  int errcode = 0;
  viewprog = pviewprog;
  ERRCODE(ENopen(f1, f2, f3));
  if (errcode != EN_OK) {
    return errcode;
  }
  if (en_defaultModel->Hydflag != USE) {
    ERRCODE(ENsolveH());
  }
  ERRCODE(ENsolveQ());
  ERRCODE(ENreport());
  ENclose();
  return (errcode);
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
  EN_Project *newModel;
  err = EN_open(f1, &newModel, f2, f3);
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
  return EN_saveinpfile(en_defaultModel, filename);
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
  return EN_close(en_defaultModel);
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
  return EN_solveH(en_defaultModel);
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
  return EN_saveH(en_defaultModel);
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
  return EN_openH(en_defaultModel);
}

/*** Updated 3/1/01 ***/
int DLLEXPORT ENinitH(int flag)
/*----------------------------------------------------------------
 **  Input:   flag = 2-digit flag where 1st (left) digit indicates
 **                  if link flows should be re-initialized (1) or
 **                  not EN_OK and 2nd digit indicates if hydraulic
 **                  results should be saved to file (1) or not EN_OK
 **  Output:  none
 **  Returns: error code
 **  Purpose: initializes hydraulic analysis
 **----------------------------------------------------------------
 */
{
  return EN_initH(en_defaultModel, flag);
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
  return EN_runH(en_defaultModel, t);
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
  return EN_nextH(en_defaultModel, tstep);
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
  return EN_closeH(en_defaultModel);
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
  return EN_savehydfile(en_defaultModel, filename);
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
  return EN_usehydfile(en_defaultModel, filename);
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
  return EN_solveQ(en_defaultModel);
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
  return EN_openQ(en_defaultModel);
}

int DLLEXPORT ENinitQ(int saveflag)
/*----------------------------------------------------------------
 **  Input:   saveflag = EN_SAVE (1) if results saved to file,
 **                      EN_NOSAVE EN_OK if not
 **  Output:  none
 **  Returns: error code
 **  Purpose: initializes WQ analysis
 **----------------------------------------------------------------
 */
{
  return EN_initQ(en_defaultModel, saveflag);
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
  return EN_runQ(en_defaultModel, t);
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
  return EN_nextQ(en_defaultModel, tstep);
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
  return EN_stepQ(en_defaultModel, tleft);
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
  return EN_closeQ(en_defaultModel);
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
  return EN_writeline(en_defaultModel, line);
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
  return EN_report(en_defaultModel);
}

int DLLEXPORT ENresetreport()
/*----------------------------------------------------------------
 **  Input:   none
 **  Output:  none
 **  Returns: error code
 **  Purpose: resets report options to default values
 **----------------------------------------------------------------
 */
{
  return EN_resetreport(en_defaultModel);
}

int DLLEXPORT ENsetreport(char *s)
/*----------------------------------------------------------------
 **  Input:   s = report format command
 **  Output:  none
 **  Returns: error code
 **  Purpose: processes a reporting format command
 **----------------------------------------------------------------
 */
{
  return EN_setreport(en_defaultModel, s);
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
  return EN_getversion(v);
}

int DLLEXPORT
ENgetcontrol(int cindex, int *ctype, int *lindex, EN_API_FLOAT_TYPE *setting,
             int *nindex, EN_API_FLOAT_TYPE *level)
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
  return EN_getcontrol(en_defaultModel, cindex, ctype, lindex, setting, nindex,
                       level);
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
  return EN_getcount(en_defaultModel, code, count);
}

int DLLEXPORT ENgetoption(int code, EN_API_FLOAT_TYPE *value)
/*----------------------------------------------------------------
 **  Input:   code = option code (see TOOLKIT.H)
 **  Output:  *value = option value
 **  Returns: error code
 **  Purpose: gets value for an analysis option
 **----------------------------------------------------------------
 */
{
  return EN_getoption(en_defaultModel, code, value);
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
  return EN_gettimeparam(en_defaultModel, code, value);
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
  return EN_getflowunits(en_defaultModel, code);
}

int DLLEXPORT ENgetpatternindex(char *id, int *index)
/*----------------------------------------------------------------
 **  Input:   id     = time pattern ID
 **  Output:  *index = index of time pattern in list of patterns
 **  Returns: error code
 **  Purpose: retrieves index of time pattern with specific ID
 **----------------------------------------------------------------
 */
{
  return EN_getpatternindex(en_defaultModel, id, index);
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
  return EN_getpatternid(en_defaultModel, index, id);
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
  return EN_getpatternlen(en_defaultModel, index, len);
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
  return EN_getpatternvalue(en_defaultModel, index, period, value);
}

int DLLEXPORT ENgetqualtype(int *qualcode, int *tracenode)
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
  return EN_getqualtype(en_defaultModel, qualcode, tracenode);
}

int DLLEXPORT
ENgetqualinfo(int *qualcode, char *chemname, char *chemunits, int *tracenode) {
  return EN_getqualinfo(en_defaultModel, qualcode, chemname, chemunits,
                        tracenode);
}

int DLLEXPORT ENgeterror(int errcode, char *errmsg, int n)
/*----------------------------------------------------------------
 **  Input:   errcode = error/warning code number
 **           n       = maximum length of string errmsg
 **  Output:  errmsg  = text of error/warning message
 **  Returns: error code
 **  Purpose: retrieves text of error/warning message
 **----------------------------------------------------------------
 */
{
  return EN_geterror(errcode, errmsg, n);
}

int DLLEXPORT ENgetstatistic(int code, EN_API_FLOAT_TYPE *value)
/*----------------------------------------------------------------
 **  Input:   code    = type of simulation statistic to retrieve
 **  Output:  value   = value of requested statistic
 **  Returns: error code
 **  Purpose: retrieves hydraulic simulation statistic
 **----------------------------------------------------------------
 */
{
  return EN_getstatistic(en_defaultModel, code, value);
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
  return EN_getnodeindex(en_defaultModel, id, index);
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
  return EN_getnodeid(en_defaultModel, index, id);
}

int DLLEXPORT ENgetnodetype(int index, EN_NodeType *code)
/*----------------------------------------------------------------
 **  Input:   index = node index
 **  Output:  *code = node type code number (see TOOLKIT.H)
 **  Returns: error code
 **  Purpose: retrieves node type of specific node
 **----------------------------------------------------------------
 */
{
  return EN_getnodetype(en_defaultModel, index, code);
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
  return EN_getcoord(en_defaultModel, index, x, y);
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
  return EN_getnodevalue(en_defaultModel, index, code, value);
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
  return EN_getlinkindex(en_defaultModel, id, index);
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
  return EN_getlinkid(en_defaultModel, index, id);
}

int DLLEXPORT ENgetlinktype(int index, int *code)
/*------------------------------------------------------------------
 **  Input:   index = link index
 **  Output:  *code = link type code number (see TOOLKIT.H)
 **  Returns: error code
 **  Purpose: retrieves link type of specific link
 **------------------------------------------------------------------
 */
{
  return EN_getlinktype(en_defaultModel, index, code);
}

int DLLEXPORT ENgetlinknodes(int index, int *node1, int *node2)
/*----------------------------------------------------------------
 **  Input:   index = link index
 **  Output:  *node1 = index of link's starting node
 **           *node2 = index of link's ending node
 **  Returns: error code
 **  Purpose: retrieves end nodes of a specific link
 **----------------------------------------------------------------
 */
{
  return EN_getlinknodes(en_defaultModel, index, node1, node2);
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
  return EN_getlinkvalue(en_defaultModel, index, code, value);
}

int DLLEXPORT
ENgetcurve(int curveIndex, char *id, int *nValues, EN_API_FLOAT_TYPE **xValues,
           EN_API_FLOAT_TYPE **yValues)
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
  return EN_getcurve(en_defaultModel, curveIndex, id, nValues, xValues,
                     yValues);
}

/*
 ----------------------------------------------------------------
 Functions for changing network data
 ----------------------------------------------------------------
 */

int DLLEXPORT
ENsetcontrol(int cindex, int ctype, int lindex, EN_API_FLOAT_TYPE setting,
             int nindex, EN_API_FLOAT_TYPE level)
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
  return EN_setcontrol(en_defaultModel, cindex, ctype, lindex, setting, nindex,
                       level);
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
  return EN_setnodevalue(en_defaultModel, index, code, v);
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
  return EN_setlinkvalue(en_defaultModel, index, code, v);
}

int DLLEXPORT ENaddpattern(char *id)
/*----------------------------------------------------------------
 **   Input:   id = ID name of the new pattern
 **   Output:  none
 **   Returns: error code
 **   Purpose: adds a new time pattern appended to the end of the
 **            existing patterns.
 **----------------------------------------------------------------
 */
{
  return EN_addpattern(en_defaultModel, id);
}

int DLLEXPORT ENsetpattern(int index, EN_API_FLOAT_TYPE *f, int n)
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
  return EN_setpattern(en_defaultModel, index, f, n);
}

int DLLEXPORT ENsetpatternvalue(int index, int period, EN_API_FLOAT_TYPE value)
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
  return EN_setpatternvalue(en_defaultModel, index, period, value);
}

int DLLEXPORT ENsettimeparam(int code, long value)
/*----------------------------------------------------------------
 **  Input:   code  = time parameter code (see TOOLKIT.H)
 **           value = time parameter value
 **  Output:  none
 **  Returns: error code
 **  Purpose: sets value for time parameter
 **----------------------------------------------------------------
 */
{
  return EN_settimeparam(en_defaultModel, code, value);
}

int DLLEXPORT ENsetoption(int code, EN_API_FLOAT_TYPE v)
/*----------------------------------------------------------------
 **  Input:   code  = option code (see TOOLKIT.H)
 **           v = option value
 **  Output:  none
 **  Returns: error code
 **  Purpose: sets value for an analysis option
 **----------------------------------------------------------------
 */
{
  return EN_setoption(en_defaultModel, code, v);
}

int DLLEXPORT ENsetstatusreport(int code)
/*----------------------------------------------------------------
 **  Input:   code = status reporting code (0, 1, or 2)
 **  Output:  none
 **  Returns: error code
 **  Purpose: sets level of hydraulic status reporting
 **----------------------------------------------------------------
 */
{
  return EN_setstatusreport(en_defaultModel, code);
}

int DLLEXPORT
ENsetqualtype(int qualcode, char *chemname, char *chemunits, char *tracenode)
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
  return EN_setqualtype(en_defaultModel, qualcode, chemname, chemunits,
                        tracenode);
}

int DLLEXPORT ENgetnumdemands(int nodeIndex, int *numDemands) {
  return EN_getnumdemands(en_defaultModel, nodeIndex, numDemands);
}

int DLLEXPORT
ENgetbasedemand(int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE *baseDemand) {
  return EN_getbasedemand(en_defaultModel, nodeIndex, demandIdx, baseDemand);
}

int DLLEXPORT
ENsetbasedemand(int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE baseDemand) {
  return EN_setbasedemand(en_defaultModel, nodeIndex, demandIdx, baseDemand);
}

int DLLEXPORT ENgetdemandpattern(int nodeIndex, int demandIdx, int *pattIdx) {
  return EN_getdemandpattern(en_defaultModel, nodeIndex, demandIdx, pattIdx);
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
  return EN_getaveragepatternvalue(en_defaultModel, index, value);
}

//====================================================//
//=== end of <=2.00.13 api functions           =======//
//====================================================//
