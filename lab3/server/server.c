#include"server.h"
#include"../common/common.h"
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<time.h>
#include<errno.h>
/*the shared data*/
struct InterAccount users[MAXACCOUNT];
struct Top itop10[10];
pthread_mutex_t mutextop10 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexusertable = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexfile = PTHREAD_MUTEX_INITIALIZER;//the mutex for the tables
FILE *fp;
int userNum;
int online;
/*time_t timers[MAXACCOUNT];

void * mytimer(void *){
	
	int i = 0;
	for(i = 0; i < MAXACCOUNT; i ++){
		timers[i] = -1;//初始化为负数未启动，置1启动
	}
	while(1){
		for(i = 0; i < MAXACCOUNT; i ++){
			
			if(timers[i] > 0){
					time_t now;
					time(&now);
					if((now - time[i]) > 10){
						struct END timeout;
						
					}
			}
		}
	}
}*/

int readn( int fd, char *bp, size_t len, struct InterAccount *me)
{
	int cnt;
	int rc;
	cnt = len;
	time_t timebegin, timeouter;
	if(me ->state != 'F')
		time(&timebegin);
	//printf("read time:%ld\n", timebegin);
	while ( cnt > 0 ){
		rc = recv( fd, bp, cnt, 0);
		if ( rc < 0 ){	/* read error? */
			if(errno != EINTR)
				return -2;
			time(&timeouter);
			if(me ->state != 'F' && (timeouter - timebegin) > 15){//15s超时
				printf("timeout time:%ld\n", timeouter);
				printf("in readn rc is %d\n", rc);
				fflush(stdout);
				return -1;				/* return error */
			}
			else continue;
			
		}
		if ( rc == 0 )				/* EOF? */
			return 0;		/* return short count */
		bp += rc;
		cnt -= rc;
	}
//	printf("readn:%d",len);
	return len;
}
void init(){//initial the users table and top10 table ,is called only once on the begining
	fp = fopen("users.db", "r");//read the local data
	if(fp == NULL){
		perror("user data is lost!\n");
		exit(-1);
	}
	char buffer[512];
	userNum = 0;
	while(fgets(buffer, 512, fp) != NULL){
		char *str = strtok(buffer, "\t");
		strncpy(users[userNum].name, str, 16);
		str = strtok(NULL, "\t");
		strncpy(users[userNum].passwd, str, 16);
		users[userNum].passwd[strlen(str) - 1] = 0;
		users[userNum].state = 'O';//offline, then F is free B is fighting C is be challenged
		users[userNum].socketfd = -1;//negitive to present no socket
		users[userNum].tid = 0;
		memset(users[userNum].mate, 0 , 16);
		pthread_cond_init(&(users[userNum].cond), NULL);
		userNum ++;
	}
	online = 0;
	//the other is not used
	memset(&users[userNum], 0, sizeof(struct InterAccount) * (MAXACCOUNT - userNum));
	fclose(fp);
	printf("the user infomation is readed! there are %d users\n", userNum);	
}

int main(int argc, char **argv){ 
	init();//initial
	int listenfd, connfd;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	int bin = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	printf("bind ans: %d\n", bin);
	listen(listenfd, LISTENQ);
	printf("Server running... waiting for connections.\n");
	while(true){
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		//then create thread to proccess
		pthread_t ctid;
		int rc = pthread_create(&ctid, NULL, proccessor, (void *)connfd);
		if(rc){
			printf("ERROR: return code from pthread_create() is %d\n", rc);
			close(connfd);//close the client socket
			continue;//can not accept the new connection;
		}
	}
	pthread_mutex_destroy(&mutextop10);
	pthread_mutex_destroy(&mutexfile);
	pthread_mutex_destroy(&mutexusertable);
	pthread_exit(NULL);
}

enum Case type_case(char *type){
	if(strncmp(type, "SGP", 4) == 0)
		return SGP;
	else if(strncmp(type, "RSP", 4) == 0)
		return RSP;
	else if(strncmp(type, "SIN", 4) == 0)
		return SIN;
	else if(strncmp(type, "RIN", 4) == 0)
		return RIN;
	else if(strncmp(type, "SOT", 4) == 0)
		return SOT;
	else if(strncmp(type, "GET", 4) == 0)
		return GET;
	else if(strncmp(type, "CHL", 4) == 0)
		return CHL;
	else if(strncmp(type, "RCH", 4) == 0)
		return RCH;
	else if(strncmp(type, "FGT", 4) == 0)
		return FGT;
	else if(strncmp(type, "ANS", 4) == 0)
		return ANS;
	else if(strncmp(type, "END", 4) == 0)
		return END;
	else if(strncmp(type, "MSG", 4) == 0)
		return MSG;
	else if(strncmp(type, "TOP", 4) == 0)
		return TOP;
	else return ERROR;
}

