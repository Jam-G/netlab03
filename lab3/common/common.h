#ifndef COMMON_H
#define COMMON_H
#pragma pack(1)
struct Header{
	int id;//the message id
	char type[4];//the message type
};
struct Account{
	char name[16];//user name
	char passwd[16];//user password
};
struct States{
	char name[16];
	char state;
};
struct Challenge{
	char initiator[16];
	char recipient[16];
	char state;//accept or refuse
};
struct Step{
	char name[16];
	char step;// 'Y' as Scissors 'O' as paper 'P' as stone
};
struct Answer{
	char ans;
	int blood;
};

struct Message{
	char sender[16];
	char reciver[16];
	char msg[128];
};
struct Top{
	char name[16];
	int victory;
};
struct SGP{
	struct Header head;
	struct Account account;
};
struct RSP{
	struct Header head;
	char ans;
};
struct SIN{
	struct Header head;
	struct Account account;
};
struct RIN{
	struct Header head;
	char info;
};
struct SOT{
	struct Header head;
	char name[16];
};

struct CHL{
	struct Header head;
	struct Challenge chl;
};
struct RCH{
	struct Header head;
	struct Challenge chl;
};
struct FGT{
	struct Header head;
	struct Step step;
};

struct ANS{
	struct Header head;
	struct Answer ans;
};

struct END{
	struct Header head;
	char end;
};

struct MSG{
	struct Header head;
	struct Message msg;
};

struct TOP{
	struct Header head;
	struct Top top10[10];
};
struct UPD{
	struct Header head;
	struct States state;
};
#pragma pack()
#endif
