#ifndef SOCK_U_H_
#define SOCK_U_H_

#include "peer.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>


static int get_listener_socket(void);

static int set_nonblocking(int fd);

#endif // !SOCK_U_H_
