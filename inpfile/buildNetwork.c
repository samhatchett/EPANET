#include <stdio.h>

#define EN_API_FLOAT_TYPE double

#include "inpfile.h"



int   main(int argc, char *argv[])
{
  char valveName[32];
  OW_Project *m;
  long t1;
  
  OW_open("/Users/sam/Desktop/gcww_v5.inp", &m, "report.rpt", "bin.out");
  
  
  int nLinks;
  OW_getcount(m, EN_LINKCOUNT, &nLinks);
  
  for (int i = 1; i <= nLinks; ++i) {
    
    EN_LinkType type;
    OW_getlinktype(m, i, &type);
    
    if (type == EN_TCV) {
      
      OW_getlinkid(m, i, valveName);
      
      double val;
      OW_getlinkvalue(m, i, EN_INITSETTING, &val);
      
      
      
      if (val >= 10000) {
        // tcv is assumed closed.
        fprintf(stdout, "%s      CLOSED\n", valveName);
      }
      
    }
    
  }
  
  
  
  /*
  
  OW_Project *model;
  OW_newNetwork(&model);
  
  
  OW_startEditingNetwork(model);
  
  OW_addNode(model, EN_JUNCTION, "demand_junction");
  OW_setnodevalue(model, 1, EN_ELEVATION, 90.0);
  OW_setnodevalue(model, 1, EN_BASEDEMAND, 10);
  
  OW_addNode(model, EN_RESERVOIR, "reservoir");
  OW_setnodevalue(model, 2, EN_HEAD, 100.0);
  
  
  
  OW_addLink(model, EN_PIPE, "connecting_pipe", "reservoir", "demand_junction");
  OW_setlinkvalue(model, 1, EN_DIAMETER, 10);
  OW_setlinkvalue(model, 1, EN_LENGTH, 100);
  
  
  OW_stopEditingNetwork(model);
  
  OW_openH(model);
  OW_initH(model, EN_NOSAVE);
  
  long t;
  OW_runH(model, &t);
  
  double flow;
  
  OW_getlinkvalue(model, 1, EN_FLOW, &flow);
  
  fprintf(stdout, "%f", flow);
  
  */
  
}