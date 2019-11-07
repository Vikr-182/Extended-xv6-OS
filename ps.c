#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

int main(void)
{
	printf(1,"Sr.No\tPID\tRT\tCQ\tQ1T\tQ2T\tQ3T\tQ4T\tQ5T\n");
    struct pstat *st = 0;
    int a = getpinfo(st);
    printf(1,"%d\n",a);
    a++;a--;
    for(int i=0;i<64;i++)
    {
        printf(1,"%d %d\n",i,st->pid);
    }
}
