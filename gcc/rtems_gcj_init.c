/*
 *   Main for GCJ Testing
 */

#include <bsp.h>
#include <stdlib.h>
#include <pthread.h>
#include <uart.h> //gdb
#if defined(__mips__)
/* GCC sometimes expects this on the mips */
void _flush_cache()
{
}
#endif
static int   argc     = 1;
static char  arg0[20] = "rtems";
static char *argv[20] = { arg0 };
extern int main( int, char ** );
extern int BSPConsolePort; //gdb
int remote_debug=1;//gdb

void *POSIX_Init(void*);

/* configuration information */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_MICROSECONDS_PER_TICK 1000
#define CONFIGURE_TICKS_PER_TIMESLICE   50

#define CONFIGURE_MAXIMUM_POSIX_THREADS 50
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES 100
#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES  100
#define CONFIGURE_MAXIMUM_POSIX_KEYS 300
#define CONFIGURE_MAXIMUM_POSIX_TIMERS 100
#define CONFIGURE_MAXIMUM_POSIX_QUEUED_SIGNALS 100
#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES 100
#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES 100
#define CONFIGURE_MAXIMUM_TASK_VARIABLES (2 * 40)
#define CONFIGURE_POSIX_INIT_THREAD_STACK_SIZE (4*1024 * 1024)
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE (1*1024 * 1024)

#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 20

#define CONFIGURE_ZERO_WORKSPACE_AUTOMATICALLY TRUE
//#define CONFIGURE_UNIFIED_WORK_AREAS --should care !!!
#define CONFIGURE_STACK_CHECKER_ENABLED TRUE

#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_INIT
#include <rtems/confdefs.h>

void *POSIX_Init(
  void *argument
)
{
  struct sched_param param;
  param.sched_priority = 5;
  pthread_setschedparam(pthread_self(),SCHED_OTHER, &param);
  //printf("Switch to GCJ Java HelloWorld!----in rtems_init.c\n");
  /*gdb 1-----------------
  if(BSPConsolePort!=BSP_UART_COM2)
    i386_stub_glue_init(BSP_UART_COM2);
  else
    i386_stub_glue_init(BSP_UART_COM1);
  set_debug_traps();
  i386_stub_glue_init_breakin();
  breakpoint();
  //gdb -----------------*/
  /*gdb 2---------------
  init_remote_gdb();
  breakpoint();
  //gdb 2--------------*/
  /*RTEMS memory view----
  printf( "RTEMS Work Space : 0x%X -- 0x%X (%dM)\n",
    Configuration.work_space_start,
    (char*)Configuration.work_space_start + Configuration.work_space_size,
    Configuration.work_space_size/1024/1024
  );
  printf( "RTEMS Malloc Heap : 0x%X -- 0x%X (%dM)\n",
    RTEMS_Malloc_Heap->area_begin,
    RTEMS_Malloc_Heap->area_end,
    (RTEMS_Malloc_Heap->area_end - RTEMS_Malloc_Heap->area_begin)/1024/1024
  );
  //memory view ----------*/
  main(argc, argv);
  exit(0);
}
