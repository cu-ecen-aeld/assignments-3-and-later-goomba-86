#include "queue.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

static int fd = 0;
static char const *const stringdata = "/var/tmp/aesdsocketdata";
static bool sigterm_received = false;
static pthread_t time_logger_thread;
static struct listhead head;

typedef struct thread_data_t {
  int clientfd;
  bool thread_completed;
  pthread_t threadid;
} thread_data_t;

typedef struct entry_t {
  thread_data_t *data;
  SLIST_ENTRY(entry_t) entries;
} entry_t;

SLIST_HEAD(listhead, entry_t);

pthread_mutex_t lock;

void write_file_back_to_client(int clientfd);
void write_bytes_to_file(int clientfd);
void *handle_thread(void *arg);
void *time_logger(void *arg);

void log_info(const char *message) {
  syslog(LOG_INFO, "%s", message);
  printf("%s\n", message);
}

void handle_signal(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    log_info("Caught signal, exiting");
    sigterm_received = true;

    while (!SLIST_EMPTY(&head)) {
      entry_t *iter, *tmp;
      SLIST_FOREACH_SAFE(iter, &head, entries, tmp) {
        if (iter->data->thread_completed) {
          SLIST_REMOVE(&head, iter, entry_t, entries);
          pthread_join(iter->data->threadid, NULL);
          close(iter->data->clientfd);
          free(iter->data);
          free(iter);
        }
      }
    }

    pthread_join(time_logger_thread, NULL);
    shutdown(fd, SHUT_RDWR);
    closelog();
    remove(stringdata);
  }
}

int main(int argc, char **argv) {
  openlog("aesdsocket", 0, LOG_USER);
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct sigaction sa;
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) {
    return -1;
  }

  if (getaddrinfo(NULL, "9000", &hints, &res)) {
    return -1;
  }

  fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    freeaddrinfo(res);
    return -1;
  }

  int yes = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
    freeaddrinfo(res);
    return -1;
  }

  freeaddrinfo(res);

  if (argc > 1 && strcmp(argv[1], "-d") == 0) {
    pid_t pid = fork();

    if (pid < 0) {
      return -1;
    } else if (pid == 0) // Child process
    {
    } else // Parent process
    {
      return 0;
    }
  }

  if (listen(fd, 10)) {
    return -1;
  }

  SLIST_INIT(&head);

  // Create a timer thread
  pthread_create(&time_logger_thread, NULL, time_logger, NULL);

  while (!sigterm_received) {
    struct sockaddr_in clientaddr;
    socklen_t clientaddrsize = sizeof(clientaddr);
    int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &clientaddrsize);
    if (clientfd < 0) {
      return -1;
    }

    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(clientaddr.sin_family, &clientaddr.sin_addr, ipstr,
              INET_ADDRSTRLEN);
    syslog(LOG_INFO, "Accepted connection from %s", ipstr);

    // 1. Create thread
    thread_data_t *new_data = malloc(sizeof(thread_data_t));
    new_data->clientfd = clientfd;
    new_data->thread_completed = false;
    if (pthread_create(&new_data->threadid, NULL, handle_thread,
                       (void *)new_data) != 0) {
      syslog(LOG_ERR, "Thread creation failed.");
      exit(1);
    }

    // 2. Store thread to a linked list
    entry_t *entry = malloc(sizeof(entry_t));
    entry->data = new_data;
    SLIST_INSERT_HEAD(&head, entry, entries);

    // 3. Remove completed threads
    entry_t *iter, *tmp;
    SLIST_FOREACH_SAFE(iter, &head, entries, tmp) {
      if (iter->data->thread_completed) {
        SLIST_REMOVE(&head, iter, entry_t, entries);
        pthread_join(iter->data->threadid, NULL);
        close(iter->data->clientfd);
        free(iter->data);
        free(iter);
      }
    }

    /* printf("Closed connection from %s\n", ipstr); */
  }

  return 0;
}

void *handle_thread(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;
  write_bytes_to_file(data->clientfd);
  write_file_back_to_client(data->clientfd);
  data->thread_completed = true;
  return NULL;
}

void write_bytes_to_file(int clientfd) {
  pthread_mutex_lock(&lock);
  int wfd = open(stringdata, O_CREAT | O_WRONLY | O_APPEND,
                 S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);

  char inbuffer[1024] = {0};
  int bytesread = 0;
  while ((bytesread = recv(clientfd, inbuffer, sizeof(inbuffer), 0)) > 0) {
    ssize_t byteswritten = write(wfd, inbuffer, bytesread);
    if (byteswritten < 0) {
      return;
    }
    if (inbuffer[bytesread - 1] == '\n') {
      break;
    }
  }
  close(wfd);
  pthread_mutex_unlock(&lock);
}

void write_file_back_to_client(int clientfd) {
  pthread_mutex_lock(&lock);
  int rfd = open(stringdata, O_RDONLY);

  if (rfd < 0) {
    syslog(LOG_ERR, "Failed to open file for reading.");
  }

  char buffer[1024] = {0};
  ssize_t received;
  while ((received = read(rfd, buffer, sizeof(buffer))) > 0) {
    send(clientfd, buffer, received, 0);
  }
  close(rfd);
  pthread_mutex_unlock(&lock);
}

void *time_logger(void *arg) {
  (void)arg;
  time_t interval = time(NULL);
  while (!sigterm_received) {
    time_t current_time = time(NULL);
    if (current_time - interval > 10) {
      pthread_mutex_lock(&lock);
      struct tm tm_struct;
      localtime_r(&current_time, &tm_struct);
      char buf[64] = {};
      int len = strftime(buf, sizeof(buf), "timestamp:%a, %d %b %Y %T %z\n",
                         &tm_struct);

      int wfd = open(stringdata, O_CREAT | O_WRONLY | O_APPEND,
                     S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
      int ret = write(wfd, buf, len);
      (void)ret;
      close(wfd);
      interval = current_time;
      pthread_mutex_unlock(&lock);
    }
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 50000000;
    nanosleep(&ts, NULL);
  }
  return NULL;
}
