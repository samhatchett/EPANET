#ifndef epanet_errors_h
#define epanet_errors_h

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

#endif
