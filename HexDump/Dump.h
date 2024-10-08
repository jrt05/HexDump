#pragma once

#include <vector>
#include <map>

#include "Graphics.h"
#include "InputHandler.h"
#include "FileManager.h"
#include "Time.h"

class Dump {
public:
    Dump(GFXs *g, InputHandler *h);
    ~Dump();

    void update();
private:
    class Pos {
    public:
        int x, y;
    };

    void Dump::copy(BMP *dst, BMP *src, int x, int y);
    FileManager *fm;

    void getBuffer(std::streampos pos);
    std::streampos bufferpos;  // Location at start of buffer
    std::vector<char> buffer;

    std::string char2hex(const std::vector<char> *buf, size_t pos);
    std::string subvec(const std::vector<char> *buf, size_t pos);
    
    std::string cmd;

    std::streampos filesize;

    void moveDisplayPos(int offset);

    bool file_opened;       // True if we opened a file
    bool edit_mode;			// Is editing enabled?
    bool precise_percentage; // Display precision percentage?

    std::map<long long, Time::nanoseconds> heldtime;

    Time::nanoseconds curserblink;

    std::streampos displaypos;
    void fillRows();
    bool rows_updated;
    void printRows();
    bool uppercase;

    // Command line stuff
    BMP cmd_line_bmp;
    SDL_Rect cmd_line_rect;
    std::string command_string;
    // Curser stuff
    BMP curser_bmp;
    SDL_Rect curser_rect;

    std::vector<std::string> rows;
    const int startdata = 3;
    const int offset = 10;          // #pixels to start x position
    int rowheight;
    int numcharsheight;
    int colwidth;
    int numcharswidth;
    Pos curser;
    Pos cm_pos;         // curser moving position
    bool curser_moving;

    BMP font;
    BMP header;
    void clearScreen();
    BMP *background;
    BMP *screen;
    GFXs *graphics;
    InputHandler *input;
    SDL_Texture *texture;
};