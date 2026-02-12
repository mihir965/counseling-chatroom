#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
namespace counsel {

    // The buffer class is for creating a dynamic container to hold the bytes.
    // We need a position tracker for partial consumption and you need to track
    // where you left off.

    class Buffer {
        private:
            std::vector<uint8_t> data_; // The actual buffer. In C we had this
                                        // as a fixed size buffer
            size_t read_pos_; // For partial consumption (used in send buffer)

        public:
            // Constructor;
            Buffer();
            explicit Buffer(size_t initial_capacity); // Pre-allocate

            // Destructor, move, copy 
            ~Buffer() = default;

            // Move semantics (transfer ownership of vector)
            Buffer(Buffer&& other) noexcept = default;
            Buffer& operator=(Buffer&& other) noexcept = default;

            //Copy semantics - should we allow copy?
            //For Buffer, copying data is something useful (unlike Socket FD)
            Buffer(const Buffer&) = default;
            Buffer& operator=(const Buffer&) = default;

            // ======= Buffer Operations =========

            // We are overloading the append function so that we can pass an
            // array of chars or a vector or even a single 8bit char

            void append(const uint8_t* data, size_t len);
            void append(const std::vector<uint8_t>& data);
            void append(uint8_t byte);

            // Consume n byte from front (after sending)
            // This will need to update the read_pos pointer
            void consume(size_t n);

            // Get pointer to the readabl data
            const uint8_t* data() const;

            // Get number of readable data 
            size_t size() const; 

            // Check if buffer is empty
            bool empty() const;

            // Clear all the data 
            void clear();

            // Get allocated capacity 
            size_t capacity() const;

            // Pre Allocate Space (this is some optimization)
            void reserve(size_t n);

            // Find first occurance of byte - for commands
            std::optional<size_t> find(uint8_t byte) const;

            // Extract up to and including the delimiter
            std::optional<std::vector<uint8_t>> extract_until(uint8_t delimiter);
    };
}
