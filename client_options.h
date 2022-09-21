#ifndef PROJEKT2_CLIENT_OPTIONS_H
#define PROJEKT2_CLIENT_OPTIONS_H

#include <iostream>
#include <unistd.h>
#include <algorithm>

#include "consts.h"

class ClientOptions {

public:
    std::string game_server;
    std::string player_name{};
    uint16_t port_num = DEFAULT_PORT_NUM;
    std::string gui_server = DEFAULT_GUI_SERVER;
    std::string gui_port = DEFAULT_GUI_PORT;
    uint16_t gui_port_num = DEFAULT_GUI_PORT_NUM;

    ClientOptions(int argc, char *argv[]) {
        if (argc < 2)
            fail_constructor("Game server not provided!");
        game_server = argv[1];

        int opt;
        argc -= 1;
        argv++;

        while ((opt = getopt(argc, argv, "n:p:i:r:")) != -1) {
            switch (opt) {
                case 'n':
                    player_name = optarg;
                    if (!is_name_valid())
                        fail_constructor("Name invalid!");
                    break;
                case 'p':
                    port_num = strtol(optarg, nullptr, 10);
                    if (port_num < 1 || port_num > MAX_PORT_NUM)
                        fail_constructor("Port number invalid!");
                    break;
                case 'i':
                    gui_server = optarg;
                    break;
                case 'r':
                    gui_port = optarg;
                    gui_port_num = strtol(optarg, nullptr, 10);
                    if (gui_port_num < 1 || gui_port_num > MAX_PORT_NUM)
                        fail_constructor("GUI port number invalid!");
                    break;
                default:
                    fail_constructor("Unrecognized program option!");
            }
        }
        if (argv[optind] != nullptr)
            fail_constructor("Trash in options!");
    }

    static void fail_constructor(const char *message) {
        std::cerr << message << std::endl;
        exit(1);
    }

    bool is_name_valid() const {
        if (player_name.size() > MAX_NAME_LENGTH)
            return false;
        return std::all_of(player_name.begin(), player_name.end(),[](char c) {
            return c >= '!' && c <= '~'; });
    }

};

#endif //PROJEKT2_CLIENT_OPTIONS_H
