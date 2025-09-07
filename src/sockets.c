#define _GNU_SOURCE

#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/common.h"

int sockets_get_listener_socket() {
  int listener = -1;
  int yes = 1;
  int rv;

  struct addrinfo hints, *ai, *p;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  /*
   * We are creating the listening socket on our own local port therefore do not
   * require to put in any IP address and NULL is okay. Once we get the pointer
   * to the array of results from getaddrinfo we iterate over it
   */
  if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
    fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
    return -1;
  }

  for (p = ai; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0) {
      /*
       * This means that we could not create a listener socket
       */
      continue;
    }

    /*
     * Once we are out of the if statement we can say that a listener was
     * created, now we put some options on the listener
     */
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
      /*
       * Means we were not able to bind the socket to our local IP address
       * And then we reset the listener to -1 and check on other results
       */
      close(listener);
      listener = -1;
      continue;
    }
    break;
  }

  if (p == NULL) {
    freeaddrinfo(ai);
    return -1;
  }

  freeaddrinfo(ai);

  /*
   * We start listening with the listener socket
   */
  if (listen(listener, 128) == -1) {
    close(listener);
    return -1;
  }

  return listener;
}

int sockets_set_non_blocking(int socket_fd) {
  int flags = fcntl(socket_fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}
