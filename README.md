# ScreenWorms
Project for networking course at University of Warsaw.

## Running server
./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]

* `-p n` – port (default `2021`)
* `-s n` – random number generator seed (default - result of `time(NULL)`)
* `-t n` – integer signifying speed of turning (default `6`)
* `-v n` – integer signifying speed of movement (default `50`)
* `-w n` – board width in pixels (default `640`)
* `-h n` – board height in pixels (default `480`)

## Running client
./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]

* `game_server` – address (IPv4 or IPv6) or name of game server
* `-n player_name` – player name
* `-p n` – game server port (default `2021`)
* `-i gui_server` – address (IPv4 or IPv6) or name of server handling user interface (default `localhost`)
* `-r n` – port of server handling user interface (default `20210`)

## Client interface
Available under
https://students.mimuw.edu.pl/~zbyszek/sieci/gui/gui2/