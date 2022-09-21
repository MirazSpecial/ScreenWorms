#ifndef PROJEKT2_SERVER_OPTIONS_H
#define PROJEKT2_SERVER_OPTIONS_H

#include <iostream>
#include <unistd.h>
#include <ctime>

#include "consts.h"


class ServerOptions {

public:
    uint16_t port_num = DEFAULT_PORT_NUM;
    uint32_t seed = time(nullptr);
    uint16_t turning_speed = DEFAULT_TURNING_SPEED;
    uint16_t rounds_per_sec = DEFAULT_ROUNDS_PER_SEC;
    uint32_t screen_width = DEFAULT_SCREEN_WIDTH;
    uint32_t screen_height = DEFAULT_SCREEN_HEIGHT;

    ServerOptions(int argc, char* argv[]) {
        int64_t helpy;
        int opt;

        while ((opt = getopt(argc, argv, "p:s:t:v:w:h:")) != -1) {
            switch (opt) {
                case 'p':
                    helpy = strtol(optarg, nullptr, 10);
                    if (helpy < 1 || helpy > MAX_PORT_NUM)
                        fail_constructor("Port number invalid!");
                    port_num = helpy;
                    break;
                case 's':
                    helpy = strtoll(optarg, nullptr, 10);
                    if (helpy < 0)
                        fail_constructor("Seed number invalid!");
                    seed = helpy;
                    break;
                case 't':
                    helpy = strtol(optarg, nullptr, 10);
                    if (helpy <= 0 || helpy > MAX_TURNING_SPEED)
                        fail_constructor("Turning speed invalid!");
                    turning_speed = helpy;
                    break;
                case 'v':
                    helpy = strtol(optarg, nullptr, 10);
                    if (helpy <= 0 || helpy > MAX_ROUNDS_PER_SEC)
                        fail_constructor("Rounds per second invalid!");
                    rounds_per_sec = helpy;
                    break;
                case 'w':
                    helpy = strtol(optarg, nullptr, 10);
                    if (helpy <= 0 || helpy > MAX_SCREEN_WIDTH)
                        fail_constructor("Screen width invalid!");
                    screen_width = helpy;
                    break;
                case 'h':
                    helpy = strtol(optarg, nullptr, 10);
                    if (helpy <= 0 || helpy > MAX_SCREEN_HEIGHT)
                        fail_constructor("Screen width invalid!");
                    screen_height = helpy;
                    break;
                default:
                    fail_constructor("Unrecognized program option!");
            }
        }
        if (argv[optind] != nullptr)
            fail_constructor("Trash in options!");
    }

    static void fail_constructor(const char* message) {
        std::cerr << message << std::endl;
        exit(1);
    }
};

#endif //PROJEKT2_SERVER_OPTIONS_H
