#include <stdio.h>

#define EN_API_FLOAT_TYPE double

#include "inpfile.h"



int   main(int argc, char *argv[])
{
  
  int err;

  EN_Project *model;
  err = EN_newNetwork(&model);
  
  
  err = EN_startEditingNetwork(model);
  
  err = EN_addNode(model, EN_JUNCTION, "demand_junction");
  err = EN_setnodevalue(model, 1, EN_ELEVATION, 90.0);
  err = EN_setnodevalue(model, 1, EN_BASEDEMAND, 10);
  
  err = EN_addNode(model, EN_RESERVOIR, "reservoir");
  err = EN_setnodevalue(model, 2, EN_ELEVATION, 100.0);
  
  err = EN_addLink(model, EN_PIPE, "connecting_pipe", "demand_junction", "reservoir");
  err = EN_setlinkvalue(model, 1, EN_SETTING, 0);
  err = EN_setlinkvalue(model, 1, EN_DIAMETER, 10);
  err = EN_setlinkvalue(model, 1, EN_LENGTH, 100);
  
  
  err = EN_stopEditingNetwork(model);
  
  err = EN_openH(model);
  err = EN_initH(model, EN_NOSAVE);
  
  long t;
  err = EN_runH(model, &t);
  
  double flow;
  
  err = EN_getlinkvalue(model, 1, EN_FLOW, &flow);
  
  fprintf(stdout, "flow: %f\n\n", flow);
  
  
  
  err = EN_setnodevalue(model, 1, EN_BASEDEMAND, 20);
  
  err = EN_runH(model, &t);
  err = EN_getlinkvalue(model, 1, EN_FLOW, &flow);
  
  fprintf(stdout, "flow2: %f", flow);
  
  
  EN_close(model);
  
  
}
