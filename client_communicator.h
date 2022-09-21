#ifndef PROJEKT2_CLIENT_COMMUNICATOR_H
#define PROJEKT2_CLIENT_COMMUNICATOR_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <utility>
#include <endian.h>
#include <fcntl.h>
#include <vector>
#include <string>

#include "consts.h"
#include "client_options.h"
#include "utils.h"

class ClientCommunicator {

    const ClientOptions client_options;
    const uint64_t session_id;
    uint8_t buffer_w[MAX_CLIENT_DATAGRAM_SIZE]{};
    uint8_t buffer_r[MAX_SERVER_DATAGRAM_SIZE]{};
    uint8_t gui_buffer[2 * MAX_SERVER_DATAGRAM_SIZE]{};
    uint16_t gui_buffer_pos{};
    int sock{}, gui_sock{};
    struct sockaddr_in srvr_address{};

    /* Current game info */
    uint32_t curr_game_id = 0;
    uint32_t next_expected_event_no = 0;
    uint32_t maxx{}, maxy{};
    std::vector<std::string> players_names;
    uint8_t turn_direction{};

    /* Last received message information */
    uint32_t game_id{};
    uint8_t event_type{};
    uint32_t event_no{};
    uint32_t x{}, y{};
    std::vector<std::string> names;
    uint8_t player_number{};
    bool crc32_valid{};


public:
    explicit ClientCommunicator(ClientOptions client_options, uint64_t session_id)
                    : client_options(std::move(client_options)), session_id(session_id) {
        init_server_connection();
        init_gui_server_connection();
    }

    void message_server() {
        *(uint64_t*)buffer_w = htobe64(session_id);
        buffer_w[8] = turn_direction;
        *(uint32_t*)(buffer_w + 9) = htonl(next_expected_event_no);
        for (int i = 0; i < client_options.player_name.size(); ++i)
            buffer_w[13 + i] = client_options.player_name[i];

        size_t buffer_len = 13 + client_options.player_name.size();
        ssize_t rcva_len = (socklen_t) sizeof(srvr_address);

        if (sendto(sock, buffer_w, buffer_len, 0,(struct sockaddr*) &srvr_address,
                   rcva_len) != buffer_len) {
            std::cerr << "Sending buffer to " << srvr_address.sin_addr.s_addr
                      << ":" << srvr_address.sin_port << " failed!" << std::endl;
            report_fail("Sending buffer failed!");
        }
    }

    void parse_message() {
        auto rcva_len = (socklen_t) sizeof(srvr_address);
        ssize_t len = recvfrom(sock, buffer_r, sizeof(buffer_r), 0,
                               (struct sockaddr *) &srvr_address, &rcva_len);
        if (len == -1)
            return;

        /* Message received needs to be parsed. Random bytes send
         * (without assigned structure) generate undefined behavior */
        game_id = ntohl(*(uint32_t*)buffer_r);
        uint32_t parsed_len = 4;

        while (len > parsed_len) {
            parsed_len = parse_event(parsed_len);

            if (!crc32_valid)
                return; // End parsing this datagram
            if (event_type > GAME_OVER)
                continue;

            /* Known type proper control sum */

            switch (event_type) {
                case NEW_GAME:
                    if (x >= MAX_SCREEN_WIDTH || y >= MAX_SCREEN_HEIGHT)
                        report_fail("[MESSAGE ERROR] screen size too big");
                    for (const auto& name : names)
                        if (name.empty() || name.size() > MAX_NAME_LENGTH)
                            report_fail("[MESSAGE ERROR] name not valid");
                    init_new_game();
                    message_gui_new_game();
                    break;
                case PIXEL:
                    if (game_id != curr_game_id)
                        report_fail("[MESSAGE ERROR] wrong game id");
                    if (player_number >= players_names.size())
                        report_fail("[MESSAGE ERROR] player number too high");
                    if (x >= maxx || y >= maxy)
                        report_fail("[MESSAGE ERROR] Pixel does not exist");
                    message_gui_pixel();
                    break;
                case PLAYER_ELIMINATED:
                    if (game_id != curr_game_id)
                        report_fail("[MESSAGE ERROR] wrong game id");
                    if (player_number >= players_names.size())
                        report_fail("[MESSAGE ERROR] player number too high");
                    message_gui_player_eliminated();
                    break;
                case GAME_OVER:
                    if (game_id != curr_game_id)
                        report_fail("[MESSAGE ERROR] wrong game id");
                    break;
            }
            next_expected_event_no = event_no + 1;
        }

    }

    void parse_gui_message() {
        ssize_t rcv_len = read(gui_sock, gui_buffer, sizeof(gui_buffer) - 1);
        if (rcv_len < 0)
            return;
        gui_buffer[rcv_len] = '\0';

        if (!strcmp((const char*)gui_buffer, "LEFT_KEY_DOWN\n"))
            turn_direction = LEFT;
        if (!strcmp((const char*)gui_buffer, "RIGHT_KEY_DOWN\n"))
            turn_direction = RIGHT;
        if (!strcmp((const char*)gui_buffer, "LEFT_KEY_UP\n"))
            turn_direction = FORWARD;
        if (!strcmp((const char*)gui_buffer, "RIGHT_KEY_UP\n"))
            turn_direction = FORWARD;

    }

private:

