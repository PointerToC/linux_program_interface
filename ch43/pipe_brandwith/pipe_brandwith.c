/** to test brandwith of pipe **/

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
#include <assert.h>


void free_data(char *data[], int data_num);
double calculate_brandwith(long long data_len, struct timespec *start_time, struct timespec *end_time);

int main(int arg, char *argv[]) {
  if (arg <= 1) {
    fprintf(stderr, "Usage: %s <number>\n", argv[0]);
    return 1;
  }

  // recv buff
  long long recv_cap = 10000000;
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

  // semaphore
  const char *sem_name_endwrite = "/sem_endwrite";
  const char *sem_name_endread = "/sem_endread";

  sem_t *sem_ptr_endwrite = NULL;
  sem_t *sem_ptr_endread = NULL;

  if ((sem_ptr_endwrite = sem_open(sem_name_endwrite, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("sem_end_write, sem_open");
    return 1;
  }

  if ((sem_ptr_endread = sem_open(sem_name_endread, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("sem_endread,sem_open");
    return 1;
  }

  if (sem_unlink(sem_name_endwrite) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("sem_unlink");
    return 1;
  }

  if (sem_unlink(sem_name_endread) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("sem_unlink");
    return 1;
  }

  // for time
  struct timespec ts;

  // use ANONYMOUS, MAP_SHARED shm
  size_t shm_size = sizeof(struct timespec);
  void *shm_addr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shm_addr == MAP_FAILED) {
    free(recv_buff);
    free_data(send_data, data_num);
    perror("mmap");
  }

  // pipe
  int pipe_fd[2];
  if (pipe(pipe_fd) == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    munmap(shm_addr, shm_size);
    perror("pipe failed!");
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    free(recv_buff);
    free_data(send_data, data_num);
    munmap(shm_addr, shm_size);
    perror("fork failed!");
    return 1;
  } else if (pid == 0) {      // child process
    if (close(pipe_fd[1]) == -1) {
        free(recv_buff);
        free_data(send_data, data_num);
        munmap(shm_addr, shm_size);
        perror("close write pipe failed!");
        return 1;
    }

    struct timespec end_read_ts;
    for (int i = 0; i < data_num; ++i) {
      sem_post(sem_ptr_endread);

      int read_cnt = 0;
      read_cnt = read(pipe_fd[0], recv_buff, recv_cap);

      if (read_cnt == -1) {
        perror("read failed!");
        return 1;
      }

      if (clock_gettime(CLOCK_REALTIME, &end_read_ts) == -1) {
        perror("get clock time failed!");
        return 1;
      }

      sem_wait(sem_ptr_endwrite);

      memcpy((void *)&ts, shm_addr, sizeof(struct timespec));

      // calculate pipe brandwith
      double brandwith = calculate_brandwith(data_len[i], &ts, &end_read_ts);
      // calculate time
      printf("start time : seconds = %ld, nanoseconds = %ld\n", ts.tv_sec, ts.tv_nsec);
      printf("end read time : seconds = %ld, nanoseconds = %ld\n", end_read_ts.tv_sec, end_read_ts.tv_nsec);
      printf("data_len = %d Bytes, brandwith = %.2f MB/s\n", data_len[i], brandwith);
      printf("---------------------------------------------\n");
    }

    free_data(send_data, data_num);
    free(recv_buff);
    if (munmap(shm_addr, shm_size) == -1) {
      perror("munmap in child process");
      return 1;
    }

    if (close(pipe_fd[0]) == -1) {
      perror("close pipe_fd[0]");
      return 1;
    }

    return 0;
  } else {      // parent process
    if (close(pipe_fd[0]) == -1) {
      free(recv_buff);
      free_data(send_data, data_num);
      munmap(shm_addr, shm_size);
      perror("close read pipe failed!");
      return 1;
    }

    for (int i = 0; i < data_num; ++i) {
      sem_wait(sem_ptr_endread);

      if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        free(recv_buff);
        free_data(send_data, data_num);
        munmap(shm_addr, shm_size);
        perror("get clock time failed!");
        return 1;
      }

      int write_sum = 0;
      write_sum = write(pipe_fd[1], (void *)send_data[i], data_len[i]);
      
      if (write_sum == -1) {
        free(recv_buff);
        free_data(send_data, data_num);
        munmap(shm_addr, shm_size);
        perror("write failed!");
        return 1;
      }

      memcpy((void *)shm_addr, (void *)&ts, sizeof(struct timespec));

      sem_post(sem_ptr_endwrite);
    }

    int status;
    waitpid(pid, &status, 0);
    if (status != 0) {
      free(recv_buff);
      free_data(send_data, data_num);
      munmap(shm_addr, shm_size);
      printf("child process exit status: %d\n", status);
      return 1;
    }

    free_data(send_data, data_num);
    free(recv_buff);

    if (munmap(shm_addr, shm_size) == -1) {
      perror("munmap failed in parent process");
      return -1;
    }

    if (close(pipe_fd[1]) == -1) {
      perror("close pipe_fd[1]");
      return 1;
    }
  }
  return 0;
}

void free_data(char *data[], int data_num) {
  for (int i = 0; i < data_num; ++i) {
    free(data[i]);
  }
}

double calculate_brandwith(long long data_len, struct timespec *start_time, struct timespec *end_time) {
  double res = 0.0;
  long diff_sec = end_time->tv_sec - start_time->tv_sec;
  long diff_nsec = end_time->tv_nsec - start_time->tv_nsec;
  double total = diff_sec + (double)diff_nsec / 1000000000;
  res = (data_len / total);
  res /= (1024 * 1024);
  return res;
}
