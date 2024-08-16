
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
	edit_mode = false;

    uppercase = true;
    CheckMenuItem(*graphics->getMenu(), ID_HEX_UPPERCASE, MF_CHECKED);

    curser.x = 4;
    curser.y = 2;
    cm_pos.x = -1;
    cm_pos.y = -1;

    command_string = "";

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

    // Create command line stuff
    cmd_line_bmp.height = 1;
    cmd_line_bmp.width = colwidth * numcharswidth - 16 * colwidth;  // calc line length
    cmd_line_bmp.pixels = new Uint32[colwidth * numcharswidth * 1];
    for (int j = 0; j != colwidth * numcharswidth * 1; ++j) {
        cmd_line_bmp.pixels[j] = 0xffbfbfbf;
    }
    cmd_line_rect.h = cmd_line_bmp.height;
    cmd_line_rect.w = cmd_line_bmp.width;
    cmd_line_rect.x = colwidth * 1;  // start line at column 1. base 0 columns.
    cmd_line_rect.y = rowheight * 2 + rowheight - 2;

    // Create curser stuff
    curser_bmp.height = 2;
    curser_bmp.width = colwidth;
    curser_bmp.pixels = new Uint32[colwidth * 2];
    for (int j = 0; j != colwidth * 2; ++j) {
        curser_bmp.pixels[j] = 0xffffffff;
    }
    curser_rect.h = curser_bmp.height;
    curser_rect.w = curser_bmp.width;
    curser_rect.x = colwidth * curser.x;
    curser_rect.y = (curser.y + 1) * rowheight - 3;

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
    if (!file_opened) {
        return;
    }
    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->width * sizeof(Uint32));

    // Draw each line
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
    }
    rows_updated = false;

    // Draw any commands
    BMP line;
    graphics->buildString(command_string, line, graphics->MEDIUMFONT);
    SDL_Rect r;
    r.h = line.height;
    r.w = line.width;
    r.x = offset + colwidth * 4;
    r.y = 2 * line.height;
    SDL_UpdateTexture(texture, &r, line.pixels, line.width * sizeof(Uint32));

    // Draw command line
    SDL_UpdateTexture(texture, &cmd_line_rect, cmd_line_bmp.pixels, cmd_line_bmp.width * sizeof(Uint32));

    // Draw Percentage
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f", 100 * ((double)displaypos / filesize));
    std::string percstr(std::string(buffer) + "%");
    BMP percbmp;
    graphics->buildString(percstr, percbmp, graphics->MEDIUMFONT);
    r.h = percbmp.height;
    r.w = percbmp.width;
    r.x = graphics->getWidth() - r.w;
    r.y = graphics->getHeight() - r.h;
    SDL_UpdateTexture(texture, &r, percbmp.pixels, percbmp.width * sizeof(Uint32));

    // Draw curser
	if (edit_mode) {
		curser_rect.x = offset + colwidth * curser.x;
		curser_rect.y = (curser.y + 1) * rowheight - 3;
		SDL_UpdateTexture(texture, &curser_rect, curser_bmp.pixels, curser_bmp.width * sizeof(Uint32));
	}

    // Do we want to draw selection field?
    if (curser_moving) {

    }
}

// Builds an array with strings of each row
void Dump::fillRows() {
    if (!file_opened) {
        return;
    }
    rows_updated = true;
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
        if (uppercase) {
            stream << std::setfill('0') << std::uppercase << std::setw(10) << std::hex << temppos;
        }
        else {
            stream << std::setfill('0') << std::nouppercase << std::setw(10) << std::hex << temppos;
        }
        temp = stream.str();

        temp.append("  ");
        std::streampos off = (temppos - bufferpos) % 0x10;      // Round down in hex
        temp.append(char2hex(&buffer, temppos - bufferpos));    // Convert binary to hex

        temp.append("  |");
        temp.append(subvec(&buffer, temppos - bufferpos));
        temp.append("|");
        temppos -= off;
        rows[x] = temp;
    }

    printRows();
}

