#ifndef CLIENT_H
#define CLIENT_H
#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>
#include"../common/common.h"
extern int readn( int fd, char *bp, size_t len);
extern void* RecvThread(void* point);
extern void registe(char*username1,char*password,int *id,int sockfd);
extern void login(char*username1,char*password,int *id,int sockfd);
extern void CheckOn(int *id,int sockfd);
extern void CheckTop(int *id,int sockfd);
extern void SignOut(char*username1,int *id,int sockfd);
extern void LevMsg(char*username1,char*pname,char*msg,int *id,int sockfd);
extern void Challange(char*username1,char*pname,int *id,int sockfd);
extern void Battle(char*username1,char act,int *id,int sockfd);
extern int sockfd;
extern int is_reg;
extern int is_log;
extern int have_mes;
extern int is_revcha;
extern int is_won;
extern int is_over;
extern char is_shell;
extern char want;
extern pthread_mutex_t mutex_want;
extern pthread_cond_t cond_want;
extern pthread_mutex_t mutex_reg;
extern pthread_cond_t cond_reg;
extern pthread_mutex_t mutex_log; 
extern pthread_cond_t cond_log;
extern pthread_mutex_t mutex_mes; 
extern pthread_cond_t cond_mes; 
extern pthread_mutex_t mutex_revcha;
extern pthread_cond_t cond_revcha;
extern pthread_mutex_t mutex_won;
extern pthread_cond_t cond_won; 
extern pthread_mutex_t mutex_shell;
extern pthread_cond_t cond_shell;
extern char username[16];
extern char itime;
#endif
