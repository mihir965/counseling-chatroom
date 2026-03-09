#include "../cpp_include/room.h"

namespace counsel {
    std::string generate_token() {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

        std::string token(6, ' ');
        for (char &c : token)
            c = charset[dist(rng)];
        return token;
    }

    Room::Room(RoomType type, int creator_fd_)
        : room_type_(type), admin_fd_(creator_fd_),
          room_token_(generate_token()) {
        // Assign max_capacity_ to room
        switch (type) {
            case RoomType::Solo: {
                max_capacity_ = 1;
                break;
            }

            case RoomType::Couples: {
                max_capacity_ = 2;
                break;
            }

            case RoomType::Group: {
                max_capacity_ = 10;
                break;
            }
        }

        // Add the creator as a joined client
        add_member(creator_fd_);
    }

    // Accessor functions
    const std::string &Room::token() const { return room_token_; }

    RoomType Room::type() const { return room_type_; }

    int Room::creator_fd() const { return admin_fd_; }

    const std::vector<int> &Room::members() const { return joined_clients_; }

    bool Room::is_full() const {
        return joined_clients_.size() >= max_capacity_;
    }

    // Modifiers
    bool Room::add_member(int fd) {
        if (is_full())
            return false;
        joined_clients_.push_back(fd);
        return true;
    }

    void Room::remove_member(int fd) {
        for (size_t i = 0; i < joined_clients_.size(); i++) {
            if (joined_clients_[i] == fd) {
                joined_clients_.erase(joined_clients_.begin() + i);
            }
        }
    }

    bool Room::is_member(int fd) const {
        for (auto &client : joined_clients_) {
            if (client == fd)
                return true;
        }
        return false;
    }

    bool Room::is_creator(int fd) const { return admin_fd_ == fd; }

} // namespace counsel
