#pragma once

#include "socket.h"
#include <netdb.h>
#include <string>
namespace counsel {
    Socket create_listening_socket(const std::string& port, int backlog = 128);

    class AddrInfoGuard {
        private:
            addrinfo* ai_;

        public:
            // This is safeguarding from constructor being called by any random
            // addrinfo object
            explicit AddrInfoGuard(addrinfo* ai);

            // Destructor
            ~AddrInfoGuard();

            // Not allowing for a copy constructor. This can cause the addrinfo
            // of two different objects pointing to the same value to be deleted
            // simultaneously
            AddrInfoGuard(const AddrInfoGuard&) = delete;

            // Not allowing users to create objects by assigning the reference
            // of a created object
            AddrInfoGuard& operator= (const AddrInfoGuard&) = delete;

            // We will allow objects to be created by passing in rvalues of
            // temporary objects created by initializer functions
            AddrInfoGuard(AddrInfoGuard&& other) noexcept;

            // Also allow for assigning temp rvalues of temp objects to already
            // created objects, we will need to make sure to remove the previous
            // values though
            AddrInfoGuard& operator= (AddrInfoGuard&& other) noexcept;

            // Classic getter function
            addrinfo* get() const;

    };
};
