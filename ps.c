#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

int main(void)
{
	printf(1,"Sr.No\tPID\tRT\tCQ\tQ1T\tQ2T\tQ3T\tQ4T\tQ5T\n");
        for(int g=0;g<64;g++)
        {
                struct pstat st;
		printf(1,"%d\t",g);
                int a = getpinfo(&st,g);
		//printf(1,"aa g");
		a++;
        }
}
