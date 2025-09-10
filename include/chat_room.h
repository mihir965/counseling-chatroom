#ifndef CHAT_ROOM_H_
#define CHAT_ROOM_H_

#include "common.h"
#include <stdbool.h>

#define MAX_ROOM_NAME_SIZE 64
#define MAX_ROOM_CLIENTS 32
#define MAX_ROOMS 64

typedef struct {
  // This is where we will define the room strcuture
  char room_name[MAX_ROOM_NAME_SIZE]; // Room name/ID
  int num_clients;                  // Number of clients in the room at any time
  int client_fds[MAX_ROOM_CLIENTS]; // An array storing the client_fds in the
                                    // room
} room_t;

extern room_t global_rooms[MAX_ROOMS]; // Global variable for the rooms

room_t *room_find_or_create(
    const char *room_name); // if a room of this name exists, or if not, either
                            // way, we will return a pointer to the room

bool room_add_client_to_room(room_t *room, int client_fd);

bool room_remove_client_from_room(room_t *room, int client_fd);

#endif // !CHAT_ROOM_H_
