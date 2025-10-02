#ifndef _COMMS_H_
#define _COMMS_H_

#include <curl/curl.h>
#include<curl/easy.h>
#include <cjson/cJSON.h>
#include <sys/types.h>

char* comms_connect_agent(const char*);

//We need to be able to write the json data into a data structure that the server can read and use for understanding the room_name that the agent is a part of.
struct MemoryStruct{
    char* memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void* userp);

#endif // !_COMMS_H_
