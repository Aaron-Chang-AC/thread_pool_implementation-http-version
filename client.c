#include "client.h"
#include<pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
struct request {
    char test[500];
    char FILE_OR_DIR[128];
    char LOCAL_HOST[50];
    char PORT[7];
    int sock;

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
    recv(sock, buffer, 500,0);
    printf("%s", buffer);
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
    send_request(temp); //send 1st request
    char buffer[500];
    while(1) {
        memset(buffer,'\0',sizeof(buffer));
        recv_msg(sock,buffer);
    }





    close(sock);
    return 0;
}
