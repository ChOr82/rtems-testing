/*
 *  $Id$
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <list>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_common.h"
#include "CoverageFactory.h"
#include "CoverageMap.h"
#include "DesiredSymbols.h"
#include "ExecutableInfo.h"
#include "Explanations.h"
#include "ObjdumpProcessor.h"
#include "ReportsBase.h"
#include "TargetFactory.h"

/*
 *  Variables to control general behavior
 */
char*                                coverageFileExtension = NULL;
std::list<std::string>               coverageFileNames;
int                                  coverageExtensionLength = 0;
Coverage::CoverageFormats_t          coverageFormat;
Coverage::CoverageReaderBase*        coverageReader = NULL;
const char*                          dynamicLibrary = NULL;
char*                                executable = NULL;
char*                                executableExtension = NULL;
int                                  executableExtensionLength = 0;
std::list<Coverage::ExecutableInfo*> executablesToAnalyze;
const char*                          explanations = NULL;
char*                                progname;
const char*                          symbolsFile = NULL;
const char*                          target = NULL;
const char*                          format = NULL;

/*
 *  Print program usage message
 */
void usage()
{
  fprintf(
    stderr,
    "Usage: %s [-v] -T TARGET -f FORMAT [-E EXPLANATIONS] -1 EXECUTABLE coverage1 ... coverageN\n"
    "--OR--\n"
    "Usage: %s [-v] -T TARGET -f FORMAT [-E EXPLANATIONS] -e EXE_EXTENSION -c COVERAGEFILE_EXTENSION EXECUTABLE1 ... EXECUTABLE2\n"
    "\n"
    "  -v                        - verbose at initialization\n"
    "  -T TARGET                 - target name\n"
    "  -f FORMAT                 - coverage file format "
           "(RTEMS, QEMU, TSIM or Skyeye)\n"
    "  -E EXPLANATIONS           - name of file with explanations\n"
    "  -s SYMBOLS_FILE           - name of file with symbols of interest\n"
    "  -1 EXECUTABLE             - name of executable to get symbols from\n"
    "  -e EXE_EXTENSION          - extension of the executables to analyze\n"
    "  -c COVERAGEFILE_EXTENSION - extension of the coverage files to analyze\n"
    "  -C ConfigurationFileName  - name of configuration file\n"
    "  -O Output_Directory       - name of output directory (default=."
    "\n",
    progname,
    progname
  );
}

#define PrintableString(_s) \
       ((!(_s)) ? "NOT SET" : (_s))

/*
 *  Configuration File Support
 */
#include "ConfigFile.h"
Configuration::FileReader *CoverageConfiguration;

Configuration::Options_t Options[] = {
  { "explanations", NULL },
  { "format",       NULL },
  { "symbolsFile ", NULL },
  { "target",       NULL },
  { "verbose",      NULL },
  { NULL,           NULL }
};

bool isTrue(const char *value)
{
  if ( !value )                  return false;
  if ( !strcmp(value, "true") )  return true;
  if ( !strcmp(value, "TRUE") )  return true;
  if ( !strcmp(value, "yes") )   return true;
  if ( !strcmp(value, "YES") )   return true;
  return false;
}

#define GET_BOOL(_opt, _val) \
  if (isTrue(CoverageConfiguration->getOption(_opt))) \
    _val = true;

#define GET_STRING(_opt, _val) \
  do { \
    const char *_t; \
    _t = CoverageConfiguration->getOption(_opt); \
    if ( _t ) _val = _t; \
  } while(0)
  

void check_configuration(void)
{
  GET_BOOL( "verbose", Verbose );

  GET_STRING( "format",           format );
  GET_STRING( "target",           target );
  GET_STRING( "explanations",     explanations );
  GET_STRING( "symbolsFile",      symbolsFile );
  GET_STRING( "outputDirectory",  outputDirectory );

  // Now calculate some values
  if ( coverageFileExtension )
    coverageExtensionLength = strlen( coverageFileExtension );

  if ( executableExtension )
    executableExtensionLength = strlen( executableExtension );

  if ( format )
    coverageFormat = Coverage::CoverageFormatToEnum( format );
}

