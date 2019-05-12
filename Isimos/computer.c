#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "simos.h"

/**
 * implement functions for computer process
 */

void send_client_result(int sockfd, char *outstr)
{ char buffer[256];
  int ret;

  bzero(buffer, sizeof(buffer));
  sprintf(buffer, "%s", outstr);
  ret = send(sockfd, buffer, sizeof(buffer), 0);
  if (ret < 0) error("Server ERROR writing to socket");
}

void read_from_client(int fd)//submit_process
{ char buffer[256];
  int ret, result;
  char *client_id, *filename, *token;
  struct sockaddr_in cli_addr;
  socklen_t cli_addrlen = sizeof(cli_addr);

  bzero(buffer, sizeof(buffer));
  token = NULL;
  client_id = NULL;
  filename = NULL;
  req = NULL;

  ret = recv(fd, buffer, sizeof(buffer), 0);
  if (ret < 0) error("Server ERROR reading from socket");

  token = strtok(buffer, " ");
  client_id = (char *)malloc(strlen(token));
  strcpy(client_id, token);
  token = strtok(NULL, " ");
  filename = (char *)malloc(strlen(token));
  strcpy(filename, token);

  if (strcmp(filename, "nullfile") == 0)
  { close(fd);
    FD_CLR(fd, &active_fd_set);
  }
  else
  { getpeername(fd, (struct sockaddr *)&cli_addr, &cli_addrlen);
    submit_process(filename,fd);
    printf ("Received from %d file name %s\n", client_id, filename);
  }
}

void accept_client()
{ int newsockfd, clilen;
  struct sockaddr_in cli_addr;

  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0) error("ERROR accepting");
  else
  { printf("Accept client socket %d, %d\n", newsockfd, (int)cli_addr.sin_port);
    FD_SET(newsockfd, &active_fd_set);
  }
}

void initialize_socket(int portno)
{ struct sockaddr_in serv_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error("ERROR opening socket");
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR binding");

  listen(sockfd, 5);
}

void socket_select()
{ int i;
  fd_set read_fd_set;

  FD_ZERO(&active_fd_set);
  FD_SET(sockfd, &active_fd_set);

  while (Active)
  { /* Block until input arrives on one or more active sockets. */
    read_fd_set = active_fd_set;
    if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
    { perror("select"); exit(EXIT_FAILURE); }

    /* Service all the sockets with input pending. */
    for (i = 0; i < FD_SETSIZE; ++i)
      if (FD_ISSET(i, &read_fd_set))
      { if (i == sockfd) accept_client();
        else read_from_client(i);
      }
  }
}

void *init_server(void *arg)
{ char *port = (char *)arg;
  int portno;

  portno = atoi(port);
  initialize_socket (portno);
  socket_select ();
}
