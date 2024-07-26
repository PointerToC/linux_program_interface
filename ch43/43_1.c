#include <unistd.h>
#include <errno.h>
#include <stdlib.h>


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
    memset(send_data[i], '1', data_len[i]);
  }

  int pipe_fd[2];
  if (pipe(pipe_fd) == -1) {
    perror("pipe failed!");
    return EXIT_FAILURE;
  }
  
  struct timespec start_ts;

  switch (fork()) {
    case -1 :
      perror("fork failed!);
      return -1;
      break;
    case 0 :
      if (close(pipe_fd[1] == -1) {
          perror("close write pipe failed!");
          return -1;
      }
      for (int i = 0; i < data_num; ++i) {
        int cnt = 0;
        int read_cnt = 0;
        while () {
          
        }
      }
      break;
    default :
      if (close(pipe_fd[0] == -1) {
          perror("close read pipe failed!");
          return -1;
      }

      for (int i = 0; i < data_num; ++i) {
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
          perror("get clock time failed!");
        }
        int start_ = 0;
        int w_cnt = 0;
        while ((w_cnt = write(pipe_fd[1], &send_data[i][start], data_len[i] - start)) != 0) {
          start += w_cnt;
        }
      }

      break;
  }
  return EXIT_SUCCESS;
}
