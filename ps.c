#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

int main(void)
{
	printf(1,"Sr.No\tPID\tInuse\tPriority\tRT\tNumRun\tQnum\tQ1T\tQ2T\tQ3T\tQ4T\tQ5T\n");
    struct pstat *st = 0;
    int a = getpinfo(st);
    printf(1,"%d\n",a);
    a++;a--;
    for(int i=0;i<64;i++)
    {
        printf(1,"%d %d %d %d %d %d %d\t",i,st->pid[i],st->inuse[i],st->priority[i],st->runtime[i],st->num_run[i],st->queue_num[i]);
        for(int b=0;b<5;b++)
        {
            printf(1,"%d\t",st->ticks[i][b]);
        }
        printf(1,"\n");
    }
}
