
#include "InputHandler.h"
#include "Logger.h"
#include "resource.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_syswm.h>

InputHandler::InputHandler() {
    key_was_pressed = false;
    escape = false;
    close_requested = false;
    load = false;
    menu = false;
    mouseY = 0;
    uppercase = true;
    bitfont = naturalfont = false;
    mouse_was_clicked = false;
    left_mouse_was_clicked = false;
    mouse_held = false;
    case_selected = false;
    displayable.clear();
    displayable.resize(256, false);
    build_display();
}

InputHandler::~InputHandler() {

}

void InputHandler::build_display() {
    std::string d = "abcdefghijklmnopqrstuvwxyz0123456789`[]\\;',./-=";
    // Set all displayable characters to true in displayable vector
    for (size_t x = 0; x != d.size(); ++x) {
        displayable[d[x]] = true;
    }
}

char InputHandler::get_char() {
    char ret = characters.front();
    characters.pop();
    return ret;
}

bool InputHandler::is_pressed(int key) {
    return (keys[key] && !prevkeys[key]);
}

bool InputHandler::left_clicked() {
    bool ret = left_mouse_was_clicked;
    left_mouse_was_clicked = false;
    return ret;
}
bool InputHandler::is_mouse_held(int *x, int *y) {
    if (!mouse_held) return false;
    int ix, iy;
    SDL_GetMouseState(&ix, &iy);
    *x = ix;
    *y = iy;
    return true;
}

bool InputHandler::mouse_clicked(int *x, int *y) {
    if (!mouse_was_clicked) return false;
    *x = mouseclicked.x;
    *y = mouseclicked.y;
    mouse_was_clicked = false;
    return true;
}

bool InputHandler::is_held(int key) {
    return keys[key];
}

bool InputHandler::menu_selected() {
    bool ret = menu;
    menu = false;
    return ret;
}
bool InputHandler::bit_selected() {
    bool ret = bitfont;
    bitfont = false;
    return ret;
}
bool InputHandler::natural_selected() {
    bool ret = naturalfont;
    naturalfont = false;
    return ret;
}
bool InputHandler::load_pressed() {
    bool ret = load;
    load = false;
    return ret;
}
int InputHandler::mouse_scrolled() {
    int ret = mouseY;
    mouseY = 0;
    return ret;
}

void InputHandler::keydown(SDL_Event &event) {
    keys[event.key.keysym.sym] = true;

    // Test for capslock
    //int temp = SDL_GetModState();
    //temp = temp & KMOD_CAPS;
    //if (temp == KMOD_CAPS)
    //    std::cout << "CAP ON";

    if (event.key.keysym.sym < 256) {
        if (displayable[event.key.keysym.sym]) {
            characters.push(event.key.keysym.sym);
        }
    }
}
void InputHandler::keyup(SDL_Event &event) {
    keys[event.key.keysym.sym] = false;
}
void InputHandler::mouseWheel() {
    mouseY = windowEvent.wheel.y;
}
void InputHandler::update() {
    prevkeys = keys;
    key_was_pressed = false;

    while (SDL_PollEvent(&windowEvent) != 0) {
        switch (windowEvent.type) {

        case SDL_SYSWMEVENT:
            switch (windowEvent.syswm.msg->msg.win.msg) {
            case WM_COMMAND:
                menu = true;            // Let others know a menu item was selected
                int messageId;
                int wmEvent;
                messageId = LOWORD(windowEvent.syswm.msg->msg.win.wParam);
                wmEvent = HIWORD(windowEvent.syswm.msg->msg.win.wParam);

                switch (messageId) {
                    // We want to quit
                case ID_FILE_EXIT:
                    close_requested = true;
                    key_was_pressed = true;
                    break;
                case ID_FILE_LOAD:
                    key_was_pressed = true;
                    load = true;
                    break;
                case ID_FONT_BITFONT:
                    key_was_pressed = true;
                    bitfont = true;
                    break;
                case ID_FONT_NATURALFONT:
                    key_was_pressed = true;
                    naturalfont = true;
                    break;
                case ID_HEX_LOWERCASE:
                    key_was_pressed = true;
                    case_selected = true;
                    uppercase = false;
                    break;
                case ID_HEX_UPPERCASE:
                    key_was_pressed = true;
                    case_selected = true;
                    uppercase = true;
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
            break;
            // Check if we want to quit
        case SDL_QUIT:
            key_was_pressed = true;
            close_requested = true;
            break;
        case SDL_MOUSEWHEEL:
            key_was_pressed = true;
            mouseWheel();
            break;
        case SDL_MOUSEBUTTONDOWN:
            SDL_GetMouseState(&mouseclicked.x, &mouseclicked.y);
            mouse_was_clicked = true;
            mouse_held = true;
            if (SDL_BUTTON(SDL_BUTTON_LEFT)) {
                left_mouse_was_clicked = true;
            }
            else {
                left_mouse_was_clicked = false;
            }
            key_was_pressed = true;
            break;
        case SDL_MOUSEBUTTONUP:
            mouse_held = false;
            key_was_pressed = true;
            break;
        case SDL_KEYDOWN:
            key_was_pressed = true;
            keydown(windowEvent);
            break;
        case SDL_KEYUP:
            key_was_pressed = true;
            keyup(windowEvent);
            break;
        default:
            break;
        }
    }
}