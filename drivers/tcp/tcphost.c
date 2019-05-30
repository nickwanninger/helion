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
#define MAX 80
#define PORT 8080
#define SA struct sockaddr




/*
uint64_t ntoh64(const uint64_t *input) {
  uint64_t rval;
  uint8_t *data = (uint8_t *)&rval;

  data[0] = *input >> 56;
  data[1] = *input >> 48;
  data[2] = *input >> 40;
  data[3] = *input >> 32;
  data[4] = *input >> 24;
  data[5] = *input >> 16;
  data[6] = *input >> 8;
  data[7] = *input >> 0;

  return rval;
}
*/




#define MAX_REQ_SIZE 128
// A normal C function that is executed as a thread
// when its name is specified in pthread_create()
void *handle_connection(void *vargp) {
  int fd = (int)vargp;

  // FILE *fp = fdopen(fd, "a+b");

  char buff[MAX_REQ_SIZE];

  for (;;) {
    // clear the buffer
    memset(buff, 0, MAX_REQ_SIZE);

    int nbytes = 0;
    // read the message from client and copy it in buffer
    if ((nbytes = read(fd, buff, sizeof(buff))) == 0) break;


    if (buff[0] == 'a') {
      char *data = buff + 1;
      if (nbytes != 5) {
        int zero = 0;
        write(fd, &zero, sizeof(int));
        continue;
      }

      int size = *(int *)data;
      void *ptr = malloc(size);
      write(fd, &ptr, sizeof(&ptr));
      continue;
    }


    if (buff[0] == 'f') {
      char *data = buff + 1;
      if (nbytes != 1 + sizeof(uint64_t)) {
        int zero = 0;
        write(fd, &zero, sizeof(int));
        continue;
      }

      void *addr = *(void **)data;
      free(addr);
      int one = 1;
      write(fd, &one, sizeof(int));
      continue;
    }



    if (buff[0] == 'r') {
      char *data = buff + 1;


      uint64_t addr = *(uint64_t*)data;
      data += sizeof(addr);

      int off = *(int*)data;
      data += sizeof(off);

      int len = *(int*)data;
      data += sizeof(len);


      char *ptr = (char*)addr;
      write(fd, ptr + off, len);
      continue;
    }
  }

  goto cleanup;
cleanup:
  printf("Socket disconnected\n");
  close(fd);
  return NULL;
}

// Driver function
int main() {
  int sockfd, connfd;

  unsigned int len;
  struct sockaddr_in servaddr, cli;

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("socket creation failed...\n");
    exit(0);
  } else
    printf("Socket successfully created..\n");
  bzero(&servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);

  // Binding newly created socket to given IP and verification
  if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
    printf("socket bind failed\n");
    exit(0);
  } else
    printf("socket successfully bound\n");

  // Now server is ready to listen and verification
  if ((listen(sockfd, 5)) != 0) {
    printf("listen failed...\n");
    exit(0);
  } else
    printf("server listening\n");
  len = sizeof(cli);


  while (1) {
    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA *)&cli, &len);
    if (connfd < 0) {
      printf("server acccept failed...\n");
      exit(0);
    } else
      printf("server acccept the client...\n");

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, handle_connection,
                   (void *)(uint64_t)connfd);
  }

  // After chatting close the socket
  close(sockfd);
}
