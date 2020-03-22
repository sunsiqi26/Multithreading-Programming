#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>

#define BUFSIZE 4096
#define  PORT 8900

#define THREAD_NUM 100

typedef struct port_seg
{
    struct in_addr dest;
    unsigned int min_port;
    unsigned int max_port;
}port_segment;

void usage(char* str)
{
    printf("the command %s usage is:\n",str);
    printf("%s ip_address[port]\n",str);
    exit(-1);
}

int scan_port(struct sockaddr_in dst, int p)
{
    int sockfd;
    struct servent* sp;
	sockfd=socket(AF_INET,SOCK_STREAM,0);
    if (0>sockfd)
    {
        fprintf(stderr,"error in creating socket\n");
        exit(-1);
    }
    if (0>(connect(sockfd,(struct sockaddr*)&dst,sizeof(struct sockaddr))))
    {
        close(sockfd);
        return -1;
    }
    else
    {
        sp=getservbyport(htons(p),"tcp");
        if(NULL!=sp)
            printf("%s : %d %s (thread_id:%ld)\n",inet_ntoa(dst.sin_addr),ntohs(dst.sin_port), sp->s_name,syscall(SYS_gettid));
        else
        {
            printf("%s : %d unknow (thread_id:%ld)\n",inet_ntoa(dst.sin_addr),ntohs(dst.sin_port),syscall(SYS_gettid));
        }        
	return 1;
}
}

void* tscan(void* arg){
    port_segment port;
    int sockfd;
    struct sockaddr_in scan;
    
    memcpy( &port, arg, sizeof(port_segment) );
    memset( &scan, 0, sizeof(struct sockaddr_in) );
    scan.sin_family=AF_INET;
    scan.sin_addr.s_addr=port.dest.s_addr;

    for(int i=port.min_port;i<=port.max_port;i++)
    {
        scan.sin_port=htons(i);
        if(scan_port(scan,i)<0){continue;}
    }
    return NULL;
}

int main(int argc,char**argv)
{
    int sockfd;
    struct servent* sp;
    //int port;
    int SEG_LEN=65535/100;
    struct in_addr dest_ip[ 5 ];

    if (2>argc)
    {
        usage(argv[0]);
        exit(-1);
    }
    for ( int i = 1; i < argc; ++i ) {
        if ( inet_aton(argv[i], &dest_ip[i - 1]) == 0 ) {
            fprintf( stderr, "invalid ip address.\n" );
            exit ( EXIT_FAILURE );
        }
    }

    for(int j=0;j<argc-1;++j){
        pthread_t *thread;
        thread=(pthread_t*)malloc(THREAD_NUM * sizeof(pthread_t));
        for(int i=0;i<THREAD_NUM;++i){
            port_segment port;
            port.dest=dest_ip[j];
            port.min_port=i*SEG_LEN + 1;
            /*THE LAST SEGMENT*/
            if(i==(THREAD_NUM-1))
                //port.max_port=MAX_PORT;
                port.max_port=65535;
            else
                port.max_port=port.min_port+SEG_LEN-1;
            if(pthread_create(&thread[i],NULL,tscan,(void*)&port)!=0)
            {
                perror("pthread_create failed\n");
                free(thread);
                exit(-2);
            }		
            pthread_join(thread[i],NULL);		
        }
        free(thread);    
    }
    return 0;
    
}
