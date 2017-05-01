#pragma once

#include "Graphics.h"
#include "InputHandler.h"

class Dump {
public:
    Dump(GFXs *g, InputHandler *h);
    ~Dump();

    void update();
private:
    BMP font;
    void clearScreen();
    BMP *background;
    BMP *screen;
    GFXs *graphics;
    InputHandler *input;
    SDL_Texture *texture;
};