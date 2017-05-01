
#include "Dump.h"

Dump::Dump(GFXs *g, InputHandler *h) {
    graphics = g;
    input = h;

    texture = graphics->getTexture(graphics->SCREEN_WIDTH / 2, graphics->SCREEN_HEIGHT / 2);

    // Create screen with backup to clear with
    background = new BMP(graphics->SCREEN_WIDTH, graphics->SCREEN_HEIGHT);
    screen = new BMP(graphics->SCREEN_WIDTH, graphics->SCREEN_HEIGHT);

    for (size_t x = 0; x != background->width * background->height; ++x) {
        background->pixels[x] = graphics->BACKGROUND;
    }

    clearScreen();

    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->width * sizeof(Uint32));  // Copy pixels on to window
    graphics->buildString("Welcome to HexDump v0.00.01", font);
    graphics->buildString("00000010    683e0d0a 23696e63 6c756465 203c6374    |h>..#include <ct|", font);
    SDL_Rect r;
    r.h = font.height;
    r.w = font.width;
    r.x = 10;
    r.y = 10;
    SDL_UpdateTexture(texture, &r, font.pixels, font.width * sizeof(Uint32));
    graphics->buildString("00000020    7970652e 683e0d0a 23696e63 6c756465    |ype.h>..#include|", font);
    r.x = 10;
    r.y += font.height;
    SDL_UpdateTexture(texture, &r, font.pixels, font.width * sizeof(Uint32));
}

Dump::~Dump() {
    SDL_DestroyTexture(texture);
}

void Dump::clearScreen() {
    memcpy(screen->pixels, background->pixels, background->width * background->height * sizeof(Uint32));
}

void Dump::update() {
    graphics->draw(texture);
}