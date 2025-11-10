#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

static int fd = 0;

void handle_signal(int signal)
{
  if (signal == SIGINT || signal == SIGTERM)
  {
    shutdown(fd, SHUT_RDWR);
    closelog();
    exit(0);
  }
}

int main(int argc, char **argv) 
{ 
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

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0)
  {
    return -1;
  }

  if (getaddrinfo(NULL, "9000", &hints, &res))
  {
    return -1;
  }
  
  fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0)
  {
    return -1;
  }

  int yes = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  
  if (bind(fd, res->ai_addr, res->ai_addrlen) < 0)
  {
    return -1;
  }

  freeaddrinfo(res);

  if (listen(fd, 10))
  {
    return -1;
  }

  while(true)
  {
    struct sockaddr_in clientaddr;
    socklen_t clientaddrsize = sizeof(clientaddr);
    int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &clientaddrsize);
    if (clientfd < 0)
    {
      return -1;
    }

    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(clientaddr.sin_family, &clientaddr.sin_addr, ipstr, INET_ADDRSTRLEN);
    syslog(LOG_INFO, "Accepted connection from %s", ipstr);
    printf("Accepted connection from %s\n", ipstr);

    char buffer[1024] = {0};
    int received = recv(clientfd, buffer, sizeof(buffer) - 1, 0);
    if (received > 0)
    {
      send(clientfd, buffer, received, 0);
    }
    close(clientfd);
  }

  return 0; 
}

