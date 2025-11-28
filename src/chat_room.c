#include <time.h>
#define _GNU_SOURCE

#include "../include/chat_room.h"
#include "../include/comms.h"
#include "../include/peer.h"
#include "../include/utils.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

int global_num_rooms = 0;
room_t global_rooms[MAX_ROOMS];

room_t *room_find_or_create(const char *room_name) {

  /*
   * First we will look for the room by comparing the room_name
   */
  if (global_num_rooms != 0) {
    for (int i = 0; i < global_num_rooms; i++) {
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
    room_t *new_room = &global_rooms[global_num_rooms++];
    strncpy(new_room->room_name, room_name, MAX_ROOM_NAME_SIZE);
    new_room->room_name[MAX_ROOM_NAME_SIZE - 1] = '\0';
    new_room->num_clients = 0; // We initialize it to 0

    uuid_t uuid;
    char uuid_str[37];

    uuid_generate_random(uuid);

    uuid_unparse(uuid, uuid_str);

    /* Creating a key value pair for the room, and adding the pointer to the
     * room as the value of the key */
    token_map *token = malloc(sizeof(token_map));
    strncpy(token->token, uuid_str, sizeof(token->token) - 1);
    token->token[sizeof(token->token) - 1] = '\0';
    token->room = new_room;
    HASH_ADD_STR(map, token, token);

    /*
     * First we need to get the agent_name for the room by first making a
     * request to the python server
     */
    char *agent_name = comms_connect_agent(room_name, uuid_str);
    printf("The name of the agent that is connected to the room is: %s\n",
           agent_name);
    free(agent_name);
    return new_room;
  } else {
    printf("There were no rooms!\n");
    room_t *new_room = &global_rooms[0];
    global_num_rooms++;
    strncpy(new_room->room_name, room_name, MAX_ROOM_NAME_SIZE);
    new_room->room_name[MAX_ROOM_NAME_SIZE - 1] = '\0';
    new_room->num_clients =
        0; // We initialize it to 1 since the ai_agent is technically a cleint

    uuid_t uuid;
    char uuid_str[37];

    uuid_generate_random(uuid);

    uuid_unparse(uuid, uuid_str);

    /* Creating a key value pair for the room, and adding the pointer to the
     * room as the value of the key */
    token_map *token = malloc(sizeof(token_map));
    strncpy(token->token, uuid_str, sizeof(token->token) - 1);
    token->token[sizeof(token->token) - 1] = '\0';
    token->room = new_room;
    HASH_ADD_STR(map, token, token);

    /*
     * Let us also hit the python server to create the ai agent socket
     */
    char *agent_name = comms_connect_agent(room_name, uuid_str);
    printf("The name of the agent that is connected to the room is: %s\n",
           agent_name);
    /* We aren't really using agent_name and it will cause a memory leak */
    free(agent_name);
    return new_room;
  }
  return NULL;
}

bool room_add_client_to_room(room_t *room, int client_fd) {
  if (!room)
    return false;
  for (int i = 0; i < room->num_clients; i++) {
    if (room->client_fds[i] == client_fd)
      return true;
  }
  room->client_fds[room->num_clients++] = client_fd;
  return true;
}

bool room_remove_client_from_room(room_t *room, int client_fd) {
  if (!room)
    return false;
  int i = 0;
  while (i <= room->num_clients) {
    if (room->client_fds[i] == client_fd) { room->client_fds[i] = 0;
      return true;
    }
    i++;
  }
  return false;
}

void room_add_message_to_history(room_t *room, const char *message,
                                 const char *username) {
  if (!room)
    return;
  /* We basically assign the content of the message to the message_index in the
   * circular buffer, also change the username correctly */
  strncpy(room->messages[room->message_index % MAX_MESSAGES_HISTORY].content,
          message, MAX_MESSAGE_LEN - 1);
  strncpy(room->messages[room->message_index % MAX_MESSAGES_HISTORY].username,
          username, USER_NAME_SIZE - 1);
  room->messages[room->message_index % MAX_MESSAGES_HISTORY].timestamp =
      time(NULL);

  room->message_index++;

  if (room->num_messages < MAX_MESSAGES_HISTORY) {
    room->num_messages++;
    /* This ensures that the num_messages in the room will stop increasing at
     * MAX_MESSAGES_HISTORY */
  }
}

void room_reset_counseling_flags(room_t *room) {
  if (!room)
    return;
  memset(room->client_has_counseled, 0, sizeof(room->client_has_counseled));
}

bool room_check_all_counseled(room_t *room) {
  printf("[DEBUG] Checking counseling status:\n");
  printf("[DEBUG]: num_clients: %d \n", room->num_clients);
  printf("[DEBUG]: room_agent_fd: %d\n", room->room_agent_fd);
  for (int i = 0; i < room->num_clients; i++) {
    printf("[DEBUG] client_fds[%d] = %d, has_counseled = %d\n", i,
           room->client_fds[i], room->client_has_counseled[i]);
    if (room->client_fds[i] == room->room_agent_fd) {
      printf("Skipping ai_agent\n");
      continue;
    }
    if (!room->client_has_counseled[i]) {
      printf("[DEBUG] Client at index %d has NOT counseled yet\n", i);
      return false;
    }
  }
  printf("[DEBUG] All humans have counseled!\n");
  return true;
}
