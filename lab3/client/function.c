#include"client.h"
int readn( int fd, char *bp, size_t len)
{
	int cnt;
	int rc;
	cnt = len;
	while ( cnt > 0 ){
		rc = recv( fd, bp, cnt, 0 );
		if ( rc < 0 ){				/* read error? */
			return -1;				/* return error */
		}
		if ( rc == 0 )				/* EOF? */
			return len - cnt;		/* return short count */
		bp += rc;
		cnt -= rc;
	}
//	printf("readn:%d",len);
	return len;
}

void* RecvThread(void* point)
{
	struct Header head;
	while(1)
	{
		memset(&head, 0, sizeof(head));
		int c = readn(sockfd,&head,sizeof(head));
		if(c != sizeof(head)){
			continue;
		}else if(strcmp(head.type,"RSP")==0)
		{
			char ans;
			readn(sockfd, &ans, 1);
			pthread_mutex_lock(&mutex_reg);
			if(ans == 'S')
			{
				is_reg = 1;
			}
			else
			{
				is_reg = 0;
			}
			pthread_cond_signal(&cond_reg);
			pthread_mutex_unlock(&mutex_reg);			
		}
		else if(strcmp(head.type,"RIN")==0)
		{
			char ans;
			readn(sockfd,&ans,1);
			pthread_mutex_lock(&mutex_log);
			if(ans == 'S')
			{
				is_log = 1;
			}
			else if(ans == 'F')
			{
				is_log = 2;
			}
			else
			{
				is_log = 3;
			}
			pthread_cond_signal(&cond_log);
			pthread_mutex_unlock(&mutex_log);	
		}
		else if(strcmp(head.type,"GET")==0)
		{
			int i0 = 0;
			int length;
			readn(sockfd,&length,4);
			pthread_mutex_lock(&mutex_mes);
			for(;i0 < length;i0++)
			{
				struct States states;
				readn(sockfd,&states,17);
				char tempstates[10];
				if(states.state == 'F')
				{
					strcpy(tempstates,"Free");
				}
				else
			   {
			   	strcpy(tempstates,"Battleing");
			   }
				printf("name:%s   status:%s\n",states.name,tempstates);
				fflush(stdout);
			}
			have_mes = 1;
			pthread_cond_signal(&cond_mes);
			pthread_mutex_unlock(&mutex_mes);
		}
		else if(strcmp(head.type,"MSG")==0)
		{
			char message[128];
			char sender[16];
			struct Message mes;
			readn(sockfd,&mes,sizeof(mes));
			strcpy(message,mes.msg);
			strcpy(sender,mes.sender);
			printf("you have received a message from %s:%s.\n",sender,message);
		}
		else if(strcmp(head.type,"TOP")==0)
		{
			printf("***************************TOP10**************************************\n");
			pthread_mutex_lock(&mutex_mes);
			int i1 = 0;
			for(;i1 < 10;i1++)
			{
				struct Top top;
				readn(sockfd,&top,sizeof(top));
				printf("name:%s victory:%d\n",top.name,top.victory);
			}
			have_mes = 1;
			pthread_cond_signal(&cond_mes);
			fflush(stdout);
			pthread_mutex_unlock(&mutex_mes);
		}
		else if(strcmp(head.type,"RCH")==0)
		{
			pthread_mutex_lock(&mutex_revcha);
			struct Challenge cha;
			readn(sockfd,&cha,sizeof(cha));
			if(cha.state == 'A')
			{
				printf("%s has accepted your challange",cha.recipient);
				is_revcha = 1;
			}
			else
			{
				printf("%s has refused your challange",cha.recipient);
				is_revcha = 0;
			}
			fflush(stdout);
			pthread_cond_signal(&cond_revcha);
			pthread_mutex_unlock(&mutex_revcha);		
		}
		else if(strcmp(head.type,"ANS")==0)
		{
			//if(want != 'w')
				//pthread_mutex_lock(&mutex_won);
			struct Answer ans;
			readn(sockfd,&ans,sizeof(ans));
			printf("readed the ans\n");
			if(ans.ans == 'V')
			{
				printf("you won!\n");
				printf("your blood:%d\n",ans.blood);
			}else if(ans.ans == 'F')
			{
				printf("you lose\n");
				printf("your blood:%d\n",ans.blood);
			}else{
				printf("Draw\n");
				printf("your blood:%d\n",ans.blood);
			}
			
			printf("Choose the act you want to perform\n");
			printf("Y.剪刀\n");
			printf("O.布\n");
			printf("p.锤子\n");
			char tempcha;
			int flag = 0;
			itime = 'N';
			while(1)
			{
				setbuf(stdin, NULL);
				scanf("%c",&tempcha);
				if(tempcha=='Y'||tempcha=='O'||tempcha=='P'){
					if(itime != 'T')
						itime = 'Y';
					break;
				}
				else if(itime != 'T')
					printf("please input again:");
				else{
					printf("the act is timeout!\n");
					flag = 1;
				}
			}
			fflush(stdout);
			if(flag == 0)
				Battle(username,tempcha,&(head.id),sockfd);
			
			
		}
		else if(strcmp(head.type,"END") == 0)
		{
			char end;
			readn(sockfd,&end,1);
			if(end == 'T'){//第一类超时,挑战超时，也就自己长时间未选择接受或者拒绝
				want = 'N';
				printf("you long time on select, time out!\n");
				printf("please choose the act you want to behavior:\n");
				printf("1.Check the user online and their status\n");
				printf("2.Check the top\n");
				printf("3.leave message\n");
				printf("4.challange others\n");
				printf("6.Sign out\n");
				fflush(stdout);
			}else if(end == 'O'){//第二类超时,对战中一方长时间未出拳
				if(itime == 'Y'){//自己出国拳，受到超时说明对面没有出拳。
					if(want != 'w')//正常的对战结束包
						pthread_mutex_lock(&mutex_won);
			
					printf("sorry , the other player didn't act!\nthe game is over! noone win!\n");
					fflush(stdout);
					is_over = 1;
					if(want == 'w'){
						want = 'N';
					}else{
						pthread_cond_signal(&cond_won);
						pthread_mutex_unlock(&mutex_won);
					}
				}else{//自己没有出国拳
					itime = 'T';//time out
				}
			}else{	
				if(want != 'w')//正常的对战结束包
					pthread_mutex_lock(&mutex_won);
			
				if(end == 'V')
				{
					printf("you are the winner\n");
				}
				else 
				printf("Sorry you lose this game\n");
				fflush(stdout);
				is_over = 1;
				if(want == 'w'){
					want = 'N';
				}else{
					pthread_cond_signal(&cond_won);
					pthread_mutex_unlock(&mutex_won);
				}
			}
		}
		else if(strcmp(head.type,"CHL")==0)
		{
			
			//pthread_mutex_lock(&mutex_shell);
			
			struct Challenge Cha;
			readn(sockfd,&Cha,sizeof(Cha));			
			struct RCH Rch;
			memset(&Rch, 0, sizeof(Rch));
			if(want != 'w'){
				want = 'w';
				//is_shell = 'X';
			}
			else{//目前正在被挑战中,或者正在游戏中, 直接拒绝
				Rch.head.id = head.id;
				strcpy(Rch.head.type,"RCH");
				memcpy(&Rch.chl, &Cha, sizeof(Cha));
				Rch.chl.state = 'R';
				send(sockfd,&Rch,sizeof(Rch),0);
				continue;
			}
			while(1)
			{			
				printf("%s has challanged you,will you accept?\n",Cha.initiator);
				printf("A to accept R to refuse");
				fflush(stdout);
				pthread_mutex_lock(&mutex_want);
				
				//pthread_cond_wait(&cond_shell,&mutex_shell);
			
				printf("funtion：is_shell is %c",is_shell);
				if(is_shell == 'A')
				{
					Rch.head.id = head.id;
					strcpy(Rch.head.type,"RCH");
					memcpy(&Rch.chl, &Cha, sizeof(Cha));
					Rch.chl.state = 'A';
					send(sockfd,&Rch,sizeof(Rch),0);
					is_won = 0;
					is_over = 0;
					printf("Choose the act you want to perform\n");
					printf("Y.剪刀\n");
					printf("O.布\n");
					printf("p.锤子\n");
					char tempcha;
					int flag = 0;
					itime = 'N';
					while(1)
					{
						setbuf(stdin, NULL);
						
						scanf("%c",&tempcha);
						if(tempcha=='Y'||tempcha=='O'||tempcha=='P'){
							if(itime != 'T')
								itime = 'Y';
							break;
						}
						else if(itime != 'T')
							printf("please input again");
						else {
							printf("the act is timeout!\n");
							flag = 1;
						}
					
					}
					pthread_mutex_unlock(&mutex_want);
					if(flag){
						printf("the act is timeout!\n");
						break;
					}
							
					printf("after accept you act as: %c", tempcha);
					fflush(stdout);
	
					Battle(username,tempcha,&(head.id),sockfd);
					break;
				}
				else if(is_shell == 'R')
				{
					Rch.head.id = head.id;
					strcpy(Rch.head.type,"RCH");
					memcpy(&Rch.chl, &Cha, sizeof(Cha));
					Rch.chl.state = 'R';
					want = 'N';
					//pthread_cond_signal(&cond_want);
					pthread_mutex_unlock(&mutex_want);
					send(sockfd,&Rch,sizeof(Rch),0);
					break;
				}
				else 
				{
					printf("wrong input%c\n", is_shell);
					printf("please input again");
					fflush(stdout);
					setbuf(stdin, NULL);
					scanf("%c", &is_shell);
					//pthread_cond_signal(&cond_want);
					//pthread_cond_wait(&cond_want, &mutex_want);
				}
			}
			
			
			//pthread_mutex_unlock(&mutex_shell);
		}
		else if(strcmp(head.type,"UPD")==0)
		{
			struct States t;
			readn(sockfd, &t, sizeof(struct States));
			
			printf("name:%s states:%c\n", t.name, t.state);
			fflush(stdout);
				
		}
		else printf("one loop\n");
		fflush(stdout);
	}
	return NULL;
}

