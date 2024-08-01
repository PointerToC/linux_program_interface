#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

// #include "binary_sems.h"


// to test brandwith of pipe
void free_data(char *data[], int data_num);

int main(int arg, char *argv[]) {
  if (arg <= 1) {
    fprintf(stderr, "Usage: %s <number>\n", argv[0]);
    return 1;
  }

  // recv buff
  long long recv_cap = 1000000000;
  char *recv_buff = (char*)malloc(sizeof(char) * recv_cap);

  // generate test data
  int data_num = atoi(argv[1]);
  int data_len[data_num];

  for (int i = 1; i <= data_num; ++i) {
    data_len[i - 1] = atoi(argv[i + 1]);
  }

  char *send_data[data_num];
  
  for (int i = 0; i < data_num; ++i) {
    send_data[i] = (char *)malloc(sizeof(char) * data_len[i]);
    memset(send_data[i], '1', data_len[i]);
  }

  printf("send_data[0] = %ld\n", strlen(send_data[0]));
  printf("send_data[1] = %ld\n", strlen(send_data[1]));

  // semaphore
  const char *sem_name_endwrite = "/sem_endwrite";
  const char *sem_name_endread = "/sem_endread";
  sem_unlink(sem_name_endwrite);
  sem_unlink(sem_name_endread);

  sem_t *sem_ptr_endwrite = NULL;
  sem_t *sem_ptr_endread = NULL;

  if ((sem_ptr_endwrite = sem_open(sem_name_endwrite, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("sem_end_write, sem_open");
    return -1;
  }

  if ((sem_ptr_endread = sem_open(sem_name_endread, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("sem_endread,sem_open");
    return -1;
  }

  if (sem_unlink(sem_name_endwrite) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("sem_unlink");
    return -1;
  }

  if (sem_unlink(sem_name_endread) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("sem_unlink");
    return -1;
  }

  // for time
  struct timespec ts;

  // for shared mem
  const char *shm_name = "/time_share";
  int shm_fd;
  int sh_size = sizeof(struct timespec);
  void *shm_addr = NULL;

  if ((shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR)) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("shm_open");
    return -1;
  }

  if (ftruncate(shm_fd, sh_size) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    close(shm_fd);
    perror("ftruncate");
    return -1;
  }

  if ((shm_addr = mmap(NULL, sh_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
    free(recv_buff);
    free_data(send_data, data_num);
    close(shm_fd);
    perror("mmap");
    return -1;
  }

  if (close(shm_fd) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    shm_unlink(shm_name);
    perror("close shm_fd");
    return -1;
  }

  // pipe
  int pipe_fd[2];
  if (pipe(pipe_fd) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    shm_unlink(shm_name);
    perror("pipe failed!");
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    shm_unlink(shm_name);
    perror("fork failed!");
    return -1;
  } else if (pid == 0) {      // child process
    if (close(pipe_fd[1]) == -1) {
        free(recv_buff);
        free_data(send_data, data_num);
        shm_unlink(shm_name);
        perror("close write pipe failed!");
        return -1;
    }

    struct timespec end_read_ts;
    for (int i = 0; i < data_num; ++i) {
      int read_cnt = 0;
      read_cnt = read(pipe_fd[0], recv_buff, recv_cap);
      printf("child proc, read_cnt = %d\n", read_cnt);

      if (read_cnt == -1) {
        perror("read failed!");
        return -1;
      }

      if (clock_gettime(CLOCK_REALTIME, &end_read_ts) == -1) {
        perror("get clock time failed!");
        return -1;
      }

      printf("child proc wait for sem_endwrite\n");
      sem_wait(sem_ptr_endwrite);
      printf("child proc get sem_endwrite\n");

      memcpy((void *)&ts, shm_addr, sizeof(struct timespec));
      //calculate time
      printf("read_cnt = %d, total_cnt = %d\n", read_cnt, data_len[i]);
      printf("start time : seconds = %ld, nanoseconds = %ld\n", ts.tv_sec, ts.tv_nsec);
      printf("end read time : seconds = %ld, nanoseconds = %ld\n", end_read_ts.tv_sec, end_read_ts.tv_nsec);

      sem_post(sem_ptr_endread);
      printf("child proc post sem_endread\n");
    }

    return 0;
  } else {      // parent process
    if (close(pipe_fd[0]) == -1) {
        free(recv_buff);
        free_data(send_data, data_num);
        shm_unlink(shm_name);
        perror("close read pipe failed!");
        return -1;
    }

    for (int i = 0; i < data_num; ++i) {
      if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("get clock time failed!");
        return -1;
      }

      int write_sum = 0;
      write_sum = write(pipe_fd[1], (void *)send_data[i], data_len[i]);
      printf("parent proc, write_sum = %d\n", write_sum);
      
      if (write_sum == -1) {
        perror("write failed!");
        return -1;
      }

      memcpy((void *)shm_addr, (void *)&ts, sizeof(struct timespec));

      sem_post(sem_ptr_endwrite);
      printf("parent proc write complete, post sem_end_write\n");
      printf("parent proc write_sum = %d, total_cnt = %d\n", write_sum, data_len[i]);

      printf("parent proc wait for sem_endread\n");
      sem_wait(sem_ptr_endread);
      printf("parent proc get sem_endread\n");
    }
    free_data(send_data, data_num);
    free(recv_buff);

    int status;
    waitpid(pid, &status, 0);
    if (status != 0) {
      printf("child process exit status: %d\n", status);
      return -1;
    }
  }
  return 0;
}

void free_data(char *data[], int data_num) {
  for (int i = 0; i < data_num; ++i) {
    free(data[i]);
  }
}
