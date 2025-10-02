#define _GNU_SOURCE

#include "../include/chat_room.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include "../include/comms.h"
#include "../include/peer.h"
#include "../include/threading.h"

int global_num_rooms = 0;
room_t global_rooms[MAX_ROOMS];

bool room_assign_ai_agent(room_t *room, char *agent_name){
    if(!room) return false;
    printf("The number of clinets in the room : (%d)", room->num_clients);
    for(int i=0; i<room->num_clients; i++){
        int client_to_check = room->client_fds[i];
        if(strcmp((char*)global_state[client_to_check].user_name, agent_name)==0){
            room->room_agent_fd = client_to_check;
            printf("Assigning (%d) as the ai_agent of the room: (%s)", client_to_check, room->room_name);
            return true;
        }
    }
    printf("Could not find the agent fd\n");
    return false;
}

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

    /*
     * First we need to get the agent_name for the room by first making a request to the python server
     */
    char* agent_name = comms_connect_agent(room_name);
    printf("The name of the agent that is connected to the room is: %s\n", agent_name);

    /* Now we have to create another thread for assigning the room the agent since it can be blocking until the python server doesn't connect to the c server */
    struct agent_assign_args* args = malloc(sizeof(*args));
    args->room = new_room;
    args->agent_name = agent_name;

    pthread_t tid;
    if(pthread_create(&tid, NULL, agent_assign_thread, args)!=0){
        perror("Failed to create agent assignment thread\n");
        free(args->agent_name);
        free(args);
    }else{
        pthread_detach(tid);
    }

    return new_room;
  } else {
      printf("There were no rooms!\n");
    room_t *new_room = &global_rooms[0];
    global_num_rooms++;
    strncpy(new_room->room_name, room_name, MAX_ROOM_NAME_SIZE);
    new_room->room_name[MAX_ROOM_NAME_SIZE - 1] = '\0';
    new_room->num_clients = 0; // We initialize it to 1 since the ai_agent is technically a cleint
    
    /*
     * Let us also hit the python server to create the ai agent socket
     */
    char* agent_name = comms_connect_agent(room_name);
    printf("The name of the agent that is connected to the room is: %s\n", agent_name);

    /* Now we have to create another thread for assigning the room the agent since it can be blocking until the python server doesn't connect to the c server */
    struct agent_assign_args* args = malloc(sizeof(*args));
    args->room = new_room;
    args->agent_name = agent_name;

    pthread_t tid;
    if(pthread_create(&tid, NULL, agent_assign_thread, args)!=0){
        perror("Failed to create agent assignment thread\n");
        free(args->agent_name);
        free(args);
    }else{
        pthread_detach(tid);
    }

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
    if (room->client_fds[i] == client_fd) {
      room->client_fds[i] = 0;
      return true;
    }
    i++;
  }
  return false;
}

