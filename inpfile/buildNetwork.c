#include <stdio.h>

#define EN_API_FLOAT_TYPE double

#include "inpfile.h"


int   main(int argc, char *argv[])
{
  
  OW_Project *m1;
  long t1;
  /*
  OW_open("/Users/sam/Desktop/sampletown.inp", &m1, "report.rpt", "bin.out");
  OW_openH(m1);
  OW_initH(m1, EN_NOSAVE);
  OW_runH(m1, &t1);
  
  */
  
  
  
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
  
  
  
}