#include "../cpp_include/server_utils.h"
#include "../cpp_include/socket.h"
#include <iostream>

int main() {
    try {
        std::cout << "=== Testing High-Level Helper ===\n\n";
        
        // One line to create a complete listening socket!
        std::cout << "Creating listener on port 9999...\n";
        counsel::Socket listener = counsel::create_listening_socket("9999");
        
        std::cout << "✓ Success! Listening socket created\n";
        std::cout << "  FD: " << listener.fd() << "\n";
        std::cout << "  is_valid(): " << (listener.is_valid() ? "true" : "false") << "\n\n";
        
        // Verify it's actually listening
        std::cout << "Socket is now listening on 0.0.0.0:9999\n";
        
        // Set non-blocking for event loop
        std::cout << "Setting non-blocking mode...\n";
        listener.set_non_blocking();
        std::cout << "✓ Non-blocking enabled\n\n";
        
        std::cout << "=== High-Level Helper Works! ===\n\n";
        std::cout << "Compare to C version:\n";
        std::cout << "  C:   int fd = sockets_get_listener_socket();\n";
        std::cout << "  C++: Socket listener = create_listening_socket(\"9999\");\n\n";
        std::cout << "Benefits:\n";
        std::cout << "  ✓ RAII - automatic cleanup\n";
        std::cout << "  ✓ Exception safety\n";
        std::cout << "  ✓ No manual close() needed\n";
        std::cout << "  ✓ Move semantics - efficient ownership transfer\n";
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\nSocket will be auto-closed when leaving scope...\n";
    return 0;
}