void * proccessor(void * sock){
	pthread_detach(pthread_self());
	struct InterAccount *me;
	me = malloc(sizeof(struct InterAccount));
	me ->socketfd = (int)sock;
	me ->tid = pthread_self();//get self tid
	struct Header head;
	struct timeval mytimer = {15,0};//10s timeout
	setsockopt(me->socketfd,SOL_SOCKET,SO_RCVTIMEO, (char *)&mytimer,sizeof(struct timeval));
	while(true){
		memset(&head, 0, sizeof(head));
	//	setsockopt(me->socketfd,SOL_SOCKET,SO_RCVTIMEO, (char *)&mytimer,sizeof(struct timeval));
		int rc = readn((int)sock, &head, sizeof(struct Header), me);
		if(rc == 0 || rc == -1){//链接关闭.,-1表示超时tuichu
			
		
			if(me ->state == 'C'){//bussy
				struct END timeout;
				timeout.head.id = 0;
				strncpy(timeout.head.type, "END", 4);
				timeout.end = 'T';//timeout;
				//readn(me->socketfd, &(rch.chl), sizeof(struct Challenge));
				send(me ->socketfd, &timeout, sizeof(struct END), 0);
				pthread_mutex_lock(&mutexusertable);
				memset(me->mate, 0, 16);
				me ->state = 'F';//空闲
				me ->blood = 2;
				me ->ivin = 0;
				pthread_cond_broadcast(&(me->cond));//唤醒所有挑战自己的线程
				pthread_mutex_unlock(&mutexusertable);
				if(rc == 0){
					signout(NULL, &me);
					pthread_exit(NULL);
				}
				//continue;
			}else if(me ->state == 'B'){//战斗中超时说明未出拳；
				
				struct END timeout;
				timeout.head.id = 0;
				strncpy(timeout.head.type, "END", 4);
				timeout.end = 'O';//timeout;
				//readn(me->socketfd, &(rch.chl), sizeof(struct Challenge));
				send(me ->socketfd, &timeout, sizeof(struct END), 0);
				pthread_mutex_lock(me->mutex_fgt);
				pthread_mutex_lock(&mutexusertable);
				memset(me->mate, 0, 16);
				me ->state = 'F';//空闲
				me ->blood = 2;
				me ->ivin = 0;
				(me -> ans) = 'O';
				pthread_mutex_unlock(&mutexusertable);
				pthread_cond_broadcast(me->cond_fgt);//唤醒对战自己的自己的线程
				pthread_mutex_unlock(me->mutex_fgt);
				if(rc == 0){
					signout(NULL, &me);
					pthread_exit(NULL);
				}
				//continue;
			}
			continue;
		}else if (rc < 0){
			printf("error num:%d\n", rc);
			continue;
		}
		printf("debug :head type:%s\n", head.type);
		switch(type_case(head.type)){
			case SGP: signup(&head, me);break;
			case SIN: signin(&head, &me);break;
			case SOT: signout(&head, &me);break;
			case CHL: challenge(&head, me);break;
			case RCH: rechallenge(&head, me);break;
			case MSG: message(&head,  me);break;
			case TOP: top10(&head, me);break;
			case GET: get(&head, me);break;
			case FGT: fight(&head, me);break;
			default:error();//other message should not be recive here
		}
	}
}

void signup(struct Header *head, struct InterAccount *me){
	struct RSP rsp;
	rsp.head.id = head ->id;
	strncpy(rsp.head.type, "RSP", 4);
	//填充结构头部
	//
	struct Account newuser;
	readn(me->socketfd, &newuser, sizeof(newuser), me);
	int i = 0;

	pthread_mutex_lock(&mutexusertable);
	for(i = 0; i < userNum; i ++){
		if(strcmp(newuser.name, users[i].name) == 0){//already signed
			rsp.ans = 'F';//失败
			break;
		}
	}
	if(rsp.ans != 'F'){
		rsp.ans = 'S';//注册成功
		memcpy(&users[userNum], &newuser, sizeof(newuser));
		users[userNum].state = 'O';
		users[userNum].socketfd = -1;
		users[userNum].tid = 0;
		pthread_cond_init(&(users[userNum].cond), NULL);
		users[i].vin = 0;
		userNum ++;
	}
	pthread_mutex_unlock(&mutexusertable);
	if(rsp.ans == 'S'){
		pthread_mutex_lock(&mutexfile);
		fp = fopen("users.db", "a");
		if(fp == NULL){
			perror("users data lost!\n");
			exit(-1);
		}
		fprintf(fp, "%s\t%s\n", newuser.name, newuser.passwd);
		printf("debug:%s pid:%d\n", newuser.passwd, pthread_self());
		fclose(fp);
		pthread_mutex_unlock(&mutexfile);
	}
	send(me ->socketfd, &rsp, sizeof(struct RSP), 0);
}

