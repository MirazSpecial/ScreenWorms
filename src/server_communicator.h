#ifndef PROJEKT2_SERVER_COMMUNICATOR_H
#define PROJEKT2_SERVER_COMMUNICATOR_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <utility>
#include <vector>
#include <algorithm>

#include "consts.h"
#include "utils.h"
#include "events.h"

class ServerCommunicator {

private:
    class ClientData {

    public:
        uint64_t session_id;
        uint64_t last_message_time;
        std::string player_name;
        bool want_to_play = false;
        struct sockaddr_in client_address{};
        uint8_t last_turn_direction; // To remember initial turn direction
        uint8_t player_number = CLIENTS_MAX_NUMBER; // Number in game_state


        explicit ClientData(uint64_t session_id, uint64_t last_message_time,
                            std::string player_name, uint8_t last_turn_direction)
                    : session_id(session_id), last_message_time(last_message_time),
                      player_name(std::move(player_name)),
                      last_turn_direction(last_turn_direction){
            if (last_turn_direction != FORWARD)
                want_to_play = true;
        }

        bool is_active() const { // Check if player send anything in last 2 seconds
            return (get_time() - last_message_time <= 2000000);
        }

        bool operator<(const ClientData &ob) const {
            // After sorting vector we want empty name players to be at the end
            if (player_name.empty())
                return false;
            if (ob.player_name.empty())
                return true;
            return player_name < ob.player_name;
        }

    };

    /* Communication fields */
    uint16_t port_num;
    uint8_t buffer_w[MAX_SERVER_DATAGRAM_SIZE]{};
    uint8_t buffer_r[MAX_CLIENT_DATAGRAM_SIZE]{};
    uint16_t buffer_pos = 0;
    int sock = 0;

    /* Last received message information */
    uint64_t session_id{};
    uint8_t turn_direction{};
    uint32_t next_expected_event_no{};
    std::string player_name;
    struct sockaddr_in client_address{};

public:

    /* Client information fields */
    std::vector<ClientData> client_data;

    explicit ServerCommunicator(uint16_t port_num): port_num(port_num) {
        init_socket();
    }

    void parse_message(const std::vector<Event>& events, uint32_t game_id) {
        if (receive_message() == EMPTY)
            return; // No message sent

        for (auto client : client_data) {
            if (client.client_address.sin_port == client_address.sin_port &&
                client.client_address.sin_addr.s_addr ==
                       client_address.sin_addr.s_addr) {

                if (client.session_id == session_id) {
                    /* Update client */
                    client.last_message_time = get_time();
                    client.last_turn_direction = turn_direction;
                    if (turn_direction != FORWARD)
                        client.want_to_play = true;
                    send_events(events, next_expected_event_no, game_id, client);
                    return;
                }
                else if (client.session_id < session_id) {
                    /* Reset this client (disconnect him from game to) */
                    client.last_message_time = get_time();
                    client.last_turn_direction = turn_direction;
                    client.session_id = session_id;

                    client.player_name = player_name;
                    client.player_number = CLIENTS_MAX_NUMBER; // It's a new client
                    if (turn_direction != FORWARD)
                        client.want_to_play = true;
                    send_events(events, next_expected_event_no, game_id, client);
                    return;
                }
                /* When new session_id is lower then previous do nothing */
            }
        }

        /* Message was sent from unknown socket */

        for (const auto& client : client_data) {
            if (client.session_id == session_id) {
                /* New socket but existing session_id - do nothing */
                return;
            }
        }

        /* Socket and session_id are new */

        if (client_data.size() == CLIENTS_MAX_NUMBER)
            return; /* Client limit */

        client_data.emplace_back(session_id, get_time(), player_name, turn_direction);
        client_data.back().client_address.sin_port = client_address.sin_port;
        client_data.back().client_address.sin_addr.s_addr = client_address.sin_addr.s_addr;
        client_data.back().client_address.sin_family = client_address.sin_family;
    }

    void set_not_ready() {
        for (auto& client : client_data)
            client.want_to_play = false;
    }

    bool ready_to_play() const {
        uint8_t ready_players = 0;
        for (const auto& client : client_data) {
            if (!client.player_name.empty()) {
                if (!client.want_to_play) {
                    /* Non-empty name player who didn't send first move */
                    return false;
                }
                ready_players++;
            }
        }
        /* Check if at least 2 ready players */
        return ready_players >= 2;
    }

    void remove_inactive_clients() {
        client_data.erase(std::remove_if(client_data.begin(),
                                         client_data.end(),
                                         [](auto client){ return !client.is_active(); }),
                          client_data.end());
    }

