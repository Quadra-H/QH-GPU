#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "myprotocol.h"
int main()
{
int dev;
printf("open start\n");
dev = open("/dev/drop",O_RDWR|O_NDELAY);
printf("open end\n");
if( dev < 0 )
{
printf("open error!\n");
return -1;
}
struct myprotocol msg;
msg.cmd = ADD_IP;
strcpy(msg.addr,"192.168.0.4");
write(dev,(char*)&msg,sizeof(msg));
close(dev);
}
