#ifndef _PSTAT_H_
#define _PSTAT_H_
#include "param.h"

struct pstat
{
	int pid[64];
    int inuse[64];
    int priority[64];
	int runtime[64];
	int num_run[64];
	int queue_num[64];
	int ticks[64][5];
};
#endif
