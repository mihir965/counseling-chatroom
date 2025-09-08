#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/peer.h"
#include "../include/sockets.h"
int main() {
  /*
   * First get the listener socket
   */
  int listener_socket = sockets_get_listener_socket();
  if (listener_socket == -1) {
    fprintf(stderr, "error getting the listner socket\n");
    return -1;
  }

  /*
   * Then make the listener socket non-blocking
   */
  if (sockets_set_non_blocking(listener_socket) == -1) {
    perror("fcntl O_NONBLOCK (listener)");
    close(listener_socket);
    return -1;
  }

  /*
   * Then create the epoll_instance and set the listener socket to EPOLLIN
   * Also create the events array for epoll
   */
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    perror("epoll_create1");
    return -1;
  }

  /*
   * We initialize an accepe event for the listener socket, and set the events
   * being monitored on that fd as EPOLLIN
   */
  struct epoll_event accept_event;
  accept_event.data.fd = listener_socket;
  accept_event.events = EPOLLIN;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener_socket, &accept_event) < 0) {
    perror("epoll_ctl EPOLL_CTL_ADD");
  }

  struct epoll_event *events = calloc(MAXFDS, sizeof(struct epoll_event));

  if (events == NULL) {
    perror("Unable to allocate memory for the epoll_events array");
    return -1;
  }

  puts("server: waiting for connections...\n");

  while (1) {
    int nready = epoll_wait(epoll_fd, events, MAXFDS, -1);
    for (int i = 0; i < nready; i++) {
      if (events[i].data.fd == listener_socket) {
        /*
         * This means that the listener_socket recorded an event meaning that
         * the listener socket might accept new peers
         */
        printf("Peer is now connecting...\n");
        struct sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        /*
         * We create the new socket as the new peer socket fd
         */
        int new_peer_fd = accept(listener_socket, (struct sockaddr *)&peer_addr,
                                 &peer_addr_len);

        if (new_peer_fd < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Can happen in a non-blocking socket
            printf("accept returned EAGAIN or EAWOULDBLOCK");
          } else {
            perror("accept");
            return -1;
          }
        } else {
          sockets_set_non_blocking(new_peer_fd);
          if (new_peer_fd >= MAXFDS) {
            fprintf(stderr, "socket fd (%d) >= MAXFDS (%d)", new_peer_fd,
                    MAXFDS);
            return -1;
          }
          fd_status_t peer_status = peer_on_peer_connected(new_peer_fd);
          struct epoll_event event = {0};
          event.data.fd = new_peer_fd;
          if (peer_status.want_read) {
            event.events |= EPOLLIN;
          }
          if (peer_status.want_write) {
            event.events |= EPOLLOUT;
          }
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_peer_fd, &event) < 0) {
            perror("epoll_ctl EPOLL_CTL_ADD");
            return -1;
          }
        }
      } else {
        /*
         * This means that another fd in the events array has recorded an event
         */
        /*
         * Reading
         */
        if (events[i].events & EPOLLIN) {
          int event_fd = events[i].data.fd;
          fd_status_t fd_status =
              peer_on_peer_connected_recv(event_fd, epoll_fd);
          struct epoll_event event = {0};
          event.data.fd = event_fd;
          if (fd_status.want_read) {
            event.events |= EPOLLIN;
          }
          if (fd_status.want_write) {
            event.events |= EPOLLOUT;
          }
          if (event.events == 0) {
            printf("socket %d closing\n", event_fd);
            disconnect_peer(epoll_fd, event_fd, "no-interest");
            continue;
          } else if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &event) < 0) {
            perror("epoll_ctl EPOLL_CTL_MOD");
            return -1;
          }
        } else if (events[i].events & EPOLLOUT) {
          int event_fd = events[i].data.fd;
          fd_status_t fd_status =
              peer_on_peer_connected_send(event_fd, epoll_fd);
          struct epoll_event event = {0};
          event.data.fd = event_fd;
          if (fd_status.want_read) {
            event.events |= EPOLLIN;
          }
          if (fd_status.want_write) {
            event.events |= EPOLLOUT;
          }
          if (event.events == 0) {
            printf("socket %d closing \n", event_fd);
            disconnect_peer(epoll_fd, event_fd, "no-interest");
            continue;
          } else if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &event) < 0) {
            perror("epoll_ctl EPOLL_CTL_MOD");
            return -1;
          }
        }
      }
    }
  }
  return -1;
}
