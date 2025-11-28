#ifndef CHAT_ROOM_H_
#define CHAT_ROOM_H_

#include "common.h"
#include "uthash.h"
#include <stdbool.h>
#include <time.h>

#define MAX_ROOM_NAME_SIZE 64
#define MAX_ROOM_CLIENTS 32
#define MAX_ROOMS 64
#define UUID_LEN 8

#define MAX_MESSAGE_LEN 512
#define MAX_MESSAGES_HISTORY 20

typedef struct {
  char username[64];
  char content[MAX_MESSAGE_LEN];
  time_t timestamp;
} message_t;

typedef struct {
  // This is where we will define the room strcuture
  char room_name[MAX_ROOM_NAME_SIZE]; // Room name/ID
  int num_clients;                  // Number of clients in the room at any time
  int client_fds[MAX_ROOM_CLIENTS]; // An array storing the client_fds in the
                                    // room
  int room_agent_fd;

  message_t messages[MAX_MESSAGES_HISTORY]; // This will be the message circular
                                            // buffer
  int num_messages;
  int message_index; // This is for modding for the index in the circular buffer

  bool client_has_counseled[MAX_ROOM_CLIENTS];
} room_t;

extern room_t global_rooms[MAX_ROOMS]; // Global variable for the rooms
extern int global_num_rooms;

room_t *room_find_or_create(
    const char *room_name); // if a room of this name exists, or if not, either
                            // way, we will return a pointer to the room

bool room_add_client_to_room(room_t *room, int client_fd);

bool room_remove_client_from_room(room_t *room, int client_fd);

bool room_assign_ai_agent(room_t *room, char *agent_name);

void room_add_message_to_history(room_t *room, const char *message,
                                 const char *username);

void room_reset_counseling_flags(room_t *room);

bool room_check_all_counseled(room_t *room);

// This struct is for assigning rooms' their ai_agent
struct agent_assign_args {
  room_t *room;
  char *agent_name;
};

#endif // !CHAT_ROOM_H_
