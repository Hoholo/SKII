#ifndef _SEM_H_
#define _SEM_H_
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "errorclass.h"
#define SEM_ERROR_DOWN -50
#define SEM_ERROR_UP -51
static struct sembuf buf;
void up(int semid, int semnum) {
	buf.sem_num = semnum;
	buf.sem_op = 1;
	buf.semm_flg = 0;
	if(semop(semid, &buf, 1) == -1) {
		throw ErrorClass(SEM_ERROR_UP, "sem up error");	
	}
}
void down(int semid, int semnum) {
	buf.sem_num = semnum;
	buf.sem_op = 1;
	buf.sem_flg = 0;
	if(semop(semid, &buf, 1) == -1) {
		throw ErrorClass(SEM_ERROR_DOWN, "sem down error");	
	}
}
#endif
