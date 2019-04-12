 /*
 新的Linux内核使用udev代替了hotplug作为热拔插管理，虽然有udevd管理热拔插，
 但有时候我们还是需要在应用程序中检测热拔插事件以便快速地处理，
 比如在读写SD卡的时候拔下SD卡，那么需要立即检测出该情况，然后结束读写线程，防止VFS崩溃。
 Netlink是面向数据包的服务，为内核与用户层搭建了一个高速通道，是udev实现的基础。
 该工作方式是异步的，用户空间程序不必使用轮询等技术来检测热拔插事件。
 */   
   
   #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <errno.h>
    #include <sys/types.h>
    #include <asm/types.h>
    //该头文件需要放在netlink.h前面防止编译出现__kernel_sa_family未定义
    #include <sys/socket.h>  
    #include <linux/netlink.h>

    void MonitorNetlinkUevent()
    {
        int sockfd;
        struct sockaddr_nl sa;
        int len;
        char buf[4096];
        struct iovec iov;
        struct msghdr msg;
        int i;

        memset(&sa,0,sizeof(sa));
        sa.nl_family=AF_NETLINK;
        sa.nl_groups=NETLINK_KOBJECT_UEVENT;
        sa.nl_pid = 0;//getpid(); both is ok
        memset(&msg,0,sizeof(msg));
        iov.iov_base=(void *)buf;
        iov.iov_len=sizeof(buf);
        msg.msg_name=(void *)&sa;
        msg.msg_namelen=sizeof(sa);
        msg.msg_iov=&iov;
        msg.msg_iovlen=1;

        sockfd=socket(AF_NETLINK,SOCK_RAW,NETLINK_KOBJECT_UEVENT);
        if(sockfd==-1)
            printf("socket creating failed:%s\n",strerror(errno));
        if(bind(sockfd,(struct sockaddr *)&sa,sizeof(sa))==-1)
            printf("bind error:%s\n",strerror(errno));

        len=recvmsg(sockfd,&msg,0);
        if(len<0)
            printf("receive error\n");
        else if(len<32||len>sizeof(buf))
            printf("invalid message");
        for(i=0;i<len;i++)
            if(*(buf+i)=='\0')
                buf[i]='\n';
        printf("received %d bytes\n%s\n",len,buf);
    }

    int main(int argc,char **argv)
    {
        MonitorNetlinkUevent();
        return 0;
    }
    
   /*
 运行程序，然后我插入一个U盘，得到下面的结果：
$ ./netlink 
received 274 bytes
add@/devices/pci0000:00/0000:00:11.0/0000:02:03.0/usb1/1-1
ACTION=add
DEVPATH=/devices/pci0000:00/0000:00:11.0/0000:02:03.0/usb1/1-1
SUBSYSTEM=usb
MAJOR=189
MINOR=1
DEVNAME=bus/usb/001/002
DEVTYPE=usb_device
PRODUCT=dd8/2005/110
TYPE=0/0/0
BUSNUM=001
DEVNUM=002
SEQNUM=3735

运行程序，拔掉U盘
$ ./netlink 
received 304 bytes
remove@/devices/pci0000:00/0000:00:11.0/0000:02:03.0/usb1/1-1/1-1:1.0/host33/target33:0:0/33:0:0:0/bsg/33:0:0:0
ACTION=remove
DEVPATH=/devices/pci0000:00/0000:00:11.0/0000:02:03.0/usb1/1-1/1-1:1.0/host33/target33:0:0/33:0:0:0/bsg/33:0:0:0
SUBSYSTEM=bsg
MAJOR=249
MINOR=2
DEVNAME=bsg/33:0:0:0
SEQNUM=3752
 */ 
  
