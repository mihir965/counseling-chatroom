#ifndef PEER_H_
#define PEER_H_

#include "common.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
typedef enum { INITIAL_ACK, WAIT_FOR_MSG, IN_MSG } ProcessingState;

#define USER_NAME_SIZE 64
#define RECVBUF_SIZE 1024

typedef struct {
  ProcessingState state;
  uint8_t user_name[USER_NAME_SIZE];

  uint8_t recvbuf[RECVBUF_SIZE];
  int recvbuf_end;

  uint8_t sendbuf[SENDBUF_SIZE];
  int sendbuf_end;
  int sendptr;
} peer_state_t;

typedef struct {
  bool want_read;
  bool want_write;
} fd_status_t;

// Constants to make fd_status_t less verbose;
const fd_status_t fd_status_R = {.want_read = true, .want_write = false};
const fd_status_t fd_status_W = {.want_read = false, .want_write = true};
const fd_status_t fd_status_RW = {.want_read = true, .want_write = true};
const fd_status_t fd_status_NORW = {.want_read = false, .want_write = false};

// Each peer is globally identified by the fd it is connected on. As long as a
// peer is connected, the fd is unique to it. When a peer disconnects, a new
// peer may connect and get the same fd. The function on_peer_connected should
// initialize the state properly to remove the trace of the old peer on the same
// fd.
peer_state_t global_state[MAXFDS];

fd_status_t peer_on_peer_connected(int sock_fd);

fd_status_t peer_on_peer_connected_recv(int sock_fd, int epoll_fd);

#endif // !PEER_H_
