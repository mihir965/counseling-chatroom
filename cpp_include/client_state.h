#pragma once
#include "buffer.h"
#include "socket.h"
#include <string>

namespace counsel {
    enum class ProcessingState { InitialAck, WaitForMsg, InMsg, InCmd };

    struct FdStatus {
        bool want_write;
        bool want_read;
    };

    class ClientState {
    private:
        Socket socket_;
        std::string username_;
        Buffer recv_buf_;
        Buffer send_buf_;
        ProcessingState state_;
        // We also need the rooms (not implemented yet)

    public:
        // We use the explicit keyword so that we cannot do an implicit
        // conversion like - ClientState = Socket s?
        explicit ClientState(Socket socket);
        ~ClientState() = default;

        ClientState(const ClientState &) = delete;
        ClientState &operator=(const ClientState &) = delete;

        ClientState(ClientState &&other) noexcept = default;
        ClientState &operator=(ClientState &&other) noexcept = default;

        int fd() const;
        ProcessingState state() const;
        const std::string &username() const;
        Buffer &recv_buf();
        Buffer &send_buf();

        FdStatus on_connected();
        FdStatus on_readable(int epoll_fd);
        FdStatus on_writeable(int epoll_fd);
        void disconnect(int epoll_fd, const char *reason);

        // Setters
        void set_username(std::string username);
    };
} // namespace counsel
