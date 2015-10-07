/*
 *******************************************************************
 
 EPANET2.H - Prototypes for EPANET Functions Exported to DLL Toolkit
 
 VERSION:    2.00
 DATE:       5/8/00
 10/25/00
 3/1/01
 8/15/07    (2.00.11)
 2/14/08    (2.00.12)
 AUTHORS:     L. Rossman - US EPA - NRMRL
              OpenWaterAnalytics members: see git stats for contributors
 
 *******************************************************************
 */

#ifndef EPANET2_H
#define EPANET2_H

#ifndef EN_API_FLOAT_TYPE
#define EN_API_FLOAT_TYPE float
#endif

#ifndef DLLEXPORT
#ifdef DLL
#ifdef __cplusplus
#define DLLEXPORT extern "C" __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllexport)
#endif
#elif defined(CYGWIN)
#define DLLEXPORT __stdcall
#else
#ifdef __cplusplus
#define DLLEXPORT
#else
#define DLLEXPORT
#endif
#endif
#endif
// --- Define the EPANET toolkit constants

#define EN_ELEVATION    0    /* Node parameters */
#define EN_BASEDEMAND   1
#define EN_PATTERN      2
#define EN_EMITTER      3
#define EN_INITQUAL     4
#define EN_SOURCEQUAL   5
#define EN_SOURCEPAT    6
#define EN_SOURCETYPE   7
#define EN_TANKLEVEL    8
#define EN_DEMAND       9
#define EN_HEAD         10
#define EN_PRESSURE     11
#define EN_QUALITY      12
#define EN_SOURCEMASS   13
#define EN_INITVOLUME   14
#define EN_MIXMODEL     15
#define EN_MIXZONEVOL   16

#define EN_TANKDIAM     17
#define EN_MINVOLUME    18
#define EN_VOLCURVE     19
#define EN_MINLEVEL     20
#define EN_MAXLEVEL     21
#define EN_MIXFRACTION  22
#define EN_TANK_KBULK   23
#define EN_TANKVOLUME   24
#define EN_MAXVOLUME    25

#define EN_DIAMETER     0    /* Link parameters */
#define EN_LENGTH       1
#define EN_ROUGHNESS    2
#define EN_MINORLOSS    3
#define EN_INITSTATUS   4
#define EN_INITSETTING  5
#define EN_KBULK        6
#define EN_KWALL        7
#define EN_FLOW         8
#define EN_VELOCITY     9
#define EN_HEADLOSS     10
#define EN_STATUS       11
#define EN_SETTING      12
#define EN_ENERGY       13
#define EN_LINKQUAL     14     /* TNT */

#define EN_DURATION     0    /* Time parameters */
#define EN_HYDSTEP      1
#define EN_QUALSTEP     2
#define EN_PATTERNSTEP  3
#define EN_PATTERNSTART 4
#define EN_REPORTSTEP   5
#define EN_REPORTSTART  6
#define EN_RULESTEP     7
#define EN_STATISTIC    8
#define EN_PERIODS      9
#define EN_STARTTIME    10
#define EN_HTIME        11
#define EN_QTIME        12
#define EN_HALTFLAG     13

typedef enum {
  EN_STEP_REPORT       = 0,
  EN_STEP_HYD          = 1,
  EN_STEP_WQ           = 2,
  EN_STEP_TANKEVENT    = 3,
  EN_STEP_CONTROLEVENT = 4
} EN_TimestepEvent;

#define EN_ITERATIONS     0
#define EN_RELATIVEERROR  1

#define EN_NODECOUNT    0   /* Component counts */
#define EN_TANKCOUNT    1
#define EN_LINKCOUNT    2
#define EN_PATCOUNT     3
#define EN_CURVECOUNT   4
#define EN_CONTROLCOUNT 5
#define EN_RULECOUNT    6

typedef enum {
  EN_JUNCTION  = 0,
  EN_RESERVOIR = 1,
  EN_TANK      = 2
} EN_NodeType;


typedef enum {
  EN_CVPIPE       = 0,    /* Link types. */
  EN_PIPE         = 1,    /* See LinkType in TYPES.H */
  EN_PUMP         = 2,
  EN_PRV          = 3,
  EN_PSV          = 4,
  EN_PBV          = 5,
  EN_FCV          = 6,
  EN_TCV          = 7,
  EN_GPV          = 8
} EN_LinkType;