// Get a substring for this line. 
std::string Dump::subvec(const std::vector<char> *buf, size_t pos) {
    std::string ret;
    size_t npos = pos % 0x10;
    // Pad in front if position isn't multiple of 0x10
    for (size_t x = 0; x != npos; ++x) {
        ret.push_back(' ');
    }
    for (size_t x = npos; x != 0x10; ++x, ++pos) {
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
    char *b;
    char lower[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    char upper[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    if (uppercase) {
        b = upper;
    }
    else {
        b = lower;
    }
    size_t npos = pos % 0x10;
    // Pad in front if position isn't multiple of 0x10
    for (size_t x = 0; x != npos; ++x) {
        if (x % 4 == 0) {
            ret.push_back(' ');
        }
        ret.push_back(' ');
        ret.push_back(' ');
    }
    for (size_t x = npos; x != 0x10; ++x, ++pos) {
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
			//ret.push_back(' ');
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
                displaypos = fm->getFilesize() - (std::streampos) 1;   // Allow showing the last char only
            }
        }
    }
}

// Update loop
void Dump::update() {
    bool draw_rows = false;
    bool fill_rows = false;

    // Load a file
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

            h = std::string("  Offset       Hex Data                             Character Data");
            rows[2] = h;

            // set parms and print to screen
            getBuffer(0);
            displaypos = 0;
            fill_rows = true;
        }
    }

    //
    //  Check for key presses here
    //
    // Here we check for key press scrolling
    if (input->is_pressed(SDLK_UP)) {
        --curser.y;
        if (curser.y < 0 || !edit_mode) { //startdata) {
            moveDisplayPos(-0x10);
            fill_rows = true;
            curser.y = 0; //startdata;
        }
		else if (input->is_held(SDLK_LALT) || input->is_held(SDLK_RALT)) {
			moveDisplayPos(-0x10);
			fill_rows = true;
			++curser.y;
		}
        draw_rows = true;
        heldtime[SDLK_UP] = Time::getNanoSeconds();
    }
    else if (input->is_held(SDLK_UP)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_UP] > Time::SECOND / 2) {
            --curser.y;
            if (curser.y < 0 || !edit_mode) { //startdata) {
                moveDisplayPos(-0x10);
                fill_rows = true;
                curser.y = 0; //startdata;
            }
			else if (input->is_held(SDLK_LALT) || input->is_held(SDLK_RALT)) {
				moveDisplayPos(-0x10);
				fill_rows = true;
				++curser.y;
			}
            draw_rows = true;
        }
    }

    // Check if left arrow key is pressed
    if (input->is_pressed(SDLK_LEFT)) {
        --curser.x;
        if (curser.x < 0 || !edit_mode) {
            moveDisplayPos(-0x1);
            fill_rows = true;
            curser.x = 0;
        }
		else if (input->is_held(SDLK_LALT) || input->is_held(SDLK_RALT)) {
			moveDisplayPos(-0x1);
			fill_rows = true;
			++curser.x;
		}
        draw_rows = true;
        heldtime[SDLK_LEFT] = Time::getNanoSeconds();
    }
    else if (input->is_held(SDLK_LEFT)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_LEFT] > Time::SECOND / 2) {
            --curser.x;
            if (curser.x < 0 || !edit_mode) {
                moveDisplayPos(-0x1);
                fill_rows = true;
                curser.x = 0;
            }
			else if (input->is_held(SDLK_LALT) || input->is_held(SDLK_RALT)) {
				moveDisplayPos(-0x1);
				fill_rows = true;
				++curser.x;
			}
            draw_rows = true;
        }
    }
    // Check if right arrow key is pressed
    if (input->is_pressed(SDLK_RIGHT)) {
        ++curser.x;
        if (curser.x >= numcharswidth || !edit_mode) {
            moveDisplayPos(0x1);
            fill_rows = true;
            curser.x = numcharswidth - 1;
        }
		else if (input->is_held(SDLK_LALT) || input->is_held(SDLK_RALT)) {
			moveDisplayPos(0x1);
			fill_rows = true;
			--curser.x;
		}
        draw_rows = true;
        heldtime[SDLK_RIGHT] = Time::getNanoSeconds();
    }
    else if (input->is_held(SDLK_RIGHT)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_RIGHT] > Time::SECOND / 2) {
            ++curser.x;
            if (curser.x >= numcharswidth || !edit_mode) {
                moveDisplayPos(0x1);
                fill_rows = true;
                curser.x = numcharswidth - 1;
            }
			else if (input->is_held(SDLK_LALT) || input->is_held(SDLK_RALT)) {
				moveDisplayPos(0x1);
				fill_rows = true;
				--curser.x;
			}
            draw_rows = true;
        }
    }
    // Check if down arrow is pressed
    if (input->is_pressed(SDLK_DOWN)) {
        ++curser.y;
        if (curser.y >= numcharsheight || !edit_mode) {
            moveDisplayPos(0x10);
            fill_rows = true;
            curser.y = numcharsheight - 1;
        }
		else if (input->is_held(SDLK_LALT) || input->is_held(SDLK_RALT)) {
			moveDisplayPos(0x10);
			fill_rows = true;
			--curser.y;
		}
        draw_rows = true;
        heldtime[SDLK_DOWN] = Time::getNanoSeconds();
    }
    else if (input->is_held(SDLK_DOWN)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_DOWN] > Time::SECOND / 2) {
            ++curser.y;
            if (curser.y >= numcharsheight || !edit_mode) {
                moveDisplayPos(0x10);
                fill_rows = true;
                curser.y = numcharsheight - 1;
            }
			else if (input->is_held(SDLK_LALT) || input->is_held(SDLK_RALT)) {
				moveDisplayPos(0x10);
				fill_rows = true;
				--curser.y;
			}
            draw_rows = true;
        }
    }
    // Check if pageup is pressed
    if (input->is_pressed(SDLK_PAGEUP)) {
        heldtime[SDLK_PAGEUP] = Time::getNanoSeconds();
        moveDisplayPos(-(int)(0x10 * (rows.size() - 3)));
        fill_rows = true;
    }
    else if (input->is_held(SDLK_PAGEUP)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_PAGEUP] > Time::SECOND / 2) {
            moveDisplayPos(-(int)(0x10 * (rows.size() - 3)));
            fill_rows = true;
        }
    }
    // Check if pagedown is pressed
    if (input->is_pressed(SDLK_PAGEDOWN)) {
        heldtime[SDLK_PAGEDOWN] = Time::getNanoSeconds();
        moveDisplayPos((int)(0x10 * (rows.size() - 3)));
        fill_rows = true;
    }
    else if (input->is_held(SDLK_PAGEDOWN)) {
        if (Time::getNanoSeconds() - heldtime[SDLK_PAGEDOWN] > Time::SECOND / 2) {
            moveDisplayPos((int)(0x10 * (rows.size() - 3)));
            fill_rows = true;
        }
    }
    // Move curser to home place
    if (input->is_pressed(SDLK_HOME)) {
        displaypos = 0;
        //curser.x = 4;
        //curser.y = 2;
        //draw_rows = true;
		fill_rows = true;
    }
    // Page all the way down
    if (input->is_pressed(SDLK_END)) {
        if (fm != NULL && fm->succeeded()) {
            displaypos = fm->getFilesize();
        }
        fill_rows = true;
    }
    int scroll = input->mouse_scrolled();
    if (scroll != 0 && fm != NULL && fm->succeeded()) {
        if (scroll < 0) {       // We scrolled down
            moveDisplayPos(0x30);
        }
        else {                  // We scrolled up
            moveDisplayPos(-0x30);
        }
        fill_rows = true;
    }
    // A font is chosen
    if (input->bit_selected()) {
        graphics->setFont(graphics->BITFONT);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_BITFONT, MF_CHECKED);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_NATURALFONT, MF_UNCHECKED);
        draw_rows = true;
    }
    else if (input->natural_selected()) {
        graphics->setFont(graphics->NATURALFONT);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_BITFONT, MF_UNCHECKED);
        CheckMenuItem(*graphics->getMenu(), ID_FONT_NATURALFONT, MF_CHECKED);
        draw_rows = true;
    }
    // A case is chosen
    if (input->is_case_selected()) {
        fill_rows = true;
        if (input->is_uppercase()) {
            CheckMenuItem(*graphics->getMenu(), ID_HEX_LOWERCASE, MF_UNCHECKED);
            CheckMenuItem(*graphics->getMenu(), ID_HEX_UPPERCASE, MF_CHECKED);
            uppercase = true;
        }
        else {
            CheckMenuItem(*graphics->getMenu(), ID_HEX_LOWERCASE, MF_CHECKED);
            CheckMenuItem(*graphics->getMenu(), ID_HEX_UPPERCASE, MF_UNCHECKED);
            uppercase = false;
        }
    }
	// Check edit state
	if (input->edit_changed()) {
		draw_rows = true;
		if (input->edit_state() == true) {
			edit_mode = true;
			CheckMenuItem(*graphics->getMenu(), ID_EDIT_ENABLEEDITING, MF_CHECKED);
		}
		else {
			edit_mode = false;
			CheckMenuItem(*graphics->getMenu(), ID_EDIT_ENABLEEDITING, MF_UNCHECKED);
		}
	}
    // Check if we clicked the mouse
    int mx, my;
    if (input->mouse_clicked(&mx, &my)) {
        // We only want to move the curser if left was clicked
        // Currently doesn't work...
        if (input->left_clicked()) {
            mx = mx - offset;
            if (mx < 0) mx = 0;
            mx = mx / colwidth;
            curser.x = mx;

            my = my / rowheight;
            curser.y = my;

            // Make sure curser stays in screen
            if (curser.x >= numcharswidth) {
                curser.x = numcharswidth - 1;
            }
            if (curser.y >= numcharsheight) {
                curser.y = numcharsheight - 1;
            }
            draw_rows = true;
        }
    }

    if (input->is_mouse_held(&mx, &my)) {
        mx = mx - offset;
        if (mx < 0) mx = 0;
        mx = mx / colwidth;
        curser.x = mx;

        my = my / rowheight;
        curser.y = my;

        // Make sure curser stays in screen
        if (curser.x >= numcharswidth) {
            curser.x = numcharswidth - 1;
        }
        if (curser.y >= numcharsheight) {
            curser.y = numcharsheight - 1;
        }
        draw_rows = true;
    }

	if (input->is_pressed(SDLK_KP_ENTER) || input->is_pressed(SDLK_RETURN)) {
		cmd = "";
		fill_rows = true;
	}

    // Check for command typing
    if (!input->is_queue_empty()) {
        cmd += input->get_char();
        fill_rows = true;
    }

    if (fill_rows) {
        fillRows();
    }
    else if (draw_rows) {
        printRows();
    }
    graphics->draw(texture);
}