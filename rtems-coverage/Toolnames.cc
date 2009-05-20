/*
 *  $Id$
 */

/*! @file Toolnames.cc
 *  @brief Toolnames Implementation
 *
 *  This file contains the implementation of the functions 
 *  which provide a base level of functionality of a CoverageMap.
 */

#include "Toolnames.h"
#include <string.h>
#include <stdlib.h>

namespace Coverage {

  Toolnames::Toolnames(
    const char *target_arg
  )
  {
    target    = target_arg; 
    addr2line = target + "-addr2line";
    objdump   = target + "-objdump";
    if ( !strncmp( target_arg, "sparc", 5 ) ) {
      nopSize = 4;
    } else if ( !strncmp( target_arg, "arm", 3 ) ) {
      nopSize = 4;
      fprintf( stderr, "SANTOSH - HOW LARGE IS NOP ON ARM? -- fix me ;)\n" );
      exit(1);
    } else {
      fprintf( stderr, "HOW LARGE IS NOP ON THIS ARCHITECTURE? -- fix me\n" );
      exit(1);
    }
  }

  Toolnames::~Toolnames()
  {
  }

  const char *Toolnames::getAddr2line()
  {
    return addr2line.c_str();
  }

  const char *Toolnames::getObjdump()
  {
    return objdump.c_str();
  }

  const int Toolnames::getNopSize( void )
  {
     return nopSize;
  }
  
}
