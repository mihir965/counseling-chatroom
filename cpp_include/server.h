#pragma once

#include "../cpp_include/nlohmann/json.hpp"
#include "client_state.h"
#include "room.h"
#include "socket.h"
#include <memory>
#include <unordered_map>
namespace counsel {
    class Server {
    private:
        Socket main_socket_;
        int epoll_fd_;
        std::unordered_map<int, std::unique_ptr<ClientState>> client_map_;
        std::unordered_map<std::string, std::unique_ptr<Room>> room_map_;

    public:
        explicit Server(const std::string &port, int backlog = 128);
        ~Server();

        // This is a singleton object
        Server(const Server &) = delete;
        Server &operator=(const Server &) = delete;

        // No moves either
        Server(Server &&other) = delete;
        Server &operator=(Server &&other) = delete;

        void run();

    private:
        void on_new_connection();
        void on_client_readable(int fd);
        void on_client_writeable(int fd);
        void parse_and_dispatch(ClientState &client);
        void handle_set_username(ClientState &client,
                                 const nlohmann::json &msg);
        void handle_create_room(ClientState &client, const nlohmann::json &msg);
        void handle_join_room(ClientState &client, const nlohmann::json &msg);
        void handle_message(ClientState &client, const nlohmann::json &msg);
        void handle_leave_room(ClientState &client, const nlohmann::json &msg);

        void send_response(ClientState &client, const nlohmann::json &msg);
    };
} // namespace counsel
