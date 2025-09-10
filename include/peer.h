#ifndef PEER_H_
#define PEER_H_

#include "chat_room.h"
#include "common.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
typedef enum { INITIAL_ACK, WAIT_FOR_MSG, IN_MSG, IN_CMD } ProcessingState;

#define USER_NAME_SIZE 64
#define RECVBUF_SIZE 1024
#define MAX_ROOMS_CAN_JOIN 64

typedef struct {
  ProcessingState state;
  uint8_t user_name[USER_NAME_SIZE];

  uint8_t recvbuf[RECVBUF_SIZE];
  int recvbuf_end;

  uint8_t sendbuf[SENDBUF_SIZE];
  int sendbuf_end;
  int sendptr;

  int num_rooms_joined;
  room_t *rooms_joined[MAX_ROOMS_CAN_JOIN];
} peer_state_t;

typedef struct {
  bool want_read;
  bool want_write;
} fd_status_t;

fd_status_t peer_on_peer_connected(int sock_fd);

fd_status_t peer_on_peer_connected_recv(int sock_fd, int epoll_fd);

fd_status_t peer_on_peer_connected_send(int sock_fd, int epoll_fd);

void disconnect_peer(int epoll_fd, int fd, const char *reason);

#endif // !PEER_H_
