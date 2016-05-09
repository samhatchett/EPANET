#ifndef epanet_inpfile_h
#define epanet_inpfile_h


#include "epanet2.h"


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


#define OW_OK 0
#define OW_ERR_CANT_OPEN_FILE 302
#define OW_ERR_FILE_SEEK 310


int DLLEXPORT OW_loadInpFile(OW_Project *modelObj, const char *filename);
int DLLEXPORT OW_saveInpFile(OW_Project *modelObj, const char *filename);



#endif
