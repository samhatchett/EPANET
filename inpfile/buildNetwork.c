#include <stdio.h>

#define EN_API_FLOAT_TYPE double

#include "inpfile.h"



int   main(int argc, char *argv[])
{
  
  int err;

  OW_Project *model;
  err = OW_newNetwork(&model);
  
  
  err = OW_startEditingNetwork(model);
  
  err = OW_addNode(model, EN_JUNCTION, "demand_junction");
  err = OW_setnodevalue(model, 1, EN_ELEVATION, 90.0);
  err = OW_setnodevalue(model, 1, EN_BASEDEMAND, 10);
  
  err = OW_addNode(model, EN_RESERVOIR, "reservoir");
  err = OW_setnodevalue(model, 2, EN_ELEVATION, 100.0);
  
  err = OW_addLink(model, EN_PIPE, "connecting_pipe", "demand_junction", "reservoir");
  err = OW_setlinkvalue(model, 1, EN_SETTING, 0);
  err = OW_setlinkvalue(model, 1, EN_DIAMETER, 10);
  err = OW_setlinkvalue(model, 1, EN_LENGTH, 100);
  
  
  err = OW_stopEditingNetwork(model);
  
  err = OW_openH(model);
  err = OW_initH(model, EN_NOSAVE);
  
  long t;
  err = OW_runH(model, &t);
  
  double flow;
  
  err = OW_getlinkvalue(model, 1, EN_FLOW, &flow);
  
  fprintf(stdout, "flow: %f\n\n", flow);
  
  
  
  err = OW_setnodevalue(model, 1, EN_BASEDEMAND, 20);
  
  err = OW_runH(model, &t);
  err = OW_getlinkvalue(model, 1, EN_FLOW, &flow);
  
  fprintf(stdout, "flow2: %f", flow);
  
  
  OW_close(model);
  
  
}