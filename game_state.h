#ifndef PROJEKT2_GAME_STATE_H
#define PROJEKT2_GAME_STATE_H

#include <set>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>

#include "utils.h"
#include "server_communicator.h"

class GameState {

private:
    class WormData {

    public:
        double x_pos, y_pos; // Screen position
        uint16_t azimuth; // From 0 to 359
        uint8_t player_number;
        bool alive = true;
        uint8_t turn_direction{};

        explicit WormData(double x_pos, double y_pos,
                          uint16_t azimuth, uint8_t player_number)
                  : x_pos(x_pos), y_pos(y_pos),
                    azimuth(azimuth), player_number(player_number) {}

    };

    std::vector<WormData> worm_data;
    bool pixels[MAX_SCREEN_WIDTH][MAX_SCREEN_HEIGHT]{};
    uint16_t turning_speed{};
    uint32_t maxx{}, maxy{};
    uint8_t alive_worms{};
    uint32_t rand;

public:
    std::vector<Event> events;
    uint32_t game_id{};

    explicit GameState(uint32_t seed): rand(seed) {};

    /*
     * Starts new game. Returns false if game has ended during
     * worm spawning and true if game is still not ended.
     */
    bool start_game(ServerCommunicator& communicator, uint32_t maxx_arg,
                    uint32_t maxy_arg, uint16_t turning_speed_arg) {
        /* Clear previous game data */
        worm_data.clear();
        events.clear();
        for (int x = 0; x < maxx_arg; ++x)
            for (int y = 0; y < maxy_arg; ++y)
                pixels[x][y] = true;

        game_id = next_rand();
        turning_speed = turning_speed_arg;
        maxx = maxx_arg;
        maxy = maxy_arg;
        alive_worms = 0;

        /* Generate new game event */
        events.emplace_back(events.size(), maxx, maxy);
        for (const auto& client : communicator.client_data)
            if (!client.player_name.empty())
                events.back().add_player(client.player_name);

        /* Initialize worms for clients with non-empty names from communicator
         * Clients are already sorted alphabetically so their worms would be too */
        sort(communicator.client_data.begin(), communicator.client_data.end());
        for (auto& client : communicator.client_data) {
            if (client.player_name.empty())
                continue; // This client is an observer

            worm_data.emplace_back((next_rand() % maxx) + 0.5, (next_rand() % maxy) + 0.5,
                                   next_rand() % 360,worm_data.size());
            alive_worms++;
            client.player_number = worm_data.back().player_number;

            if (pixels[(uint32_t)worm_data.back().x_pos]
                      [(uint32_t)worm_data.back().y_pos]) {
                /* Pixel was free, can be eaten */
                events.emplace_back(events.size(),
                                    worm_data.back().player_number,
                                    (uint32_t)worm_data.back().x_pos,
                                    (uint32_t)worm_data.back().y_pos);
                pixels[(uint32_t)worm_data.back().x_pos]
                      [(uint32_t)worm_data.back().y_pos] = false;
            }
            else {
                /* Pixel already eaten, player eliminated */
                events.emplace_back(events.size(), worm_data.back().player_number);
                worm_data.back().alive = false;
                alive_worms--;
            }
        }
        if (alive_worms == 1) {
            /* Only one worm survived */
            events.emplace_back(events.size());
            communicator.send_events_to_everyone(events, 0, game_id);
            return false;
        }
        communicator.send_events_to_everyone(events, 0, game_id);
        return true;
    }

    /*
     * Finishes round. Returns true if game is still rolling and false if
     * GAME_OVER event was generated.
     */
    bool finish_round(ServerCommunicator& communicator) {
        uint32_t first_event_no = events.size();

        for (auto worm : worm_data) {
            if (!worm.alive)
                continue;
            if (alive_worms == 1) {
                /* This worm is the last one standing */
                events.emplace_back(events.size());
                communicator.send_events_to_everyone(events, first_event_no, game_id);
                return false;
            }

            /* Update turn_direction only when client is still connected */
            if (communicator.get_new_turn_direction(worm.player_number) != NO_CHANGES)
                worm.turn_direction = communicator.get_new_turn_direction(
                                            worm.player_number);

            if (worm.turn_direction == LEFT)
                worm.azimuth = (worm.azimuth + turning_speed) % 360;
            else if (worm.turn_direction == RIGHT)
                worm.azimuth = (360 + worm.azimuth - turning_speed) % 360;
            double old_x_pos = worm.x_pos, old_y_pos = worm.y_pos;

            worm.x_pos += cos(worm.azimuth * M_PI / 180);
            worm.y_pos += sin(worm.azimuth * M_PI / 180);

            if ((uint32_t)worm.x_pos != (uint32_t)old_x_pos ||
                (uint32_t)worm.y_pos != (uint32_t)old_y_pos) {

                if (worm.x_pos < 0 || worm.x_pos > maxx - 1 ||
                    worm.y_pos < 0 || worm.y_pos > maxy - 1 ||
                    !pixels[(uint32_t)worm.x_pos][(uint32_t)worm.y_pos]) {

                    /* Pixel already eaten or out of screen, player eliminated */
                    events.emplace_back(events.size(), worm.player_number);
                    worm.alive = false;
                    alive_worms--;
                }
                else {
                    /* Pixel was free, can be eaten */
                    events.emplace_back(events.size(), worm.player_number,
                                        (uint32_t)worm.x_pos, (uint32_t)worm.y_pos);
                    pixels[(uint32_t)worm.x_pos][(uint32_t)worm.y_pos] = false;
                }
            }

        }
        communicator.send_events_to_everyone(events, first_event_no, game_id);
        return true; // Game still rolling
    }

private:
    uint32_t next_rand() {
        uint32_t res = rand;
        rand = ((uint64_t)rand * 279410273) % 4294967291;
        return res;
    }

};


#endif //PROJEKT2_GAME_STATE_H
