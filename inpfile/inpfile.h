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


#define EN_OK 0
#define EN_ERR_CANT_OPEN_FILE 302
#define EN_ERR_FILE_SEEK 310


int DLLEXPORT EN_loadInpFile(EN_Project *modelObj, const char *filename);
int DLLEXPORT EN_saveInpFile(EN_Project *modelObj, const char *filename);



#endif
