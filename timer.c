#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc,char *argv[])
{
    int pid = fork();
    if(pid == 0)
    {
        exec(argv[1],argv+1);
        printf(1,"It failed %s\n",argv[1]);
    }
    else
    {
        int a,b ;
        int status = waitx(&a,&b);
        printf(1,"Wait time = %d\n Run Time = %d\nStatus = %d\n",a,b,status);
    }
    exit();
}
