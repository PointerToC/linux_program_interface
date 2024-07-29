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

int main(int arg, char *argv[]) {
  int semid;

  if (arg == 2) {
    union semun arg;

    semid = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (semid == -1) {
      perror("semid");
      exit(EXIT_FAILURE);
    }

    arg.val = atoi(argv[1]);
    if (semctl(semid, 0, SETVAL, arg) == -1) {
      perror("semid");
      exit(EXIT_FAILURE);
    }

    printf("Semphore ID = %d\n", semid);
  } else {
    struct sembuf sop;

    semid = atoi(argv[1]);
    sop.sem_num = 0;
    sop.sem_op = atoi(argv[2]);
    sop.sem_flg = 0;

    printf("%ld: about to semop\n", (long)getpid());
    if (semop(semid, &sop, 1) == -1) {
      perror("semop");
      exit(EXIT_FAILURE);
    }
    printf("%ld: semop complete!\n", (long)getpid());
  }
  
  return 0;
}
