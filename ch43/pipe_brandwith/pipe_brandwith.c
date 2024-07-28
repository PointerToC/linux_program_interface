#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "binary_sems.h"


// to test brandwith of pipe

int main(int arg, char *argv[]) {
  if (arg <= 1) {
    fprintf(stderr, "Usage: %s <number>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int data_num = argv[1];
  int data_len[data_num];

  for (int i = 1; i <= data_num; ++i) {
    data_len[i - 1] = atoi(argv[i + 1]);
  }

  char *send_data[data_num];
  
  for (int i = 0; i < data_num; ++i) {
    send_data[i] = (char *)malloc(sizeof(char) * data_len[i]);
    memset(send_data[i], '1', data_len[i] - 1);
    send_data[data_len[i] - 1] = '\n';
  }

  int pipe_fd[2];
  if (pipe(pipe_fd) == -1) {
    perror("pipe failed!");
    return EXIT_FAILURE;
  }
  
  struct timespec start_ts;

  // get sem id
  int sem_id;
  int sem_num = 1;
  semid = semget(IPC_PRIVATE, sem_num, S_IRUSR | S_IWUSR);
  if (semid == -1) {
    perror("semget failed!");
    return -1;
  }
  
  if (initSemZero(sem_id, sum_num) == -1){
    perror("initSemZero failed!");
    return -1;
  }
  
  switch (fork()) {
    case -1 :
      perror("fork failed!);
      return -1;
      break;
    case 0 : // child process
      if (close(pipe_fd[1] == -1) {
          perror("close write pipe failed!");
          return -1;
      }

      long long recv_cap = 1000000000;
      char &recv_data = (char*)malloc(sizeof(char) * recv_cap);
      for (int i = 0; i < data_num; ++i) {
        if (reserveSem(sem_id, 0) == -1) {
          perror("reserveSem failed in child process");
          return -1;
        }

        int cnt = 0;
        int read_cnt = 0;
        while ((cnt = read(pipe_fd[0], recv_data, recv_cap) > 0) {
            read_cnt += cnt;
        }
        if (cnt == 0) {
          
        }

        //calculate time
        printf("read_cnt = %ld, total_cnt = %ld\n", read_cnt, data_len[i]);
      }
      break;
    default :  // parent process
      if (close(pipe_fd[0] == -1) {
          perror("close read pipe failed!");
          return -1;
      }

      for (int i = 0; i < data_num; ++i) {
        if (releaseSem(sem_id, 0) == -1) {
          perror("releaseSem failed in parent process!");
          return -1;
        }

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
          perror("get clock time failed!");
          return -1;
        }

        int start_ = 0;
        int w_cnt = 0;
        while ((w_cnt = write(pipe_fd[1], &send_data[i][start], data_len[i] - start)) != 0) {
          start += w_cnt;
        }

        printf("read_cnt = %ld, total_cnt = %ld\n", read_cnt, data_len[i]);
      }

      break;
  }
  return EXIT_SUCCESS;
}
