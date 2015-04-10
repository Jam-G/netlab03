#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include"server.h"
#include"../common/common.h"
extern void testsignup();
extern void testsignin(char *, char *);
extern void testtop10();
int sockfd;
int main(int argc, char **argv){
	struct sockaddr_in servaddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(8888);
	int c = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	printf("connect :%d\n", c);
	testtop10();
	return 0;
}
void testsignup(){
	
	struct SGP sgp;
	memset(&sgp, 0, sizeof(sgp));
	sgp.head.id = 0;
	strcpy(sgp.head.type, "SGP");
	strcpy(sgp.account.name, "jam");
	strcpy(sgp.account.passwd, "jam");
	send(sockfd, &sgp, sizeof(sgp), 0);
	struct RSP rsp;
	recv(sockfd, &rsp, sizeof(rsp), 0);
	printf("rsp:%s\n%c", rsp.head.type, rsp.ans);
}
void testsignin(char *user, char *passwd){
	struct SIN sin;
	memset(&sin, 0, sizeof(sin));
	sin.head.id = 1;
	strcpy(sin.head.type, "SIN");
	strcpy(sin.account.name, user);
	strcpy(sin.account.passwd, passwd);
	send(sockfd, &sin, sizeof(sin), 0);
	struct RIN rin;
	recv(sockfd, &rin, sizeof(rin),0);
	printf("rin:%c", rin.info);
}

void testtop10(){
	struct Header head;
	memset(&head, 0, sizeof(head));
	head.id = 3;
	strcpy(head.type, "TOP");
	send(sockfd, &head, sizeof(head), 0);
	struct TOP top10;
	memset(&top10, 0, sizeof(top10));
	recv(sockfd, &top10, sizeof(top10), 0);
	int i = 0;
	for(i = 0; i < 10; i ++){
		printf("No.%d name:%s, victory:%d\n",i+1, top10.top10[i].name, top10.top10[i].victory);
	}
}