#define EN_NONE         0    /* Quality analysis types. */
#define EN_CHEM         1    /* See QualType in TYPES.H */
#define EN_AGE          2
#define EN_TRACE        3

#define EN_CONCEN       0    /* Source quality types.      */
#define EN_MASS         1    /* See SourceType in TYPES.H. */
#define EN_SETPOINT     2
#define EN_FLOWPACED    3

#define EN_CFS          0    /* Flow units types.   */
#define EN_GPM          1    /* See FlowUnitsType   */
#define EN_MGD          2    /* in TYPES.H.         */
#define EN_IMGD         3
#define EN_AFD          4
#define EN_LPS          5
#define EN_LPM          6
#define EN_MLD          7
#define EN_CMH          8
#define EN_CMD          9

#define EN_TRIALS       0   /* Misc. options */
#define EN_ACCURACY     1
#define EN_TOLERANCE    2
#define EN_EMITEXPON    3
#define EN_DEMANDMULT   4
#define EN_HEADLOSSFORMULA  5

#define EN_LOWLEVEL     0   /* Control types.  */
#define EN_HILEVEL      1   /* See ControlType */
#define EN_TIMER        2   /* in TYPES.H.     */
#define EN_TIMEOFDAY    3

#define EN_AVERAGE      1   /* Time statistic types.    */
#define EN_MINIMUM      2   /* See TstatType in TYPES.H */
#define EN_MAXIMUM      3
#define EN_RANGE        4

#define EN_MIX1         0   /* Tank mixing models */
#define EN_MIX2         1
#define EN_FIFO         2
#define EN_LIFO         3

#define EN_NOSAVE       0   /* Save-results-to-file flag */
#define EN_SAVE         1

#define EN_DISABLE      0
#define EN_ENABLE       1

#define EN_INITFLOW    10   /* Re-initialize flows flag  */


#pragma mark - error codes

#define EN_OK                0
#define OW_ERR_INSUFFICIENT_MEMORY 101
#define OW_ERR_NO_DATA 102
#define OW_ERR_HYDRAULICS_NOT_INITIALIZED 103
#define OW_ERR_NO_HYDRAULICS 104
#define OW_ERR_WQ_NOT_INITIALIZED 105
#define OW_ERR_NO_RESULTS_SAVED 106
#define OW_ERR_HYDRUAULICS_EXT_FILE 107
#define OW_ERR_CANT_USE_EXT_FILE 108
#define OW_ERR_CANT_CHANGE_TIME_PARAM 109
#define OW_ERR_CANT_SOLVE_HYD 110
#define OW_ERR_CANT_SOLVE_WQ 120

#define OW_ERR_INPUT_FILE_ERROR 200
#define OW_ERR_SYNTAX 201
#define OW_ERR_ILLEGAL_NUMERIC_VALUE 202
#define OW_ERR_UNDEF_NODE 203
#define OW_ERR_UNDEF_LINK 204
#define OW_ERR_UNDEF_TIME_PAT 205
#define OW_ERR_UNDEF_CURVE 206
#define OW_ERR_CONTROL_CV 207

#define OW_ERR_SPEC_UNDEF_NODE 208
#define OW_ERR_ILLEGAL_VAL_NODE 209
#define OW_ERR_SPEC_UNDEF_LINK 210
#define OW_ERR_ILLEGAL_VAL_LINK 211
#define OW_ERR_UNDEF_TRACE_NODE 212
#define OW_ERR_ILLEGAL_OPTION 213
#define OW_ERR_TOO_MANY_CHARACTERS 214
#define OW_ERR_DUPLICATE_ID 215
#define OW_ERR_DATA_UNDEF_PUMP 216
#define OW_ERR_DATA_INVALID_PUMP 217
#define OW_ERR_ILLEGAL_TANK_CONN 219
#define OW_ERR_ILLEGAL_VALVE_CONN 220

/*** Updated on 10/25/00 ***/
#define OW_ERR_SAME_START_END_NODES 222

