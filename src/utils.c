#include "../include/utils.h"

token_map* find_room(char* uuid){
    token_map *s;
    HASH_FIND_STR(map, uuid, s);
    return s;
}

bool find_and_assign_agent(char *uuid, int agent_socket){
    token_map* room_struct = find_room(uuid);
    if(room_struct){
        room_struct->room->room_agent_fd = agent_socket;
        return true;
    }
    return false;
}
