#include "socket_utils.h"
#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

/*
 * Function to initialize the listener socket
 */
static int get_listener_socket(void) {
  int listener = -1;
  int yes = 1;
  int rv;

  /*
   * hints is for defining the kind of addresses we want to get from the local
   * IP - through AI_PASSIVE
   */
  struct addrinfo hints, *ai, *p;
  /*
   * Clearing hints and setting to 0 for new data
   */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
    fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
    return -1;
  }

  /*
   * Basically what this loop does is it goes over all the results that
   * getaddrinfo gets us and tries to create a listening socket of that
   * socsocket type, and protocol. We then set the socket options(configuration)
   * and then try binding the socket at the address at which we are iterating.
   * If the binding is successful, then we break the for loop and return this
   * socket as the listening socket otherwise we close the socket
   */
  for (p = ai; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0) {
      continue;
    }

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    /*
     * Bind the socket to ther particular address
     */
    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
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

  if (listen(listener, 128) == -1) {
    close(listener);
    return -1;
  }

  return listener;
}

/*
 * We want to set the listening socket to be non-blocking using the fcntl
 * function. This function can be used to set any file descriptor to be
 * non-blocking
 */
static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

fd_status_t on_peer_connected(int fd, const struct sockaddr_in *peer_addr,
                              socklen_t peer_addr_len) {
  assert(fd <= MAXFDS);

  peer_state_t *peer_state = &global_state[fd];
  memset(peer_state, 0, sizeof(*peer_state));
  peer_state->state = INITIAL_ACK;
  peer_state->sendbuf[0] = '*';
  peer_state->sendptr = 0;
  peer_state->sendbuf_end = 1;
  peer_state->connected = true;

  printf("Peer got connected on %d\n", fd);
}



