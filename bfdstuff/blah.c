#include <stdio.h>

#ifdef __cplusplus
extern int meir;

int *falladah=&meir;
int kall=meir;
int brall;


class blarr {
public:
	blarr(int i):r(i){};
	~blarr(){printf("ciao\n");};
	int getr() {return r;}
	int method(int i);
static	int method(char i);
private:
int r;
static int rs;
};

int
blarr::method(char i)
{
return rs+i;
}


int
blarr::method(int i)
{
return r+i;
}

blarr rall(25);


int
blah(int arg)
{
	printf("hallo\n");
	return *falladah+arg+rall.getr();
}

#else
extern int falladah;
int *kall=&falladah;
int bralldef=0xded;
int brall;
int zero[23]={0};
int zero0=0;
int zero1 __attribute__((section("bss")));
int zero2 __attribute__((section(".bss")));

struct ali_ {
	int a_a __attribute__((aligned(256)));
} ali;

int
blah(int arg)
{
	printf("Hallo, this is blah; my arg is %x, falladah is %x\n",arg,falladah);
	printf("Zero is %x, %x, %x, %x\n",zero0,zero1,zero2,zero[0]);
	return falladah+arg;
}
#endif