int main(
  int    argc,
  char** argv
)
{
  std::list<std::string>::iterator               citr;
  std::string                                    coverageFileName;
  std::list<Coverage::ExecutableInfo*>::iterator eitr;
  Coverage::ExecutableInfo*                      executableInfo = NULL;
  int                                            i;
  int                                            opt;
  const char*                                    singleExecutable = NULL;

  CoverageConfiguration = new Configuration::FileReader(Options);
  
  //
  // Process command line options.
  //
  progname = argv[0];

  while ((opt = getopt(argc, argv, "C:1:L:e:c:E:f:s:T:O:v")) != -1) {
    switch (opt) {
      case 'C': CoverageConfiguration->processFile( optarg ); break;
      case '1': singleExecutable      = optarg; break;
      case 'L': dynamicLibrary        = optarg; break;
      case 'e': executableExtension   = optarg; break;
      case 'c': coverageFileExtension = optarg; break;
      case 'E': explanations          = optarg; break;
      case 'f': format                = optarg; break;
      case 's': symbolsFile           = optarg; break;
      case 'T': target                = optarg; break;
      case 'O': outputDirectory       = optarg; break;
      case 'v': Verbose               = true;   break;
      default: /* '?' */
        usage();
        exit( -1 );
    }
  }

  // Do not trust any arguments until after this point.
  check_configuration();

  // XXX We need to verify that all of the needed arguments are non-NULL.

  // If a single executable was specified, process the remaining
  // arguments as coverage file names.
  if (singleExecutable) {

    // Ensure that the executable is readable.
    if (!FileIsReadable( singleExecutable )) {
      fprintf(
        stderr,
        "WARNING: Unable to read executable %s\n",
        singleExecutable
      );
    }

    else {

      for (i=optind; i < argc; i++) {

        // Ensure that the coverage file is readable.
        if (!FileIsReadable( argv[i] )) {
          fprintf(
            stderr,
            "WARNING: Unable to read coverage file %s\n",
            argv[i]
          );
        }

        else
          coverageFileNames.push_back( argv[i] );
      }

      // If there was at least one coverage file, create the
      // executable information.
      if (!coverageFileNames.empty()) {
        if (dynamicLibrary)
          executableInfo = new Coverage::ExecutableInfo(
            singleExecutable, dynamicLibrary
          );
        else
          executableInfo = new Coverage::ExecutableInfo( singleExecutable );

        executablesToAnalyze.push_back( executableInfo );
      }
    }
  }

  // If not invoked with a single executable, process the remaining
  // arguments as executables and derive the coverage file names.
  else {
    for (i = optind; i < argc; i++) {

      // Ensure that the executable is readable.
      if (!FileIsReadable( argv[i] )) {
        fprintf(
          stderr,
          "WARNING: Unable to read executable %s\n",
          argv[i]
        );
      }

      else {
        coverageFileName = argv[i];
        coverageFileName.replace(
          coverageFileName.length() - executableExtensionLength,
          executableExtensionLength,
          coverageFileExtension
        );

        if (!FileIsReadable( coverageFileName.c_str() )) {
          fprintf(
            stderr,
            "WARNING: Unable to read coverage file %s\n",
            coverageFileName.c_str()
          );
        }

        else {
          executableInfo = new Coverage::ExecutableInfo( argv[i] );
          executablesToAnalyze.push_back( executableInfo );
          coverageFileNames.push_back( coverageFileName );
        }
      }
    }
  }

  // Ensure that there is at least one executable to process.
  if (executablesToAnalyze.empty()) {
    fprintf(
      stderr, "ERROR: No information to analyze\n"
    );
    exit( -1 );
  }

  if (Verbose) {
    if (singleExecutable)
      fprintf(
        stderr,
        "Processing a single executable and multiple coverage files\n"
      );
    else
      fprintf(
        stderr,
        "Processing multiple executable/coverage file pairs\n"
      );
    fprintf( stderr, "Coverage Format : %s\n", format );
    fprintf( stderr, "Target          : %s\n", PrintableString(target) );
    fprintf( stderr, "\n" );
#if 1
    // Process each executable/coverage file pair.
    eitr = executablesToAnalyze.begin();
    for (citr = coverageFileNames.begin();
	 citr != coverageFileNames.end();
	 citr++) {

	fprintf(
	  stderr,
	  "Coverage file %s for executable %s\n",
	  (*citr).c_str(),
	  ((*eitr)->getFileName()).c_str()
	);

	if (!singleExecutable)
	  eitr++;
    }
#endif
  }

  //
  // Validate inputs.
  //

  // Target name must be set.
  if (!target) {
    fprintf( stderr, "ERROR: target not specified\n" );
    usage();
    exit(-1);
  }

  // Validate format.
  if (!format) {
    fprintf( stderr, "ERROR: coverage format report not specified\n" );
    usage();
    exit(-1);
  }

  // Validate that we have a symbols of interest file.
  if (!symbolsFile) {
    fprintf( stderr, "ERROR: symbols of interest file not specified\n" );
    usage();
    exit(-1);
  }

  //
  // Create data to support analysis.
  //

  // Create data based on target.
  TargetInfo = Target::TargetFactory( target );

  // Create the set of desired symbols.
  SymbolsToAnalyze = new Coverage::DesiredSymbols();
  SymbolsToAnalyze->load( symbolsFile );
  if (Verbose)
    fprintf(
      stderr, "Analyzing %d symbols\n", SymbolsToAnalyze->set.size()
    );

  // Create explanations.
  AllExplanations = new Coverage::Explanations();
  if ( explanations )
    AllExplanations->load( explanations );

  // Create coverage map reader.
  coverageReader = Coverage::CreateCoverageReader(coverageFormat);
  if (!coverageReader) {
    fprintf( stderr, "ERROR: Unable to create coverage file reader\n" );
    exit(-1);
  }

  // Create the objdump processor.
  objdumpProcessor = new Coverage::ObjdumpProcessor();

  // Prepare each executable for analysis.
  for (eitr = executablesToAnalyze.begin();
       eitr != executablesToAnalyze.end();
       eitr++) {

    if (Verbose)
      fprintf(
        stderr,
        "Extracting information from %s\n",
        ((*eitr)->getFileName()).c_str()
      );

    // If a dynamic library was specified, determine the load address.
    if (dynamicLibrary)
      (*eitr)->setLoadAddress(
        objdumpProcessor->determineLoadAddress( *eitr )
      );

    // Load the objdump for the symbols in this executable.
    objdumpProcessor->load( *eitr );
  }

  //
  // Analyze the coverage data.
  //

  // Process each executable/coverage file pair.
  eitr = executablesToAnalyze.begin();
  for (citr = coverageFileNames.begin();
       citr != coverageFileNames.end();
       citr++) {

    if (Verbose)
      fprintf(
        stderr,
        "Processing coverage file %s for executable %s\n",
        (*citr).c_str(),
        ((*eitr)->getFileName()).c_str()
      );

    // Process its coverage file.
    coverageReader->processFile( (*citr).c_str(), *eitr );

    // Merge each symbols coverage map into a unified coverage map.
    (*eitr)->mergeCoverage();

    if (!singleExecutable)
      eitr++;
  }

  // Do necessary preprocessing of uncovered ranges and branches
  if (Verbose)
    fprintf( stderr, "Preprocess uncovered ranges and branches\n" );
  SymbolsToAnalyze->preprocess();

  // Determine the uncovered ranges and branches.
  if (Verbose)
    fprintf( stderr, "Computing uncovered ranges and branches\n" );
  SymbolsToAnalyze->computeUncovered();

  // Calculate remainder of statistics.
  if (Verbose)
    fprintf( stderr, "Calculate statistics\n" );
  SymbolsToAnalyze->caculateStatistics();

  // Look up the source lines for any uncovered ranges and branches.
  if (Verbose)
    fprintf(
      stderr, "Looking up source lines for uncovered ranges and branches\n"
    );
  SymbolsToAnalyze->findSourceForUncovered();

  //
  // Report the coverage data.
  //
  if (Verbose)
    fprintf(
      stderr, "Generate Reports\n"
    );
  Coverage::GenerateReports();

  // Write explanations that were not found.
  if ( explanations ) {
    std::string notFound;

    notFound = outputDirectory;
    notFound += "/";
    notFound += "ExplanationsNotFound.txt";

    if (Verbose)
      fprintf( stderr, "Writing Not Found Report (%s)\n", notFound.c_str() );
    AllExplanations->writeNotFound( notFound.c_str() );
  }

  // Calculate coverage statistics and output results.
  {
    uint32_t                                        a;
    uint32_t                                        endAddress;
    Coverage::DesiredSymbols::symbolSet_t::iterator itr;
    uint32_t                                        notExecuted = 0;
    double                                          percentage;
    Coverage::CoverageMapBase*                      theCoverageMap;
    uint32_t                                        totalBytes = 0;

    // Look at each symbol.
    for (itr = SymbolsToAnalyze->set.begin();
         itr != SymbolsToAnalyze->set.end();
         itr++) {

      // If the symbol's unified coverage map exists, scan through it
      // and count bytes.
      theCoverageMap = itr->second.unifiedCoverageMap;
      if (theCoverageMap) {

        endAddress = itr->second.stats.sizeInBytes - 1;

        for (a = 0; a <= endAddress; a++) {
          totalBytes++;
          if (!theCoverageMap->wasExecuted( a ))
            notExecuted++;
        }
      }
    }

    percentage = (double) notExecuted;
    percentage /= (double) totalBytes;
    percentage *= 100.0;

    printf( "Bytes Analyzed           : %d\n", totalBytes );
    printf( "Bytes Not Executed       : %d\n", notExecuted );
    printf( "Percentage Executed      : %5.4g\n", 100.0 - percentage  );
    printf( "Percentage Not Executed  : %5.4g\n", percentage  );
    printf(
      "Uncovered ranges found   : %d\n",
      SymbolsToAnalyze->getNumberUncoveredRanges()
    );
    if ((SymbolsToAnalyze->getNumberBranchesFound() == 0) || 
        (BranchInfoAvailable == false) ) {
      printf( "No branch information available\n" );
    } else {
      printf(
        "Total branches found     : %d\n",
        SymbolsToAnalyze->getNumberBranchesFound()
      );
      printf(
        "Uncovered branches found : %d\n",
        SymbolsToAnalyze->getNumberBranchesAlwaysTaken() +
         SymbolsToAnalyze->getNumberBranchesNeverTaken()
      );
      printf(
        "   %d branches always taken\n",
        SymbolsToAnalyze->getNumberBranchesAlwaysTaken()
      );
      printf(
        "   %d branches never taken\n",
        SymbolsToAnalyze->getNumberBranchesNeverTaken()
      );
    }
  }

  return 0;
}
