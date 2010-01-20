/*
 *  $Id$
 */

/*! @file CoverageReaderQEMU.cc
 *  @brief CoverageReaderQEMU Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading the QEMU coverage data files.
 */

#include "CoverageReaderQEMU.h"
#include "CoverageMapBase.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/* XXX really not always right */
typedef uint32_t target_ulong;

#include "qemu-traces.h"

namespace Coverage {

  CoverageReaderQEMU::CoverageReaderQEMU()
  {
  }

  CoverageReaderQEMU::~CoverageReaderQEMU()
  {
  }

  bool CoverageReaderQEMU::ProcessFile(
    const char      *file,
    CoverageMapBase *coverage
  )
  {
    uint32_t            beginning;
    struct trace_entry  entry;
    struct trace_header header;
    uintptr_t           i;
    struct stat         statbuf;
    int                 status;
    FILE               *traceFile;

    /*
     *  Verify it has a non-zero size
     */
    status = stat( file, &statbuf );
    if ( status == -1 ) {
      fprintf( stderr, "Unable to stat %s\n", file );
      return false;
    }

    if ( statbuf.st_size == 0 ) {
      fprintf( stderr, "%s is 0 bytes long\n", file );
      return false;
    }

    /*
     *  read the file and update the coverage map passed in
     */
    traceFile = fopen( file, "r" );
    if ( !traceFile ) {
      fprintf( stderr, "Unable to open %s\n", file );
      return false;
    }

    status = fread( &header, sizeof(trace_header), 1, traceFile );
    if ( status != 1 ) {
      fprintf( stderr, "Unable to read header from %s\n", file );
      return false;
    }

    #if 0
      fprintf(
        stderr,
        "magic = %s\n"
        "version = %d\n"
        "kind = %d\n"
        "sizeof_target_pc = %d\n"
        "big_endian = %d\n"
        "machine = %02x:%02x\n",
        header.magic,
        header.version,
        header.kind,
        header.sizeof_target_pc,
        header.big_endian,
        header.machine[0], header.machine[1]
       );
    #endif

    while (1) {
      status = fread( &entry, sizeof(struct trace_entry), 1, traceFile );
      if ( status != 1 )
        break;
      #if 0
        fprintf( stderr, "0x%08x %d 0x%2x\n", entry.pc, entry.size, entry.op );
      #endif

      /* Mark block as fully executed. */
      if ( entry.op & TRACE_OP_BLOCK ) {
        for ( i=0 ; i<entry.size ; i++ ) {
          coverage->setWasExecuted( entry.pc + i );
        }
      }

      /* Determine if additional branch information is available. */
      if (entry.op & 0x0f) {

        if (coverage->getBeginningOfInstruction(
              entry.pc + entry.size -1, &beginning
            )) {

          if (entry.op & TRACE_OP_TAKEN) {
            coverage->setIsBranch( beginning );
            coverage->setWasTaken( beginning );
          }

          else if (entry.op & TRACE_OP_NOT_TAKEN) {
            coverage->setIsBranch( beginning );
            coverage->setWasNotTaken( beginning );
          }
        }
      }
    }

    fclose( traceFile );
    return true;
  }
}