#pragma once

#include <map>
#include <vector>

#include <SDL_events.h>

class InputHandler {
private:
    SDL_Event windowEvent;

    bool escape;
    bool close_requested;
    bool load;
    bool menu;

    std::map<int, bool> keys;
    std::map<int, bool> prevkeys;
    std::map<int, bool> winEvents;
    std::map<int, bool> prevWinEvents;

    void keydown(SDL_Event &event);
    void keyup(SDL_Event &event);

public:
    InputHandler();
    ~InputHandler();
    void update();
    bool quit() { return close_requested; }
    bool is_held(int key);
    bool is_pressed(int key);
    bool load_pressed();
    bool menu_selected();
};