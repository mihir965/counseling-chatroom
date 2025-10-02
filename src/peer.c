#define _GNU_SOURCE

#include "../include/peer.h"
#include "../include/chat_room.h"
#include "../include/cmd.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

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

static inline void mod_interest(int epoll_fd, int fd, bool want_read,
                                bool want_write) {
  if (!want_read && !want_write) {
    disconnect_peer(epoll_fd, fd, "no-interest");
    return;
  }
  struct epoll_event ev = {0};
  ev.data.fd = fd;
  if (want_read)
    ev.events |= EPOLLIN;
  if (want_write)
    ev.events |= EPOLLOUT;
  epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void disconnect_peer(int epoll_fd, int fd, const char *reason) {
  if (fd < 0 || fd >= MAXFDS)
    return;
  fprintf(stderr, "disc fd=%d reason = %s\n", fd, reason ? reason : "unknown");

  // Stop watching it
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
  close(fd);
  memset(&global_state[fd], 0, sizeof(global_state[fd]));
}

fd_status_t peer_on_peer_connected(int sock_fd) {
  assert(sock_fd < MAXFDS);

  peer_state_t *peer_state = &global_state[sock_fd];
  memset(peer_state, 0, sizeof(*peer_state));
  peer_state->state = INITIAL_ACK;
  strncpy((char *)peer_state->sendbuf, "Please enter your username:\n",
          SENDBUF_SIZE);
  peer_state->sendbuf_end = strlen((char *)peer_state->sendbuf);
  peer_state->sendptr = 0;
  peer_state->num_rooms_joined = 0;
  printf("Peer got connected on %d\n", sock_fd);
  /*
   * The file descriptor is ready to write to the peer
   */
  return fd_status_W;
}

fd_status_t peer_on_peer_connected_recv(int sock_fd, int epoll_fd) {
  assert(sock_fd <= MAXFDS);
  peer_state_t *peer_state = &global_state[sock_fd];
  if (peer_state->state == INITIAL_ACK ||
      peer_state->sendptr < peer_state->sendbuf_end) {
    /*
     * Until the intial ACK has been sent to the peer, there's nothing we want
     * to receive. Also wait until all data staged for sending is sent to
     * receive more data
     */
    return fd_status_W;
  }

  uint8_t buf[1024];
  /*
   * Returns the amount that the listener gets from the file descriptor
   */
  int nbytes = recv(sock_fd, buf, sizeof(buf), 0);
  if (nbytes == 0) {
    /*
     * The peer disconnected
     */
    printf("Peer %d disconnected from the server", sock_fd);
    disconnect_peer(epoll_fd, sock_fd, "eof");
    return fd_status_NORW;
  } else if (nbytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      /*
       * The socket is not ready for recv; wait till it is.
       */
      return fd_status_R;
    } else {
      perror("recv\n");
      disconnect_peer(epoll_fd, sock_fd, "recv error");
      return fd_status_NORW;
    }
  }
  bool ready_to_send = false;
  for (int i = 0; i < nbytes; i++) {
    switch (peer_state->state) {
    case INITIAL_ACK:
      assert(0 && "can't reach here");
      break;
    case WAIT_FOR_MSG:
      if (buf[i] == '^') {
        peer_state->state = IN_MSG;
        peer_state->recvbuf_end = 0;
      } else if (buf[i] == '@') {
        peer_state->state = IN_CMD;
        peer_state->recvbuf_end = 0;
        peer_state->recvbuf[peer_state->recvbuf_end++] = '@';
      }
      break;
    case IN_CMD:
      if (buf[i] == '#') {
        /*
         * This means that the command is over
         * We will compare the first 6 bytes of the recieved buffer to determine
         * the command
         */
        printf("Okay IN_CMD\n");

        /*
         * Adding null termination to the buffer that we recieved from the
         * client
         */
        peer_state->recvbuf[peer_state->recvbuf_end++] = '\0';

        printf("recvbuf = \"%s\"\n", (char *)peer_state->recvbuf);
        if (strncmp((char *)peer_state->recvbuf, "@JOIN", 5) == 0) {
          /*
           * Then we want to know the chat_room name
           */
          cmd_join_or_create_room(peer_state, sock_fd);
        }
        /*
         * Adding functionality to leave a room
         */
        else if (strncmp((char *)peer_state->recvbuf, "@LEAVE", 6) == 0) {
          cmd_leave_room(peer_state, sock_fd);
        }
        /*
         * Adding functionality to list the available rooms in the system
         */
        else if (strncmp((char *)peer_state->recvbuf, "@LIST", 5) == 0) {
          if (cmd_list_rooms(peer_state, sock_fd))
            return fd_status_W;
        }
        /*
         * This is where I will add the functions needed to talk to the AI agent
         */
        else if (strncmp((char *)peer_state->recvbuf, "@COUNSEL", 8) == 0) {
          /*
           * This is the idea that I have. First let's only create a 2 user
           * counseling agent. Therefore, the server must look if there are two
           * messages that are associated to the command @COUNSEL that are
           * stored in the room's counsel buffer. We will be concatenating these
           * messages togehter and then inputting them to the counseling agent
           */
        }
        peer_state->recvbuf_end = 0;
        peer_state->state = WAIT_FOR_MSG;
      } else {
        if (peer_state->recvbuf_end < sizeof(peer_state->recvbuf)) {
          peer_state->recvbuf[peer_state->recvbuf_end++] = (char)buf[i];
        }
      }
      break;
    case IN_MSG:
      if (buf[i] == '$') {
        printf("This ran\n");
        peer_state->state = WAIT_FOR_MSG;
        if (strlen((char *)peer_state->user_name) == 0) {
          /*
           * We basically first null terminate the recv buffer. This will be
           * the first message that the client sends, which is the username
           */
          peer_state->recvbuf[peer_state->recvbuf_end] = '\0';
          strncpy((char *)peer_state->user_name, (char *)peer_state->recvbuf,
                  USER_NAME_SIZE - 1);
          peer_state->user_name[USER_NAME_SIZE - 1] = '\0';

          /*
           * Setting the recvbuf_end to 0 will clear the recv buffer since we
           * will just be overwriting the buffer with new data
           */
          peer_state->recvbuf_end = 0;
          snprintf((char *)peer_state->sendbuf, SENDBUF_SIZE, "Welcome, %s!\n",
                   peer_state->user_name);
          printf("Welcome, %s!\n", peer_state->user_name);
          peer_state->sendptr = 0;
        } else {
          /*
           * This is when there is an actual message or command that is not
           * related to the username being inputted
           */
          printf("Actual Message\n");

          /*
           * This is to stop peers from putting random chats
           */
          if (peer_state->num_rooms_joined == 0) {
            const char *msg = "You have not joined any rooms!\n";
            strncpy((char *)peer_state->sendbuf, msg, SENDBUF_SIZE - 1);
            peer_state->sendbuf[SENDBUF_SIZE - 1] = '\0';
            peer_state->sendbuf_end = strlen((char *)peer_state->sendbuf);
            peer_state->sendptr = 0;
            return fd_status_W;
          }
          /*
           * This is where we are taking just the 0th room for now and then
           * sending to all the clients
           */
          room_t *room = peer_state->rooms_joined[0];
          printf("Sending to peers in the room (%s)\n",
                 (char *)room->room_name);
          for (int i = 0; i < room->num_clients; i++) {
            printf("%d\n", i);
            int other_fd = room->client_fds[i];
            if (sock_fd == other_fd) {
              printf("Nope\n");
              continue;
            }
            peer_state_t *other = &global_state[other_fd];
            bool was_empty = (other->sendptr >= other->sendbuf_end);
            if (other->state == INITIAL_ACK)
              continue;
            size_t need = peer_state->recvbuf_end + 1;
            if (other->sendbuf_end + need <= SENDBUF_SIZE) {
              memcpy(&other->sendbuf[other->sendbuf_end], peer_state->recvbuf,
                     peer_state->recvbuf_end);
              other->sendbuf_end += peer_state->recvbuf_end;
              other->sendbuf[other->sendbuf_end++] = '\n';
            } else {
              /*
               * Backpressure policy
               */
              fprintf(stderr,
                      "disc: %d is attempting to send too long "
                      "messages...dropping\n",
                      sock_fd);
              disconnect_peer(epoll_fd, sock_fd, "backpressure");
              return fd_status_NORW;
            }
            if (was_empty)
              mod_interest(epoll_fd, other_fd, true, true);
          }
        }
        /*
         * Resetting the recvBuf of the fd to 0 for next messages
         */
        memset(&peer_state->recvbuf, 0, peer_state->recvbuf_end);
      } else {
        /*
         * We are basically accumulating all the bytes from the client into
         * the recv buffer, which is then copied into the username or the send
         * buf
         */
        if (peer_state->recvbuf_end < sizeof(peer_state->recvbuf)) {
          peer_state->recvbuf[peer_state->recvbuf_end++] = (char)buf[i];
        }
        ready_to_send = true;
      }
      break;
    }
  }
  return (fd_status_t){.want_read = !ready_to_send,
                       .want_write = ready_to_send};
}

fd_status_t peer_on_peer_connected_send(int sock_fd, int epoll_fd) {
  assert(sock_fd < MAXFDS);
  peer_state_t *peer_state = &global_state[sock_fd];

  if (peer_state->sendptr >= peer_state->sendbuf_end) {
    /*
     * Nothing to send
     */
    return fd_status_RW;
  }

  int send_len = peer_state->sendbuf_end - peer_state->sendptr;
  int nsent =
      send(sock_fd, &peer_state->sendbuf[peer_state->sendptr], send_len, 0);
  if (nsent == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      /*
       * This file descriptor needs to write to the client but has blocked
       */
      return fd_status_W;
    } else {
      disconnect_peer(epoll_fd, sock_fd, "Send error");
      return fd_status_NORW;
    }
  }
  if (nsent < send_len) {
    /*
     * This means that there was a partial send, and the file descriptor still
     * needs to write more to the client end
     */
    peer_state->sendptr += nsent;
    return fd_status_W;
  } else {
    /*
     * Everything was sent
     */
    peer_state->sendptr = 0;
    peer_state->sendbuf_end = 0;

    if (peer_state->state == INITIAL_ACK) {
      peer_state->state = WAIT_FOR_MSG;
    }
    return fd_status_R;
  }
}
