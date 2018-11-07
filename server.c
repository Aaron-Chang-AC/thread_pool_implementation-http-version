#include "server.h"
#include "status.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include<pthread.h>
#include<dirent.h>
struct request_queue {
    char buffer[500];
};
struct thread_members {
    pthread_t thread;
    int is_alive;
};
struct request_queue RQ[20]; //RQ[20]
pthread_mutex_t mutex_producer; //mutex for the producer(a lock)
pthread_mutex_t mutex_consumer; //mutex for the consumer(a lock)
struct thread_members thread_pool[20]; //TP[20]

void send_file_content(int client_sock,FILE *ipt,int ty)//take care if ty is -1
{
    char c;
    char content[300];
    char msg[500];
    int i=0;
    memset(content,'\0',sizeof(content));
    while((c=getc(ipt))!= EOF) {
        content[i]=c;
        i++;
    }
    fclose(ipt);
    sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s",status_code[OK],"OK",extensions[ty].mime_type,content);
    send(client_sock, msg, strlen(msg),0);
    return;

}
int check_if_dir(char FILE_OR_DIR[])//1 for file, and 0 for directory
{
    int i;
    if(FILE_OR_DIR[0] != '/')
        return -1;
    for(i=0; i<strlen(FILE_OR_DIR); i++)
        if(FILE_OR_DIR[i]=='.')
            return 1;
    return 0;
}

void recv_request(int client_sock)
{
    pthread_mutex_lock(&mutex_consumer);
    int i;
    for(i=0; i<20; i++)
        if(strlen(RQ[i].buffer)==0) {
            recv(client_sock,RQ[i].buffer,sizeof(RQ[i].buffer),0);
            break;
        }
    pthread_mutex_unlock(&mutex_consumer);
    return;
}
void request_queue_ini(void)
{
    int i;
    for(i=0; i<20; i++)
        memset(RQ[i].buffer,'\0',sizeof(RQ[i].buffer));
    return;
}
int check_file_type(char file_type[7])
{
    int i;
    printf("%s\n",file_type);
    for(i=0; i<8; i++) {
        if(strcmp(file_type,extensions[i].ext) == 0) {
            return i;
        }
    }
    return -1;
}
void request_decode(int client_sock)
{
    pthread_mutex_lock(&mutex_producer);
    int i,j=0;
    char FILE_OR_DIR[128];
    for(i=0; i<20; i++) {
        if(strlen(RQ[i].buffer)!=0) {
            memset(FILE_OR_DIR,'\0',sizeof(FILE_OR_DIR));
            sscanf(RQ[0].buffer,"GET %s HTTP/1.x\r\nHOST: 127.0.0.1:1234\r\n\r\n",FILE_OR_DIR);
            memset(RQ[i].buffer,'\0',sizeof(RQ[i].buffer));
            j=1;
            break;
        }
    }
    if(j==0)//if no request is sent by the client
        return;

    int flag=check_if_dir(FILE_OR_DIR);
    if(flag==1) { //if the target is a file
        DIR* d;
        char temp1[128];
        char file_type[7];
        sscanf(FILE_OR_DIR,"%[^.].%s",temp1,file_type);

        int ty=check_file_type(file_type);//0~8 or-1

        char t[200]="/home/os2018/hw2-simple-my-http-server-Aaron-Chang-AC/testdir"; //this can be changed by the user
        int tmp=chdir(t);//0 for success -1 for errors
        if(tmp==-1) {
            printf("Please ensure that the directory is /home/os2018/hw2-simple-my-http-server-Aaron-Chang-AC/testdir\n");

        } else {
            char file_path[200];
            memset(file_path,'\0',sizeof(file_path));
            sprintf(file_path,".%s",FILE_OR_DIR);

            FILE *ipt=fopen(file_path,"r");
            if(ty==-1) { //415 UNSUPPORT_MEDIA_TYPE
                char msg[500];
                sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: \r\nServer: httpserver/1.x\r\n\r\n",status_code[UNSUPPORT_MEDIA_TYPE],"UNSUPPORT_MEDIA_TYPE");
                send(client_sock, msg, strlen(msg),0);
            } else if(!ipt) {
                send_file_content(client_sock,ipt,ty);
            } else { //404 NOT_FOUND
                char msg[500];
                sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: \r\nServer: httpserver/1.x\r\n\r\n",status_code[NOT_FOUND],"NOT_FOUND");
                send(client_sock, msg, strlen(msg),0);
            }
        }
    } else if(flag==0) { //if the target is a directory

    } else { //BAD_REQUEST
        char msg[500];
        sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: \r\nServer: httpserver/1.x\r\n\r\n",status_code[BAD_REQUEST],"BAD_REQUEST");
        send(client_sock, msg, strlen(msg),0);
    }
    pthread_mutex_unlock(&mutex_producer);
    return;

}
void *thread_start(void* data)
{
    char *c=(char*)data;
    int client_sock=atoi(c);
    request_decode(client_sock);
    return 0;
}
void choose_thread_in_pool(int client_sock)//choose a thread in the thread pool
{
    int i;
    for(i=0; i<20; i++) {
        if(thread_pool[i].is_alive==0) {
            char c[20];
            sprintf(c,"%d",client_sock);
            int ret=pthread_create(&thread_pool[i].thread,NULL,thread_start,c);
            thread_pool[i].is_alive=1;
            pthread_join(thread_pool[i].thread,NULL);
            thread_pool[i].is_alive=0;
            break;
        }
    }
    return;
}
int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server;

    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(1234);
    bind(sock, (struct sockaddr*)&server, sizeof(server));

    listen(sock, 20);

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    int client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_addr_size);

    /******request handling**************/
    request_queue_ini(); //RQ init

    int i;
    for(i=0; i<20; i++)
        thread_pool[i].is_alive=0;

    while(1) {
        recv_request(client_sock); // RQ recv
        choose_thread_in_pool(client_sock);

    }



    close(client_sock);
    close(sock);
    return 0;
}