void signin(struct Header *head, struct InterAccount **me){
	struct RIN rin;
	rin.head.id = head ->id;
	strncpy(rin.head.type, "RIN", 4);
	struct Account newlogin;
	readn((*me) ->socketfd, &newlogin, sizeof(newlogin), *me);
	printf("debug:login:name:%s,passwd:%s\n", newlogin.name, newlogin.passwd);
	int i = 0;
	pthread_mutex_lock(&mutexusertable);
	for(i = 0; i < userNum; i ++){
		printf("debug:has name:%s, has passwd:%s\n", users[i].name, users[i].passwd);
		if(strcmp(newlogin.name, users[i].name) == 0 && \
				strcmp(newlogin.passwd, users[i].passwd) == 0){
			printf("debug find the user login successful!\n");
			users[i].state = 'F';
			users[i].socketfd = (*me)->socketfd;
			users[i].tid = (*me) ->tid;
			free(*me);
			*me = &(users[i]);
			rin.info = 'S';
			online ++;
			break;
		}
	}

	struct UPD upd;
	upd.head.id = 0;
	strncpy(upd.head.type, "UPD", 4);
	memcpy(upd.state.name, (*me) ->name, 16);
	upd.state.state = 'F';
	int k = 0;
	for(k = 0; k < userNum; k ++){
		if(users[k].state != 'O' && k != i)
			send(users[k].socketfd, &upd, sizeof(struct UPD), 0);
	}
	pthread_mutex_unlock(&mutexusertable);
	
	if(rin.info != 'S'){
		rin.info = 'F';//登录失败
	}else{//登录成功
		memcpy(*me, &newlogin, sizeof(newlogin));
		(*me) ->state = 'F';//空闲
	}
	send((*me) ->socketfd, &rin, sizeof(struct RIN), 0);
}

void signout(struct Header *head, struct InterAccount **me){
	char name[16];
	if((*me) ->state == 'O')//直接未登录可以直接返回
		return;
	readn((*me) ->socketfd, name, 16, *me);
	
	int i = 0;
	pthread_mutex_lock(&mutexusertable);

	(*me) ->state = 'O';//offline
	online --;
	struct UPD upd;
	upd.head.id = 0;
	strncpy(upd.head.type, "UPD", 4);
	memcpy(upd.state.name, (*me) ->name, 16);
	upd.state.state = 'O';
	int k = 0;
	for(k = 0; k < userNum; k ++){
		if(users[k].state != 'O')
			send(users[k].socketfd, &upd, sizeof(struct UPD), 0);
	}
	pthread_mutex_unlock(&mutexusertable);
	struct InterAccount *temp  = malloc(sizeof(struct InterAccount));
	temp ->tid = (*me)->tid;
	temp ->socketfd = (*me)->socketfd;
	(*me)  = temp;
}

void challenge(struct Header *head, struct InterAccount *me){
	
	struct CHL chl;
	memcpy(&(chl.head), head, sizeof(struct Header));
	
	readn(me ->socketfd, &(chl.chl), sizeof(struct Challenge), me);

	
	memset(me ->mate, 0, 16);//初始化为没有mate
	pthread_mutex_lock(&mutexusertable);
	int i = 0;
	int chsock = -1;
	me ->state = 'C';
	for(i = 0; i < userNum; i ++){
		if(strncmp(chl.chl.recipient, users[i].name, 16) == 0){
			if(users[i].state == 'F'){// free
				chsock = users[i].socketfd;
				users[i].state = 'C';//置为被挑战
				break;
			}
		}
	}

	if(chsock == -1){//there no user can challenge then chsock never change
		chl.chl.state = 'R';
	}else{//can be challenged
		send(chsock, &chl, sizeof(struct CHL), 0);
		pthread_cond_wait(&(users[i].cond), &mutexusertable);//等待在自己身上
		if(users[i].state == 'B' && strncmp(users[i].mate, me->name, 16) == 0){//means recive the challenge
			me ->state = 'B';//战斗中
			me ->blood = 2;//血量初始化为2,为3局两胜制
			me ->cond_fgt = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
			me ->mutex_fgt = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
			me ->counter_fgt = (int *)malloc(sizeof(int));
			me ->ans = (char *)malloc(sizeof(char));
			me ->goon = (char *)malloc(sizeof(char));
			//init pthread
			*(me->counter_fgt) = 0;
			*(me ->goon) = 'Y';
			*(me->ans) = '?';//unknow;
			pthread_mutex_init(me->mutex_fgt, NULL);
			pthread_cond_init(me ->cond_fgt, NULL);
			//对战双方的对战信息指向同一块内存
			users[i].cond_fgt = me->cond_fgt;
			users[i].mutex_fgt = me ->mutex_fgt;
			users[i].counter_fgt = me ->counter_fgt;
			users[i].ans = me->ans;
			users[i].goon = me->goon;
			memcpy(me->mate, chl.chl.recipient, 16);
			//初始化对战信息使用
			me->ivin = 0;
			chl.chl.state = 'A';//accept 
		}else{
			chl.chl.state = 'R';//refuse
			me ->state = 'F';
		}
	}
	pthread_mutex_unlock(&mutexusertable);
	strcpy(chl.head.type, "RCH");
	send(me->socketfd, &chl, sizeof(struct CHL), 0);
}