    void message_gui_new_game() {
        std::string gui_message = "NEW_GAME " + std::to_string(maxx) + " " +
                                  std::to_string(maxy);
        for (const auto& name : players_names)
            gui_message += (" " + name);
        send_to_gui(gui_message + '\n');
    }

    void message_gui_pixel() {
        send_to_gui("PIXEL " + std::to_string(x) + " " + std::to_string(y)
                    + ' ' + players_names[player_number] + '\n');
    }

    void message_gui_player_eliminated() {
        send_to_gui("PLAYER_ELIMINATED " + players_names[player_number] + '\n');
    }

    void init_new_game() {
        curr_game_id = game_id;
        players_names = names;
        maxx = x;
        maxy = y;
    }

    void send_to_gui(const std::string& gui_message) {
        gui_buffer_pos = 0;
        for (char c : gui_message) {
            gui_buffer[gui_buffer_pos] = c;
            gui_buffer_pos++;
        }
        if (write(gui_sock, gui_buffer, gui_buffer_pos) != gui_buffer_pos)
            report_fail("Error while messaging gui server");
    }

    uint32_t parse_event(uint32_t parsed_len) {
        uint32_t event_pos = parsed_len;
        uint32_t event_len = ntohl(*(uint32_t*)(buffer_r + parsed_len));
        event_no = ntohl(*(uint32_t*)(buffer_r + parsed_len + 4));
        event_type = *(uint8_t *)(buffer_r + parsed_len + 8);
        parsed_len += 9;
        switch (event_type) {
            case NEW_GAME:
                x =  ntohl(*(uint32_t*)(buffer_r + parsed_len));
                y =  ntohl(*(uint32_t*)(buffer_r + parsed_len + 4));
                parsed_len += 8;
                break;
            case PIXEL:
                player_number = *(uint8_t *)(buffer_r + parsed_len);
                x =  ntohl(*(uint32_t*)(buffer_r + parsed_len + 1));
                y =  ntohl(*(uint32_t*)(buffer_r + parsed_len + 5));
                parsed_len += 9;
                break;
            case PLAYER_ELIMINATED:
                player_number = *(uint8_t *)(buffer_r + parsed_len);
                parsed_len += 1;
                break;
        }
        uint32_t crc32 = ntohl(*(uint32_t*)(buffer_r + parsed_len));
        crc32_valid = (crc32 == generate_crc32(buffer_r + event_pos,
                                               event_len + 4));
        return parsed_len;
    }

    void init_gui_server_connection() {
        struct addrinfo addr_hints{};
        struct addrinfo *addr_result;

        memset(&addr_hints, 0, sizeof(struct addrinfo));
        addr_hints.ai_family = AF_INET; // IPv4
        addr_hints.ai_socktype = SOCK_STREAM;
        addr_hints.ai_protocol = IPPROTO_TCP;
        if (getaddrinfo(client_options.gui_server.c_str(),
                        client_options.gui_port.c_str(),
                        &addr_hints, &addr_result) != 0)
            report_fail("Getting gui address info failed!");

        gui_sock = socket(addr_result->ai_family, addr_result->ai_socktype,
                          addr_result->ai_protocol);
        if (gui_sock < 0)
            report_fail("Gui socket initialization failed!");

        if (connect(gui_sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
            report_fail("Gui server connection failed!");
        fcntl(gui_sock , F_SETFL, O_NONBLOCK); // Set socket to nonblock

        freeaddrinfo(addr_result);
    }

    void init_server_connection() {
        struct addrinfo addr_hints{};
        struct addrinfo *addr_result;

        (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
        addr_hints.ai_family = AF_INET; // IPv4
        addr_hints.ai_socktype = SOCK_DGRAM;
        addr_hints.ai_protocol = IPPROTO_UDP;
        addr_hints.ai_flags = 0;
        addr_hints.ai_addrlen = 0;
        addr_hints.ai_addr = nullptr;
        addr_hints.ai_canonname = nullptr;
        addr_hints.ai_next = nullptr;
        if (getaddrinfo(client_options.game_server.c_str(), nullptr,
                        &addr_hints, &addr_result) != 0)
            report_fail("Getting address info failed!");

        srvr_address.sin_family = AF_INET; // IPv4
        srvr_address.sin_addr.s_addr =
                ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr; // address IP
        srvr_address.sin_port = htons(client_options.port_num);

        freeaddrinfo(addr_result);

        sock = socket(PF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            report_fail("Socket initialization failed!");
        fcntl(sock , F_SETFL, O_NONBLOCK); // Set socket to nonblock
    }

    static void report_fail(const char* message) {
        std::cerr << message << std::endl;
        exit(1);
    }



};

#endif
