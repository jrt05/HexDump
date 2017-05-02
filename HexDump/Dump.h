#pragma once

#include <vector>

#include "Graphics.h"
#include "InputHandler.h"
#include "FileManager.h"

class Dump {
public:
    Dump(GFXs *g, InputHandler *h);
    ~Dump();

    void update();
private:
    void Dump::copy(BMP *dst, BMP *src, int x, int y);
    FileManager *fm;

    void getBuffer(std::streampos pos);
    std::streampos bufferpos;  // Location at start of buffer
    std::vector<char> buffer;

    std::string char2hex(const std::vector<char> *buf, size_t pos);
    std::string subvec(const std::vector<char> *buf, size_t pos);
    
    std::streampos filesize;

    std::streampos displaypos;
    void fillRows();

    BMP font;
    BMP header;
    void clearScreen();
    BMP *background;
    BMP *screen;
    GFXs *graphics;
    InputHandler *input;
    SDL_Texture *texture;
};