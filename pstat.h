#ifndef _PSTAT_H_
#define _PSTAT_H_
#include "param.h"
struct pstat
{
	int pid;
	int runtime;
	int num_run;
	int current_queue;
	int ticks[5];
};
#endif
