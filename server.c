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
struct string_matrix {
    char member[70];
};
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
    char content[3000];
    char msg[4000];
    int i=0;
    memset(content,'\0',sizeof(content));
    while((c=getc(ipt))!= EOF) {
        content[i]=c;
        i++;
    }
    sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s",status_code[OK],"OK",extensions[ty].mime_type,content);
    send(client_sock, msg, strlen(msg),0);
    return;

}
void send_dir_content(int client_sock,char content[])
{

    char msg[4000];
    memset(msg,'\0',sizeof(msg)-1);
    sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: directory\r\nServer: httpserver/1.x\r\n\r\n",status_code[OK],"OK");
    strcat(msg,content);
    send(client_sock, msg, strlen(msg),0);

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

int recv_request(int client_sock)
{
    pthread_mutex_lock(&mutex_consumer);
    int i;
    int u=0;
    for(i=0; i<20; i++)
        if(strlen(RQ[i].buffer)==0) {
            u=recv(client_sock,RQ[i].buffer,sizeof(RQ[i].buffer),0);
            break;
        }
    pthread_mutex_unlock(&mutex_consumer);
    return u;
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

    if(j==0)
        return;

    int flag=check_if_dir(FILE_OR_DIR);
    if(flag==1) {
        DIR* d;
        char temp1[128];
        char file_type[7];
        sscanf(FILE_OR_DIR,"%[^.].%s",temp1,file_type);

        int ty=check_file_type(file_type);//0 to 8 or -1

        char t[300];
        memset(temp1,'\0',strlen(temp1));
        getcwd(temp1,sizeof(temp1)-1);
        sprintf(t,"%s/testdir",temp1);
        int tmp=-1;
        tmp=chdir(t);
        if(tmp==-1) {
            printf("chdir error!!\n");

        } else {
            char file_path[300];
            memset(file_path,'\0',sizeof(file_path)-1);
            sprintf(file_path,".%s",FILE_OR_DIR);
            FILE *ipt;
            ipt=fopen(file_path,"r");
            if(ty==-1) {
                char msg[4000];
                sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: \r\nServer: httpserver/1.x\r\n\r\n",status_code[UNSUPPORT_MEDIA_TYPE],"UNSUPPORT_MEDIA_TYPE");
                send(client_sock, msg, strlen(msg),0);
            } else if(ipt) {
                send_file_content(client_sock,ipt,ty);
            } else {
                char msg[4000];
                sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: \r\nServer: httpserver/1.x\r\n\r\n",status_code[NOT_FOUND],"NOT_FOUND");
                send(client_sock, msg, strlen(msg),0);
            }
            if(ipt)
                fclose(ipt);
        }
    } else if(flag==0) {
        DIR *dir;
        struct dirent *p;
        char temp1[128];
        struct string_matrix str_m[100];
        int cnt=0;
        char t[300];
        char file_or_dir_name[70];

        memset(temp1,'\0',strlen(temp1));
        getcwd(temp1,sizeof(temp1)-1);

        sprintf(t,"%s/testdir%s",temp1,FILE_OR_DIR);
        dir=opendir(t); //stop here
        memset(t,'\0',sizeof(t));

        if(!dir) {
            char msg[4000];
            memset(msg,'\0',sizeof(msg));
            sprintf(msg,"HTTP/1.x %d %s\r\nContent-Type: \r\nServer: httpserver/1.x\r\n\r\n",status_code[NOT_FOUND],"NOT_FOUND");
            send(client_sock, msg, strlen(msg),0);
        } else {
            while(p=readdir(dir)) {
                memset(file_or_dir_name,'\0',sizeof(file_or_dir_name));
                memset(str_m[cnt].member,'\0',sizeof(str_m[cnt].member));

                strcpy(file_or_dir_name,p->d_name);
                if(file_or_dir_name[0]=='.' || file_or_dir_name[0]=='\0') {
                    continue;
                }
                strcpy(str_m[cnt].member,file_or_dir_name);
                cnt++;
            }
            char under_current_dir[300];
            memset(under_current_dir,'\0',sizeof(under_current_dir));
            for(i=0; i<cnt; i++) {
                if(i!=cnt-1) {
                    strcat(under_current_dir,str_m[i].member);
                    strcat(under_current_dir," ");
                } else
                    strcat(under_current_dir,str_m[i].member);

            }
            send_dir_content(client_sock,under_current_dir);


        }

    } else {
        char msg[4000];
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

    int client_sock=0;
    int b;
    request_queue_ini(); //RQ init
    int i;
    for(i=0; i<20; i++)
        thread_pool[i].is_alive=0;

    while(1) {
        client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_addr_size);
        /******request handling**************/
        if(!fork()) {
            b=1;
            while(b!=0) {
                b=recv_request(client_sock); // RQ recv
                choose_thread_in_pool(client_sock);
            }
            close(client_sock);
            exit(0);

        } else {
            close(client_sock);
        }
    }

    close(sock);
    return 0;
}

