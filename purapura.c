#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
	int a[10];
	volatile int x = 0;

	for(int u=0;u<10;u++)
	{
		a[u] = fork();
		if(!a[u])
		{
			for(int y = 0; y < 10; y++)
			{
				printf(1, "pid: %d, %d\n", getpid(), y);
				for(int i = 0; i < 10000000; i++)
					x++;
			}
			//for(int i=0;i<1e9;i++);
			exit();
		}
	}	
	for(int i=0;i<10;i++)
	{
		wait();
	}
	exit();
}
