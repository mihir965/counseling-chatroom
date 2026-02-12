#include "../cpp_include/buffer.h"
#include <algorithm>
#include <cstdint>
#include <optional>

namespace counsel {
    
    Buffer::Buffer() {
        // We are just initializing it to 0
        read_pos_ = 0;
    }

    Buffer::Buffer(size_t initial_capacity){
        data_.reserve(initial_capacity);
        read_pos_ = 0;
    }

    void Buffer::append(const uint8_t* data, size_t len){
        for(size_t i = 0; i < len; i++){
            data_.push_back(data[i]);
        }
    }

    void Buffer::append(const std::vector<uint8_t>& data){
        data_.insert(data_.end(), data.begin(), data.end());
    }

    void Buffer::append(uint8_t byte){
        data_.push_back(byte);
    }

    void Buffer::consume(size_t n){
        read_pos_ += n;
    }

    const uint8_t* Buffer::data() const {
        return &(data_[read_pos_]);
    }

    size_t Buffer::size() const {
        return data_.size() - read_pos_;
    }

    bool Buffer::empty() const {
        return size() == 0;
    }

    void Buffer::clear() {
        data_.erase(data_.begin(), data_.end());
        read_pos_ = 0;
    }

    size_t Buffer::capacity() const {
        return data_.capacity();
    }

    void Buffer::reserve(size_t n){
        data_.reserve(n);
    }

    std::optional<size_t> Buffer::find(uint8_t byte) const {
        auto it = std::find(data_.begin() + read_pos_, data_.end(), byte);
        if(it == data_.end()) return std::nullopt;
        return it - (data_.begin() + read_pos_);
    }

    std::optional<std::vector<uint8_t>> Buffer::extract_until(uint8_t delimiter){
        // Find the delimiter
        auto pos{find(delimiter)};

        // If not found, return nullptr
        if(!pos.has_value()) return std::nullopt;

        // Extract data 
        size_t extract_len = *pos + 1; // This helps include delimiter

        std::vector<uint8_t> result(data_.begin() + read_pos_, data_.begin() + read_pos_ +extract_len);

        // Consume the extracted bytes
        consume(extract_len);

        return result;
    }


}
