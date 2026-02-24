#include "../cpp_include/server_utils.h"
#include "../cpp_include/socket.h"
#include "../cpp_include/buffer.h"
#include <cstdint>
#include <iostream>

void check(bool condition, const char* test_name){
    if(condition){
        std::cout << "  [PASS] " << test_name << "\n";
    }else{
        std::cout << "  [FAIL] " << test_name << "\n";
    }
}

int main() {
    try {
        std::cout << "=== Buffer Class Tests ===\n\n";
        // ==== Test 1: Default Constructor =====
        {
            counsel::Buffer b;
            check(b.empty(), "default ctor: empty");
            check(b.size() == 0, "default ctor: size == 0");
        }

        // ===== Test 2: Capacity Constructor ====
        {
            counsel::Buffer b(10);
            check(b.size() == 0, "default ctor: b.size() == 0");
        }

        // ==== Test 3: append (raw pointer) ======
        {
            const char* msg = "hello";
            counsel::Buffer b;
            b.append(reinterpret_cast<const uint8_t*>(msg), 5);
            check(b.size() == 5, "append(ptr): size == 5");
            check(b.data()[0] == 'h', "append(ptr): first byte is 'h'");
            check(b.data()[4] == 'o', "append(ptr): last byte is 'o'");
        }

        // ====== Test 4: append(const std::vector<uint8_t>&) =====
        {
            counsel::Buffer b;
            std::vector<uint8_t> vec = {'a', 'b', 'c'};
            b.append(vec);
            check(b.size() == 3, "append(vec): size == 3");
            check(b.data()[0] == 'a', "append(vec): first byte is 'a'");
            check(b.data()[2] == 'c', "append(vec): last byte is 'c'");
        }

        {
            counsel::Buffer b;
            b.append(static_cast<uint8_t>('X'));
            check(b.size() == 1, "append(byte): size == 1");
            check(b.data()[0] == 'X', "append(byte): byte is 'X'");
        }

        // ======== Test 5: consume(size_t n) ==========
        {
            counsel::Buffer b;
            const char* msg = "hello";
            b.append(reinterpret_cast<const uint8_t*>(msg), 5);
            b.consume(2);
            check(b.size() == 3, "consume(2): size == 3");
            check(b.data()[0] == 'l', "consume(2): first byte = 'l'");
        }

        // ============ Test 6: find(uint8_t byte) =======
        {
            counsel::Buffer b;
            const char* msg = "hello @drcrocs22";
            b.append(reinterpret_cast<const uint8_t*>(msg), 16);
            check(b.find('@') == 6, "find('@'): size_t == 6");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\nSocket will be auto-closed when leaving scope...\n";
    return 0;
}
