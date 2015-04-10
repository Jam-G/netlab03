#ifndef SERVER_H
#define SERVER_H
#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<pthread.h>
#include"../common/common.h"
#define MAXACCOUNT 1024 //最大支持用户数
#define MAXONLINE 512	//最大在线用户数
#define SERV_PORT 8888
#define LISTENQ 8
#define true 1
#define false 0


struct InterAccount{
	char name[16];
	char passwd[16];
	char state;
	int socketfd;//if online then has a socketfd
	pthread_t tid;//the thread id
	char mate[16];//挑战id
	int blood;
	pthread_cond_t cond;
	//接下来是一组信号量,counter是出拳者数量为2时进行判断结果.ans是判断的结果
	//两个线程中只需要一个进行裁决即可.即维护了一致性也节省了运算
	//这组变量在挑战成功时由`挑战者线程初始化,并由后出拳者进行销毁操作
	pthread_cond_t *cond_fgt;
	pthread_mutex_t *mutex_fgt;
	int *counter_fgt;
	char *goon;
	char *ans;
	int ivin;
	int vin;
};
enum Case{
	SGP,RSP,SIN,RIN,SOT,GET,CHL,RCH,FGT,ANS,END,MSG,TOP,ERROR
};
extern struct InterAccount users[MAXACCOUNT];//all the users
extern struct Top itop10[10]; //top 10 users
extern int readn(int fd, char *bp, size_t len, struct InterAccount *me);
extern void * proccessor(void *);
extern pthread_mutex_t mutextop10, mutexusertable, mutexfile;
extern FILE *fp;
extern int userNum;//the usernum
extern int online;//the online num
extern enum Case type_case(char *type);
extern void signup(struct Header *head, struct InterAccount *me);
extern void signin(struct Header *head, struct InterAccount **me);
extern void signout(struct Header *head, struct InterAccount **me);
extern void challenge(struct Header *head, struct InterAccount *me);
extern void rechallenge(struct Header *head, struct InterAccount *me);
extern void message(struct Header *head, struct InterAccount *me);
extern void top10(struct Header *head, struct InterAccount *me);
extern void get(struct Header *head, struct InterAccount *me);
extern void fight(struct Header *head, struct InterAccount *me);
extern void error();
extern int judge(char ch1, char ch2);
extern void upgradetop10(struct InterAccount *me);
#endif
