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
private:
int r;
};

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

int
blah(int arg)
{
		return falladah+arg;
}
#endif
