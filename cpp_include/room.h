#pragma once
#include <random>
#include <string>
#include <vector>

namespace counsel {
    std::string generate_token();

    enum class RoomType { Solo, Couples, Group };

    class Room {
        // We need things like the token for the room, the type of the room and
        // the max capacity We need the creator id so that admin privileges can
        // be assigned
    private:
        std::string room_token_;
        RoomType room_type_;
        int max_capacity_;
        // for the creator id, we will maintain the admin_fd_ and then a list of
        // the fds of clients that have joined and can continually join the room
        int admin_fd_;
        std::vector<int> joined_clients_;
        std::string room_name_;

    public:
        Room(RoomType type, int creator_fd_);
        ~Room() = default;

        // Cannot allow a copy constructor, we cannot afford to have two copies
        // of the same room
        Room(const Room &) = delete;
        Room &operator=(const Room &) = delete;

        // Move constructors are also being deleted since we are not going to be
        // creating Rooms from other functions, moving the temporary room into
        // the constructor
        Room(Room &&other) noexcept = delete;
        Room &operator=(Room &&other) noexcept = delete;

        // Accessors
        const std::string &
        token() const; // Return the value of the main_token_ of this room
        RoomType type() const;
        int creator_fd() const;
        const std::vector<int> &members() const;
        bool is_full() const;

        // Modifiers
        bool add_member(int fd); // false if room is is_full
        void remove_member(int fd);
        bool is_member(int fd) const;
        bool is_creator(int fd) const;
        void name_room(std::string name);
    };
}; // namespace counsel
