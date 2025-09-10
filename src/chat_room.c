#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define _GNU_SOURCE

#include "../include/chat_room.h"

int num_rooms = 0;
room_t global_rooms[MAX_ROOMS];

room_t *room_find_or_create(const char *room_name) {
  /*
   * First we will look for the room by comparing the room_name
   */
  if (num_rooms != 0) {
    for (int i = 0; i < num_rooms; i++) {
      if (strcmp(room_name, global_rooms[i].room_name) == 0) {
        /*
         * We found the room
         */
        printf("The room (%s) already exists!\n",
               (char *)global_rooms[i].room_name);
        return &global_rooms[i];
      }
    }
    /*
     * Once we come out of that loop that means that the room does not exist
     */
    room_t *new_room = &global_rooms[num_rooms++];
    strncpy(new_room->room_name, room_name, MAX_ROOM_NAME_SIZE);
    new_room->room_name[MAX_ROOM_NAME_SIZE - 1] = '\0';
    new_room->num_clients = 0;
    return new_room;
  } else {
    printf("There were no rooms!\n");
    room_t *new_room = &global_rooms[0];
    num_rooms++;
    strncpy(new_room->room_name, room_name, MAX_ROOM_NAME_SIZE);
    new_room->room_name[MAX_ROOM_NAME_SIZE - 1] = '\0';
    new_room->num_clients = 0;
    return new_room;
  }
  return NULL;
}

bool room_add_client_to_room(room_t *room, int client_fd) {
  if (!room)
    return false;
  room->num_clients++;
  room->client_fds[client_fd] = client_fd;
  return true;
}

bool room_remove_client_from_room(room_t *room, int client_fd) {
  if (!room)
    return false;
  room->num_clients--;
  room->client_fds[client_fd] = 0;
  return true;
}
