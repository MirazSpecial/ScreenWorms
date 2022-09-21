#include "client_options.h"
#include "client_communicator.h"
#include "utils.h"


[[noreturn]] void run_client(ClientCommunicator& communicator) {
    uint64_t moment_length = 30000,
             moment_start = get_time();

    while (true) {
        if (get_time() - moment_start >= moment_length) {
            /* Send routine message */
            communicator.message_server();
            moment_start = get_time();
        }

        /* If server sent anything */
        communicator.parse_message();

        /* If gui sever sent anything */
        communicator.parse_gui_message();

    }

}

int main(int argc, char *argv[])
{
    uint64_t session_id = get_time();
    ClientOptions client_options = ClientOptions(argc, argv);
    ClientCommunicator communicator =
            ClientCommunicator(client_options, session_id);

    run_client(communicator);
}