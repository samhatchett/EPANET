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
#define EN_INLETQUALITY 26

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
#define EN_HEADCURVE    15
#define EN_EFFICIENCYCURVE 16

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
#define EN_ERR_INSUFFICIENT_MEMORY 101
#define EN_ERR_NO_DATA 102
#define EN_ERR_HYDRAULICS_NOT_INITIALIZED 103
#define EN_ERR_NO_HYDRAULICS 104
#define EN_ERR_WQ_NOT_INITIALIZED 105
#define EN_ERR_NO_RESULTS_SAVED 106
#define EN_ERR_HYDRUAULICS_EXT_FILE 107
#define EN_ERR_CANT_USE_EXT_FILE 108
#define EN_ERR_CANT_CHANGE_TIME_PARAM 109
#define EN_ERR_CANT_SOLVE_HYD 110
#define EN_ERR_CANT_SOLVE_WQ 120

#define EN_ERR_INPUT_FILE_ERROR 200
#define EN_ERR_SYNTAX 201
#define EN_ERR_ILLEGAL_NUMERIC_VALUE 202
#define EN_ERR_UNDEF_NODE 203
#define EN_ERR_UNDEF_LINK 204
#define EN_ERR_UNDEF_TIME_PAT 205
#define EN_ERR_UNDEF_CURVE 206
#define EN_ERR_CONTROL_CV 207

#define EN_ERR_SPEC_UNDEF_NODE 208
#define EN_ERR_ILLEGAL_VAL_NODE 209
#define EN_ERR_SPEC_UNDEF_LINK 210
#define EN_ERR_ILLEGAL_VAL_LINK 211
#define EN_ERR_UNDEF_TRACE_NODE 212
#define EN_ERR_ILLEGAL_OPTION 213
#define EN_ERR_TOO_MANY_CHARACTERS 214
#define EN_ERR_DUPLICATE_ID 215
#define EN_ERR_DATA_UNDEF_PUMP 216
#define EN_ERR_DATA_INVALID_PUMP 217
#define EN_ERR_ILLEGAL_TANK_CONN 219
#define EN_ERR_ILLEGAL_VALVE_CONN 220

/*** Updated on 10/25/00 ***/
#define EN_ERR_SAME_START_END_NODES 222

#define EN_ERR_NOT_ENOUGH_NODES 223
#define EN_ERR_NO_TANKS 224
#define EN_ERR_INVALID_TANK_LEVELS 225
#define EN_ERR_NO_HEAD_CURVE 226
#define EN_ERR_INV_HEAD_CURVE 227
#define EN_ERR_CURVE_NONINCREASE 230
#define EN_ERR_NODE_UNCONNECTED 233
#define EN_ERR_UNDEF_SOURCE 240
#define EN_ERR_UNDEF_CONTROL 241
#define EN_ERR_FN_INVALID_FORMAT 250
#define EN_ERR_FN_INVALID_CODE 251
#define EN_ERR_NO_EFF_CURVE 268

