
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

    curser.x = 2;
    curser.y = 3;

    rows.clear();
    int r = graphics->getHeight();
    colwidth = graphics->getFontWidth() / 16;
    rowheight = graphics->getFontHeight() / 16;
    numcharswidth = (graphics->getWidth() - offset) / colwidth;
    numcharsheight = graphics->getHeight() / rowheight;
    for (int x = 0; x != r / rowheight; ++x) {
        rows.push_back(std::string(""));
    }

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

void Dump::printRows() {
    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->width * sizeof(Uint32));
    for (int x = 0; x != rows.size(); ++x) {
        BMP line;
        graphics->buildString(rows[x], line, graphics->MEDIUMFONT);
        //copy(screen, &line, offset, 00 + x * line.height);
        SDL_Rect r;
        r.h = line.height;
        r.w = line.width;
        r.x = offset;
        r.y = x * line.height;
        SDL_UpdateTexture(texture, &r, line.pixels, line.width * sizeof(Uint32));
        if (x == curser.y) {
            BMP us;
            us.height = 2;
            us.width = colwidth;
            us.pixels = new Uint32[colwidth * 2];
            for (int j = 0; j != colwidth * 2; ++j) {
                us.pixels[j] = 0xffffffff;
            }
            SDL_Rect r;
            r.h = us.height;
            r.w = us.width;
            r.x = offset + colwidth * curser.x;
            r.y = (x + 1) * rowheight - 2;
            SDL_UpdateTexture(texture, &r, us.pixels, us.width * sizeof(Uint32));
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
    for (int x = startdata; x != rows.size(); ++x, temppos += 0x10) {
        if (temppos >= filesize) {          // Our position is out of file, so don't print anything
            rows[x] = "";
            continue;
        }
        std::stringstream stream;
        std::string temp;
        stream << std::setfill('0') << std::uppercase << std::setw(10) << std::hex << temppos;
        temp = stream.str();

        temp.append("    ");

        temp.append(char2hex(&buffer, temppos - bufferpos));

        temp.append("    |");
        temp.append(subvec(&buffer, temppos - bufferpos));
        temp.append("|");

        rows[x] = temp;
    }

    printRows();
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

void Dump::moveDisplayPos(int offset) {
    if (fm != NULL && fm->succeeded()) {
        if (offset < 0) {
            if (displaypos >= -offset) {
                displaypos += offset;
            }
            else {
                displaypos = 0;
            }
        }
        else {
            displaypos += offset;
            if (displaypos >= fm->getFilesize()) {
                displaypos = fm->getFilesize();
            }
        }
    }
}

void Dump::update() {
    if (input->load_pressed()) {
        if (fm != NULL) {
            delete fm;
            fm = NULL;
        }
        fm = new FileManager();
        if (fm->succeeded()) {          // File is opened!
            file_opened = true;
            filesize = fm->getFilesize();
            clearScreen();

            // Create header
            std::string h("File: ");
            h.append(fm->getfilename());
            rows[0] = h;

            h = std::string("  Size - Dec: ");
            h.append(std::to_string(fm->getFilesize()));
            h.append("  Hex: ");

            std::stringstream stream;

            stream << std::uppercase << std::hex << fm->getFilesize();
            h.append(stream.str());
            rows[1] = h;

            // set parms and print to screen
            getBuffer(0);
            displaypos = 0;
            fillRows();
        }
    }

    // Here we check for key press scrolling
    if (input->is_pressed(SDLK_UP)) {
        --curser.y;
        if (curser.y < startdata) {
            curser.y = startdata;
            moveDisplayPos(-0x10);
        }
        fillRows();
        heldtime[SDLK_UP] = Time::getNanoSeconds();
    }
    else if (input->is_held(SDLK_UP)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_UP] > Time::SECOND / 2) {
            --curser.y;
            if (curser.y < startdata) {
                curser.y = startdata;
                moveDisplayPos(-0x10);
            }
            fillRows();
        }
    }

    if (input->is_pressed(SDLK_LEFT)) {
        --curser.x;
        if (curser.x < 0) {
            curser.x = 0;
        }
        fillRows();
        heldtime[SDLK_LEFT] = Time::getNanoSeconds();
    }
    else if (input->is_held(SDLK_LEFT)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_LEFT] > Time::SECOND / 2) {
            --curser.x;
            if (curser.x < 0) {
                curser.x = 0;
            }
            fillRows();
        }
    }
    if (input->is_pressed(SDLK_RIGHT)) {
        ++curser.x;
        if (curser.x >= numcharswidth) {
            curser.x = numcharswidth - 1;
        }
        fillRows();
        heldtime[SDLK_RIGHT] = Time::getNanoSeconds();
    }
    else if (input->is_held(SDLK_RIGHT)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_RIGHT] > Time::SECOND / 2) {
            ++curser.x;
            if (curser.x >= numcharswidth) {
                curser.x = numcharswidth - 1;
            }
            fillRows();
        }
    }
    if (input->is_pressed(SDLK_DOWN)) {
        ++curser.y;
        if (curser.y >= numcharsheight) {
            curser.y = numcharsheight - 1;
            moveDisplayPos(0x10);
        }
        fillRows();
        heldtime[SDLK_DOWN] = Time::getNanoSeconds();
    }
    else if (input->is_held(SDLK_DOWN)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_DOWN] > Time::SECOND / 2) {
            ++curser.y;
            if (curser.y >= numcharsheight) {
                curser.y = numcharsheight - 1;
                moveDisplayPos(0x10);
            }
            fillRows();
        }
    }

    if (input->is_pressed(SDLK_PAGEUP)) {
        heldtime[SDLK_PAGEUP] = Time::getNanoSeconds();
        moveDisplayPos(-(int)(0x10 * (rows.size() - 3)));
        fillRows();
    }
    else if (input->is_held(SDLK_PAGEUP)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_PAGEUP] > Time::SECOND / 2) {
            moveDisplayPos(-(int)(0x10 * (rows.size() - 3)));
            fillRows();
        }
    }

    if (input->is_pressed(SDLK_PAGEDOWN)) {
        heldtime[SDLK_PAGEDOWN] = Time::getNanoSeconds();
        moveDisplayPos((int)(0x10 * (rows.size() - 3)));
        fillRows();
    }
    else if (input->is_held(SDLK_PAGEDOWN)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_PAGEDOWN] > Time::SECOND / 2) {
            moveDisplayPos((int)(0x10 * (rows.size() - 3)));
            fillRows();
        }
    }

    if (input->is_pressed(SDLK_HOME)) {
        displaypos = 0;
        fillRows();
    }
    if (input->is_pressed(SDLK_END)) {
        if (fm != NULL && fm->succeeded()) {
            displaypos = fm->getFilesize();
        }
        fillRows();
    }
    int scroll = input->mouse_scrolled();
    if (scroll != 0 && fm != NULL && fm->succeeded()) {
        if (scroll < 0) {       // We scrolled down
            moveDisplayPos(0x30);
        }
        else {                  // We scrolled up
            moveDisplayPos(-0x30);
        }
        fillRows();
    }
    // A font is chosen
    if (input->bit_selected()) {
        graphics->setFont(graphics->BITFONT);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_BITFONT, MF_CHECKED);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_NATURALFONT, MF_UNCHECKED);
        fillRows();
    }
    else if (input->natural_selected()) {
        graphics->setFont(graphics->NATURALFONT);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_BITFONT, MF_UNCHECKED);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_NATURALFONT, MF_CHECKED);
        fillRows();
    }
    //fillRows();
    graphics->draw(texture);
}