#pragma once

#include <map>
#include <vector>
#include <queue>
#include <SDL_events.h>

struct Point { int x; int y; };

class InputHandler {
private:
    SDL_Event windowEvent;

    bool escape;
    bool close_requested;
    bool load;
    bool menu;
    bool bitfont;
    bool naturalfont;
    bool key_was_pressed;
    bool uppercase;
    bool case_selected;
    bool mouse_was_clicked;
    bool left_mouse_was_clicked;
    bool mouse_held;

    void mouseWheel();
    int mouseY;

    Point mouseclicked;

    std::queue<char> characters;
    std::vector<bool> displayable;
    void build_display();

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
    bool key_updated() {return key_was_pressed;}
    bool is_held(int key);
    bool is_pressed(int key);
    bool load_pressed();
    bool is_case_selected() { return case_selected; }
    bool is_uppercase() { return uppercase; }
    bool menu_selected();
    bool bit_selected();
    bool natural_selected();
    int mouse_scrolled();
    bool mouse_clicked(int *x, int *y);
    bool left_clicked();
    bool is_mouse_held(int *x, int *y);
    bool is_queue_empty() { return characters.empty(); }
    char get_char();
};