#include "server_options.h"
#include "server_communicator.h"
#include "game_state.h"

void run_server(ServerCommunicator& communicator, const ServerOptions& server_options,
                GameState& game_state) {

    /* Last game ended so all clients need to say that they are ready */
    communicator.set_not_ready();

    while (true) {
        /* Disconnecting clients who haven't sent anything in last 2 seconds/ */
        communicator.remove_inactive_clients();

        /* Parse new message */
        communicator.parse_message(game_state.events, game_state.game_id);

        /* Check if conditions to start game have been fulfilled */
        if (communicator.ready_to_play())
            break;
    }

    /* Game can be started
     * Till this moment all information about previous game was held in game_state
     * and clients could see events from previous game, now they will be deleted */
    if (!game_state.start_game(communicator, server_options.screen_width,
                          server_options.screen_height, server_options.turning_speed)) {
        /* Game ended while initializing worms */
        return;
    }
    uint64_t round_length = 1000000 / server_options.rounds_per_sec,
             round_start = get_time();

    while (true) {
        /* Disconnecting clients who haven't sent anything in last 2 seconds/ */
        communicator.remove_inactive_clients();

        /* Check if round ended */
        if (get_time() - round_start >= round_length) {
            /* Round ended */
            if (!game_state.finish_round(communicator)) {
                /* Game ended in this round */
                return;
            }
            round_start = get_time();
        }

        /* Parse new message */
        communicator.parse_message(game_state.events, game_state.game_id);
    }
}

int main(int argc, char *argv[])
{
    ServerOptions server_options = ServerOptions(argc, argv);
    ServerCommunicator communicator = ServerCommunicator(server_options.port_num);
    GameState game_state = GameState(server_options.seed);

    /* Server runs in infinite loop, every iteration is new game */
    while(true) {
        run_server(communicator, server_options, game_state);

    }
}