#pragma once
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

// Keeping code organized with namespaces
namespace counsel {
    class Socket {
    private:
        int fd_;
        // This is explicit since we need to make sure that implicit
        // conversion does not happen this way - Socket s = 5
        // This is good practice for single argument constructors
        explicit Socket(int fd);

    public:
        // This is a wrapper function over the sys call of socket()
        Socket(int domain, int type, int protocol = 0);
        ~Socket();

        // Delete copy (preventing double-close bug)
        Socket(const Socket &) = delete; // Says that don't allow this operation
        Socket &operator=(const Socket &) = delete; //

        // Move constructor and assignment
        Socket(
            Socket &&other) noexcept; // This allows for moving the socket
                                      // without actually creating another copy
                                      // This is allowing for something like
                                      // Socket s1(create_listening_socket());
        Socket &operator=(
            Socket
                &&other) noexcept; // This is allowing for something like Socket
                                   // s1 (A socket that already exists, to be
                                   // assigned the temp (rvalue) - s1 =
                                   // create_listening_socket) Since the copy
                                   // constructors are simple operations, we
                                   // tell the compiler that this function is
                                   // not going to throw any exceptions

        int fd() const; // The const here signifies that the function is not
                        // allowed to modify the object
        void bind(const sockaddr *addr, socklen_t addrlen);
        void listen(int backlog);
        Socket accept(sockaddr *addr, socklen_t *addrlen);
        void set_non_blocking();
        bool is_valid() const;
        int release();
    };
} // namespace counsel
