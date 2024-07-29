#ifndef BINARY_SEMS_H
#define BINARY_SEMS_H

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


union semun {                   /* Used in calls to semctl() */
    int                 val;
    struct semid_ds *   buf;
    unsigned short *    array;
#if defined(__linux_)
    struct seminfo *    __buf;
#endif
};

extern bool bsUseSemUndo;        // usr SEM_UNDO
extern bool bsRetryOnEintr;      // retry if semop() interrupted by signal handler

int initSemAvailable(int semId, int semNum);
int initSemInUse(int semId, int semNum);
int reserveSem(int semId, int semNum);
int releaseSem(int semId, int semNum);


#endif
