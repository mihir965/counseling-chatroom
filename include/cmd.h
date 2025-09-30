#ifndef CMD_H_
#define CMD_H_

#include "chat_room.h"
#include "peer.h"

void cmd_join_or_create_room(peer_state_t *, int sock_fd);

void cmd_leave_room(peer_state_t *, int sock_fd);

int cmd_list_rooms(peer_state_t *, int sock_fd);

#endif // !CMD_H_
#define CMD_H_