void registe(char*username1,char*password,int *id,int sockfd)
{
	struct SGP Sgp;
	Sgp.head.id = *id;
	strcpy(Sgp.head.type,"SGP");
	strcpy(Sgp.account.name,username1);
	strcpy(Sgp.account.passwd,password);
	send(sockfd,&Sgp,sizeof(Sgp),0);
	id++;
}

void login(char*username1,char*password,int *id,int sockfd)
{
	struct SIN Sin;
	Sin.head.id = *id;
	strcpy(Sin.head.type,"SIN");
	strcpy(Sin.account.name,username1);
	strcpy(Sin.account.passwd,password);
	send(sockfd,&Sin,sizeof(Sin),0);
	id++;
}

void CheckOn(int *id,int sockfd)
{
	struct Header header;
	header.id = *id;
	strcpy(header.type,"GET");
	send(sockfd,&header,sizeof(header),0);
	id++;
}

void CheckTop(int *id,int sockfd)
{
	struct Header header;
	header.id = *id;
	strcpy(header.type,"TOP");
	send(sockfd,&header,sizeof(header),0);
	id++;
}

void SignOut(char*username1,int *id,int sockfd)
{
	struct SOT Sot;
	Sot.head.id = *id;
	strcpy(Sot.head.type,"SOT");
	strcpy(Sot.name,username1);
	send(sockfd,&Sot,sizeof(Sot),0);
	id++;
}

void LevMsg(char*username1,char*pname,char*msg,int *id,int sockfd)
{
	struct MSG message;
	message.head.id = *id;
	strcpy(message.head.type,"MSG");
	strcpy(message.msg.sender,username1);
	strcpy(message.msg.reciver,pname);
	strcpy(message.msg.msg,msg);
	send(sockfd,&message,sizeof(message),0);
	id++;
}

void Challange(char*username1,char*pname,int *id,int sockfd)
{
	struct CHL Chl;
	Chl.head.id = *id;
	strcpy(Chl.head.type,"CHL");
	strcpy(Chl.chl.initiator,username1);
	strcpy(Chl.chl.recipient,pname);
	send(sockfd,&Chl,sizeof(Chl),0);
	id++;
}

void Battle(char*username1,char act,int *id,int sockfd)
{
	struct FGT Fgt;
	Fgt.head.id = *id;
	strcpy(Fgt.head.type,"FGT");
	strcpy(Fgt.step.name,username1);
	Fgt.step.step = act;
	printf("act is :%c\n",act);
	send(sockfd,&Fgt,sizeof(Fgt),0);
	id++;
}
















