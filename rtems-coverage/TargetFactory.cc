//
//  $Id$
//

//! @file TargetFactory.cc
//! @brief TargetFactory Implementation
//!
//! This file contains the implementation of a factory for a
//! instances of a family of classes derived from TargetBase.
//!

#include "TargetFactory.h"

#include "Target_arm.h"
#include "Target_i386.h"
#include "Target_m68k.h"
#include "Target_powerpc.h"
#include "Target_lm32.h"
#include "Target_sparc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace Target {

  //!
  //! @brief TargetBase Factory Table Entry
  //!
  //! This structure contains the @a name associated with the target
  //! in the configuration structures.  The table of names is scanned
  //! to find a constructor helper.
  //!
  typedef struct {
     //! This is the string found in configuration to match.
     const char    *theTarget;
     //! This is the static wrapper for the constructor.
     // XXX Change the calling parameters to "theCtor".
     TargetBase *(*theCtor)( 
       std::string
     );
  } FactoryEntry_t;

  //!
  //! @brief TargetBase Factory Table
  //!
  //! This is the table of possible types we can construct
  //! dynamically based upon user specified configuration information.
  //! All must be derived from TargetBase.
  //!
  static FactoryEntry_t FactoryTable[] = {
    { "arm",     Target_arm_Constructor },
    { "i386",    Target_i386_Constructor },
    { "m68k",    Target_m68k_Constructor },
    { "powerpc", Target_powerpc_Constructor },
    { "lm32",    Target_lm32_Constructor },
    { "sparc",   Target_sparc_Constructor },
    { "TBD",     NULL },
  };

  TargetBase* TargetFactory(
    std::string          targetName
  )
  {
    size_t i;

    // Iterate over the table trying to find an entry with a matching name
    for ( i=0 ; i < sizeof(FactoryTable) / sizeof(FactoryEntry_t); i++ ) {
      if ( !strcmp(FactoryTable[i].theTarget, targetName.c_str() ) )
        return FactoryTable[i].theCtor( targetName );
    }

    fprintf(
      stderr,
      "ERROR!!! %s is not a known architecture!!!\n",
      targetName.c_str()
    );
    fprintf( stderr, "-- fix me\n" );
    exit( 1 );

    return NULL;
  }
}