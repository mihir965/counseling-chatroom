#include "../cpp_include/server_utils.h"
#include <netdb.h>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <system_error>

namespace counsel {

    // Modern C++
    AddrInfoGuard::AddrInfoGuard(addrinfo* ai) : ai_(ai){};

    // Simple destructor. This will run whenever the object goes out of scope
    AddrInfoGuard::~AddrInfoGuard(){
        if(ai_){
            freeaddrinfo(ai_);
        }
    }

    AddrInfoGuard::AddrInfoGuard(AddrInfoGuard&& other) noexcept : ai_(other.ai_) {
        other.ai_ = nullptr;
    }

    AddrInfoGuard& AddrInfoGuard::operator=(AddrInfoGuard&& other) noexcept {
        if(this != &other){
            if(ai_) freeaddrinfo(ai_);
            ai_ = other.ai_;
            other.ai_ = nullptr;
        }
        return *this;
    }

    addrinfo* AddrInfoGuard::get() const { return ai_; }

    // Lets create the create_listening_socket function
    Socket create_listening_socket(const std::string &port, int backlog){
        //We first setup hints for the getaddrinfo functions
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        // Since we create the listening socket on our own port therefore there
        // is no need to put any IP address and NULL is find. Once we get the
        // Linked List of resolved addresses, we iterate over them
        addrinfo* resolvedList = nullptr;
        int resolutionResult{getaddrinfo(nullptr, port.c_str(), &hints, &resolvedList)};
        if(resolutionResult != 0){
            throw std::runtime_error("getaddrinfo " + std::string(gai_strerror(resolutionResult)));
        }

        AddrInfoGuard guard(resolvedList); // This will allow us to
                                           // automatically clean up
                                           // resolvedList, when we go out of
                                           // scope

        for(addrinfo* p = resolvedList; p != nullptr; p = p->ai_next){
            try {
                Socket sock(p->ai_family, p->ai_socktype, p->ai_protocol);
                sock.bind(p->ai_addr, p->ai_addrlen);
                sock.listen(backlog);
                return sock;
            }catch (const std::system_error &e){
                continue;
            }
        }

        throw std::runtime_error("Failed to create listener on port" + port);
    }
};