    uint8_t get_new_turn_direction(uint8_t player_number) const {
        for (const auto& client : client_data)
            if (client.player_number == player_number)
                return client.last_turn_direction;
        return NO_CHANGES; // When client disconnected
    }

    void send_events_to_everyone(const std::vector<Event>& events, uint32_t event_no,
                                 uint32_t game_id) {
        for (const auto& client : client_data)
            send_events(events, event_no, game_id, client);
    }

    void send_events(const std::vector<Event>& events, uint32_t event_no,
                     uint32_t game_id, const ClientData& client) {
        if (event_no == events.size())
            return; // No events to send
        bool first_in_datagram = true;

        for (; event_no < events.size(); ++event_no) {
            if (first_in_datagram)
                add_header_to_buffer(game_id);

            if (buffer_pos + events[event_no].get_length() + 8 > MAX_SERVER_DATAGRAM_SIZE) {
                /* New event cannot be added, send datagram */
                send_and_clear_buffer(&client.client_address);
                event_no -= 1; // To parse this event again
                first_in_datagram = true;
            }
            else {
                /* New event can be added */
                const auto& event = events[event_no];
                add_event_to_buffer(event);
                first_in_datagram = false;
            }
        }
        send_and_clear_buffer(&client.client_address);
    }

private:
    uint8_t receive_message() {
        auto rcva_len = (socklen_t) sizeof(client_address);
        ssize_t len = recvfrom(sock, buffer_r, sizeof(buffer_r), 0,
                               (struct sockaddr *) &client_address, &rcva_len);
        if (len == -1)
            return EMPTY; // Empty message
        if (len < 13)
            return EMPTY; // Invalid message

        session_id = be64toh(*(uint64_t*)buffer_r);
        turn_direction = buffer_r[8];
        next_expected_event_no = ntohl(*(uint32_t*)(buffer_r + 9));
        player_name.clear();
        for (int i = 13; i < len; ++i)
            player_name.push_back(buffer_r[i]);

        if (turn_direction > LEFT)
            return EMPTY;
        return RECEIVED;
    }

    void add_header_to_buffer(uint32_t game_id) {
        add_uint32_to_buffer(game_id);
    }

    void add_event_to_buffer(const Event& event) {
        uint32_t event_pos = buffer_pos;
        add_uint32_to_buffer(event.get_length());
        add_uint32_to_buffer(event.event_no);
        add_byte_to_buffer(event.event_type);

        switch (event.event_type) {
            case NEW_GAME: {
                add_uint32_to_buffer(event.x);
                add_uint32_to_buffer(event.y);
                for (const auto &name : event.player_names)
                    add_string_to_buffer(name);
                break;
            }
            case PIXEL: {
                add_byte_to_buffer(event.player_number);
                add_uint32_to_buffer(event.x);
                add_uint32_to_buffer(event.y);
                break;
            }
            case PLAYER_ELIMINATED: {
                add_byte_to_buffer(event.player_number);
                break;
            }
        }

        add_uint32_to_buffer(generate_crc32(buffer_w + event_pos,
                                            event.get_length() + 4));
    }

    void send_and_clear_buffer(const struct sockaddr_in* client_address_ptr) {
        ssize_t snd_len = (socklen_t) sizeof(*client_address_ptr);
        if (sendto(sock, buffer_w, (size_t) buffer_pos, 0,
                   (struct sockaddr*) client_address_ptr, snd_len) != buffer_pos) {
            // Just report error, no need to stop program
            std::cerr << "Sending buffer to " << client_address_ptr->sin_addr.s_addr
                      << ":" << client_address_ptr->sin_port << " failed!" << std::endl;
        }
        buffer_pos = 0;
    }

    void init_socket() {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            report_fail("Socket initialization failed!");

        int val = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        fcntl(sock, F_SETFL, O_NONBLOCK); // Setting socket to nonblock

        struct sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        server_address.sin_port = htons(port_num);

        if (bind(sock, (struct sockaddr *) &server_address,
                 (socklen_t) sizeof(server_address)) < 0)
            report_fail("Binding failed!");
    }

    void add_uint32_to_buffer(uint32_t val) {
        *(uint32_t*)(&buffer_w[buffer_pos]) = htonl(val);
        buffer_pos += sizeof(val);
    }

    void add_byte_to_buffer(uint8_t val) {
        buffer_w[buffer_pos] = val;
        buffer_pos += sizeof(val);
    }

    void add_string_to_buffer(const std::string& val) {
        for (char c : val)
            add_byte_to_buffer(c);
        add_byte_to_buffer('\0');
    }

    static void report_fail(const char* message) {
        std::cerr << message << std::endl;
        exit(1);
    }

};

#endif //PROJEKT2_SERVER_COMMUNICATOR_H