void rechallenge(struct Header *head, struct InterAccount *me){
	
	struct RCH rch;
	memcpy(&(rch.head), head, sizeof(struct Header));
	readn(me->socketfd, &(rch.chl), sizeof(struct Challenge), me);
	pthread_mutex_lock(&mutexusertable);
	if(me->state == 'F')
		;
	else if(rch.chl.state == 'A'){
		memcpy(me->mate, rch.chl.initiator, 16);
		me ->state = 'B';//战斗中
		me ->blood = 2;
		me ->ivin = 0;
	}else
		me ->state = 'F';
	pthread_cond_broadcast(&(me->cond));//唤醒所有挑战自己的线程
	pthread_mutex_unlock(&mutexusertable);
}

void message(struct Header *head, struct InterAccount *me){
	struct MSG msg;
	memcpy(&(msg.head), head, sizeof(struct Header));
	readn(me ->socketfd, &(msg.msg), sizeof(struct Message), me);
	
	pthread_mutex_lock(&mutexusertable);
	int i = 0;
	for(i = 0; i < userNum; i ++){
		if(users[i].state != 'O' && strncmp(msg.msg.reciver, users[i].name, 16) == 0){
			send(users[i].socketfd, &msg, sizeof(struct MSG), 0);
			break;
		}
	}
	pthread_mutex_unlock(&mutexusertable);

}

void top10(struct Header *head, struct InterAccount *me){
	struct TOP top;
	memcpy(&(top.head), head, sizeof(struct Header));
	
	pthread_mutex_lock(&mutextop10);
	memcpy(top.top10, itop10, sizeof(struct Top) * 10);
	pthread_mutex_unlock(&mutextop10);
	send(me ->socketfd, &top, sizeof(struct TOP), 0);
}	

void get(struct Header *head, struct InterAccount *me){
	int size = 0;
	pthread_mutex_lock(&mutexusertable);
	size = sizeof(struct States) * online + sizeof(struct Header) + sizeof(int);
	char *buf = (char *)malloc(size);
	int *c = &buf[sizeof(struct Header)];
	*c = online;
	memcpy(buf, head, sizeof(struct Header));
	struct States *sta = &buf[sizeof(struct Header) + sizeof(int)];
	int i = 0;
	for(i = 0; i <	userNum; i ++){
		if(users[i].state != 'O'){//online 'O' means offline
			memcpy(sta->name, users[i].name, 16);
			sta->state = users[i].state;
			sta ++;
		}
	}		
	pthread_mutex_unlock(&mutexusertable);
	send(me ->socketfd, buf, size, 0);
	free(buf);
}

