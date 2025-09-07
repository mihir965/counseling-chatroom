#ifndef SOCKETS_H_
#define SOCKETS_H_

#include "common.h"

int sockets_get_listener_socket(void);

int sockets_set_non_blocking(int socket_fd);

#endif // !SOCKETS_H_
