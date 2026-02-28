#include "../cpp_include/server.h"
#include "../cpp_include/server_utils.h"
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

namespace counsel {
    Server::Server(const std::string &port, int backlog)
        : main_socket_(create_listening_socket(port, backlog)),
          epoll_fd_(epoll_create1(0)) {
        if (main_socket_.fd() == -1) {
            fprintf(stderr, "error getting the listening socket\n");
        }

        main_socket_.set_non_blocking();

        // We need to tell epoll to waitch for incoming connections
        struct epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = main_socket_.fd();
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, main_socket_.fd(), &ev);
    }

    Server::~Server() {
        if (epoll_fd_ >= 0) {
            close(epoll_fd_);
        }
    }

    void Server::on_new_connection() {
        // This means that the listener socket recorded an event
        // meaning we have to accept a new peer
        printf("Peer is now connecting...\n");

        struct sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        Socket new_peer = main_socket_.accept(
            reinterpret_cast<sockaddr *>(&peer_addr), &peer_addr_len);

        if (new_peer.fd() < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            perror("accept");
            return;
        }

        // We should also set this new_peer to non_blocking and add it to the
        // events array of epoll
        new_peer.set_non_blocking();
        struct epoll_event event = {0};
        event.data.fd = new_peer.fd();

        // We also have to create the ClientState of this new peer
        ClientState new_peer_client_state(std::move(new_peer));

        // This will send welcome message, loads send_buf
        counsel::FdStatus new_peer_fd_status =
            new_peer_client_state.on_connected();

        // To store in the client_map_
        int fd{new_peer_client_state.fd()};
        client_map_[fd] =
            std::make_unique<ClientState>(std::move(new_peer_client_state));

        // Now based on the sataus of the new client
        if (new_peer_fd_status.want_read)
            event.events |= EPOLLIN;
        if (new_peer_fd_status.want_write)
            event.events |= EPOLLOUT;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0)
            perror("epoll_ctl EPOLL_CTL_ADD");
    }

    void Server::on_client_writeable(int fd) {
        auto it = client_map_.find(fd);
        if (it == client_map_.end())
            return;
        ClientState &peer_state = *it->second;
        counsel::FdStatus peer_status = peer_state.on_writeable(epoll_fd_);
        struct epoll_event event = {0};
        event.data.fd = peer_state.fd();
        if (peer_status.want_read)
            event.events |= EPOLLIN;
        if (peer_status.want_write)
            event.events |= EPOLLOUT;
        if (event.events == 0) {
            printf("socket %d closing \n", peer_state.fd());
            peer_state.disconnect(epoll_fd_, "no-interest");

            // We also erase the fd from the map
            client_map_.erase(fd);
        } else if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, peer_state.fd(),
                             &event) < 0) {
            perror("epoll_ctl EPOLL_CTL_MOD\n");
        }
    }

    void Server::on_client_readable(int fd) {
        // This means that we already have the client state in the hashmap for
        // client_map_
        auto it = client_map_.find(fd);
        if (it == client_map_.end())
            return;
        ClientState &peer_state = *it->second;
        counsel::FdStatus peer_status = peer_state.on_readable(epoll_fd_);
        struct epoll_event event = {0};
        event.data.fd = peer_state.fd();
        if (peer_status.want_read)
            event.events |= EPOLLIN;
        if (peer_status.want_write)
            event.events |= EPOLLOUT;
        if (event.events == 0) {
            printf("socket %d closing \n", peer_state.fd());
            peer_state.disconnect(epoll_fd_, "no-interest");

            // Erase the entry from map
            client_map_.erase(fd);
        } else if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, peer_state.fd(),
                             &event) < 0) {
            perror("epoll_ctl EPOLL_CTL_MOD\n");
        }
    }

    void Server::run() {
        int MAX_FDS{1024};
        struct epoll_event events[MAX_FDS];

        puts("server: waiting for connections....\n");

        while (1) {
            int nready{epoll_wait(epoll_fd_, events, MAX_FDS, -1)};
            for (int i = 0; i < nready; i++) {
                if (events[i].data.fd == main_socket_.fd()) {
                    on_new_connection();
                } else {
                    // This is where another observed file descriptor is
                    // recording an event meaning that either a write or read is
                    // happening
                    if (events[i].events & EPOLLIN) {
                        on_client_readable(events[i].data.fd);
                    } else if (events[i].events & EPOLLOUT)
                        on_client_writeable(events[i].data.fd);
                }
            }
        }
    }

} // namespace counsel
