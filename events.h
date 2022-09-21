#ifndef PROJEKT2_EVENTS_H
#define PROJEKT2_EVENTS_H

#include <vector>


class Event {

public:

    const uint8_t event_type;
    const uint32_t event_no;
    const uint32_t x{}, y{};
    std::vector<std::string> player_names;
    const uint8_t player_number{};

    explicit Event(uint32_t event_no, uint32_t maxx, uint32_t maxy)
                : event_no(event_no), x(maxx), y(maxy), event_type(NEW_GAME) {}

    explicit Event(uint32_t event_no, uint8_t player_number, uint32_t x, uint32_t y)
                : event_no(event_no), player_number(player_number),
                  x(x), y(y), event_type(PIXEL) {}

    explicit Event(uint32_t event_no, uint8_t player_number)
            : event_no(event_no), player_number(player_number),
              event_type(PLAYER_ELIMINATED) {}

    explicit Event(uint32_t event_no): event_no(event_no), event_type(GAME_OVER) {}

    void add_player(const std::string& player_name) {
        player_names.push_back(player_name);
    }

    uint32_t get_length() const {
        uint32_t length = 0;
        switch (event_type) {
            case NEW_GAME:
                length = 13; // 4 + 1 + 4 + 4
                break;
            case PIXEL:
                length = 14; // 4 + 1 + 1 + 4 + 4
                break;
            case PLAYER_ELIMINATED:
                length = 6; // 4 + 1 + 1
                break;
            case GAME_OVER:
                length = 5; // 4 + 1
                break;
        }
        if (event_type == NEW_GAME) {
            for (const std::string& player_name : player_names)
                length += (player_name.size() + 1);
        }

        return length;
    }

};


#endif //PROJEKT2_EVENTS_H
