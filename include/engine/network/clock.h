#ifndef ENGINE_NETWORK_CLOCK_H
#define ENGINE_NETWORK_CLOCK_H
#include <stdint.h>
typedef struct {unsigned year,month,day,hour,minute,second;uint32_t days,seconds;} clock7_time;
unsigned clock7_calendar(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,clock7_time *);
unsigned clock7_read(clock7_time *);
#endif
