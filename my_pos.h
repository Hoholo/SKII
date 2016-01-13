#ifndef _MY_POS_H_
#define _MY_POS_H_
#include "flags.h"
#include<cstdlib>
#include<cstdio>
union my_pos {
	my_pos(int pos):my_pos_int(pos) {}
	int my_pos_int;
	char my_pos_char[4];
};
#endif

