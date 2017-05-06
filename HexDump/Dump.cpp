
#include <string>
#include <atlbase.h>
#include <iomanip>
#include <sstream>

#include "Dump.h"
#include "FileManager.h"
#include "Time.h"
#include "Logger.h"
#include "resource.h"

Dump::Dump(GFXs *g, InputHandler *h) {
    graphics = g;
    input = h;

    bufferpos = 0;
    displaypos = 0;
    filesize = 0;
    file_opened = false;

    texture = graphics->getTexture(graphics->getWidth(), graphics->getHeight());
    //texture = graphics->getTexture(graphics->SCREEN_WIDTH, graphics->SCREEN_HEIGHT);

    // Create screen with backup to clear with
    background = new BMP(graphics->getWidth(), graphics->getHeight());
    screen = new BMP(graphics->getWidth(), graphics->getHeight());

    for (size_t x = 0; x != background->width * background->height; ++x) {
        background->pixels[x] = graphics->BACKGROUND;
        background->pixels[x] = 0xff000000;
    }

    clearScreen();

    graphics->setFont(graphics->BITFONT);
    CheckMenuItem(*graphics->getMenu(), ID_FONT_BITFONT, MF_CHECKED);

    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->width * sizeof(Uint32));  // Copy pixels on to window
}

Dump::~Dump() {
    if (fm != NULL) {
        delete fm;
    }
    SDL_DestroyTexture(texture);
}

void Dump::copy(BMP *dst, BMP *src, int xpos, int ypos) {
    if (xpos < 0 || ypos < 0) {
        return;
    }
    for (int y = 0; (y + ypos) < dst->height && y < src->height; ++y) {
        for (int x = 0; (x + xpos) < dst->width && x < src->width; ++x) {
            dst->pixels[xpos + x + ((ypos + y) * dst->width)] = src->pixels[x + y * src->width];
        }
    }
}

void Dump::fillRows() {
    if (!file_opened) {
        return;
    }

    // Want to display an address that is out of range
    if (displaypos >= filesize) {
        displaypos = filesize - (filesize % std::streampos(0x10));
        if (filesize % std::streampos(0x10) == 0 && filesize != std::streampos(0)) {
            displaypos -= 0x10;
        }
    }

    // We currently don't have the file offset, fetch 1mb before and 1mb after desired location
    if (displaypos < bufferpos) {
        std::streampos newpos;
        if (displaypos <= 1024 * 1024) {
            newpos = 0;
        }
        else {
            newpos = displaypos - std::streampos(1024 * 1024);
        }
        getBuffer(newpos);
    }
    //else if (displaypos >= bufferpos + std::streamsize(buffer.size())) {
    else if (displaypos >= bufferpos + std::streampos(fm->CHUNKSIZE - 4096)) {      // Get a new chunk when we're 4k away from the end
        std::streampos newpos;
        if (displaypos <= 1024 * 1024) {
            newpos = 0;
        }
        else {
            newpos = displaypos - std::streampos(1024 * 1024);
        }
        getBuffer(newpos);
    }

    std::streampos temppos = displaypos;
    for (int x = 0; x != 37 && temppos < filesize; ++x) {
        std::stringstream stream;
        std::string temp;
        stream << std::setfill('0') << std::uppercase << std::setw(10) << std::hex << temppos;
        temp = stream.str();

        temp.append("    ");

        temp.append(char2hex(&buffer, temppos - bufferpos));

        temp.append("    |");
        temp.append(subvec(&buffer, temppos - bufferpos));
        temp.append("|");

        BMP line;
        graphics->buildString(temp, line, graphics->MEDIUMFONT);
        copy(screen, &line, 10, 40 + x * line.height);
        temppos += 0x10;
    }

    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->width * sizeof(Uint32));  // Copy pixels on to window
}

std::string Dump::subvec(const std::vector<char> *buf, size_t pos) {
    std::string ret;
    for (size_t x = 0; x != 0x10; ++x, ++pos) {
        if (pos >= buf->size()) {
            ret.push_back(' ');
        }
        else {
            ret.push_back((*buf)[pos]);
        }
    }

    return ret;
}

std::string Dump::char2hex(const std::vector<char> *buf, size_t pos) {
    std::string ret;
    //char b[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    char b[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    for (size_t x = 0; x != 0x10; ++x, ++pos) {
        if (x % 4 == 0) {
            ret.push_back(' ');
        }
        if (pos >= buf->size()) {
            ret.push_back(' ');
            ret.push_back(' ');
        }
        else {
            ret.push_back(b[((*buf)[pos] >> 4) & 0x0f]);
            ret.push_back(b[(*buf)[pos] & 0x0f]);
        }
    }

    return ret;
}

void Dump::getBuffer(std::streampos pos) {
    Logger::LogOut.logstream << Time::getTimeString() << " Recieved new chunk at pos " << pos << std::endl;
    buffer = fm->getChunk(pos);
    bufferpos = pos;
}

void Dump::clearScreen() {
    memcpy(screen->pixels, background->pixels, background->width * background->height * sizeof(Uint32));
}

void Dump::update() {
    if (input->load_pressed()) {
        if (fm != NULL) {
            delete fm;
            fm = NULL;
        }
        fm = new FileManager();
        if (fm->succeeded()) {
            file_opened = true;
            filesize = fm->getFilesize();
            clearScreen();
            std::string h("File: ");
            h.append(fm->getfilename());
            h.append("    Size - Dec: ");
            h.append(std::to_string(fm->getFilesize()));
            h.append("  Hex: ");

            std::stringstream stream;
            std::string temp;
            stream << std::uppercase << std::hex << fm->getFilesize();
            h.append(stream.str());

            h.append(std::string(100, ' '));

            graphics->buildString(h, header, graphics->MEDIUMFONT);
            copy(screen, &header, 10, 5);
            SDL_UpdateTexture(texture, NULL, screen->pixels, screen->width * sizeof(Uint32));  // Copy pixels on to window

            getBuffer(0);
            displaypos = 0;
            fillRows();
        }
    }
    int scroll = input->mouse_scrolled();
    if (scroll != 0 && fm != NULL && fm->succeeded()) {
        if (scroll < 0) {       // We scrolled down
            displaypos += 0x30;
            if (displaypos >= fm->getFilesize()) {
                displaypos = fm->getFilesize();
            }
        }
        else {                  // We scrolled up
            if (displaypos >= 0x30) {
                displaypos -= 0x30;
            }
            else {
                displaypos = 0;
            }
        }
        clearScreen();
        copy(screen, &header, 10, 5);
        fillRows();
    }
    // A font is chosen
    if (input->bit_selected()) {
        graphics->setFont(graphics->BITFONT);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_BITFONT, MF_CHECKED);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_NATURALFONT, MF_UNCHECKED);
        fillRows();
    }
    if (input->natural_selected()) {
        graphics->setFont(graphics->NATURALFONT);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_BITFONT, MF_UNCHECKED);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_NATURALFONT, MF_CHECKED);
        fillRows();
    }
    graphics->draw(texture);
}