#define EN_ERR_FILE_IDENTICAL 301
#define EN_ERR_FILE_CANT_OPEN_INP 302
#define EN_ERR_FILE_CANT_OPEN_RPT 303
#define EN_ERR_FILE_CANT_OPEN_BIN 304
/*
 #define EN_ERR_FILE 305 "File Error 305: cannot open hydraulics file."
 #define EN_ERR_ 306 "File Error 306: hydraulics file does not match network data."
 #define EN_ERR_ 307 "File Error 307: cannot read hydraulics file."
 #define EN_ERR_ 308 "File Error 308: cannot save results to file."
 #define EN_ERR_ 309 "File Error 309: cannot save results to report file."
 
 #define EN_ERR_ 401 "Sync Error 401: Qstep is not dividable by Hstep. Can't sync."
 
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
  
  typedef struct EN_Project EN_Project;
  
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
  

  // NEW (>2.1) api functions are threadsafe. the have the same name format, except with a different prefix and underscore ("EN_[...]")
  int  DLLEXPORT EN_newModel(EN_Project **modelObj);
  int  DLLEXPORT EN_freeModel(EN_Project *modelObj);
  
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
   @fn EN_open(char *inpFile, EN_Project **modelObj, char *rptFile, char *binOutFile)
   @brief Open an epanet input file and create a model object.
   
   @param inpFile The path to the *.inp formatted text file
   @param modelObj Output: model object pointer
   @param rptFile The path to a report file (will be created, optional) 
   @param binOutFile The path to the binary output file (will be created, optional)
   */
  int  DLLEXPORT EN_open(char *inpFile, EN_Project **modelObj, char *rptFile, char *binOutFile);
  int  DLLEXPORT EN_saveinpfile(EN_Project *modelObj, char *filename);
  int  DLLEXPORT EN_close(EN_Project *modelObj);
  
  // Hydraulic solver
  int  DLLEXPORT EN_solveH(EN_Project *modelObj);
  int  DLLEXPORT EN_saveH(EN_Project *modelObj);
  int  DLLEXPORT EN_openH(EN_Project *modelObj);
  int  DLLEXPORT EN_initH(EN_Project *modelObj, int initFlag);
  int  DLLEXPORT EN_runH(EN_Project *modelObj, long *currentTime);
  int  DLLEXPORT EN_nextH(EN_Project *modelObj, long *tStep);
  int  DLLEXPORT EN_closeH(EN_Project *modelObj);
  int  DLLEXPORT EN_savehydfile(EN_Project *modelObj, char *filename);
  int  DLLEXPORT EN_usehydfile(EN_Project *modelObj, char *filename);
  int  DLLEXPORT EN_getstatistic(EN_Project *modelObj, int code, EN_API_FLOAT_TYPE* value);
  int  DLLEXPORT EN_timeToNextEvent(EN_Project *modelObj, EN_TimestepEvent *eventType, long *duration, int *elementIndex);
  
  // Water Quality solver
  int  DLLEXPORT EN_solveQ(EN_Project *modelObj);
  int  DLLEXPORT EN_openQ(EN_Project *modelObj);
  int  DLLEXPORT EN_initQ(EN_Project *modelObj, int saveFlag);
  int  DLLEXPORT EN_runQ(EN_Project *modelObj, long *currentTime);
  int  DLLEXPORT EN_nextQ(EN_Project *modelObj, long *tStep);
  int  DLLEXPORT EN_stepQ(EN_Project *modelObj, long *timeLeft);
  int  DLLEXPORT EN_closeQ(EN_Project *modelObj);
  int  DLLEXPORT EN_setqualtype(EN_Project *modelObj, int qualcode, char *chemname, char *chemunits, char *tracenode);
  int  DLLEXPORT EN_getqualinfo(EN_Project *modelObj, int *qualcode, char *chemname, char *chemunits, int *tracenode);
  
  // Reporting
  int  DLLEXPORT EN_setReportCallback(EN_Project *m, void (*callback)(void *userData, EN_Project*,char*)); /**< set a callback function for logging. by default EN_writeline */
  int  DLLEXPORT EN_setReportCallbackUserData(EN_Project *m, void *userData);
  int  DLLEXPORT EN_writeline(EN_Project *modelObj, char *line);
  int  DLLEXPORT EN_report(EN_Project *modelObj);
  int  DLLEXPORT EN_resetreport(EN_Project *modelObj);
  int  DLLEXPORT EN_setreport(EN_Project *modelObj, char *reportFormat);
  int  DLLEXPORT EN_setstatusreport(EN_Project *modelObj, int code);
  
  // Get/Set basic network info
  int  DLLEXPORT EN_getcount(EN_Project *modelObj, int code, int *count);
  int  DLLEXPORT EN_getoption(EN_Project *modelObj, int code, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT EN_setoption(EN_Project *modelObj, int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT EN_gettimeparam(EN_Project *modelObj, int code, long *value);
  int  DLLEXPORT EN_settimeparam(EN_Project *modelObj, int code, long value);
  int  DLLEXPORT EN_getflowunits(EN_Project *modelObj, int *code);
  int  DLLEXPORT EN_getqualtype(EN_Project *modelObj, int *qualcode, int *tracenode);
  int  DLLEXPORT EN_geterror(int errcode, char *errmsg, int maxLen);
  int  DLLEXPORT EN_getversion(int *version);
  
  // Controls & Rules
  int  DLLEXPORT EN_controlEnabled(EN_Project *modelObj, int controlIndex);
  int  DLLEXPORT EN_setControlEnabled(EN_Project *modelObj, int controlIndex, int enable);
  int  DLLEXPORT EN_setcontrol(EN_Project *modelObj, int cindex, int ctype, int lindex, EN_API_FLOAT_TYPE setting, int nindex, EN_API_FLOAT_TYPE level);
  int  DLLEXPORT EN_getcontrol(EN_Project *modelObj, int controlIndex, int *controlType, int *linkIdx, EN_API_FLOAT_TYPE *setting, int *nodeIdx, EN_API_FLOAT_TYPE *level);
  int  DLLEXPORT EN_addcontrol(EN_Project *modelObj, int ctype, int lindex, EN_API_FLOAT_TYPE setting, int nindex, EN_API_FLOAT_TYPE level, int *cindex);
  int  DLLEXPORT EN_deletecontrol(EN_Project *modelObj, int cindex);
  int  DLLEXPORT EN_ruleEnabled(EN_Project *modelObj, int ruleIndex);
  int  DLLEXPORT EN_setRuleEnabled(EN_Project *modelObj, int ruleIndex, int enable);
  int  DLLEXPORT EN_getRuleName(EN_Project *modelObj, int ruleIndex, char* id);
  int  DLLEXPORT EN_getRuleAffectedLinks(EN_Project *m, int ruleIndex, int* nLinks, int* linkIndexes);
  int  DLLEXPORT EN_freeRuleAffectedLinks(int* linkIndexes);
  
  // Patterns
  int  DLLEXPORT EN_getpatternindex(EN_Project *modelObj, char *id, int *index);
  int  DLLEXPORT EN_getpatternid(EN_Project *modelObj, int index, char *id);
  int  DLLEXPORT EN_getpatternlen(EN_Project *modelObj, int index, int *len);
  int  DLLEXPORT EN_getpatternvalue(EN_Project *modelObj, int index, int period, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT EN_getaveragepatternvalue(EN_Project *modelObj, int index, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT EN_addpattern(EN_Project *modelObj, char *id);
  int  DLLEXPORT EN_setpattern(EN_Project *modelObj, int index, EN_API_FLOAT_TYPE *f, int len);
  int  DLLEXPORT EN_setpatternvalue(EN_Project *modelObj, int index, int period, EN_API_FLOAT_TYPE value);
  
  // Node elements
  int  DLLEXPORT EN_getnodeindex(EN_Project *modelObj, char *id, int *index);
  int  DLLEXPORT EN_getnodeid(EN_Project *modelObj, int index, char *id);
  int  DLLEXPORT EN_getnodetype(EN_Project *modelObj, int index, EN_NodeType *code);
  int  DLLEXPORT EN_getnodevalue(EN_Project *modelObj, int index, int code, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT EN_setnodevalue(EN_Project *modelObj, int index, int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT EN_getnodecomment(EN_Project *modelObj, int nIndex, char *comment);
  int  DLLEXPORT EN_setnodecomment(EN_Project *modelObj, int nIndex, const char *comment);
  int  DLLEXPORT EN_getcoord(EN_Project *modelObj, int index, EN_API_FLOAT_TYPE *x, EN_API_FLOAT_TYPE *y);
  
  // Demands
  int  DLLEXPORT EN_getnumdemands(EN_Project *modelObj, int nodeIndex, int *numDemands);
  int  DLLEXPORT EN_getbasedemand(EN_Project *modelObj, int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE *baseDemand);
  int  DLLEXPORT EN_getdemandpattern(EN_Project *modelObj, int nodeIndex, int demandIdx, int *pattIdx);
  int  DLLEXPORT EN_setbasedemand(EN_Project *modelObj, int nodeIndex, int demandIdx, EN_API_FLOAT_TYPE baseDemand);
  
  // Link elements
  int  DLLEXPORT EN_getlinkindex(EN_Project *modelObj, char *id, int *index);
  int  DLLEXPORT EN_getlinkid(EN_Project *modelObj, int index, char *id);
  int  DLLEXPORT EN_getlinktype(EN_Project *modelObj, int index, EN_LinkType *type);
  int  DLLEXPORT EN_getlinknodes(EN_Project *modelObj, int index, int *node1, int *node2);
  int  DLLEXPORT EN_getlinkvalue(EN_Project *modelObj, int index, int code, EN_API_FLOAT_TYPE *value);
  int  DLLEXPORT EN_setlinkvalue(EN_Project *modelObj, int index, int code, EN_API_FLOAT_TYPE v);
  int  DLLEXPORT EN_getlinkcomment(EN_Project *modelObj, int linkIndex, char *comment);
  int  DLLEXPORT EN_setlinkcomment(EN_Project *modelObj, int linkIndex, const char *comment);
  
  // Curves
  int  DLLEXPORT EN_getcurve(EN_Project *modelObj, int curveIndex, char* id, int *nValues, EN_API_FLOAT_TYPE **xValues, EN_API_FLOAT_TYPE **yValues);
  
  
  // network creation api set
  
  int DLLEXPORT EN_newNetwork(EN_Project **modelObj);
  int DLLEXPORT EN_startEditingNetwork(EN_Project *modelObj);
  int DLLEXPORT EN_addNode(EN_Project *modelObj, EN_NodeType type, char *name);
  int DLLEXPORT EN_addLink(EN_Project *modelObj, EN_LinkType type, char *name, char *upstreamNode, char* downstreamNode);
  int DLLEXPORT EN_stopEditingNetwork(EN_Project *modelObj);
  
#if defined(__cplusplus)
}
#endif

#endif //TOOLKIT_H
