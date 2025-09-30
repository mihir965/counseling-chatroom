#define _GNU_SOURCE

#include "../include/chat_room.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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
    new_room->num_clients = 0;
    return new_room;
  } else {
    /*
     * Let us also hit the python server to create the ai agent socket
     */
    printf("Making a curl request\n");
    CURL *curl;
    CURLcode res;
    char url[256];
    snprintf(url, sizeof url, "http://127.0.0.1:5000/get_agent?room=%s",
             room_name);

    curl = curl_easy_init();
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, url);

      res = curl_easy_perform(curl);

      if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
      curl_easy_cleanup(curl);
    } else {
      printf("Could not make the request\n");
    }

    printf("There were no rooms!\n");
    room_t *new_room = &global_rooms[0];
    global_num_rooms++;
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
