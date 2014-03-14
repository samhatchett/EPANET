/*
************************************************************************
            Global Variables for EPANET Program                            
                                                                    
VERSION:    2.00                                               
DATE:       5/8/00
            6/24/02
            2/14/08    (2.00.12)
AUTHOR:     L. Rossman                                         
            US EPA - NRMRL
                                                                     
************************************************************************
*/
#ifndef VARS_H
#define VARS_H

#include <stdio.h>
#include "hash.h"

 

/* Array pointers not allocated and freed in same routine */

Model en_defaultModel;



/*
** NOTE: Hydraulic analysis of the pipe network at a given point in time
**       is done by repeatedly solving a linearized version of the 
**       equations for conservation of flow & energy:
**
**           A*H = F
**
**       where H = vector of heads (unknowns) at each node,
**             F = vector of right-hand side coeffs.
**             A = square matrix of coeffs.
**       and both A and F are updated at each iteration until there is
**       negligible change in pipe flows.
**
**       Each row (or column) of A corresponds to a junction in the pipe
**       network. Each link (pipe, pump or valve) in the network has a
**       non-zero entry in the row-column of A that corresponds to its
**       end points. This results in A being symmetric and very sparse.
**       The following arrays are used to efficiently manage this sparsity:
*/

// hydraulic solution vars moved to Model struct.


/*
** The following arrays store the positions of the non-zero coeffs.    
** of the lower triangular portion of A whose values are stored in Aij:
*/

// also moved to Model

#endif