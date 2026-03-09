#include "../cpp_include/client_state.h"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace counsel {
    const FdStatus status_R = {.want_write = false, .want_read = true};
    const FdStatus status_W = {.want_write = true, .want_read = false};
    const FdStatus status_RW = {.want_write = true, .want_read = true};
    const FdStatus status_NORW = {.want_write = false, .want_read = false};

    ClientState::ClientState(Socket socket)
        : socket_(std::move(socket)), state_(ProcessingState::InitialAck),
          recv_buf_(1024), send_buf_(1024) {}

    int ClientState::fd() const { return socket_.fd(); }

    ProcessingState ClientState::state() const { return state_; }

    const std::string &ClientState::username() const { return username_; };

    Buffer &ClientState::recv_buf() { return recv_buf_; }

    Buffer &ClientState::send_buf() { return send_buf_; }

    FdStatus ClientState::on_connected() {
        const char *welcome_message = "Please enter your username: \n";
        send_buf_.append(reinterpret_cast<const uint8_t *>(welcome_message),
                         strlen(welcome_message));
        return status_W;
    }

    void ClientState::disconnect(int epoll_fd, const char *reason) {
        printf("Disconnected fd=%d reason=%s\n", fd(), reason);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd(), NULL);
        ::close(socket_.release());
    }

    FdStatus ClientState::on_readable(int epoll_fd) {
        if (state_ == ProcessingState::InitialAck || !send_buf_.empty())
            return status_W;
        uint8_t tmp[1024];
        int nbytes = ::recv(this->fd(), tmp, sizeof(tmp), 0);
        if (nbytes > 0) {
            recv_buf_.append(tmp, static_cast<size_t>(nbytes));
        } else if (nbytes == 0) {
            disconnect(epoll_fd, "eof");
            return status_NORW;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return status_R;
            else {
                disconnect(epoll_fd, "recv error");
                return status_NORW;
            }
        }
        return status_R;
    }

    FdStatus ClientState::on_writeable(int epoll_fd) {
        if (send_buf_.empty())
            return status_R;
        int nsent = ::send(this->fd(), this->send_buf_.data(),
                           this->send_buf().size(), 0);
        if (nsent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return status_W;
            else {
                disconnect(epoll_fd, "send error");
                return status_NORW;
            }
        } else if (static_cast<size_t>(nsent) < send_buf_.size()) {
            // This means that data was sent only partially
            send_buf_.consume(nsent);
            return status_W;
        } else {
            // Everythin was sent
            send_buf_.clear();
            if (state_ == ProcessingState::InitialAck)
                state_ = ProcessingState::WaitForMsg;
            return status_R;
        }
    }

    void ClientState::set_username(std::string username) {
        username_ = username;
    }

} // namespace counsel
