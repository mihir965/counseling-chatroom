#include "../cpp_include/server_utils.h"
#include "../cpp_include/socket.h"
#include <iostream>

int main() {
    try {
        std::cout << "=== Buffer Class Tests ===\n\n";


       
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\nSocket will be auto-closed when leaving scope...\n";
    return 0;
}
