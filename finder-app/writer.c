#include <fcntl.h>
#include <string.h>
#include <sys/syslog.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char **argv) {
  openlog(NULL, LOG_PID, LOG_USER);

  if (argc < 3) {
    syslog(LOG_ERR, "Invalid arguments.");
    closelog();
    return 1;
  }

  char *writefile = argv[1];
  char *writestr = argv[2];

  int fd = open(writefile, O_CREAT | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH);

  if (fd < 0) {
    syslog(LOG_ERR, "File could not be opened");
    closelog();
    return 1;
  }

  size_t writestrLength = strlen(writestr);

  syslog(LOG_DEBUG, "Writing %s to file %s", writestr, writefile);
  ssize_t bytesWritten = write(fd, writestr, writestrLength);

  if (bytesWritten < writestrLength) {
    syslog(LOG_ERR, "Not all bytes could be written.");
    close(fd);
    closelog();
    return 1;
  }

  int closeStatus = close(fd);
  if (closeStatus < 0) {
    syslog(LOG_ERR, "File could not be closed.");
    closelog();
    return 1;
  }

  closelog();
  return 0;
}
