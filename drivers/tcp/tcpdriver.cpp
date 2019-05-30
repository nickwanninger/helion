#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <mutex>

std::mutex tcplock;
int sockfd = -1;
struct sockaddr_in servaddr;

extern "C" {

int helion_driver_init(const char *options) {
  // socket create and varification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("socket creation failed...\n");
    exit(0);
  }
  bzero(&servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(8080);

  // connect the client socket to server socket
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
    printf("connection with the server failed...\n");
    exit(0);
  }

  return 1;
}


uint64_t helion_driver_alloc(int size) {
  std::unique_lock lock(tcplock);
  char buf[5];
  buf[0] = 'a';
  *(int *)(buf + 1) = size;

  int sent = write(sockfd, buf, 5);
  if (sent == 0) {
    printf("failed to send\n");
    exit(1);
  }

  uint64_t addr = 0;
  read(sockfd, &addr, sizeof(uint64_t));
  return addr;
}

int helion_driver_free(uint64_t addr) {
  std::unique_lock lock(tcplock);
  char buf[1 + sizeof(uint64_t)];
  buf[0] = 'f';
  *(uint64_t *)(buf + 1) = addr;

  int sent = write(sockfd, buf, 1 + sizeof(uint64_t));
  if (sent == 0) {
    printf("failed to send\n");
    exit(1);
  }

  int res = 0;
  read(sockfd, &res, sizeof(res));
  return res;
}


int helion_driver_read(uint64_t addr, int off, int len, void *dst) {
  std::unique_lock lock(tcplock);
  char cmd[20];

  char *data = cmd;
  data[0] = 'r';
  data++;

  *(uint64_t *)data = addr;
  data += sizeof(addr);

  *(int *)data = off;
  data += sizeof(off);

  *(int *)data = len;
  data += sizeof(len);

  write(sockfd, cmd, 1 + sizeof(uint64_t) + sizeof(int) + sizeof(int));

  int rl = read(sockfd, dst, len);
  if (rl != len) {
    return 0;
  }

  return 1;
}

int helion_driver_write(uint64_t addr, int offset, int len, void *src) {
  std::unique_lock lock(tcplock);
  return 0;
}


}
