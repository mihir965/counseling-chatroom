#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "peer.h"
#include "socket_utils.h"

int main(void) {
  signal(SIGPIPE, SIG_IGN);

  /*
   * This is where we will use the get_listener_socket function to get the main
   * listening socket that will accept clients
   */
  int listener = get_listener_socket();
  if (listener == -1) {
    fprintf(stderr, "error getting listening socket\n");
    return -1;
  }

  /*
   * We will then set the listener socket to be non-blocking since pipes are
   * slow files due to the non-deterministic nature of the read and write
   * functions associated with network sockets
   */
  if (set_nonblocking(listener) == -1) {
    perror("fcntl O_NONBLOCK (listener)");
    close(listener);
    return 1;
  }

  /*
   * Now let's create the epoll instance using the epoll_create1 function. This
   * fd is then used to add the listener fd to the list that is being monitored
   * for events EPOLLIN
   */
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    perror("epoll_create1");
    exit(1);
  }

  struct epoll_event accept_event;
  accept_event.data.fd = listener;
  accept_event.events = EPOLLIN;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &accept_event) < 0) {
    perror("epoll_ctl EPOLL_CTL_ADD");
  }

  struct epoll_event *events = calloc(MAXFDS, sizeof(struct epoll_event));
  if (events == NULL) {
    perror("Unable to allocate memory for epoll_events");
    exit(1);
  }

  puts("server: waiting for connections ...\n");

  /*
   * Main epoll loop
   */
  while (1) {
    int nready = epoll_wait(epoll_fd, events, MAXFDS, -1);
    for (int i = 0; i < nready; i++) {
      /*
       * if the ith events fd == listener that means that a new peer is
       * connecting to the server
       */
      if (events[i].data.fd == listener) {
        printf("New peer is connecting...\n");
        /*
         * We need the information about the new peer for accepting it and
         * working with newly created file descriptor
         */
        struct sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof peer_addr;
        int new_sock_fd =
            accept(listener, (struct sockaddr *)&peer_addr, &peer_addr_len);

        if (new_sock_fd < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            perror("accept returned EAGAIN or EWOULDBLOCK");
          } else {
            perror("accept");
            exit(1);
          }
        } else {
          /*
           * We should also set the new file descriptor for the new client to be
           * non-blocking using our function
           */
          set_nonblocking(new_sock_fd);
          if (new_sock_fd >= MAXFDS) {
            fprintf(stderr, "socket fd (%d) >= MAXFDS (%d)", new_sock_fd,
                    MAXFDS);
            exit(1);
          }
        }

        /*
         * We get the initial file descriptor status of the peer and look if it
         * already needs to read from client or write into client
         */
        fd_status_t peer_status = on_peer_connected(
            new_sock_fd, (struct sockaddr *)&peer_addr, peer_addr_len);
        struct epoll_event event = {0};
        event.data.fd = new_sock_fd;
        if (peer_status.want_write) {
          event.events |= EPOLLIN;
        }
        if (peer_status.want_read) {
          event.events |= EPOLLOUT;
        }
        /*
         * This adds the new peer file descriptor to the monitored set of file
         * descriptors
         */
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock_fd, &event) < 0) {
          perror("epoll_ctl EPOLL_CTL_ADD");
          exit(1);
        }
      }
      /*
       * This means that a file descriptor in the events arrays for a client has
       * recorded an event
       */
      else {
        /*
         * This means that the client is trying to send packets of data
         */
        if (events[i].events & EPOLLIN) {
          // Reading
          int fd = events[i].data.fd;
          fd_status_t peer_status = on_peer_ready_recv(fd, epoll_fd);
        }
      }
    }
  }
}