#define OW_ERR_NOT_ENOUGH_NODES 223
#define OW_ERR_NO_TANKS 224
#define OW_ERR_INVALID_TANK_LEVELS 225
#define OW_ERR_NO_HEAD_CURVE 226
#define OW_ERR_INV_HEAD_CURVE 227
#define OW_ERR_CURVE_NONINCREASE 230
#define OW_ERR_NODE_UNCONNECTED 233
#define OW_ERR_UNDEF_SOURCE 240
#define OW_ERR_UNDEF_CONTROL 241
#define OW_ERR_FN_INVALID_FORMAT 250
#define OW_ERR_FN_INVALID_CODE 251

#define OW_ERR_FILE_IDENTICAL 301
#define OW_ERR_FILE_CANT_OPEN_INP 302
#define OW_ERR_FILE_CANT_OPEN_RPT 303
#define OW_ERR_FILE_CANT_OPEN_BIN 304
/*
 #define OW_ERR_FILE 305 "File Error 305: cannot open hydraulics file."
 #define OW_ERR_ 306 "File Error 306: hydraulics file does not match network data."
 #define OW_ERR_ 307 "File Error 307: cannot read hydraulics file."
 #define OW_ERR_ 308 "File Error 308: cannot save results to file."
 #define OW_ERR_ 309 "File Error 309: cannot save results to report file."
 
 #define OW_ERR_ 401 "Sync Error 401: Qstep is not dividable by Hstep. Can't sync."
 
 #define R_ERR201 "Input Error 201: syntax error in following line of "
 #define R_ERR202 "Input Error 202: illegal numeric value in following line of "
 #define R_ERR203 "Input Error 203: undefined node in following line of "
 #define R_ERR204 "Input Error 204: undefined link in following line of "
 #define R_ERR207 "Input Error 207: attempt to control a CV in following line of "
 
 #define R_ERR221 "Input Error 221: mis-placed clause in following line of "
 */


#pragma mark - API

