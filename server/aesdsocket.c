#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
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

void log_info(const char *message) {
  syslog(LOG_INFO, "%s", message);
  printf("%s\n", message);
}

void handle_signal(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    shutdown(fd, SHUT_RDWR);
    log_info("Caught signal, exiting");
    closelog();
    remove(stringdata);
    exit(0);
  }
}

int main(int argc, char **argv) {
  openlog("aesdsocket", 0, LOG_USER);
  (void)argc;
  (void)argv;
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
    return -1;
  }

  int yes = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
    return -1;
  }

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

  freeaddrinfo(res);

  if (listen(fd, 10)) {
    return -1;
  }

  while (true) {
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
    /* printf("Accepted connection from %s\n", ipstr); */

    int wfd = open(stringdata, O_CREAT | O_WRONLY | O_APPEND,
                   S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);

    char inbuffer[1024] = {0};
    int bytesread = 0;
    while ((bytesread = recv(clientfd, inbuffer, sizeof(inbuffer), 0)) > 0) {
      ssize_t byteswritten = write(wfd, inbuffer, bytesread);
      if (byteswritten < 0) {
        return -1;
      }
      if (inbuffer[bytesread - 1] == '\n') {
        break;
      }
    }
    close(wfd);

    int rfd = open(stringdata, O_RDONLY);

    if (rfd < 0) {
      syslog(LOG_ERR, "Failed to open file for reading.");
    }

    char buffer[1024] = {0};
    ssize_t received;
    while ((received = read(rfd, buffer, sizeof(buffer))) > 0) {
      /* printf("Writing buffer back %s\n", buffer); */
      /* printf("Received bytes: %zi\n", received); */
      send(clientfd, buffer, received, 0);
    }

    close(rfd);
    close(clientfd);
    printf("Closed connection from %s\n", ipstr);
  }

  remove(stringdata);
  return 0;
}
