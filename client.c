#include "client.h"
#include<pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
pthread_mutex_t lock;
struct request {
    char test[500];
    char FILE_OR_DIR[128];
    char LOCAL_HOST[50];
    char PORT[7];
    int sock;

};
struct string_array {
    char members[30];
};
void request_ini(struct request temp)
{
    memset(temp.test,'\0',sizeof(temp.test));
    memset(temp.FILE_OR_DIR,'\0',sizeof(temp.FILE_OR_DIR));
    memset(temp.LOCAL_HOST,'\0',sizeof(temp.LOCAL_HOST));
    memset(temp.PORT,'\0',sizeof(temp.PORT));

}
int check_if_dir(struct request temp)
{
    int i;
    for(i=0; i<strlen(temp.FILE_OR_DIR); i++)
        if(temp.FILE_OR_DIR[i]=='.')
            return 1;
    return 0;
}
void send_request(struct request temp)
{
    sprintf(temp.test,"GET %s HTTP/1.x\r\nHOST: %s:%s\r\n\r\n",temp.FILE_OR_DIR,temp.LOCAL_HOST,temp.PORT);
    send(temp.sock,temp.test,strlen(temp.test),0);
    return;
}
void recv_msg(int sock,char buffer[])
{
    pthread_mutex_lock(&lock);

    recv(sock, buffer,4000,0);
    printf("%s\n", buffer);

    pthread_mutex_unlock(&lock);

    return;
}
void list_all_under_dir(char buffer[],struct request temp,int sock)
{
    char content_type[20];
    char files_under_dir[250];
    char temp1[200];

    memset(content_type,'\0',sizeof(content_type));
    memset(files_under_dir,'\0',sizeof(files_under_dir));
    sscanf(buffer,"%[^:]: %s\r\nServer: httpserver/1.x\r\n\r\n%[_. -a-zA-Z0-9]",temp1,content_type,files_under_dir);
    memset(buffer,'\0',4000);

    if(strcmp(content_type,"directory")==0 && files_under_dir[0]!='\0') {
        char *d=" ";
        char *pt;
        pt=strtok(files_under_dir,d);
        struct string_array SA[100];
        int i,cnt=0;
        for(i=0; i<100; i++)
            memset(SA[i].members,'\0',sizeof(SA[i].members));
        i=0;
        while(pt!=NULL) {
            strcpy(SA[i].members,pt);
            pt=strtok(NULL,d);
            i++;

        }
        cnt=i;
        for(i=0; i<cnt; i++) {
            struct request temp2= {0};
            request_ini(temp2);
            strcpy(temp2.FILE_OR_DIR,temp.FILE_OR_DIR);
            strcpy(temp2.LOCAL_HOST,temp.LOCAL_HOST);
            strcpy(temp2.PORT,temp.PORT);
            temp2.sock=sock;

            strcat(temp2.FILE_OR_DIR,"/");
            strcat(temp2.FILE_OR_DIR,SA[i].members);//concatenate dir

            int flag=check_if_dir(temp2);

            char buffer1[4000];
            memset(buffer1,'\0',4000);
            if(flag==0) {
                send_request(temp2);
                recv_msg(sock,buffer1);
                list_all_under_dir(buffer1,temp2,sock);//execute recursive function

            } else {
                send_request(temp2);
                recv_msg(sock,buffer1);
            }
            memset(buffer,'\0',4000);
        }


    }
    return;



}
int main(int argc,char *argv[])
{
    struct sockaddr_in server;
    int i;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");//host
    server.sin_port = htons(1234);//port

    connect(sock, (struct sockaddr*)&server, sizeof(server));//connect

    struct request temp= {0};
    request_ini(temp);//request init
    if(argc>=7) {
        if(strlen(argv[2])<128)
            strcpy(temp.FILE_OR_DIR,argv[2]);
        if(strlen(argv[4])<50)
            strcpy(temp.LOCAL_HOST,argv[4]);
        if(strlen(argv[6])<7)
            strcpy(temp.PORT,argv[6]);
    }
    temp.sock=sock;
    if(strcmp(argv[4],"127.0.0.1")!=0 || strcmp(argv[6],"1234")!=0 ) { //connection failed
        printf("failed to connect the server!\n");
        return 0;
    }

    char buffer[4000];
    memset(buffer,'\0',4000);

    send_request(temp); //send 1st request
    recv_msg(sock,buffer);

    if(check_if_dir(temp)==0)
        list_all_under_dir(buffer,temp,sock);//if 1st request is for a dir




    close(sock);
    return 0;
}