// --- Declare the EPANET toolkit functions
#if defined(__cplusplus)
extern "C" {
#endif
  
  typedef struct OW_Project OW_Project;
  
  // OLD (<= 2.1) api functions use the global default model pointer.
  int  DLLEXPORT ENepanet(char *inpFile, char *rptFile, char *binOutFile, void (*callback) (char *));
  
  int  DLLEXPORT ENopen(char *inpFile, char *rptFile, char *binOutFile);
  int  DLLEXPORT ENsaveinpfile(char *filename);
  int  DLLEXPORT ENclose();
  
  int  DLLEXPORT ENsolveH();
  int  DLLEXPORT ENsaveH();
  int  DLLEXPORT ENopenH();
  int  DLLEXPORT ENinitH(int initFlag);
  int  DLLEXPORT ENrunH(long *currentTime);
  int  DLLEXPORT ENnextH(long *tStep);
  int  DLLEXPORT ENcloseH();
  int  DLLEXPORT ENsavehydfile(char *filename);
  int  DLLEXPORT ENusehydfile(char *filename);
  
  int  DLLEXPORT ENsolveQ();
  int  DLLEXPORT ENopenQ();
  int  DLLEXPORT ENinitQ(int saveFlag);
  int  DLLEXPORT ENrunQ(long *currentTime);
  int  DLLEXPORT ENnextQ(long *tStep);
  int  DLLEXPORT ENstepQ(long *timeLeft);
  int  DLLEXPORT ENcloseQ();
  
  int  DLLEXPORT ENwriteline(char *line);
  int  DLLEXPORT ENreport();
  int  DLLEXPORT ENresetreport();
  int  DLLEXPORT ENsetreport(char *reportFormat);
  
  int  DLLEXPORT ENgetcontrol(int controlIndex, int *controlType, int *linkIdx, EN_API_FLOAT_TYPE *setting, int *nodeIdx, EN_API_FLOAT_TYPE *level);
  int  DLLEXPORT ENgetcount(int code, int *count);
  int  DLLEXPORT ENgetoption(int code, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT ENgettimeparam(int code, long *value);
  int  DLLEXPORT ENgetflowunits(int *code);
  int  DLLEXPORT ENgetpatternindex(char *id, int *index);
  int  DLLEXPORT ENgetpatternid(int index, char *id);
  int  DLLEXPORT ENgetpatternlen(int index, int *len);
  int  DLLEXPORT ENgetpatternvalue(int index, int period, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT ENgetaveragepatternvalue(int index, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT ENgetqualtype(int *qualcode, int *tracenode);
  int  DLLEXPORT ENgeterror(int errcode, char *errmsg, int maxLen);
  int  DLLEXPORT ENgetstatistic(int code, EN_API_FLOAT_TYPE* value);
  
  int  DLLEXPORT ENgetnodeindex(char *id, int *index);
  int  DLLEXPORT ENgetnodeid(int index, char *id);
  int  DLLEXPORT ENgetnodetype(int index, EN_NodeType *code);
  int  DLLEXPORT ENgetnodevalue(int index, int code, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT ENgetcoord(int index, EN_API_FLOAT_TYPE *x, EN_API_FLOAT_TYPE *y);
  
  int  DLLEXPORT ENgetnumdemands(int nodeIndex, int *numDemands);
  int  DLLEXPORT ENgetbasedemand(int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE *baseDemand);
  int  DLLEXPORT ENgetdemandpattern(int nodeIndex, int demandIdx, int *pattIdx);
  
  int  DLLEXPORT ENgetlinkindex(char *id, int *index);
  int  DLLEXPORT ENgetlinkid(int index, char *id);
  int  DLLEXPORT ENgetlinktype(int index, int *code);
  int  DLLEXPORT ENgetlinknodes(int index, int *node1, int *node2);
  int  DLLEXPORT ENgetlinkvalue(int index, int code, EN_API_FLOAT_TYPE *value);
  
  int  DLLEXPORT ENgetcurve(int curveIndex, char* id, int *nValues, EN_API_FLOAT_TYPE **xValues, EN_API_FLOAT_TYPE **yValues);

  
  int  DLLEXPORT ENgetversion(int *version);
  
  int  DLLEXPORT ENsetcontrol(int cindex, int ctype, int lindex, EN_API_FLOAT_TYPE setting, int nindex, EN_API_FLOAT_TYPE level);
  int  DLLEXPORT ENsetnodevalue(int index, int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT ENsetlinkvalue(int index, int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT ENaddpattern(char *id);
  int  DLLEXPORT ENsetpattern(int index, EN_API_FLOAT_TYPE *f, int len);
  int  DLLEXPORT ENsetpatternvalue(int index, int period, EN_API_FLOAT_TYPE value);
  int  DLLEXPORT ENsettimeparam(int code, long value);
  int  DLLEXPORT ENsetoption(int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT ENsetstatusreport(int code);
  int  DLLEXPORT ENsetqualtype(int qualcode, char *chemname, char *chemunits, char *tracenode);
  int  DLLEXPORT ENgetqualinfo(int *qualcode, char *chemname, char *chemunits, int *tracenode);
  int  DLLEXPORT ENsetbasedemand(int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE baseDemand);
  
#pragma mark - Threadsafe API (>2.2) functions
  

  // NEW (>2.1) api functions are threadsafe. the have the same name format, except with a different prefix and underscore ("OW_[...]")
  int  DLLEXPORT OW_newModel(OW_Project **modelObj);
  int  DLLEXPORT OW_freeModel(OW_Project *modelObj);
  
  // Project Management
  /*----------------------------------------------------------------
   **  Input:   f1 = pointer to name of input file
   **           f2 = pointer to name of report file
   **           f3 = pointer to name of binary output file
   **  Output:  none
   **  Returns: error code
   **  Purpose: opens EPANET input file & reads in network data
   **----------------------------------------------------------------
   */
  /**
   @fn OW_open(char *inpFile, OW_Project **modelObj, char *rptFile, char *binOutFile)
   @brief Open an epanet input file and create a model object.
   
   @param inpFile The path to the *.inp formatted text file
   @param modelObj Output: model object pointer
   @param rptFile The path to a report file (will be created, optional) 
   @param binOutFile The path to the binary output file (will be created, optional)
   */
  int  DLLEXPORT OW_open(char *inpFile, OW_Project **modelObj, char *rptFile, char *binOutFile);
  int  DLLEXPORT OW_saveinpfile(OW_Project *modelObj, const char *filename);
  int  DLLEXPORT OW_close(OW_Project *modelObj);
  
  // Hydraulic solver
  int  DLLEXPORT OW_solveH(OW_Project *modelObj);
  int  DLLEXPORT OW_saveH(OW_Project *modelObj);
  int  DLLEXPORT OW_openH(OW_Project *modelObj);
  int  DLLEXPORT OW_initH(OW_Project *modelObj, int initFlag);
  int  DLLEXPORT OW_runH(OW_Project *modelObj, long *currentTime);
  int  DLLEXPORT OW_nextH(OW_Project *modelObj, long *tStep);
  int  DLLEXPORT OW_closeH(OW_Project *modelObj);
  int  DLLEXPORT OW_savehydfile(OW_Project *modelObj, char *filename);
  int  DLLEXPORT OW_usehydfile(OW_Project *modelObj, char *filename);
  int  DLLEXPORT OW_getstatistic(OW_Project *modelObj, int code, EN_API_FLOAT_TYPE* value);
  int  DLLEXPORT OW_timeToNextEvent(OW_Project *modelObj, EN_TimestepEvent *eventType, long *duration, int *elementIndex);
  
  // Water Quality solver
  int  DLLEXPORT OW_solveQ(OW_Project *modelObj);
  int  DLLEXPORT OW_openQ(OW_Project *modelObj);
  int  DLLEXPORT OW_initQ(OW_Project *modelObj, int saveFlag);
  int  DLLEXPORT OW_runQ(OW_Project *modelObj, long *currentTime);
  int  DLLEXPORT OW_nextQ(OW_Project *modelObj, long *tStep);
  int  DLLEXPORT OW_stepQ(OW_Project *modelObj, long *timeLeft);
  int  DLLEXPORT OW_closeQ(OW_Project *modelObj);
  int  DLLEXPORT OW_setqualtype(OW_Project *modelObj, int qualcode, char *chemname, char *chemunits, char *tracenode);
  int  DLLEXPORT OW_getqualinfo(OW_Project *modelObj, int *qualcode, char *chemname, char *chemunits, int *tracenode);
  
  // Reporting
  int  DLLEXPORT OW_setReportCallback(OW_Project *m, void (*callback)(void *userData, OW_Project*,char*)); /**< set a callback function for logging. by default OW_writeline */
  int  DLLEXPORT OW_setReportCallbackUserData(OW_Project *m, void *userData);
  int  DLLEXPORT OW_writeline(OW_Project *modelObj, char *line);
  int  DLLEXPORT OW_report(OW_Project *modelObj);
  int  DLLEXPORT OW_resetreport(OW_Project *modelObj);
  int  DLLEXPORT OW_setreport(OW_Project *modelObj, char *reportFormat);
  int  DLLEXPORT OW_setstatusreport(OW_Project *modelObj, int code);
  
  // Get/Set basic network info
  int  DLLEXPORT OW_getcount(OW_Project *modelObj, int code, int *count);
  int  DLLEXPORT OW_getoption(OW_Project *modelObj, int code, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT OW_setoption(OW_Project *modelObj, int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT OW_gettimeparam(OW_Project *modelObj, int code, long *value);
  int  DLLEXPORT OW_settimeparam(OW_Project *modelObj, int code, long value);
  int  DLLEXPORT OW_getflowunits(OW_Project *modelObj, int *code);
  int  DLLEXPORT OW_getqualtype(OW_Project *modelObj, int *qualcode, int *tracenode);
  int  DLLEXPORT OW_geterror(int errcode, char *errmsg, int maxLen);
  int  DLLEXPORT OW_getversion(int *version);
  
  // Controls & Rules
  int  DLLEXPORT OW_controlEnabled(OW_Project *modelObj, int controlIndex);
  int  DLLEXPORT OW_setControlEnabled(OW_Project *modelObj, int controlIndex, int enable);
  int  DLLEXPORT OW_setcontrol(OW_Project *modelObj, int cindex, int ctype, int lindex, EN_API_FLOAT_TYPE setting, int nindex, EN_API_FLOAT_TYPE level);
  int  DLLEXPORT OW_getcontrol(OW_Project *modelObj, int controlIndex, int *controlType, int *linkIdx, EN_API_FLOAT_TYPE *setting, int *nodeIdx, EN_API_FLOAT_TYPE *level);
  
  int  DLLEXPORT OW_ruleEnabled(OW_Project *modelObj, int ruleIndex);
  int  DLLEXPORT OW_setRuleEnabled(OW_Project *modelObj, int ruleIndex, int enable);
  int  DLLEXPORT OW_getRuleName(OW_Project *modelObj, int ruleIndex, char* id);
  int  DLLEXPORT OW_getRuleAffectedLinks(OW_Project *m, int ruleIndex, int* nLinks, int* linkIndexes);
  int  DLLEXPORT OW_freeRuleAffectedLinks(int* linkIndexes);
  
  // Patterns
  int  DLLEXPORT OW_getpatternindex(OW_Project *modelObj, char *id, int *index);
  int  DLLEXPORT OW_getpatternid(OW_Project *modelObj, int index, char *id);
  int  DLLEXPORT OW_getpatternlen(OW_Project *modelObj, int index, int *len);
  int  DLLEXPORT OW_getpatternvalue(OW_Project *modelObj, int index, int period, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT OW_getaveragepatternvalue(OW_Project *modelObj, int index, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT OW_addpattern(OW_Project *modelObj, char *id);
  int  DLLEXPORT OW_setpattern(OW_Project *modelObj, int index, EN_API_FLOAT_TYPE *f, int len);
  int  DLLEXPORT OW_setpatternvalue(OW_Project *modelObj, int index, int period, EN_API_FLOAT_TYPE value);
  
  // Node elements
  int  DLLEXPORT OW_getnodeindex(OW_Project *modelObj, char *id, int *index);
  int  DLLEXPORT OW_getnodeid(OW_Project *modelObj, int index, char *id);
  int  DLLEXPORT OW_getnodetype(OW_Project *modelObj, int index, EN_NodeType *code);
  int  DLLEXPORT OW_getnodevalue(OW_Project *modelObj, int index, int code, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT OW_setnodevalue(OW_Project *modelObj, int index, int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT OW_getnodecomment(OW_Project *modelObj, int nIndex, char *comment);
  int  DLLEXPORT OW_setnodecomment(OW_Project *modelObj, int nIndex, const char *comment);
  int  DLLEXPORT OW_getcoord(OW_Project *modelObj, int index, EN_API_FLOAT_TYPE *x, EN_API_FLOAT_TYPE *y);
  
  // Demands
  int  DLLEXPORT OW_getnumdemands(OW_Project *modelObj, int nodeIndex, int *numDemands);
  int  DLLEXPORT OW_getbasedemand(OW_Project *modelObj, int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE *baseDemand);
  int  DLLEXPORT OW_getdemandpattern(OW_Project *modelObj, int nodeIndex, int demandIdx, int *pattIdx);
  int  DLLEXPORT OW_setbasedemand(OW_Project *modelObj, int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE baseDemand);
  
  // Link elements
  int  DLLEXPORT OW_getlinkindex(OW_Project *modelObj, char *id, int *index);
  int  DLLEXPORT OW_getlinkid(OW_Project *modelObj, int index, char *id);
  int  DLLEXPORT OW_getlinktype(OW_Project *modelObj, int index, EN_LinkType *type);
  int  DLLEXPORT OW_getlinknodes(OW_Project *modelObj, int index, int *node1, int *node2);
  int  DLLEXPORT OW_getlinkvalue(OW_Project *modelObj, int index, int code, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT OW_setlinkvalue(OW_Project *modelObj, int index, int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT OW_getlinkcomment(OW_Project *modelObj, int linkIndex, char *comment);
  int  DLLEXPORT OW_setlinkcomment(OW_Project *modelObj, int linkIndex, const char *comment);
  
  // Curves
  int  DLLEXPORT OW_getcurve(OW_Project *modelObj, int curveIndex, char* id, int *nValues, EN_API_FLOAT_TYPE **xValues, EN_API_FLOAT_TYPE **yValues);
  
  
  // network creation api set
  
  int DLLEXPORT OW_newNetwork(OW_Project **modelObj);
  int DLLEXPORT OW_startEditingNetwork(OW_Project *modelObj);
  int DLLEXPORT OW_addNode(OW_Project *modelObj, EN_NodeType type, char *name);
  int DLLEXPORT OW_addLink(OW_Project *modelObj, EN_LinkType type, char *name, char *upstreamNode, char* downstreamNode);
  int DLLEXPORT OW_stopEditingNetwork(OW_Project *modelObj);
  
#if defined(__cplusplus)
}
#endif

#endif //TOOLKIT_H