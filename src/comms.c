#define _GNU_SOURCE

#include "../include/comms.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

char *comms_connect_agent(const char *room_name, const char *uuid_str) {
  printf("Making curl request\n");
  CURL *curl;
  CURLcode res;

  /*
   * Construct the url for the python server that will serve the ai_agent
   */
  char url[256];
  snprintf(url, sizeof url, "http://127.0.0.1:5000/get_agent?room=%s&uuid=%s",
           room_name, uuid_str);

  /*
   * Let's now make the data structure that will store metadata related to the
   * agent that is going to connect to a room
   */
  struct MemoryStruct chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  /*
   * Init the curl session
   */
  curl = curl_easy_init();

  /*
   * Specify the url to get
   */
  curl_easy_setopt(curl, CURLOPT_URL, url);

  /* send all data to this function */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our chunk struct to the callback function */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  /* Don't really understand why this is done */
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  /* get the data */
  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
  } else {
    printf("%lu bytes retrieved\n", (unsigned long)chunk.size);
  }

  cJSON *json = cJSON_Parse(chunk.memory);

  char *agent_name =
      cJSON_GetObjectItemCaseSensitive(json, "agent_name")->valuestring;

  char *name = strdup(agent_name);

  /* Very important to clean up the memory struct and the cJSON object that is
   * created */
  curl_easy_cleanup(curl);
  free(chunk.memory);
  cJSON_Delete(json);

  return name;
}

char *comms_get_counselor_response(room_t *room) {
  if (!room)
    return NULL;

  CURL *curl;
  CURLcode res;

  /* Build the JSON payload with conversation history */
  cJSON *json = cJSON_CreateObject();
  cJSON *messages_array = cJSON_CreateArray();

  /* Add each message from histroy */
  int start_index =
      (room->num_messages < MAX_MESSAGES_HISTORY) ? 0 : room->message_index;
  for (int i = 0; i < room->num_messages; i++) {
    int idx = (start_index + i) % MAX_MESSAGES_HISTORY;

    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "username", room->messages[idx].username);
    cJSON_AddStringToObject(msg, "content", room->messages[idx].content);
    cJSON_AddNumberToObject(msg, "timestamp",
                            (double)room->messages[idx].timestamp);
    cJSON_AddItemToArray(messages_array, msg);
  }
  cJSON_AddItemToObject(json, "messages", messages_array);
  cJSON_AddStringToObject(json, "room_name", room->room_name);

  char *json_str = cJSON_Print(json);

  struct MemoryStruct chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  char *ai_response = NULL;

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL,
                     "http://127.0.0.1:5000/get_counselor_response");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      curl_slist_free_all(headers);
      free(json_str);
      cJSON_Delete(json);
      free(chunk.memory);
      return NULL;
    }

    /* Parse the response */
    cJSON *response_json = cJSON_Parse(chunk.memory);

    if (response_json) {
      cJSON *response_field =
          cJSON_GetObjectItemCaseSensitive(response_json, "response");
      if (response_field && cJSON_IsString(response_field)) {
        ai_response = strdup(response_field->valuestring);
      }
      cJSON_Delete(response_json);
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(chunk.memory);
  }

  free(json_str);
  cJSON_Delete(json);
  return ai_response;
}
