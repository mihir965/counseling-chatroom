#include "../cpp_include/buffer.h"
#include "../cpp_include/client_state.h"
#include "../cpp_include/server.h"
#include "../cpp_include/server_utils.h"
#include "../cpp_include/socket.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

void check(bool condition, const char *test_name) {
    if (condition) {
        std::cout << "  [PASS] " << test_name << "\n";
    } else {
        std::cout << "  [FAIL] " << test_name << "\n";
    }
}

int main() {
    try {
        std::cout << "=== Server Class Tests ===\n\n";
        // ==== Test 1: Default Constructor =====

        counsel::Server main_server("8080", 1024);
        main_server.run();
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nSocket will be auto-closed when leaving scope...\n";
    return 0;
}