void fight(struct Header *head, struct InterAccount *me){
	struct Step step;
	readn(me ->socketfd, &step, sizeof(struct Step), me);
	int des = 0;
	pthread_mutex_lock(me->mutex_fgt);
	if(me ->state != 'B'){
		printf("useless FGT messge\n");
		fflush(stdout);
		pthread_mutex_unlock(me->mutex_fgt);
		return;
	}
	if(*(me->counter_fgt) == 1){//对面已经出拳了,出拳保存在ans中
		*(me->counter_fgt) = 0;
		char ians;
		int tj = judge(step.step, *(me->ans));
		if(tj > 0){//win
			*(me->ans) = 'F';//胜利了自己,则对方就是失败
			me ->ivin ++;
			ians = 'V';
		}else if(tj < 0){
			*(me->ans) = 'V';//对方胜利
			(me->blood) --;
			ians = 'F';
		}else{//平局
			ians = 'P';
			*(me->ans) = 'P';
		}
		if(me ->blood == 0 || me->ivin == 2){//自己输了两局或者赢了两局
			*(me ->goon) = 'N';
			struct END end;
			memcpy(&end.head, head, sizeof(struct Header));
			strncpy(end.head.type, "END", 4);
			me ->state = 'F';
			end.end = ians;
			if(ians == 'V'){
				(me ->vin) ++;
				upgradetop10(me);
			}
			des = 1;
			send(me ->socketfd, &end, sizeof(struct END), 0);
		}else{//游戏还要继续
			struct ANS ans;
			memcpy(&ans.head, head, sizeof(struct Header));
			strncpy(ans.head.type, "ANS", 4);
			ans.ans.ans = ians;
			ans.ans.blood = me->blood;
			send(me ->socketfd, &ans, sizeof(struct ANS), 0);
		}
		pthread_cond_signal(me->cond_fgt);

	}else{//对方还未出拳,则将自己的结果保存在ans中并等待
		*(me->ans) = step.step;
		*(me->counter_fgt) = 1;
		pthread_cond_wait(me->cond_fgt, me->mutex_fgt);
		int flag = 1;
		if(*(me ->ans) == 'F')
			me ->blood --;
		else if(*(me ->ans) == 'V')
			me ->ivin ++;
		else{//超时唤醒的
			struct END end;
			memcpy(&(end.head), head, sizeof(struct Header));
			strncpy(end.head.type, "END", 4);
			end.end = 'O';//timeout
			me ->state = 'F';
			send(me ->socketfd, &end, sizeof(struct END), 0);
			flag = 0;
			des = 1;
		}
		if(flag){
			if(*(me->goon) == 'N'){
				struct END end;
				memcpy(&(end.head), head, sizeof(struct Header));
				strncpy(end.head.type, "END", 4);
				end.end = *(me->ans);
				me ->state = 'F';
				if(*(me ->ans) == 'V'){
					(me->vin) ++;
					upgradetop10(me);
				}
				des = 1;
				send(me ->socketfd, &end, sizeof(struct END), 0);
			
		
			}else{
				struct ANS ans;
				memcpy((char *)&(ans.head), (char *)head, sizeof(struct Header));
				strncpy(ans.head.type, "ANS", 4);
				ans.ans.ans = *(me ->ans);
				ans.ans.blood = me ->blood;
				send(me->socketfd, &ans, sizeof(struct ANS), 0);
			
			}
		}
	}
	
	pthread_mutex_unlock(me->mutex_fgt);
	if(des == 1){
		if(*(me ->ans) == 'A'){
			if(me->mutex_fgt != NULL)
				pthread_mutex_destroy(me->mutex_fgt);
			if(me->cond_fgt !=NULL)
				pthread_cond_destroy(me->cond_fgt);
			if(me->mutex_fgt != NULL)
				free(me->mutex_fgt);
			if(me->cond_fgt != NULL)
				free(me->cond_fgt);
			if(me->goon != NULL)
				free(me->goon);
			if(me->ans != NULL)
				free(me->ans);
		}else
			*(me->ans) = 'A';
	}
}

void error(){
	printf("error msg\n");
}


int judge(char ch1, char ch2){
	switch(ch1){
		case 'Y':
			switch(ch2){
				case 'Y':return 0;
				case 'O':return 1;
				case 'P':return -1;
				default:return 1;
			}break;
		case 'O':
			switch(ch2){
				case 'Y':return -1;
				case 'O':return 0;
				case 'P':return 1;
				default:return 1;
			}break;
		case 'P':
			switch(ch2){
				case 'Y':return 1;
				case 'O':return -1;
				case 'P':return 0;
				default:return 1;
			}break;
		default :return -1;
	}
}

void upgradetop10(struct InterAccount *me){
	pthread_mutex_lock(&mutextop10);
	if(me->vin < itop10[9].victory)//未能挤进前10
		return;//直接返回
	//说明可以进入前10.因此在前10中插入新位置
	int i = 0;
	for(i = 8; i >= 0; i -- ){
		if(itop10[i].victory <= me ->vin){//在我的位置之后
			memcpy(&(itop10[i + 1]), &(itop10[i]), sizeof(struct Top));
		}
	}
	//插入
	memcpy(itop10[i + 1].name, me ->name, 16);
	itop10[i + 1].victory = me ->vin;
	
	pthread_mutex_unlock(&mutextop10);
	return ;//插入完成直接返回
}
