#ifndef UTILS_H_
#define UTILS_H_

#include "chat_room.h"
#include "uthash.h"
#define UUID_LEN 37

typedef struct {
  char token[UUID_LEN];
  room_t *room;
  UT_hash_handle hh;
} token_map;

extern token_map *map;

token_map* find_room(char* uuid);

bool find_and_assign_agent(char* uuid, int agent_socket);

#endif
