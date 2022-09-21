#ifndef PROJEKT2_CONSTS_H
#define PROJEKT2_CONSTS_H

#include <sys/types.h>


/* Default connection values */
const uint16_t DEFAULT_PORT_NUM = 2021;
const char* DEFAULT_GUI_PORT = "20210";
const uint16_t DEFAULT_GUI_PORT_NUM = 20210;
const char *DEFAULT_GUI_SERVER = "localhost";

/* Ports limits */
const uint16_t MAX_PORT_NUM = 65535;

/* Datagram sizes */
const size_t MAX_SERVER_DATAGRAM_SIZE = 550;
const size_t MAX_CLIENT_DATAGRAM_SIZE = 33;

/* Event event_type values */
const uint8_t NEW_GAME = 0;
const uint8_t PIXEL = 1;
const uint8_t PLAYER_ELIMINATED = 2;
const uint8_t GAME_OVER = 3;

/* Default game parameters */
const uint16_t DEFAULT_TURNING_SPEED = 6;
const uint16_t DEFAULT_ROUNDS_PER_SEC = 50;
const uint32_t DEFAULT_SCREEN_WIDTH = 640;
const uint32_t DEFAULT_SCREEN_HEIGHT = 480;

/* Max game parameters */
const uint16_t MAX_TURNING_SPEED = 90;
const uint16_t MAX_ROUNDS_PER_SEC = 250;
const uint32_t MAX_SCREEN_WIDTH = 2048;
const uint32_t MAX_SCREEN_HEIGHT = 2048;
const uint8_t MAX_NAME_LENGTH = 20;

/* Communicators messages */
const uint8_t RECEIVED = 0;
const uint8_t EMPTY = 1;

/* Turning directions */
const uint8_t FORWARD = 0;
const uint8_t RIGHT = 1;
const uint8_t LEFT = 2;
const uint8_t NO_CHANGES = 3; // When client disconnected

/* Clients limits */
const uint8_t CLIENTS_MAX_NUMBER = 25;


#endif //PROJEKT2_CONSTS_H
