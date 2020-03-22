#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>

#define BUFSIZE 4096
#define  PORT 8900

void exec(const char * command, char * result)
{
    FILE *fpRead;

    memset(result,0,BUFSIZE);
    fpRead = popen(command, "r");
    char buf[BUFSIZ] = {0};
    memset(buf,'\0',sizeof(buf));

    while(fgets(buf,BUFSIZ-1,fpRead)!=NULL)
    {         
        strcat(result, buf);
    }
    if(fpRead!=NULL)
        pclose(fpRead);
}
                
void* th_func(void* arg){
    char buf[BUFSIZ];
    char send_buf[BUFSIZ];
    char command[BUFSIZE];
    memset(send_buf,0,BUFSIZ);
    memset(buf,0,sizeof(buf));
    memset(command,0,sizeof(command));
    int number;
    char cmd[]="sh -c ";
    int connectd; 
    connectd=(int)(long) arg;
    
    while(1){
        number = recv(connectd,buf,(int)(sizeof(buf)),0);
        if (0==number)
        {
            fprintf(stderr,"the other side has been closed\n");
        }
        if(0>number)
        {
            fprintf(stderr,"error in com\n");
            close(connectd);
        }	        
        buf[number]='\0';
        fprintf(stderr,"recv message:%s\n",buf);
        
        if(strcmp(buf,"quit")==0)
        {
            fprintf(stderr,"the client is quit\n");
            close(connectd);
        }
        sprintf(command,"%s%s",cmd,buf);
        exec(command,send_buf);
            
        fprintf(stderr,"send message\n:%s\n",send_buf);
        if(0>send(connectd,send_buf,strlen(send_buf),0))
        {
            perror("communication error\n");
            close(connectd);
        }

    }
    
    close(connectd);
    pthread_exit(NULL); 
}

int main()
{
    int sockfd;
    struct sockaddr_in serv;
    struct sockaddr_in client;
    char buf[BUFSIZ];
    char send_buf[BUFSIZ];
    int number;
    int length;
    int opt;
    char command[BUFSIZE];
    int connectd;
    int len;

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if (0>sockfd)
    {
        fprintf(stderr,"error in creating socket\n");
        exit(-1);
    }

    // set the address rebinding
    opt=1;
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))!=0)
    {
        fprintf(stderr,"error in setting socket option\n");
        close(sockfd);
        exit(-1);
    }

    // set the value for the socket
    memset(buf,0,sizeof(buf));
    memset(send_buf,0,sizeof(send_buf));
    memset(&serv,0,sizeof(serv));
    memset(&client,0,sizeof(client));
    serv.sin_family=AF_INET;
    serv.sin_port=htons(PORT);
    serv.sin_addr.s_addr=htonl(INADDR_ANY);

    // bind the socket
    if (bind(sockfd,(struct sockaddr*)&serv,sizeof(serv))!=0)
    {
        fprintf(stderr,"error in binding server address\n");
        exit(-1);
    }

    //listen
    if (-1==listen(sockfd,10))
    {
        perror("listen error\n");
        exit(1);
    }
    
    // communication
    while(1)
    {
        //accept
        if (-1==(connectd=accept(sockfd,(struct sockaddr*)&client,&len)))
    	{
		    perror("create connect socket error\n");
		    return -1;
    	}
    
        pthread_t tid;
        int con;
        con=connectd;

        if(pthread_create(&tid,NULL,th_func,(void*)(long)con)!=0)
                {
                    perror("pthread_create failed\n");
                    free((void*)tid);
                    exit(-2);
                }                
    }
    close(sockfd);
    return 0;
    
}
