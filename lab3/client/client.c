#include"client.h"
#define SERV_PORT 8888

char username[16];
int sockfd;
int is_reg = 0;
int is_log = 0;
int have_mes = 0;
int is_revcha = 0;
int is_won = 0;
int is_over = 0;
char is_shell = 'a';
int all_lock = 0;
char want = 'N';
pthread_mutex_t mutex_want = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_want = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_reg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_reg = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_log = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_mes = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_mes = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_revcha = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_revcha = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_won = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_won = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_shell = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_shell = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_all_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_all_lock = PTHREAD_COND_INITIALIZER;

char itime;
int main(int argc,char** argv)
{
	/*与服务器进行连接*/
	printf("please input the ip address you want to connect");
	char ipaddr[20];
	scanf("%s",ipaddr);
	struct sockaddr_in servaddr;
	sockfd = socket(AF_INET, SOCK_STREAM,0);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ipaddr);
	servaddr.sin_port = htons(SERV_PORT);                       //这里需要设定端口,在宏定义中可以修改
	connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	printf("%s", "\033[1H\033[2J");
	printf("successful connected\n");
	/*与服务器连接部分结束*/
	
	
	/*全局变量设置区*/
	int idnum = 1;                       //用于标识每一个通讯的id数，写在每一个包的前端	
	
	
	pthread_t tid;
	pthread_create(&tid, NULL, RecvThread, NULL);
	/*注册登陆区*/
	while(1)
	{
		printf("Welcome to the battle,press '1' to register,press '2' to log in\n");
		int mode1 = 0;
		scanf("%d",&mode1);
		if(mode1 == 1)																			//用户请求注册
		{
			printf("please enter the username: ");
			char name[10];
			scanf("%s",name);
			printf("please enter the password: ");
			char password[10];
			scanf("%s",password);
			pthread_mutex_lock(&mutex_reg);
			is_reg = 0;
			registe(name,password,&idnum,sockfd);								//注册函数
			printf("xx");
			pthread_cond_wait(&cond_reg,&mutex_reg);
			if(is_reg == 1)
			{
				printf("successful registe\n");
				continue;
			}
			else
			{
				printf("registe failed\n");
				continue;
			}
			pthread_mutex_unlock(&mutex_reg);
		}
		else if(mode1 == 2)														//用户请求登陆
		{
			printf("please enter the username: ");
			char name[10];
			scanf("%s",name);
			strcpy(username,name);
			printf("please enter the password: ");
			char password[10];
			scanf("%s",password);
			pthread_mutex_lock(&mutex_log);
			is_log = 0;
			login(name,password,&idnum,sockfd);			       //登陆函数					
			pthread_cond_wait(&cond_log,&mutex_log);
			if(is_log == 1)
			{
				printf("Log in successful\n");
				break;
			}					
			else if (is_log == 2)
			{
				printf("Log in failed,username or password is wrong\n");
			}					
			else
			{
				printf("You have already loged in,please don't do this again\n");
				break;
			}	
			pthread_mutex_unlock(&mutex_log);												
		}		
	}
	while(1)
	{
		//pthread_mutex_lock(&mutex_all_lock);
		printf("please choose the act you want to behavior:\n");
		printf("1.Check the user online and their status\n");
		printf("2.Check the top\n");
		printf("3.leave message\n");
		printf("4.challange others\n");
		printf("6.Sign out\n");
		char mode2 ='a';
		while(1)
		{
			if(want != 'w'){
				pthread_mutex_lock(&mutex_want);
				setbuf(stdin, NULL);
				scanf("%c",&mode2);
				if(want == 'w'){
					printf("you wang the ans:%c", mode2);
					is_shell = mode2;
					pthread_mutex_unlock(&mutex_want);
				}
				else if(mode2 == '1'||mode2 == '2'||mode2 == '3'||mode2 == '4'||mode2 == '6'){
					pthread_mutex_unlock(&mutex_want);
					break;
				}
				else {
					printf("please input again");	
					pthread_mutex_unlock(&mutex_want);
				}
			}
		}
		printf("%c",mode2);
		if(mode2 == '1')
		{
			printf("xx4");
			pthread_mutex_lock(&mutex_mes);
			have_mes = 0;
			printf("xx3");
			CheckOn(&idnum,sockfd);
			printf("xx1");
			pthread_cond_wait(&cond_mes,&mutex_mes);
			printf("xx2");
			pthread_mutex_unlock(&mutex_mes);	
		}
		else if(mode2 == '2')
		{
			pthread_mutex_lock(&mutex_mes);
			have_mes = 0;
			CheckTop(&idnum,sockfd);
			pthread_cond_wait(&cond_mes,&mutex_mes);
			pthread_mutex_unlock(&mutex_mes);	
		}
		else if(mode2 == '3')
		{
			printf("Input the player you want to leave message:");
			char pname[16];
			scanf("%s",pname);
			char message[128];
			memset(message, 0, 128);
			printf("Input the message you want to leave:");
			//scanf("%s",message);
			setbuf(stdin, NULL);
			fgets(message, 128, stdin);
			LevMsg(username,pname,message,&idnum,sockfd);
		}
		else if(mode2 == '4')
		{
			pthread_mutex_lock(&mutex_revcha);
			is_revcha = 0;
			printf("Input the player you want to challange");
			char pname[16];
			scanf("%s",pname);
			Challange(username,pname,&idnum,sockfd);
			//pthread_mutex_lock(&mutex_want);
			pthread_cond_wait(&cond_revcha,&mutex_revcha);
			if(is_revcha == 1)
			{
				pthread_mutex_lock(&mutex_won);
				is_won = 0;
				is_over = 0;
				printf("%s has accepted your challange,game start!\n",pname);

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
					else if(itime != 'T')//未超时
						printf("please input again\n");	
					else{//超时
						printf("the act is timeout!\n");
						flag = 1;
					}		
				}
				if(flag)
					continue;
				Battle(username,tempcha,&idnum,sockfd);
				pthread_cond_wait(&cond_won,&mutex_won);
			
				pthread_mutex_unlock(&mutex_won);	
			}
			else
			{
				//pthread_mutex_unlock(&mutex_want);
				printf("%s refuse your challange",pname);
			}
			pthread_mutex_unlock(&mutex_revcha);	
		}
		else if(mode2 == '6')
		{
			SignOut(username,&idnum,sockfd);
			printf("You have successful loged out");
		}
		pthread_mutex_unlock(&mutex_all_lock);
	}	
}			
