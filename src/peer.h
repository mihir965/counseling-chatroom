#ifndef PEER_H_
#define PEER_H_

#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdbool.h>
#include "../include/common.h"



/*
 * For understanding the state of the file descriptor
 */
typedef enum { INITIAL_ACK, WAIT_FOR_MSG, IN_MSG } ProcessingState;

/*
 * You will be storing the buffers, states and IDs per connection
 */
typedef struct {
  ProcessingState state;
  uint8_t sendbuf[SENDBUF_SIZE];
  int sendbuf_end;
  int sendptr;
  bool connected;
} peer_state_t;

/*
 * Struct to designate whether a file descriptor is ready to write or read
 */
typedef struct {
  bool want_read;
  bool want_write;
} fd_status_t;



/*
 * Constants to amke creating fd_status_t less verbose
 */
const fd_status_t fd_status_R = {.want_read = true, .want_write = false};
const fd_status_t fd_status_W = {.want_read = false, .want_write = true};
const fd_status_t fd_status_NORW = {.want_read = false, .want_write = false};
const fd_status_t fd_status_RW = {.want_read = true, .want_write = true};

/*
 * Global arrays of file descriptors that will help us maintain uniqueness and
 * will allow us to assign and clear used file descriptors.
 */
peer_state_t global_state[MAXFDS];


static void disconnect_peer(int epoll_fd, int fd, const char *reason);

fd_status_t on_peer_connected(int fd, const struct sockaddr_in *peer_addr,
                              socklen_t peer_addr_len);

fd_status_t on_peer_ready_recv(int fd, int epoll_fd);
#endif
