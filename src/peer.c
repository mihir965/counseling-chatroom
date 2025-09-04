#include "peer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

/*
 * This function allows us to carefully disconnect from a peer that caused the
 * corresponding fd on server end to record an event but is not sending or
 * receiving any bytes of data, letting us know that the peer has been
 * disconnected
 */
static void disconnect_peer(int epoll_fd, int fd, const char *reason) {
  if (fd < 0 || fd > MAXFDS)
    return;
  fprintf(stderr, "disconnected fd=%d reason=%s\n", fd,
          reason ? reason : "unknown");
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
  close(fd);
  memset(&global_state[fd], 0, sizeof(global_state[fd]));
  return;
}

fd_status_t on_peer_ready_recv(int fd, int epoll_fd) {
  assert(fd <= MAXFDS);
  peer_state_t *peer_state = &global_state[fd];
  if (peer_state->state == INITIAL_ACK ||
      peer_state->sendptr < peer_state->sendbuf_end) {
    /*
     * This means that the initial acknowledgement is not yet sent to the client
     * about being connected to the server. Till this we have nothing we want to
     * receive from the client. Also, the condition checks if all the data is
     * staged by checking the sendptr and the sendbuf_end We return fd_status_W
     * because the file descriptor is ready to write to the client as no
     * acknowledgement is yet sent
     */
    return fd_status_W;
  }
  uint8_t buf[1024];
  /*
   * This function returns the number of bytes that the socket got from the
   * client
   */
  int nbytes = recv(fd, &buf, sizeof buf, 0);

  if (nbytes == 0) {
    printf("The client %d disconnected from the server", fd);
    disconnect_peer(epoll_fd, fd, "eof");
    /*
     * This file descriptor will be closed in the disconnect_peer method and the
     * fd_status need not be ready to read or write
     */
    return fd_status_NORW;
  }

  /*
   * This boolean variable is for when this file descriptor will be sending the
   * staged buffer. Here we will be setting the status for this file descriptor
   * to want_read which means that the client associated with the file
   * descriptor wants to read from ther server, which will change the flag in
   * the main epoll loop to EPOLLOUT
   */
  bool ready_to_send = false;
  for (int i = 0; i < nbytes; i++) {
    switch (peer_state->state) {
    case INITIAL_ACK:
      /*
       * This is a throwaway case
       */
      assert(0 && "can't reach here");
      break;
    case WAIT_FOR_MSG:
      if (buf[i] == '^') {
        peer_state->state = IN_MSG;
      }
      break;
    case IN_MSG:
      if (buf[i] == '$') {
        peer_state->state =
            WAIT_FOR_MSG; // This is because after the end of the message, this
                          // file descriptor will be waiting for message again
                          // from the client
      }
    }
  }
}
