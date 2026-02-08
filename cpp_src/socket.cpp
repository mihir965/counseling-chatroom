#include "../cpp_include/socket.h"
#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

namespace counsel {

    //Private Constructor (for accept)
    Socket::Socket(int fd) : fd_(fd) {

    }

    Socket::Socket(int domain, int type, int protocol){
        fd_ = socket(domain, type, protocol);
        if(fd_ < 0){
            throw std::system_error(errno, std::system_category(), "socket(), failed");
        }
    }

    Socket::~Socket(){
        // What the below logic allows us to do is that when we do the move
        // contructor, we assign the previous temp fd to be -1 and this way when
        // out of scope it is not destroyed, thereby destroying the fd that we
        // actually want to use.
        if(fd_ >= 0){
            close(fd_);
        }
    }

    Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
        // What we are going to do here is allow the user to be able to
        // initialize a socket using an rvalue of a temp socket being returned
        // from a function
        other.fd_ = -1;
    }

    // Move Assignment with operator - This also ensures that if there already
    // was an open file descriptor being referenced by let's say s1, then we
    // first close that.
    Socket& Socket::operator=(Socket&& other) noexcept {
        if(this == &other) return *this;
        if(fd_ >= 0) close(fd_);
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    int Socket::fd() const {
        return fd_;
    }

    void Socket::bind(const sockaddr* addr, socklen_t addrlen){
        if(::bind(fd_, addr, addrlen) < 0){
            throw std::system_error(errno, std::system_category(), "port already in use!");
        }
    }

    void Socket::listen(int backlog){
        if(::listen(fd_, backlog) == -1){
            throw std::system_error(errno, std::system_category(), "error listening");
        }
    }

    void Socket::set_non_blocking(){
        int flags = fcntl(fd_, F_GETFL, 0);
        if(flags == -1) throw std::system_error(errno, std::system_category(), "error while setting to non-blocking");
        if(fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) throw std::system_error(errno, std::system_category(), "error");
    }

    bool Socket::is_valid() const {
        return fd_ >= 0;
    }

    int Socket::release(){
        int old_fd{fd_};
        fd_ = -1;
        return old_fd;
    }

    Socket Socket::accept(sockaddr* addr, socklen_t* addrlen){
        int new_fd{::accept(fd_, addr, addrlen)};
        if(new_fd == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                return Socket(-1);
            }
            throw std::system_error(errno, std::system_category(), "error accepting client");
        }
        return Socket(new_fd); // This will invoke the private constructor
    }

}
