#include <stdint.h>
#define _TOD_Ticks_per_second 1
#define _TOD_Microseconds_per_tick 1000000
typedef uint32_t rtems_interval;
#define rtems_clock_get(a,b) time(b)
