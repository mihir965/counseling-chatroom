#include "../include/cmd.h"
#include <stdio.h>
#include <string.h>

void cmd_join_or_create_room(peer_state_t *peer_state, int sock_fd) {
  printf("This ran\n");
  char room_name[MAX_ROOM_NAME_SIZE];
  int itr = 0;
  for (int i = 5; i < peer_state->recvbuf_end; i++) {
    room_name[itr] = peer_state->recvbuf[i];
    itr++;
  }
  room_name[itr] = '\0';
  /*
   * Now we will have the room_name. This will go through the entire logic of
   * creating the ai_agent using pthread and then returns the newly created room
   * with the agent. Users are only added after this, therefore, no need to
   * protect the room's client_fds from concurrency issues from threading
   */
  room_t *room = room_find_or_create(&room_name[0]);
  printf("Got room (%s)\n", room->room_name);
  if (room_add_client_to_room(room, sock_fd)) {
    printf("Server: Client (%d) has joined room %s..\n", sock_fd,
           (char *)room_name);
    peer_state->rooms_joined[peer_state->num_rooms_joined++] = room;
  } else {
    printf("Server: Could not add client to room\n");
  }
}

void cmd_leave_room(peer_state_t *peer_state, int sock_fd) {
  printf("Removing client...\n");
  char room_name[MAX_ROOM_NAME_SIZE];

  int itr = 0;
  for (int i = 6; i < peer_state->recvbuf_end; i++) {
    room_name[itr++] = peer_state->recvbuf[i];
  }
  room_name[itr] = '\0';
  /*
   * Now remove the client from the room
   */
  room_t *room = room_find_or_create(&room_name[0]);
  if (room_remove_client_from_room(room, sock_fd)) {
    printf("Server: Client (%d) has left the room %s...\n", sock_fd,
           (char *)room_name);
    /*
     * We need to iterate over all the rooms that this client had joined and get
     * to this room and remove it from the array
     */
    for (int i = 0; i < peer_state->num_rooms_joined; i++) {
      if (peer_state->rooms_joined[i] == room) {
        peer_state->rooms_joined[i] = NULL;
        peer_state->num_rooms_joined--;
        break;
      }
    }
    printf("Client: (%d) is now connected to (%d) rooms\n", sock_fd,
           peer_state->num_rooms_joined);
  } else {
    printf("server: Could not remove the cleint from room\n");
  }
}

int cmd_list_rooms(peer_state_t *peer_state, int sock_fd) {
  printf("There are (%d)\n", global_num_rooms);
  char msg[MAX_ROOM_NAME_SIZE * 64] = "";
  for (int i = 0; i < global_num_rooms; i++) {
    printf("Adding (%s)\n", (char *)global_rooms[i].room_name);
    strncat(msg, (char *)global_rooms[i].room_name,
            sizeof(msg) - strlen(msg) - 1);
    strncat(msg, "\n", sizeof(msg) - strlen(msg) - 1);
  }
  size_t msglen = strlen(msg);
  memcpy(peer_state->sendbuf, msg, msglen);
  peer_state->sendbuf_end = msglen;
  peer_state->sendptr = 0;
  peer_state->recvbuf_end = 0;
  peer_state->state = WAIT_FOR_MSG;
  return 1;
}
