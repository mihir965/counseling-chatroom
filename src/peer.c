#include <stdbool.h>
#define _GNU_SOURCE

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "../include/peer.h"

void disconnect_peer(int epoll_fd, int fd, const char* reason){
    if(fd<0 || fd>=MAXFDS) return;
    fprintf(stderr, "disc fd=%d reason = %s\n", fd, reason?reason: "unknown");

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
  peer_state->sendbuf[0] = '* Please enter your username:\n';
  peer_state->sendptr = 0;
  peer_state->sendbuf_end = strlen(peer_state->sendbuf);

  printf("Peer got connected on %d\n", sock_fd);

  /*
   * The file descriptor is ready to write to the peer
   */
  return fd_status_W;
}

fd_status_t peer_on_peer_connected_recv(int sock_fd, int epoll_fd){
    assert(sock_fd <= MAXFDS);
    peer_state_t *peer_state = &global_state[sock_fd];
    if(peer_state->state == INITIAL_ACK || peer_state->sendptr < peer_state->sendbuf_end){
        /*
         * Until the intial ACK has been sent to the peer, there's nothing we want to receive. Also wait until all data staged for sending is sent to receive more data
         */
        return fd_status_W;
    }

    uint8_t buf[1024];
    /*
     * Returns the amount that the listener gets from the file descriptor
     */
    int nbytes = recv(sock_fd, buf, sizeof(buf), 0);
    if(nbytes == 0){
        /*
         * The peer disconnected
         */
        printf("Peer %d disconnected from the server", sock_fd);
        disconnect_peer(epoll_fd, sock_fd, "eof");
        return fd_status_NORW;
    }else if(nbytes < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            /*
             * The socket is not ready for recv; wait till it is.
             */
            return fd_status_R;
        }else{
            perror("recv\n");
            disconnect_peer(epoll_fd, sock_fd, "recv error");
            return fd_status_NORW;
        }
    }
    else{
        bool ready_to_send = false;
        for(int i=0; i<nbytes; i++){
            switch (peer_state->state) {
                case INITIAL_ACK:
                    assert(0 && "can't reach here");
                    break;
                case WAIT_FOR_MSG:
                    if(buf[i] == '^'){
                        peer_state->state = IN_MSG;
                    }
                    break;
                case IN_MSG:
                    if(buf[i] == '$'){
                        peer_state->state = WAIT_FOR_MSG;
                        if(strlen(peer_state->user_name)==0){
                            /*
                             * We basically first null terminate the recv buffer. This will be the first message that the client sends, which is the username
                             */
                            peer_state->recvbuf[peer_state->recvbuf_end] = '\0';
                            strncpy((char*)peer_state->user_name, (char*)peer_state->recvbuf, USER_NAME_SIZE-1);
                            peer_state->user_name[USER_NAME_SIZE-1] = '\0';

                            /*
                             * Setting the recvbuf_end to 0 will clear the recv buffer since we will just be overwriting the buffer with new data
                             */
                            peer_state->recvbuf_end = 0;

                            

                        }
                    }else{

                        /*
                         * We are basically accumulating all the bytes from the client into the recv buffer, which is then copied into the username or the send buf
                         */
                        if(peer_state->recvbuf_end <= sizeof(peer_state->recvbuf)){
                            peer_state->recvbuf[peer_state->recvbuf_end++] = (char)buf[i];
                        }
                    }
            }
        }
    }
}
